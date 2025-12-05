/**
 ******************************************************************************
 * @file    safety_selftest.h
 * @brief   Lightweight Self-Test Interface
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Startup and runtime self-tests for functional safety
 * Target: STM32F407VGT6
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

#ifndef __SAFETY_SELFTEST_H
#define __SAFETY_SELFTEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "safety_config.h"

/* ============================================================================
 * Self-Test Mode
 * ============================================================================*/

typedef enum {
    SELFTEST_MODE_STARTUP,      /* Complete test at startup */
    SELFTEST_MODE_RUNTIME       /* Incremental test during runtime */
} selftest_mode_t;

/* ============================================================================
 * Self-Test Result
 * ============================================================================*/

typedef enum {
    SELFTEST_PASS           = 0x00U,
    SELFTEST_FAIL_CPU       = 0x01U,
    SELFTEST_FAIL_RAM       = 0x02U,
    SELFTEST_FAIL_FLASH     = 0x03U,
    SELFTEST_FAIL_CLOCK     = 0x04U,
    SELFTEST_FAIL_CRC       = 0x05U,
    SELFTEST_IN_PROGRESS    = 0xFEU,
    SELFTEST_NOT_RUN        = 0xFFU
} selftest_result_t;

/* ============================================================================
 * Flash CRC Context (for incremental verification)
 * ============================================================================*/

typedef struct {
    uint32_t current_offset;    /* Current offset in flash */
    uint32_t accumulated_crc;   /* Accumulated CRC value */
    uint32_t total_size;        /* Total size to check */
    uint32_t block_size;        /* Size per check cycle */
    bool     in_progress;       /* Check in progress */
    bool     completed;         /* Check completed */
} flash_crc_context_t;

/* ============================================================================
 * Function Prototypes
 * ============================================================================*/

/**
 * @brief Initialize self-test module
 * @retval safety_status_t Status
 */
safety_status_t Safety_SelfTest_Init(void);

/**
 * @brief Run all startup self-tests
 * @retval selftest_result_t Test result
 */
selftest_result_t Safety_SelfTest_RunStartup(void);

/**
 * @brief Run CPU register test
 * @retval selftest_result_t Test result
 */
selftest_result_t Safety_SelfTest_CPU(void);

/**
 * @brief Run RAM test
 * @param mode Test mode (startup or runtime)
 * @retval selftest_result_t Test result
 */
selftest_result_t Safety_SelfTest_RAM(selftest_mode_t mode);

/**
 * @brief Run Flash CRC verification
 * @param mode Test mode (startup = full, runtime = incremental)
 * @retval selftest_result_t Test result
 */
selftest_result_t Safety_SelfTest_FlashCRC(selftest_mode_t mode);

/**
 * @brief Continue incremental Flash CRC check
 * @note Called periodically from safety thread
 * @retval selftest_result_t Test result (IN_PROGRESS until complete)
 */
selftest_result_t Safety_SelfTest_FlashCRC_Continue(void);

/**
 * @brief Run clock frequency verification
 * @retval selftest_result_t Test result
 */
selftest_result_t Safety_SelfTest_Clock(void);

/**
 * @brief Get Flash CRC context for diagnostics
 * @retval const flash_crc_context_t* Context pointer
 */
const flash_crc_context_t* Safety_SelfTest_GetFlashCRCContext(void);

/**
 * @brief Reset Flash CRC context for new check cycle
 */
void Safety_SelfTest_ResetFlashCRC(void);

#ifdef __cplusplus
}
#endif

#endif /* __SAFETY_SELFTEST_H */
