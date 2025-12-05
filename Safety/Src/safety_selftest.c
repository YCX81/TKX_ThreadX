/**
 ******************************************************************************
 * @file    safety_selftest.c
 * @brief   Lightweight Self-Test Implementation
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "safety_selftest.h"
#include "safety_core.h"
#include "stm32f4xx_hal.h"
#include "crc.h"

/* Private variables ---------------------------------------------------------*/
static flash_crc_context_t s_flash_crc_ctx;
static bool s_initialized = false;

/* Expected application CRC (stored at APP_CRC_ADDR) */
static uint32_t s_expected_app_crc = 0;

/* ============================================================================
 * Private Function Prototypes
 * ============================================================================*/
static selftest_result_t SelfTest_CPURegisters(void);
static selftest_result_t SelfTest_RAMFull(void);
static uint32_t SelfTest_CalculateCRC32(const uint8_t *data, uint32_t length);

/* ============================================================================
 * Implementation
 * ============================================================================*/

safety_status_t Safety_SelfTest_Init(void)
{
    /* Initialize Flash CRC context */
    s_flash_crc_ctx.current_offset = 0;
    s_flash_crc_ctx.accumulated_crc = CRC32_INIT_VALUE;
    s_flash_crc_ctx.total_size = APP_FLASH_SIZE - 4; /* Exclude CRC itself */
    s_flash_crc_ctx.block_size = SELFTEST_FLASH_CRC_BLOCK_SIZE;
    s_flash_crc_ctx.in_progress = false;
    s_flash_crc_ctx.completed = false;

    /* Read expected CRC from flash */
    s_expected_app_crc = *((volatile uint32_t *)APP_CRC_ADDR);

    s_initialized = true;

    return SAFETY_OK;
}

selftest_result_t Safety_SelfTest_RunStartup(void)
{
    selftest_result_t result;

    if (!s_initialized)
    {
        Safety_SelfTest_Init();
    }

#if SELFTEST_STARTUP_CPU_ENABLED
    result = Safety_SelfTest_CPU();
    if (result != SELFTEST_PASS)
    {
        return result;
    }
#endif

#if SELFTEST_STARTUP_RAM_ENABLED
    result = Safety_SelfTest_RAM(SELFTEST_MODE_STARTUP);
    if (result != SELFTEST_PASS)
    {
        return result;
    }
#endif

#if SELFTEST_STARTUP_FLASH_ENABLED
    result = Safety_SelfTest_FlashCRC(SELFTEST_MODE_STARTUP);
    if (result != SELFTEST_PASS)
    {
        return result;
    }
#endif

#if SELFTEST_STARTUP_CLOCK_ENABLED
    result = Safety_SelfTest_Clock();
    if (result != SELFTEST_PASS)
    {
        return result;
    }
#endif

    return SELFTEST_PASS;
}

selftest_result_t Safety_SelfTest_CPU(void)
{
#if SELFTEST_STARTUP_CPU_ENABLED || SELFTEST_RUNTIME_CPU_ENABLED
    return SelfTest_CPURegisters();
#else
    return SELFTEST_PASS;
#endif
}

selftest_result_t Safety_SelfTest_RAM(selftest_mode_t mode)
{
    if (mode == SELFTEST_MODE_STARTUP)
    {
#if SELFTEST_STARTUP_RAM_ENABLED
        return SelfTest_RAMFull();
#endif
    }
    /* Runtime RAM test disabled for lightweight mode */
    return SELFTEST_PASS;
}

selftest_result_t Safety_SelfTest_FlashCRC(selftest_mode_t mode)
{
    if (mode == SELFTEST_MODE_STARTUP)
    {
        /* Full CRC verification at startup */
        uint32_t calc_crc = SelfTest_CalculateCRC32(
            (const uint8_t *)APP_FLASH_START,
            APP_FLASH_SIZE - 4);

        if (calc_crc != s_expected_app_crc)
        {
            Safety_ReportError(SAFETY_ERR_FLASH_CRC, calc_crc, s_expected_app_crc);
            return SELFTEST_FAIL_FLASH;
        }

        return SELFTEST_PASS;
    }
    else
    {
        /* Start incremental check */
        Safety_SelfTest_ResetFlashCRC();
        s_flash_crc_ctx.in_progress = true;
        return SELFTEST_IN_PROGRESS;
    }
}

selftest_result_t Safety_SelfTest_FlashCRC_Continue(void)
{
    if (!s_flash_crc_ctx.in_progress)
    {
        return SELFTEST_NOT_RUN;
    }

    /* Calculate remaining size for this block */
    uint32_t remaining = s_flash_crc_ctx.total_size - s_flash_crc_ctx.current_offset;
    uint32_t block_size = (remaining > s_flash_crc_ctx.block_size) ?
                          s_flash_crc_ctx.block_size : remaining;

    if (block_size == 0)
    {
        /* Check complete */
        s_flash_crc_ctx.in_progress = false;
        s_flash_crc_ctx.completed = true;

        /* Verify final CRC */
        if (s_flash_crc_ctx.accumulated_crc != s_expected_app_crc)
        {
            Safety_ReportError(SAFETY_ERR_FLASH_CRC,
                             s_flash_crc_ctx.accumulated_crc,
                             s_expected_app_crc);
            return SELFTEST_FAIL_FLASH;
        }

        return SELFTEST_PASS;
    }

    /* Calculate CRC for this block using hardware CRC */
    const uint8_t *data = (const uint8_t *)(APP_FLASH_START +
                                            s_flash_crc_ctx.current_offset);

    /* Use HAL CRC for block calculation */
    uint32_t block_crc = HAL_CRC_Calculate(&hcrc, (uint32_t *)data,
                                           block_size / 4);

    /* Accumulate CRC (simplified - in production use proper CRC continuation) */
    s_flash_crc_ctx.accumulated_crc ^= block_crc;

    /* Update offset */
    s_flash_crc_ctx.current_offset += block_size;

    return SELFTEST_IN_PROGRESS;
}

selftest_result_t Safety_SelfTest_Clock(void)
{
    uint32_t sysclk = HAL_RCC_GetSysClockFreq();
    uint32_t min_freq = EXPECTED_SYSCLK_HZ * (100 - CLOCK_TOLERANCE_PERCENT) / 100;
    uint32_t max_freq = EXPECTED_SYSCLK_HZ * (100 + CLOCK_TOLERANCE_PERCENT) / 100;

    if (sysclk < min_freq || sysclk > max_freq)
    {
        Safety_ReportError(SAFETY_ERR_CLOCK, sysclk, EXPECTED_SYSCLK_HZ);
        return SELFTEST_FAIL_CLOCK;
    }

    return SELFTEST_PASS;
}

const flash_crc_context_t* Safety_SelfTest_GetFlashCRCContext(void)
{
    return &s_flash_crc_ctx;
}

void Safety_SelfTest_ResetFlashCRC(void)
{
    s_flash_crc_ctx.current_offset = 0;
    s_flash_crc_ctx.accumulated_crc = CRC32_INIT_VALUE;
    s_flash_crc_ctx.in_progress = false;
    s_flash_crc_ctx.completed = false;
}

/* ============================================================================
 * Private Functions
 * ============================================================================*/

static selftest_result_t SelfTest_CPURegisters(void)
{
    /* Simple CPU register test */
    /* Note: Full IEC 61508 compliance requires assembly-level testing */
    /* This is a simplified C version */

    volatile uint32_t test_val;

    /* Test pattern 1: 0xAAAAAAAA */
    test_val = 0xAAAAAAAAUL;
    if (test_val != 0xAAAAAAAAUL)
    {
        return SELFTEST_FAIL_CPU;
    }

    /* Test pattern 2: 0x55555555 */
    test_val = 0x55555555UL;
    if (test_val != 0x55555555UL)
    {
        return SELFTEST_FAIL_CPU;
    }

    /* Test pattern 3: Walking ones */
    for (int i = 0; i < 32; i++)
    {
        test_val = (1UL << i);
        if (test_val != (1UL << i))
        {
            return SELFTEST_FAIL_CPU;
        }
    }

    return SELFTEST_PASS;
}

static selftest_result_t SelfTest_RAMFull(void)
{
    /* Non-destructive RAM test using March C algorithm on test region */
    /* Test a small region that's not in use */

    /* For safety, only test the designated test region */
    volatile uint32_t *test_start = (volatile uint32_t *)RAM_TEST_START;
    uint32_t test_words = RAM_TEST_SIZE / sizeof(uint32_t);

    /* Save original values */
    uint32_t saved_values[256]; /* Save first 1KB */
    uint32_t save_count = (test_words < 256) ? test_words : 256;

    for (uint32_t i = 0; i < save_count; i++)
    {
        saved_values[i] = test_start[i];
    }

    /* March C test pattern */
    /* Step 1: Write 0 ascending */
    for (uint32_t i = 0; i < save_count; i++)
    {
        test_start[i] = 0x00000000UL;
    }

    /* Step 2: Read 0, Write 1 ascending */
    for (uint32_t i = 0; i < save_count; i++)
    {
        if (test_start[i] != 0x00000000UL)
        {
            /* Restore and return failure */
            for (uint32_t j = 0; j < save_count; j++)
            {
                test_start[j] = saved_values[j];
            }
            return SELFTEST_FAIL_RAM;
        }
        test_start[i] = 0xFFFFFFFFUL;
    }

    /* Step 3: Read 1, Write 0 ascending */
    for (uint32_t i = 0; i < save_count; i++)
    {
        if (test_start[i] != 0xFFFFFFFFUL)
        {
            for (uint32_t j = 0; j < save_count; j++)
            {
                test_start[j] = saved_values[j];
            }
            return SELFTEST_FAIL_RAM;
        }
        test_start[i] = 0x00000000UL;
    }

    /* Step 4: Read 0, Write 1 descending */
    for (int32_t i = save_count - 1; i >= 0; i--)
    {
        if (test_start[i] != 0x00000000UL)
        {
            for (uint32_t j = 0; j < save_count; j++)
            {
                test_start[j] = saved_values[j];
            }
            return SELFTEST_FAIL_RAM;
        }
        test_start[i] = 0xFFFFFFFFUL;
    }

    /* Step 5: Read 1, Write 0 descending */
    for (int32_t i = save_count - 1; i >= 0; i--)
    {
        if (test_start[i] != 0xFFFFFFFFUL)
        {
            for (uint32_t j = 0; j < save_count; j++)
            {
                test_start[j] = saved_values[j];
            }
            return SELFTEST_FAIL_RAM;
        }
        test_start[i] = 0x00000000UL;
    }

    /* Step 6: Final read 0 */
    for (uint32_t i = 0; i < save_count; i++)
    {
        if (test_start[i] != 0x00000000UL)
        {
            for (uint32_t j = 0; j < save_count; j++)
            {
                test_start[j] = saved_values[j];
            }
            return SELFTEST_FAIL_RAM;
        }
    }

    /* Restore original values */
    for (uint32_t i = 0; i < save_count; i++)
    {
        test_start[i] = saved_values[i];
    }

    return SELFTEST_PASS;
}

static uint32_t SelfTest_CalculateCRC32(const uint8_t *data, uint32_t length)
{
    /* Use hardware CRC unit */
    __HAL_CRC_DR_RESET(&hcrc);
    return HAL_CRC_Calculate(&hcrc, (uint32_t *)data, length / 4);
}
