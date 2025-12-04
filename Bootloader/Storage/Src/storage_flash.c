/**
 ******************************************************************************
 * @file    storage_flash.c
 * @brief   Flash Storage Driver Implementation
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Flash storage operations for bootloader configuration and safety parameters.
 * Implements read/write/erase operations with CRC verification.
 *
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "storage_flash.h"
#include "boot_crc.h"
#include "stm32f4xx_hal.h"
#include <string.h>

/* Private variables ---------------------------------------------------------*/
static uint8_t storage_initialized = 0;

/* ============================================================================
 * Initialization
 * ============================================================================*/

/**
 * @brief  Initialize Flash storage
 */
storage_status_t Storage_Init(void)
{
    /* Initialize CRC unit if not already done */
    Boot_CRC_Init();

    storage_initialized = 1;

    return STORAGE_OK;
}

/* ============================================================================
 * Boot Configuration Operations
 * ============================================================================*/

/**
 * @brief  Read boot configuration from Flash
 */
storage_status_t Storage_ReadConfig(boot_config_t *config)
{
    if (config == NULL)
    {
        return STORAGE_ERROR;
    }

    /* Read directly from Flash */
    memcpy(config, (void *)CONFIG_FLASH_START, sizeof(boot_config_t));

    /* Verify magic number */
    if (config->magic != CONFIG_MAGIC)
    {
        return STORAGE_MAGIC_ERROR;
    }

    /* Verify CRC */
    return Storage_VerifyConfigCRC(config);
}

/**
 * @brief  Write boot configuration to Flash
 */
storage_status_t Storage_WriteConfig(const boot_config_t *config)
{
    storage_status_t status;
    boot_config_t config_with_crc;

    if (config == NULL)
    {
        return STORAGE_ERROR;
    }

    /* Copy config and calculate CRC */
    memcpy(&config_with_crc, config, sizeof(boot_config_t));
    config_with_crc.magic = CONFIG_MAGIC;
    config_with_crc.crc = Boot_CRC32_Calculate((uint8_t *)&config_with_crc,
                                                sizeof(boot_config_t) - sizeof(uint32_t));

    /* Erase sector first */
    status = Storage_EraseSector();
    if (status != STORAGE_OK)
    {
        return status;
    }

    /* Program Flash */
    status = Storage_ProgramFlash(CONFIG_FLASH_START,
                                   (uint8_t *)&config_with_crc,
                                   sizeof(boot_config_t));
    if (status != STORAGE_OK)
    {
        return status;
    }

    /* Verify write */
    status = Storage_VerifyFlash(CONFIG_FLASH_START,
                                  (uint8_t *)&config_with_crc,
                                  sizeof(boot_config_t));

    return status;
}

/**
 * @brief  Verify boot configuration CRC
 */
storage_status_t Storage_VerifyConfigCRC(const boot_config_t *config)
{
    uint32_t calc_crc;

    if (config == NULL)
    {
        return STORAGE_ERROR;
    }

    /* Calculate CRC over structure (excluding CRC field) */
    calc_crc = Boot_CRC32_Calculate((uint8_t *)config,
                                     sizeof(boot_config_t) - sizeof(uint32_t));

    if (calc_crc != config->crc)
    {
        return STORAGE_CRC_ERROR;
    }

    return STORAGE_OK;
}

/* ============================================================================
 * Safety Parameters Operations
 * ============================================================================*/

/**
 * @brief  Read safety parameters from Flash
 */
storage_status_t Storage_ReadSafetyParams(safety_params_t *params)
{
    if (params == NULL)
    {
        return STORAGE_ERROR;
    }

    /* Read directly from Flash */
    memcpy(params, (void *)SAFETY_PARAMS_ADDR, sizeof(safety_params_t));

    /* Verify magic number */
    if (params->magic != SAFETY_PARAMS_MAGIC)
    {
        return STORAGE_MAGIC_ERROR;
    }

    /* Full validation */
    return Storage_ValidateSafetyParams(params);
}

/**
 * @brief  Write safety parameters to Flash
 */
storage_status_t Storage_WriteSafetyParams(const safety_params_t *params)
{
    storage_status_t status;
    safety_params_t params_with_crc;
    boot_config_t config;

    if (params == NULL)
    {
        return STORAGE_ERROR;
    }

    /* Read existing config to preserve it */
    status = Storage_ReadConfig(&config);
    if (status != STORAGE_OK && status != STORAGE_MAGIC_ERROR && status != STORAGE_CRC_ERROR)
    {
        /* Initialize default config if not present */
        memset(&config, 0, sizeof(boot_config_t));
        config.magic = CONFIG_MAGIC;
        config.factory_mode = 0;
        config.cal_valid = 0;
        config.app_crc = 0;
    }

    /* Copy params and set magic */
    memcpy(&params_with_crc, params, sizeof(safety_params_t));
    params_with_crc.magic = SAFETY_PARAMS_MAGIC;
    params_with_crc.version = SAFETY_PARAMS_VERSION;
    params_with_crc.size = sizeof(safety_params_t);

    /* Calculate CRC */
    params_with_crc.crc32 = Boot_CRC32_Calculate((uint8_t *)&params_with_crc,
                                                  sizeof(safety_params_t) - sizeof(uint32_t));

    /* Erase sector (this also erases config) */
    status = Storage_EraseSector();
    if (status != STORAGE_OK)
    {
        return status;
    }

    /* Write config first */
    config.cal_valid = 1;  /* Mark calibration as valid */
    config.crc = Boot_CRC32_Calculate((uint8_t *)&config,
                                       sizeof(boot_config_t) - sizeof(uint32_t));

    status = Storage_ProgramFlash(CONFIG_FLASH_START,
                                   (uint8_t *)&config,
                                   sizeof(boot_config_t));
    if (status != STORAGE_OK)
    {
        return status;
    }

    /* Write safety params */
    status = Storage_ProgramFlash(SAFETY_PARAMS_ADDR,
                                   (uint8_t *)&params_with_crc,
                                   sizeof(safety_params_t));
    if (status != STORAGE_OK)
    {
        return status;
    }

    /* Verify write */
    status = Storage_VerifyFlash(SAFETY_PARAMS_ADDR,
                                  (uint8_t *)&params_with_crc,
                                  sizeof(safety_params_t));

    return status;
}

/**
 * @brief  Validate safety parameters (CRC + redundancy)
 */
storage_status_t Storage_ValidateSafetyParams(const safety_params_t *params)
{
    uint32_t calc_crc;
    uint32_t i;
    uint32_t val, inv;

    if (params == NULL)
    {
        return STORAGE_ERROR;
    }

    /* Check magic */
    if (params->magic != SAFETY_PARAMS_MAGIC)
    {
        return STORAGE_MAGIC_ERROR;
    }

    /* Check version */
    if (params->version != SAFETY_PARAMS_VERSION)
    {
        return STORAGE_ERROR;
    }

    /* Check size */
    if (params->size != sizeof(safety_params_t))
    {
        return STORAGE_ERROR;
    }

    /* Calculate and verify CRC */
    calc_crc = Boot_CRC32_Calculate((uint8_t *)params,
                                     sizeof(safety_params_t) - sizeof(uint32_t));
    if (calc_crc != params->crc32)
    {
        return STORAGE_CRC_ERROR;
    }

    /* Verify redundancy: hall_offset[i] == ~hall_offset_inv[i] */
    for (i = 0; i < 3; i++)
    {
        /* Compare as uint32_t bit patterns */
        memcpy(&val, &params->hall_offset[i], sizeof(uint32_t));
        memcpy(&inv, &params->hall_offset_inv[i], sizeof(uint32_t));
        if (val != ~inv)
        {
            return STORAGE_VERIFY_ERROR;
        }

        memcpy(&val, &params->hall_gain[i], sizeof(uint32_t));
        memcpy(&inv, &params->hall_gain_inv[i], sizeof(uint32_t));
        if (val != ~inv)
        {
            return STORAGE_VERIFY_ERROR;
        }
    }

    return STORAGE_OK;
}

/**
 * @brief  Check if safety parameters are present and valid
 */
storage_status_t Storage_CheckSafetyParamsExist(void)
{
    safety_params_t params;

    /* Read and validate */
    return Storage_ReadSafetyParams(&params);
}

/* ============================================================================
 * Flash Operations
 * ============================================================================*/

/**
 * @brief  Erase Config sector (Sector 3)
 */
storage_status_t Storage_EraseSector(void)
{
    FLASH_EraseInitTypeDef erase_init = {0};
    uint32_t sector_error = 0;
    HAL_StatusTypeDef hal_status;

    /* Unlock Flash */
    hal_status = HAL_FLASH_Unlock();
    if (hal_status != HAL_OK)
    {
        return STORAGE_ERROR;
    }

    /* Configure erase */
    erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_init.Banks = FLASH_BANK_1;
    erase_init.Sector = FLASH_SECTOR_CONFIG;
    erase_init.NbSectors = 1;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE;

    /* Perform erase */
    hal_status = HAL_FLASHEx_Erase(&erase_init, &sector_error);

    /* Lock Flash */
    HAL_FLASH_Lock();

    if (hal_status != HAL_OK)
    {
        return STORAGE_ERASE_ERROR;
    }

    if (sector_error != 0xFFFFFFFF)
    {
        return STORAGE_ERASE_ERROR;
    }

    return STORAGE_OK;
}

/**
 * @brief  Program Flash with data
 */
storage_status_t Storage_ProgramFlash(uint32_t address, const uint8_t *data, uint32_t size)
{
    HAL_StatusTypeDef hal_status;
    uint32_t i;
    uint32_t word;

    if (data == NULL || size == 0)
    {
        return STORAGE_ERROR;
    }

    /* Size must be multiple of 4 */
    if ((size & 0x03) != 0)
    {
        return STORAGE_ERROR;
    }

    /* Unlock Flash */
    hal_status = HAL_FLASH_Unlock();
    if (hal_status != HAL_OK)
    {
        return STORAGE_ERROR;
    }

    /* Program word by word */
    for (i = 0; i < size; i += 4)
    {
        /* Get word from data buffer */
        word = *(uint32_t *)&data[i];

        hal_status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address + i, word);
        if (hal_status != HAL_OK)
        {
            HAL_FLASH_Lock();
            return STORAGE_WRITE_ERROR;
        }
    }

    /* Lock Flash */
    HAL_FLASH_Lock();

    return STORAGE_OK;
}

/**
 * @brief  Verify Flash data
 */
storage_status_t Storage_VerifyFlash(uint32_t address, const uint8_t *data, uint32_t size)
{
    if (data == NULL || size == 0)
    {
        return STORAGE_ERROR;
    }

    /* Compare memory */
    if (memcmp((void *)address, data, size) != 0)
    {
        return STORAGE_VERIFY_ERROR;
    }

    return STORAGE_OK;
}

/* ============================================================================
 * Factory Mode Flag Operations
 * ============================================================================*/

/**
 * @brief  Set factory mode flag
 */
storage_status_t Storage_SetFactoryModeFlag(void)
{
    boot_config_t config;
    storage_status_t status;

    /* Read current config */
    status = Storage_ReadConfig(&config);
    if (status != STORAGE_OK && status != STORAGE_MAGIC_ERROR && status != STORAGE_CRC_ERROR)
    {
        /* Initialize default config */
        memset(&config, 0, sizeof(boot_config_t));
        config.magic = CONFIG_MAGIC;
    }

    /* Set factory mode flag */
    config.factory_mode = FACTORY_MODE_MAGIC;

    /* Write back */
    return Storage_WriteConfig(&config);
}

/**
 * @brief  Clear factory mode flag
 */
storage_status_t Storage_ClearFactoryModeFlag(void)
{
    boot_config_t config;
    storage_status_t status;

    /* Read current config */
    status = Storage_ReadConfig(&config);
    if (status != STORAGE_OK)
    {
        return status;
    }

    /* Clear factory mode flag */
    config.factory_mode = 0;

    /* Write back */
    return Storage_WriteConfig(&config);
}

/**
 * @brief  Check if factory mode is requested
 */
uint32_t Storage_IsFactoryModeRequested(void)
{
    boot_config_t config;

    /* Read config */
    if (Storage_ReadConfig(&config) != STORAGE_OK)
    {
        return 0;  /* Default to normal mode if config invalid */
    }

    return (config.factory_mode == FACTORY_MODE_MAGIC) ? 1 : 0;
}
