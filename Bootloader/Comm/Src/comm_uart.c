/**
 ******************************************************************************
 * @file    comm_uart.c
 * @brief   UART Communication Driver Implementation
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * UART driver for bootloader diagnostic output only.
 * Does NOT accept commands to modify safety parameters.
 *
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "comm_uart.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* Private variables ---------------------------------------------------------*/
static UART_HandleTypeDef huart_comm;
static uint8_t tx_buffer[COMM_TX_BUFFER_SIZE];
static uint8_t uart_initialized = 0;

/* ============================================================================
 * Initialization
 * ============================================================================*/

/**
 * @brief  Initialize UART communication
 */
comm_status_t Comm_UART_Init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    /* Enable clocks */
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Configure GPIO pins */
    gpio_init.Pin = COMM_UART_TX_PIN | COMM_UART_RX_PIN;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.Alternate = COMM_UART_AF;
    HAL_GPIO_Init(COMM_UART_TX_PORT, &gpio_init);

    /* Configure UART */
    huart_comm.Instance = COMM_UART_INSTANCE;
    huart_comm.Init.BaudRate = COMM_UART_BAUDRATE;
    huart_comm.Init.WordLength = COMM_UART_WORDLENGTH;
    huart_comm.Init.StopBits = COMM_UART_STOPBITS;
    huart_comm.Init.Parity = COMM_UART_PARITY;
    huart_comm.Init.Mode = UART_MODE_TX;  /* TX only - no command reception */
    huart_comm.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart_comm.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart_comm) != HAL_OK)
    {
        return COMM_ERROR;
    }

    uart_initialized = 1;

    /* Send startup message */
    Comm_UART_SendString("\r\n========================================\r\n");
    Comm_UART_SendString("STM32F407 Safety Bootloader v1.0\r\n");
    Comm_UART_SendString("IEC 61508 SIL 2 / ISO 13849 PL d\r\n");
    Comm_UART_SendString("========================================\r\n");

    return COMM_OK;
}

/**
 * @brief  Deinitialize UART communication
 */
void Comm_UART_DeInit(void)
{
    if (uart_initialized)
    {
        HAL_UART_DeInit(&huart_comm);
        __HAL_RCC_USART1_CLK_DISABLE();
        uart_initialized = 0;
    }
}

/* ============================================================================
 * Data Transmission
 * ============================================================================*/

/**
 * @brief  Send data via UART (blocking)
 */
comm_status_t Comm_UART_Send(const uint8_t *data, uint16_t size)
{
    HAL_StatusTypeDef status;

    if (!uart_initialized || data == NULL || size == 0)
    {
        return COMM_ERROR;
    }

    status = HAL_UART_Transmit(&huart_comm, (uint8_t *)data, size, COMM_UART_TIMEOUT_MS);

    switch (status)
    {
        case HAL_OK:
            return COMM_OK;
        case HAL_BUSY:
            return COMM_BUSY;
        case HAL_TIMEOUT:
            return COMM_TIMEOUT;
        default:
            return COMM_ERROR;
    }
}

/**
 * @brief  Send string via UART
 */
comm_status_t Comm_UART_SendString(const char *str)
{
    if (str == NULL)
    {
        return COMM_ERROR;
    }

    return Comm_UART_Send((const uint8_t *)str, strlen(str));
}

/**
 * @brief  Print formatted output to UART
 */
int Comm_UART_Printf(const char *format, ...)
{
    va_list args;
    int len;

    if (!uart_initialized || format == NULL)
    {
        return -1;
    }

    va_start(args, format);
    len = vsnprintf((char *)tx_buffer, COMM_TX_BUFFER_SIZE, format, args);
    va_end(args);

    if (len > 0)
    {
        if (len > COMM_TX_BUFFER_SIZE - 1)
        {
            len = COMM_TX_BUFFER_SIZE - 1;
        }
        Comm_UART_Send(tx_buffer, len);
    }

    return len;
}

/* ============================================================================
 * Status Messages
 * ============================================================================*/

/**
 * @brief  Send boot status message
 */
void Comm_UART_SendBootStatus(boot_status_t status)
{
    const char *status_str;

    switch (status)
    {
        case BOOT_OK:
            status_str = "BOOT_OK";
            break;
        case BOOT_ERROR:
            status_str = "BOOT_ERROR";
            break;
        case BOOT_CRC_ERROR:
            status_str = "BOOT_CRC_ERROR";
            break;
        case BOOT_TIMEOUT:
            status_str = "BOOT_TIMEOUT";
            break;
        case BOOT_INVALID_APP:
            status_str = "BOOT_INVALID_APP";
            break;
        default:
            status_str = "UNKNOWN";
            break;
    }

    Comm_UART_Printf("[BOOT] Status: %s (0x%02X)\r\n", status_str, (unsigned int)status);
}

/**
 * @brief  Send self-test result
 */
void Comm_UART_SendSelfTestResult(selftest_result_t result)
{
    const char *result_str;
    const char *test_name;

    switch (result)
    {
        case SELFTEST_OK:
            Comm_UART_SendString("[SELFTEST] All tests PASSED\r\n");
            return;

        case SELFTEST_CPU_FAIL:
            test_name = "CPU Register";
            result_str = "FAILED";
            break;

        case SELFTEST_RAM_FAIL:
            test_name = "RAM March C";
            result_str = "FAILED";
            break;

        case SELFTEST_FLASH_FAIL:
            test_name = "Flash CRC";
            result_str = "FAILED";
            break;

        case SELFTEST_CLOCK_FAIL:
            test_name = "Clock System";
            result_str = "FAILED";
            break;

        case SELFTEST_WDG_FAIL:
            test_name = "Watchdog Init";
            result_str = "FAILED";
            break;

        default:
            test_name = "Unknown";
            result_str = "FAILED";
            break;
    }

    Comm_UART_Printf("[SELFTEST] %s Test: %s (0x%02X)\r\n",
                      test_name, result_str, (unsigned int)result);
}

/**
 * @brief  Send factory mode status
 */
void Comm_UART_SendFactoryStatus(const char *state_str)
{
    if (state_str == NULL)
    {
        return;
    }

    Comm_UART_Printf("[FACTORY] %s\r\n", state_str);
}
