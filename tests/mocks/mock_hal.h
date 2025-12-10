/**
 ******************************************************************************
 * @file    mock_hal.h
 * @brief   STM32 HAL Mock for Unit Testing
 *          STM32 HAL 模拟层用于单元测试
 ******************************************************************************
 */

#ifndef __MOCK_HAL_H
#define __MOCK_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Type Definitions
 * ============================================================================*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* HAL Status */
typedef enum {
    HAL_OK       = 0x00U,
    HAL_ERROR    = 0x01U,
    HAL_BUSY     = 0x02U,
    HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

/* ============================================================================
 * Mock HAL Functions
 * ============================================================================*/

/**
 * @brief Get current tick count (mock)
 */
uint32_t HAL_GetTick(void);

/**
 * @brief Set mock tick value for testing
 */
void Mock_HAL_SetTick(uint32_t tick);

/**
 * @brief Advance mock tick by specified amount
 */
void Mock_HAL_AdvanceTick(uint32_t delta);

/**
 * @brief Reset all mocks
 */
void Mock_HAL_Reset(void);

/* ============================================================================
 * Mock CRC Functions
 * ============================================================================*/

/**
 * @brief Calculate CRC32 (mock implementation)
 */
uint32_t Mock_CRC_Calculate(const uint32_t *pBuffer, uint32_t BufferLength);

/**
 * @brief Set expected CRC result for testing
 */
void Mock_CRC_SetResult(uint32_t result);

/* ============================================================================
 * Mock GPIO Functions
 * ============================================================================*/

typedef struct {
    uint32_t dummy;
} GPIO_TypeDef;

typedef enum {
    GPIO_PIN_RESET = 0U,
    GPIO_PIN_SET = 1U
} GPIO_PinState;

void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

/* ============================================================================
 * Mock IWDG/WWDG Functions
 * ============================================================================*/

typedef struct {
    uint32_t dummy;
} IWDG_HandleTypeDef;

typedef struct {
    uint32_t dummy;
} WWDG_HandleTypeDef;

HAL_StatusTypeDef HAL_IWDG_Refresh(IWDG_HandleTypeDef *hiwdg);
HAL_StatusTypeDef HAL_WWDG_Refresh(WWDG_HandleTypeDef *hwwdg);

/**
 * @brief Get IWDG refresh count for verification
 */
uint32_t Mock_IWDG_GetRefreshCount(void);

/**
 * @brief Get WWDG refresh count for verification
 */
uint32_t Mock_WWDG_GetRefreshCount(void);

#ifdef __cplusplus
}
#endif

#endif /* __MOCK_HAL_H */
