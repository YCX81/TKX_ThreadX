/**
 ******************************************************************************
 * @file    safety_stack.h
 * @brief   Stack Monitoring Interface
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * ThreadX integrated stack monitoring for runtime safety
 * Target: STM32F407VGT6
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

#ifndef __SAFETY_STACK_H
#define __SAFETY_STACK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "safety_config.h"
#include "tx_api.h"

/* ============================================================================
 * Stack Information Structure
 * ============================================================================*/

typedef struct {
    TX_THREAD   *thread;            /* Thread pointer */
    const char  *name;              /* Thread name */
    ULONG       stack_size;         /* Total stack size */
    ULONG       stack_used;         /* Currently used */
    ULONG       stack_available;    /* Available space */
    ULONG       stack_highest;      /* Highest water mark */
    uint8_t     usage_percent;      /* Usage percentage */
    bool        warning;            /* Warning threshold reached */
    bool        critical;           /* Critical threshold reached */
} stack_info_t;

/* Maximum number of monitored threads */
#define MAX_MONITORED_THREADS   8

/* ============================================================================
 * Function Prototypes
 * ============================================================================*/

/**
 * @brief Initialize stack monitoring
 * @retval safety_status_t Status
 */
safety_status_t Safety_Stack_Init(void);

/**
 * @brief Register a thread for stack monitoring
 * @param thread Thread to monitor
 * @retval safety_status_t Status
 */
safety_status_t Safety_Stack_RegisterThread(TX_THREAD *thread);

/**
 * @brief Unregister a thread from monitoring
 * @param thread Thread to unregister
 * @retval safety_status_t Status
 */
safety_status_t Safety_Stack_UnregisterThread(TX_THREAD *thread);

/**
 * @brief Check all registered thread stacks
 * @retval safety_status_t Status (ERROR if any critical)
 */
safety_status_t Safety_Stack_CheckAll(void);

/**
 * @brief Get stack info for a thread
 * @param thread Thread to query
 * @param info Pointer to store info
 * @retval safety_status_t Status
 */
safety_status_t Safety_Stack_GetInfo(TX_THREAD *thread, stack_info_t *info);

/**
 * @brief Get number of monitored threads
 * @retval uint32_t Number of threads
 */
uint32_t Safety_Stack_GetMonitoredCount(void);

/**
 * @brief Get info by index
 * @param index Index in monitored list
 * @param info Pointer to store info
 * @retval safety_status_t Status
 */
safety_status_t Safety_Stack_GetInfoByIndex(uint32_t index, stack_info_t *info);

/**
 * @brief ThreadX stack error callback
 * @param thread_ptr Thread with stack error
 * @note Registered with ThreadX
 */
void Safety_Stack_ErrorCallback(TX_THREAD *thread_ptr);

#ifdef __cplusplus
}
#endif

#endif /* __SAFETY_STACK_H */
