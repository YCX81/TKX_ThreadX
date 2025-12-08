/**
 ******************************************************************************
 * @file    bsp_w25qxx.c
 * @brief   W25Q Series SPI Flash Driver Implementation
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * W25Q128 SPI Flash Driver Implementation
 * Target: STM32F407VGT6
 * Flash: W25Q128 (128Mbit / 16MB)
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "bsp_w25qxx.h"
#include "bsp_debug.h"
#include "main.h"
#include <string.h>

/* ============================================================================
 * Private Defines
 * ============================================================================*/

/* CS Pin Control - Uses SPI_FLASH_CS_Pin defined in main.h */
#ifndef SPI_FLASH_CS_Pin
#define SPI_FLASH_CS_Pin        GPIO_PIN_4
#define SPI_FLASH_CS_GPIO_Port  GPIOA
#endif

#define W25QXX_CS_LOW()         HAL_GPIO_WritePin(SPI_FLASH_CS_GPIO_Port, SPI_FLASH_CS_Pin, GPIO_PIN_RESET)
#define W25QXX_CS_HIGH()        HAL_GPIO_WritePin(SPI_FLASH_CS_GPIO_Port, SPI_FLASH_CS_Pin, GPIO_PIN_SET)

/* Dummy byte for SPI read operations */
#define W25QXX_DUMMY_BYTE       0xFFU

/* ============================================================================
 * Private Variables
 * ============================================================================*/

static SPI_HandleTypeDef *s_hspi = NULL;
static w25qxx_info_t s_device_info = {0};

/* Sector buffer for write-with-erase operation */
static uint8_t s_sector_buffer[W25Q128_SECTOR_SIZE];

/* ============================================================================
 * Private Function Prototypes
 * ============================================================================*/

static w25qxx_status_t W25QXX_SPI_Transmit(uint8_t *pData, uint16_t size);
static w25qxx_status_t W25QXX_SPI_Receive(uint8_t *pData, uint16_t size);
static w25qxx_status_t W25QXX_WriteEnable(void);
static w25qxx_status_t W25QXX_WaitBusy(uint32_t timeout_ms);
static w25qxx_status_t W25QXX_WritePage(uint8_t *pBuffer, uint32_t addr, uint16_t size);

/* ============================================================================
 * Public Functions
 * ============================================================================*/

/**
 * @brief Initialize W25QXX device
 */
w25qxx_status_t BSP_W25QXX_Init(SPI_HandleTypeDef *hspi)
{
    uint32_t jedec_id;

    if (hspi == NULL)
    {
        return W25QXX_INVALID_PARAM;
    }

    s_hspi = hspi;

    /* Ensure CS is high (deselected) */
    W25QXX_CS_HIGH();
    HAL_Delay(1);

    /* Read JEDEC ID to verify communication */
    jedec_id = BSP_W25QXX_ReadJEDECID();

    DEBUG_INFO("W25QXX: JEDEC ID = 0x%06X", (unsigned int)jedec_id);

    /* Verify W25Q128 ID */
    if (jedec_id != W25Q128_JEDEC_ID)
    {
        DEBUG_ERROR("W25QXX: ID mismatch! Expected 0x%06X, got 0x%06X",
                   (unsigned int)W25Q128_JEDEC_ID, (unsigned int)jedec_id);
        s_device_info.initialized = false;
        return W25QXX_ID_ERROR;
    }

    /* Fill device info */
    s_device_info.manufacturer_id = (jedec_id >> 16) & 0xFF;
    s_device_info.device_id = jedec_id & 0xFFFF;
    s_device_info.jedec_id = jedec_id;
    s_device_info.flash_size = W25Q128_FLASH_SIZE;
    s_device_info.sector_size = W25Q128_SECTOR_SIZE;
    s_device_info.page_size = W25Q128_PAGE_SIZE;
    s_device_info.initialized = true;

    DEBUG_INFO("W25QXX: W25Q128 initialized successfully");
    DEBUG_INFO("W25QXX: Flash size = %lu MB", s_device_info.flash_size / (1024 * 1024));

    return W25QXX_OK;
}

/**
 * @brief Deinitialize W25QXX device
 */
w25qxx_status_t BSP_W25QXX_DeInit(void)
{
    s_hspi = NULL;
    memset(&s_device_info, 0, sizeof(s_device_info));
    return W25QXX_OK;
}

/**
 * @brief Read device ID
 */
uint16_t BSP_W25QXX_ReadID(void)
{
    uint8_t cmd[4] = {W25QXX_CMD_READ_ID, 0x00, 0x00, 0x00};
    uint8_t id[2] = {0};

    W25QXX_CS_LOW();
    W25QXX_SPI_Transmit(cmd, 4);
    W25QXX_SPI_Receive(id, 2);
    W25QXX_CS_HIGH();

    return ((uint16_t)id[0] << 8) | id[1];
}

/**
 * @brief Read JEDEC ID
 */
uint32_t BSP_W25QXX_ReadJEDECID(void)
{
    uint8_t cmd = W25QXX_CMD_JEDEC_ID;
    uint8_t id[3] = {0};

    W25QXX_CS_LOW();
    W25QXX_SPI_Transmit(&cmd, 1);
    W25QXX_SPI_Receive(id, 3);
    W25QXX_CS_HIGH();

    return ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];
}

/**
 * @brief Read data from flash
 */
w25qxx_status_t BSP_W25QXX_Read(uint8_t *pBuffer, uint32_t addr, uint32_t size)
{
    uint8_t cmd[4];
    w25qxx_status_t status;

    if (pBuffer == NULL || size == 0)
    {
        return W25QXX_INVALID_PARAM;
    }

    if ((addr + size) > W25Q128_FLASH_SIZE)
    {
        return W25QXX_INVALID_PARAM;
    }

    cmd[0] = W25QXX_CMD_READ_DATA;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    W25QXX_CS_LOW();
    status = W25QXX_SPI_Transmit(cmd, 4);
    if (status == W25QXX_OK)
    {
        status = W25QXX_SPI_Receive(pBuffer, size);
    }
    W25QXX_CS_HIGH();

    return status;
}

/**
 * @brief Write data to flash (handles page boundaries)
 */
w25qxx_status_t BSP_W25QXX_Write(uint8_t *pBuffer, uint32_t addr, uint32_t size)
{
    w25qxx_status_t status;
    uint32_t page_offset;
    uint32_t page_remain;
    uint32_t bytes_to_write;

    if (pBuffer == NULL || size == 0)
    {
        return W25QXX_INVALID_PARAM;
    }

    if ((addr + size) > W25Q128_FLASH_SIZE)
    {
        return W25QXX_INVALID_PARAM;
    }

    /* Calculate offset within first page */
    page_offset = addr % W25Q128_PAGE_SIZE;
    page_remain = W25Q128_PAGE_SIZE - page_offset;

    /* If data fits in remaining space of first page */
    if (size <= page_remain)
    {
        page_remain = size;
    }

    while (size > 0)
    {
        bytes_to_write = (size < page_remain) ? size : page_remain;

        status = W25QXX_WritePage(pBuffer, addr, bytes_to_write);
        if (status != W25QXX_OK)
        {
            return status;
        }

        addr += bytes_to_write;
        pBuffer += bytes_to_write;
        size -= bytes_to_write;

        /* After first page, always start at page boundary */
        page_remain = W25Q128_PAGE_SIZE;
    }

    return W25QXX_OK;
}

/**
 * @brief Write data with automatic erase
 */
w25qxx_status_t BSP_W25QXX_WriteWithErase(uint8_t *pBuffer, uint32_t addr, uint32_t size)
{
    w25qxx_status_t status;
    uint32_t sector_addr;
    uint32_t sector_offset;
    uint32_t sector_remain;
    uint32_t bytes_to_write;

    if (pBuffer == NULL || size == 0)
    {
        return W25QXX_INVALID_PARAM;
    }

    if ((addr + size) > W25Q128_FLASH_SIZE)
    {
        return W25QXX_INVALID_PARAM;
    }

    while (size > 0)
    {
        /* Calculate sector address and offset */
        sector_addr = addr & ~(W25Q128_SECTOR_SIZE - 1);
        sector_offset = addr - sector_addr;
        sector_remain = W25Q128_SECTOR_SIZE - sector_offset;

        bytes_to_write = (size < sector_remain) ? size : sector_remain;

        /* Read existing sector data */
        status = BSP_W25QXX_Read(s_sector_buffer, sector_addr, W25Q128_SECTOR_SIZE);
        if (status != W25QXX_OK)
        {
            return status;
        }

        /* Erase sector */
        status = BSP_W25QXX_EraseSector(sector_addr);
        if (status != W25QXX_OK)
        {
            return status;
        }

        /* Merge new data into sector buffer */
        memcpy(&s_sector_buffer[sector_offset], pBuffer, bytes_to_write);

        /* Write back entire sector */
        status = BSP_W25QXX_Write(s_sector_buffer, sector_addr, W25Q128_SECTOR_SIZE);
        if (status != W25QXX_OK)
        {
            return status;
        }

        addr += bytes_to_write;
        pBuffer += bytes_to_write;
        size -= bytes_to_write;
    }

    return W25QXX_OK;
}

/**
 * @brief Erase a 4KB sector
 */
w25qxx_status_t BSP_W25QXX_EraseSector(uint32_t sectorAddr)
{
    uint8_t cmd[4];
    w25qxx_status_t status;

    /* Align to sector boundary */
    sectorAddr &= ~(W25Q128_SECTOR_SIZE - 1);

    if (sectorAddr >= W25Q128_FLASH_SIZE)
    {
        return W25QXX_INVALID_PARAM;
    }

    status = W25QXX_WriteEnable();
    if (status != W25QXX_OK)
    {
        return status;
    }

    cmd[0] = W25QXX_CMD_SECTOR_ERASE_4K;
    cmd[1] = (sectorAddr >> 16) & 0xFF;
    cmd[2] = (sectorAddr >> 8) & 0xFF;
    cmd[3] = sectorAddr & 0xFF;

    W25QXX_CS_LOW();
    status = W25QXX_SPI_Transmit(cmd, 4);
    W25QXX_CS_HIGH();

    if (status != W25QXX_OK)
    {
        return status;
    }

    return W25QXX_WaitBusy(W25QXX_TIMEOUT_SECTOR_ERASE);
}

/**
 * @brief Erase a 32KB block
 */
w25qxx_status_t BSP_W25QXX_EraseBlock32K(uint32_t blockAddr)
{
    uint8_t cmd[4];
    w25qxx_status_t status;

    /* Align to 32KB block boundary */
    blockAddr &= ~(W25Q128_BLOCK_SIZE_32K - 1);

    if (blockAddr >= W25Q128_FLASH_SIZE)
    {
        return W25QXX_INVALID_PARAM;
    }

    status = W25QXX_WriteEnable();
    if (status != W25QXX_OK)
    {
        return status;
    }

    cmd[0] = W25QXX_CMD_BLOCK_ERASE_32K;
    cmd[1] = (blockAddr >> 16) & 0xFF;
    cmd[2] = (blockAddr >> 8) & 0xFF;
    cmd[3] = blockAddr & 0xFF;

    W25QXX_CS_LOW();
    status = W25QXX_SPI_Transmit(cmd, 4);
    W25QXX_CS_HIGH();

    if (status != W25QXX_OK)
    {
        return status;
    }

    return W25QXX_WaitBusy(W25QXX_TIMEOUT_BLOCK_ERASE);
}

/**
 * @brief Erase a 64KB block
 */
w25qxx_status_t BSP_W25QXX_EraseBlock64K(uint32_t blockAddr)
{
    uint8_t cmd[4];
    w25qxx_status_t status;

    /* Align to 64KB block boundary */
    blockAddr &= ~(W25Q128_BLOCK_SIZE_64K - 1);

    if (blockAddr >= W25Q128_FLASH_SIZE)
    {
        return W25QXX_INVALID_PARAM;
    }

    status = W25QXX_WriteEnable();
    if (status != W25QXX_OK)
    {
        return status;
    }

    cmd[0] = W25QXX_CMD_BLOCK_ERASE_64K;
    cmd[1] = (blockAddr >> 16) & 0xFF;
    cmd[2] = (blockAddr >> 8) & 0xFF;
    cmd[3] = blockAddr & 0xFF;

    W25QXX_CS_LOW();
    status = W25QXX_SPI_Transmit(cmd, 4);
    W25QXX_CS_HIGH();

    if (status != W25QXX_OK)
    {
        return status;
    }

    return W25QXX_WaitBusy(W25QXX_TIMEOUT_BLOCK_ERASE);
}

/**
 * @brief Erase entire chip
 */
w25qxx_status_t BSP_W25QXX_EraseChip(void)
{
    uint8_t cmd;
    w25qxx_status_t status;

    DEBUG_WARN("W25QXX: Chip erase started - this may take a while...");

    status = W25QXX_WriteEnable();
    if (status != W25QXX_OK)
    {
        return status;
    }

    cmd = W25QXX_CMD_CHIP_ERASE;

    W25QXX_CS_LOW();
    status = W25QXX_SPI_Transmit(&cmd, 1);
    W25QXX_CS_HIGH();

    if (status != W25QXX_OK)
    {
        return status;
    }

    return W25QXX_WaitBusy(W25QXX_TIMEOUT_CHIP_ERASE);
}

/**
 * @brief Get device information
 */
const w25qxx_info_t* BSP_W25QXX_GetInfo(void)
{
    return &s_device_info;
}

/**
 * @brief Check if device is busy
 */
bool BSP_W25QXX_IsBusy(void)
{
    return (BSP_W25QXX_ReadStatusReg(1) & W25QXX_STATUS_BUSY) != 0;
}

/**
 * @brief Enter power down mode
 */
w25qxx_status_t BSP_W25QXX_PowerDown(void)
{
    uint8_t cmd = W25QXX_CMD_POWER_DOWN;
    w25qxx_status_t status;

    W25QXX_CS_LOW();
    status = W25QXX_SPI_Transmit(&cmd, 1);
    W25QXX_CS_HIGH();

    /* Wait for power down (3us typical) */
    HAL_Delay(1);

    return status;
}

/**
 * @brief Wake up from power down mode
 */
w25qxx_status_t BSP_W25QXX_WakeUp(void)
{
    uint8_t cmd = W25QXX_CMD_RELEASE_POWER_DOWN;
    w25qxx_status_t status;

    W25QXX_CS_LOW();
    status = W25QXX_SPI_Transmit(&cmd, 1);
    W25QXX_CS_HIGH();

    /* Wait for wake up (3us typical) */
    HAL_Delay(1);

    return status;
}

/**
 * @brief Read status register
 */
uint8_t BSP_W25QXX_ReadStatusReg(uint8_t reg)
{
    uint8_t cmd;
    uint8_t status = 0;

    if (reg == 1)
    {
        cmd = W25QXX_CMD_READ_STATUS_R1;
    }
    else
    {
        cmd = W25QXX_CMD_READ_STATUS_R2;
    }

    W25QXX_CS_LOW();
    W25QXX_SPI_Transmit(&cmd, 1);
    W25QXX_SPI_Receive(&status, 1);
    W25QXX_CS_HIGH();

    return status;
}

/* ============================================================================
 * Private Functions
 * ============================================================================*/

/**
 * @brief SPI transmit wrapper
 */
static w25qxx_status_t W25QXX_SPI_Transmit(uint8_t *pData, uint16_t size)
{
    if (s_hspi == NULL)
    {
        return W25QXX_ERROR;
    }

    if (HAL_SPI_Transmit(s_hspi, pData, size, W25QXX_TIMEOUT_DEFAULT) != HAL_OK)
    {
        return W25QXX_SPI_ERROR;
    }

    return W25QXX_OK;
}

/**
 * @brief SPI receive wrapper
 */
static w25qxx_status_t W25QXX_SPI_Receive(uint8_t *pData, uint16_t size)
{
    if (s_hspi == NULL)
    {
        return W25QXX_ERROR;
    }

    if (HAL_SPI_Receive(s_hspi, pData, size, W25QXX_TIMEOUT_DEFAULT) != HAL_OK)
    {
        return W25QXX_SPI_ERROR;
    }

    return W25QXX_OK;
}

/**
 * @brief Send write enable command
 */
static w25qxx_status_t W25QXX_WriteEnable(void)
{
    uint8_t cmd = W25QXX_CMD_WRITE_ENABLE;
    w25qxx_status_t status;

    W25QXX_CS_LOW();
    status = W25QXX_SPI_Transmit(&cmd, 1);
    W25QXX_CS_HIGH();

    if (status != W25QXX_OK)
    {
        return status;
    }

    /* Wait for WEL bit to be set */
    uint32_t start = HAL_GetTick();
    while ((BSP_W25QXX_ReadStatusReg(1) & W25QXX_STATUS_WEL) == 0)
    {
        if ((HAL_GetTick() - start) > 100)
        {
            return W25QXX_TIMEOUT;
        }
    }

    return W25QXX_OK;
}

/**
 * @brief Wait for busy flag to clear
 */
static w25qxx_status_t W25QXX_WaitBusy(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();

    while (BSP_W25QXX_IsBusy())
    {
        if ((HAL_GetTick() - start) > timeout_ms)
        {
            DEBUG_ERROR("W25QXX: Wait busy timeout");
            return W25QXX_TIMEOUT;
        }
    }

    return W25QXX_OK;
}

/**
 * @brief Write a single page (up to 256 bytes)
 */
static w25qxx_status_t W25QXX_WritePage(uint8_t *pBuffer, uint32_t addr, uint16_t size)
{
    uint8_t cmd[4];
    w25qxx_status_t status;

    if (size == 0 || size > W25Q128_PAGE_SIZE)
    {
        return W25QXX_INVALID_PARAM;
    }

    status = W25QXX_WriteEnable();
    if (status != W25QXX_OK)
    {
        return status;
    }

    cmd[0] = W25QXX_CMD_PAGE_PROGRAM;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    W25QXX_CS_LOW();
    status = W25QXX_SPI_Transmit(cmd, 4);
    if (status == W25QXX_OK)
    {
        status = W25QXX_SPI_Transmit(pBuffer, size);
    }
    W25QXX_CS_HIGH();

    if (status != W25QXX_OK)
    {
        return status;
    }

    return W25QXX_WaitBusy(W25QXX_TIMEOUT_PAGE_PROGRAM);
}
