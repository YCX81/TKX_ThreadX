/**
 ******************************************************************************
 * @file    storage_flash.h
 * @brief   Flash Storage Driver Header
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Flash storage operations for bootloader configuration and safety parameters.
 * Uses STM32F4 internal Flash Sector 3 (0x0800C000-0x0800FFFF, 16KB).
 *
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

#ifndef __STORAGE_FLASH_H
#define __STORAGE_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "boot_config.h"

/* ============================================================================
 * Flash Storage Status Codes
 * ============================================================================*/

typedef enum {
    STORAGE_OK           = 0x00,
    STORAGE_ERROR        = 0x01,
    STORAGE_BUSY         = 0x02,
    STORAGE_TIMEOUT      = 0x03,
    STORAGE_CRC_ERROR    = 0x04,
    STORAGE_MAGIC_ERROR  = 0x05,
    STORAGE_ERASE_ERROR  = 0x06,
    STORAGE_WRITE_ERROR  = 0x07,
    STORAGE_VERIFY_ERROR = 0x08
} storage_status_t;

/* ============================================================================
 * Flash Sector Definitions (STM32F407)
 * ============================================================================*/

#define FLASH_SECTOR_CONFIG     FLASH_SECTOR_3
#define FLASH_VOLTAGE_RANGE     FLASH_VOLTAGE_RANGE_3  /* 2.7V - 3.6V */

/* Flash operation timeout (milliseconds) */
#define FLASH_TIMEOUT_MS        5000U

/* ============================================================================
 * Function Prototypes - Boot Configuration
 * ============================================================================*/

/**
 * @brief  Initialize Flash storage
 * @retval STORAGE_OK on success
 */
storage_status_t Storage_Init(void);

/**
 * @brief  Read boot configuration from Flash
 * @param  config: Pointer to boot_config_t structure to fill
 * @retval STORAGE_OK on success, error code otherwise
 */
storage_status_t Storage_ReadConfig(boot_config_t *config);

/**
 * @brief  Write boot configuration to Flash
 * @param  config: Pointer to boot_config_t structure to write
 * @retval STORAGE_OK on success, error code otherwise
 */
storage_status_t Storage_WriteConfig(const boot_config_t *config);

/**
 * @brief  Verify boot configuration CRC
 * @param  config: Pointer to boot_config_t structure to verify
 * @retval STORAGE_OK if CRC valid, STORAGE_CRC_ERROR otherwise
 */
storage_status_t Storage_VerifyConfigCRC(const boot_config_t *config);

/* ============================================================================
 * Function Prototypes - Safety Parameters
 * ============================================================================*/

/**
 * @brief  Read safety parameters from Flash
 * @param  params: Pointer to safety_params_t structure to fill
 * @retval STORAGE_OK on success, error code otherwise
 */
storage_status_t Storage_ReadSafetyParams(safety_params_t *params);

/**
 * @brief  Write safety parameters to Flash
 * @note   This should only be called during factory mode
 * @param  params: Pointer to safety_params_t structure to write
 * @retval STORAGE_OK on success, error code otherwise
 */
storage_status_t Storage_WriteSafetyParams(const safety_params_t *params);

/**
 * @brief  Validate safety parameters (CRC + redundancy)
 * @param  params: Pointer to safety_params_t structure to validate
 * @retval STORAGE_OK if valid, error code otherwise
 */
storage_status_t Storage_ValidateSafetyParams(const safety_params_t *params);

/**
 * @brief  Check if safety parameters are present and valid
 * @retval STORAGE_OK if valid parameters exist
 */
storage_status_t Storage_CheckSafetyParamsExist(void);

/* ============================================================================
 * Function Prototypes - Flash Operations
 * ============================================================================*/

/**
 * @brief  Erase Config sector (Sector 3)
 * @retval STORAGE_OK on success, error code otherwise
 */
storage_status_t Storage_EraseSector(void);

/**
 * @brief  Program Flash with data
 * @param  address: Flash address to write
 * @param  data: Pointer to data buffer
 * @param  size: Number of bytes to write (must be multiple of 4)
 * @retval STORAGE_OK on success, error code otherwise
 */
storage_status_t Storage_ProgramFlash(uint32_t address, const uint8_t *data, uint32_t size);

/**
 * @brief  Verify Flash data
 * @param  address: Flash address to verify
 * @param  data: Pointer to expected data
 * @param  size: Number of bytes to verify
 * @retval STORAGE_OK if match, STORAGE_VERIFY_ERROR otherwise
 */
storage_status_t Storage_VerifyFlash(uint32_t address, const uint8_t *data, uint32_t size);

/* ============================================================================
 * Function Prototypes - Factory Mode Flag
 * ============================================================================*/

/**
 * @brief  Set factory mode flag
 * @retval STORAGE_OK on success
 */
storage_status_t Storage_SetFactoryModeFlag(void);

/**
 * @brief  Clear factory mode flag
 * @retval STORAGE_OK on success
 */
storage_status_t Storage_ClearFactoryModeFlag(void);

/**
 * @brief  Check if factory mode is requested
 * @retval 1 if factory mode requested, 0 otherwise
 */
uint32_t Storage_IsFactoryModeRequested(void);

#ifdef __cplusplus
}
#endif

#endif /* __STORAGE_FLASH_H */
