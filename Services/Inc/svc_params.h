/**
 ******************************************************************************
 * @file    svc_params.h
 * @brief   Parameter Service Interface
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Service for reading and validating safety parameters from Flash
 * Target: STM32F407VGT6
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

#ifndef __SVC_PARAMS_H
#define __SVC_PARAMS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "shared_config.h"

/* ============================================================================
 * Function Prototypes
 * ============================================================================*/

/**
 * @brief Initialize parameter service
 * @retval shared_status_t Status
 */
shared_status_t Svc_Params_Init(void);

/**
 * @brief Validate safety parameters
 * @retval shared_status_t Status
 */
shared_status_t Svc_Params_Validate(void);

/**
 * @brief Check if parameters are valid
 * @retval bool true if valid
 */
bool Svc_Params_IsValid(void);

/**
 * @brief Get safety parameters pointer (read-only)
 * @retval const safety_params_t* Parameters pointer
 */
const safety_params_t* Svc_Params_GetSafety(void);

/**
 * @brief Get boot configuration pointer (read-only)
 * @retval const boot_config_t* Configuration pointer
 */
const boot_config_t* Svc_Params_GetBootConfig(void);

/**
 * @brief Get HALL sensor offset
 * @param channel Channel (0-2)
 * @retval float Offset value
 */
float Svc_Params_GetHallOffset(uint8_t channel);

/**
 * @brief Get HALL sensor gain
 * @param channel Channel (0-2)
 * @retval float Gain value
 */
float Svc_Params_GetHallGain(uint8_t channel);

/**
 * @brief Get ADC gain
 * @param channel Channel (0-7)
 * @retval float Gain value
 */
float Svc_Params_GetAdcGain(uint8_t channel);

/**
 * @brief Get ADC offset
 * @param channel Channel (0-7)
 * @retval float Offset value
 */
float Svc_Params_GetAdcOffset(uint8_t channel);

/**
 * @brief Get safety threshold
 * @param index Threshold index (0-3)
 * @retval float Threshold value
 */
float Svc_Params_GetSafetyThreshold(uint8_t index);

#ifdef __cplusplus
}
#endif

#endif /* __SVC_PARAMS_H */
