/**
 ******************************************************************************
 * @file    factory_calibration.h
 * @brief   Factory Calibration Data Management Header
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Calibration data management for safety-critical parameters.
 * Includes range validation and redundancy preparation.
 *
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

#ifndef __FACTORY_CALIBRATION_H
#define __FACTORY_CALIBRATION_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "boot_config.h"
#include "factory_mode.h"

/* ============================================================================
 * Calibration Parameter Limits
 * ============================================================================*/

/* HALL sensor offset limits (in ADC counts or normalized units) */
#define HALL_OFFSET_MIN         (-1000.0f)
#define HALL_OFFSET_MAX         (1000.0f)

/* HALL sensor gain limits */
#define HALL_GAIN_MIN           (0.5f)
#define HALL_GAIN_MAX           (2.0f)

/* ADC gain limits */
#define ADC_GAIN_MIN            (0.8f)
#define ADC_GAIN_MAX            (1.2f)

/* ADC offset limits */
#define ADC_OFFSET_MIN          (-500.0f)
#define ADC_OFFSET_MAX          (500.0f)

/* Safety threshold limits */
#define SAFETY_THRESHOLD_MIN    (0.0f)
#define SAFETY_THRESHOLD_MAX    (10000.0f)

/* ============================================================================
 * Function Prototypes
 * ============================================================================*/

/**
 * @brief  Initialize calibration module
 * @retval FACTORY_OK on success
 */
factory_status_t Factory_Calibration_Init(void);

/**
 * @brief  Validate calibration parameters
 * @param  params: Pointer to safety_params_t to validate
 * @retval FACTORY_OK if all parameters within valid range
 */
factory_status_t Factory_Calibration_Validate(const safety_params_t *params);

/**
 * @brief  Prepare redundancy fields (inverted copies)
 * @param  params: Pointer to safety_params_t to prepare
 */
void Factory_Calibration_PrepareRedundancy(safety_params_t *params);

/**
 * @brief  Set default calibration values
 * @param  params: Pointer to safety_params_t to fill with defaults
 */
void Factory_Calibration_SetDefaults(safety_params_t *params);

/**
 * @brief  Get HALL sensor offset
 * @param  params: Pointer to safety_params_t
 * @param  channel: HALL channel (0-2)
 * @param  offset: Pointer to store offset value
 * @retval FACTORY_OK if valid
 */
factory_status_t Factory_Calibration_GetHallOffset(const safety_params_t *params,
                                                    uint8_t channel,
                                                    float *offset);

/**
 * @brief  Set HALL sensor offset
 * @param  params: Pointer to safety_params_t
 * @param  channel: HALL channel (0-2)
 * @param  offset: Offset value to set
 * @retval FACTORY_OK if valid and set
 */
factory_status_t Factory_Calibration_SetHallOffset(safety_params_t *params,
                                                    uint8_t channel,
                                                    float offset);

/**
 * @brief  Get HALL sensor gain
 * @param  params: Pointer to safety_params_t
 * @param  channel: HALL channel (0-2)
 * @param  gain: Pointer to store gain value
 * @retval FACTORY_OK if valid
 */
factory_status_t Factory_Calibration_GetHallGain(const safety_params_t *params,
                                                  uint8_t channel,
                                                  float *gain);

/**
 * @brief  Set HALL sensor gain
 * @param  params: Pointer to safety_params_t
 * @param  channel: HALL channel (0-2)
 * @param  gain: Gain value to set
 * @retval FACTORY_OK if valid and set
 */
factory_status_t Factory_Calibration_SetHallGain(safety_params_t *params,
                                                  uint8_t channel,
                                                  float gain);

/**
 * @brief  Get ADC channel gain
 * @param  params: Pointer to safety_params_t
 * @param  channel: ADC channel (0-7)
 * @param  gain: Pointer to store gain value
 * @retval FACTORY_OK if valid
 */
factory_status_t Factory_Calibration_GetAdcGain(const safety_params_t *params,
                                                 uint8_t channel,
                                                 float *gain);

/**
 * @brief  Set ADC channel gain
 * @param  params: Pointer to safety_params_t
 * @param  channel: ADC channel (0-7)
 * @param  gain: Gain value to set
 * @retval FACTORY_OK if valid and set
 */
factory_status_t Factory_Calibration_SetAdcGain(safety_params_t *params,
                                                 uint8_t channel,
                                                 float gain);

/**
 * @brief  Get ADC channel offset
 * @param  params: Pointer to safety_params_t
 * @param  channel: ADC channel (0-7)
 * @param  offset: Pointer to store offset value
 * @retval FACTORY_OK if valid
 */
factory_status_t Factory_Calibration_GetAdcOffset(const safety_params_t *params,
                                                   uint8_t channel,
                                                   float *offset);

/**
 * @brief  Set ADC channel offset
 * @param  params: Pointer to safety_params_t
 * @param  channel: ADC channel (0-7)
 * @param  offset: Offset value to set
 * @retval FACTORY_OK if valid and set
 */
factory_status_t Factory_Calibration_SetAdcOffset(safety_params_t *params,
                                                   uint8_t channel,
                                                   float offset);

/**
 * @brief  Get safety threshold
 * @param  params: Pointer to safety_params_t
 * @param  index: Threshold index (0-3)
 * @param  threshold: Pointer to store threshold value
 * @retval FACTORY_OK if valid
 */
factory_status_t Factory_Calibration_GetThreshold(const safety_params_t *params,
                                                   uint8_t index,
                                                   float *threshold);

/**
 * @brief  Set safety threshold
 * @param  params: Pointer to safety_params_t
 * @param  index: Threshold index (0-3)
 * @param  threshold: Threshold value to set
 * @retval FACTORY_OK if valid and set
 */
factory_status_t Factory_Calibration_SetThreshold(safety_params_t *params,
                                                   uint8_t index,
                                                   float threshold);

#ifdef __cplusplus
}
#endif

#endif /* __FACTORY_CALIBRATION_H */
