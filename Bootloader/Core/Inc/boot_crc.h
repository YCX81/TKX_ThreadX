/**
 ******************************************************************************
 * @file    boot_crc.h
 * @brief   CRC Calculation for Functional Safety Bootloader
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * CRC functions using STM32 hardware CRC unit
 * Supports CRC32 (Ethernet polynomial) and CRC16 (CCITT)
 *
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

#ifndef __BOOT_CRC_H
#define __BOOT_CRC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "boot_config.h"

/* ============================================================================
 * CRC Configuration
 * ============================================================================*/

/* CRC32 Ethernet Polynomial: 0x04C11DB7 (used by STM32 hardware CRC) */
#define CRC32_POLYNOMIAL        0x04C11DB7UL
#define CRC32_INIT_VALUE        0xFFFFFFFFUL

/* CRC16 CCITT Polynomial: 0x1021 */
#define CRC16_POLYNOMIAL        0x1021U
#define CRC16_INIT_VALUE        0xFFFFU

/* ============================================================================
 * Function Prototypes
 * ============================================================================*/

/**
 * @brief  Initialize CRC hardware unit
 */
void Boot_CRC_Init(void);

/**
 * @brief  Calculate CRC32 using hardware CRC unit
 * @param  data: Pointer to data buffer
 * @param  length: Length of data in bytes
 * @retval Calculated CRC32 value
 */
uint32_t Boot_CRC32_Calculate(const uint8_t *data, uint32_t length);

/**
 * @brief  Calculate CRC32 for a memory region
 * @param  start_addr: Start address
 * @param  end_addr: End address (exclusive)
 * @retval Calculated CRC32 value
 */
uint32_t Boot_CRC32_Region(uint32_t start_addr, uint32_t end_addr);

/**
 * @brief  Verify CRC32 of a region against stored value
 * @param  start_addr: Start address of data
 * @param  length: Length of data
 * @param  crc_addr: Address where expected CRC is stored
 * @retval BOOT_OK if CRC matches, BOOT_ERROR_CRC otherwise
 */
boot_status_t Boot_CRC32_Verify(uint32_t start_addr, uint32_t length, uint32_t crc_addr);

/**
 * @brief  Calculate CRC16 (software implementation)
 * @param  data: Pointer to data buffer
 * @param  length: Length of data in bytes
 * @retval Calculated CRC16 value
 */
uint16_t Boot_CRC16_Calculate(const uint8_t *data, uint32_t length);

/**
 * @brief  Reset CRC hardware unit
 */
void Boot_CRC_Reset(void);

#ifdef __cplusplus
}
#endif

#endif /* __BOOT_CRC_H */
