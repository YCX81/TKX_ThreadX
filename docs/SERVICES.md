# 服务层文档

## 概述

服务层位于安全层之上、应用层之下，提供对安全参数和系统配置的访问接口。

## 模块列表

| 模块 | 文件 | 功能 |
|------|------|------|
| svc_params | svc_params.h/c | 参数服务 |

---

## 参数服务 (svc_params)

### 功能

- 从 Flash 读取安全参数和启动配置
- 验证参数完整性 (Magic, CRC, 冗余)
- 验证参数范围
- 提供参数访问接口

### 参数存储位置

| 参数类型 | Flash 地址 | 大小 |
|----------|------------|------|
| boot_config_t | 0x0800C000 | 36 字节 |
| safety_params_t | boot_config 之后 | 168 字节 |

### 参数结构

#### boot_config_t

```c
typedef struct __attribute__((packed)) {
    uint32_t magic;         /* 0xC0F16000 */
    uint32_t factory_mode;  /* 工厂模式标志 */
    uint32_t cal_valid;     /* 校准数据有效 */
    uint32_t app_crc;       /* 应用程序 CRC */
    uint32_t boot_count;    /* 启动计数 */
    uint32_t last_error;    /* 上次错误 */
    uint32_t reserved[2];   /* 保留 */
    uint32_t crc;           /* 结构体 CRC32 */
} boot_config_t;
```

#### safety_params_t

```c
typedef struct __attribute__((packed)) {
    /* 头部 */
    uint32_t magic;             /* 0xCA11B000 */
    uint16_t version;           /* 版本 0x0100 */
    uint16_t size;              /* 结构体大小 */

    /* HALL 传感器校准 */
    float hall_offset[3];       /* HALL 偏移 */
    float hall_gain[3];         /* HALL 增益 */
    float hall_offset_inv[3];   /* 偏移反码 */
    float hall_gain_inv[3];     /* 增益反码 */

    /* ADC 校准 */
    float adc_gain[8];          /* ADC 增益 */
    float adc_offset[8];        /* ADC 偏移 */

    /* 安全阈值 */
    float safety_threshold[4];

    /* 保留 */
    uint32_t reserved[7];

    /* CRC */
    uint32_t crc32;
} safety_params_t;
```

### API 参考

#### Svc_Params_Init

```c
shared_status_t Svc_Params_Init(void);
```

初始化参数服务，从 Flash 读取参数并验证。

**返回值**:
- `STATUS_OK` - 初始化成功，参数有效
- `STATUS_ERROR_MAGIC` - Magic 验证失败
- `STATUS_ERROR_CRC` - CRC 校验失败
- `STATUS_ERROR_REDUNDANCY` - 冗余校验失败
- `STATUS_ERROR_RANGE` - 参数超出范围

**使用示例**:
```c
if (Svc_Params_Init() != STATUS_OK)
{
    /* 参数无效，报告错误 */
    Safety_ReportError(SAFETY_ERR_PARAM_INVALID, 0, 0);
}
```

#### Svc_Params_Validate

```c
shared_status_t Svc_Params_Validate(void);
```

重新验证已加载的参数。

**验证步骤**:
1. 检查 Magic Number
2. 验证 CRC32
3. 验证冗余字段 (位反转)
4. 检查参数范围

#### Svc_Params_IsValid

```c
bool Svc_Params_IsValid(void);
```

检查参数是否有效。

**返回值**: `true` 如果参数已通过所有验证

#### Svc_Params_GetSafety

```c
const safety_params_t* Svc_Params_GetSafety(void);
```

获取安全参数只读指针。

**返回值**: 参数指针，如果无效则返回 `NULL`

#### Svc_Params_GetBootConfig

```c
const boot_config_t* Svc_Params_GetBootConfig(void);
```

获取启动配置只读指针。

**返回值**: 配置指针，如果未初始化则返回 `NULL`

### 参数访问函数

| 函数 | 参数 | 返回值 | 默认值 |
|------|------|--------|--------|
| `Svc_Params_GetHallOffset()` | channel (0-2) | float | 0.0 |
| `Svc_Params_GetHallGain()` | channel (0-2) | float | 1.0 |
| `Svc_Params_GetAdcGain()` | channel (0-7) | float | 1.0 |
| `Svc_Params_GetAdcOffset()` | channel (0-7) | float | 0.0 |
| `Svc_Params_GetSafetyThreshold()` | index (0-3) | float | 0.0 |

**注意**: 如果参数无效或通道超出范围，返回默认值。

### 参数范围验证

| 参数 | 最小值 | 最大值 |
|------|--------|--------|
| hall_offset | -1000.0 | 1000.0 |
| hall_gain | 0.5 | 2.0 |
| adc_gain | 0.8 | 1.2 |
| adc_offset | -500.0 | 500.0 |
| safety_threshold | 0.0 | 10000.0 |

### 冗余校验

安全参数使用位反转冗余存储关键参数：

```c
/* 验证冗余 */
static shared_status_t ValidateRedundancy(void)
{
    for (int i = 0; i < 3; i++)
    {
        uint32_t val, val_inv;

        /* 获取原值和反码 */
        memcpy(&val, &params.hall_offset[i], sizeof(uint32_t));
        memcpy(&val_inv, &params.hall_offset_inv[i], sizeof(uint32_t));

        /* 验证: val == ~val_inv */
        if (val != ~val_inv)
        {
            return STATUS_ERROR_REDUNDANCY;
        }
    }
    return STATUS_OK;
}
```

### 使用流程

```
┌─────────────────────────────────────────┐
│          App_PreInit()                  │
│              │                          │
│              ▼                          │
│       Svc_Params_Init()                 │
│              │                          │
│    ┌─────────┴─────────┐                │
│    ▼                   ▼                │
│ STATUS_OK         STATUS_ERROR_xxx      │
│    │                   │                │
│    ▼                   ▼                │
│ 正常使用         Safety_ReportError()   │
│ 参数             继续运行(降级)          │
└─────────────────────────────────────────┘
```

### 与安全模块集成

参数服务在 `App_PreInit()` 中初始化：

```c
shared_status_t App_PreInit(void)
{
    shared_status_t status;

    /* 初始化参数服务 */
    status = Svc_Params_Init();
    if (status != STATUS_OK)
    {
        /* 参数无效 - 报告错误但继续 */
        Safety_ReportError(SAFETY_ERR_PARAM_INVALID, status, 0);
    }

    return STATUS_OK;
}
```

应用程序使用参数：

```c
void ProcessSensorData(void)
{
    /* 检查参数有效性 */
    if (!Svc_Params_IsValid())
    {
        /* 使用默认值或拒绝处理 */
        return;
    }

    /* 应用校准参数 */
    for (int i = 0; i < 3; i++)
    {
        float offset = Svc_Params_GetHallOffset(i);
        float gain = Svc_Params_GetHallGain(i);

        calibrated_value[i] = (raw_value[i] - offset) * gain;
    }
}
```

### 参数烧录

参数需要通过调试器或专用工具烧录到 Config Flash：

1. **准备参数数据**
   - 填充 safety_params_t 结构
   - 计算冗余值 (`~原值`)
   - 计算 CRC32

2. **烧录流程**
   - 解锁 Flash
   - 擦除 Sector 3 (0x0800C000)
   - 写入 boot_config_t
   - 写入 safety_params_t
   - 锁定 Flash

3. **验证**
   - 复位系统
   - 检查 `Svc_Params_IsValid()` 返回 true

### 错误处理

| 错误码 | 含义 | 处理方式 |
|--------|------|----------|
| STATUS_ERROR_MAGIC | Magic 不匹配 | 参数未烧录或损坏 |
| STATUS_ERROR_CRC | CRC 不匹配 | 数据损坏 |
| STATUS_ERROR_REDUNDANCY | 冗余校验失败 | 位翻转错误 |
| STATUS_ERROR_RANGE | 参数超范围 | 参数值异常 |

所有错误都应导致系统进入降级模式或使用默认参数。
