/**
 ******************************************************************************
 * @file    safety_flow.c
 * @brief   Program Flow Monitoring Implementation
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "safety_flow.h"
#include "stm32f4xx_hal.h"

/* Private defines -----------------------------------------------------------*/
/* Signature update algorithm using CRC-like XOR and rotate */
#define FLOW_UPDATE_SIGNATURE(sig, cp) \
    ((((sig) << 1) | ((sig) >> 31)) ^ ((uint32_t)(cp) * 0x9E3779B9UL))

/* Private variables ---------------------------------------------------------*/
static flow_context_t s_flow_ctx;

/* ============================================================================
 * Implementation
 * ============================================================================*/

void Safety_Flow_Init(void)
{
    s_flow_ctx.signature = FLOW_SIGNATURE_SEED;
    s_flow_ctx.expected_signature = 0;
    s_flow_ctx.checkpoint_count = 0;
    s_flow_ctx.last_checkpoint = 0;
    s_flow_ctx.last_checkpoint_time = 0;
    s_flow_ctx.sequence_complete = false;
    s_flow_ctx.error_detected = false;
}

void Safety_Flow_Checkpoint(uint8_t checkpoint)
{
    /* Update signature using checkpoint value */
    s_flow_ctx.signature = FLOW_UPDATE_SIGNATURE(s_flow_ctx.signature, checkpoint);

    /* Record checkpoint info */
    s_flow_ctx.last_checkpoint = checkpoint;
    s_flow_ctx.last_checkpoint_time = HAL_GetTick();
    s_flow_ctx.checkpoint_count++;

    /* Check if expected signature matches (if set) */
    if (s_flow_ctx.expected_signature != 0)
    {
        if (s_flow_ctx.signature == s_flow_ctx.expected_signature)
        {
            s_flow_ctx.sequence_complete = true;
        }
    }
}

bool Safety_Flow_Verify(void)
{
    /* If expected signature is set, verify it */
    if (s_flow_ctx.expected_signature != 0)
    {
        if (s_flow_ctx.signature != s_flow_ctx.expected_signature)
        {
            s_flow_ctx.error_detected = true;
            return false;
        }
    }

    /* Check that checkpoints are being hit (liveness check) */
    if (s_flow_ctx.checkpoint_count == 0)
    {
        s_flow_ctx.error_detected = true;
        return false;
    }

    /* Reset for next verification cycle */
    s_flow_ctx.checkpoint_count = 0;

    return true;
}

void Safety_Flow_Reset(void)
{
    s_flow_ctx.signature = FLOW_SIGNATURE_SEED;
    s_flow_ctx.checkpoint_count = 0;
    s_flow_ctx.last_checkpoint = 0;
    s_flow_ctx.sequence_complete = false;
    s_flow_ctx.error_detected = false;
}

uint32_t Safety_Flow_GetSignature(void)
{
    return s_flow_ctx.signature;
}

void Safety_Flow_SetExpected(uint32_t expected)
{
    s_flow_ctx.expected_signature = expected;
}

const flow_context_t* Safety_Flow_GetContext(void)
{
    return &s_flow_ctx;
}

bool Safety_Flow_CheckpointRecent(uint8_t checkpoint, uint32_t timeout_ms)
{
    if (s_flow_ctx.last_checkpoint != checkpoint)
    {
        return false;
    }

    uint32_t current_time = HAL_GetTick();
    uint32_t elapsed = current_time - s_flow_ctx.last_checkpoint_time;

    return (elapsed <= timeout_ms);
}
