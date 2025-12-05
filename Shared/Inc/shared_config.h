/**
 ******************************************************************************
 * @file    shared_config.h
 * @brief   Bootloader and Application Shared Configuration
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Shared configuration between Bootloader and Application
 * Ensures consistent memory layout, data structures, and magic numbers
 * Target: STM32F407VGT6
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

#ifndef __SHARED_CONFIG_H
#define __SHARED_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * Memory Map Configuration
 * ============================================================================*/

/* Bootloader Region (48KB, Sectors 0-2) */
#define BOOT_FLASH_START        0x08000000UL
#define BOOT_FLASH_END          0x0800BFFFUL
#define BOOT_FLASH_SIZE         0x0000C000UL    /* 48KB */
#define BOOT_CRC_ADDR           0x0800BFFCUL    /* Last 4 bytes for CRC */

/* Config/Calibration Region (16KB, Sector 3) */
#define CONFIG_FLASH_START      0x0800C000UL
#define CONFIG_FLASH_END        0x0800FFFFUL
#define CONFIG_FLASH_SIZE       0x00004000UL    /* 16KB */
#define CONFIG_FLASH_SECTOR     3U

/* Application Region (448KB, Sectors 4-7) */
#define APP_FLASH_START         0x08010000UL
#define APP_FLASH_END           0x0807FFFFUL
#define APP_FLASH_SIZE          0x00070000UL    /* 448KB */
#define APP_CRC_ADDR            0x0807FFFCUL    /* Last 4 bytes for CRC */

/* RAM Regions */
#define RAM_START               0x20000000UL
#define RAM_END                 0x2001FFFFUL
#define RAM_SIZE                0x00020000UL    /* 128KB */

#define CCMRAM_START            0x10000000UL
#define CCMRAM_END              0x1000FFFFUL
#define CCMRAM_SIZE             0x00010000UL    /* 64KB */

/* RAM Test Configuration (subset of RAM for startup test) */
#define RAM_TEST_START          0x20018000UL    /* Last 32KB of RAM for test */
#define RAM_TEST_SIZE           0x00008000UL    /* 32KB test area */

/* Peripheral Region */
#define PERIPH_BASE_ADDR        0x40000000UL
#define PERIPH_SIZE             0x20000000UL    /* 512MB */

/* ============================================================================
 * Magic Numbers
 * ============================================================================*/
#define BOOT_CONFIG_MAGIC       0xC0F16000UL    /* Boot config magic */
#define SAFETY_PARAMS_MAGIC     0xCA11B000UL    /* Safety params magic */
#define NONSAFETY_PARAMS_MAGIC  0xEE9A0000UL    /* Non-safety params magic */
#define FACTORY_MODE_MAGIC      0xFAC70000UL    /* Factory mode magic */
#define APP_VALID_MAGIC         0xA5A5A5A5UL    /* Application valid marker */

/* Safety Parameters Version */
#define SAFETY_PARAMS_VERSION   0x0100U         /* Version 1.0 */

/* ============================================================================
 * Common Status Codes
 * ============================================================================*/
typedef enum {
    STATUS_OK                   = 0x00U,
    STATUS_ERROR                = 0x01U,
    STATUS_ERROR_CRC            = 0x02U,
    STATUS_ERROR_MAGIC          = 0x03U,
    STATUS_ERROR_REDUNDANCY     = 0x04U,
    STATUS_ERROR_RANGE          = 0x05U,
    STATUS_ERROR_SELFTEST       = 0x06U,
    STATUS_ERROR_TIMEOUT        = 0x07U,
    STATUS_ERROR_CLOCK          = 0x08U,
    STATUS_ERROR_INVALID        = 0x09U,
    STATUS_ERROR_MPU            = 0x0AU,
    STATUS_ERROR_STACK          = 0x0BU,
    STATUS_ERROR_FLOW           = 0x0CU
} shared_status_t;

/* ============================================================================
 * Boot Configuration Structure (stored in Config Flash)
 * ============================================================================*/
typedef struct __attribute__((packed)) {
    uint32_t magic;             /* 0xC0F16000 */
    uint32_t factory_mode;      /* 0 = Normal, non-zero = Factory Mode */
    uint32_t cal_valid;         /* Calibration data valid flag */
    uint32_t app_crc;           /* Application CRC (for quick validation) */
    uint32_t boot_count;        /* Boot counter for diagnostics */
    uint32_t last_error;        /* Last error code */
    uint32_t reserved[2];       /* Reserved for future use */
    uint32_t crc;               /* Structure CRC32 */
} boot_config_t;

#define BOOT_CONFIG_ADDR        (CONFIG_FLASH_START)
#define BOOT_CONFIG_SIZE        sizeof(boot_config_t)

/* ============================================================================
 * Safety Parameters Structure (stored in Config Flash)
 * ============================================================================*/
typedef struct __attribute__((packed)) {
    /* Header - 8 bytes */
    uint32_t magic;             /* 0xCA11B000 */
    uint16_t version;           /* Structure version */
    uint16_t size;              /* Structure size */

    /* HALL Sensor Calibration - 48 bytes */
    float hall_offset[3];       /* HALL sensor offset */
    float hall_gain[3];         /* HALL sensor gain */
    float hall_offset_inv[3];   /* Inverted copy (redundancy) */
    float hall_gain_inv[3];     /* Inverted copy (redundancy) */

    /* ADC Calibration - 64 bytes */
    float adc_gain[8];          /* ADC channel gain */
    float adc_offset[8];        /* ADC channel offset */

    /* Safety Thresholds - 16 bytes */
    float safety_threshold[4];  /* Safety threshold values */

    /* Reserved - 28 bytes */
    uint32_t reserved[7];

    /* Integrity Check - 4 bytes */
    uint32_t crc32;             /* CRC32 of entire structure */
} safety_params_t;

#define SAFETY_PARAMS_ADDR      (CONFIG_FLASH_START + sizeof(boot_config_t))
#define SAFETY_PARAMS_SIZE      sizeof(safety_params_t)

/* ============================================================================
 * Non-Safety Parameters Structure
 * ============================================================================*/
typedef struct __attribute__((packed)) {
    uint32_t magic;             /* 0xEE9A0000 */
    uint32_t can_baudrate;      /* CAN baud rate (125000-1000000) */
    uint32_t can_id_base;       /* CAN base ID */
    uint16_t comm_timeout_ms;   /* Communication timeout in ms */
    uint16_t reserved;          /* Reserved */
    uint16_t crc16;             /* CRC16 checksum */
    uint16_t padding;           /* Alignment padding */
} nonsafety_params_t;

/* Default values */
#define DEFAULT_CAN_BAUDRATE    500000U
#define DEFAULT_CAN_ID_BASE     0x100U
#define DEFAULT_COMM_TIMEOUT    1000U

/* ============================================================================
 * Parameter Validation Ranges
 * ============================================================================*/
#define HALL_OFFSET_MIN         (-1000.0f)
#define HALL_OFFSET_MAX         (1000.0f)
#define HALL_GAIN_MIN           (0.5f)
#define HALL_GAIN_MAX           (2.0f)
#define ADC_GAIN_MIN            (0.8f)
#define ADC_GAIN_MAX            (1.2f)
#define ADC_OFFSET_MIN          (-500.0f)
#define ADC_OFFSET_MAX          (500.0f)
#define SAFETY_THRESHOLD_MIN    (0.0f)
#define SAFETY_THRESHOLD_MAX    (10000.0f)

/* ============================================================================
 * Program Flow Monitor Configuration
 * ============================================================================*/
#define PFM_SIGNATURE_INIT      0x5A5A5A5AUL

/* Bootloader Checkpoints (0x01 - 0x0F) */
#define PFM_CP_BOOT_INIT            0x01U
#define PFM_CP_BOOT_SELFTEST_START  0x02U
#define PFM_CP_BOOT_SELFTEST_END    0x07U
#define PFM_CP_BOOT_PARAMS_CHECK    0x08U
#define PFM_CP_BOOT_APP_VERIFY      0x0BU
#define PFM_CP_BOOT_JUMP            0x0DU

/* Application Checkpoints (0x10 - 0x3F) */
#define PFM_CP_APP_INIT             0x10U
#define PFM_CP_APP_SAFETY_MONITOR   0x11U
#define PFM_CP_APP_WATCHDOG_FEED    0x12U
#define PFM_CP_APP_SELFTEST_START   0x13U
#define PFM_CP_APP_SELFTEST_END     0x14U
#define PFM_CP_APP_MAIN_LOOP        0x15U
#define PFM_CP_APP_COMM_HANDLER     0x16U
#define PFM_CP_APP_PARAM_CHECK      0x17U

/* ============================================================================
 * Watchdog Configuration
 * ============================================================================*/
#define IWDG_TIMEOUT_MS         1000U   /* Independent Watchdog timeout */
#define IWDG_PRESCALER          64U     /* IWDG prescaler value */
#define IWDG_RELOAD_VALUE       500U    /* IWDG reload counter */

/* ============================================================================
 * Timing Configuration
 * ============================================================================*/
#define BOOT_TIMEOUT_MS         5000U   /* Maximum boot time */
#define SELFTEST_TIMEOUT_MS     2000U   /* Maximum self-test time */
#define FACTORY_MODE_TIMEOUT_MS 300000U /* Factory mode timeout (5 min) */

/* Application timing */
#define SAFETY_MONITOR_PERIOD_MS    100U    /* Safety thread period */
#define FLASH_CRC_CHECK_INTERVAL_MS 300000U /* Flash CRC check (5 min) */

/* ============================================================================
 * Version Information
 * ============================================================================*/
#define SHARED_VERSION_MAJOR    1U
#define SHARED_VERSION_MINOR    0U
#define SHARED_VERSION_PATCH    0U

/* ============================================================================
 * Utility Macros
 * ============================================================================*/

/* Bit inversion for redundancy check */
#define INVERT_BITS_32(x)       (~(x))
#define INVERT_FLOAT(x)         (*(uint32_t*)&(x) = ~(*(uint32_t*)&(x)))

/* Check if value is inverted copy */
#define IS_INVERTED_32(val, inv) ((val) == INVERT_BITS_32(inv))

/* Range check macro */
#define IN_RANGE(val, min, max) (((val) >= (min)) && ((val) <= (max)))

#ifdef __cplusplus
}
#endif

#endif /* __SHARED_CONFIG_H */
