/**
 ******************************************************************************
 * @file    safety_monitor.h
 * @brief   Safety Monitor Thread Interface
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * High-priority safety monitoring thread for runtime protection
 * Target: STM32F407VGT6
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

#ifndef __SAFETY_MONITOR_H
#define __SAFETY_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "safety_config.h"
#include "tx_api.h"

/* ============================================================================
 * Monitor Statistics
 * ============================================================================*/

typedef struct {
    uint32_t run_count;             /* Number of monitor cycles */
    uint32_t last_run_time;         /* Last run timestamp */
    uint32_t wdg_feeds;             /* Watchdog feed count */
    uint32_t selftest_runs;         /* Runtime selftest runs */
    uint32_t stack_checks;          /* Stack check runs */
    uint32_t flow_checks;           /* Flow verification runs */
    uint32_t errors_detected;       /* Errors detected */
} monitor_stats_t;

/* ============================================================================
 * Function Prototypes
 * ============================================================================*/

/**
 * @brief Initialize safety monitor (creates thread)
 * @param byte_pool ThreadX byte pool for stack allocation
 * @retval UINT ThreadX status
 */
UINT Safety_Monitor_Init(TX_BYTE_POOL *byte_pool);

/**
 * @brief Safety monitor thread entry function
 * @param thread_input Thread input parameter (unused)
 */
void Safety_Monitor_ThreadEntry(ULONG thread_input);

/**
 * @brief Get monitor statistics
 * @retval const monitor_stats_t* Statistics pointer
 */
const monitor_stats_t* Safety_Monitor_GetStats(void);

/**
 * @brief Get safety thread handle
 * @retval TX_THREAD* Thread pointer
 */
TX_THREAD* Safety_Monitor_GetThread(void);

/**
 * @brief Signal monitor to run immediately
 * @note For emergency checks
 */
void Safety_Monitor_Signal(void);

#ifdef __cplusplus
}
#endif

#endif /* __SAFETY_MONITOR_H */
