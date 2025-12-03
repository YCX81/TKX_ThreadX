/**
 ******************************************************************************
 * @file    boot_selftest.h
 * @brief   Functional Safety Startup Self-Test
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Startup self-test routines for functional safety compliance
 * Tests: CPU registers, RAM, Flash CRC, Clock system
 *
 * Note: This is a simplified implementation. For production use,
 *       consider using ST X-CUBE-STL (Self-Test Library) which is
 *       certified for IEC 61508 SIL 2/3.
 *
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

#ifndef __BOOT_SELFTEST_H
#define __BOOT_SELFTEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "boot_config.h"

/* ============================================================================
 * Self-Test Configuration
 * ============================================================================*/

/* Test patterns for register and RAM tests */
#define TEST_PATTERN_1          0x55555555UL
#define TEST_PATTERN_2          0xAAAAAAAAUL
#define TEST_PATTERN_3          0x00000000UL
#define TEST_PATTERN_4          0xFFFFFFFFUL

/* Clock frequency tolerance (percentage) */
#define CLOCK_TOLERANCE_PERCENT 5U

/* Expected system clock frequency */
#define EXPECTED_SYSCLK_HZ      168000000UL  /* 168 MHz */

/* ============================================================================
 * Function Prototypes
 * ============================================================================*/

/**
 * @brief  Run all startup self-tests
 * @retval SELFTEST_OK if all tests pass, error code otherwise
 */
selftest_result_t Boot_SelfTest(void);

/**
 * @brief  CPU register test
 * @note   Tests R0-R12, MSP, PSP, LR, CONTROL, PRIMASK
 * @retval TEST_PASS or TEST_FAIL
 */
test_result_t Boot_CPU_Test(void);

/**
 * @brief  RAM March C test
 * @note   Non-destructive test - saves and restores data
 * @retval TEST_PASS or TEST_FAIL
 */
test_result_t Boot_RAM_Test(void);

/**
 * @brief  Flash CRC test (Bootloader integrity)
 * @retval TEST_PASS or TEST_FAIL
 */
test_result_t Boot_Flash_Test(void);

/**
 * @brief  Clock system test
 * @note   Verifies HSE/PLL are running at expected frequency
 * @retval TEST_PASS or TEST_FAIL
 */
test_result_t Boot_Clock_Test(void);

/**
 * @brief  Initialize watchdog (IWDG)
 * @retval TEST_PASS or TEST_FAIL
 */
test_result_t Boot_Watchdog_Init(void);

/**
 * @brief  Refresh watchdog
 */
void Boot_Watchdog_Refresh(void);

#ifdef __cplusplus
}
#endif

#endif /* __BOOT_SELFTEST_H */
