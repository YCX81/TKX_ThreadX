/**
 ******************************************************************************
 * @file    app_main.c
 * @brief   Application Main Implementation
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "app_main.h"
#include "safety_core.h"
#include "safety_monitor.h"
#include "safety_watchdog.h"
#include "safety_stack.h"
#include "safety_flow.h"
#include "svc_params.h"

/* Private defines -----------------------------------------------------------*/
#define MAIN_THREAD_NAME    "App Main"
#define COMM_THREAD_NAME    "App Comm"

/* Private variables ---------------------------------------------------------*/
static TX_THREAD s_main_thread;
static TX_THREAD s_comm_thread;
static UCHAR *s_main_stack = NULL;
static UCHAR *s_comm_stack = NULL;

/* ============================================================================
 * Implementation
 * ============================================================================*/

shared_status_t App_PreInit(void)
{
    shared_status_t status;

    /* Initialize parameter service */
    status = Svc_Params_Init();
    if (status != STATUS_OK)
    {
        /* Parameters invalid - report error but continue */
        /* Safety module will handle this appropriately */
        Safety_ReportError(SAFETY_ERR_PARAM_INVALID, status, 0);
    }

    return STATUS_OK;
}

UINT App_CreateThreads(TX_BYTE_POOL *byte_pool)
{
    UINT status;

    if (byte_pool == NULL)
    {
        return TX_PTR_ERROR;
    }

    /* === Create Safety Monitor Thread First === */
    status = Safety_Monitor_Init(byte_pool);
    if (status != TX_SUCCESS)
    {
        return status;
    }

    /* === Allocate Main Thread Stack === */
    status = tx_byte_allocate(byte_pool,
                              (VOID **)&s_main_stack,
                              APP_MAIN_THREAD_STACK_SIZE,
                              TX_NO_WAIT);
    if (status != TX_SUCCESS)
    {
        return status;
    }

    /* === Create Main Thread === */
    status = tx_thread_create(&s_main_thread,
                              (CHAR *)MAIN_THREAD_NAME,
                              App_MainThreadEntry,
                              0,
                              s_main_stack,
                              APP_MAIN_THREAD_STACK_SIZE,
                              APP_MAIN_THREAD_PRIORITY,
                              APP_MAIN_THREAD_PREEMPT_THRESH,
                              TX_NO_TIME_SLICE,
                              TX_AUTO_START);
    if (status != TX_SUCCESS)
    {
        return status;
    }

    /* Register for stack monitoring */
    Safety_Stack_RegisterThread(&s_main_thread);

    /* === Allocate Comm Thread Stack === */
    status = tx_byte_allocate(byte_pool,
                              (VOID **)&s_comm_stack,
                              APP_COMM_THREAD_STACK_SIZE,
                              TX_NO_WAIT);
    if (status != TX_SUCCESS)
    {
        return status;
    }

    /* === Create Comm Thread === */
    status = tx_thread_create(&s_comm_thread,
                              (CHAR *)COMM_THREAD_NAME,
                              App_CommThreadEntry,
                              0,
                              s_comm_stack,
                              APP_COMM_THREAD_STACK_SIZE,
                              APP_COMM_THREAD_PRIORITY,
                              APP_COMM_THREAD_PREEMPT_THRESH,
                              TX_NO_TIME_SLICE,
                              TX_AUTO_START);
    if (status != TX_SUCCESS)
    {
        return status;
    }

    /* Register for stack monitoring */
    Safety_Stack_RegisterThread(&s_comm_thread);

    return TX_SUCCESS;
}

void App_MainThreadEntry(ULONG thread_input)
{
    (void)thread_input;

    /* Wait for safety system to be ready */
    while (!Safety_IsOperational())
    {
        tx_thread_sleep(10);
    }

    /* Main application loop */
    while (1)
    {
        /* Check safety state */
        safety_state_t state = Safety_GetState();

        if (state == SAFETY_STATE_NORMAL)
        {
            /* Normal operation */
            /* TODO: Add application logic here */

            /* Record flow checkpoint */
            Safety_Flow_Checkpoint(PFM_CP_APP_MAIN_LOOP);

            /* Report watchdog token */
            Safety_Watchdog_ReportToken(WDG_TOKEN_MAIN_THREAD);
        }
        else if (state == SAFETY_STATE_DEGRADED)
        {
            /* Degraded operation - limited functionality */
            /* TODO: Implement degraded mode behavior */

            /* Still report watchdog token */
            Safety_Watchdog_ReportToken(WDG_TOKEN_MAIN_THREAD);
        }
        else
        {
            /* Safe state or error - do nothing */
        }

        /* Thread sleep */
        tx_thread_sleep(10);
    }
}

void App_CommThreadEntry(ULONG thread_input)
{
    (void)thread_input;

    /* Wait for safety system to be ready */
    while (!Safety_IsOperational())
    {
        tx_thread_sleep(10);
    }

    /* Communication loop */
    while (1)
    {
        /* Check safety state */
        safety_state_t state = Safety_GetState();

        if (state == SAFETY_STATE_NORMAL || state == SAFETY_STATE_DEGRADED)
        {
            /* TODO: Handle communication */

            /* Record flow checkpoint */
            Safety_Flow_Checkpoint(PFM_CP_APP_COMM_HANDLER);

            /* Report watchdog token */
            Safety_Watchdog_ReportToken(WDG_TOKEN_COMM_THREAD);
        }

        /* Thread sleep - event driven in real implementation */
        tx_thread_sleep(100);
    }
}

TX_THREAD* App_GetMainThread(void)
{
    return &s_main_thread;
}

TX_THREAD* App_GetCommThread(void)
{
    return &s_comm_thread;
}
