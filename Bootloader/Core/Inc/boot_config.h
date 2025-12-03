/**
 ******************************************************************************
 * @file    boot_config.h
 * @brief   Functional Safety Bootloader Configuration
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Functional Safety Configuration for SIL 2 / PL d
 * Target: STM32F407VGT6
 * Compliance: IEC 61508 / ISO 13849
 *
 ******************************************************************************
 */

#ifndef __BOOT_CONFIG_H
#define __BOOT_CONFIG_H

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

/* RAM Test Configuration */
#define RAM_TEST_START          0x20000000UL
#define RAM_TEST_SIZE           0x00001000UL    /* Test 4KB */

/* ============================================================================
 * Magic Numbers
 * ============================================================================*/
#define BOOT_CONFIG_MAGIC       0xC0F16000UL    /* "CONFIG" */
#define SAFETY_PARAMS_MAGIC     0xCAL1B000UL    /* "CALIB" - Note: will be hex */
#define SAFETY_PARAMS_MAGIC_VAL 0xCA11B000UL    /* Actual hex value */
#define NONSAFETY_PARAMS_MAGIC  0xEEPR0000UL
#define NONSAFETY_PARAMS_MAGIC_VAL 0xEE9A0000UL /* Actual hex value */

/* ============================================================================
 * Boot Status Codes
 * ============================================================================*/
typedef enum {
    BOOT_OK                     = 0x00U,
    BOOT_ERROR                  = 0x01U,
    BOOT_ERROR_CRC              = 0x02U,
    BOOT_ERROR_MAGIC            = 0x03U,
    BOOT_ERROR_REDUNDANCY       = 0x04U,
    BOOT_ERROR_RANGE            = 0x05U,
    BOOT_ERROR_SELFTEST         = 0x06U,
    BOOT_ERROR_TIMEOUT          = 0x07U,
    BOOT_ERROR_CLOCK            = 0x08U
} boot_status_t;

/* ============================================================================
 * Self-Test Result Codes
 * ============================================================================*/
typedef enum {
    SELFTEST_OK                 = 0x00U,
    SELFTEST_CPU_FAIL           = 0x01U,
    SELFTEST_RAM_FAIL           = 0x02U,
    SELFTEST_FLASH_FAIL         = 0x03U,
    SELFTEST_CLOCK_FAIL         = 0x04U,
    SELFTEST_WDG_FAIL           = 0x05U
} selftest_result_t;

typedef enum {
    TEST_PASS                   = 0U,
    TEST_FAIL                   = 1U
} test_result_t;

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

/* ============================================================================
 * Non-Safety Parameters Structure (stored in EEPROM or Flash)
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

/* Default values for non-safety parameters */
#define DEFAULT_CAN_BAUDRATE    500000U
#define DEFAULT_CAN_ID_BASE     0x100U
#define DEFAULT_COMM_TIMEOUT    1000U

/* ============================================================================
 * Program Flow Monitor Configuration
 * ============================================================================*/
#define PFM_SIGNATURE_INIT      0x5A5A5A5AUL
#define PFM_SIGNATURE_SELFTEST  0xA5A5A5A5UL
#define PFM_SIGNATURE_MAIN      0x12345678UL
#define PFM_SIGNATURE_JUMP      0x87654321UL

/* Program Flow Checkpoints */
typedef enum {
    PFM_CP_INIT             = 0x01U,
    PFM_CP_SELFTEST_START   = 0x02U,
    PFM_CP_SELFTEST_CPU     = 0x03U,
    PFM_CP_SELFTEST_RAM     = 0x04U,
    PFM_CP_SELFTEST_FLASH   = 0x05U,
    PFM_CP_SELFTEST_CLOCK   = 0x06U,
    PFM_CP_SELFTEST_END     = 0x07U,
    PFM_CP_PARAMS_CHECK     = 0x08U,
    PFM_CP_CONFIG_CHECK     = 0x09U,
    PFM_CP_FACTORY_MODE     = 0x0AU,
    PFM_CP_APP_VERIFY       = 0x0BU,
    PFM_CP_JUMP_PREPARE     = 0x0CU,
    PFM_CP_JUMP_EXECUTE     = 0x0DU
} pfm_checkpoint_t;

/* ============================================================================
 * Watchdog Configuration
 * ============================================================================*/
#define IWDG_TIMEOUT_MS         1000U   /* Independent Watchdog timeout */
#define WWDG_WINDOW_MS          50U     /* Window Watchdog window */
#define WWDG_COUNTER            127U    /* WWDG counter value */
#define WWDG_WINDOW             80U     /* WWDG window value */

/* ============================================================================
 * Timing Configuration
 * ============================================================================*/
#define BOOT_TIMEOUT_MS         5000U   /* Maximum boot time before watchdog */
#define SELFTEST_TIMEOUT_MS     2000U   /* Maximum self-test time */
#define FACTORY_MODE_TIMEOUT_MS 300000U /* Factory mode timeout (5 minutes) */

/* ============================================================================
 * Version Information
 * ============================================================================*/
#define BOOT_VERSION_MAJOR      1U
#define BOOT_VERSION_MINOR      0U
#define BOOT_VERSION_PATCH      0U
#define BOOT_VERSION            ((BOOT_VERSION_MAJOR << 16) | \
                                 (BOOT_VERSION_MINOR << 8) | \
                                 BOOT_VERSION_PATCH)

#ifdef __cplusplus
}
#endif

#endif /* __BOOT_CONFIG_H */
