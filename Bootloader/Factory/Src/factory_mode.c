/**
 ******************************************************************************
 * @file    factory_mode.c
 * @brief   Factory Mode Handler Implementation
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Factory mode implementation for safety parameter calibration.
 * Communication is done via debugger memory read/write, NOT UART/CAN.
 * This ensures safety parameters cannot be modified remotely.
 *
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "factory_mode.h"
#include "factory_calibration.h"
#include "storage_flash.h"
#include "boot_selftest.h"
#include "stm32f4xx_hal.h"
#include <string.h>

/* Private variables ---------------------------------------------------------*/
static factory_state_t factory_state = FACTORY_STATE_INIT;

/* Command/Response RAM locations */
static volatile uint32_t *const factory_cmd = (volatile uint32_t *)FACTORY_CMD_ADDR;
static volatile uint32_t *const factory_rsp = (volatile uint32_t *)FACTORY_RSP_ADDR;
static volatile uint8_t *const factory_data = (volatile uint8_t *)FACTORY_DATA_ADDR;

/* Private function prototypes -----------------------------------------------*/
static factory_status_t Factory_HandleReadCal(void);
static factory_status_t Factory_HandleWriteCal(void);
static factory_status_t Factory_HandleVerify(void);

/* ============================================================================
 * Initialization
 * ============================================================================*/

/**
 * @brief  Initialize factory mode
 */
factory_status_t Factory_Mode_Init(void)
{
    /* Check if debugger is connected - required for factory mode */
    if (!Factory_Mode_IsDebuggerConnected())
    {
        return FACTORY_NOT_AUTHORIZED;
    }

    /* Clear command/response area */
    *factory_cmd = FACTORY_CMD_NONE;
    *factory_rsp = FACTORY_RSP_READY;

    /* Initialize calibration module */
    if (Factory_Calibration_Init() != FACTORY_OK)
    {
        return FACTORY_ERROR;
    }

    factory_state = FACTORY_STATE_IDLE;

    return FACTORY_OK;
}

/* ============================================================================
 * Main Factory Mode Loop
 * ============================================================================*/

/**
 * @brief  Run factory mode main loop
 */
factory_status_t Factory_Mode_Run(void)
{
    factory_status_t status;

    /* Initialize */
    status = Factory_Mode_Init();
    if (status != FACTORY_OK)
    {
        return status;
    }

    /* Main loop - wait for debugger commands */
    while (1)
    {
        /* Refresh watchdog */
        Boot_Watchdog_Refresh();

        /* Check if debugger is still connected */
        if (!Factory_Mode_IsDebuggerConnected())
        {
            /* Debugger disconnected - check if calibration was completed */
            if (factory_state == FACTORY_STATE_COMPLETE)
            {
                return FACTORY_OK;
            }
            else
            {
                /* Abnormal disconnect - return to safe state */
                return FACTORY_NOT_AUTHORIZED;
            }
        }

        /* Process commands */
        status = Factory_Mode_ProcessCommand();
        if (status != FACTORY_OK && status != FACTORY_ERROR)
        {
            /* Exit command received or critical error */
            break;
        }

        /* Small delay */
        HAL_Delay(10);
    }

    return (factory_state == FACTORY_STATE_COMPLETE) ? FACTORY_OK : FACTORY_ERROR;
}

/* ============================================================================
 * Command Processing
 * ============================================================================*/

/**
 * @brief  Process factory mode command
 */
factory_status_t Factory_Mode_ProcessCommand(void)
{
    uint32_t cmd;
    factory_status_t status = FACTORY_OK;

    cmd = Factory_Mode_GetCommand();

    if (cmd == FACTORY_CMD_NONE)
    {
        return FACTORY_OK;  /* No command pending */
    }

    /* Set busy response */
    Factory_Mode_SetResponse(FACTORY_RSP_BUSY);

    switch (cmd)
    {
        case FACTORY_CMD_READ_CAL:
            status = Factory_HandleReadCal();
            factory_state = FACTORY_STATE_READ_CAL;
            break;

        case FACTORY_CMD_WRITE_CAL:
            status = Factory_HandleWriteCal();
            factory_state = FACTORY_STATE_WRITE_CAL;
            break;

        case FACTORY_CMD_VERIFY:
            status = Factory_HandleVerify();
            if (status == FACTORY_OK)
            {
                factory_state = FACTORY_STATE_COMPLETE;
            }
            else
            {
                factory_state = FACTORY_STATE_VERIFY;
            }
            break;

        case FACTORY_CMD_EXIT:
            if (factory_state == FACTORY_STATE_COMPLETE)
            {
                Factory_Mode_SetResponse(FACTORY_RSP_OK);
                Factory_Mode_ClearCommand();
                return FACTORY_OK;  /* Normal exit */
            }
            else
            {
                /* Exit without completing calibration */
                Factory_Mode_SetResponse(FACTORY_RSP_ERROR);
                Factory_Mode_ClearCommand();
                return FACTORY_CAL_INVALID;
            }

        case FACTORY_CMD_ABORT:
            Factory_Mode_SetResponse(FACTORY_RSP_OK);
            Factory_Mode_ClearCommand();
            factory_state = FACTORY_STATE_ERROR;
            return FACTORY_ERROR;  /* Abort requested */

        default:
            status = FACTORY_ERROR;
            break;
    }

    /* Set response based on status */
    if (status == FACTORY_OK)
    {
        Factory_Mode_SetResponse(FACTORY_RSP_OK);
    }
    else
    {
        Factory_Mode_SetResponse(FACTORY_RSP_ERROR);
    }

    /* Clear command after processing */
    Factory_Mode_ClearCommand();

    return status;
}

/* ============================================================================
 * Command Handlers
 * ============================================================================*/

/**
 * @brief  Handle read calibration command
 */
static factory_status_t Factory_HandleReadCal(void)
{
    safety_params_t params;
    storage_status_t status;

    /* Read current calibration data */
    status = Storage_ReadSafetyParams(&params);

    if (status == STORAGE_OK)
    {
        /* Copy params to shared data area for debugger to read */
        memcpy((void *)factory_data, &params, sizeof(safety_params_t));
        return FACTORY_OK;
    }
    else if (status == STORAGE_MAGIC_ERROR)
    {
        /* No calibration data - return empty structure */
        memset((void *)factory_data, 0, sizeof(safety_params_t));
        return FACTORY_OK;
    }

    return FACTORY_ERROR;
}

/**
 * @brief  Handle write calibration command
 */
static factory_status_t Factory_HandleWriteCal(void)
{
    safety_params_t params;
    storage_status_t status;

    /* Copy params from shared data area */
    memcpy(&params, (void *)factory_data, sizeof(safety_params_t));

    /* Validate params before writing */
    if (Factory_Calibration_Validate(&params) != FACTORY_OK)
    {
        return FACTORY_CAL_INVALID;
    }

    /* Prepare redundant fields */
    Factory_Calibration_PrepareRedundancy(&params);

    /* Write to Flash */
    status = Storage_WriteSafetyParams(&params);

    if (status != STORAGE_OK)
    {
        return FACTORY_WRITE_FAIL;
    }

    return FACTORY_OK;
}

/**
 * @brief  Handle verify calibration command
 */
static factory_status_t Factory_HandleVerify(void)
{
    storage_status_t status;

    /* Verify stored calibration data */
    status = Storage_CheckSafetyParamsExist();

    if (status != STORAGE_OK)
    {
        return FACTORY_VERIFY_FAIL;
    }

    /* Full verification of calibration data */
    safety_params_t params;
    status = Storage_ReadSafetyParams(&params);

    if (status != STORAGE_OK)
    {
        return FACTORY_VERIFY_FAIL;
    }

    /* Validate parameter ranges */
    if (Factory_Calibration_Validate(&params) != FACTORY_OK)
    {
        return FACTORY_CAL_INVALID;
    }

    return FACTORY_OK;
}

/* ============================================================================
 * Utility Functions
 * ============================================================================*/

/**
 * @brief  Get current factory mode state
 */
factory_state_t Factory_Mode_GetState(void)
{
    return factory_state;
}

/**
 * @brief  Check if debugger is connected
 */
uint32_t Factory_Mode_IsDebuggerConnected(void)
{
    /* Check CoreDebug DHCSR C_DEBUGEN bit */
    return (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) ? 1 : 0;
}

/**
 * @brief  Set response for debugger
 */
void Factory_Mode_SetResponse(uint32_t response)
{
    *factory_rsp = response;
    __DSB();  /* Ensure write is visible */
}

/**
 * @brief  Get command from debugger
 */
uint32_t Factory_Mode_GetCommand(void)
{
    return *factory_cmd;
}

/**
 * @brief  Clear command after processing
 */
void Factory_Mode_ClearCommand(void)
{
    *factory_cmd = FACTORY_CMD_NONE;
    __DSB();
}
