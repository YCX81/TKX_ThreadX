/**
 ******************************************************************************
 * @file    safety_watchdog.h
 * @brief   Watchdog Management Interface
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Token-based watchdog management for multi-threaded safety
 * Target: STM32F407VGT6
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

#ifndef __SAFETY_WATCHDOG_H
#define __SAFETY_WATCHDOG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "safety_config.h"

/* ============================================================================
 * Watchdog Token Definitions
 * ============================================================================*/

typedef uint8_t wdg_token_t;

/* Token values defined in safety_config.h:
 * WDG_TOKEN_SAFETY_THREAD  0x01
 * WDG_TOKEN_MAIN_THREAD    0x02
 * WDG_TOKEN_COMM_THREAD    0x04
 * WDG_TOKEN_ALL            0x07
 */

/* ============================================================================
 * Dual Watchdog Configuration
 * ============================================================================*/

/* Enable/disable WWDG (Window Watchdog) for dual-channel safety */
#ifndef WWDG_ENABLED
#define WWDG_ENABLED            1   /* WWDG enabled for dual-channel watchdog */
#endif

/* WWDG timing parameters (based on 42MHz PCLK1) */
#define WWDG_PRESCALER          8U          /* WWDG_CFR prescaler */
#define WWDG_WINDOW             0x50U       /* Window value (80) */
#define WWDG_COUNTER            0x7FU       /* Counter value (127) */

/* ============================================================================
 * Watchdog Status
 * ============================================================================*/

typedef struct {
    uint32_t last_feed_time;        /* Last IWDG feed time */
    uint32_t feed_count;            /* Total feed count */
    uint8_t  tokens_received;       /* Tokens received this cycle */
    uint8_t  tokens_required;       /* Required tokens mask */
    bool     enabled;               /* Watchdog enabled flag */
    bool     degraded_mode;         /* In degraded mode */
#if WWDG_ENABLED
    uint32_t wwdg_feed_count;       /* WWDG feed count */
    uint32_t wwdg_last_feed;        /* Last WWDG feed time */
    bool     wwdg_enabled;          /* WWDG enabled flag */
#endif
} wdg_status_t;

/* ============================================================================
 * Function Prototypes
 * ============================================================================*/

/**
 * @brief Initialize watchdog management
 * @retval safety_status_t Status
 */
safety_status_t Safety_Watchdog_Init(void);

/**
 * @brief Start watchdog operation
 * @note IWDG cannot be stopped once started
 * @retval safety_status_t Status
 */
safety_status_t Safety_Watchdog_Start(void);

/**
 * @brief Report thread alive token
 * @param token Thread token to report
 */
void Safety_Watchdog_ReportToken(wdg_token_t token);

/**
 * @brief Check if all required tokens received
 * @retval bool true if all tokens received
 */
bool Safety_Watchdog_CheckAllTokens(void);

/**
 * @brief Feed the IWDG (internal use)
 * @note Only called when all tokens verified
 */
void Safety_Watchdog_FeedIWDG(void);

/**
 * @brief Process watchdog in safety thread
 * @note Called every SAFETY_MONITOR_PERIOD_MS
 */
void Safety_Watchdog_Process(void);

/**
 * @brief Enter degraded watchdog mode
 * @note Feeds watchdog without token verification
 */
void Safety_Watchdog_EnterDegraded(void);

/**
 * @brief Exit degraded watchdog mode
 */
void Safety_Watchdog_ExitDegraded(void);

/**
 * @brief Get watchdog status
 * @retval const wdg_status_t* Status pointer
 */
const wdg_status_t* Safety_Watchdog_GetStatus(void);

/**
 * @brief Set required tokens mask
 * @param tokens_mask Required tokens mask
 */
void Safety_Watchdog_SetRequiredTokens(uint8_t tokens_mask);

/**
 * @brief Tick handler (called from SysTick)
 * @note Can be used for timeout detection
 */
void Safety_Watchdog_TickHandler(void);

#if WWDG_ENABLED
/**
 * @brief Initialize and start WWDG
 * @note WWDG cannot be stopped once started
 * @retval safety_status_t Status
 */
safety_status_t Safety_Watchdog_StartWWDG(void);

/**
 * @brief Feed the WWDG
 * @note Must be called within the window period
 */
void Safety_Watchdog_FeedWWDG(void);

/**
 * @brief WWDG early wakeup interrupt handler
 * @note Called when WWDG counter reaches 0x40
 */
void Safety_Watchdog_WWDG_IRQHandler(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __SAFETY_WATCHDOG_H */
