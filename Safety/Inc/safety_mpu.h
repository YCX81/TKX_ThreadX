/**
 ******************************************************************************
 * @file    safety_mpu.h
 * @brief   MPU Memory Protection Interface
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Memory Protection Unit configuration for runtime safety
 * Target: STM32F407VGT6 (Cortex-M4 with MPU)
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

#ifndef __SAFETY_MPU_H
#define __SAFETY_MPU_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "safety_config.h"
#include "stm32f4xx_hal.h"

/* ============================================================================
 * MPU Access Permission Definitions
 * ============================================================================*/

/* Access permissions (AP field) */
#define MPU_AP_NONE             0x00U   /* No access */
#define MPU_AP_PRIV_RW          0x01U   /* Privileged RW only */
#define MPU_AP_PRIV_RW_USER_RO  0x02U   /* Priv RW, User RO */
#define MPU_AP_FULL_ACCESS      0x03U   /* Full access */
#define MPU_AP_PRIV_RO          0x05U   /* Privileged RO only */
#define MPU_AP_RO               0x06U   /* Read-only */

/* Execute permission */
#define MPU_XN_ENABLE           0x01U   /* Execute Never */
#define MPU_XN_DISABLE          0x00U   /* Execute allowed */

/* Memory type attributes */
#define MPU_TEX_STRONGLY_ORDERED    0x00U
#define MPU_TEX_DEVICE              0x00U
#define MPU_TEX_NORMAL_NONCACHE     0x01U
#define MPU_TEX_NORMAL_WBWA         0x01U
#define MPU_TEX_NORMAL_WTNA         0x00U

/* ============================================================================
 * MPU Region Configuration Structure
 * ============================================================================*/

/**
 * @brief MPU region configuration
 */
typedef struct {
    uint32_t base_address;      /* Region base address (must be aligned) */
    uint8_t  region_number;     /* Region number (0-7 for Cortex-M4) */
    uint8_t  size;              /* Region size (MPU_REGION_SIZE_xxx) */
    uint8_t  access_permission; /* Access permission (MPU_AP_xxx) */
    uint8_t  execute_never;     /* Execute Never flag */
    uint8_t  shareable;         /* Shareable flag */
    uint8_t  cacheable;         /* Cacheable flag */
    uint8_t  bufferable;        /* Bufferable flag */
    uint8_t  tex;               /* TEX field for memory type */
    uint8_t  subregion_disable; /* Subregion disable mask */
    uint8_t  enable;            /* Region enable flag */
} mpu_region_config_t;

/* ============================================================================
 * Function Prototypes
 * ============================================================================*/

/**
 * @brief Initialize MPU with safety configuration
 * @retval safety_status_t Status
 */
safety_status_t Safety_MPU_Init(void);

/**
 * @brief Configure a single MPU region
 * @param config Region configuration
 * @retval safety_status_t Status
 */
safety_status_t Safety_MPU_ConfigRegion(const mpu_region_config_t *config);

/**
 * @brief Enable MPU protection
 * @retval safety_status_t Status
 */
safety_status_t Safety_MPU_Enable(void);

/**
 * @brief Disable MPU protection
 * @retval safety_status_t Status
 */
safety_status_t Safety_MPU_Disable(void);

/**
 * @brief Check if MPU is enabled
 * @retval bool true if enabled
 */
bool Safety_MPU_IsEnabled(void);

/**
 * @brief Get MPU region configuration
 * @param region_number Region number
 * @param config Pointer to store configuration
 * @retval safety_status_t Status
 */
safety_status_t Safety_MPU_GetRegion(uint8_t region_number,
                                     mpu_region_config_t *config);

/**
 * @brief Disable a specific MPU region
 * @param region_number Region number to disable
 * @retval safety_status_t Status
 */
safety_status_t Safety_MPU_DisableRegion(uint8_t region_number);

/**
 * @brief Get MPU type information
 * @param num_regions Pointer to store number of regions
 * @retval safety_status_t Status
 */
safety_status_t Safety_MPU_GetInfo(uint8_t *num_regions);

#ifdef __cplusplus
}
#endif

#endif /* __SAFETY_MPU_H */
