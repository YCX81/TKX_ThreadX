/**
 ******************************************************************************
 * @file    safety_flow.h
 * @brief   Program Flow Monitoring Interface
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Program flow monitoring using signature accumulation
 * Target: STM32F407VGT6
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

#ifndef __SAFETY_FLOW_H
#define __SAFETY_FLOW_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "safety_config.h"

/* ============================================================================
 * Flow Checkpoint Definitions
 * ============================================================================*/

/* Checkpoints defined in shared_config.h:
 * PFM_CP_APP_INIT          = 0x10
 * PFM_CP_APP_SAFETY_MONITOR = 0x11
 * PFM_CP_APP_WATCHDOG_FEED  = 0x12
 * PFM_CP_APP_SELFTEST_START = 0x13
 * PFM_CP_APP_SELFTEST_END   = 0x14
 * PFM_CP_APP_MAIN_LOOP      = 0x15
 * PFM_CP_APP_COMM_HANDLER   = 0x16
 * PFM_CP_APP_PARAM_CHECK    = 0x17
 */

/* ============================================================================
 * Flow Monitor Context
 * ============================================================================*/

typedef struct {
    uint32_t signature;             /* Current accumulated signature */
    uint32_t expected_signature;    /* Expected signature after sequence */
    uint32_t checkpoint_count;      /* Number of checkpoints hit */
    uint32_t last_checkpoint;       /* Last checkpoint value */
    uint32_t last_checkpoint_time;  /* Timestamp of last checkpoint */
    bool     sequence_complete;     /* Expected sequence completed */
    bool     error_detected;        /* Flow error detected */
} flow_context_t;

/* ============================================================================
 * Function Prototypes
 * ============================================================================*/

/**
 * @brief Initialize program flow monitor
 */
void Safety_Flow_Init(void);

/**
 * @brief Record a checkpoint
 * @param checkpoint Checkpoint value (PFM_CP_xxx)
 */
void Safety_Flow_Checkpoint(uint8_t checkpoint);

/**
 * @brief Verify flow signature
 * @retval bool true if valid
 */
bool Safety_Flow_Verify(void);

/**
 * @brief Reset flow monitor for new sequence
 */
void Safety_Flow_Reset(void);

/**
 * @brief Get current signature value
 * @retval uint32_t Current signature
 */
uint32_t Safety_Flow_GetSignature(void);

/**
 * @brief Set expected signature for verification
 * @param expected Expected signature value
 */
void Safety_Flow_SetExpected(uint32_t expected);

/**
 * @brief Get flow context for diagnostics
 * @retval const flow_context_t* Context pointer
 */
const flow_context_t* Safety_Flow_GetContext(void);

/**
 * @brief Check if checkpoint was reached within timeout
 * @param checkpoint Checkpoint to check
 * @param timeout_ms Maximum allowed time since checkpoint
 * @retval bool true if checkpoint reached within timeout
 */
bool Safety_Flow_CheckpointRecent(uint8_t checkpoint, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* __SAFETY_FLOW_H */
