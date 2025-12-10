/**
 ******************************************************************************
 * @file    mock_hal.c
 * @brief   STM32 HAL Mock Implementation
 ******************************************************************************
 */

#include "mock_hal.h"

/* ============================================================================
 * Mock State Variables
 * ============================================================================*/

static uint32_t s_mock_tick = 0;
static uint32_t s_mock_crc_result = 0;
static uint32_t s_iwdg_refresh_count = 0;
static uint32_t s_wwdg_refresh_count = 0;
static GPIO_PinState s_gpio_states[16] = {0};

/* ============================================================================
 * HAL Tick Mock
 * ============================================================================*/

uint32_t HAL_GetTick(void)
{
    return s_mock_tick;
}

void Mock_HAL_SetTick(uint32_t tick)
{
    s_mock_tick = tick;
}

void Mock_HAL_AdvanceTick(uint32_t delta)
{
    s_mock_tick += delta;
}

void Mock_HAL_Reset(void)
{
    s_mock_tick = 0;
    s_mock_crc_result = 0;
    s_iwdg_refresh_count = 0;
    s_wwdg_refresh_count = 0;
    memset(s_gpio_states, 0, sizeof(s_gpio_states));
}

/* ============================================================================
 * CRC Mock
 * ============================================================================*/

uint32_t Mock_CRC_Calculate(const uint32_t *pBuffer, uint32_t BufferLength)
{
    (void)pBuffer;
    (void)BufferLength;
    return s_mock_crc_result;
}

void Mock_CRC_SetResult(uint32_t result)
{
    s_mock_crc_result = result;
}

/* ============================================================================
 * GPIO Mock
 * ============================================================================*/

void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)
{
    (void)GPIOx;
    if (GPIO_Pin < 16)
    {
        s_gpio_states[GPIO_Pin] = PinState;
    }
}

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    (void)GPIOx;
    if (GPIO_Pin < 16)
    {
        return s_gpio_states[GPIO_Pin];
    }
    return GPIO_PIN_RESET;
}

/* ============================================================================
 * Watchdog Mock
 * ============================================================================*/

HAL_StatusTypeDef HAL_IWDG_Refresh(IWDG_HandleTypeDef *hiwdg)
{
    (void)hiwdg;
    s_iwdg_refresh_count++;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_WWDG_Refresh(WWDG_HandleTypeDef *hwwdg)
{
    (void)hwwdg;
    s_wwdg_refresh_count++;
    return HAL_OK;
}

uint32_t Mock_IWDG_GetRefreshCount(void)
{
    return s_iwdg_refresh_count;
}

uint32_t Mock_WWDG_GetRefreshCount(void)
{
    return s_wwdg_refresh_count;
}
