/**
 ******************************************************************************
 * @file    safety_stack.c
 * @brief   Stack Monitoring Implementation
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "safety_stack.h"
#include "safety_core.h"
#include <string.h>

/* Private variables ---------------------------------------------------------*/
static TX_THREAD *s_monitored_threads[MAX_MONITORED_THREADS];
static uint32_t s_monitored_count = 0;
static bool s_initialized = false;

/* ============================================================================
 * Private Function Prototypes
 * ============================================================================*/
static uint32_t CalculateStackUsage(TX_THREAD *thread);

/* ============================================================================
 * Implementation
 * ============================================================================*/

safety_status_t Safety_Stack_Init(void)
{
    /* Clear monitored thread list */
    memset(s_monitored_threads, 0, sizeof(s_monitored_threads));
    s_monitored_count = 0;
    s_initialized = true;

    return SAFETY_OK;
}

safety_status_t Safety_Stack_RegisterThread(TX_THREAD *thread)
{
    if (!s_initialized || thread == NULL)
    {
        return SAFETY_INVALID_PARAM;
    }

    if (s_monitored_count >= MAX_MONITORED_THREADS)
    {
        return SAFETY_ERROR;
    }

    /* Check if already registered */
    for (uint32_t i = 0; i < s_monitored_count; i++)
    {
        if (s_monitored_threads[i] == thread)
        {
            return SAFETY_OK; /* Already registered */
        }
    }

    /* Add to list */
    s_monitored_threads[s_monitored_count++] = thread;

    return SAFETY_OK;
}

safety_status_t Safety_Stack_UnregisterThread(TX_THREAD *thread)
{
    if (!s_initialized || thread == NULL)
    {
        return SAFETY_INVALID_PARAM;
    }

    /* Find and remove from list */
    for (uint32_t i = 0; i < s_monitored_count; i++)
    {
        if (s_monitored_threads[i] == thread)
        {
            /* Shift remaining entries */
            for (uint32_t j = i; j < s_monitored_count - 1; j++)
            {
                s_monitored_threads[j] = s_monitored_threads[j + 1];
            }
            s_monitored_count--;
            s_monitored_threads[s_monitored_count] = NULL;
            return SAFETY_OK;
        }
    }

    return SAFETY_ERROR; /* Not found */
}

safety_status_t Safety_Stack_CheckAll(void)
{
    if (!s_initialized)
    {
        return SAFETY_ERROR;
    }

    safety_status_t overall_status = SAFETY_OK;
    stack_info_t info;

    for (uint32_t i = 0; i < s_monitored_count; i++)
    {
        TX_THREAD *thread = s_monitored_threads[i];
        if (thread != NULL)
        {
            safety_status_t status = Safety_Stack_GetInfo(thread, &info);
            if (status == SAFETY_OK)
            {
                if (info.critical)
                {
                    Safety_ReportError(SAFETY_ERR_STACK_OVERFLOW,
                                     (uint32_t)thread,
                                     info.usage_percent);
                    overall_status = SAFETY_ERROR;
                }
                else if (info.warning)
                {
                    /* Log warning but continue */
                }
            }
        }
    }

    return overall_status;
}

safety_status_t Safety_Stack_GetInfo(TX_THREAD *thread, stack_info_t *info)
{
    if (thread == NULL || info == NULL)
    {
        return SAFETY_INVALID_PARAM;
    }

    /* Get stack information from ThreadX */
    CHAR *thread_name;
    TX_THREAD *next_thread;
    UINT state;
    ULONG run_count;
    UINT priority;
    UINT preemption_threshold;
    ULONG time_slice;

    /* Get thread info */
    UINT tx_status = tx_thread_info_get(thread,
                                        &thread_name,
                                        &state,
                                        &run_count,
                                        &priority,
                                        &preemption_threshold,
                                        &time_slice,
                                        &next_thread,
                                        &next_thread);

    if (tx_status != TX_SUCCESS)
    {
        return SAFETY_ERROR;
    }

    /* Fill in basic info */
    info->thread = thread;
    info->name = thread_name;
    info->stack_size = thread->tx_thread_stack_size;

    /* Calculate stack usage */
    info->stack_used = CalculateStackUsage(thread);
    info->stack_available = info->stack_size - info->stack_used;
    info->stack_highest = info->stack_used; /* Simplified */

    /* Calculate percentage */
    if (info->stack_size > 0)
    {
        info->usage_percent = (uint8_t)((info->stack_used * 100) / info->stack_size);
    }
    else
    {
        info->usage_percent = 100;
    }

    /* Check thresholds */
    info->warning = (info->usage_percent >= STACK_WARNING_THRESHOLD);
    info->critical = (info->usage_percent >= STACK_CRITICAL_THRESHOLD);

    return SAFETY_OK;
}

uint32_t Safety_Stack_GetMonitoredCount(void)
{
    return s_monitored_count;
}

safety_status_t Safety_Stack_GetInfoByIndex(uint32_t index, stack_info_t *info)
{
    if (index >= s_monitored_count || info == NULL)
    {
        return SAFETY_INVALID_PARAM;
    }

    return Safety_Stack_GetInfo(s_monitored_threads[index], info);
}

void Safety_Stack_ErrorCallback(TX_THREAD *thread_ptr)
{
    /* Called by ThreadX when stack error detected */
    /* TX_ENABLE_STACK_CHECKING must be enabled */

    Safety_ReportError(SAFETY_ERR_STACK_OVERFLOW, (uint32_t)thread_ptr, 0);
}

/* ============================================================================
 * Private Functions
 * ============================================================================*/

static uint32_t CalculateStackUsage(TX_THREAD *thread)
{
    /* ThreadX fills stack with 0xEF pattern */
    /* Count how many bytes are still 0xEF from stack start */

    uint8_t *stack_start = (uint8_t *)thread->tx_thread_stack_start;
    uint32_t stack_size = thread->tx_thread_stack_size;
    uint32_t unused = 0;

    /* Scan from bottom of stack (high address grows down) */
    for (uint32_t i = 0; i < stack_size; i++)
    {
        if (stack_start[i] == 0xEF)
        {
            unused++;
        }
        else
        {
            break;
        }
    }

    return stack_size - unused;
}

/* ThreadX stack error notification callback */
#ifdef TX_ENABLE_STACK_CHECKING
void tx_application_stack_error_notify(TX_THREAD *thread_ptr)
{
    Safety_Stack_ErrorCallback(thread_ptr);
}
#endif
