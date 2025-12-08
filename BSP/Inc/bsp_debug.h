/**
 ******************************************************************************
 * @file    bsp_debug.h
 * @brief   Debug Interface using Segger RTT
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Debug output interface based on Segger RTT for real-time debugging
 * Target: STM32F407VGT6
 * Debugger: J-Link with RTT support
 *
 ******************************************************************************
 */

#ifndef __BSP_DEBUG_H
#define __BSP_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* ============================================================================
 * Debug Configuration
 * ============================================================================*/

/**
 * @brief Enable/disable debug output
 * @note Set to 0 to completely disable all debug output in release builds
 */
#ifndef DEBUG_ENABLED
#define DEBUG_ENABLED           1
#endif

/**
 * @brief Debug output levels
 */
#define DEBUG_LEVEL_NONE        0   /* No output */
#define DEBUG_LEVEL_ERROR       1   /* Errors only */
#define DEBUG_LEVEL_WARN        2   /* Errors and warnings */
#define DEBUG_LEVEL_INFO        3   /* Errors, warnings, and info */
#define DEBUG_LEVEL_VERBOSE     4   /* All output including verbose */

/**
 * @brief Current debug level
 */
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL             DEBUG_LEVEL_INFO
#endif

/* ============================================================================
 * RTT Include
 * ============================================================================*/

#if DEBUG_ENABLED
#include "SEGGER_RTT.h"
#endif

/* ============================================================================
 * Debug Output Macros
 * ============================================================================*/

#if DEBUG_ENABLED

/**
 * @brief Raw print without prefix
 */
#define DEBUG_PRINT(fmt, ...)   SEGGER_RTT_printf(0, fmt, ##__VA_ARGS__)

/**
 * @brief Error level output (red)
 */
#if (DEBUG_LEVEL >= DEBUG_LEVEL_ERROR)
#define DEBUG_ERROR(fmt, ...)   SEGGER_RTT_printf(0, \
    RTT_CTRL_TEXT_BRIGHT_RED "[ERR] " fmt RTT_CTRL_RESET "\r\n", ##__VA_ARGS__)
#else
#define DEBUG_ERROR(fmt, ...)   ((void)0)
#endif

/**
 * @brief Warning level output (yellow)
 */
#if (DEBUG_LEVEL >= DEBUG_LEVEL_WARN)
#define DEBUG_WARN(fmt, ...)    SEGGER_RTT_printf(0, \
    RTT_CTRL_TEXT_BRIGHT_YELLOW "[WRN] " fmt RTT_CTRL_RESET "\r\n", ##__VA_ARGS__)
#else
#define DEBUG_WARN(fmt, ...)    ((void)0)
#endif

/**
 * @brief Info level output (green)
 */
#if (DEBUG_LEVEL >= DEBUG_LEVEL_INFO)
#define DEBUG_INFO(fmt, ...)    SEGGER_RTT_printf(0, \
    RTT_CTRL_TEXT_BRIGHT_GREEN "[INF] " fmt RTT_CTRL_RESET "\r\n", ##__VA_ARGS__)
#else
#define DEBUG_INFO(fmt, ...)    ((void)0)
#endif

/**
 * @brief Verbose level output (cyan)
 */
#if (DEBUG_LEVEL >= DEBUG_LEVEL_VERBOSE)
#define DEBUG_VERBOSE(fmt, ...) SEGGER_RTT_printf(0, \
    RTT_CTRL_TEXT_BRIGHT_CYAN "[VRB] " fmt RTT_CTRL_RESET "\r\n", ##__VA_ARGS__)
#else
#define DEBUG_VERBOSE(fmt, ...) ((void)0)
#endif

/**
 * @brief Log with custom tag
 */
#define DEBUG_LOG(tag, fmt, ...)  SEGGER_RTT_printf(0, "[%s] " fmt "\r\n", tag, ##__VA_ARGS__)

/**
 * @brief Hex dump utility
 * @param data Pointer to data buffer
 * @param len Number of bytes to dump
 */
#define DEBUG_HEXDUMP(data, len)  do { \
    const uint8_t *_p = (const uint8_t *)(data); \
    SEGGER_RTT_printf(0, "HEX[%d]: ", (int)(len)); \
    for (uint32_t _i = 0; _i < (len); _i++) { \
        SEGGER_RTT_printf(0, "%02X ", _p[_i]); \
    } \
    SEGGER_RTT_printf(0, "\r\n"); \
} while(0)

#else /* DEBUG_ENABLED == 0 */

/* All debug macros are no-ops when disabled */
#define DEBUG_PRINT(fmt, ...)   ((void)0)
#define DEBUG_ERROR(fmt, ...)   ((void)0)
#define DEBUG_WARN(fmt, ...)    ((void)0)
#define DEBUG_INFO(fmt, ...)    ((void)0)
#define DEBUG_VERBOSE(fmt, ...) ((void)0)
#define DEBUG_LOG(tag, fmt, ...) ((void)0)
#define DEBUG_HEXDUMP(data, len) ((void)0)

#endif /* DEBUG_ENABLED */

/* ============================================================================
 * Debug Initialization
 * ============================================================================*/

/**
 * @brief Initialize debug interface
 * @note Call this function early in main() before using debug macros
 */
static inline void BSP_Debug_Init(void)
{
#if DEBUG_ENABLED
    SEGGER_RTT_Init();
    SEGGER_RTT_printf(0, "\r\n");
    SEGGER_RTT_printf(0, RTT_CTRL_TEXT_BRIGHT_WHITE "================================\r\n");
    SEGGER_RTT_printf(0, "  TKX_ThreadX Application\r\n");
    SEGGER_RTT_printf(0, "  Debug via Segger RTT\r\n");
    SEGGER_RTT_printf(0, "================================" RTT_CTRL_RESET "\r\n\r\n");
#endif
}

/* ============================================================================
 * Assertion Support
 * ============================================================================*/

/**
 * @brief Debug assertion macro
 * @param expr Expression to evaluate
 */
#if DEBUG_ENABLED
#define DEBUG_ASSERT(expr) do { \
    if (!(expr)) { \
        SEGGER_RTT_printf(0, RTT_CTRL_TEXT_BRIGHT_RED \
            "[ASSERT] %s:%d: %s\r\n" RTT_CTRL_RESET, \
            __FILE__, __LINE__, #expr); \
        while(1) { __NOP(); } \
    } \
} while(0)
#else
#define DEBUG_ASSERT(expr)  ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __BSP_DEBUG_H */
