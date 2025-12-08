/**
 ******************************************************************************
 * @file    safety_config.h
 * @brief   Functional Safety Configuration
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Runtime Functional Safety Configuration for Application
 * Target: STM32F407VGT6
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

#ifndef __SAFETY_CONFIG_H
#define __SAFETY_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "shared_config.h"

/* ============================================================================
 * Safety Operation Status
 * ============================================================================*/

/**
 * @brief Safety operation status
 */
typedef enum {
    SAFETY_OK                   = 0x00U,    /* Operation successful */
    SAFETY_ERROR                = 0x01U,    /* Operation failed */
    SAFETY_BUSY                 = 0x02U,    /* Operation in progress */
    SAFETY_TIMEOUT              = 0x03U,    /* Operation timeout */
    SAFETY_INVALID_PARAM        = 0x04U     /* Invalid parameter */
} safety_status_t;

/* ============================================================================
 * Safety Thread Configuration
 * ============================================================================*/
#define SAFETY_THREAD_STACK_SIZE    2048U       /* Safety thread stack size */
#define SAFETY_THREAD_PRIORITY      1U          /* Highest priority */
#define SAFETY_THREAD_PREEMPT_THRESH 1U         /* Preemption threshold */
#define SAFETY_THREAD_TIME_SLICE    0U          /* No time slicing */

/* ============================================================================
 * Lightweight Self-Test Configuration (Runtime)
 * ============================================================================*/

/* Startup self-test enables */
#define SELFTEST_STARTUP_CPU_ENABLED        1   /* CPU test at startup */
#define SELFTEST_STARTUP_RAM_ENABLED        1   /* RAM test at startup */
#define SELFTEST_STARTUP_FLASH_ENABLED      1   /* Flash CRC at startup */
#define SELFTEST_STARTUP_CLOCK_ENABLED      1   /* Clock verify at startup */

/* Runtime self-test enables (lightweight) */
#define SELFTEST_RUNTIME_CPU_ENABLED        0   /* No runtime CPU test */
#define SELFTEST_RUNTIME_RAM_ENABLED        0   /* No runtime RAM test */
#define SELFTEST_RUNTIME_FLASH_ENABLED      1   /* Incremental Flash CRC */
#define SELFTEST_RUNTIME_CLOCK_ENABLED      0   /* No runtime clock check */

/* Runtime self-test intervals (ms) */
#define SELFTEST_FLASH_CRC_INTERVAL_MS      300000U /* 5 minutes */
#define SELFTEST_FLASH_CRC_BLOCK_SIZE       4096U   /* 4KB per check */

/* ============================================================================
 * Stack Monitoring Configuration
 * ============================================================================*/
#define STACK_CHECK_INTERVAL_MS     100U        /* Check every 100ms */
#define STACK_WARNING_THRESHOLD     70U         /* Warning at 70% usage */
#define STACK_CRITICAL_THRESHOLD    90U         /* Error at 90% usage */
#define STACK_FILL_PATTERN          0xEFEFEFEFU /* ThreadX stack fill */

/* ============================================================================
 * Watchdog Configuration
 * ============================================================================*/

/* Token-based watchdog feeding */
#define WDG_TOKEN_SAFETY_THREAD     0x01U
#define WDG_TOKEN_MAIN_THREAD       0x02U
#define WDG_TOKEN_COMM_THREAD       0x04U
#define WDG_TOKEN_ALL               (WDG_TOKEN_SAFETY_THREAD | \
                                     WDG_TOKEN_MAIN_THREAD | \
                                     WDG_TOKEN_COMM_THREAD)

/* Watchdog feed period */
#define WDG_FEED_PERIOD_MS          500U        /* Feed every 500ms */
#define WDG_TOKEN_TIMEOUT_MS        800U        /* Token must arrive within */

/* ============================================================================
 * Program Flow Monitor Configuration
 * ============================================================================*/
#define FLOW_VERIFY_INTERVAL_MS     1000U       /* Verify every 1 second */
#define FLOW_SIGNATURE_SEED         0x5A5A5A5AUL

/* ============================================================================
 * MPU Configuration
 * ============================================================================*/
#define MPU_REGION_FLASH            0U          /* Application Flash */
#define MPU_REGION_RAM              1U          /* Main RAM */
#define MPU_REGION_CCM              2U          /* CCM RAM (stacks) */
#define MPU_REGION_PERIPH           3U          /* Peripheral region */
#define MPU_REGION_CONFIG           4U          /* Config Flash (RO) */
#define MPU_REGION_BOOT             5U          /* Bootloader (no access) */
#define MPU_REGION_COUNT            6U

/* MPU Access Permissions */
#define MPU_AP_NO_ACCESS            0x00U
#define MPU_AP_RW_PRIV_ONLY         0x01U
#define MPU_AP_RW_ALL               0x03U
#define MPU_AP_RO_PRIV_ONLY         0x05U
#define MPU_AP_RO_ALL               0x06U

/* ============================================================================
 * Degraded Mode Configuration
 * ============================================================================*/

/* Degraded mode behavior */
#define DEGRADED_MODE_ENABLED       1           /* Enable degraded mode */
#define DEGRADED_MODE_TIMEOUT_MS    30000U      /* Stay in degraded 30s max */
#define DEGRADED_MODE_WDG_FEED      1           /* Continue feeding WDG */

/* Safe output states (for GPIO outputs) */
#define SAFE_OUTPUT_DEFAULT         0           /* Default safe state */

/* ============================================================================
 * Error Logging Configuration
 * ============================================================================*/
#define ERROR_LOG_MAX_ENTRIES       16U         /* Max error log entries */
#define ERROR_LOG_IN_CCM            1           /* Store in CCM RAM */

/* Error log entry structure */
typedef struct {
    uint32_t timestamp;         /* System tick when error occurred */
    uint32_t error_code;        /* Error code */
    uint32_t param1;            /* Additional parameter 1 */
    uint32_t param2;            /* Additional parameter 2 */
} safety_error_log_t;

/* ============================================================================
 * Diagnostic Interface Configuration
 * ============================================================================*/
#define DIAG_UART_ENABLED           0           /* Disable UART diagnostics */
#define DIAG_RTT_ENABLED            1           /* Enable Segger RTT diagnostics */
#define DIAG_UART_BAUDRATE          115200U     /* Diagnostic UART baudrate (if enabled) */

/* ============================================================================
 * Clock Configuration
 * ============================================================================*/
#define EXPECTED_SYSCLK_HZ          168000000UL /* Expected 168MHz */
#define CLOCK_TOLERANCE_PERCENT     5U          /* +/- 5% tolerance */

/* ============================================================================
 * Memory Test Configuration
 * ============================================================================*/
#define RAM_TEST_PATTERN_1          0xAAAAAAAAUL
#define RAM_TEST_PATTERN_2          0x55555555UL
#define RAM_TEST_PATTERN_3          0xFF00FF00UL
#define RAM_TEST_PATTERN_4          0x00FF00FFUL

/* ============================================================================
 * CRC Configuration
 * ============================================================================*/
#define CRC32_POLYNOMIAL            0x04C11DB7UL /* Ethernet polynomial */
#define CRC32_INIT_VALUE            0xFFFFFFFFUL

#ifdef __cplusplus
}
#endif

#endif /* __SAFETY_CONFIG_H */
