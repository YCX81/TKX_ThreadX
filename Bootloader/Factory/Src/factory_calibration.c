/**
 ******************************************************************************
 * @file    factory_calibration.c
 * @brief   Factory Calibration Data Management Implementation
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Calibration data management with range validation and redundancy.
 *
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "factory_calibration.h"
#include <string.h>
#include <math.h>

/* Private function prototypes -----------------------------------------------*/
static int Factory_IsFloatValid(float value);
static void Factory_StoreWithInverse(float value, float *primary, float *inverse);

/* ============================================================================
 * Initialization
 * ============================================================================*/

/**
 * @brief  Initialize calibration module
 */
factory_status_t Factory_Calibration_Init(void)
{
    /* Nothing to initialize currently */
    return FACTORY_OK;
}

/* ============================================================================
 * Validation
 * ============================================================================*/

/**
 * @brief  Validate calibration parameters
 */
factory_status_t Factory_Calibration_Validate(const safety_params_t *params)
{
    uint32_t i;

    if (params == NULL)
    {
        return FACTORY_ERROR;
    }

    /* Validate HALL offsets */
    for (i = 0; i < 3; i++)
    {
        if (!Factory_IsFloatValid(params->hall_offset[i]))
        {
            return FACTORY_CAL_INVALID;
        }
        if (params->hall_offset[i] < HALL_OFFSET_MIN ||
            params->hall_offset[i] > HALL_OFFSET_MAX)
        {
            return FACTORY_CAL_INVALID;
        }
    }

    /* Validate HALL gains */
    for (i = 0; i < 3; i++)
    {
        if (!Factory_IsFloatValid(params->hall_gain[i]))
        {
            return FACTORY_CAL_INVALID;
        }
        if (params->hall_gain[i] < HALL_GAIN_MIN ||
            params->hall_gain[i] > HALL_GAIN_MAX)
        {
            return FACTORY_CAL_INVALID;
        }
    }

    /* Validate ADC gains */
    for (i = 0; i < 8; i++)
    {
        if (!Factory_IsFloatValid(params->adc_gain[i]))
        {
            return FACTORY_CAL_INVALID;
        }
        if (params->adc_gain[i] < ADC_GAIN_MIN ||
            params->adc_gain[i] > ADC_GAIN_MAX)
        {
            return FACTORY_CAL_INVALID;
        }
    }

    /* Validate ADC offsets */
    for (i = 0; i < 8; i++)
    {
        if (!Factory_IsFloatValid(params->adc_offset[i]))
        {
            return FACTORY_CAL_INVALID;
        }
        if (params->adc_offset[i] < ADC_OFFSET_MIN ||
            params->adc_offset[i] > ADC_OFFSET_MAX)
        {
            return FACTORY_CAL_INVALID;
        }
    }

    /* Validate safety thresholds */
    for (i = 0; i < 4; i++)
    {
        if (!Factory_IsFloatValid(params->safety_threshold[i]))
        {
            return FACTORY_CAL_INVALID;
        }
        if (params->safety_threshold[i] < SAFETY_THRESHOLD_MIN ||
            params->safety_threshold[i] > SAFETY_THRESHOLD_MAX)
        {
            return FACTORY_CAL_INVALID;
        }
    }

    return FACTORY_OK;
}

/**
 * @brief  Prepare redundancy fields (inverted copies)
 */
void Factory_Calibration_PrepareRedundancy(safety_params_t *params)
{
    uint32_t i;

    if (params == NULL)
    {
        return;
    }

    /* Prepare HALL offset redundancy */
    for (i = 0; i < 3; i++)
    {
        Factory_StoreWithInverse(params->hall_offset[i],
                                  &params->hall_offset[i],
                                  &params->hall_offset_inv[i]);
    }

    /* Prepare HALL gain redundancy */
    for (i = 0; i < 3; i++)
    {
        Factory_StoreWithInverse(params->hall_gain[i],
                                  &params->hall_gain[i],
                                  &params->hall_gain_inv[i]);
    }
}

/**
 * @brief  Set default calibration values
 */
void Factory_Calibration_SetDefaults(safety_params_t *params)
{
    uint32_t i;

    if (params == NULL)
    {
        return;
    }

    memset(params, 0, sizeof(safety_params_t));

    /* Set magic and version */
    params->magic = SAFETY_PARAMS_MAGIC;
    params->version = SAFETY_PARAMS_VERSION;
    params->size = sizeof(safety_params_t);

    /* Default HALL calibration (unity gain, zero offset) */
    for (i = 0; i < 3; i++)
    {
        params->hall_offset[i] = 0.0f;
        params->hall_gain[i] = 1.0f;
    }

    /* Default ADC calibration (unity gain, zero offset) */
    for (i = 0; i < 8; i++)
    {
        params->adc_gain[i] = 1.0f;
        params->adc_offset[i] = 0.0f;
    }

    /* Default safety thresholds */
    params->safety_threshold[0] = 1000.0f;  /* Default threshold 1 */
    params->safety_threshold[1] = 2000.0f;  /* Default threshold 2 */
    params->safety_threshold[2] = 3000.0f;  /* Default threshold 3 */
    params->safety_threshold[3] = 4000.0f;  /* Default threshold 4 */

    /* Prepare redundancy */
    Factory_Calibration_PrepareRedundancy(params);
}

/* ============================================================================
 * HALL Sensor Calibration
 * ============================================================================*/

/**
 * @brief  Get HALL sensor offset
 */
factory_status_t Factory_Calibration_GetHallOffset(const safety_params_t *params,
                                                    uint8_t channel,
                                                    float *offset)
{
    if (params == NULL || offset == NULL || channel >= 3)
    {
        return FACTORY_ERROR;
    }

    *offset = params->hall_offset[channel];
    return FACTORY_OK;
}

/**
 * @brief  Set HALL sensor offset
 */
factory_status_t Factory_Calibration_SetHallOffset(safety_params_t *params,
                                                    uint8_t channel,
                                                    float offset)
{
    if (params == NULL || channel >= 3)
    {
        return FACTORY_ERROR;
    }

    /* Validate range */
    if (offset < HALL_OFFSET_MIN || offset > HALL_OFFSET_MAX)
    {
        return FACTORY_CAL_INVALID;
    }

    params->hall_offset[channel] = offset;
    return FACTORY_OK;
}

/**
 * @brief  Get HALL sensor gain
 */
factory_status_t Factory_Calibration_GetHallGain(const safety_params_t *params,
                                                  uint8_t channel,
                                                  float *gain)
{
    if (params == NULL || gain == NULL || channel >= 3)
    {
        return FACTORY_ERROR;
    }

    *gain = params->hall_gain[channel];
    return FACTORY_OK;
}

/**
 * @brief  Set HALL sensor gain
 */
factory_status_t Factory_Calibration_SetHallGain(safety_params_t *params,
                                                  uint8_t channel,
                                                  float gain)
{
    if (params == NULL || channel >= 3)
    {
        return FACTORY_ERROR;
    }

    /* Validate range */
    if (gain < HALL_GAIN_MIN || gain > HALL_GAIN_MAX)
    {
        return FACTORY_CAL_INVALID;
    }

    params->hall_gain[channel] = gain;
    return FACTORY_OK;
}

/* ============================================================================
 * ADC Calibration
 * ============================================================================*/

/**
 * @brief  Get ADC channel gain
 */
factory_status_t Factory_Calibration_GetAdcGain(const safety_params_t *params,
                                                 uint8_t channel,
                                                 float *gain)
{
    if (params == NULL || gain == NULL || channel >= 8)
    {
        return FACTORY_ERROR;
    }

    *gain = params->adc_gain[channel];
    return FACTORY_OK;
}

/**
 * @brief  Set ADC channel gain
 */
factory_status_t Factory_Calibration_SetAdcGain(safety_params_t *params,
                                                 uint8_t channel,
                                                 float gain)
{
    if (params == NULL || channel >= 8)
    {
        return FACTORY_ERROR;
    }

    /* Validate range */
    if (gain < ADC_GAIN_MIN || gain > ADC_GAIN_MAX)
    {
        return FACTORY_CAL_INVALID;
    }

    params->adc_gain[channel] = gain;
    return FACTORY_OK;
}

/**
 * @brief  Get ADC channel offset
 */
factory_status_t Factory_Calibration_GetAdcOffset(const safety_params_t *params,
                                                   uint8_t channel,
                                                   float *offset)
{
    if (params == NULL || offset == NULL || channel >= 8)
    {
        return FACTORY_ERROR;
    }

    *offset = params->adc_offset[channel];
    return FACTORY_OK;
}

/**
 * @brief  Set ADC channel offset
 */
factory_status_t Factory_Calibration_SetAdcOffset(safety_params_t *params,
                                                   uint8_t channel,
                                                   float offset)
{
    if (params == NULL || channel >= 8)
    {
        return FACTORY_ERROR;
    }

    /* Validate range */
    if (offset < ADC_OFFSET_MIN || offset > ADC_OFFSET_MAX)
    {
        return FACTORY_CAL_INVALID;
    }

    params->adc_offset[channel] = offset;
    return FACTORY_OK;
}

/* ============================================================================
 * Safety Thresholds
 * ============================================================================*/

/**
 * @brief  Get safety threshold
 */
factory_status_t Factory_Calibration_GetThreshold(const safety_params_t *params,
                                                   uint8_t index,
                                                   float *threshold)
{
    if (params == NULL || threshold == NULL || index >= 4)
    {
        return FACTORY_ERROR;
    }

    *threshold = params->safety_threshold[index];
    return FACTORY_OK;
}

/**
 * @brief  Set safety threshold
 */
factory_status_t Factory_Calibration_SetThreshold(safety_params_t *params,
                                                   uint8_t index,
                                                   float threshold)
{
    if (params == NULL || index >= 4)
    {
        return FACTORY_ERROR;
    }

    /* Validate range */
    if (threshold < SAFETY_THRESHOLD_MIN || threshold > SAFETY_THRESHOLD_MAX)
    {
        return FACTORY_CAL_INVALID;
    }

    params->safety_threshold[index] = threshold;
    return FACTORY_OK;
}

/* ============================================================================
 * Private Functions
 * ============================================================================*/

/**
 * @brief  Check if float value is valid (not NaN or Inf)
 */
static int Factory_IsFloatValid(float value)
{
    /* Check for NaN */
    if (value != value)
    {
        return 0;
    }

    /* Check for infinity */
    if (value > 3.4e38f || value < -3.4e38f)
    {
        return 0;
    }

    return 1;
}

/**
 * @brief  Store value with its bitwise inverse
 */
static void Factory_StoreWithInverse(float value, float *primary, float *inverse)
{
    uint32_t val_bits;
    uint32_t inv_bits;

    *primary = value;

    /* Get bit representation */
    memcpy(&val_bits, &value, sizeof(uint32_t));

    /* Create inverse */
    inv_bits = ~val_bits;

    /* Store inverse */
    memcpy(inverse, &inv_bits, sizeof(uint32_t));
}
