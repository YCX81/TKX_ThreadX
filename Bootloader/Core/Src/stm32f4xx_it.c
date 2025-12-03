/**
 ******************************************************************************
 * @file    stm32f4xx_it.c
 * @brief   Interrupt Service Routines for Bootloader
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Minimal interrupt handlers for Bootloader.
 * Faults trigger safe state entry.
 *
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stm32f4xx_it.h"

/* External variables --------------------------------------------------------*/
extern void Boot_EnterSafeState(void);

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/

/**
 * @brief  Non Maskable Interrupt Handler
 */
void NMI_Handler(void)
{
    /* Enter safe state on NMI */
    Boot_EnterSafeState();
}

/**
 * @brief  Hard Fault Handler
 */
void HardFault_Handler(void)
{
    /* Enter safe state on Hard Fault */
    Boot_EnterSafeState();
}

/**
 * @brief  Memory Management Fault Handler
 */
void MemManage_Handler(void)
{
    /* Enter safe state on Memory Fault */
    Boot_EnterSafeState();
}

/**
 * @brief  Bus Fault Handler
 */
void BusFault_Handler(void)
{
    /* Enter safe state on Bus Fault */
    Boot_EnterSafeState();
}

/**
 * @brief  Usage Fault Handler
 */
void UsageFault_Handler(void)
{
    /* Enter safe state on Usage Fault */
    Boot_EnterSafeState();
}

/**
 * @brief  Supervisor Call Handler
 */
void SVC_Handler(void)
{
    /* Not used in Bootloader */
}

/**
 * @brief  Debug Monitor Handler
 */
void DebugMon_Handler(void)
{
    /* Not used in Bootloader */
}

/**
 * @brief  Pending SV Handler
 */
void PendSV_Handler(void)
{
    /* Not used in Bootloader */
}

/**
 * @brief  SysTick Handler
 */
void SysTick_Handler(void)
{
    HAL_IncTick();
}
