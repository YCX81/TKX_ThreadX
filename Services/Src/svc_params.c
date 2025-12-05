/**
 ******************************************************************************
 * @file    svc_params.c
 * @brief   Parameter Service Implementation
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "svc_params.h"
#include "stm32f4xx_hal.h"
#include "crc.h"
#include <string.h>
#include <stddef.h>

/* Private variables ---------------------------------------------------------*/
static safety_params_t s_safety_params;
static boot_config_t s_boot_config;
static bool s_params_valid = false;
static bool s_initialized = false;

/* ============================================================================
 * Private Function Prototypes
 * ============================================================================*/
static shared_status_t ValidateMagicNumber(void);
static shared_status_t ValidateCRC(void);
static shared_status_t ValidateRedundancy(void);
static shared_status_t ValidateRanges(void);

/* ============================================================================
 * Implementation
 * ============================================================================*/

shared_status_t Svc_Params_Init(void)
{
    /* Read boot configuration from Flash */
    memcpy(&s_boot_config,
           (const void *)BOOT_CONFIG_ADDR,
           sizeof(boot_config_t));

    /* Read safety parameters from Flash */
    memcpy(&s_safety_params,
           (const void *)SAFETY_PARAMS_ADDR,
           sizeof(safety_params_t));

    s_initialized = true;

    /* Validate parameters */
    return Svc_Params_Validate();
}

shared_status_t Svc_Params_Validate(void)
{
    shared_status_t status;

    if (!s_initialized)
    {
        return STATUS_ERROR;
    }

    s_params_valid = false;

    /* Step 1: Validate magic numbers */
    status = ValidateMagicNumber();
    if (status != STATUS_OK)
    {
        return status;
    }

    /* Step 2: Validate CRC */
    status = ValidateCRC();
    if (status != STATUS_OK)
    {
        return status;
    }

    /* Step 3: Validate redundancy fields */
    status = ValidateRedundancy();
    if (status != STATUS_OK)
    {
        return status;
    }

    /* Step 4: Validate parameter ranges */
    status = ValidateRanges();
    if (status != STATUS_OK)
    {
        return status;
    }

    s_params_valid = true;
    return STATUS_OK;
}

bool Svc_Params_IsValid(void)
{
    return s_params_valid;
}

const safety_params_t* Svc_Params_GetSafety(void)
{
    return s_params_valid ? &s_safety_params : NULL;
}

const boot_config_t* Svc_Params_GetBootConfig(void)
{
    return s_initialized ? &s_boot_config : NULL;
}

float Svc_Params_GetHallOffset(uint8_t channel)
{
    if (!s_params_valid || channel >= 3)
    {
        return 0.0f;
    }
    return s_safety_params.hall_offset[channel];
}

float Svc_Params_GetHallGain(uint8_t channel)
{
    if (!s_params_valid || channel >= 3)
    {
        return 1.0f;
    }
    return s_safety_params.hall_gain[channel];
}

float Svc_Params_GetAdcGain(uint8_t channel)
{
    if (!s_params_valid || channel >= 8)
    {
        return 1.0f;
    }
    return s_safety_params.adc_gain[channel];
}

float Svc_Params_GetAdcOffset(uint8_t channel)
{
    if (!s_params_valid || channel >= 8)
    {
        return 0.0f;
    }
    return s_safety_params.adc_offset[channel];
}

float Svc_Params_GetSafetyThreshold(uint8_t index)
{
    if (!s_params_valid || index >= 4)
    {
        return 0.0f;
    }
    return s_safety_params.safety_threshold[index];
}

/* ============================================================================
 * Private Functions
 * ============================================================================*/

static shared_status_t ValidateMagicNumber(void)
{
    /* Check boot config magic */
    if (s_boot_config.magic != BOOT_CONFIG_MAGIC)
    {
        return STATUS_ERROR_MAGIC;
    }

    /* Check safety params magic */
    if (s_safety_params.magic != SAFETY_PARAMS_MAGIC)
    {
        return STATUS_ERROR_MAGIC;
    }

    /* Check version */
    if (s_safety_params.version != SAFETY_PARAMS_VERSION)
    {
        return STATUS_ERROR_MAGIC;
    }

    return STATUS_OK;
}

static shared_status_t ValidateCRC(void)
{
    /* Calculate CRC32 of safety parameters (excluding CRC field) */
    __HAL_CRC_DR_RESET(&hcrc);

    uint32_t calc_crc = HAL_CRC_Calculate(&hcrc,
                                          (uint32_t *)&s_safety_params,
                                          (sizeof(safety_params_t) - 4) / 4);

    if (calc_crc != s_safety_params.crc32)
    {
        return STATUS_ERROR_CRC;
    }

    return STATUS_OK;
}

static shared_status_t ValidateRedundancy(void)
{
    uint32_t val, val_inv;
    const uint8_t *base = (const uint8_t *)&s_safety_params;

    /* Verify HALL offset redundancy */
    for (int i = 0; i < 3; i++)
    {
        memcpy(&val, base + offsetof(safety_params_t, hall_offset) + i * sizeof(float), sizeof(uint32_t));
        memcpy(&val_inv, base + offsetof(safety_params_t, hall_offset_inv) + i * sizeof(float), sizeof(uint32_t));

        if (val != ~val_inv)
        {
            return STATUS_ERROR_REDUNDANCY;
        }
    }

    /* Verify HALL gain redundancy */
    for (int i = 0; i < 3; i++)
    {
        memcpy(&val, base + offsetof(safety_params_t, hall_gain) + i * sizeof(float), sizeof(uint32_t));
        memcpy(&val_inv, base + offsetof(safety_params_t, hall_gain_inv) + i * sizeof(float), sizeof(uint32_t));

        if (val != ~val_inv)
        {
            return STATUS_ERROR_REDUNDANCY;
        }
    }

    return STATUS_OK;
}

static shared_status_t ValidateRanges(void)
{
    /* Validate HALL offsets */
    for (int i = 0; i < 3; i++)
    {
        if (!IN_RANGE(s_safety_params.hall_offset[i],
                     HALL_OFFSET_MIN, HALL_OFFSET_MAX))
        {
            return STATUS_ERROR_RANGE;
        }
    }

    /* Validate HALL gains */
    for (int i = 0; i < 3; i++)
    {
        if (!IN_RANGE(s_safety_params.hall_gain[i],
                     HALL_GAIN_MIN, HALL_GAIN_MAX))
        {
            return STATUS_ERROR_RANGE;
        }
    }

    /* Validate ADC gains */
    for (int i = 0; i < 8; i++)
    {
        if (!IN_RANGE(s_safety_params.adc_gain[i],
                     ADC_GAIN_MIN, ADC_GAIN_MAX))
        {
            return STATUS_ERROR_RANGE;
        }
    }

    /* Validate ADC offsets */
    for (int i = 0; i < 8; i++)
    {
        if (!IN_RANGE(s_safety_params.adc_offset[i],
                     ADC_OFFSET_MIN, ADC_OFFSET_MAX))
        {
            return STATUS_ERROR_RANGE;
        }
    }

    /* Validate safety thresholds */
    for (int i = 0; i < 4; i++)
    {
        if (!IN_RANGE(s_safety_params.safety_threshold[i],
                     SAFETY_THRESHOLD_MIN, SAFETY_THRESHOLD_MAX))
        {
            return STATUS_ERROR_RANGE;
        }
    }

    return STATUS_OK;
}
