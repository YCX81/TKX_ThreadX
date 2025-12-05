/**
 ******************************************************************************
 * @file    safety_core.h
 * @brief   Functional Safety Core Interface
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Core functional safety management for application runtime
 * Target: STM32F407VGT6
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

#ifndef __SAFETY_CORE_H
#define __SAFETY_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "safety_config.h"
#include "tx_api.h"

/* ============================================================================
 * Safety State Definitions
 * ============================================================================*/

/**
 * @brief Safety system state machine states
 */
typedef enum {
    SAFETY_STATE_INIT           = 0x00U,    /* Initial state */
    SAFETY_STATE_STARTUP_TEST   = 0x01U,    /* Running startup tests */
    SAFETY_STATE_NORMAL         = 0x02U,    /* Normal operation */
    SAFETY_STATE_DEGRADED       = 0x03U,    /* Degraded operation */
    SAFETY_STATE_SAFE           = 0x04U,    /* Safe state (stopped) */
    SAFETY_STATE_ERROR          = 0xFFU     /* Error state */
} safety_state_t;

/**
 * @brief Safety error codes
 */
typedef enum {
    SAFETY_ERR_NONE             = 0x00U,    /* No error */
    SAFETY_ERR_CPU_TEST         = 0x01U,    /* CPU register test failed */
    SAFETY_ERR_RAM_TEST         = 0x02U,    /* RAM test failed */
    SAFETY_ERR_FLASH_CRC        = 0x03U,    /* Flash CRC mismatch */
    SAFETY_ERR_CLOCK            = 0x04U,    /* Clock frequency error */
    SAFETY_ERR_WATCHDOG         = 0x05U,    /* Watchdog error */
    SAFETY_ERR_STACK_OVERFLOW   = 0x06U,    /* Stack overflow detected */
    SAFETY_ERR_FLOW_MONITOR     = 0x07U,    /* Program flow error */
    SAFETY_ERR_PARAM_INVALID    = 0x08U,    /* Parameter validation failed */
    SAFETY_ERR_RUNTIME_TEST     = 0x09U,    /* Runtime test failed */
    SAFETY_ERR_MPU_FAULT        = 0x0AU,    /* MPU access violation */
    SAFETY_ERR_HARDFAULT        = 0x0BU,    /* Hard fault occurred */
    SAFETY_ERR_BUSFAULT         = 0x0CU,    /* Bus fault occurred */
    SAFETY_ERR_USAGEFAULT       = 0x0DU,    /* Usage fault occurred */
    SAFETY_ERR_NMI              = 0x0EU,    /* NMI occurred */
    SAFETY_ERR_INTERNAL         = 0xFFU     /* Internal error */
} safety_error_t;

/* ============================================================================
 * Callback Type Definitions
 * ============================================================================*/

/**
 * @brief Error callback function type
 * @param error Error code that triggered the callback
 */
typedef void (*safety_error_callback_t)(safety_error_t error);

/**
 * @brief State change callback function type
 * @param old_state Previous state
 * @param new_state New state
 */
typedef void (*safety_state_callback_t)(safety_state_t old_state,
                                        safety_state_t new_state);

/* ============================================================================
 * Safety Context Structure
 * ============================================================================*/

/**
 * @brief Safety system runtime context
 */
typedef struct {
    safety_state_t state;               /* Current safety state */
    safety_error_t last_error;          /* Last error code */
    uint32_t error_count;               /* Total error count */
    uint32_t startup_time;              /* Startup timestamp */
    uint32_t degraded_enter_time;       /* Degraded mode entry time */
    bool startup_test_passed;           /* Startup test result */
    bool params_valid;                  /* Parameters validated */
    bool mpu_enabled;                   /* MPU protection enabled */
    bool watchdog_active;               /* Watchdog active */
    safety_error_callback_t error_cb;   /* Error callback */
    safety_state_callback_t state_cb;   /* State change callback */
} safety_context_t;

/* ============================================================================
 * Initialization Functions
 * ============================================================================*/

/**
 * @brief Early initialization before HAL_Init
 * @note Called from USER CODE BEGIN 1 in main.c
 * @retval safety_status_t Status
 */
safety_status_t Safety_EarlyInit(void);

/**
 * @brief Post clock configuration initialization
 * @note Called from USER CODE BEGIN SysInit in main.c
 * @retval safety_status_t Status
 */
safety_status_t Safety_PostClockInit(void);

/**
 * @brief Peripheral initialization
 * @note Called after all MX_xxx_Init functions
 * @retval safety_status_t Status
 */
safety_status_t Safety_PeripheralInit(void);

/**
 * @brief Run startup self-tests
 * @note Called from USER CODE BEGIN 2 in main.c
 * @retval safety_status_t Status
 */
safety_status_t Safety_StartupTest(void);

/**
 * @brief Pre-kernel initialization
 * @note Called before ThreadX kernel starts
 * @retval safety_status_t Status
 */
safety_status_t Safety_PreKernelInit(void);

/* ============================================================================
 * State Management Functions
 * ============================================================================*/

/**
 * @brief Get current safety state
 * @retval safety_state_t Current state
 */
safety_state_t Safety_GetState(void);

/**
 * @brief Set safety state
 * @param state New state to set
 * @retval safety_status_t Status
 */
safety_status_t Safety_SetState(safety_state_t state);

/**
 * @brief Enter normal operation mode
 * @retval safety_status_t Status
 */
safety_status_t Safety_EnterNormal(void);

/**
 * @brief Enter degraded operation mode
 * @param error Error that caused degraded mode
 * @retval safety_status_t Status
 */
safety_status_t Safety_EnterDegraded(safety_error_t error);

/**
 * @brief Enter safe state
 * @param error Error that caused safe state
 */
void Safety_EnterSafeState(safety_error_t error);

/**
 * @brief Check if system is in safe operational state
 * @retval bool true if NORMAL or DEGRADED
 */
bool Safety_IsOperational(void);

/* ============================================================================
 * Error Handling Functions
 * ============================================================================*/

/**
 * @brief Report a safety error
 * @param error Error code
 * @param param1 Additional parameter 1
 * @param param2 Additional parameter 2
 */
void Safety_ReportError(safety_error_t error, uint32_t param1, uint32_t param2);

/**
 * @brief Get last error code
 * @retval safety_error_t Last error
 */
safety_error_t Safety_GetLastError(void);

/**
 * @brief Get total error count
 * @retval uint32_t Error count
 */
uint32_t Safety_GetErrorCount(void);

/**
 * @brief Clear error status
 * @note Only clears if system is in NORMAL state
 * @retval safety_status_t Status
 */
safety_status_t Safety_ClearError(void);

/**
 * @brief Get error log entry
 * @param index Log entry index
 * @param entry Pointer to store entry
 * @retval safety_status_t Status
 */
safety_status_t Safety_GetErrorLog(uint32_t index, safety_error_log_t *entry);

/* ============================================================================
 * Callback Registration Functions
 * ============================================================================*/

/**
 * @brief Register error callback
 * @param callback Callback function pointer
 */
void Safety_RegisterErrorCallback(safety_error_callback_t callback);

/**
 * @brief Register state change callback
 * @param callback Callback function pointer
 */
void Safety_RegisterStateCallback(safety_state_callback_t callback);

/* ============================================================================
 * Diagnostic Functions
 * ============================================================================*/

/**
 * @brief Get safety context for diagnostics
 * @retval const safety_context_t* Pointer to context
 */
const safety_context_t* Safety_GetContext(void);

/**
 * @brief Get uptime in milliseconds
 * @retval uint32_t Uptime in ms
 */
uint32_t Safety_GetUptime(void);

/**
 * @brief Print diagnostic information
 * @note Outputs to diagnostic UART if enabled
 */
void Safety_PrintDiagnostics(void);

/* ============================================================================
 * Fault Handlers (called from stm32f4xx_it.c)
 * ============================================================================*/

/**
 * @brief Hard fault handler hook
 */
void Safety_HardFaultHandler(void);

/**
 * @brief Memory management fault handler hook
 */
void Safety_MemManageHandler(void);

/**
 * @brief Bus fault handler hook
 */
void Safety_BusFaultHandler(void);

/**
 * @brief Usage fault handler hook
 */
void Safety_UsageFaultHandler(void);

/**
 * @brief NMI handler hook
 */
void Safety_NMIHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* __SAFETY_CORE_H */
