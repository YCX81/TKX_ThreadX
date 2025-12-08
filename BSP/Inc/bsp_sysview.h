/**
 ******************************************************************************
 * @file    bsp_sysview.h
 * @brief   Segger SystemView Integration Interface
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * SystemView integration for ThreadX RTOS tracing
 * Target: STM32F407VGT6
 *
 ******************************************************************************
 */

#ifndef __BSP_SYSVIEW_H
#define __BSP_SYSVIEW_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * SystemView Configuration
 * ============================================================================*/

/* Enable/disable SystemView tracing */
#ifndef SYSVIEW_ENABLED
#define SYSVIEW_ENABLED         1       /* SystemView enabled for ThreadX tracing */
#endif

/* ============================================================================
 * Public Function Prototypes
 * ============================================================================*/

#if SYSVIEW_ENABLED

/**
 * @brief Initialize SystemView for ThreadX tracing
 * @note Call before starting ThreadX kernel
 */
void BSP_SysView_Init(void);

/**
 * @brief Start SystemView recording
 */
void BSP_SysView_Start(void);

/**
 * @brief Stop SystemView recording
 */
void BSP_SysView_Stop(void);

/**
 * @brief Record a user-defined event
 * @param id Event ID (0-31)
 * @param msg Event message string
 */
void BSP_SysView_RecordEvent(uint32_t id, const char *msg);

/**
 * @brief Record a value event
 * @param id Event ID (0-31)
 * @param value Value to record
 */
void BSP_SysView_RecordValue(uint32_t id, uint32_t value);

/**
 * @brief Enter ISR recording
 * @param isr_id ISR identifier
 */
void BSP_SysView_EnterISR(uint32_t isr_id);

/**
 * @brief Exit ISR recording
 */
void BSP_SysView_ExitISR(void);

/**
 * @brief Enable DWT cycle counter for timestamps
 * @note Called automatically by BSP_SysView_Init
 */
void BSP_SysView_EnableCycleCounter(void);

#else /* !SYSVIEW_ENABLED */

/* Empty macros when SystemView is disabled */
#define BSP_SysView_Init()
#define BSP_SysView_Start()
#define BSP_SysView_Stop()
#define BSP_SysView_RecordEvent(id, msg)
#define BSP_SysView_RecordValue(id, value)
#define BSP_SysView_EnterISR(isr_id)
#define BSP_SysView_ExitISR()
#define BSP_SysView_EnableCycleCounter()

#endif /* SYSVIEW_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* __BSP_SYSVIEW_H */
