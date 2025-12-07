# Bootloader 设计文档

## 概述

安全启动引导程序 (Safety Bootloader) 负责在系统上电后执行必要的安全检测，验证应用程序完整性，并安全跳转到主应用程序。

## 设计目标

- 启动时完整性校验
- 安全参数验证
- 程序流监控
- 失败时进入安全状态
- 支持工厂模式

## 内存配置

| 区域 | 地址范围 | 大小 | 说明 |
|------|----------|------|------|
| Bootloader 代码 | 0x08000000 - 0x0800BFFB | 47KB | 引导程序 |
| Bootloader CRC | 0x0800BFFC - 0x0800BFFF | 4B | 自身 CRC |
| Config 区域 | 0x0800C000 - 0x0800FFFF | 16KB | 配置参数 |

## 启动流程

```
┌────────────────────────────────────────────────────────────┐
│                       System Reset                          │
└────────────────────────────┬───────────────────────────────┘
                             │
                             ▼
┌────────────────────────────────────────────────────────────┐
│ 1. BOOT_STATE_INIT                                         │
│    - HAL_Init()                                            │
│    - SystemClock_Config() → 168MHz from HSE               │
│    - Enable CRC peripheral                                 │
└────────────────────────────┬───────────────────────────────┘
                             │
                             ▼
┌────────────────────────────────────────────────────────────┐
│ 2. BOOT_STATE_SELFTEST                                     │
│    - CPU Register Test                                     │
│    - RAM March-C Test                                      │
│    - Flash CRC Verification                                │
│    - Clock Frequency Check                                 │
└────────────────────────────┬───────────────────────────────┘
                             │ 失败 → Safe State
                             ▼
┌────────────────────────────────────────────────────────────┐
│ 3. BOOT_STATE_VALIDATE_PARAMS                              │
│    - 读取安全参数 (SAFETY_PARAMS_ADDR)                      │
│    - Magic Number 验证                                      │
│    - CRC32 验证                                             │
│    - 冗余字段验证                                            │
└────────────────────────────┬───────────────────────────────┘
                             │ 失败 → Safe State
                             ▼
┌────────────────────────────────────────────────────────────┐
│ 4. BOOT_STATE_CHECK_CONFIG                                 │
│    - 读取启动配置                                           │
│    - 检查 factory_mode 标志                                 │
│    - 若为工厂模式 → 进入工厂模式流程                          │
└────────────────────────────┬───────────────────────────────┘
                             │
                             ▼
┌────────────────────────────────────────────────────────────┐
│ 5. BOOT_STATE_VERIFY_APP                                   │
│    - 计算应用程序 CRC (0x08010000 - 0x0807FFFB)            │
│    - 与存储的 CRC 比较 (0x0807FFFC)                         │
└────────────────────────────┬───────────────────────────────┘
                             │ 失败 → Safe State
                             ▼
┌────────────────────────────────────────────────────────────┐
│ 6. BOOT_STATE_JUMP_TO_APP                                  │
│    - 验证程序流签名                                         │
│    - 设置 MSP (Main Stack Pointer)                         │
│    - 跳转到应用程序入口                                      │
└────────────────────────────────────────────────────────────┘
```

## 状态定义

```c
typedef enum {
    BOOT_STATE_INIT         = 0x00U,    /* 初始化 */
    BOOT_STATE_SELFTEST     = 0x01U,    /* 自检中 */
    BOOT_STATE_VALIDATE_PARAMS = 0x02U, /* 参数验证 */
    BOOT_STATE_CHECK_CONFIG = 0x03U,    /* 配置检查 */
    BOOT_STATE_VERIFY_APP   = 0x04U,    /* 应用验证 */
    BOOT_STATE_JUMP_TO_APP  = 0x05U,    /* 准备跳转 */
    BOOT_STATE_FACTORY_MODE = 0x06U,    /* 工厂模式 */
    BOOT_STATE_SAFE         = 0xFFU     /* 安全停止 */
} boot_state_t;
```

## 自检模块

### CPU 寄存器测试

测试内容：
- R0-R12 通用寄存器
- LR (Link Register)
- APSR (Application Program Status Register)

测试方法：
- 写入测试模式 (0xAAAAAAAA, 0x55555555)
- 读回验证

### RAM 测试 (March-C)

测试区域: 0x20018000 - 0x2001FFFF (32KB)

March-C 算法步骤：
1. ↑ w0 - 向上写0
2. ↑ r0,w1 - 向上读0写1
3. ↑ r1,w0 - 向上读1写0
4. ↓ r0,w1 - 向下读0写1
5. ↓ r1,w0 - 向下读1写0
6. r0 - 读0验证

### Flash CRC 校验

```c
boot_status_t Boot_VerifyAppCRC(void)
{
    uint32_t calc_crc;
    uint32_t stored_crc;

    /* 计算应用程序区域 CRC */
    calc_crc = Boot_CRC32_Calculate(
        (uint8_t *)APP_FLASH_START,
        APP_FLASH_SIZE - 4  /* 排除最后4字节CRC */
    );

    /* 读取存储的 CRC */
    stored_crc = *(uint32_t *)APP_CRC_ADDR;

    return (calc_crc == stored_crc) ? BOOT_OK : BOOT_ERROR_CRC;
}
```

### 时钟验证

验证系统时钟是否正确配置为 168MHz (允许 ±5% 误差)。

## 参数结构

### boot_config_t

```c
typedef struct __attribute__((packed)) {
    uint32_t magic;         /* 0xC0F16000 */
    uint32_t factory_mode;  /* 工厂模式标志 */
    uint32_t cal_valid;     /* 校准数据有效标志 */
    uint32_t app_crc;       /* 应用程序 CRC (快速验证用) */
    uint32_t boot_count;    /* 启动计数器 */
    uint32_t last_error;    /* 上次错误码 */
    uint32_t reserved[2];   /* 保留 */
    uint32_t crc;           /* 结构体 CRC32 */
} boot_config_t;
```

### safety_params_t

```c
typedef struct __attribute__((packed)) {
    /* 头部 - 8 bytes */
    uint32_t magic;             /* 0xCA11B000 */
    uint16_t version;           /* 版本 1.0 */
    uint16_t size;              /* 结构体大小 */

    /* HALL 传感器校准 - 48 bytes */
    float hall_offset[3];       /* HALL 偏移 */
    float hall_gain[3];         /* HALL 增益 */
    float hall_offset_inv[3];   /* 偏移反码 (冗余) */
    float hall_gain_inv[3];     /* 增益反码 (冗余) */

    /* ADC 校准 - 64 bytes */
    float adc_gain[8];          /* ADC 通道增益 */
    float adc_offset[8];        /* ADC 通道偏移 */

    /* 安全阈值 - 16 bytes */
    float safety_threshold[4];  /* 安全阈值 */

    /* 保留 - 28 bytes */
    uint32_t reserved[7];

    /* 完整性校验 - 4 bytes */
    uint32_t crc32;             /* CRC32 */
} safety_params_t;
```

## 程序流监控

### 检查点定义

```c
typedef enum {
    PFM_CP_INIT             = 0x01U,
    PFM_CP_SELFTEST_START   = 0x02U,
    PFM_CP_SELFTEST_END     = 0x07U,
    PFM_CP_PARAMS_CHECK     = 0x08U,
    PFM_CP_CONFIG_CHECK     = 0x09U,
    PFM_CP_APP_VERIFY       = 0x0BU,
    PFM_CP_JUMP_PREPARE     = 0x0DU,
    PFM_CP_JUMP_EXECUTE     = 0x0EU
} pfm_checkpoint_t;
```

### 签名计算

```c
static void Boot_FlowMonitor_Update(pfm_checkpoint_t checkpoint)
{
    s_flow_signature ^= ((uint32_t)checkpoint << 24) |
                        ((uint32_t)checkpoint << 16) |
                        ((uint32_t)checkpoint << 8) |
                        ((uint32_t)checkpoint);
}
```

在跳转前验证签名，确保所有检查点都按顺序执行。

## 跳转到应用程序

```c
void Boot_JumpToApplication(void)
{
    typedef void (*pFunction)(void);
    pFunction Jump_To_Application;
    uint32_t JumpAddress;

    /* 关闭所有中断 */
    __disable_irq();

    /* 反初始化外设 */
    HAL_DeInit();

    /* 获取应用程序入口地址 */
    JumpAddress = *(__IO uint32_t *)(APP_FLASH_START + 4);
    Jump_To_Application = (pFunction)JumpAddress;

    /* 设置主栈指针 */
    __set_MSP(*(__IO uint32_t *)APP_FLASH_START);

    /* 跳转 */
    Jump_To_Application();
}
```

## 安全停止状态

当检测到以下错误时进入安全停止状态：

- 自检失败
- 安全参数校验失败
- 应用程序 CRC 错误
- 程序流签名错误

安全停止行为：
1. 禁用所有中断
2. 设置所有输出为安全状态
3. 停止喂狗，等待看门狗复位
4. 或进入无限循环等待

## 工厂模式

工厂模式通过调试器设置 `factory_mode` 标志触发：

1. 调试器写入 `factory_mode = 1` 到 Config Flash
2. 系统复位
3. Bootloader 检测到工厂模式标志
4. 进入工厂模式处理
5. 完成后清除标志并复位

工厂模式用途：
- 校准数据写入
- 参数配置
- 生产测试

## API 参考

### Boot_Main

```c
void Boot_Main(void);
```
Bootloader 主入口函数，执行完整启动流程。

### Boot_GetState

```c
boot_state_t Boot_GetState(void);
```
获取当前 Bootloader 状态。

### Boot_EnterSafeState

```c
void Boot_EnterSafeState(boot_status_t error);
```
进入安全停止状态。

### Boot_ValidateSafetyParams

```c
boot_status_t Boot_ValidateSafetyParams(safety_params_t *params);
```
验证安全参数完整性。

### Boot_VerifyAppCRC

```c
boot_status_t Boot_VerifyAppCRC(void);
```
验证应用程序 CRC。

## 编译配置

### IAR 链接器配置

```
define symbol __ICFEDIT_region_ROM_start__ = 0x08000000;
define symbol __ICFEDIT_region_ROM_end__   = 0x0800BFFF;
define symbol __ICFEDIT_region_RAM_start__ = 0x20000000;
define symbol __ICFEDIT_region_RAM_end__   = 0x2001FFFF;

define region ROM_region = mem:[from __ICFEDIT_region_ROM_start__
                                to   __ICFEDIT_region_ROM_end__];
```

### CRC 位置

在链接器脚本中保留最后 4 字节用于存储 CRC：

```
place at address mem:0x0800BFFC { readonly section .boot_crc };
```
