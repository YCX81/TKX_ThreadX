/**
 ******************************************************************************
 * @file    bsp_sysview.c
 * @brief   Segger SystemView Integration Implementation
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

/* Includes ------------------------------------------------------------------*/
#include "bsp_sysview.h"

#if SYSVIEW_ENABLED

#include "SEGGER_SYSVIEW.h"
#include "stm32f4xx.h"

/* ============================================================================
 * Private Defines
 * ============================================================================*/

/* DWT (Debug Watchpoint and Trace) registers */
#define DWT_CTRL    (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT  (*(volatile uint32_t *)0xE0001004)
#define SCB_DEMCR   (*(volatile uint32_t *)0xE000EDFC)

/* DWT_CTRL bits */
#define DWT_CTRL_CYCCNTENA  (1UL << 0)

/* SCB_DEMCR bits */
#define SCB_DEMCR_TRCENA    (1UL << 24)

/* ============================================================================
 * External Functions
 * ============================================================================*/

extern void SEGGER_SYSVIEW_Conf(void);

/* ============================================================================
 * Public Functions
 * ============================================================================*/

void BSP_SysView_Init(void)
{
    /* Enable DWT cycle counter for high-resolution timestamps */
    BSP_SysView_EnableCycleCounter();

    /* Configure SystemView */
    SEGGER_SYSVIEW_Conf();
}

void BSP_SysView_Start(void)
{
    SEGGER_SYSVIEW_Start();
}

void BSP_SysView_Stop(void)
{
    SEGGER_SYSVIEW_Stop();
}

void BSP_SysView_RecordEvent(uint32_t id, const char *msg)
{
    if (id < 32 && msg != NULL)
    {
        SEGGER_SYSVIEW_Print(msg);
    }
}

void BSP_SysView_RecordValue(uint32_t id, uint32_t value)
{
    if (id < 32)
    {
        SEGGER_SYSVIEW_RecordU32(id, value);
    }
}

void BSP_SysView_EnterISR(uint32_t isr_id)
{
    SEGGER_SYSVIEW_RecordEnterISR();
}

void BSP_SysView_ExitISR(void)
{
    SEGGER_SYSVIEW_RecordExitISR();
}

void BSP_SysView_EnableCycleCounter(void)
{
    /* Enable trace and debug block */
    SCB_DEMCR |= SCB_DEMCR_TRCENA;

    /* Reset cycle counter */
    DWT_CYCCNT = 0;

    /* Enable cycle counter */
    DWT_CTRL |= DWT_CTRL_CYCCNTENA;
}

#endif /* SYSVIEW_ENABLED */
