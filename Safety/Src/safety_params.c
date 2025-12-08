/**
 ******************************************************************************
 * @file    safety_params.c
 * @brief   Safety Parameter Validation Implementation
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Parameter validation for functional safety
 * Target: STM32F407VGT6
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "safety_params.h"
#include "safety_core.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include <math.h>

#if DIAG_RTT_ENABLED
#include "bsp_debug.h"
#endif

/* ============================================================================
 * Private Defines
 * ============================================================================*/

#define FLOAT_TOLERANCE         0.0001f     /* Float comparison tolerance */

/* ============================================================================
 * Private Variables
 * ============================================================================*/

static params_stats_t s_params_stats;
static bool s_params_valid = false;
static safety_params_t s_cached_params;

/* ============================================================================
 * Private Function Prototypes
 * ============================================================================*/

static params_result_t Params_ValidateHeader(const safety_params_t *params);
static params_result_t Params_ValidateHallParams(const safety_params_t *params);
static params_result_t Params_ValidateAdcParams(const safety_params_t *params);
static params_result_t Params_ValidateThresholds(const safety_params_t *params);
static params_result_t Params_ValidateRedundancy(const safety_params_t *params);
static params_result_t Params_ValidateCRC(const safety_params_t *params);
static bool Params_FloatInRange(float value, float min, float max);
static bool Params_CheckFloatInverted(float val, float inv);

/* ============================================================================
 * Public Functions
 * ============================================================================*/

safety_status_t Safety_Params_Init(void)
{
    /* Clear statistics */
    memset(&s_params_stats, 0, sizeof(params_stats_t));
    memset(&s_cached_params, 0, sizeof(safety_params_t));
    s_params_valid = false;

#if DIAG_RTT_ENABLED
    DEBUG_INFO("Safety Params: Module initialized");
#endif

    return SAFETY_OK;
}

params_result_t Safety_Params_Validate(const safety_params_t *params)
{
    params_result_t result;

    if (params == NULL)
    {
        s_params_stats.fail_count++;
        s_params_stats.last_result = PARAMS_ERR_NULL_PTR;
        return PARAMS_ERR_NULL_PTR;
    }

    s_params_stats.validation_count++;
    s_params_stats.last_validation_time = HAL_GetTick();

    /* Step 1: Validate header (magic, version, size) */
    result = Params_ValidateHeader(params);
    if (result != PARAMS_VALID)
    {
        goto validation_failed;
    }

    /* Step 2: Validate CRC */
    result = Params_ValidateCRC(params);
    if (result != PARAMS_VALID)
    {
        goto validation_failed;
    }

    /* Step 3: Validate HALL parameters */
    result = Params_ValidateHallParams(params);
    if (result != PARAMS_VALID)
    {
        goto validation_failed;
    }

    /* Step 4: Validate ADC parameters */
    result = Params_ValidateAdcParams(params);
    if (result != PARAMS_VALID)
    {
        goto validation_failed;
    }

    /* Step 5: Validate thresholds */
    result = Params_ValidateThresholds(params);
    if (result != PARAMS_VALID)
    {
        goto validation_failed;
    }

    /* Step 6: Validate redundancy (inverted copies) */
    result = Params_ValidateRedundancy(params);
    if (result != PARAMS_VALID)
    {
        goto validation_failed;
    }

    /* All checks passed */
    s_params_stats.pass_count++;
    s_params_stats.last_result = PARAMS_VALID;
    s_params_valid = true;

    /* Cache valid parameters */
    memcpy(&s_cached_params, params, sizeof(safety_params_t));

#if DIAG_RTT_ENABLED
    DEBUG_INFO("Safety Params: Validation PASSED");
#endif

    return PARAMS_VALID;

validation_failed:
    s_params_stats.fail_count++;
    s_params_stats.last_result = result;
    s_params_valid = false;

#if DIAG_RTT_ENABLED
    DEBUG_ERROR("Safety Params: Validation FAILED (result=%d)", result);
#endif

    /* Report to safety core */
    Safety_ReportError(SAFETY_ERR_PARAM_INVALID, (uint32_t)result, 0);

    return result;
}

params_result_t Safety_Params_ValidateFlash(void)
{
    const safety_params_t *flash_params = (const safety_params_t *)SAFETY_PARAMS_ADDR;

#if DIAG_RTT_ENABLED
    DEBUG_INFO("Safety Params: Validating Flash @ 0x%08lX", SAFETY_PARAMS_ADDR);
#endif

    return Safety_Params_Validate(flash_params);
}

params_result_t Safety_Params_ValidateBootConfig(const boot_config_t *config)
{
    if (config == NULL)
    {
        return PARAMS_ERR_NULL_PTR;
    }

    /* Check magic number */
    if (config->magic != BOOT_CONFIG_MAGIC)
    {
#if DIAG_RTT_ENABLED
        DEBUG_ERROR("Boot Config: Invalid magic 0x%08lX", config->magic);
#endif
        return PARAMS_ERR_MAGIC;
    }

    /* Calculate CRC (excluding CRC field itself) */
    uint32_t calc_crc = Safety_Params_CalculateCRC(config,
                        sizeof(boot_config_t) - sizeof(uint32_t));

    if (calc_crc != config->crc)
    {
#if DIAG_RTT_ENABLED
        DEBUG_ERROR("Boot Config: CRC mismatch (calc=0x%08lX, stored=0x%08lX)",
                    calc_crc, config->crc);
#endif
        return PARAMS_ERR_CRC;
    }

#if DIAG_RTT_ENABLED
    DEBUG_INFO("Boot Config: Validation PASSED");
#endif

    return PARAMS_VALID;
}

const safety_params_t* Safety_Params_Get(void)
{
    return s_params_valid ? &s_cached_params : NULL;
}

bool Safety_Params_IsValid(void)
{
    return s_params_valid;
}

const params_stats_t* Safety_Params_GetStats(void)
{
    return &s_params_stats;
}

params_result_t Safety_Params_PeriodicCheck(void)
{
    if (!s_params_valid)
    {
        return PARAMS_ERR_NULL_PTR;
    }

    /* Re-validate cached parameters */
    const safety_params_t *flash_params = (const safety_params_t *)SAFETY_PARAMS_ADDR;

    /* Quick CRC check */
    params_result_t result = Params_ValidateCRC(flash_params);
    if (result != PARAMS_VALID)
    {
#if DIAG_RTT_ENABLED
        DEBUG_ERROR("Safety Params: Periodic check FAILED");
#endif
        s_params_valid = false;
        Safety_ReportError(SAFETY_ERR_PARAM_INVALID, (uint32_t)result, 1);
        return result;
    }

    return PARAMS_VALID;
}

uint32_t Safety_Params_CalculateCRC(const void *data, uint32_t size)
{
    /* Use hardware CRC unit */
    CRC_HandleTypeDef hcrc;
    hcrc.Instance = CRC;

    __HAL_RCC_CRC_CLK_ENABLE();
    HAL_CRC_Init(&hcrc);

    /* Reset CRC calculation unit */
    __HAL_CRC_DR_RESET(&hcrc);

    /* Calculate CRC word by word */
    const uint32_t *pData = (const uint32_t *)data;
    uint32_t words = size / 4;
    uint32_t crc = 0;

    for (uint32_t i = 0; i < words; i++)
    {
        crc = HAL_CRC_Accumulate(&hcrc, (uint32_t *)&pData[i], 1);
    }

    /* Handle remaining bytes */
    uint32_t remaining = size % 4;
    if (remaining > 0)
    {
        uint32_t last_word = 0;
        memcpy(&last_word, &((const uint8_t *)data)[words * 4], remaining);
        crc = HAL_CRC_Accumulate(&hcrc, &last_word, 1);
    }

    return crc;
}

/* ============================================================================
 * Private Functions
 * ============================================================================*/

static params_result_t Params_ValidateHeader(const safety_params_t *params)
{
    /* Check magic number */
    if (params->magic != SAFETY_PARAMS_MAGIC)
    {
#if DIAG_RTT_ENABLED
        DEBUG_ERROR("Params: Invalid magic 0x%08lX (expected 0x%08lX)",
                    params->magic, SAFETY_PARAMS_MAGIC);
#endif
        return PARAMS_ERR_MAGIC;
    }

    /* Check version */
    if (params->version != SAFETY_PARAMS_VERSION)
    {
#if DIAG_RTT_ENABLED
        DEBUG_WARN("Params: Version mismatch 0x%04X (expected 0x%04X)",
                   params->version, SAFETY_PARAMS_VERSION);
#endif
        /* Version mismatch is warning, not error for now */
    }

    /* Check size */
    if (params->size != sizeof(safety_params_t))
    {
#if DIAG_RTT_ENABLED
        DEBUG_ERROR("Params: Size mismatch %u (expected %u)",
                    params->size, sizeof(safety_params_t));
#endif
        return PARAMS_ERR_SIZE;
    }

    return PARAMS_VALID;
}

static params_result_t Params_ValidateCRC(const safety_params_t *params)
{
    /* Calculate CRC (excluding CRC field itself) */
    uint32_t calc_crc = Safety_Params_CalculateCRC(params,
                        sizeof(safety_params_t) - sizeof(uint32_t));

    if (calc_crc != params->crc32)
    {
#if DIAG_RTT_ENABLED
        DEBUG_ERROR("Params: CRC mismatch (calc=0x%08lX, stored=0x%08lX)",
                    calc_crc, params->crc32);
#endif
        return PARAMS_ERR_CRC;
    }

    return PARAMS_VALID;
}

static params_result_t Params_ValidateHallParams(const safety_params_t *params)
{
    /* Validate HALL offset values */
    for (int i = 0; i < 3; i++)
    {
        if (!Params_FloatInRange(params->hall_offset[i],
                                 HALL_OFFSET_MIN, HALL_OFFSET_MAX))
        {
            s_params_stats.last_fail_index = i;
#if DIAG_RTT_ENABLED
            DEBUG_ERROR("Params: HALL offset[%d] out of range: %f", i,
                        (double)params->hall_offset[i]);
#endif
            return PARAMS_ERR_HALL_RANGE;
        }

        if (!Params_FloatInRange(params->hall_gain[i],
                                 HALL_GAIN_MIN, HALL_GAIN_MAX))
        {
            s_params_stats.last_fail_index = i + 3;
#if DIAG_RTT_ENABLED
            DEBUG_ERROR("Params: HALL gain[%d] out of range: %f", i,
                        (double)params->hall_gain[i]);
#endif
            return PARAMS_ERR_HALL_RANGE;
        }
    }

    return PARAMS_VALID;
}

static params_result_t Params_ValidateAdcParams(const safety_params_t *params)
{
    /* Validate ADC gain and offset values */
    for (int i = 0; i < 8; i++)
    {
        if (!Params_FloatInRange(params->adc_gain[i],
                                 ADC_GAIN_MIN, ADC_GAIN_MAX))
        {
            s_params_stats.last_fail_index = i;
#if DIAG_RTT_ENABLED
            DEBUG_ERROR("Params: ADC gain[%d] out of range: %f", i,
                        (double)params->adc_gain[i]);
#endif
            return PARAMS_ERR_ADC_RANGE;
        }

        if (!Params_FloatInRange(params->adc_offset[i],
                                 ADC_OFFSET_MIN, ADC_OFFSET_MAX))
        {
            s_params_stats.last_fail_index = i + 8;
#if DIAG_RTT_ENABLED
            DEBUG_ERROR("Params: ADC offset[%d] out of range: %f", i,
                        (double)params->adc_offset[i]);
#endif
            return PARAMS_ERR_ADC_RANGE;
        }
    }

    return PARAMS_VALID;
}

static params_result_t Params_ValidateThresholds(const safety_params_t *params)
{
    /* Validate safety threshold values */
    for (int i = 0; i < 4; i++)
    {
        if (!Params_FloatInRange(params->safety_threshold[i],
                                 SAFETY_THRESHOLD_MIN, SAFETY_THRESHOLD_MAX))
        {
            s_params_stats.last_fail_index = i;
#if DIAG_RTT_ENABLED
            DEBUG_ERROR("Params: Threshold[%d] out of range: %f", i,
                        (double)params->safety_threshold[i]);
#endif
            return PARAMS_ERR_THRESHOLD;
        }
    }

    return PARAMS_VALID;
}

static params_result_t Params_ValidateRedundancy(const safety_params_t *params)
{
    /* Validate inverted copies of HALL parameters */
    for (int i = 0; i < 3; i++)
    {
        if (!Params_CheckFloatInverted(params->hall_offset[i],
                                       params->hall_offset_inv[i]))
        {
            s_params_stats.last_fail_index = i;
#if DIAG_RTT_ENABLED
            DEBUG_ERROR("Params: HALL offset[%d] redundancy check failed", i);
#endif
            return PARAMS_ERR_REDUNDANCY;
        }

        if (!Params_CheckFloatInverted(params->hall_gain[i],
                                       params->hall_gain_inv[i]))
        {
            s_params_stats.last_fail_index = i + 3;
#if DIAG_RTT_ENABLED
            DEBUG_ERROR("Params: HALL gain[%d] redundancy check failed", i);
#endif
            return PARAMS_ERR_REDUNDANCY;
        }
    }

    return PARAMS_VALID;
}

static bool Params_FloatInRange(float value, float min, float max)
{
    /* Check for NaN or Inf */
    if (isnan(value) || isinf(value))
    {
        return false;
    }

    return (value >= min) && (value <= max);
}

static bool Params_CheckFloatInverted(float val, float inv)
{
    /* Check if inv is bit-inverted copy of val */
    uint32_t val_bits = *(uint32_t *)&val;
    uint32_t inv_bits = *(uint32_t *)&inv;

    return IS_INVERTED_32(val_bits, inv_bits);
}
