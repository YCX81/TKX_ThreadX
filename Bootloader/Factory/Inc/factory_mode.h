/**
 ******************************************************************************
 * @file    factory_mode.h
 * @brief   Factory Mode Handler Header
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Factory mode for safety parameter calibration.
 * Entry: Debugger connection + specific trigger (NOT via communication)
 * Exit: Calibration complete + validation pass + debugger disconnect + reset
 *
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

#ifndef __FACTORY_MODE_H
#define __FACTORY_MODE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "boot_config.h"

/* ============================================================================
 * Factory Mode States
 * ============================================================================*/

typedef enum {
    FACTORY_STATE_INIT       = 0x00,
    FACTORY_STATE_IDLE       = 0x01,
    FACTORY_STATE_READ_CAL   = 0x02,
    FACTORY_STATE_WRITE_CAL  = 0x03,
    FACTORY_STATE_VERIFY     = 0x04,
    FACTORY_STATE_COMPLETE   = 0x05,
    FACTORY_STATE_ERROR      = 0xFF
} factory_state_t;

/* ============================================================================
 * Factory Mode Status Codes
 * ============================================================================*/

typedef enum {
    FACTORY_OK                = 0x00,
    FACTORY_ERROR             = 0x01,
    FACTORY_CAL_INVALID       = 0x02,
    FACTORY_WRITE_FAIL        = 0x03,
    FACTORY_VERIFY_FAIL       = 0x04,
    FACTORY_TIMEOUT           = 0x05,
    FACTORY_NOT_AUTHORIZED    = 0x06
} factory_status_t;

/* ============================================================================
 * Factory Mode Commands (via debugger memory write)
 * ============================================================================*/

/* Commands are written to a specific RAM location by debugger */
#define FACTORY_CMD_NONE          0x00000000UL
#define FACTORY_CMD_READ_CAL      0xCAL1READ
#define FACTORY_CMD_WRITE_CAL     0xCAL1WRIT
#define FACTORY_CMD_VERIFY        0xCAL1VRFY
#define FACTORY_CMD_EXIT          0xCAL1EXIT
#define FACTORY_CMD_ABORT         0xCAL1ABRT

/* Command/Response RAM locations (CCM RAM) */
#define FACTORY_CMD_ADDR          0x10000000UL
#define FACTORY_RSP_ADDR          0x10000004UL
#define FACTORY_DATA_ADDR         0x10000008UL

/* Response codes */
#define FACTORY_RSP_READY         0x52454459UL  /* "REDY" */
#define FACTORY_RSP_BUSY          0x42555359UL  /* "BUSY" */
#define FACTORY_RSP_OK            0x4F4B4F4BUL  /* "OKOK" */
#define FACTORY_RSP_ERROR         0x4552524FUL  /* "ERRO" */

/* ============================================================================
 * Function Prototypes
 * ============================================================================*/

/**
 * @brief  Initialize factory mode
 * @retval FACTORY_OK on success
 */
factory_status_t Factory_Mode_Init(void);

/**
 * @brief  Run factory mode main loop
 * @note   This function blocks until calibration is complete or aborted
 * @retval FACTORY_OK if calibration successful
 */
factory_status_t Factory_Mode_Run(void);

/**
 * @brief  Get current factory mode state
 * @retval Current state
 */
factory_state_t Factory_Mode_GetState(void);

/**
 * @brief  Process factory mode command
 * @note   Commands are received via debugger writing to RAM
 * @retval FACTORY_OK if command processed
 */
factory_status_t Factory_Mode_ProcessCommand(void);

/**
 * @brief  Check if debugger is connected
 * @note   Uses CoreDebug DHCSR register
 * @retval 1 if debugger connected, 0 otherwise
 */
uint32_t Factory_Mode_IsDebuggerConnected(void);

/**
 * @brief  Set response for debugger
 * @param  response: Response code to set
 */
void Factory_Mode_SetResponse(uint32_t response);

/**
 * @brief  Get command from debugger
 * @retval Command code from RAM location
 */
uint32_t Factory_Mode_GetCommand(void);

/**
 * @brief  Clear command after processing
 */
void Factory_Mode_ClearCommand(void);

#ifdef __cplusplus
}
#endif

#endif /* __FACTORY_MODE_H */
