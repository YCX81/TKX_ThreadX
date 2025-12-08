# 安全模块文档

本文档描述功能安全框架中各安全模块的设计与 API。

## 模块概览

| 模块 | 文件 | 功能 |
|------|------|------|
| safety_core | safety_core.h/c | 安全核心，状态机，错误处理 |
| safety_config | safety_config.h | 安全配置参数 |
| safety_monitor | safety_monitor.h/c | 安全监控线程 |
| safety_watchdog | safety_watchdog.h/c | 多线程令牌看门狗 |
| safety_selftest | safety_selftest.h/c | 启动/运行时自检 |
| safety_stack | safety_stack.h/c | 线程栈监控 |
| safety_flow | safety_flow.h/c | 程序流监控 |
| safety_mpu | safety_mpu.h/c | MPU 内存保护 |

---

## 1. Safety Core (安全核心)

### 概述

安全核心模块是整个安全框架的中枢，负责：
- 安全状态机管理
- 错误报告与日志
- 初始化协调
- 故障处理回调

### 数据类型

```c
/* 安全状态 */
typedef enum {
    SAFETY_STATE_INIT           = 0x00U,    /* 初始化 */
    SAFETY_STATE_STARTUP_TEST   = 0x01U,    /* 启动自检 */
    SAFETY_STATE_NORMAL         = 0x02U,    /* 正常运行 */
    SAFETY_STATE_DEGRADED       = 0x03U,    /* 降级运行 */
    SAFETY_STATE_SAFE           = 0x04U,    /* 安全停止 */
    SAFETY_STATE_ERROR          = 0xFFU     /* 错误状态 */
} safety_state_t;

/* 安全错误码 */
typedef enum {
    SAFETY_ERR_NONE             = 0x00U,
    SAFETY_ERR_CPU_TEST         = 0x01U,
    SAFETY_ERR_RAM_TEST         = 0x02U,
    SAFETY_ERR_FLASH_CRC        = 0x03U,
    SAFETY_ERR_CLOCK            = 0x04U,
    SAFETY_ERR_WATCHDOG         = 0x05U,
    SAFETY_ERR_STACK_OVERFLOW   = 0x06U,
    SAFETY_ERR_FLOW_MONITOR     = 0x07U,
    SAFETY_ERR_PARAM_INVALID    = 0x08U,
    SAFETY_ERR_RUNTIME_TEST     = 0x09U,
    SAFETY_ERR_MPU_FAULT        = 0x0AU,
    SAFETY_ERR_HARDFAULT        = 0x0BU,
    SAFETY_ERR_BUSFAULT         = 0x0CU,
    SAFETY_ERR_USAGEFAULT       = 0x0DU,
    SAFETY_ERR_NMI              = 0x0EU,
    SAFETY_ERR_INTERNAL         = 0xFFU
} safety_error_t;
```

### 核心 API

| 函数 | 说明 |
|------|------|
| `Safety_EarlyInit()` | HAL 初始化前调用 |
| `Safety_PostClockInit()` | 时钟配置后调用 |
| `Safety_PeripheralInit()` | 外设初始化后调用 |
| `Safety_StartupTest()` | 执行启动自检 |
| `Safety_PreKernelInit()` | ThreadX 启动前调用 |

### 状态管理 API

| 函数 | 说明 |
|------|------|
| `Safety_GetState()` | 获取当前安全状态 |
| `Safety_SetState()` | 设置安全状态 |
| `Safety_EnterNormal()` | 进入正常模式 |
| `Safety_EnterDegraded()` | 进入降级模式 |
| `Safety_EnterSafeState()` | 进入安全停止 |
| `Safety_IsOperational()` | 检查是否可运行 |

### 错误处理 API

| 函数 | 说明 |
|------|------|
| `Safety_ReportError()` | 报告安全错误 |
| `Safety_GetLastError()` | 获取最后错误 |
| `Safety_GetErrorCount()` | 获取错误计数 |
| `Safety_ClearError()` | 清除错误状态 |
| `Safety_GetErrorLog()` | 获取错误日志 |

### 使用示例

```c
/* main.c 中的初始化顺序 */
int main(void)
{
    /* 1. 早期初始化 */
    Safety_EarlyInit();

    /* 2. HAL 初始化 */
    HAL_Init();

    /* 3. 时钟配置 */
    SystemClock_Config();
    Safety_PostClockInit();

    /* 4. 外设初始化 */
    MX_GPIO_Init();
    MX_CRC_Init();
    // ...

    /* 5. 启动自检 */
    if (Safety_StartupTest() != SAFETY_OK)
    {
        /* 自检失败处理 */
    }

    /* 6. 启动 ThreadX */
    MX_ThreadX_Init();
}
```

---

## 2. Safety Monitor (安全监控线程)

### 概述

安全监控线程是最高优先级线程，周期性执行以下任务：
- 检查看门狗令牌并喂狗
- 监控线程栈使用率
- 验证程序流签名
- 执行增量式 Flash CRC

### 配置参数

```c
#define SAFETY_THREAD_STACK_SIZE    2048U   /* 栈大小 */
#define SAFETY_THREAD_PRIORITY      1U      /* 最高优先级 */
#define SAFETY_MONITOR_PERIOD_MS    100U    /* 周期 100ms */
```

### API

| 函数 | 说明 |
|------|------|
| `Safety_Monitor_Init()` | 初始化并创建线程 |
| `Safety_Monitor_ThreadEntry()` | 线程入口函数 |
| `Safety_Monitor_GetStats()` | 获取统计信息 |
| `Safety_Monitor_GetThread()` | 获取线程句柄 |
| `Safety_Monitor_Signal()` | 触发立即执行 |

### 线程执行流程

```c
void Safety_Monitor_ThreadEntry(ULONG thread_input)
{
    while (1)
    {
        /* 1. 记录流检查点 */
        Safety_Flow_Checkpoint(PFM_CP_APP_SAFETY_MONITOR);

        /* 2. 检查看门狗令牌 */
        Safety_Watchdog_Process();

        /* 3. 检查线程栈 */
        Safety_Stack_CheckAll();

        /* 4. 验证程序流 */
        Safety_Flow_Verify();

        /* 5. 增量 Flash CRC */
        Safety_SelfTest_FlashCRC_Continue();

        /* 6. 等待下一周期 */
        tx_thread_sleep(SAFETY_MONITOR_PERIOD_MS / 10);
    }
}
```

---

## 3. Safety Watchdog (看门狗管理)

### 概述

基于令牌的多线程看门狗机制：
- 每个关键线程必须周期性报告令牌
- 只有收齐所有令牌才喂狗
- 任一线程卡死会导致看门狗复位

### 令牌定义

```c
#define WDG_TOKEN_SAFETY_THREAD     0x01U
#define WDG_TOKEN_MAIN_THREAD       0x02U
#define WDG_TOKEN_COMM_THREAD       0x04U
#define WDG_TOKEN_ALL               0x07U
```

### API

| 函数 | 说明 |
|------|------|
| `Safety_Watchdog_Init()` | 初始化看门狗管理 |
| `Safety_Watchdog_Start()` | 启动 IWDG |
| `Safety_Watchdog_ReportToken()` | 报告线程令牌 |
| `Safety_Watchdog_CheckAllTokens()` | 检查令牌完整性 |
| `Safety_Watchdog_Process()` | 周期处理 |
| `Safety_Watchdog_EnterDegraded()` | 进入降级模式 |
| `Safety_Watchdog_GetStatus()` | 获取状态 |

### 使用示例

```c
/* 应用线程中 */
void App_MainThreadEntry(ULONG thread_input)
{
    while (1)
    {
        /* 业务逻辑 */
        ProcessBusinessLogic();

        /* 报告看门狗令牌 */
        Safety_Watchdog_ReportToken(WDG_TOKEN_MAIN_THREAD);

        tx_thread_sleep(10);
    }
}
```

### 时序要求

```
IWDG 超时: 1000ms
令牌超时: 800ms
喂狗周期: 500ms

时序图:
0ms      500ms    1000ms
|--------|--------|
    喂狗     喂狗

每个线程必须在 800ms 内报告令牌
```

---

## 4. Safety SelfTest (自检模块)

### 概述

提供启动时和运行时自检功能：
- 启动时：完整 CPU/RAM/Flash/时钟测试
- 运行时：增量式 Flash CRC 校验

### 测试结果

```c
typedef enum {
    SELFTEST_PASS           = 0x00U,
    SELFTEST_FAIL_CPU       = 0x01U,
    SELFTEST_FAIL_RAM       = 0x02U,
    SELFTEST_FAIL_FLASH     = 0x03U,
    SELFTEST_FAIL_CLOCK     = 0x04U,
    SELFTEST_FAIL_CRC       = 0x05U,
    SELFTEST_IN_PROGRESS    = 0xFEU,
    SELFTEST_NOT_RUN        = 0xFFU
} selftest_result_t;
```

### API

| 函数 | 说明 |
|------|------|
| `Safety_SelfTest_Init()` | 初始化 |
| `Safety_SelfTest_RunStartup()` | 运行启动自检 |
| `Safety_SelfTest_CPU()` | CPU 寄存器测试 |
| `Safety_SelfTest_RAM()` | RAM 测试 |
| `Safety_SelfTest_FlashCRC()` | Flash CRC 测试 |
| `Safety_SelfTest_FlashCRC_Continue()` | 增量 CRC 继续 |
| `Safety_SelfTest_Clock()` | 时钟验证 |

### 增量式 Flash CRC

```c
#define SELFTEST_FLASH_CRC_INTERVAL_MS  300000U  /* 5分钟完整周期 */
#define SELFTEST_FLASH_CRC_BLOCK_SIZE   4096U    /* 每次 4KB */

/* 应用程序大小 448KB，每次 4KB:
 * 448KB / 4KB = 112 次
 * 5分钟 / 112 = 约 2.7 秒/次
 */
```

---

## 5. Safety Stack (栈监控)

### 概述

监控 ThreadX 线程栈使用情况：
- 利用 ThreadX 栈填充模式检测使用率
- 设置警告和临界阈值
- 检测栈溢出

### 配置参数

```c
#define STACK_CHECK_INTERVAL_MS     100U        /* 检查周期 */
#define STACK_WARNING_THRESHOLD     70U         /* 70% 警告 */
#define STACK_CRITICAL_THRESHOLD    90U         /* 90% 临界 */
#define STACK_FILL_PATTERN          0xEFEFEFEFU /* 填充模式 */
```

### API

| 函数 | 说明 |
|------|------|
| `Safety_Stack_Init()` | 初始化 |
| `Safety_Stack_RegisterThread()` | 注册线程 |
| `Safety_Stack_UnregisterThread()` | 注销线程 |
| `Safety_Stack_CheckAll()` | 检查所有线程 |
| `Safety_Stack_GetInfo()` | 获取线程栈信息 |

### 使用示例

```c
/* 创建线程后注册 */
tx_thread_create(&my_thread, ...);
Safety_Stack_RegisterThread(&my_thread);
```

### 栈信息结构

```c
typedef struct {
    TX_THREAD   *thread;
    const char  *name;
    ULONG       stack_size;
    ULONG       stack_used;
    ULONG       stack_available;
    ULONG       stack_highest;      /* 历史最高 */
    uint8_t     usage_percent;
    bool        warning;            /* >= 70% */
    bool        critical;           /* >= 90% */
} stack_info_t;
```

---

## 6. Safety Flow (程序流监控)

### 概述

使用签名累积方式监控程序执行流程：
- 在关键位置记录检查点
- 累积签名值
- 周期性验证签名正确性

### 检查点定义

```c
/* 应用检查点 */
#define PFM_CP_APP_INIT             0x10U
#define PFM_CP_APP_SAFETY_MONITOR   0x11U
#define PFM_CP_APP_WATCHDOG_FEED    0x12U
#define PFM_CP_APP_SELFTEST_START   0x13U
#define PFM_CP_APP_SELFTEST_END     0x14U
#define PFM_CP_APP_MAIN_LOOP        0x15U
#define PFM_CP_APP_COMM_HANDLER     0x16U
#define PFM_CP_APP_PARAM_CHECK      0x17U
```

### API

| 函数 | 说明 |
|------|------|
| `Safety_Flow_Init()` | 初始化 |
| `Safety_Flow_Checkpoint()` | 记录检查点 |
| `Safety_Flow_Verify()` | 验证签名 |
| `Safety_Flow_Reset()` | 重置签名 |
| `Safety_Flow_GetSignature()` | 获取当前签名 |
| `Safety_Flow_SetExpected()` | 设置期望签名 |

### 使用示例

```c
void App_MainLoop(void)
{
    /* 记录循环入口检查点 */
    Safety_Flow_Checkpoint(PFM_CP_APP_MAIN_LOOP);

    /* 业务处理 */
    ProcessData();

    /* 通信处理 */
    Safety_Flow_Checkpoint(PFM_CP_APP_COMM_HANDLER);
    HandleCommunication();
}
```

---

## 7. Safety MPU (内存保护)

### 概述

配置 Cortex-M4 MPU 实现内存保护：
- 防止非法内存访问
- 保护 Flash 免受意外写入
- 隔离 Bootloader 区域

### 区域配置

| Region | 地址 | 大小 | 权限 | 用途 |
|--------|------|------|------|------|
| 0 | 0x08010000 | 512KB | RO+X | App Flash |
| 1 | 0x20000000 | 128KB | RW | Main RAM |
| 2 | 0x10000000 | 64KB | RW | CCM RAM |
| 3 | 0x40000000 | 512MB | RW+Device | Peripherals |
| 4 | 0x0800C000 | 16KB | RO | Config Flash |
| 5 | 0x08000000 | 64KB | No Access | Bootloader |

### API

| 函数 | 说明 |
|------|------|
| `Safety_MPU_Init()` | 初始化 MPU |
| `Safety_MPU_ConfigRegion()` | 配置单个区域 |
| `Safety_MPU_Enable()` | 启用 MPU |
| `Safety_MPU_Disable()` | 禁用 MPU |
| `Safety_MPU_IsEnabled()` | 检查是否启用 |
| `Safety_MPU_GetRegion()` | 获取区域配置 |

### 故障处理

MPU 访问违规会触发 MemManage 异常：

```c
void Safety_MemManageHandler(void)
{
    uint32_t mmfar = SCB->MMFAR;    /* 故障地址 */
    uint32_t cfsr = SCB->CFSR;      /* 故障状态 */

    Safety_LogError(SAFETY_ERR_MPU_FAULT, mmfar, cfsr);
    Safety_EnterSafeState(SAFETY_ERR_MPU_FAULT);
}
```

---

## 错误处理策略

### 错误分类

| 错误类型 | 处理方式 | 示例 |
|----------|----------|------|
| 致命错误 | 进入安全停止 | RAM/Flash 损坏 |
| 严重错误 | 进入降级模式 | 参数无效 |
| 警告 | 记录日志继续 | 栈使用率高 |

### 错误日志

```c
typedef struct {
    uint32_t timestamp;     /* 发生时间 */
    uint32_t error_code;    /* 错误码 */
    uint32_t param1;        /* 参数1 */
    uint32_t param2;        /* 参数2 */
} safety_error_log_t;

#define ERROR_LOG_MAX_ENTRIES   16
```

---

## 配置汇总

### 时间参数

| 参数 | 值 | 说明 |
|------|-----|------|
| `SAFETY_MONITOR_PERIOD_MS` | 100ms | 安全线程周期 |
| `WDG_FEED_PERIOD_MS` | 500ms | 喂狗周期 |
| `WDG_TOKEN_TIMEOUT_MS` | 800ms | 令牌超时 |
| `IWDG_TIMEOUT_MS` | 1000ms | IWDG 超时 |
| `FLASH_CRC_CHECK_INTERVAL_MS` | 300000ms | Flash CRC 周期 |
| `STACK_CHECK_INTERVAL_MS` | 100ms | 栈检查周期 |
| `FLOW_VERIFY_INTERVAL_MS` | 1000ms | 流验证周期 |

### 阈值参数

| 参数 | 值 | 说明 |
|------|-----|------|
| `STACK_WARNING_THRESHOLD` | 70% | 栈警告阈值 |
| `STACK_CRITICAL_THRESHOLD` | 90% | 栈临界阈值 |
| `CLOCK_TOLERANCE_PERCENT` | 5% | 时钟误差容限 |
| `SELFTEST_FLASH_CRC_BLOCK_SIZE` | 4KB | Flash CRC 块大小 |

---

## 8. Safety Params (参数验证)

### 概述

参数验证模块负责：
- 验证安全参数结构完整性
- 检查参数范围有效性
- 验证冗余副本一致性
- 周期性参数完整性检查

### 验证结果

```c
typedef enum {
    PARAMS_VALID            = 0x00U,    /* 参数有效 */
    PARAMS_ERR_MAGIC        = 0x01U,    /* 魔数无效 */
    PARAMS_ERR_VERSION      = 0x02U,    /* 版本不匹配 */
    PARAMS_ERR_SIZE         = 0x03U,    /* 大小不匹配 */
    PARAMS_ERR_CRC          = 0x04U,    /* CRC 错误 */
    PARAMS_ERR_HALL_RANGE   = 0x05U,    /* HALL 参数越界 */
    PARAMS_ERR_ADC_RANGE    = 0x06U,    /* ADC 参数越界 */
    PARAMS_ERR_THRESHOLD    = 0x07U,    /* 阈值越界 */
    PARAMS_ERR_REDUNDANCY   = 0x08U,    /* 冗余检查失败 */
} params_result_t;
```

### API

| 函数 | 说明 |
|------|------|
| `Safety_Params_Init()` | 初始化模块 |
| `Safety_Params_Validate()` | 验证参数结构 |
| `Safety_Params_ValidateFlash()` | 验证 Flash 中的参数 |
| `Safety_Params_Get()` | 获取已验证参数 |
| `Safety_Params_IsValid()` | 检查参数有效性 |
| `Safety_Params_PeriodicCheck()` | 周期性完整性检查 |

### 验证流程

```
1. 验证头部 (magic, version, size)
2. 验证 CRC32
3. 验证 HALL 参数范围
4. 验证 ADC 参数范围
5. 验证安全阈值范围
6. 验证冗余副本 (位取反)
```

### 参数范围定义

```c
#define HALL_OFFSET_MIN         (-1000.0f)
#define HALL_OFFSET_MAX         (1000.0f)
#define HALL_GAIN_MIN           (0.5f)
#define HALL_GAIN_MAX           (2.0f)
#define ADC_GAIN_MIN            (0.8f)
#define ADC_GAIN_MAX            (1.2f)
```

---

## 9. 双看门狗 (IWDG + WWDG)

### 概述

双通道看门狗提供更高可靠性：
- **IWDG**: 独立看门狗，使用 LSI 时钟
- **WWDG**: 窗口看门狗，使用 PCLK1 时钟

### 配置

```c
/* WWDG 启用/禁用 */
#define WWDG_ENABLED            0   /* 需要在 CubeMX 中配置后设为 1 */

/* WWDG 时序参数 */
#define WWDG_PRESCALER          8U
#define WWDG_WINDOW             0x50U   /* 窗口值 */
#define WWDG_COUNTER            0x7FU   /* 计数器值 */
```

### API (WWDG 扩展)

| 函数 | 说明 |
|------|------|
| `Safety_Watchdog_StartWWDG()` | 启动 WWDG |
| `Safety_Watchdog_FeedWWDG()` | 喂 WWDG |
| `Safety_Watchdog_WWDG_IRQHandler()` | WWDG 早期唤醒中断 |

### 双看门狗优势

| 特性 | IWDG | WWDG |
|------|------|------|
| 时钟源 | LSI (独立) | PCLK1 (系统) |
| 复位条件 | 超时 | 超时或过早喂狗 |
| 检测能力 | 卡死检测 | 时序异常检测 |
| 精度 | 低 | 高 |

---

## 10. 诊断输出 (RTT/SystemView)

### Segger RTT 诊断

RTT 提供实时调试输出，不占用 GPIO：

```c
/* bsp_debug.h */
#define DEBUG_INFO(fmt, ...)    SEGGER_RTT_printf(0, "[INF] " fmt "\r\n", ##__VA_ARGS__)
#define DEBUG_WARN(fmt, ...)    SEGGER_RTT_printf(0, "[WRN] " fmt "\r\n", ##__VA_ARGS__)
#define DEBUG_ERROR(fmt, ...)   SEGGER_RTT_printf(0, "[ERR] " fmt "\r\n", ##__VA_ARGS__)
```

### SystemView ThreadX 跟踪

SystemView 提供 RTOS 可视化分析：

```c
/* bsp_sysview.h */
#define SYSVIEW_ENABLED         0   /* 设为 1 启用 */

/* 初始化 */
BSP_SysView_Init();

/* 自定义事件记录 */
BSP_SysView_RecordEvent(0, "Safety check passed");
BSP_SysView_RecordValue(1, temperature);
```

### 使用工具

| 工具 | 用途 |
|------|------|
| J-Link RTT Viewer | 查看 RTT 调试输出 |
| Segger SystemView | 可视化 ThreadX 线程/中断 |

---

## 安全开发流程

### 1. 代码风格规范

```c
/**
 ******************************************************************************
 * @file    safety_xxx.h
 * @brief   Module Description
 * @author  YCX81
 * @version V1.0.0
 ******************************************************************************
 */

/* 章节分隔符 */
/* ============================================================================
 * Section Name
 * ============================================================================*/

/* 静态变量前缀 */
static xxx_t s_variable_name;

/* 函数命名 */
safety_status_t Safety_Module_Function(void);
```

### 2. IEC 61508 SIL 2 合规检查清单

| 项目 | 状态 | 说明 |
|------|------|------|
| CPU 自检 | ✓ | 寄存器模式测试 |
| RAM 自检 | ✓ | March C 算法 |
| Flash CRC | ✓ | 启动+增量验证 |
| 时钟监控 | ✓ | 频率范围检查 |
| 看门狗 | ✓ | 令牌机制 + 可选双通道 |
| 栈监控 | ✓ | ThreadX 集成 |
| 程序流 | ✓ | 签名累积 |
| MPU 保护 | ✓ | 6 区域配置 |
| 参数验证 | ✓ | CRC + 范围 + 冗余 |
| 错误日志 | ✓ | 循环缓冲区 |
| 诊断输出 | ✓ | RTT + SystemView |

### 3. 待完成项

- [ ] 正式 FMEA 文档
- [ ] 故障注入测试用例
- [ ] 代码覆盖率分析
- [ ] 汇编级 CPU 测试 (可选)
