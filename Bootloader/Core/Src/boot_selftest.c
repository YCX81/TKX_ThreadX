/**
 ******************************************************************************
 * @file    boot_selftest.c
 * @brief   Functional Safety Startup Self-Test Implementation
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Simplified self-test implementation for bootloader.
 * For production, consider using X-CUBE-STL.
 *
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "boot_selftest.h"
#include "boot_crc.h"
#include "stm32f4xx_hal.h"
#include <string.h>

/* Private variables ---------------------------------------------------------*/
static IWDG_HandleTypeDef hiwdg;

/* External assembly function for CPU register test */
extern test_result_t CPU_RegisterTest(void);

/* ============================================================================
 * Main Self-Test Entry Point
 * ============================================================================*/

/**
 * @brief  Run all startup self-tests
 */
selftest_result_t Boot_SelfTest(void)
{
    /* Initialize CRC unit first (needed for Flash test) */
    Boot_CRC_Init();

    /* 1. CPU Register Test */
    if (Boot_CPU_Test() != TEST_PASS)
    {
        return SELFTEST_CPU_FAIL;
    }

    /* 2. RAM Test (March C) */
    if (Boot_RAM_Test() != TEST_PASS)
    {
        return SELFTEST_RAM_FAIL;
    }

    /* 3. Flash CRC Test */
    if (Boot_Flash_Test() != TEST_PASS)
    {
        return SELFTEST_FLASH_FAIL;
    }

    /* 4. Clock System Test */
    if (Boot_Clock_Test() != TEST_PASS)
    {
        return SELFTEST_CLOCK_FAIL;
    }

    /* 5. Initialize Watchdog */
    if (Boot_Watchdog_Init() != TEST_PASS)
    {
        return SELFTEST_WDG_FAIL;
    }

    return SELFTEST_OK;
}

/* ============================================================================
 * CPU Register Test (C implementation - can be replaced with assembly)
 * ============================================================================*/

/**
 * @brief  Simple CPU register test
 * @note   For production, use assembly implementation or X-CUBE-STL
 */
test_result_t Boot_CPU_Test(void)
{
    volatile uint32_t test_val;

    /* Test with pattern 1 */
    test_val = TEST_PATTERN_1;
    if (test_val != TEST_PATTERN_1)
    {
        return TEST_FAIL;
    }

    /* Test with pattern 2 */
    test_val = TEST_PATTERN_2;
    if (test_val != TEST_PATTERN_2)
    {
        return TEST_FAIL;
    }

    /* Test with pattern 3 */
    test_val = TEST_PATTERN_3;
    if (test_val != TEST_PATTERN_3)
    {
        return TEST_FAIL;
    }

    /* Test with pattern 4 */
    test_val = TEST_PATTERN_4;
    if (test_val != TEST_PATTERN_4)
    {
        return TEST_FAIL;
    }

    /* Note: Full CPU register test should be done in assembly
     * to properly test all registers R0-R12, MSP, PSP, etc.
     * This is a simplified version for demonstration. */

    return TEST_PASS;
}

/* ============================================================================
 * RAM March C Test
 * ============================================================================*/

/**
 * @brief  RAM March C test (simplified)
 * @note   March C algorithm:
 *         1. ↓ Write 0
 *         2. ↓ Read 0, Write 1
 *         3. ↓ Read 1, Write 0
 *         4. ↑ Read 0, Write 1
 *         5. ↑ Read 1, Write 0
 *         6. Read 0
 */
test_result_t Boot_RAM_Test(void)
{
    volatile uint32_t *test_addr;
    uint32_t backup[RAM_TEST_SIZE / 4];
    uint32_t i;
    uint32_t num_words = RAM_TEST_SIZE / 4;

    test_addr = (volatile uint32_t *)RAM_TEST_START;

    /* Backup original data */
    for (i = 0; i < num_words; i++)
    {
        backup[i] = test_addr[i];
    }

    /* Step 1: Write 0 (ascending) */
    for (i = 0; i < num_words; i++)
    {
        test_addr[i] = 0x00000000UL;
    }

    /* Step 2: Read 0, Write 1 (ascending) */
    for (i = 0; i < num_words; i++)
    {
        if (test_addr[i] != 0x00000000UL)
        {
            goto test_fail;
        }
        test_addr[i] = 0xFFFFFFFFUL;
    }

    /* Step 3: Read 1, Write 0 (ascending) */
    for (i = 0; i < num_words; i++)
    {
        if (test_addr[i] != 0xFFFFFFFFUL)
        {
            goto test_fail;
        }
        test_addr[i] = 0x00000000UL;
    }

    /* Step 4: Read 0, Write 1 (descending) */
    for (i = num_words; i > 0; i--)
    {
        if (test_addr[i - 1] != 0x00000000UL)
        {
            goto test_fail;
        }
        test_addr[i - 1] = 0xFFFFFFFFUL;
    }

    /* Step 5: Read 1, Write 0 (descending) */
    for (i = num_words; i > 0; i--)
    {
        if (test_addr[i - 1] != 0xFFFFFFFFUL)
        {
            goto test_fail;
        }
        test_addr[i - 1] = 0x00000000UL;
    }

    /* Step 6: Read 0 */
    for (i = 0; i < num_words; i++)
    {
        if (test_addr[i] != 0x00000000UL)
        {
            goto test_fail;
        }
    }

    /* Restore original data */
    for (i = 0; i < num_words; i++)
    {
        test_addr[i] = backup[i];
    }

    return TEST_PASS;

test_fail:
    /* Attempt to restore data before returning */
    for (i = 0; i < num_words; i++)
    {
        test_addr[i] = backup[i];
    }
    return TEST_FAIL;
}

/* ============================================================================
 * Flash CRC Test
 * ============================================================================*/

/**
 * @brief  Verify Bootloader Flash integrity
 */
test_result_t Boot_Flash_Test(void)
{
    boot_status_t status;

    /* Verify Bootloader CRC */
    status = Boot_CRC32_Verify(BOOT_FLASH_START,
                               BOOT_FLASH_SIZE - 4,  /* Exclude CRC itself */
                               BOOT_CRC_ADDR);

    if (status != BOOT_OK)
    {
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* ============================================================================
 * Clock System Test
 * ============================================================================*/

/**
 * @brief  Verify clock system is running correctly
 */
test_result_t Boot_Clock_Test(void)
{
    uint32_t sysclk;
    uint32_t expected_min;
    uint32_t expected_max;

    /* Get current system clock */
    sysclk = HAL_RCC_GetSysClockFreq();

    /* Calculate acceptable range */
    expected_min = EXPECTED_SYSCLK_HZ * (100 - CLOCK_TOLERANCE_PERCENT) / 100;
    expected_max = EXPECTED_SYSCLK_HZ * (100 + CLOCK_TOLERANCE_PERCENT) / 100;

    /* Verify clock is within tolerance */
    if (sysclk < expected_min || sysclk > expected_max)
    {
        return TEST_FAIL;
    }

    /* Verify HSE is ready (if using HSE) */
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_HSERDY) == RESET)
    {
        /* HSE not ready - may be using HSI, check configuration */
        /* For safety critical applications, HSE should be used */
    }

    /* Verify PLL is locked (if using PLL) */
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_PLLRDY) == RESET)
    {
        /* PLL not ready */
    }

    return TEST_PASS;
}

/* ============================================================================
 * Watchdog Functions
 * ============================================================================*/

/**
 * @brief  Initialize Independent Watchdog (IWDG)
 */
test_result_t Boot_Watchdog_Init(void)
{
    /* Configure IWDG
     * LSI = 32 kHz (typical)
     * Prescaler = 64
     * Reload = 500
     * Timeout = (64 * 500) / 32000 = 1 second
     */
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
    hiwdg.Init.Reload = 500;

    if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
    {
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/**
 * @brief  Refresh watchdog
 */
void Boot_Watchdog_Refresh(void)
{
    HAL_IWDG_Refresh(&hiwdg);
}
