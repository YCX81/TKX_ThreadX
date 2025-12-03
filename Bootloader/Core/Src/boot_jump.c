/**
 ******************************************************************************
 * @file    boot_jump.c
 * @brief   LAT1182 Compliant Safe Jump to Application
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Implementation follows ST LAT1182 Application Note for safe jump
 * from Bootloader to Application in functional safety systems.
 *
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "boot_jump.h"
#include "boot_crc.h"
#include "stm32f4xx_hal.h"

/* Private defines -----------------------------------------------------------*/
#define NVIC_INT_CTRL_REGS      8U      /* Number of NVIC control registers */

/* Valid stack pointer range for STM32F407 */
#define VALID_SP_MIN            0x20000000UL
#define VALID_SP_MAX            0x20020000UL

/* ============================================================================
 * LAT1182 Compliant Jump to Application
 *
 * This function is marked with:
 * - #pragma optimize=none : Prevent compiler optimization that might
 *                           reorder critical operations
 * - __stackless          : Prevent any stack operations that could
 *                           cause issues when changing MSP
 *
 * Steps:
 * 1. Disable global interrupts
 * 2. Disable all NVIC interrupts
 * 3. Clear all NVIC pending interrupts
 * 4. Disable SysTick
 * 5. Clear SysTick pending bit
 * 6. Set VTOR to application vector table
 * 7. Memory barriers (DSB, ISB)
 * 8. Set MSP to application stack pointer
 * 9. Memory barriers (DSB, ISB)
 * 10. Jump to application Reset Handler
 * ============================================================================*/

#pragma optimize=none
__stackless void Boot_JumpToApplication(void)
{
    /* Application vector table pointer */
    volatile uint32_t *app_vector = (volatile uint32_t *)APP_FLASH_START;

    /* Application stack pointer (first word of vector table) */
    uint32_t app_sp = app_vector[0];

    /* Application Reset Handler address (second word of vector table) */
    uint32_t app_reset = app_vector[1];

    /* Function pointer type for Reset Handler */
    typedef void (*pFunction)(void);
    pFunction JumpToApp;

    /* ========================================================================
     * Step 1: Disable global interrupts
     * ======================================================================*/
    __disable_irq();

    /* ========================================================================
     * Step 2 & 3: Disable and clear all NVIC interrupts
     * ======================================================================*/
    for (uint32_t i = 0; i < NVIC_INT_CTRL_REGS; i++)
    {
        /* Disable all interrupts in this register */
        NVIC->ICER[i] = 0xFFFFFFFFUL;

        /* Clear all pending interrupts in this register */
        NVIC->ICPR[i] = 0xFFFFFFFFUL;
    }

    /* ========================================================================
     * Step 4: Disable SysTick
     * ======================================================================*/
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    /* ========================================================================
     * Step 5: Clear SysTick pending bit in SCB
     * ======================================================================*/
    SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;

    /* ========================================================================
     * Step 6: Set VTOR to application vector table
     * ======================================================================*/
    SCB->VTOR = APP_FLASH_START;

    /* ========================================================================
     * Step 7: Memory barriers to ensure VTOR write completes
     * ======================================================================*/
    __DSB();
    __ISB();

    /* ========================================================================
     * Step 8: Set MSP to application stack pointer
     * Note: This is the critical step - any stack access after this
     *       will use the new stack pointer
     * ======================================================================*/
    __set_MSP(app_sp);

    /* ========================================================================
     * Step 9: Memory barriers to ensure MSP write completes
     * ======================================================================*/
    __DSB();
    __ISB();

    /* ========================================================================
     * Step 10: Jump to application Reset Handler
     * ======================================================================*/
    JumpToApp = (pFunction)app_reset;
    JumpToApp();

    /* Should never reach here */
    while (1)
    {
        __NOP();
    }
}

/**
 * @brief  Verify Application CRC
 */
boot_status_t Boot_VerifyAppCRC(void)
{
    uint32_t stored_crc;
    uint32_t calc_crc;
    uint32_t app_size;

    /* First check if valid application exists */
    if (!Boot_IsValidApplication())
    {
        return BOOT_ERROR;
    }

    /* Read stored CRC from end of application area */
    stored_crc = *(volatile uint32_t *)APP_CRC_ADDR;

    /* Calculate CRC of application (excluding CRC itself) */
    app_size = APP_CRC_ADDR - APP_FLASH_START;
    calc_crc = Boot_CRC32_Calculate((uint8_t *)APP_FLASH_START, app_size);

    /* Compare CRCs */
    if (calc_crc != stored_crc)
    {
        return BOOT_ERROR_CRC;
    }

    return BOOT_OK;
}

/**
 * @brief  Check if valid application exists
 */
bool Boot_IsValidApplication(void)
{
    volatile uint32_t *app_vector = (volatile uint32_t *)APP_FLASH_START;
    uint32_t app_sp = app_vector[0];
    uint32_t app_reset = app_vector[1];

    /* Check 1: Stack pointer should be in valid RAM range */
    if (app_sp < VALID_SP_MIN || app_sp > VALID_SP_MAX)
    {
        return false;
    }

    /* Check 2: Stack pointer should be word-aligned */
    if ((app_sp & 0x03) != 0)
    {
        return false;
    }

    /* Check 3: Reset Handler should be in valid Flash range */
    if (app_reset < APP_FLASH_START || app_reset > APP_FLASH_END)
    {
        return false;
    }

    /* Check 4: Reset Handler should be thumb instruction (bit 0 set) */
    if ((app_reset & 0x01) == 0)
    {
        return false;
    }

    /* Check 5: Application area should not be erased (all 0xFF) */
    if (app_sp == 0xFFFFFFFF || app_reset == 0xFFFFFFFF)
    {
        return false;
    }

    return true;
}
