/**
 ******************************************************************************
 * @file    app_main.h
 * @brief   Application Main Interface
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Application layer entry point and thread management
 * Target: STM32F407VGT6
 *
 ******************************************************************************
 */

#ifndef __APP_MAIN_H
#define __APP_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "tx_api.h"
#include "shared_config.h"

/* ============================================================================
 * Application Thread Configuration
 * ============================================================================*/

#define APP_MAIN_THREAD_STACK_SIZE      4096U
#define APP_MAIN_THREAD_PRIORITY        5U
#define APP_MAIN_THREAD_PREEMPT_THRESH  5U

#define APP_COMM_THREAD_STACK_SIZE      2048U
#define APP_COMM_THREAD_PRIORITY        10U
#define APP_COMM_THREAD_PREEMPT_THRESH  10U

/* ============================================================================
 * Function Prototypes
 * ============================================================================*/

/**
 * @brief Pre-initialization before ThreadX starts
 * @note Called from main() before MX_ThreadX_Init()
 * @retval shared_status_t Status
 */
shared_status_t App_PreInit(void);

/**
 * @brief Create application threads
 * @param byte_pool ThreadX byte pool
 * @retval UINT ThreadX status
 */
UINT App_CreateThreads(TX_BYTE_POOL *byte_pool);

/**
 * @brief Main application thread entry
 * @param thread_input Thread input parameter
 */
void App_MainThreadEntry(ULONG thread_input);

/**
 * @brief Communication thread entry
 * @param thread_input Thread input parameter
 */
void App_CommThreadEntry(ULONG thread_input);

/**
 * @brief Get main thread handle
 * @retval TX_THREAD* Thread pointer
 */
TX_THREAD* App_GetMainThread(void);

/**
 * @brief Get communication thread handle
 * @retval TX_THREAD* Thread pointer
 */
TX_THREAD* App_GetCommThread(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_MAIN_H */
