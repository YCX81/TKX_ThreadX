/**
 ******************************************************************************
 * @file    safety_monitor.c
 * @brief   Safety Monitor Thread Implementation
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "safety_monitor.h"
#include "safety_core.h"
#include "safety_selftest.h"
#include "safety_watchdog.h"
#include "safety_stack.h"
#include "safety_flow.h"
#include "safety_mpu.h"

/* Private defines -----------------------------------------------------------*/
#define MONITOR_THREAD_NAME     "Safety Monitor"

/* Private variables ---------------------------------------------------------*/
static TX_THREAD s_monitor_thread;
static UCHAR *s_monitor_stack = NULL;
static monitor_stats_t s_monitor_stats;
static uint32_t s_flash_crc_timer = 0;
static bool s_initialized = false;

/* ============================================================================
 * Implementation
 * ============================================================================*/

UINT Safety_Monitor_Init(TX_BYTE_POOL *byte_pool)
{
    UINT status;

    if (byte_pool == NULL)
    {
        return TX_PTR_ERROR;
    }

    /* Initialize statistics */
    s_monitor_stats.run_count = 0;
    s_monitor_stats.last_run_time = 0;
    s_monitor_stats.wdg_feeds = 0;
    s_monitor_stats.selftest_runs = 0;
    s_monitor_stats.stack_checks = 0;
    s_monitor_stats.flow_checks = 0;
    s_monitor_stats.errors_detected = 0;

    /* Allocate stack from byte pool */
    status = tx_byte_allocate(byte_pool,
                              (VOID **)&s_monitor_stack,
                              SAFETY_THREAD_STACK_SIZE,
                              TX_NO_WAIT);

    if (status != TX_SUCCESS)
    {
        return status;
    }

    /* Create safety monitor thread */
    status = tx_thread_create(&s_monitor_thread,
                              (CHAR *)MONITOR_THREAD_NAME,
                              Safety_Monitor_ThreadEntry,
                              0,
                              s_monitor_stack,
                              SAFETY_THREAD_STACK_SIZE,
                              SAFETY_THREAD_PRIORITY,
                              SAFETY_THREAD_PREEMPT_THRESH,
                              SAFETY_THREAD_TIME_SLICE,
                              TX_AUTO_START);

    if (status != TX_SUCCESS)
    {
        return status;
    }

    /* Register this thread for stack monitoring */
    Safety_Stack_RegisterThread(&s_monitor_thread);

    s_initialized = true;

    return TX_SUCCESS;
}

void Safety_Monitor_ThreadEntry(ULONG thread_input)
{
    (void)thread_input;

    /* Initialize safety modules */
    Safety_SelfTest_Init();
    Safety_Watchdog_Init();
    Safety_Stack_Init();
    Safety_Flow_Init();

    /* Start watchdog */
    Safety_Watchdog_Start();

    /* Initialize flow checkpoint */
    Safety_Flow_Checkpoint(PFM_CP_APP_INIT);

    /* Run startup self-test */
    selftest_result_t selftest_result = Safety_SelfTest_RunStartup();
    if (selftest_result != SELFTEST_PASS)
    {
        Safety_EnterSafeState(SAFETY_ERR_RUNTIME_TEST);
        /* Will not return if safe state is blocking */
    }

    /* Mark startup test passed */
    Safety_StartupTest();

    /* Transition to normal operation */
    Safety_PreKernelInit();

    /* Initialize flash CRC timer */
    s_flash_crc_timer = 0;

    /* Main monitoring loop */
    while (1)
    {
        /* Record checkpoint */
        Safety_Flow_Checkpoint(PFM_CP_APP_SAFETY_MONITOR);

        /* Update statistics */
        s_monitor_stats.run_count++;
        s_monitor_stats.last_run_time = tx_time_get();

        /* === 1. Report watchdog token === */
        Safety_Watchdog_ReportToken(WDG_TOKEN_SAFETY_THREAD);

        /* === 2. Process watchdog (check tokens and feed) === */
        Safety_Watchdog_Process();
        Safety_Flow_Checkpoint(PFM_CP_APP_WATCHDOG_FEED);
        s_monitor_stats.wdg_feeds++;

        /* === 3. Stack monitoring === */
        if ((s_monitor_stats.run_count % (STACK_CHECK_INTERVAL_MS / SAFETY_MONITOR_PERIOD_MS)) == 0)
        {
            safety_status_t stack_status = Safety_Stack_CheckAll();
            s_monitor_stats.stack_checks++;

            if (stack_status != SAFETY_OK)
            {
                s_monitor_stats.errors_detected++;
                /* Error already reported by Safety_Stack_CheckAll */
            }
        }

        /* === 4. Program flow verification === */
        if ((s_monitor_stats.run_count % (FLOW_VERIFY_INTERVAL_MS / SAFETY_MONITOR_PERIOD_MS)) == 0)
        {
            if (!Safety_Flow_Verify())
            {
                s_monitor_stats.errors_detected++;
                Safety_ReportError(SAFETY_ERR_FLOW_MONITOR,
                                 Safety_Flow_GetSignature(), 0);
            }
            s_monitor_stats.flow_checks++;

            /* Reset flow for next cycle */
            Safety_Flow_Reset();
            Safety_Flow_Checkpoint(PFM_CP_APP_SAFETY_MONITOR);
        }

        /* === 5. Incremental Flash CRC check === */
#if SELFTEST_RUNTIME_FLASH_ENABLED
        s_flash_crc_timer += SAFETY_MONITOR_PERIOD_MS;
        if (s_flash_crc_timer >= SELFTEST_FLASH_CRC_INTERVAL_MS)
        {
            Safety_Flow_Checkpoint(PFM_CP_APP_SELFTEST_START);

            /* Start new CRC check cycle */
            Safety_SelfTest_FlashCRC(SELFTEST_MODE_RUNTIME);

            /* Process one block per monitor cycle until complete */
            selftest_result_t crc_result;
            do {
                crc_result = Safety_SelfTest_FlashCRC_Continue();
            } while (crc_result == SELFTEST_IN_PROGRESS);

            if (crc_result == SELFTEST_FAIL_FLASH)
            {
                s_monitor_stats.errors_detected++;
                /* Error already reported */
            }

            s_monitor_stats.selftest_runs++;
            s_flash_crc_timer = 0;

            Safety_Flow_Checkpoint(PFM_CP_APP_SELFTEST_END);
        }
#endif

        /* === 6. Check degraded mode timeout === */
#if DEGRADED_MODE_ENABLED
        if (Safety_GetState() == SAFETY_STATE_DEGRADED)
        {
            const safety_context_t *ctx = Safety_GetContext();
            uint32_t elapsed = tx_time_get() - ctx->degraded_enter_time;

            if (elapsed > DEGRADED_MODE_TIMEOUT_MS)
            {
                /* Timeout in degraded mode - go to safe state */
                Safety_EnterSafeState(SAFETY_ERR_INTERNAL);
            }
        }
#endif

        /* Sleep until next period */
        tx_thread_sleep(SAFETY_MONITOR_PERIOD_MS);
    }
}

const monitor_stats_t* Safety_Monitor_GetStats(void)
{
    return &s_monitor_stats;
}

TX_THREAD* Safety_Monitor_GetThread(void)
{
    return s_initialized ? &s_monitor_thread : NULL;
}

void Safety_Monitor_Signal(void)
{
    if (s_initialized)
    {
        /* Resume thread immediately if suspended */
        tx_thread_resume(&s_monitor_thread);
    }
}
