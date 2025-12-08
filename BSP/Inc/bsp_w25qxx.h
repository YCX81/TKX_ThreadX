/**
 ******************************************************************************
 * @file    bsp_w25qxx.h
 * @brief   W25Q Series SPI Flash Driver
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * W25Q128 SPI Flash Driver for external storage
 * Target: STM32F407VGT6
 * Flash: W25Q128 (128Mbit / 16MB)
 *
 * Hardware Connection:
 *   PA4 -> SPI_FLASH_CS (Software controlled)
 *   PA5 -> SPI1_SCK
 *   PA6 -> SPI1_MISO
 *   PA7 -> SPI1_MOSI
 *
 ******************************************************************************
 */

#ifndef __BSP_W25QXX_H
#define __BSP_W25QXX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * W25Q128 Specifications
 * ============================================================================*/

#define W25Q128_FLASH_SIZE              (16U * 1024U * 1024U)   /* 16MB */
#define W25Q128_PAGE_SIZE               256U
#define W25Q128_SECTOR_SIZE             (4U * 1024U)            /* 4KB */
#define W25Q128_BLOCK_SIZE_32K          (32U * 1024U)           /* 32KB */
#define W25Q128_BLOCK_SIZE_64K          (64U * 1024U)           /* 64KB */
#define W25Q128_SECTOR_COUNT            (W25Q128_FLASH_SIZE / W25Q128_SECTOR_SIZE)
#define W25Q128_PAGE_COUNT              (W25Q128_FLASH_SIZE / W25Q128_PAGE_SIZE)

/* Device Identification */
#define W25Q128_MANUFACTURER_ID         0xEFU
#define W25Q128_DEVICE_ID               0x4018U
#define W25Q128_JEDEC_ID                0xEF4018UL

/* ============================================================================
 * W25Q Command Set
 * ============================================================================*/

#define W25QXX_CMD_WRITE_ENABLE         0x06U
#define W25QXX_CMD_WRITE_DISABLE        0x04U
#define W25QXX_CMD_READ_STATUS_R1       0x05U
#define W25QXX_CMD_READ_STATUS_R2       0x35U
#define W25QXX_CMD_WRITE_STATUS         0x01U
#define W25QXX_CMD_READ_DATA            0x03U
#define W25QXX_CMD_FAST_READ            0x0BU
#define W25QXX_CMD_PAGE_PROGRAM         0x02U
#define W25QXX_CMD_SECTOR_ERASE_4K      0x20U
#define W25QXX_CMD_BLOCK_ERASE_32K      0x52U
#define W25QXX_CMD_BLOCK_ERASE_64K      0xD8U
#define W25QXX_CMD_CHIP_ERASE           0xC7U
#define W25QXX_CMD_POWER_DOWN           0xB9U
#define W25QXX_CMD_RELEASE_POWER_DOWN   0xABU
#define W25QXX_CMD_READ_ID              0x90U
#define W25QXX_CMD_JEDEC_ID             0x9FU

/* Status Register Bits */
#define W25QXX_STATUS_BUSY              0x01U
#define W25QXX_STATUS_WEL               0x02U

/* Timeout Values (ms) */
#define W25QXX_TIMEOUT_DEFAULT          1000U
#define W25QXX_TIMEOUT_PAGE_PROGRAM     10U
#define W25QXX_TIMEOUT_SECTOR_ERASE     400U
#define W25QXX_TIMEOUT_BLOCK_ERASE      2000U
#define W25QXX_TIMEOUT_CHIP_ERASE       200000U

/* ============================================================================
 * Type Definitions
 * ============================================================================*/

/**
 * @brief W25QXX operation status
 */
typedef enum {
    W25QXX_OK                   = 0x00U,    /* Operation successful */
    W25QXX_ERROR                = 0x01U,    /* General error */
    W25QXX_BUSY                 = 0x02U,    /* Device busy */
    W25QXX_TIMEOUT              = 0x03U,    /* Operation timeout */
    W25QXX_INVALID_PARAM        = 0x04U,    /* Invalid parameter */
    W25QXX_ID_ERROR             = 0x05U,    /* Device ID mismatch */
    W25QXX_SPI_ERROR            = 0x06U     /* SPI communication error */
} w25qxx_status_t;

/**
 * @brief W25QXX device information
 */
typedef struct {
    uint8_t  manufacturer_id;               /* Manufacturer ID */
    uint16_t device_id;                     /* Device ID */
    uint32_t jedec_id;                      /* JEDEC ID */
    uint32_t flash_size;                    /* Flash size in bytes */
    uint32_t sector_size;                   /* Sector size in bytes */
    uint32_t page_size;                     /* Page size in bytes */
    bool     initialized;                   /* Initialization status */
} w25qxx_info_t;

/* ============================================================================
 * Public Function Prototypes
 * ============================================================================*/

/**
 * @brief Initialize W25QXX device
 * @param hspi Pointer to SPI handle
 * @retval w25qxx_status_t Operation status
 */
w25qxx_status_t BSP_W25QXX_Init(SPI_HandleTypeDef *hspi);

/**
 * @brief Deinitialize W25QXX device
 * @retval w25qxx_status_t Operation status
 */
w25qxx_status_t BSP_W25QXX_DeInit(void);

/**
 * @brief Read device ID
 * @retval uint16_t Device ID (manufacturer << 8 | device)
 */
uint16_t BSP_W25QXX_ReadID(void);

/**
 * @brief Read JEDEC ID
 * @retval uint32_t JEDEC ID
 */
uint32_t BSP_W25QXX_ReadJEDECID(void);

/**
 * @brief Read data from flash
 * @param pBuffer Pointer to data buffer
 * @param addr Flash address
 * @param size Number of bytes to read
 * @retval w25qxx_status_t Operation status
 */
w25qxx_status_t BSP_W25QXX_Read(uint8_t *pBuffer, uint32_t addr, uint32_t size);

/**
 * @brief Write data to flash (handles page boundaries)
 * @param pBuffer Pointer to data buffer
 * @param addr Flash address
 * @param size Number of bytes to write
 * @note User must ensure target area is erased before writing
 * @retval w25qxx_status_t Operation status
 */
w25qxx_status_t BSP_W25QXX_Write(uint8_t *pBuffer, uint32_t addr, uint32_t size);

/**
 * @brief Write data with automatic erase
 * @param pBuffer Pointer to data buffer
 * @param addr Flash address
 * @param size Number of bytes to write
 * @note Automatically erases sectors as needed (slower but safer)
 * @retval w25qxx_status_t Operation status
 */
w25qxx_status_t BSP_W25QXX_WriteWithErase(uint8_t *pBuffer, uint32_t addr, uint32_t size);

/**
 * @brief Erase a 4KB sector
 * @param sectorAddr Sector start address (must be 4KB aligned)
 * @retval w25qxx_status_t Operation status
 */
w25qxx_status_t BSP_W25QXX_EraseSector(uint32_t sectorAddr);

/**
 * @brief Erase a 32KB block
 * @param blockAddr Block start address (must be 32KB aligned)
 * @retval w25qxx_status_t Operation status
 */
w25qxx_status_t BSP_W25QXX_EraseBlock32K(uint32_t blockAddr);

/**
 * @brief Erase a 64KB block
 * @param blockAddr Block start address (must be 64KB aligned)
 * @retval w25qxx_status_t Operation status
 */
w25qxx_status_t BSP_W25QXX_EraseBlock64K(uint32_t blockAddr);

/**
 * @brief Erase entire chip
 * @note This operation takes a long time (~30-200 seconds)
 * @retval w25qxx_status_t Operation status
 */
w25qxx_status_t BSP_W25QXX_EraseChip(void);

/**
 * @brief Get device information
 * @retval const w25qxx_info_t* Pointer to device info
 */
const w25qxx_info_t* BSP_W25QXX_GetInfo(void);

/**
 * @brief Check if device is busy
 * @retval bool true if busy, false if ready
 */
bool BSP_W25QXX_IsBusy(void);

/**
 * @brief Enter power down mode
 * @retval w25qxx_status_t Operation status
 */
w25qxx_status_t BSP_W25QXX_PowerDown(void);

/**
 * @brief Wake up from power down mode
 * @retval w25qxx_status_t Operation status
 */
w25qxx_status_t BSP_W25QXX_WakeUp(void);

/**
 * @brief Read status register
 * @param reg Register number (1 or 2)
 * @retval uint8_t Status register value
 */
uint8_t BSP_W25QXX_ReadStatusReg(uint8_t reg);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_W25QXX_H */
