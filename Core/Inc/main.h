/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LCD_RES_Pin GPIO_PIN_0
#define LCD_RES_GPIO_Port GPIOC
#define LCD_DC_Pin GPIO_PIN_1
#define LCD_DC_GPIO_Port GPIOC
#define LCD_GSO_Pin GPIO_PIN_2
#define LCD_GSO_GPIO_Port GPIOC
#define LCD_SDA_Pin GPIO_PIN_3
#define LCD_SDA_GPIO_Port GPIOC
#define KEY_Pin GPIO_PIN_0
#define KEY_GPIO_Port GPIOA
#define SPI_FLASH_CS_Pin GPIO_PIN_4
#define SPI_FLASH_CS_GPIO_Port GPIOA
#define SPI_FLASH_SCK_Pin GPIO_PIN_5
#define SPI_FLASH_SCK_GPIO_Port GPIOA
#define SPI_FLASH_MISO_Pin GPIO_PIN_6
#define SPI_FLASH_MISO_GPIO_Port GPIOA
#define SPI_FLASH_MOSI_Pin GPIO_PIN_7
#define SPI_FLASH_MOSI_GPIO_Port GPIOA
#define LCD_BLK_Pin GPIO_PIN_4
#define LCD_BLK_GPIO_Port GPIOC
#define LED_G_Pin GPIO_PIN_2
#define LED_G_GPIO_Port GPIOB
#define LCD_SCL_Pin GPIO_PIN_10
#define LCD_SCL_GPIO_Port GPIOB
#define LCD_CS_Pin GPIO_PIN_12
#define LCD_CS_GPIO_Port GPIOB
#define SD_CARD_DET_Pin GPIO_PIN_3
#define SD_CARD_DET_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

/* ============================================================================
 * Board Pin Aliases
 * ============================================================================*/

/* Wake-up Button (alias for KEY on PA0) */
#define WKUP_Pin                KEY_Pin
#define WKUP_GPIO_Port          KEY_GPIO_Port

/* USB FS Pins */
#define USB_FS_DM_Pin           GPIO_PIN_11
#define USB_FS_DM_GPIO_Port     GPIOA
#define USB_FS_DP_Pin           GPIO_PIN_12
#define USB_FS_DP_GPIO_Port     GPIOA

/* USART1 Pins (Debug) */
#define USART1_TX_Pin           GPIO_PIN_9
#define USART1_TX_GPIO_Port     GPIOA
#define USART1_RX_Pin           GPIO_PIN_10
#define USART1_RX_GPIO_Port     GPIOA

/* Debug Port (SWD) - Reference Only */
#define SWDIO_Pin               GPIO_PIN_13
#define SWDIO_GPIO_Port         GPIOA
#define SWCLK_Pin               GPIO_PIN_14
#define SWCLK_GPIO_Port         GPIOA
#define SWO_Pin                 GPIO_PIN_3
#define SWO_GPIO_Port           GPIOB

/* Boot Configuration Reference */
/* BOOT0: External pin (via resistor to GND/VCC) */
/* BOOT1/PB2: Shared with LED_G */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
