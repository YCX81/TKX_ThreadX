/**
 ******************************************************************************
 * @file    safety_core.c
 * @brief   Functional Safety Core Implementation
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Core functional safety management implementation
 * Target: STM32F407VGT6
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "safety_core.h"
#include "stm32f4xx_hal.h"
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define ERROR_LOG_SIZE      ERROR_LOG_MAX_ENTRIES

/* Private variables ---------------------------------------------------------*/
static safety_context_t s_safety_ctx;
static safety_error_log_t s_error_log[ERROR_LOG_SIZE];
static uint32_t s_error_log_index = 0;
static uint32_t s_startup_tick = 0;

/* ============================================================================
 * Private Function Prototypes
 * ============================================================================*/
static void Safety_LogError(safety_error_t error, uint32_t param1, uint32_t param2);
static void Safety_CallErrorCallback(safety_error_t error);
static void Safety_CallStateCallback(safety_state_t old_state, safety_state_t new_state);
static void Safety_SetSafeOutputs(void);

/* ============================================================================
 * Initialization Functions
 * ============================================================================*/

safety_status_t Safety_EarlyInit(void)
{
    /* Clear context */
    memset(&s_safety_ctx, 0, sizeof(safety_context_t));
    memset(s_error_log, 0, sizeof(s_error_log));

    /* Set initial state */
    s_safety_ctx.state = SAFETY_STATE_INIT;
    s_safety_ctx.last_error = SAFETY_ERR_NONE;
    s_safety_ctx.error_count = 0;
    s_safety_ctx.startup_test_passed = false;
    s_safety_ctx.params_valid = false;
    s_safety_ctx.mpu_enabled = false;
    s_safety_ctx.watchdog_active = false;

    /* Record startup time (will be set properly after HAL_Init) */
    s_startup_tick = 0;

    return SAFETY_OK;
}

safety_status_t Safety_PostClockInit(void)
{
    /* Record actual startup tick */
    s_startup_tick = HAL_GetTick();
    s_safety_ctx.startup_time = s_startup_tick;

    /* Verify clock configuration */
    uint32_t sysclk = HAL_RCC_GetSysClockFreq();
    uint32_t min_freq = EXPECTED_SYSCLK_HZ * (100 - CLOCK_TOLERANCE_PERCENT) / 100;
    uint32_t max_freq = EXPECTED_SYSCLK_HZ * (100 + CLOCK_TOLERANCE_PERCENT) / 100;

    if (sysclk < min_freq || sysclk > max_freq)
    {
        Safety_ReportError(SAFETY_ERR_CLOCK, sysclk, EXPECTED_SYSCLK_HZ);
        return SAFETY_ERROR;
    }

    return SAFETY_OK;
}

safety_status_t Safety_PeripheralInit(void)
{
    /* Transition to startup test state */
    s_safety_ctx.state = SAFETY_STATE_STARTUP_TEST;

    return SAFETY_OK;
}

safety_status_t Safety_StartupTest(void)
{
    safety_status_t status = SAFETY_OK;

    /* State should be STARTUP_TEST */
    if (s_safety_ctx.state != SAFETY_STATE_STARTUP_TEST)
    {
        Safety_ReportError(SAFETY_ERR_INTERNAL, s_safety_ctx.state, 0);
        return SAFETY_ERROR;
    }

    /* Startup tests will be performed by safety_selftest module */
    /* This function is called after all tests pass */

    s_safety_ctx.startup_test_passed = true;

    return status;
}

safety_status_t Safety_PreKernelInit(void)
{
    /* Final checks before kernel starts */
    if (!s_safety_ctx.startup_test_passed)
    {
        Safety_EnterSafeState(SAFETY_ERR_INTERNAL);
        return SAFETY_ERROR;
    }

    /* Transition to normal state */
    Safety_SetState(SAFETY_STATE_NORMAL);

    return SAFETY_OK;
}

/* ============================================================================
 * State Management Functions
 * ============================================================================*/

safety_state_t Safety_GetState(void)
{
    return s_safety_ctx.state;
}

safety_status_t Safety_SetState(safety_state_t state)
{
    safety_state_t old_state = s_safety_ctx.state;

    /* Validate state transition */
    switch (s_safety_ctx.state)
    {
        case SAFETY_STATE_INIT:
            /* Can only go to STARTUP_TEST */
            if (state != SAFETY_STATE_STARTUP_TEST && state != SAFETY_STATE_SAFE)
            {
                return SAFETY_INVALID_PARAM;
            }
            break;

        case SAFETY_STATE_STARTUP_TEST:
            /* Can go to NORMAL or SAFE */
            if (state != SAFETY_STATE_NORMAL && state != SAFETY_STATE_SAFE)
            {
                return SAFETY_INVALID_PARAM;
            }
            break;

        case SAFETY_STATE_NORMAL:
            /* Can go to DEGRADED or SAFE */
            if (state != SAFETY_STATE_DEGRADED && state != SAFETY_STATE_SAFE)
            {
                return SAFETY_INVALID_PARAM;
            }
            break;

        case SAFETY_STATE_DEGRADED:
            /* Can go to NORMAL or SAFE */
            if (state != SAFETY_STATE_NORMAL && state != SAFETY_STATE_SAFE)
            {
                return SAFETY_INVALID_PARAM;
            }
            break;

        case SAFETY_STATE_SAFE:
            /* Cannot leave SAFE state (requires reset) */
            return SAFETY_ERROR;

        default:
            return SAFETY_INVALID_PARAM;
    }

    s_safety_ctx.state = state;
    Safety_CallStateCallback(old_state, state);

    return SAFETY_OK;
}

safety_status_t Safety_EnterNormal(void)
{
    if (s_safety_ctx.state == SAFETY_STATE_DEGRADED)
    {
        return Safety_SetState(SAFETY_STATE_NORMAL);
    }
    return SAFETY_ERROR;
}

safety_status_t Safety_EnterDegraded(safety_error_t error)
{
#if DEGRADED_MODE_ENABLED
    safety_state_t old_state = s_safety_ctx.state;

    if (old_state == SAFETY_STATE_NORMAL || old_state == SAFETY_STATE_STARTUP_TEST)
    {
        s_safety_ctx.state = SAFETY_STATE_DEGRADED;
        s_safety_ctx.degraded_enter_time = HAL_GetTick();
        s_safety_ctx.last_error = error;

        Safety_CallStateCallback(old_state, SAFETY_STATE_DEGRADED);
        Safety_CallErrorCallback(error);

        return SAFETY_OK;
    }
#else
    /* If degraded mode not enabled, go directly to safe state */
    Safety_EnterSafeState(error);
#endif

    return SAFETY_ERROR;
}

void Safety_EnterSafeState(safety_error_t error)
{
    safety_state_t old_state = s_safety_ctx.state;

    /* Log the error */
    Safety_LogError(error, 0, 0);

    /* Set safe outputs */
    Safety_SetSafeOutputs();

    /* Update state */
    s_safety_ctx.state = SAFETY_STATE_SAFE;
    s_safety_ctx.last_error = error;
    s_safety_ctx.error_count++;

    /* Notify callbacks */
    Safety_CallStateCallback(old_state, SAFETY_STATE_SAFE);
    Safety_CallErrorCallback(error);

#if !DEGRADED_MODE_WDG_FEED
    /* If not feeding watchdog in safe state, system will reset */
    __disable_irq();
    while (1)
    {
        /* Wait for watchdog reset */
    }
#endif
}

bool Safety_IsOperational(void)
{
    return (s_safety_ctx.state == SAFETY_STATE_NORMAL ||
            s_safety_ctx.state == SAFETY_STATE_DEGRADED);
}

/* ============================================================================
 * Error Handling Functions
 * ============================================================================*/

void Safety_ReportError(safety_error_t error, uint32_t param1, uint32_t param2)
{
    /* Log the error */
    Safety_LogError(error, param1, param2);

    /* Update context */
    s_safety_ctx.last_error = error;
    s_safety_ctx.error_count++;

    /* Determine action based on current state and error severity */
    switch (error)
    {
        case SAFETY_ERR_CPU_TEST:
        case SAFETY_ERR_RAM_TEST:
        case SAFETY_ERR_HARDFAULT:
        case SAFETY_ERR_BUSFAULT:
        case SAFETY_ERR_USAGEFAULT:
        case SAFETY_ERR_NMI:
            /* Critical errors - go to safe state */
            Safety_EnterSafeState(error);
            break;

        case SAFETY_ERR_FLASH_CRC:
        case SAFETY_ERR_CLOCK:
        case SAFETY_ERR_FLOW_MONITOR:
        case SAFETY_ERR_MPU_FAULT:
            /* Serious errors - enter degraded mode */
            if (s_safety_ctx.state == SAFETY_STATE_NORMAL)
            {
                Safety_EnterDegraded(error);
            }
            else if (s_safety_ctx.state == SAFETY_STATE_DEGRADED)
            {
                /* Already degraded, go to safe */
                Safety_EnterSafeState(error);
            }
            break;

        case SAFETY_ERR_STACK_OVERFLOW:
        case SAFETY_ERR_PARAM_INVALID:
        case SAFETY_ERR_RUNTIME_TEST:
            /* Warning level - log and continue or degrade */
            Safety_CallErrorCallback(error);
            break;

        default:
            Safety_CallErrorCallback(error);
            break;
    }
}

safety_error_t Safety_GetLastError(void)
{
    return s_safety_ctx.last_error;
}

uint32_t Safety_GetErrorCount(void)
{
    return s_safety_ctx.error_count;
}

safety_status_t Safety_ClearError(void)
{
    if (s_safety_ctx.state != SAFETY_STATE_NORMAL)
    {
        return SAFETY_ERROR;
    }

    s_safety_ctx.last_error = SAFETY_ERR_NONE;
    return SAFETY_OK;
}

safety_status_t Safety_GetErrorLog(uint32_t index, safety_error_log_t *entry)
{
    if (entry == NULL || index >= ERROR_LOG_SIZE)
    {
        return SAFETY_INVALID_PARAM;
    }

    *entry = s_error_log[index];
    return SAFETY_OK;
}

/* ============================================================================
 * Callback Registration Functions
 * ============================================================================*/

void Safety_RegisterErrorCallback(safety_error_callback_t callback)
{
    s_safety_ctx.error_cb = callback;
}

void Safety_RegisterStateCallback(safety_state_callback_t callback)
{
    s_safety_ctx.state_cb = callback;
}

/* ============================================================================
 * Diagnostic Functions
 * ============================================================================*/

const safety_context_t* Safety_GetContext(void)
{
    return &s_safety_ctx;
}

uint32_t Safety_GetUptime(void)
{
    return HAL_GetTick() - s_startup_tick;
}

void Safety_PrintDiagnostics(void)
{
#if DIAG_UART_ENABLED
    /* TODO: Implement UART diagnostic output */
#endif
}

/* ============================================================================
 * Fault Handlers
 * ============================================================================*/

void Safety_HardFaultHandler(void)
{
    Safety_LogError(SAFETY_ERR_HARDFAULT, __get_MSP(), __get_PSP());
    Safety_EnterSafeState(SAFETY_ERR_HARDFAULT);
}

void Safety_MemManageHandler(void)
{
    uint32_t mmfar = SCB->MMFAR;
    uint32_t cfsr = SCB->CFSR;
    Safety_LogError(SAFETY_ERR_MPU_FAULT, mmfar, cfsr);
    Safety_EnterSafeState(SAFETY_ERR_MPU_FAULT);
}

void Safety_BusFaultHandler(void)
{
    uint32_t bfar = SCB->BFAR;
    uint32_t cfsr = SCB->CFSR;
    Safety_LogError(SAFETY_ERR_BUSFAULT, bfar, cfsr);
    Safety_EnterSafeState(SAFETY_ERR_BUSFAULT);
}

void Safety_UsageFaultHandler(void)
{
    Safety_LogError(SAFETY_ERR_USAGEFAULT, 0, SCB->CFSR);
    Safety_EnterSafeState(SAFETY_ERR_USAGEFAULT);
}

void Safety_NMIHandler(void)
{
    Safety_LogError(SAFETY_ERR_NMI, 0, 0);
    Safety_EnterSafeState(SAFETY_ERR_NMI);
}

/* ============================================================================
 * Private Functions
 * ============================================================================*/

static void Safety_LogError(safety_error_t error, uint32_t param1, uint32_t param2)
{
    safety_error_log_t *entry = &s_error_log[s_error_log_index];

    entry->timestamp = HAL_GetTick();
    entry->error_code = (uint32_t)error;
    entry->param1 = param1;
    entry->param2 = param2;

    s_error_log_index = (s_error_log_index + 1) % ERROR_LOG_SIZE;
}

static void Safety_CallErrorCallback(safety_error_t error)
{
    if (s_safety_ctx.error_cb != NULL)
    {
        s_safety_ctx.error_cb(error);
    }
}

static void Safety_CallStateCallback(safety_state_t old_state, safety_state_t new_state)
{
    if (s_safety_ctx.state_cb != NULL)
    {
        s_safety_ctx.state_cb(old_state, new_state);
    }
}

static void Safety_SetSafeOutputs(void)
{
    /* Set all safety-critical outputs to safe state */
    /* This should be customized based on application requirements */

    /* Example: Set LED to indicate error */
    /* HAL_GPIO_WritePin(LED_ERROR_GPIO_Port, LED_ERROR_Pin, GPIO_PIN_SET); */

    /* Example: Disable motor outputs */
    /* HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin, GPIO_PIN_RESET); */
}
