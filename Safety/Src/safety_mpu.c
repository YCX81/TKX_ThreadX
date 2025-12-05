/**
 ******************************************************************************
 * @file    safety_mpu.c
 * @brief   MPU Memory Protection Implementation
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Memory Protection Unit configuration implementation
 * Target: STM32F407VGT6 (Cortex-M4 with MPU)
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "safety_mpu.h"
#include "safety_core.h"
#include "stm32f4xx_hal.h"

/* Private defines -----------------------------------------------------------*/
#define MPU_CTRL_ENABLE         (1UL << 0)
#define MPU_CTRL_HFNMIENA       (1UL << 1)
#define MPU_CTRL_PRIVDEFENA     (1UL << 2)

#define MPU_RASR_ENABLE         (1UL << 0)
#define MPU_RASR_SIZE_SHIFT     1
#define MPU_RASR_SRD_SHIFT      8
#define MPU_RASR_B_SHIFT        16
#define MPU_RASR_C_SHIFT        17
#define MPU_RASR_S_SHIFT        18
#define MPU_RASR_TEX_SHIFT      19
#define MPU_RASR_AP_SHIFT       24
#define MPU_RASR_XN_SHIFT       28

/* Maximum number of MPU regions for Cortex-M4 */
#define MPU_MAX_REGIONS         8

/* ============================================================================
 * Default Region Configurations
 * ============================================================================*/

static const mpu_region_config_t s_default_regions[] = {
    /* Region 0: Application Flash (448KB, RO+Execute) */
    {
        .base_address = APP_FLASH_START,
        .region_number = MPU_REGION_FLASH,
        .size = MPU_REGION_SIZE_512KB,      /* Closest size >= 448KB */
        .access_permission = MPU_AP_RO,
        .execute_never = MPU_XN_DISABLE,
        .shareable = 0,
        .cacheable = 1,
        .bufferable = 0,
        .tex = MPU_TEX_NORMAL_WTNA,
        .subregion_disable = 0x80,          /* Disable last 64KB subregion */
        .enable = 1
    },

    /* Region 1: Main RAM (128KB, RW, No Execute) */
    {
        .base_address = RAM_START,
        .region_number = MPU_REGION_RAM,
        .size = MPU_REGION_SIZE_128KB,
        .access_permission = MPU_AP_FULL_ACCESS,
        .execute_never = MPU_XN_ENABLE,
        .shareable = 1,
        .cacheable = 1,
        .bufferable = 1,
        .tex = MPU_TEX_NORMAL_WBWA,
        .subregion_disable = 0,
        .enable = 1
    },

    /* Region 2: CCM RAM (64KB, RW, No Execute) - Used for stacks */
    {
        .base_address = CCMRAM_START,
        .region_number = MPU_REGION_CCM,
        .size = MPU_REGION_SIZE_64KB,
        .access_permission = MPU_AP_FULL_ACCESS,
        .execute_never = MPU_XN_ENABLE,
        .shareable = 0,
        .cacheable = 0,
        .bufferable = 0,
        .tex = MPU_TEX_STRONGLY_ORDERED,
        .subregion_disable = 0,
        .enable = 1
    },

    /* Region 3: Peripheral Region (512MB, RW, No Execute, Device) */
    {
        .base_address = PERIPH_BASE_ADDR,
        .region_number = MPU_REGION_PERIPH,
        .size = MPU_REGION_SIZE_512MB,
        .access_permission = MPU_AP_FULL_ACCESS,
        .execute_never = MPU_XN_ENABLE,
        .shareable = 1,
        .cacheable = 0,
        .bufferable = 1,
        .tex = MPU_TEX_DEVICE,
        .subregion_disable = 0,
        .enable = 1
    },

    /* Region 4: Config Flash (16KB, RO, No Execute) */
    {
        .base_address = CONFIG_FLASH_START,
        .region_number = MPU_REGION_CONFIG,
        .size = MPU_REGION_SIZE_16KB,
        .access_permission = MPU_AP_RO,
        .execute_never = MPU_XN_ENABLE,
        .shareable = 0,
        .cacheable = 1,
        .bufferable = 0,
        .tex = MPU_TEX_NORMAL_WTNA,
        .subregion_disable = 0,
        .enable = 1
    },

    /* Region 5: Bootloader (48KB, No Access - prevent corruption) */
    {
        .base_address = BOOT_FLASH_START,
        .region_number = MPU_REGION_BOOT,
        .size = MPU_REGION_SIZE_64KB,       /* Closest size >= 48KB */
        .access_permission = MPU_AP_PRIV_RO, /* Read-only from privileged */
        .execute_never = MPU_XN_ENABLE,
        .shareable = 0,
        .cacheable = 1,
        .bufferable = 0,
        .tex = MPU_TEX_NORMAL_WTNA,
        .subregion_disable = 0xC0,          /* Disable upper subregions */
        .enable = 1
    }
};

#define NUM_DEFAULT_REGIONS (sizeof(s_default_regions) / sizeof(s_default_regions[0]))

/* ============================================================================
 * Implementation
 * ============================================================================*/

safety_status_t Safety_MPU_Init(void)
{
    /* Check if MPU is present */
    if ((MPU->TYPE & MPU_TYPE_DREGION_Msk) == 0)
    {
        /* No MPU present */
        return SAFETY_ERROR;
    }

    /* Disable MPU before configuration */
    Safety_MPU_Disable();

    /* Configure all default regions */
    for (uint32_t i = 0; i < NUM_DEFAULT_REGIONS; i++)
    {
        safety_status_t status = Safety_MPU_ConfigRegion(&s_default_regions[i]);
        if (status != SAFETY_OK)
        {
            return status;
        }
    }

    /* Enable MPU */
    return Safety_MPU_Enable();
}

safety_status_t Safety_MPU_ConfigRegion(const mpu_region_config_t *config)
{
    if (config == NULL)
    {
        return SAFETY_INVALID_PARAM;
    }

    if (config->region_number >= MPU_MAX_REGIONS)
    {
        return SAFETY_INVALID_PARAM;
    }

    /* Check base address alignment (must be aligned to region size) */
    uint32_t region_size_bytes = 1UL << (config->size + 1);
    if ((config->base_address & (region_size_bytes - 1)) != 0)
    {
        return SAFETY_INVALID_PARAM;
    }

    /* Disable interrupts during MPU configuration */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    /* Select region */
    MPU->RNR = config->region_number;

    /* Configure base address */
    MPU->RBAR = config->base_address & MPU_RBAR_ADDR_Msk;

    /* Configure region attributes */
    uint32_t rasr = 0;

    if (config->enable)
    {
        rasr |= MPU_RASR_ENABLE;
    }

    rasr |= ((uint32_t)config->size << MPU_RASR_SIZE_SHIFT);
    rasr |= ((uint32_t)config->subregion_disable << MPU_RASR_SRD_SHIFT);
    rasr |= ((uint32_t)config->bufferable << MPU_RASR_B_SHIFT);
    rasr |= ((uint32_t)config->cacheable << MPU_RASR_C_SHIFT);
    rasr |= ((uint32_t)config->shareable << MPU_RASR_S_SHIFT);
    rasr |= ((uint32_t)config->tex << MPU_RASR_TEX_SHIFT);
    rasr |= ((uint32_t)config->access_permission << MPU_RASR_AP_SHIFT);
    rasr |= ((uint32_t)config->execute_never << MPU_RASR_XN_SHIFT);

    MPU->RASR = rasr;

    /* Restore interrupts */
    __set_PRIMASK(primask);

    /* Memory barrier */
    __DSB();
    __ISB();

    return SAFETY_OK;
}

safety_status_t Safety_MPU_Enable(void)
{
    /* Enable MPU with:
     * - PRIVDEFENA: Enable default memory map for privileged access
     * - HFNMIENA: Enable MPU during hard fault and NMI (optional)
     */
    uint32_t ctrl = MPU_CTRL_ENABLE | MPU_CTRL_PRIVDEFENA;

    /* Disable interrupts during enable */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    MPU->CTRL = ctrl;

    /* Memory barrier */
    __DSB();
    __ISB();

    /* Restore interrupts */
    __set_PRIMASK(primask);

    /* Enable MemManage fault */
    SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;

    return SAFETY_OK;
}

safety_status_t Safety_MPU_Disable(void)
{
    /* Disable interrupts during disable */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    /* Disable MPU */
    MPU->CTRL = 0;

    /* Memory barrier */
    __DSB();
    __ISB();

    /* Restore interrupts */
    __set_PRIMASK(primask);

    return SAFETY_OK;
}

bool Safety_MPU_IsEnabled(void)
{
    return ((MPU->CTRL & MPU_CTRL_ENABLE) != 0);
}

safety_status_t Safety_MPU_GetRegion(uint8_t region_number,
                                     mpu_region_config_t *config)
{
    if (config == NULL || region_number >= MPU_MAX_REGIONS)
    {
        return SAFETY_INVALID_PARAM;
    }

    /* Select region */
    MPU->RNR = region_number;

    /* Read configuration */
    config->region_number = region_number;
    config->base_address = MPU->RBAR & MPU_RBAR_ADDR_Msk;

    uint32_t rasr = MPU->RASR;

    config->enable = (rasr & MPU_RASR_ENABLE) ? 1 : 0;
    config->size = (rasr >> MPU_RASR_SIZE_SHIFT) & 0x1F;
    config->subregion_disable = (rasr >> MPU_RASR_SRD_SHIFT) & 0xFF;
    config->bufferable = (rasr >> MPU_RASR_B_SHIFT) & 0x01;
    config->cacheable = (rasr >> MPU_RASR_C_SHIFT) & 0x01;
    config->shareable = (rasr >> MPU_RASR_S_SHIFT) & 0x01;
    config->tex = (rasr >> MPU_RASR_TEX_SHIFT) & 0x07;
    config->access_permission = (rasr >> MPU_RASR_AP_SHIFT) & 0x07;
    config->execute_never = (rasr >> MPU_RASR_XN_SHIFT) & 0x01;

    return SAFETY_OK;
}

safety_status_t Safety_MPU_DisableRegion(uint8_t region_number)
{
    if (region_number >= MPU_MAX_REGIONS)
    {
        return SAFETY_INVALID_PARAM;
    }

    /* Disable interrupts */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    /* Select and disable region */
    MPU->RNR = region_number;
    MPU->RASR = 0;

    /* Memory barrier */
    __DSB();
    __ISB();

    /* Restore interrupts */
    __set_PRIMASK(primask);

    return SAFETY_OK;
}

safety_status_t Safety_MPU_GetInfo(uint8_t *num_regions)
{
    if (num_regions == NULL)
    {
        return SAFETY_INVALID_PARAM;
    }

    *num_regions = (MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;

    return SAFETY_OK;
}
