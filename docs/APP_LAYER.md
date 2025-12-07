# 应用层文档

## 概述

应用层是功能安全框架的最上层，包含业务逻辑实现。应用层线程在安全框架的保护下运行。

## 文件结构

```
App/
├── Inc/
│   └── app_main.h      # 应用层接口
└── Src/
    └── app_main.c      # 应用层实现
```

## 线程配置

| 线程 | 栈大小 | 优先级 | 说明 |
|------|--------|--------|------|
| App Main | 4KB | 5 | 主业务逻辑 |
| App Comm | 2KB | 10 | 通信处理 |

### 配置定义

```c
#define APP_MAIN_THREAD_STACK_SIZE      4096U
#define APP_MAIN_THREAD_PRIORITY        5U
#define APP_MAIN_THREAD_PREEMPT_THRESH  5U

#define APP_COMM_THREAD_STACK_SIZE      2048U
#define APP_COMM_THREAD_PRIORITY        10U
#define APP_COMM_THREAD_PREEMPT_THRESH  10U
```

## API 参考

### App_PreInit

```c
shared_status_t App_PreInit(void);
```

ThreadX 启动前的初始化，主要初始化参数服务。

**调用位置**: `main()` 中 `MX_ThreadX_Init()` 之前

### App_CreateThreads

```c
UINT App_CreateThreads(TX_BYTE_POOL *byte_pool);
```

创建应用层线程。

**参数**:
- `byte_pool` - ThreadX 字节池，用于分配线程栈

**调用位置**: `tx_application_define()` 中

**执行流程**:
1. 创建 Safety Monitor 线程
2. 分配 Main 线程栈
3. 创建 Main 线程
4. 注册 Main 线程栈监控
5. 分配 Comm 线程栈
6. 创建 Comm 线程
7. 注册 Comm 线程栈监控

### App_MainThreadEntry

```c
void App_MainThreadEntry(ULONG thread_input);
```

主应用线程入口函数。

**执行内容**:
- 等待安全系统就绪
- 执行主业务逻辑
- 报告看门狗令牌
- 记录程序流检查点

### App_CommThreadEntry

```c
void App_CommThreadEntry(ULONG thread_input);
```

通信线程入口函数。

**执行内容**:
- 等待安全系统就绪
- 处理通信
- 报告看门狗令牌
- 记录程序流检查点

### App_GetMainThread / App_GetCommThread

```c
TX_THREAD* App_GetMainThread(void);
TX_THREAD* App_GetCommThread(void);
```

获取线程句柄。

## 应用线程模板

### 主线程示例

```c
void App_MainThreadEntry(ULONG thread_input)
{
    (void)thread_input;

    /* 等待安全系统就绪 */
    while (!Safety_IsOperational())
    {
        tx_thread_sleep(10);
    }

    /* 主循环 */
    while (1)
    {
        /* 检查安全状态 */
        safety_state_t state = Safety_GetState();

        if (state == SAFETY_STATE_NORMAL)
        {
            /* ======================================
             * 正常操作 - 添加业务逻辑
             * ====================================== */

            /* 示例: 读取传感器 */
            ReadSensors();

            /* 示例: 处理数据 */
            ProcessData();

            /* 示例: 控制输出 */
            UpdateOutputs();

            /* 记录程序流检查点 */
            Safety_Flow_Checkpoint(PFM_CP_APP_MAIN_LOOP);

            /* 报告看门狗令牌 */
            Safety_Watchdog_ReportToken(WDG_TOKEN_MAIN_THREAD);
        }
        else if (state == SAFETY_STATE_DEGRADED)
        {
            /* ======================================
             * 降级操作 - 功能受限
             * ====================================== */

            /* 示例: 只读取传感器，不控制输出 */
            ReadSensors();

            /* 仍需报告令牌 */
            Safety_Watchdog_ReportToken(WDG_TOKEN_MAIN_THREAD);
        }
        else
        {
            /* 安全停止或错误状态 - 不执行操作 */
        }

        /* 线程休眠 */
        tx_thread_sleep(10);  /* 10ms 周期 */
    }
}
```

### 通信线程示例

```c
void App_CommThreadEntry(ULONG thread_input)
{
    (void)thread_input;

    /* 等待安全系统就绪 */
    while (!Safety_IsOperational())
    {
        tx_thread_sleep(10);
    }

    /* 通信循环 */
    while (1)
    {
        safety_state_t state = Safety_GetState();

        if (state == SAFETY_STATE_NORMAL || state == SAFETY_STATE_DEGRADED)
        {
            /* ======================================
             * 通信处理
             * ====================================== */

            /* 示例: 检查 CAN 消息 */
            if (CAN_MessageAvailable())
            {
                ProcessCANMessage();
            }

            /* 示例: 发送状态 */
            SendStatusMessage();

            /* 记录检查点 */
            Safety_Flow_Checkpoint(PFM_CP_APP_COMM_HANDLER);

            /* 报告令牌 */
            Safety_Watchdog_ReportToken(WDG_TOKEN_COMM_THREAD);
        }

        /* 事件驱动休眠 */
        tx_thread_sleep(100);
    }
}
```

## 添加新线程

### 步骤

1. **定义线程配置**

```c
/* app_main.h */
#define APP_NEW_THREAD_STACK_SIZE      2048U
#define APP_NEW_THREAD_PRIORITY        8U
#define APP_NEW_THREAD_PREEMPT_THRESH  8U
```

2. **添加线程变量**

```c
/* app_main.c */
static TX_THREAD s_new_thread;
static UCHAR *s_new_stack = NULL;
```

3. **创建线程**

```c
/* App_CreateThreads() 中 */
status = tx_byte_allocate(byte_pool,
                          (VOID **)&s_new_stack,
                          APP_NEW_THREAD_STACK_SIZE,
                          TX_NO_WAIT);
if (status != TX_SUCCESS) return status;

status = tx_thread_create(&s_new_thread,
                          "App New",
                          App_NewThreadEntry,
                          0,
                          s_new_stack,
                          APP_NEW_THREAD_STACK_SIZE,
                          APP_NEW_THREAD_PRIORITY,
                          APP_NEW_THREAD_PREEMPT_THRESH,
                          TX_NO_TIME_SLICE,
                          TX_AUTO_START);
if (status != TX_SUCCESS) return status;

/* 注册栈监控 */
Safety_Stack_RegisterThread(&s_new_thread);
```

4. **定义看门狗令牌** (如果需要)

```c
/* safety_config.h */
#define WDG_TOKEN_NEW_THREAD    0x08U
#define WDG_TOKEN_ALL           (WDG_TOKEN_SAFETY_THREAD | \
                                 WDG_TOKEN_MAIN_THREAD | \
                                 WDG_TOKEN_COMM_THREAD | \
                                 WDG_TOKEN_NEW_THREAD)
```

5. **实现线程入口**

```c
void App_NewThreadEntry(ULONG thread_input)
{
    (void)thread_input;

    while (!Safety_IsOperational())
    {
        tx_thread_sleep(10);
    }

    while (1)
    {
        if (Safety_GetState() == SAFETY_STATE_NORMAL)
        {
            /* 业务逻辑 */
            DoWork();

            /* 报告令牌 (如果注册了) */
            Safety_Watchdog_ReportToken(WDG_TOKEN_NEW_THREAD);
        }

        tx_thread_sleep(50);
    }
}
```

## 与安全模块交互

### 检查安全状态

```c
/* 获取当前状态 */
safety_state_t state = Safety_GetState();

switch (state)
{
    case SAFETY_STATE_NORMAL:
        /* 正常运行 */
        break;

    case SAFETY_STATE_DEGRADED:
        /* 降级运行 */
        break;

    case SAFETY_STATE_SAFE:
    case SAFETY_STATE_ERROR:
        /* 停止操作 */
        break;

    default:
        break;
}

/* 检查是否可运行 */
if (Safety_IsOperational())
{
    /* 可以执行操作 */
}
```

### 报告错误

```c
/* 检测到异常时报告 */
if (sensor_value > MAX_SAFE_VALUE)
{
    Safety_ReportError(SAFETY_ERR_INTERNAL,
                       (uint32_t)sensor_value,
                       MAX_SAFE_VALUE);
}
```

### 使用参数服务

```c
/* 检查参数有效性 */
if (Svc_Params_IsValid())
{
    /* 获取校准参数 */
    float gain = Svc_Params_GetHallGain(0);
    float offset = Svc_Params_GetHallOffset(0);

    /* 应用校准 */
    calibrated = (raw - offset) * gain;
}
else
{
    /* 使用默认值或拒绝处理 */
    calibrated = raw;
}
```

## 初始化时序

```
main()
├── Safety_EarlyInit()
├── HAL_Init()
├── SystemClock_Config()
├── Safety_PostClockInit()
├── MX_xxx_Init()
├── Safety_StartupTest()
├── App_PreInit()              ← 参数服务初始化
├── Safety_PreKernelInit()
└── MX_ThreadX_Init()
    └── tx_application_define()
        └── App_CreateThreads()
            ├── Safety_Monitor_Init()  ← 安全监控线程
            ├── Create Main Thread
            ├── Stack_RegisterThread(Main)
            ├── Create Comm Thread
            └── Stack_RegisterThread(Comm)

ThreadX Kernel Running
├── Safety_Monitor_ThreadEntry()  [Priority 1]
├── App_MainThreadEntry()         [Priority 5]
└── App_CommThreadEntry()         [Priority 10]
```

## 注意事项

### 1. 等待安全系统就绪

所有应用线程必须在开始业务逻辑前等待安全系统就绪：

```c
while (!Safety_IsOperational())
{
    tx_thread_sleep(10);
}
```

### 2. 看门狗令牌报告

如果线程注册了看门狗令牌，必须周期性报告：

- 令牌超时: 800ms
- 建议报告周期: 每次主循环

### 3. 程序流检查点

在关键执行点记录检查点：

```c
Safety_Flow_Checkpoint(PFM_CP_APP_MAIN_LOOP);
```

### 4. 降级模式行为

降级模式下应限制功能：
- 停止控制输出
- 继续监控/通信
- 仍需报告看门狗令牌

### 5. 栈大小

根据线程功能分配合适的栈：
- 简单任务: 1-2KB
- 复杂计算: 4KB+
- 使用栈监控验证

### 6. 优先级设计

| 优先级 | 线程类型 |
|--------|----------|
| 1 | 安全监控 (最高) |
| 2-4 | 实时控制 |
| 5-7 | 主业务 |
| 8-15 | 通信/UI |
| 16+ | 后台任务 |

## 调试技巧

### 1. 查看线程状态

```c
/* 通过 IAR 调试器查看 ThreadX 变量 */
/* _tx_thread_created_ptr - 线程链表 */
/* _tx_thread_current_ptr - 当前线程 */
```

### 2. 栈使用分析

```c
stack_info_t info;
Safety_Stack_GetInfo(&s_main_thread, &info);

/* 检查 usage_percent, warning, critical */
```

### 3. 安全状态诊断

```c
const safety_context_t *ctx = Safety_GetContext();

/* 查看:
 * - ctx->state
 * - ctx->last_error
 * - ctx->error_count
 */
```
