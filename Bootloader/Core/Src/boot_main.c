/**
 ******************************************************************************
 * @file    boot_main.c
 * @brief   Functional Safety Bootloader Main Implementation
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Main bootloader state machine implementation
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 * Boot Flow:
 * 1. Initialize hardware (minimal)
 * 2. Run functional safety self-tests
 * 3. Validate safety parameters (Flash)
 * 4. Load non-safety parameters (EEPROM)
 * 5. Check factory mode flag
 * 6. Verify Application CRC
 * 7. Jump to Application
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "boot_main.h"
#include "boot_jump.h"
#include "boot_crc.h"
#include "boot_selftest.h"
#include "stm32f4xx_hal.h"
#include <string.h>

/* Private variables ---------------------------------------------------------*/
static boot_state_t s_boot_state = BOOT_STATE_INIT;
static boot_status_t s_last_error = BOOT_OK;
static uint32_t s_flow_signature = PFM_SIGNATURE_INIT;

/* Private function prototypes -----------------------------------------------*/
static void Boot_SystemInit(void);
static void Boot_SystemClock_Config(void);
static void Boot_FlowMonitor_Update(pfm_checkpoint_t checkpoint);
static bool Boot_FlowMonitor_Verify(uint32_t expected);

/* ============================================================================
 * Public Functions
 * ============================================================================*/

/**
 * @brief  Main bootloader entry point
 */
void Boot_Main(void)
{
    selftest_result_t selftest_result;
    safety_params_t safety_params;
    nonsafety_params_t nonsafety_params;
    boot_config_t config;
    boot_status_t status;

    /* ========================================================================
     * 1. Basic Hardware Initialization
     * ======================================================================*/
    s_boot_state = BOOT_STATE_INIT;
    Boot_FlowMonitor_Update(PFM_CP_INIT);

    Boot_SystemInit();

    /* ========================================================================
     * 2. Functional Safety Self-Test
     * ======================================================================*/
    s_boot_state = BOOT_STATE_SELFTEST;
    Boot_FlowMonitor_Update(PFM_CP_SELFTEST_START);

    selftest_result = Boot_SelfTest();
    if (selftest_result != SELFTEST_OK)
    {
        s_last_error = BOOT_ERROR_SELFTEST;
        Boot_EnterSafeState(BOOT_ERROR_SELFTEST);
        /* Never returns */
    }

    Boot_FlowMonitor_Update(PFM_CP_SELFTEST_END);

    /* ========================================================================
     * 3. Validate Safety Parameters (Flash)
     * ======================================================================*/
    s_boot_state = BOOT_STATE_VALIDATE_PARAMS;
    Boot_FlowMonitor_Update(PFM_CP_PARAMS_CHECK);

    status = Boot_ValidateSafetyParams(&safety_params);
    if (status != BOOT_OK)
    {
        /* Safety parameters corrupted - enter safe state */
        s_last_error = status;
        Boot_EnterSafeState(status);
        /* Never returns */
    }

    /* ========================================================================
     * 4. Load Non-Safety Parameters (EEPROM/Flash)
     * ======================================================================*/
    status = Boot_LoadNonSafetyParams(&nonsafety_params);
    if (status != BOOT_OK)
    {
        /* Non-safety params corrupted - use defaults, continue */
        Boot_LoadDefaultParams(&nonsafety_params);
        /* Log warning but don't enter safe state */
    }

    /* ========================================================================
     * 5. Check Boot Configuration (Factory Mode Flag)
     * ======================================================================*/
    s_boot_state = BOOT_STATE_CHECK_CONFIG;
    Boot_FlowMonitor_Update(PFM_CP_CONFIG_CHECK);

    status = Boot_ReadConfig(&config);
    if (status == BOOT_OK && config.factory_mode != 0)
    {
        /* Factory mode requested - enter factory mode */
        s_boot_state = BOOT_STATE_FACTORY_MODE;
        Boot_FlowMonitor_Update(PFM_CP_FACTORY_MODE);

        /* Note: Factory mode is only accessible via debugger
         * The factory_mode flag is set by debugger, not by communication */

        /* TODO: Implement factory mode handler */
        /* Factory_Mode_Run(&safety_params); */

        /* After factory mode completes, clear flag and reset */
        config.factory_mode = 0;
        Boot_WriteConfig(&config);

        /* System reset to start fresh */
        NVIC_SystemReset();
        /* Never returns */
    }

    /* ========================================================================
     * 6. Verify Application CRC
     * ======================================================================*/
    s_boot_state = BOOT_STATE_VERIFY_APP;
    Boot_FlowMonitor_Update(PFM_CP_APP_VERIFY);

    if (Boot_VerifyAppCRC() != BOOT_OK)
    {
        /* Application corrupted - enter safe state */
        s_last_error = BOOT_ERROR_CRC;
        Boot_EnterSafeState(BOOT_ERROR_CRC);
        /* Never returns */
    }

    /* ========================================================================
     * 7. Verify Program Flow and Jump to Application
     * ======================================================================*/
    s_boot_state = BOOT_STATE_JUMP_TO_APP;
    Boot_FlowMonitor_Update(PFM_CP_JUMP_PREPARE);

    /* Verify program flow before jump */
    if (!Boot_FlowMonitor_Verify(PFM_SIGNATURE_JUMP))
    {
        /* Program flow error - enter safe state */
        s_last_error = BOOT_ERROR;
        Boot_EnterSafeState(BOOT_ERROR);
        /* Never returns */
    }

    Boot_FlowMonitor_Update(PFM_CP_JUMP_EXECUTE);

    /* Jump to Application - this function never returns */
    Boot_JumpToApplication();

    /* Should never reach here */
    Boot_EnterSafeState(BOOT_ERROR);
}

/**
 * @brief  Get current bootloader state
 */
boot_state_t Boot_GetState(void)
{
    return s_boot_state;
}

/**
 * @brief  Get last error code
 */
boot_status_t Boot_GetLastError(void)
{
    return s_last_error;
}

/**
 * @brief  Enter safe state
 */
void Boot_EnterSafeState(boot_status_t error)
{
    s_boot_state = BOOT_STATE_SAFE;
    s_last_error = error;

    /* Disable all interrupts */
    __disable_irq();

    /* Configure safe outputs */
    /* TODO: Set all outputs to safe state based on application requirements */

    /* Stop watchdog refresh - system will reset */
    /* Or stay in infinite loop with LED indication */

    while (1)
    {
        /* Toggle LED or other indication */
        /* HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin); */
        /* HAL_Delay(100); */

        /* In production, watchdog will reset the system */
        __NOP();
    }
}

/**
 * @brief  Validate safety parameters from Flash
 */
boot_status_t Boot_ValidateSafetyParams(safety_params_t *params)
{
    uint32_t calc_crc;
    int i;

    if (params == NULL)
    {
        return BOOT_ERROR;
    }

    /* 1. Read from Flash */
    memcpy(params, (void *)SAFETY_PARAMS_ADDR, sizeof(safety_params_t));

    /* 2. Verify magic number */
    if (params->magic != SAFETY_PARAMS_MAGIC_VAL)
    {
        return BOOT_ERROR_MAGIC;
    }

    /* 3. Verify CRC32 */
    calc_crc = Boot_CRC32_Calculate((uint8_t *)params,
                                     sizeof(safety_params_t) - sizeof(uint32_t));
    if (calc_crc != params->crc32)
    {
        return BOOT_ERROR_CRC;
    }

    /* 4. Verify redundancy (inverted copies) */
    for (i = 0; i < 3; i++)
    {
        /* Check HALL offset redundancy */
        uint32_t *offset = (uint32_t *)&params->hall_offset[i];
        uint32_t *offset_inv = (uint32_t *)&params->hall_offset_inv[i];
        if (*offset != ~(*offset_inv))
        {
            return BOOT_ERROR_REDUNDANCY;
        }

        /* Check HALL gain redundancy */
        uint32_t *gain = (uint32_t *)&params->hall_gain[i];
        uint32_t *gain_inv = (uint32_t *)&params->hall_gain_inv[i];
        if (*gain != ~(*gain_inv))
        {
            return BOOT_ERROR_REDUNDANCY;
        }
    }

    /* 5. Range check (application specific) */
    /* TODO: Add range checks based on actual parameter requirements */

    return BOOT_OK;
}

/**
 * @brief  Load non-safety parameters
 */
boot_status_t Boot_LoadNonSafetyParams(nonsafety_params_t *params)
{
    uint16_t calc_crc;

    if (params == NULL)
    {
        return BOOT_ERROR;
    }

    /* TODO: Read from EEPROM or Flash backup area */
    /* For now, use defaults */
    Boot_LoadDefaultParams(params);

    /* Verify magic */
    if (params->magic != NONSAFETY_PARAMS_MAGIC_VAL)
    {
        return BOOT_ERROR_MAGIC;
    }

    /* Verify CRC16 */
    calc_crc = Boot_CRC16_Calculate((uint8_t *)params,
                                     sizeof(nonsafety_params_t) - sizeof(uint32_t));
    if (calc_crc != params->crc16)
    {
        return BOOT_ERROR_CRC;
    }

    return BOOT_OK;
}

/**
 * @brief  Load default non-safety parameters
 */
void Boot_LoadDefaultParams(nonsafety_params_t *params)
{
    if (params == NULL)
    {
        return;
    }

    params->magic = NONSAFETY_PARAMS_MAGIC_VAL;
    params->can_baudrate = DEFAULT_CAN_BAUDRATE;
    params->can_id_base = DEFAULT_CAN_ID_BASE;
    params->comm_timeout_ms = DEFAULT_COMM_TIMEOUT;
    params->reserved = 0;
    params->crc16 = Boot_CRC16_Calculate((uint8_t *)params,
                                          sizeof(nonsafety_params_t) - sizeof(uint32_t));
    params->padding = 0;
}

/**
 * @brief  Read boot configuration from Flash
 */
boot_status_t Boot_ReadConfig(boot_config_t *config)
{
    uint32_t calc_crc;

    if (config == NULL)
    {
        return BOOT_ERROR;
    }

    /* Read from Flash */
    memcpy(config, (void *)BOOT_CONFIG_ADDR, sizeof(boot_config_t));

    /* Verify magic */
    if (config->magic != BOOT_CONFIG_MAGIC)
    {
        /* First boot or corrupted - initialize with defaults */
        memset(config, 0, sizeof(boot_config_t));
        config->magic = BOOT_CONFIG_MAGIC;
        return BOOT_ERROR_MAGIC;
    }

    /* Verify CRC */
    calc_crc = Boot_CRC32_Calculate((uint8_t *)config,
                                     sizeof(boot_config_t) - sizeof(uint32_t));
    if (calc_crc != config->crc)
    {
        return BOOT_ERROR_CRC;
    }

    return BOOT_OK;
}

/**
 * @brief  Write boot configuration to Flash
 */
boot_status_t Boot_WriteConfig(const boot_config_t *config)
{
    /* TODO: Implement Flash write with proper erase/program sequence */
    /* This requires HAL_FLASH_Unlock, sector erase, and program */

    (void)config;  /* Suppress unused parameter warning */

    return BOOT_OK;
}

/* ============================================================================
 * Private Functions
 * ============================================================================*/

/**
 * @brief  Basic system initialization
 */
static void Boot_SystemInit(void)
{
    /* Initialize HAL */
    HAL_Init();

    /* Configure system clock to 168MHz */
    Boot_SystemClock_Config();

    /* Enable required peripheral clocks */
    __HAL_RCC_CRC_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
    /* Note: Flash doesn't need clock enable on STM32F4 */
}

/**
 * @brief  System Clock Configuration (168MHz from HSE)
 */
static void Boot_SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Configure the main internal regulator output voltage */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* Initialize HSE Oscillator and PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        /* Clock configuration failed - enter safe state */
        Boot_EnterSafeState(BOOT_ERROR_CLOCK);
    }

    /* Select PLL as system clock source and configure HCLK, PCLK1 and PCLK2 */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        /* Clock configuration failed - enter safe state */
        Boot_EnterSafeState(BOOT_ERROR_CLOCK);
    }
}

/* ============================================================================
 * Main Entry Point
 * ============================================================================*/

/**
 * @brief  Application entry point (called by startup code)
 */
int main(void)
{
    /* Call bootloader main function */
    Boot_Main();

    /* Should never reach here */
    while (1)
    {
        __NOP();
    }
}

/**
 * @brief  Update program flow monitor signature
 */
static void Boot_FlowMonitor_Update(pfm_checkpoint_t checkpoint)
{
    /* XOR checkpoint into signature to track flow */
    s_flow_signature ^= ((uint32_t)checkpoint << 24) |
                        ((uint32_t)checkpoint << 16) |
                        ((uint32_t)checkpoint << 8) |
                        ((uint32_t)checkpoint);
}

/**
 * @brief  Verify program flow signature
 */
static bool Boot_FlowMonitor_Verify(uint32_t expected)
{
    /* Calculate expected signature based on all checkpoints */
    uint32_t calculated = PFM_SIGNATURE_INIT;

    /* XOR all expected checkpoints */
    pfm_checkpoint_t checkpoints[] = {
        PFM_CP_INIT,
        PFM_CP_SELFTEST_START,
        PFM_CP_SELFTEST_END,
        PFM_CP_PARAMS_CHECK,
        PFM_CP_CONFIG_CHECK,
        PFM_CP_APP_VERIFY,
        PFM_CP_JUMP_PREPARE
    };

    for (size_t i = 0; i < sizeof(checkpoints) / sizeof(checkpoints[0]); i++)
    {
        calculated ^= ((uint32_t)checkpoints[i] << 24) |
                      ((uint32_t)checkpoints[i] << 16) |
                      ((uint32_t)checkpoints[i] << 8) |
                      ((uint32_t)checkpoints[i]);
    }

    (void)expected;  /* Use expected or calculated based on design */

    return (s_flow_signature == calculated);
}
