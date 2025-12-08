/**
 ******************************************************************************
 * @file    safety_watchdog.c
 * @brief   Watchdog Management Implementation
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "safety_watchdog.h"
#include "safety_core.h"
#include "stm32f4xx_hal.h"
#include "iwdg.h"

#if WWDG_ENABLED
#include "wwdg.h"
#endif

#if DIAG_RTT_ENABLED
#include "bsp_debug.h"
#endif

/* Private variables ---------------------------------------------------------*/
static wdg_status_t s_wdg_status;
static uint32_t s_token_timestamp[8]; /* Timestamp per token bit */
static bool s_initialized = false;

/* ============================================================================
 * Implementation
 * ============================================================================*/

safety_status_t Safety_Watchdog_Init(void)
{
    /* Initialize status */
    s_wdg_status.last_feed_time = 0;
    s_wdg_status.feed_count = 0;
    s_wdg_status.tokens_received = 0;
    s_wdg_status.tokens_required = WDG_TOKEN_ALL;
    s_wdg_status.enabled = false;
    s_wdg_status.degraded_mode = false;

#if WWDG_ENABLED
    s_wdg_status.wwdg_feed_count = 0;
    s_wdg_status.wwdg_last_feed = 0;
    s_wdg_status.wwdg_enabled = false;
#endif

    /* Clear token timestamps */
    for (int i = 0; i < 8; i++)
    {
        s_token_timestamp[i] = 0;
    }

    s_initialized = true;

    return SAFETY_OK;
}

safety_status_t Safety_Watchdog_Start(void)
{
    if (!s_initialized)
    {
        return SAFETY_ERROR;
    }

    /* IWDG is initialized by CubeMX in MX_IWDG_Init() */
    /* Just mark as enabled */
    s_wdg_status.enabled = true;
    s_wdg_status.last_feed_time = HAL_GetTick();

    return SAFETY_OK;
}

void Safety_Watchdog_ReportToken(wdg_token_t token)
{
    if (!s_initialized)
    {
        return;
    }

    uint32_t current_time = HAL_GetTick();

    /* Record token */
    s_wdg_status.tokens_received |= token;

    /* Record timestamp for each bit */
    for (int i = 0; i < 8; i++)
    {
        if (token & (1 << i))
        {
            s_token_timestamp[i] = current_time;
        }
    }
}

bool Safety_Watchdog_CheckAllTokens(void)
{
    if (!s_initialized || s_wdg_status.degraded_mode)
    {
        return true; /* In degraded mode, always allow feeding */
    }

    uint32_t current_time = HAL_GetTick();

    /* Check if all required tokens received within timeout */
    for (int i = 0; i < 8; i++)
    {
        if (s_wdg_status.tokens_required & (1 << i))
        {
            /* This token is required */
            if (!(s_wdg_status.tokens_received & (1 << i)))
            {
                /* Token not received */
                return false;
            }

            /* Check token freshness */
            if ((current_time - s_token_timestamp[i]) > WDG_TOKEN_TIMEOUT_MS)
            {
                /* Token too old */
                return false;
            }
        }
    }

    return true;
}

void Safety_Watchdog_FeedIWDG(void)
{
    if (!s_wdg_status.enabled)
    {
        return;
    }

    /* Refresh IWDG */
    HAL_IWDG_Refresh(&hiwdg);

    /* Update status */
    s_wdg_status.last_feed_time = HAL_GetTick();
    s_wdg_status.feed_count++;

    /* Reset tokens for next cycle */
    s_wdg_status.tokens_received = 0;
}

void Safety_Watchdog_Process(void)
{
    if (!s_wdg_status.enabled)
    {
        return;
    }

    /* Check if it's time to feed */
    uint32_t current_time = HAL_GetTick();
    uint32_t elapsed = current_time - s_wdg_status.last_feed_time;

    if (elapsed >= WDG_FEED_PERIOD_MS)
    {
        if (s_wdg_status.degraded_mode)
        {
            /* In degraded mode, feed without token check */
            Safety_Watchdog_FeedIWDG();
        }
        else if (Safety_Watchdog_CheckAllTokens())
        {
            /* All tokens verified, safe to feed */
            Safety_Watchdog_FeedIWDG();
        }
        else
        {
            /* Tokens missing - report error but still feed in degraded */
            Safety_ReportError(SAFETY_ERR_WATCHDOG, s_wdg_status.tokens_received,
                             s_wdg_status.tokens_required);

            /* Enter degraded mode */
            Safety_Watchdog_EnterDegraded();
            Safety_Watchdog_FeedIWDG();
        }
    }
}

void Safety_Watchdog_EnterDegraded(void)
{
    s_wdg_status.degraded_mode = true;
}

void Safety_Watchdog_ExitDegraded(void)
{
    s_wdg_status.degraded_mode = false;
    s_wdg_status.tokens_received = 0;

    /* Reset timestamps */
    for (int i = 0; i < 8; i++)
    {
        s_token_timestamp[i] = 0;
    }
}

const wdg_status_t* Safety_Watchdog_GetStatus(void)
{
    return &s_wdg_status;
}

void Safety_Watchdog_SetRequiredTokens(uint8_t tokens_mask)
{
    s_wdg_status.tokens_required = tokens_mask;
}

void Safety_Watchdog_TickHandler(void)
{
    /* Can be used for additional timeout detection */
    /* Called from SysTick if needed */
}

/* ============================================================================
 * WWDG (Window Watchdog) Implementation
 * ============================================================================*/

#if WWDG_ENABLED

safety_status_t Safety_Watchdog_StartWWDG(void)
{
    if (!s_initialized)
    {
        return SAFETY_ERROR;
    }

    /* WWDG is initialized by CubeMX in MX_WWDG_Init() */
    /* Enable WWDG */
    s_wdg_status.wwdg_enabled = true;
    s_wdg_status.wwdg_last_feed = HAL_GetTick();
    s_wdg_status.wwdg_feed_count = 0;

#if DIAG_RTT_ENABLED
    DEBUG_INFO("WWDG: Started (dual watchdog active)");
#endif

    return SAFETY_OK;
}

void Safety_Watchdog_FeedWWDG(void)
{
    if (!s_wdg_status.wwdg_enabled)
    {
        return;
    }

    /* Refresh WWDG counter */
    HAL_WWDG_Refresh(&hwwdg);

    s_wdg_status.wwdg_last_feed = HAL_GetTick();
    s_wdg_status.wwdg_feed_count++;
}

void Safety_Watchdog_WWDG_IRQHandler(void)
{
    /*
     * WWDG Early Wakeup Interrupt
     * This is called when counter reaches 0x40 (before reset at 0x3F)
     * We have a small window to feed the watchdog or log error
     */

#if DIAG_RTT_ENABLED
    DEBUG_ERROR("WWDG: Early wakeup! Counter about to expire");
#endif

    /* Try to feed if possible, otherwise system will reset */
    if (s_wdg_status.wwdg_enabled && Safety_Watchdog_CheckAllTokens())
    {
        Safety_Watchdog_FeedWWDG();
    }
    else
    {
        /* Log error before reset */
        Safety_ReportError(SAFETY_ERR_WATCHDOG, 0xAADD0000UL, s_wdg_status.tokens_received);
    }
}

#endif /* WWDG_ENABLED */
