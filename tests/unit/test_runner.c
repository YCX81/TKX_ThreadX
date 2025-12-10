/**
 ******************************************************************************
 * @file    test_runner.c
 * @brief   Main Test Runner for TKX_ThreadX Unit Tests
 *          TKX_ThreadX 单元测试主运行器
 ******************************************************************************
 */

#include "unity.h"

/* ============================================================================
 * External Test Declarations
 * ============================================================================*/

/* safety_flow tests */
extern void test_Flow_Init_ShouldResetSignature(void);
extern void test_Flow_Reset_ShouldClearState(void);
extern void test_Flow_Checkpoint_ShouldChangeSignature(void);
extern void test_Flow_DifferentCheckpoints_ShouldProduceDifferentSignatures(void);
extern void test_Flow_SameSequence_ShouldProduceSameSignature(void);
extern void test_Flow_DifferentOrder_ShouldProduceDifferentSignature(void);
extern void test_Flow_Verify_ShouldReturnTrueForExpectedSequence(void);
extern void test_Flow_MultipleSameCheckpoints_ShouldAllChangeSignature(void);
extern void test_Flow_ZeroCheckpoint_ShouldStillWork(void);
extern void test_Flow_MaxCheckpoint_ShouldWork(void);

/* ============================================================================
 * Setup / Teardown
 * ============================================================================*/

void setUp(void)
{
    /* Called before each test */
}

void tearDown(void)
{
    /* Called after each test */
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================*/

int main(void)
{
    UNITY_BEGIN();

    /* ========================================
     * Safety Flow Module Tests
     * ======================================== */
    RUN_TEST(test_Flow_Init_ShouldResetSignature);
    RUN_TEST(test_Flow_Reset_ShouldClearState);
    RUN_TEST(test_Flow_Checkpoint_ShouldChangeSignature);
    RUN_TEST(test_Flow_DifferentCheckpoints_ShouldProduceDifferentSignatures);
    RUN_TEST(test_Flow_SameSequence_ShouldProduceSameSignature);
    RUN_TEST(test_Flow_DifferentOrder_ShouldProduceDifferentSignature);
    RUN_TEST(test_Flow_Verify_ShouldReturnTrueForExpectedSequence);
    RUN_TEST(test_Flow_MultipleSameCheckpoints_ShouldAllChangeSignature);
    RUN_TEST(test_Flow_ZeroCheckpoint_ShouldStillWork);
    RUN_TEST(test_Flow_MaxCheckpoint_ShouldWork);

    /* ========================================
     * Add more test suites here as they are created:
     *
     * Safety Params Tests (future)
     * Safety Watchdog Tests (future)
     * Safety Selftest Tests (future)
     * ======================================== */

    return UNITY_END();
}
