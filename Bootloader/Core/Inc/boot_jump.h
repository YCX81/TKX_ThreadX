/**
 ******************************************************************************
 * @file    boot_jump.h
 * @brief   LAT1182 Compliant Safe Jump to Application
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * This module implements safe jump from Bootloader to Application
 * following ST LAT1182 Application Note guidelines.
 *
 * Key requirements (LAT1182):
 * 1. Use __stackless keyword to avoid stack operations
 * 2. Disable all interrupts before jump
 * 3. Clear all NVIC interrupt enable and pending bits
 * 4. Disable SysTick
 * 5. Set VTOR to application vector table
 * 6. Set MSP to application stack pointer
 * 7. Jump to application Reset Handler
 *
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

#ifndef __BOOT_JUMP_H
#define __BOOT_JUMP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "boot_config.h"

/* ============================================================================
 * Function Prototypes
 * ============================================================================*/

/**
 * @brief  Jump to Application (LAT1182 compliant)
 * @note   This function never returns
 * @note   Uses __stackless to prevent stack corruption issues
 */
void Boot_JumpToApplication(void);

/**
 * @brief  Verify Application CRC before jump
 * @retval BOOT_OK if CRC matches, error code otherwise
 */
boot_status_t Boot_VerifyAppCRC(void);

/**
 * @brief  Check if valid application exists
 * @retval true if valid stack pointer found at APP_FLASH_START
 */
bool Boot_IsValidApplication(void);

#ifdef __cplusplus
}
#endif

#endif /* __BOOT_JUMP_H */
