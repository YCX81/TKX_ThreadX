/**
 ******************************************************************************
 * @file    boot_main.h
 * @brief   Functional Safety Bootloader Main Header
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Main bootloader state machine interface
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

#ifndef __BOOT_MAIN_H
#define __BOOT_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "boot_config.h"

/* ============================================================================
 * Bootloader State Machine
 * ============================================================================*/
typedef enum {
    BOOT_STATE_INIT             = 0x00U,
    BOOT_STATE_SELFTEST         = 0x01U,
    BOOT_STATE_VALIDATE_PARAMS  = 0x02U,
    BOOT_STATE_CHECK_CONFIG     = 0x03U,
    BOOT_STATE_FACTORY_MODE     = 0x04U,
    BOOT_STATE_VERIFY_APP       = 0x05U,
    BOOT_STATE_JUMP_TO_APP      = 0x06U,
    BOOT_STATE_SAFE             = 0x07U,
    BOOT_STATE_ERROR            = 0xFFU
} boot_state_t;

/* ============================================================================
 * Function Prototypes
 * ============================================================================*/

/**
 * @brief  Main bootloader entry point
 * @note   This function never returns under normal operation
 */
void Boot_Main(void);

/**
 * @brief  Get current bootloader state
 * @retval Current boot state
 */
boot_state_t Boot_GetState(void);

/**
 * @brief  Get last error code
 * @retval Last boot error code
 */
boot_status_t Boot_GetLastError(void);

/**
 * @brief  Enter safe state
 * @param  error: Error code that triggered safe state
 * @note   This function does not return
 */
void Boot_EnterSafeState(boot_status_t error);

/**
 * @brief  Validate safety parameters from Flash
 * @param  params: Pointer to safety params structure to fill
 * @retval BOOT_OK if valid, error code otherwise
 */
boot_status_t Boot_ValidateSafetyParams(safety_params_t *params);

/**
 * @brief  Load non-safety parameters from EEPROM/Flash
 * @param  params: Pointer to non-safety params structure to fill
 * @retval BOOT_OK if valid, error code otherwise
 */
boot_status_t Boot_LoadNonSafetyParams(nonsafety_params_t *params);

/**
 * @brief  Load default non-safety parameters
 * @param  params: Pointer to non-safety params structure to fill
 */
void Boot_LoadDefaultParams(nonsafety_params_t *params);

/**
 * @brief  Read boot configuration from Flash
 * @param  config: Pointer to config structure to fill
 * @retval BOOT_OK if valid, error code otherwise
 */
boot_status_t Boot_ReadConfig(boot_config_t *config);

/**
 * @brief  Write boot configuration to Flash
 * @param  config: Pointer to config structure to write
 * @retval BOOT_OK if successful, error code otherwise
 */
boot_status_t Boot_WriteConfig(const boot_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* __BOOT_MAIN_H */
