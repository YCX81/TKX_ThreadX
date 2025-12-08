/**
 ******************************************************************************
 * @file    safety_params.h
 * @brief   Safety Parameter Validation Interface
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

#ifndef __SAFETY_PARAMS_H
#define __SAFETY_PARAMS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "safety_config.h"
#include "shared_config.h"

/* ============================================================================
 * Validation Result Codes
 * ============================================================================*/

/**
 * @brief Parameter validation result
 */
typedef enum {
    PARAMS_VALID            = 0x00U,    /* All parameters valid */
    PARAMS_ERR_MAGIC        = 0x01U,    /* Magic number invalid */
    PARAMS_ERR_VERSION      = 0x02U,    /* Version mismatch */
    PARAMS_ERR_SIZE         = 0x03U,    /* Size mismatch */
    PARAMS_ERR_CRC          = 0x04U,    /* CRC mismatch */
    PARAMS_ERR_HALL_RANGE   = 0x05U,    /* HALL parameter out of range */
    PARAMS_ERR_ADC_RANGE    = 0x06U,    /* ADC parameter out of range */
    PARAMS_ERR_THRESHOLD    = 0x07U,    /* Threshold out of range */
    PARAMS_ERR_REDUNDANCY   = 0x08U,    /* Redundancy check failed */
    PARAMS_ERR_NULL_PTR     = 0x09U,    /* Null pointer */
    PARAMS_ERR_FLASH_READ   = 0x0AU     /* Flash read error */
} params_result_t;

/* ============================================================================
 * Validation Statistics
 * ============================================================================*/

/**
 * @brief Parameter validation statistics
 */
typedef struct {
    uint32_t validation_count;          /* Total validation runs */
    uint32_t pass_count;                /* Successful validations */
    uint32_t fail_count;                /* Failed validations */
    params_result_t last_result;        /* Last validation result */
    uint32_t last_fail_index;           /* Index of last failed parameter */
    uint32_t last_validation_time;      /* Timestamp of last validation */
} params_stats_t;

/* ============================================================================
 * Public Function Prototypes
 * ============================================================================*/

/**
 * @brief Initialize parameter validation module
 * @retval safety_status_t Status
 */
safety_status_t Safety_Params_Init(void);

/**
 * @brief Validate safety parameters structure
 * @param params Pointer to safety parameters
 * @retval params_result_t Validation result
 */
params_result_t Safety_Params_Validate(const safety_params_t *params);

/**
 * @brief Validate safety parameters from Flash
 * @retval params_result_t Validation result
 */
params_result_t Safety_Params_ValidateFlash(void);

/**
 * @brief Validate boot configuration
 * @param config Pointer to boot configuration
 * @retval params_result_t Validation result
 */
params_result_t Safety_Params_ValidateBootConfig(const boot_config_t *config);

/**
 * @brief Get pointer to validated safety parameters
 * @note Returns NULL if parameters not validated
 * @retval const safety_params_t* Pointer to parameters or NULL
 */
const safety_params_t* Safety_Params_Get(void);

/**
 * @brief Check if parameters are valid
 * @retval bool true if valid
 */
bool Safety_Params_IsValid(void);

/**
 * @brief Get validation statistics
 * @retval const params_stats_t* Pointer to statistics
 */
const params_stats_t* Safety_Params_GetStats(void);

/**
 * @brief Run periodic parameter integrity check
 * @note Called from safety monitor thread
 * @retval params_result_t Validation result
 */
params_result_t Safety_Params_PeriodicCheck(void);

/**
 * @brief Calculate CRC32 for parameter structure
 * @param data Pointer to data
 * @param size Data size in bytes
 * @retval uint32_t CRC32 value
 */
uint32_t Safety_Params_CalculateCRC(const void *data, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* __SAFETY_PARAMS_H */
