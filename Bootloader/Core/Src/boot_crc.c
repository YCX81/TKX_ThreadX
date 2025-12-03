/**
 ******************************************************************************
 * @file    boot_crc.c
 * @brief   CRC Calculation Implementation
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Uses STM32 hardware CRC unit for CRC32 calculation
 * Software implementation for CRC16
 *
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "boot_crc.h"
#include "stm32f4xx_hal.h"

/* Private variables ---------------------------------------------------------*/
static CRC_HandleTypeDef hcrc;

/* ============================================================================
 * CRC32 using Hardware CRC Unit
 * ============================================================================*/

/**
 * @brief  Initialize CRC hardware unit
 */
void Boot_CRC_Init(void)
{
    /* Enable CRC clock */
    __HAL_RCC_CRC_CLK_ENABLE();

    /* Configure CRC */
    hcrc.Instance = CRC;

    if (HAL_CRC_Init(&hcrc) != HAL_OK)
    {
        /* Initialization Error - should not happen */
        while (1)
        {
            __NOP();
        }
    }
}

/**
 * @brief  Reset CRC hardware unit
 */
void Boot_CRC_Reset(void)
{
    __HAL_CRC_DR_RESET(&hcrc);
}

/**
 * @brief  Calculate CRC32 using hardware CRC unit
 * @note   STM32 CRC unit processes 32-bit words
 *         This function handles byte alignment
 */
uint32_t Boot_CRC32_Calculate(const uint8_t *data, uint32_t length)
{
    uint32_t crc;
    uint32_t i;
    uint32_t words;
    uint32_t remaining;
    const uint32_t *data32;

    if (data == NULL || length == 0)
    {
        return 0;
    }

    /* Reset CRC unit */
    Boot_CRC_Reset();

    /* Calculate number of complete 32-bit words */
    words = length / 4;
    remaining = length % 4;

    /* Process complete words */
    data32 = (const uint32_t *)data;
    for (i = 0; i < words; i++)
    {
        CRC->DR = data32[i];
    }

    /* Handle remaining bytes */
    if (remaining > 0)
    {
        uint32_t last_word = 0;
        const uint8_t *remaining_data = data + (words * 4);

        for (i = 0; i < remaining; i++)
        {
            last_word |= ((uint32_t)remaining_data[i]) << (i * 8);
        }

        /* Pad with 0xFF for remaining bytes (like erased Flash) */
        for (i = remaining; i < 4; i++)
        {
            last_word |= 0xFFUL << (i * 8);
        }

        CRC->DR = last_word;
    }

    /* Read calculated CRC */
    crc = CRC->DR;

    return crc;
}

/**
 * @brief  Calculate CRC32 for a memory region
 */
uint32_t Boot_CRC32_Region(uint32_t start_addr, uint32_t end_addr)
{
    uint32_t length;

    if (end_addr <= start_addr)
    {
        return 0;
    }

    length = end_addr - start_addr;

    return Boot_CRC32_Calculate((const uint8_t *)start_addr, length);
}

/**
 * @brief  Verify CRC32 of a region against stored value
 */
boot_status_t Boot_CRC32_Verify(uint32_t start_addr, uint32_t length, uint32_t crc_addr)
{
    uint32_t calc_crc;
    uint32_t stored_crc;

    /* Calculate CRC */
    calc_crc = Boot_CRC32_Calculate((const uint8_t *)start_addr, length);

    /* Read stored CRC */
    stored_crc = *(volatile uint32_t *)crc_addr;

    /* Compare */
    if (calc_crc != stored_crc)
    {
        return BOOT_ERROR_CRC;
    }

    return BOOT_OK;
}

/* ============================================================================
 * CRC16 Software Implementation
 * ============================================================================*/

/**
 * @brief  Calculate CRC16 CCITT
 * @note   Software implementation since STM32F4 CRC unit only supports CRC32
 */
uint16_t Boot_CRC16_Calculate(const uint8_t *data, uint32_t length)
{
    uint16_t crc = CRC16_INIT_VALUE;
    uint32_t i;
    uint8_t j;

    if (data == NULL || length == 0)
    {
        return 0;
    }

    for (i = 0; i < length; i++)
    {
        crc ^= ((uint16_t)data[i]) << 8;

        for (j = 0; j < 8; j++)
        {
            if (crc & 0x8000)
            {
                crc = (crc << 1) ^ CRC16_POLYNOMIAL;
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}
