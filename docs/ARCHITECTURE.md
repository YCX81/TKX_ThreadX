# 架构设计 / Architecture Design

**项目 / Project**: TKX_ThreadX
**合规标准 / Compliance**: IEC 61508 SIL 2 / ISO 13849 PL d
**版本 / Version**: 1.0.0

> 本文档使用 Mermaid.js 绘制架构图，可在 GitHub、VS Code (Markdown Preview Mermaid) 或其他支持 Mermaid 的工具中查看。

---

## 目录 / Table of Contents

1. [系统架构概览](#1-系统架构概览)
2. [分层职责](#2-分层职责)
3. [内存布局详解](#3-内存布局详解)
4. [线程架构](#4-线程架构)
5. [初始化时序](#5-初始化时序)
6. [安全状态机](#6-安全状态机)
7. [模块依赖关系](#7-模块依赖关系)
8. [设计决策](#8-设计决策)
9. [CI/CD 流程](#9-cicd-流程)

---

## 1. 系统架构概览

### 1.1 硬件架构 / Hardware Architecture

```mermaid
graph TB
    subgraph MCU["STM32F407VGT6"]
        CPU["ARM Cortex-M4<br/>168 MHz + FPU"]
        FLASH["Flash 1MB"]
        RAM["SRAM 192KB"]
        CCM["CCM 64KB"]
    end

    subgraph Peripherals["外设 / Peripherals"]
        IWDG["IWDG<br/>独立看门狗"]
        WWDG["WWDG<br/>窗口看门狗"]
        CRC["CRC 单元"]
        MPU["MPU<br/>内存保护"]
        SPI["SPI1/SPI2"]
        UART["USART1/2/3"]
        GPIO["GPIO"]
    end

    subgraph External["外部器件"]
        W25Q["W25Q128<br/>SPI Flash"]
        RTT["Segger RTT<br/>调试输出"]
    end

    CPU --> FLASH
    CPU --> RAM
    CPU --> CCM
    CPU --> IWDG
    CPU --> WWDG
    CPU --> CRC
    CPU --> MPU
    SPI --> W25Q
    CPU --> RTT
```

### 1.2 系统组件关系 / System Component Relations

```mermaid
graph LR
    subgraph Bootloader["Bootloader (48KB)"]
        BOOT_SELF[自检模块]
        BOOT_VERIFY[固件验证]
        BOOT_JUMP[跳转逻辑]
    end

    subgraph Application["Application (448KB)"]
        APP_SAFETY[Safety Framework]
        APP_RTOS[ThreadX RTOS]
        APP_LOGIC[业务逻辑]
    end

    subgraph SharedParams["共享参数区 (16KB)"]
        BOOT_CFG[boot_config]
        SAFETY_PARAM[safety_params]
    end

    BOOT_SELF --> BOOT_VERIFY
    BOOT_VERIFY --> BOOT_JUMP
    BOOT_JUMP -->|验证通过| APP_SAFETY
    SharedParams <-->|读写| Bootloader
    SharedParams <-->|读取| Application
```

### 1.3 软件分层架构 (ASCII)

```
┌─────────────────────────────────────────────────────────────────┐
│                      Application Layer                          │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐              │
│  │ Main Thread │  │ Comm Thread │  │ Other Tasks │              │
│  └─────────────┘  └─────────────┘  └─────────────┘              │
├─────────────────────────────────────────────────────────────────┤
│                       Services Layer                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐              │
│  │ Param Svc   │  │ Diag Svc    │  │ Comm Svc    │              │
│  └─────────────┘  └─────────────┘  └─────────────┘              │
├─────────────────────────────────────────────────────────────────┤
│                       Safety Layer                               │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐            │
│  │ Core     │ │ Watchdog │ │ Stack    │ │ Flow     │            │
│  ├──────────┤ ├──────────┤ ├──────────┤ ├──────────┤            │
│  │ Monitor  │ │ SelfTest │ │ MPU      │ │ Config   │            │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘            │
├─────────────────────────────────────────────────────────────────┤
│                    Azure RTOS ThreadX                            │
├─────────────────────────────────────────────────────────────────┤
│                    STM32 HAL Driver                              │
├─────────────────────────────────────────────────────────────────┤
│                    Hardware (STM32F407)                          │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. 分层职责

### 2.1 六层架构 / Six-Layer Architecture

```mermaid
graph TB
    subgraph L6["Layer 6: Application 应用层"]
        APP["app_main.c<br/>业务逻辑"]
    end

    subgraph L5["Layer 5: Services 服务层"]
        SVC_PARAM["svc_params<br/>参数服务"]
        SVC_COMM["svc_comm<br/>通信服务"]
    end

    subgraph L4["Layer 4: Safety 安全层"]
        SAFE_CORE["safety_core"]
        SAFE_MON["safety_monitor"]
        SAFE_WDG["safety_watchdog"]
        SAFE_TEST["safety_selftest"]
        SAFE_STACK["safety_stack"]
        SAFE_FLOW["safety_flow"]
        SAFE_MPU["safety_mpu"]
    end

    subgraph L3["Layer 3: RTOS 实时操作系统层"]
        TX["ThreadX RTOS"]
        FX["FileX 文件系统"]
    end

    subgraph L2["Layer 2: BSP/HAL 板级支持层"]
        BSP["BSP Drivers"]
        HAL["STM32 HAL"]
    end

    subgraph L1["Layer 1: Hardware 硬件层"]
        HW["STM32F407VGT6"]
    end

    L6 --> L5
    L5 --> L4
    L4 --> L3
    L3 --> L2
    L2 --> L1

    style L4 fill:#f96,stroke:#333,stroke-width:2px
```

### 2.2 各层职责

| 层级 | 名称 | 职责 |
|------|------|------|
| **Hardware Layer** | 硬件层 | STM32F407VGT6 MCU、外设：CRC, IWDG, WWDG, MPU, Flash, GPIO |
| **HAL Layer** | 驱动层 | STM32Cube HAL 驱动、硬件抽象，提供统一 API |
| **RTOS Layer** | 系统层 | Azure RTOS ThreadX 6.1.10、多线程调度、同步原语、内存管理 |
| **Safety Layer** | 安全层 | 功能安全核心模块、自检、监控、保护机制 |
| **Services Layer** | 服务层 | 参数管理服务、诊断服务 |
| **Application Layer** | 应用层 | 业务逻辑实现、应用线程 |

---

## 3. 内存布局详解

### 3.1 Flash 分区

```mermaid
graph TB
    subgraph Flash["Flash Memory (1MB)"]
        direction TB
        BOOT["0x0800_0000<br/>Bootloader<br/>48KB"]
        BOOT_CFG["0x0800_C000<br/>Boot Config<br/>16KB"]
        APP["0x0801_0000<br/>Application<br/>448KB"]
        PARAMS["0x0808_0000<br/>Parameters<br/>512KB"]
    end

    style BOOT fill:#4a9,stroke:#333
    style BOOT_CFG fill:#9cf,stroke:#333
    style APP fill:#f96,stroke:#333
    style PARAMS fill:#fc9,stroke:#333
```

**Flash 分区 (ASCII)**:
```
┌───────────────────────────┐ 0x08080000
│                           │
│      Reserved (512KB)     │
│                           │
├───────────────────────────┤ 0x08010000
│                           │
│    Application (448KB)    │
│    Sectors 4-7            │
│                           │
├───────────────────────────┤ 0x0800C000
│   Config/Calibration      │
│        (16KB)             │
│      Sector 3             │
├───────────────────────────┤ 0x08000000
│    Bootloader (48KB)      │
│    Sectors 0-2            │
└───────────────────────────┘
```

### 3.2 RAM 分区

```mermaid
graph TB
    subgraph RAM["SRAM (192KB)"]
        direction TB
        STACK["栈区<br/>Stack"]
        HEAP["堆区<br/>Heap (禁用)"]
        BSS["BSS 段"]
        DATA["Data 段"]
        TX_POOL["ThreadX<br/>Byte Pool"]
    end

    subgraph CCM["CCM RAM (64KB)"]
        direction TB
        SAFETY_CTX["Safety Context"]
        ERROR_LOG["Error Log"]
        CRITICAL["关键数据"]
    end

    style CCM fill:#f96,stroke:#333,stroke-width:2px
```

**RAM 分区 (ASCII)**:
```
┌───────────────────────────┐ 0x20020000
│   RAM Test Area (32KB)    │
│   (Startup test only)     │
├───────────────────────────┤ 0x20018000
│                           │
│   Application Data        │
│        (96KB)             │
│                           │
└───────────────────────────┘ 0x20000000

┌───────────────────────────┐ 0x10010000
│                           │
│   CCM RAM (64KB)          │
│   - Thread Stacks         │
│   - Error Logs            │
│   - Critical Data         │
│                           │
└───────────────────────────┘ 0x10000000
```

### 3.3 MPU 保护配置

```mermaid
graph LR
    subgraph MPU_Regions["MPU 保护区域"]
        R0["Region 0<br/>Flash (RO+X)"]
        R1["Region 1<br/>SRAM (RW+NX)"]
        R2["Region 2<br/>CCM (RW+NX)"]
        R3["Region 3<br/>Peripherals (RW+NX)"]
        R4["Region 4<br/>Stack Guard (NO ACCESS)"]
    end

    R0 -->|只读执行| FLASH_AREA[Flash]
    R1 -->|读写禁执行| SRAM_AREA[SRAM]
    R2 -->|读写禁执行| CCM_AREA[CCM RAM]
    R3 -->|设备访问| PERIPH_AREA[外设]
    R4 -->|禁止访问| GUARD_AREA[栈保护区]
```

| Region | 起始地址 | 大小 | 权限 | 用途 |
|--------|----------|------|------|------|
| 0 | 0x08010000 | 512KB | RO+X | Application Flash |
| 1 | 0x20000000 | 128KB | RW | Main RAM |
| 2 | 0x10000000 | 64KB | RW | CCM RAM (Stacks) |
| 3 | 0x40000000 | 512MB | RW+Device | Peripherals |
| 4 | 0x0800C000 | 16KB | RO | Config Flash |
| 5 | 0x08000000 | 64KB | No Access | Bootloader (Protect) |

---

## 4. 线程架构

### 4.1 线程优先级

```mermaid
graph TB
    subgraph Priority1["优先级 1 (最高)"]
        SAFETY_THREAD["Safety Monitor<br/>安全监控线程<br/>Stack: 2KB"]
    end

    subgraph Priority2["优先级 2"]
        COMM_THREAD["Communication<br/>通信线程<br/>Stack: 4KB"]
    end

    subgraph Priority3["优先级 3"]
        APP_THREAD["Application<br/>应用线程<br/>Stack: 4KB"]
    end

    subgraph Priority10["优先级 10 (最低)"]
        IDLE_THREAD["Idle<br/>空闲处理"]
    end

    SAFETY_THREAD -->|监控| COMM_THREAD
    SAFETY_THREAD -->|监控| APP_THREAD
    SAFETY_THREAD -->|喂狗| WDG[看门狗]
```

| 线程 | 优先级 | 栈大小 | 周期 | 职责 |
|------|--------|--------|------|------|
| Safety Monitor | 1 (最高) | 2KB | 100ms | 安全监控 |
| App Main | 5 | 4KB | 10ms | 主业务逻辑 |
| App Comm | 10 | 2KB | 事件驱动 | 通信处理 |

### 4.2 线程交互

```mermaid
sequenceDiagram
    participant Safety as Safety Thread
    participant App as App Thread
    participant WDG as Watchdog

    loop 每 100ms
        Safety->>Safety: 检查栈使用
        Safety->>Safety: 验证程序流
        App->>WDG: 报告令牌 (APP_TOKEN)
        Safety->>WDG: 报告令牌 (SAFETY_TOKEN)
        Safety->>WDG: 检查所有令牌
        alt 令牌有效
            Safety->>WDG: 喂狗 (IWDG + WWDG)
        else 令牌缺失
            Safety->>Safety: 进入降级模式
        end
    end
```

**线程交互 (ASCII)**:
```
┌─────────────────┐
│ Safety Monitor  │──────────────────────────────────┐
│   (Priority 1)  │                                  │
└────────┬────────┘                                  │
         │ Feed WDG                                  │
         │ Check Stacks                              │
         │ Verify Flow                               │
         ▼                                           │
┌─────────────────┐    ┌─────────────────┐          │
│   App Main      │◄───│   App Comm      │          │
│   (Priority 5)  │    │   (Priority 10) │          │
└────────┬────────┘    └────────┬────────┘          │
         │                      │                    │
         │ ReportToken()        │ ReportToken()      │
         │ Checkpoint()         │ Checkpoint()       │
         └──────────────────────┴────────────────────┘
```

---

## 5. 初始化时序

### 5.1 完整启动流程

```mermaid
sequenceDiagram
    participant HW as 硬件
    participant BOOT as Bootloader
    participant SAFE as Safety Init
    participant RTOS as ThreadX
    participant APP as Application

    HW->>BOOT: 上电复位
    activate BOOT

    BOOT->>BOOT: 1. 硬件初始化
    BOOT->>BOOT: 2. CPU 自检
    BOOT->>BOOT: 3. RAM 自检
    BOOT->>BOOT: 4. 时钟自检
    BOOT->>BOOT: 5. 读取启动配置
    BOOT->>BOOT: 6. 验证应用固件 CRC

    alt 验证通过
        BOOT->>SAFE: 跳转到应用
        deactivate BOOT
    else 验证失败
        BOOT->>BOOT: 进入恢复模式
    end

    activate SAFE
    SAFE->>SAFE: 7. Safety_EarlyInit()
    SAFE->>SAFE: 8. MPU 配置
    SAFE->>SAFE: 9. 看门狗初始化
    SAFE->>SAFE: 10. 启动自检
    SAFE->>RTOS: tx_kernel_enter()
    deactivate SAFE

    activate RTOS
    RTOS->>RTOS: 11. 内核初始化
    RTOS->>APP: 创建应用线程
    deactivate RTOS

    activate APP
    APP->>APP: 12. Safety Monitor 线程启动
    APP->>APP: 13. 应用线程启动
    APP->>APP: 14. 进入正常运行
    deactivate APP
```

### 5.2 Bootloader 启动流程

```
Reset
  │
  ▼
┌─────────────────────────────┐
│ 1. Boot_SystemInit()        │ HAL + 时钟配置
└──────────────┬──────────────┘
               ▼
┌─────────────────────────────┐
│ 2. Boot_SelfTest()          │ CPU/RAM/Flash 自检
└──────────────┬──────────────┘
               ▼
┌─────────────────────────────┐
│ 3. Boot_ValidateSafetyParams│ 安全参数校验
└──────────────┬──────────────┘
               ▼
┌─────────────────────────────┐
│ 4. Boot_VerifyAppCRC()      │ 应用程序 CRC
└──────────────┬──────────────┘
               ▼
┌─────────────────────────────┐
│ 5. Boot_JumpToApplication() │ 跳转到应用
└─────────────────────────────┘
```

### 5.3 Application 启动流程

```
Entry (0x08010000)
  │
  ▼
┌─────────────────────────────┐
│ 1. Safety_EarlyInit()       │ 早期初始化
└──────────────┬──────────────┘
               ▼
┌─────────────────────────────┐
│ 2. HAL_Init()               │ HAL 初始化
└──────────────┬──────────────┘
               ▼
┌─────────────────────────────┐
│ 3. SystemClock_Config()     │ 时钟配置
└──────────────┬──────────────┘
               ▼
┌─────────────────────────────┐
│ 4. Safety_PostClockInit()   │ 时钟后初始化
└──────────────┬──────────────┘
               ▼
┌─────────────────────────────┐
│ 5. MX_xxx_Init()            │ 外设初始化
└──────────────┬──────────────┘
               ▼
┌─────────────────────────────┐
│ 6. Safety_StartupTest()     │ 启动自检
└──────────────┬──────────────┘
               ▼
┌─────────────────────────────┐
│ 7. MX_ThreadX_Init()        │ ThreadX 启动
└──────────────┬──────────────┘
               ▼
┌─────────────────────────────┐
│ 8. App_CreateThreads()      │ 创建应用线程
└──────────────┬──────────────┘
               ▼
       Kernel Running
```

### 5.4 自检流程 / Self-Test Flow

```mermaid
flowchart TB
    START([启动]) --> CPU[CPU 寄存器测试]
    CPU -->|Pass| RAM[RAM March-C 测试]
    CPU -->|Fail| SAFE_STATE[进入安全状态]

    RAM -->|Pass| FLASH[Flash CRC 验证]
    RAM -->|Fail| SAFE_STATE

    FLASH -->|Pass| CLOCK[时钟频率检测]
    FLASH -->|Fail| SAFE_STATE

    CLOCK -->|Pass| WDG[看门狗启动]
    CLOCK -->|Fail| SAFE_STATE

    WDG --> NORMAL([正常运行])

    style SAFE_STATE fill:#f66,stroke:#333
    style NORMAL fill:#6f6,stroke:#333
```

---

## 6. 安全状态机

### 6.1 安全状态机 / Safety State Machine

```mermaid
stateDiagram-v2
    [*] --> INIT: Power On

    INIT --> STARTUP_TEST: Safety_PreKernelInit()
    INIT --> SAFE: Critical Init Failure

    STARTUP_TEST --> NORMAL: All Tests Pass
    STARTUP_TEST --> SAFE: Test Failure

    NORMAL --> DEGRADED: Non-Critical Error
    NORMAL --> SAFE: Critical Error

    DEGRADED --> NORMAL: Error Cleared
    DEGRADED --> SAFE: Timeout (30s)
    DEGRADED --> SAFE: Critical Error

    SAFE --> [*]: System Reset Required

    note right of NORMAL
        正常运行状态
        所有功能可用
    end note

    note right of DEGRADED
        降级运行状态
        限制部分功能
        超时后进入安全状态
    end note

    note right of SAFE
        安全状态
        停止所有输出
        等待复位
    end note
```

**安全状态机 (ASCII)**:
```
                    ┌───────────────┐
                    │     INIT      │
                    └───────┬───────┘
                            │ 自检开始
                            ▼
                    ┌───────────────┐
                    │ STARTUP_TEST  │
                    └───────┬───────┘
         自检失败           │ 自检通过
    ┌───────────────────────┼───────────────────────┐
    │                       ▼                       │
    │               ┌───────────────┐               │
    │               │    NORMAL     │◄──────────────┤
    │               └───────┬───────┘   恢复正常     │
    │                       │                       │
    │         非致命错误     │  致命错误              │
    │                       ▼                       │
    │               ┌───────────────┐               │
    │               │   DEGRADED    │───────────────┤
    │               └───────┬───────┘   超时         │
    │                       │ 致命错误              │
    │                       ▼                       │
    │               ┌───────────────┐               │
    └──────────────►│     SAFE      │◄──────────────┘
                    └───────────────┘
                            │
                            ▼
                    停止所有输出
                    等待看门狗复位
```

### 6.2 状态说明

| 状态 | 值 | 说明 |
|------|-----|------|
| INIT | 0x00 | 初始状态，系统启动 |
| STARTUP_TEST | 0x01 | 执行启动自检 |
| NORMAL | 0x02 | 正常运行状态 |
| DEGRADED | 0x03 | 降级运行，功能受限 |
| SAFE | 0x04 | 安全停止状态 |
| ERROR | 0xFF | 错误状态 |

### 6.3 Bootloader 状态机

```mermaid
stateDiagram-v2
    [*] --> BOOT_INIT

    BOOT_INIT --> BOOT_SELFTEST: HW Init Done
    BOOT_SELFTEST --> BOOT_VERIFY: Tests Pass
    BOOT_SELFTEST --> BOOT_FACTORY: Tests Fail

    BOOT_VERIFY --> BOOT_JUMP: App Valid
    BOOT_VERIFY --> BOOT_UPDATE: App Invalid
    BOOT_VERIFY --> BOOT_FACTORY: No App

    BOOT_UPDATE --> BOOT_VERIFY: Update Complete
    BOOT_UPDATE --> BOOT_FACTORY: Update Fail

    BOOT_JUMP --> [*]: Jump to App

    BOOT_FACTORY --> BOOT_FACTORY: Wait for Recovery
```

---

## 7. 模块依赖关系

### 7.1 安全模块架构

```mermaid
graph TB
    subgraph SafetyCore["safety_core 核心模块"]
        INIT["Safety_Init()"]
        STATE["状态管理"]
        ERROR["错误处理"]
        CALLBACK["回调机制"]
    end

    subgraph SelfTest["safety_selftest 自检模块"]
        CPU_TEST["CPU 寄存器测试"]
        RAM_TEST["RAM March-C 测试"]
        FLASH_TEST["Flash CRC 校验"]
        CLOCK_TEST["时钟频率检测"]
    end

    subgraph Watchdog["safety_watchdog 看门狗"]
        IWDG_CTRL["IWDG 控制"]
        WWDG_CTRL["WWDG 控制"]
        TOKEN["令牌机制"]
    end

    subgraph Monitor["safety_monitor 监控线程"]
        PERIODIC["周期性检查"]
        RUNTIME_TEST["运行时测试"]
    end

    subgraph Stack["safety_stack 栈监控"]
        PATTERN["水印模式填充"]
        CHECK["栈使用检测"]
    end

    subgraph Flow["safety_flow 程序流"]
        CHECKPOINT["检查点记录"]
        SIGNATURE["签名验证"]
    end

    subgraph MPUModule["safety_mpu 内存保护"]
        REGION["区域配置"]
        ACCESS["访问控制"]
    end

    SafetyCore --> SelfTest
    SafetyCore --> Watchdog
    SafetyCore --> Monitor
    SafetyCore --> Stack
    SafetyCore --> Flow
    SafetyCore --> MPUModule
```

### 7.2 模块依赖 (ASCII)

```
┌─────────────────────────────────────────────────────┐
│                     app_main                        │
└───────────────────────┬─────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────┐
│                    svc_params                       │
└───────────────────────┬─────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────┐
│    safety_core  ◄──── safety_monitor                │
│         │                   │                       │
│         ▼                   ▼                       │
│    safety_mpu         safety_watchdog               │
│                            │                        │
│    safety_flow ◄───────────┘                        │
│         │                                           │
│         ▼                                           │
│    safety_stack       safety_selftest               │
└───────────────────────┬─────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────┐
│                   safety_config                     │
│                        │                            │
│                        ▼                            │
│                   shared_config                     │
└─────────────────────────────────────────────────────┘
```

### 7.3 双看门狗架构

```mermaid
graph TB
    subgraph Threads["应用线程"]
        T1["Safety Thread"]
        T2["App Thread"]
        T3["Comm Thread"]
    end

    subgraph TokenMgr["令牌管理器"]
        TOKENS["令牌收集<br/>Token Collector"]
        CHECK["令牌验证<br/>Token Validator"]
    end

    subgraph Watchdogs["看门狗"]
        IWDG["IWDG<br/>超时: ~4s<br/>LSI 时钟"]
        WWDG["WWDG<br/>超时: ~50ms<br/>PCLK1 时钟"]
    end

    T1 -->|TOKEN_SAFETY| TOKENS
    T2 -->|TOKEN_APP| TOKENS
    T3 -->|TOKEN_COMM| TOKENS

    TOKENS --> CHECK

    CHECK -->|所有令牌有效| IWDG
    CHECK -->|所有令牌有效| WWDG
    CHECK -->|令牌缺失| DEGRADED[降级模式]

    IWDG -->|超时| RESET1[系统复位]
    WWDG -->|超时| RESET2[系统复位]
```

### 7.4 诊断覆盖率

```mermaid
pie title 诊断覆盖率分布 / Diagnostic Coverage
    "CPU 测试" : 15
    "RAM 测试" : 20
    "Flash CRC" : 15
    "看门狗" : 20
    "栈监控" : 10
    "程序流" : 10
    "时钟监控" : 10
```

---

## 8. 设计决策

### 8.1 分离 Bootloader 和 Application

**理由**:
- Bootloader 不变性：一旦烧录，通常不更新
- 安全参数独立：在专用 Flash 扇区存储
- 应用升级支持：可单独更新应用程序

### 8.2 基于令牌的看门狗

**理由**:
- 多线程环境下，单一喂狗点不足以验证所有线程正常
- 每个关键线程必须报告令牌，全部收齐才喂狗
- 任一线程卡死都会导致看门狗复位

### 8.3 增量式 Flash CRC

**理由**:
- 全量 CRC 耗时过长，影响实时性
- 每次检查 4KB，5分钟完成全量验证
- 平衡安全性和性能

### 8.4 CCM RAM 存储线程栈

**理由**:
- CCM RAM 仅 CPU 可访问，更安全
- DMA 无法访问 CCM，避免意外覆盖
- 栈溢出不会破坏应用数据

### 8.5 参数冗余存储

**理由**:
- 关键参数存储两份 (原值 + 位反转)
- 单比特翻转可检测
- 符合 IEC 61508 要求

---

## 9. CI/CD 流程

### 9.1 完整 CI/CD 流程

```mermaid
flowchart TB
    subgraph Trigger["触发器"]
        PUSH["Git Push"]
        PR["Pull Request"]
        TAG["Git Tag"]
        MANUAL["手动触发"]
    end

    subgraph CI["CI Pipeline"]
        VERSION["生成版本<br/>generate_version.ps1"]
        BUILD_APP["构建应用<br/>build.ps1"]
        BUILD_BOOT["构建 Bootloader<br/>build_bootloader.ps1"]
        CSTAT["C-STAT 分析<br/>cstat_analyze.ps1"]
        ARTIFACTS["上传产物"]
    end

    subgraph Checks["质量门禁"]
        SIZE_CHECK["大小检查<br/>App < 448KB<br/>Boot < 48KB"]
        MISRA_CHECK["MISRA 检查<br/>High = 0"]
    end

    subgraph Release["发布"]
        PACKAGE["打包固件"]
        GH_RELEASE["GitHub Release"]
    end

    PUSH --> VERSION
    PR --> VERSION
    MANUAL --> VERSION

    VERSION --> BUILD_APP
    VERSION --> BUILD_BOOT

    BUILD_APP --> CSTAT
    BUILD_BOOT --> CSTAT

    BUILD_APP --> SIZE_CHECK
    BUILD_BOOT --> SIZE_CHECK

    CSTAT --> MISRA_CHECK

    SIZE_CHECK --> ARTIFACTS
    MISRA_CHECK --> ARTIFACTS

    TAG --> PACKAGE
    ARTIFACTS --> PACKAGE
    PACKAGE --> GH_RELEASE
```

### 9.2 本地开发流程

```mermaid
flowchart LR
    subgraph Local["本地开发"]
        CODE["编写代码"]
        IAR_BUILD["IAR 编译 (F7)"]
        CRUN["C-RUN 调试"]
        FIX["修复问题"]
    end

    subgraph Git["Git 操作"]
        ADD["git add"]
        COMMIT["git commit"]
        HOOK["pre-commit hook"]
        PUSH["git push"]
    end

    CODE --> IAR_BUILD
    IAR_BUILD -->|编译错误| FIX
    IAR_BUILD -->|成功| CRUN
    CRUN -->|运行时错误| FIX
    CRUN -->|通过| ADD
    FIX --> CODE

    ADD --> COMMIT
    COMMIT --> HOOK
    HOOK -->|检查失败| FIX
    HOOK -->|通过| PUSH
```

### 9.3 数据流

```mermaid
flowchart LR
    subgraph External["外部"]
        HOST["上位机"]
    end

    subgraph MCU["MCU"]
        COMM["通信模块"]
        SVC["svc_params"]
        VALID["safety_params<br/>验证"]
        RAM_CACHE["RAM 缓存"]
    end

    subgraph Storage["存储"]
        FLASH_INT["内部 Flash"]
        FLASH_EXT["W25Q128<br/>外部 Flash"]
    end

    HOST -->|"写入请求"| COMM
    COMM -->|"原始数据"| SVC
    SVC -->|"验证"| VALID
    VALID -->|"有效参数"| RAM_CACHE
    RAM_CACHE -->|"持久化"| FLASH_INT
    RAM_CACHE -->|"备份"| FLASH_EXT

    FLASH_INT -->|"启动加载"| RAM_CACHE
```

---

## 附录: Mermaid 使用说明

### 查看方式

1. **GitHub** - 直接在 GitHub 仓库中查看，自动渲染
2. **VS Code** - 安装 "Markdown Preview Mermaid Support" 扩展
3. **在线编辑器** - https://mermaid.live/

### 本地预览

```bash
# 安装 mermaid-cli
npm install -g @mermaid-js/mermaid-cli

# 生成 PNG
mmdc -i ARCHITECTURE.md -o output.png

# 生成 SVG
mmdc -i ARCHITECTURE.md -o output.svg -f svg
```

---

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2024-12-10 | 初始版本，整合 Mermaid 图表 |

---

*本文档使用 Mermaid.js 绘制，符合 IEC 61508 SIL 2 文档要求*
