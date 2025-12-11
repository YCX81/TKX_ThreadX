# 编码规范

**项目**: TKX_ThreadX
**合规标准**: IEC 61508 SIL 2 / ISO 13849 PL d / MISRA-C:2012
**版本**: 1.0.0

---

## 目录

1. [文件结构](#1-文件结构)
2. [命名规范](#2-命名规范)
3. [注释规范](#3-注释规范)
4. [代码格式](#4-代码格式)
5. [安全编程规则](#5-安全编程规则)
6. [MISRA-C 关键规则](#6-misra-c-关键规则)
7. [禁止使用](#7-禁止使用)

---

## 1. 文件结构

### 1.1 头文件模板

```c
/**
 ******************************************************************************
 * @file    module_name.h
 * @brief   模块的简要描述
 * @author  作者名称
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * 详细描述和注意事项
 * 目标: STM32F407VGT6
 * 合规: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

#ifndef __MODULE_NAME_H
#define __MODULE_NAME_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Exported defines ----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __MODULE_NAME_H */
```

### 1.2 源文件模板

```c
/**
 ******************************************************************************
 * @file    module_name.c
 * @brief   模块的简要描述
 * @author  作者名称
 * @version V1.0.0
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "module_name.h"

/* Private defines -----------------------------------------------------------*/

/* Private types -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* ============================================================================
 * Public Functions
 * ============================================================================*/

/* ============================================================================
 * Private Functions
 * ============================================================================*/
```

---

## 2. 命名规范

### 2.1 文件命名

| 模块类型 | 格式 | 示例 |
|---------|------|------|
| Safety 模块 | `safety_xxx.c/h` | `safety_core.c` |
| BSP 模块 | `bsp_xxx.c/h` | `bsp_debug.c` |
| App 模块 | `app_xxx.c/h` | `app_main.c` |
| Service 模块 | `svc_xxx.c/h` | `svc_params.c` |

### 2.2 函数命名

```c
// 公共函数: Module_Function()
safety_status_t Safety_Watchdog_Init(void);
void BSP_Debug_Printf(const char *fmt, ...);
shared_status_t App_PreInit(void);

// 私有函数: 使用 static，小写或混合
static void update_state_machine(void);
static bool check_token_validity(uint8_t token);
```

### 2.3 变量命名

```c
// 静态变量: s_ 前缀
static safety_context_t s_safety_ctx;
static bool s_initialized = false;
static uint32_t s_error_count = 0;

// 全局变量: g_ 前缀 (尽量避免)
extern IWDG_HandleTypeDef g_hiwdg;

// 局部变量: 小写下划线
uint32_t current_time = 0;
bool is_valid = false;
```

### 2.4 类型定义

```c
// 类型名: 小写 + _t 后缀
typedef struct {
    safety_state_t state;
    safety_error_t last_error;
    uint32_t error_count;
} safety_context_t;

// 枚举值: 大写，显式赋值，U 后缀
typedef enum {
    SAFETY_STATE_INIT           = 0x00U,
    SAFETY_STATE_STARTUP_TEST   = 0x01U,
    SAFETY_STATE_NORMAL         = 0x02U,
    SAFETY_STATE_DEGRADED       = 0x03U,
    SAFETY_STATE_SAFE           = 0x04U,
    SAFETY_STATE_ERROR          = 0xFFU
} safety_state_t;
```

### 2.5 宏和常量

```c
// 常量: 大写 + U/UL 后缀
#define SAFETY_THREAD_STACK_SIZE    2048U
#define WDG_TOKEN_TIMEOUT_MS        800U
#define FLOW_SIGNATURE_SEED         0x5A5A5A5AUL
#define BOOT_CONFIG_MAGIC           0xC0F16000UL

// 位掩码: 显式十六进制
#define WDG_TOKEN_SAFETY_THREAD     0x01U
#define WDG_TOKEN_APP_THREAD        0x02U
#define WDG_TOKEN_ALL               0x03U
```

---

## 3. 注释规范

### 3.1 Doxygen 注释

```c
/**
 * @brief 初始化看门狗管理模块
 * @note  在启动 RTOS 内核前调用
 * @retval SAFETY_OK 成功
 * @retval SAFETY_ERROR 初始化失败
 */
safety_status_t Safety_Watchdog_Init(void);

/**
 * @brief 报告带参数的安全错误
 * @param error 错误码
 * @param param1 附加参数 1
 * @param param2 附加参数 2
 */
void Safety_ReportError(safety_error_t error, uint32_t param1, uint32_t param2);
```

### 3.2 段落分隔注释

```c
/* ============================================================================
 * 段落名称
 * ============================================================================*/

/* Private defines -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
```

### 3.3 行内注释

```c
uint32_t timeout = 1000U;  /* 超时时间（毫秒） */

/* 检查是否在超时窗口内收到所有令牌 */
if (Safety_Watchdog_CheckAllTokens())
{
    Safety_Watchdog_FeedIWDG();
}
```

---

## 4. 代码格式

### 4.1 缩进

- **使用空格，不使用 Tab**
- **缩进宽度: 4 空格**

### 4.2 大括号风格

```c
// Allman 风格: 大括号独占一行
if (condition)
{
    do_something();
}
else
{
    do_other();
}

void Function(void)
{
    /* Function body */
}
```

### 4.3 指针对齐

```c
// 指针符号靠右
char *ptr;
uint32_t *data_ptr;
void *(*callback)(void *arg);
```

### 4.4 行长度

- **最大 100 字符**
- 长表达式应换行

```c
safety_status_t result = Safety_Params_ValidateRange(
    params->hall_offset[i],
    HALL_OFFSET_MIN,
    HALL_OFFSET_MAX
);
```

---

## 5. 安全编程规则

### 5.1 输入验证

```c
safety_status_t Safety_Stack_RegisterThread(TX_THREAD *thread)
{
    /* 防御性检查 */
    if (!s_initialized)
    {
        return SAFETY_ERROR;
    }

    if (thread == NULL)
    {
        return SAFETY_INVALID_PARAM;
    }

    if (s_monitored_count >= MAX_MONITORED_THREADS)
    {
        return SAFETY_ERROR;
    }

    /* 正常处理 */
    /* ... */
}
```

### 5.2 返回值检查

```c
/* 始终检查返回值 */
safety_status_t status = Safety_Watchdog_Init();
if (status != SAFETY_OK)
{
    Safety_ReportError(SAFETY_ERR_WATCHDOG, status, 0);
    return status;
}
```

### 5.3 状态机规范

```c
safety_status_t Safety_SetState(safety_state_t new_state)
{
    safety_state_t old_state = s_safety_ctx.state;

    /* 验证状态转换有效性 */
    switch (s_safety_ctx.state)
    {
        case SAFETY_STATE_INIT:
            /* 只能转到 STARTUP_TEST 或 SAFE */
            if (new_state != SAFETY_STATE_STARTUP_TEST &&
                new_state != SAFETY_STATE_SAFE)
            {
                return SAFETY_ERROR;
            }
            break;

        /* 其他状态... */
    }

    s_safety_ctx.state = new_state;
    return SAFETY_OK;
}
```

### 5.4 数据冗余

```c
/* 关键数据使用位反转副本 */
typedef struct {
    float hall_offset[3];
    float hall_gain[3];
    uint32_t hall_offset_inv[3];  /* 位反转副本 */
    uint32_t hall_gain_inv[3];    /* 位反转副本 */
} safety_params_t;

/* 验证冗余 */
bool verify_redundancy(float value, uint32_t inverted)
{
    uint32_t val_bits = *(uint32_t *)&value;
    return (val_bits ^ inverted) == 0xFFFFFFFFUL;
}
```

### 5.5 超时处理

```c
/* 所有等待操作必须有超时 */
UINT tx_status = tx_thread_sleep(SAFETY_MONITOR_PERIOD_MS);
if (tx_status != TX_SUCCESS)
{
    /* 处理超时或错误 */
}

/* 硬件操作超时 */
uint32_t start_time = HAL_GetTick();
while (!flag_ready)
{
    if ((HAL_GetTick() - start_time) > TIMEOUT_MS)
    {
        return SAFETY_TIMEOUT;
    }
}
```

---

## 6. MISRA-C 关键规则

### 6.1 必须遵守

| 规则 | 描述 |
|------|------|
| Rule 1.3 | 不应有未定义或关键的未指定行为 |
| Rule 10.1 | 操作数类型不应不当 |
| Rule 11.3 | 不应在指针类型之间转换 |
| Rule 12.2 | 移位运算符右操作数应在适当范围内 |
| Rule 13.5 | 逻辑运算符的右操作数不应有副作用 |
| Rule 14.3 | 控制表达式不应是不变的 |
| Rule 17.7 | 函数返回值应使用 |
| Rule 21.3 | 不应使用 stdlib.h 的内存函数 |

### 6.2 建议遵守

| 规则 | 描述 |
|------|------|
| Rule 2.2 | 不应有死代码 |
| Rule 8.7 | 如果只在一个单元使用，函数应有内部链接 |
| Rule 15.5 | 函数应只有一个出口点 |
| Rule 18.4 | 不应使用 +、-、+= 和 -= 操作指针 |

---

## 7. 禁止使用

### 7.1 禁止的函数

```c
/* ❌ 禁止 */
malloc(), free(), realloc(), calloc()  /* 动态内存分配 */
sprintf(), vsprintf()                   /* 无边界检查 */
gets()                                  /* 不安全输入 */
strcpy(), strcat()                      /* 无边界检查 */

/* ✅ 使用替代 */
snprintf(), vsnprintf()                 /* 有边界检查 */
strncpy(), strncat()                    /* 有长度限制 */
```

### 7.2 禁止的模式

```c
/* ❌ 禁止: 未初始化变量 */
int value;
use_value(value);

/* ❌ 禁止: 魔法数字 */
if (status == 3)  /* 3 是什么？ */

/* ❌ 禁止: goto 语句 */
goto error_handler;

/* ❌ 禁止: 无限循环无退出条件 */
while (1)
{
    /* 没有 break 或 return */
}

/* ❌ 禁止: 副作用在逻辑表达式中 */
if (x++ && y--)
```

### 7.3 推荐模式

```c
/* ✅ 初始化所有变量 */
int value = 0;

/* ✅ 使用命名常量 */
if (status == STATUS_TIMEOUT)

/* ✅ 使用明确的循环控制 */
while (running)
{
    if (should_exit)
    {
        break;
    }
    /* ... */
}

/* ✅ 分开副作用和逻辑判断 */
x++;
y--;
if (x != 0 && y != 0)
```

---

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2025-12-10 | 初始版本 |
