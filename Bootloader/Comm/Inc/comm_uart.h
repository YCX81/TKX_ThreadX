/**
 ******************************************************************************
 * @file    comm_uart.h
 * @brief   UART Communication Driver Header
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * UART driver for bootloader diagnostic output.
 * NOTE: This is OUTPUT ONLY for diagnostics. Safety parameters
 *       CANNOT be modified via UART - only via debugger in factory mode.
 *
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

#ifndef __COMM_UART_H
#define __COMM_UART_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "boot_config.h"

/* ============================================================================
 * UART Configuration
 * ============================================================================*/

/* Default UART settings */
#define COMM_UART_INSTANCE      USART1
#define COMM_UART_BAUDRATE      115200U
#define COMM_UART_WORDLENGTH    UART_WORDLENGTH_8B
#define COMM_UART_STOPBITS      UART_STOPBITS_1
#define COMM_UART_PARITY        UART_PARITY_NONE

/* GPIO pins (USART1) */
#define COMM_UART_TX_PORT       GPIOA
#define COMM_UART_TX_PIN        GPIO_PIN_9
#define COMM_UART_RX_PORT       GPIOA
#define COMM_UART_RX_PIN        GPIO_PIN_10
#define COMM_UART_AF            GPIO_AF7_USART1

/* Timeout */
#define COMM_UART_TIMEOUT_MS    1000U

/* Buffer sizes */
#define COMM_TX_BUFFER_SIZE     256U
#define COMM_RX_BUFFER_SIZE     64U

/* ============================================================================
 * UART Status Codes
 * ============================================================================*/

typedef enum {
    COMM_OK       = 0x00,
    COMM_ERROR    = 0x01,
    COMM_BUSY     = 0x02,
    COMM_TIMEOUT  = 0x03
} comm_status_t;

/* ============================================================================
 * Function Prototypes
 * ============================================================================*/

/**
 * @brief  Initialize UART communication
 * @retval COMM_OK on success
 */
comm_status_t Comm_UART_Init(void);

/**
 * @brief  Deinitialize UART communication
 */
void Comm_UART_DeInit(void);

/**
 * @brief  Send data via UART (blocking)
 * @param  data: Pointer to data buffer
 * @param  size: Number of bytes to send
 * @retval COMM_OK on success
 */
comm_status_t Comm_UART_Send(const uint8_t *data, uint16_t size);

/**
 * @brief  Send string via UART
 * @param  str: Null-terminated string
 * @retval COMM_OK on success
 */
comm_status_t Comm_UART_SendString(const char *str);

/**
 * @brief  Print formatted output to UART
 * @param  format: Format string
 * @retval Number of characters printed
 */
int Comm_UART_Printf(const char *format, ...);

/**
 * @brief  Send boot status message
 * @param  status: Boot status code
 */
void Comm_UART_SendBootStatus(boot_status_t status);

/**
 * @brief  Send self-test result
 * @param  result: Self-test result code
 */
void Comm_UART_SendSelfTestResult(selftest_result_t result);

/**
 * @brief  Send factory mode status
 * @param  state_str: State description string
 */
void Comm_UART_SendFactoryStatus(const char *state_str);

#ifdef __cplusplus
}
#endif

#endif /* __COMM_UART_H */
