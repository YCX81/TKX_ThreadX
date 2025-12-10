# =============================================================================
# CMakeLists.txt - TKX_ThreadX 单元测试配置 / Unit Test Configuration
# =============================================================================
# 使用方法 / Usage:
#   mkdir build && cd build
#   cmake .. -G "MinGW Makefiles"
#   cmake --build .
#   ctest --output-on-failure
# =============================================================================

cmake_minimum_required(VERSION 3.16)
project(TKX_ThreadX_Tests C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 测试定义 / Test definitions
add_definitions(-DUNIT_TEST)
add_definitions(-DTEST_HOST)

# 项目根目录 / Project root
set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/..)

# =============================================================================
# Unity 测试框架 / Unity Test Framework
# =============================================================================
add_library(unity STATIC
    unity/unity.c
)
target_include_directories(unity PUBLIC unity)

# =============================================================================
# Mock 库 / Mock Library
# =============================================================================
add_library(mocks STATIC
    mocks/mock_hal.c
)
target_include_directories(mocks PUBLIC
    mocks
    ${PROJECT_ROOT}/Shared/Inc
    ${PROJECT_ROOT}/Safety/Inc
)

# =============================================================================
# 源文件 / Source Files (testable modules)
# =============================================================================
set(SAFETY_SRC_DIR ${PROJECT_ROOT}/Safety/Src)
set(SAFETY_INC_DIR ${PROJECT_ROOT}/Safety/Inc)
set(SHARED_INC_DIR ${PROJECT_ROOT}/Shared/Inc)

# 可测试的源文件（无硬件依赖）
# Testable source files (no hardware dependencies)
set(TESTABLE_SOURCES
    ${SAFETY_SRC_DIR}/safety_flow.c
    # safety_params.c 需要 CRC 硬件，暂不包含
    # Add more as mocks are created
)

# =============================================================================
# 测试可执行文件 / Test Executables
# =============================================================================

# Test: safety_flow
add_executable(test_safety_flow
    unit/test_safety_flow.c
    ${SAFETY_SRC_DIR}/safety_flow.c
)
target_include_directories(test_safety_flow PRIVATE
    ${SAFETY_INC_DIR}
    ${SHARED_INC_DIR}
    mocks
)
target_link_libraries(test_safety_flow unity mocks)

# 主测试运行器 / Main test runner
add_executable(test_runner
    unit/test_runner.c
    unit/test_safety_flow.c
    ${SAFETY_SRC_DIR}/safety_flow.c
)
target_include_directories(test_runner PRIVATE
    ${SAFETY_INC_DIR}
    ${SHARED_INC_DIR}
    mocks
)
target_link_libraries(test_runner unity mocks)

# =============================================================================
# CTest 配置 / CTest Configuration
# =============================================================================
enable_testing()

add_test(NAME FlowTests COMMAND test_safety_flow)
add_test(NAME AllTests COMMAND test_runner)

# =============================================================================
# 自定义目标 / Custom Targets
# =============================================================================
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_runner
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all unit tests..."
)
