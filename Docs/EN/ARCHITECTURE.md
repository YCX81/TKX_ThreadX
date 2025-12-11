# Architecture Design

**Project**: TKX_ThreadX
**Compliance**: IEC 61508 SIL 2 / ISO 13849 PL d
**Version**: 1.0.1

---

## Table of Contents

1. [System Architecture Overview](#1-system-architecture-overview)
2. [Layer Responsibilities](#2-layer-responsibilities)
3. [Memory Layout Details](#3-memory-layout-details)
4. [Thread Architecture](#4-thread-architecture)
5. [Initialization Sequence](#5-initialization-sequence)
6. [Safety State Machine](#6-safety-state-machine)
7. [Module Dependencies](#7-module-dependencies)
8. [Design Decisions](#8-design-decisions)
9. [CI/CD Workflow](#9-cicd-workflow)

---

## 1. System Architecture Overview

### 1.1 Hardware Architecture

```mermaid
graph TB
    subgraph MCU["STM32F407VGT6"]
        CPU["ARM Cortex-M4<br/>168 MHz + FPU"]
        FLASH["Flash 1MB"]
        RAM["SRAM 192KB"]
        CCM["CCM 64KB"]
    end

    subgraph Peripherals["Peripherals"]
        IWDG["IWDG<br/>Independent Watchdog"]
        WWDG["WWDG<br/>Window Watchdog"]
        CRC["CRC Unit"]
        MPU["MPU<br/>Memory Protection"]
        SPI["SPI1/SPI2"]
        UART["USART1/2/3"]
        GPIO["GPIO"]
    end

    subgraph External["External Devices"]
        W25Q["W25Q128<br/>SPI Flash"]
        RTT["Segger RTT<br/>Debug Output"]
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

### 1.2 System Component Relations

```mermaid
graph LR
    subgraph Bootloader["Bootloader (48KB)"]
        BOOT_SELF[Self-Test Module]
        BOOT_VERIFY[Firmware Verification]
        BOOT_JUMP[Jump Logic]
    end

    subgraph Application["Application (448KB)"]
        APP_SAFETY[Safety Framework]
        APP_RTOS[ThreadX RTOS]
        APP_LOGIC[Business Logic]
    end

    subgraph SharedParams["Shared Parameters (16KB)"]
        BOOT_CFG[boot_config]
        SAFETY_PARAM[safety_params]
    end

    BOOT_SELF --> BOOT_VERIFY
    BOOT_VERIFY --> BOOT_JUMP
    BOOT_JUMP -->|Verification Passed| APP_SAFETY
    SharedParams <-->|Read/Write| Bootloader
    SharedParams <-->|Read Only| Application
```

---

## 2. Layer Responsibilities

### 2.1 Six-Layer Architecture

```mermaid
graph TB
    subgraph L6["Layer 6: Application Layer"]
        APP["app_main.c<br/>Business Logic"]
    end

    subgraph L5["Layer 5: Services Layer"]
        SVC_PARAM["svc_params<br/>Parameter Service"]
        SVC_COMM["svc_comm<br/>Communication Service"]
    end

    subgraph L4["Layer 4: Safety Layer"]
        SAFE_CORE["safety_core"]
        SAFE_MON["safety_monitor"]
        SAFE_WDG["safety_watchdog"]
        SAFE_TEST["safety_selftest"]
        SAFE_STACK["safety_stack"]
        SAFE_FLOW["safety_flow"]
        SAFE_MPU["safety_mpu"]
    end

    subgraph L3["Layer 3: RTOS Layer"]
        TX["ThreadX RTOS"]
        FX["FileX File System"]
    end

    subgraph L2["Layer 2: BSP/HAL Layer"]
        BSP["BSP Drivers"]
        HAL["STM32 HAL"]
    end

    subgraph L1["Layer 1: Hardware Layer"]
        HW["STM32F407VGT6"]
    end

    L6 --> L5
    L5 --> L4
    L4 --> L3
    L3 --> L2
    L2 --> L1

    style L4 fill:#f96,stroke:#333,stroke-width:2px
```

### 2.2 Layer Responsibilities

| Layer | Name | Responsibility |
|------|------|------|
| **Hardware Layer** | Hardware | STM32F407VGT6 MCU, Peripherals: CRC, IWDG, WWDG, MPU, Flash, GPIO |
| **HAL Layer** | Driver | STM32Cube HAL drivers, Hardware abstraction, Unified API |
| **RTOS Layer** | System | Azure RTOS ThreadX 6.1.10, Multi-thread scheduling, Synchronization primitives, Memory management |
| **Safety Layer** | Safety | Functional safety core modules, Self-test, Monitoring, Protection mechanisms |
| **Services Layer** | Services | Parameter management service, Diagnostic service |
| **Application Layer** | Application | Business logic implementation, Application threads |

---

## 3. Memory Layout Details

### 3.1 Flash Partitions

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

### 3.2 RAM Partitions

```mermaid
graph TB
    subgraph RAM["SRAM (192KB)"]
        direction TB
        STACK["Stack"]
        HEAP["Heap (Disabled)"]
        BSS["BSS Segment"]
        DATA["Data Segment"]
        TX_POOL["ThreadX<br/>Byte Pool"]
    end

    subgraph CCM["CCM RAM (64KB)"]
        direction TB
        SAFETY_CTX["Safety Context"]
        ERROR_LOG["Error Log"]
        CRITICAL["Critical Data"]
    end

    style CCM fill:#f96,stroke:#333,stroke-width:2px
```

### 3.3 MPU Protection Configuration

```mermaid
graph LR
    subgraph MPU_Regions["MPU Protection Regions"]
        R0["Region 0<br/>Flash (RO+X)"]
        R1["Region 1<br/>SRAM (RW+NX)"]
        R2["Region 2<br/>CCM (RW+NX)"]
        R3["Region 3<br/>Peripherals (RW+NX)"]
        R4["Region 4<br/>Stack Guard (NO ACCESS)"]
    end

    R0 -->|Read-Only Execute| FLASH_AREA[Flash]
    R1 -->|Read-Write No-Execute| SRAM_AREA[SRAM]
    R2 -->|Read-Write No-Execute| CCM_AREA[CCM RAM]
    R3 -->|Device Access| PERIPH_AREA[Peripherals]
    R4 -->|No Access| GUARD_AREA[Stack Guard]
```

| Region | Start Address | Size | Permissions | Purpose |
|--------|----------|------|------|------|
| 0 | 0x08010000 | 512KB | RO+X | Application Flash |
| 1 | 0x20000000 | 128KB | RW | Main RAM |
| 2 | 0x10000000 | 64KB | RW | CCM RAM (Stacks) |
| 3 | 0x40000000 | 512MB | RW+Device | Peripherals |
| 4 | 0x0800C000 | 16KB | RO | Config Flash |
| 5 | 0x08000000 | 64KB | No Access | Bootloader (Protect) |

---

## 4. Thread Architecture

### 4.1 Thread Priorities

```mermaid
graph TB
    subgraph Priority1["Priority 1 (Highest)"]
        SAFETY_THREAD["Safety Monitor<br/>Safety Monitoring Thread<br/>Stack: 2KB"]
    end

    subgraph Priority2["Priority 2"]
        COMM_THREAD["Communication<br/>Communication Thread<br/>Stack: 4KB"]
    end

    subgraph Priority3["Priority 3"]
        APP_THREAD["Application<br/>Application Thread<br/>Stack: 4KB"]
    end

    subgraph Priority10["Priority 10 (Lowest)"]
        IDLE_THREAD["Idle<br/>Idle Handler"]
    end

    SAFETY_THREAD -->|Monitor| COMM_THREAD
    SAFETY_THREAD -->|Monitor| APP_THREAD
    SAFETY_THREAD -->|Feed| WDG[Watchdog]
```

| Thread | Priority | Stack Size | Period | Responsibility |
|------|--------|--------|------|------|
| Safety Monitor | 1 (Highest) | 2KB | 100ms | Safety monitoring |
| App Main | 5 | 4KB | 10ms | Main business logic |
| App Comm | 10 | 2KB | Event-driven | Communication handling |

### 4.2 Thread Interaction

```mermaid
sequenceDiagram
    participant Safety as Safety Thread
    participant App as App Thread
    participant WDG as Watchdog

    loop Every 100ms
        Safety->>Safety: Check stack usage
        Safety->>Safety: Verify program flow
        App->>WDG: Report token (APP_TOKEN)
        Safety->>WDG: Report token (SAFETY_TOKEN)
        Safety->>WDG: Check all tokens
        alt Tokens valid
            Safety->>WDG: Feed watchdog (IWDG + WWDG)
        else Token missing
            Safety->>Safety: Enter degraded mode
        end
    end
```

---

## 5. Initialization Sequence

### 5.1 Complete Startup Flow

```mermaid
sequenceDiagram
    participant HW as Hardware
    participant BOOT as Bootloader
    participant SAFE as Safety Init
    participant RTOS as ThreadX
    participant APP as Application

    HW->>BOOT: Power-On Reset
    activate BOOT

    BOOT->>BOOT: 1. Hardware initialization
    BOOT->>BOOT: 2. CPU self-test
    BOOT->>BOOT: 3. RAM self-test
    BOOT->>BOOT: 4. Clock self-test
    BOOT->>BOOT: 5. Read boot configuration
    BOOT->>BOOT: 6. Verify application firmware CRC

    alt Verification passed
        BOOT->>SAFE: Jump to application
        deactivate BOOT
    else Verification failed
        BOOT->>BOOT: Enter recovery mode
    end

    activate SAFE
    SAFE->>SAFE: 7. Safety_EarlyInit()
    SAFE->>SAFE: 8. MPU configuration
    SAFE->>SAFE: 9. Watchdog initialization
    SAFE->>SAFE: 10. Startup self-test
    SAFE->>RTOS: tx_kernel_enter()
    deactivate SAFE

    activate RTOS
    RTOS->>RTOS: 11. Kernel initialization
    RTOS->>APP: Create application threads
    deactivate RTOS

    activate APP
    APP->>APP: 12. Safety Monitor thread start
    APP->>APP: 13. Application thread start
    APP->>APP: 14. Enter normal operation
    deactivate APP
```

### 5.2 Self-Test Flow

```mermaid
flowchart TB
    START([Startup]) --> CPU[CPU Register Test]
    CPU -->|Pass| RAM[RAM March-C Test]
    CPU -->|Fail| SAFE_STATE[Enter Safe State]

    RAM -->|Pass| FLASH[Flash CRC Verification]
    RAM -->|Fail| SAFE_STATE

    FLASH -->|Pass| CLOCK[Clock Frequency Detection]
    FLASH -->|Fail| SAFE_STATE

    CLOCK -->|Pass| WDG[Watchdog Startup]
    CLOCK -->|Fail| SAFE_STATE

    WDG --> NORMAL([Normal Operation])

    style SAFE_STATE fill:#f66,stroke:#333
    style NORMAL fill:#6f6,stroke:#333
```

---

## 6. Safety State Machine

### 6.1 Safety State Machine

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
        Normal Operation State
        All functions available
    end note

    note right of DEGRADED
        Degraded Operation State
        Limited functionality
        Enter safe state after timeout
    end note

    note right of SAFE
        Safe State
        Stop all outputs
        Wait for reset
    end note
```

### 6.2 State Descriptions

| State | Value | Description |
|------|-----|------|
| INIT | 0x00 | Initial state, system startup |
| STARTUP_TEST | 0x01 | Executing startup self-test |
| NORMAL | 0x02 | Normal operation state |
| DEGRADED | 0x03 | Degraded operation, limited functionality |
| SAFE | 0x04 | Safe stop state |
| ERROR | 0xFF | Error state |

### 6.3 Bootloader State Machine

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

## 7. Module Dependencies

### 7.1 Safety Module Architecture

```mermaid
graph TB
    subgraph SafetyCore["safety_core Core Module"]
        INIT["Safety_Init()"]
        STATE["State Management"]
        ERROR["Error Handling"]
        CALLBACK["Callback Mechanism"]
    end

    subgraph SelfTest["safety_selftest Self-Test Module"]
        CPU_TEST["CPU Register Test"]
        RAM_TEST["RAM March-C Test"]
        FLASH_TEST["Flash CRC Verification"]
        CLOCK_TEST["Clock Frequency Detection"]
    end

    subgraph Watchdog["safety_watchdog Watchdog"]
        IWDG_CTRL["IWDG Control"]
        WWDG_CTRL["WWDG Control"]
        TOKEN["Token Mechanism"]
    end

    subgraph Monitor["safety_monitor Monitor Thread"]
        PERIODIC["Periodic Check"]
        RUNTIME_TEST["Runtime Test"]
    end

    subgraph Stack["safety_stack Stack Monitor"]
        PATTERN["Watermark Pattern Fill"]
        CHECK["Stack Usage Detection"]
    end

    subgraph Flow["safety_flow Program Flow"]
        CHECKPOINT["Checkpoint Recording"]
        SIGNATURE["Signature Verification"]
    end

    subgraph MPUModule["safety_mpu Memory Protection"]
        REGION["Region Configuration"]
        ACCESS["Access Control"]
    end

    SafetyCore --> SelfTest
    SafetyCore --> Watchdog
    SafetyCore --> Monitor
    SafetyCore --> Stack
    SafetyCore --> Flow
    SafetyCore --> MPUModule
```

### 7.2 Dual Watchdog Architecture

```mermaid
graph TB
    subgraph Threads["Application Threads"]
        T1["Safety Thread"]
        T2["App Thread"]
        T3["Comm Thread"]
    end

    subgraph TokenMgr["Token Manager"]
        TOKENS["Token Collector"]
        CHECK["Token Validator"]
    end

    subgraph Watchdogs["Watchdogs"]
        IWDG["IWDG<br/>Timeout: ~4s<br/>LSI Clock"]
        WWDG["WWDG<br/>Timeout: ~50ms<br/>PCLK1 Clock"]
    end

    T1 -->|TOKEN_SAFETY| TOKENS
    T2 -->|TOKEN_APP| TOKENS
    T3 -->|TOKEN_COMM| TOKENS

    TOKENS --> CHECK

    CHECK -->|All tokens valid| IWDG
    CHECK -->|All tokens valid| WWDG
    CHECK -->|Token missing| DEGRADED[Degraded Mode]

    IWDG -->|Timeout| RESET1[System Reset]
    WWDG -->|Timeout| RESET2[System Reset]
```

### 7.4 Diagnostic Coverage

```mermaid
pie title Diagnostic Coverage Distribution
    "CPU Test" : 15
    "RAM Test" : 20
    "Flash CRC" : 15
    "Watchdog" : 20
    "Stack Monitor" : 10
    "Program Flow" : 10
    "Clock Monitor" : 10
```

---

## 8. Design Decisions

### 8.1 Separate Bootloader and Application

**Rationale**:
- Bootloader immutability: Once programmed, typically not updated
- Independent safety parameters: Stored in dedicated Flash sector
- Application upgrade support: Can update application independently

### 8.2 Token-Based Watchdog

**Rationale**:
- In multi-threaded environment, single watchdog feed point insufficient to verify all threads are running
- Each critical thread must report a token, watchdog only fed when all tokens collected
- Any thread deadlock will cause watchdog reset

### 8.3 Incremental Flash CRC

**Rationale**:
- Full CRC verification takes too long, affecting real-time performance
- Each check verifies 4KB, completing full verification in 5 minutes
- Balance between safety and performance

### 8.4 CCM RAM for Thread Stacks

**Rationale**:
- CCM RAM accessible only by CPU, more secure
- DMA cannot access CCM, avoiding accidental overwrites
- Stack overflow won't corrupt application data

### 8.5 Redundant Parameter Storage

**Rationale**:
- Critical parameters stored twice (original + bit-inverted)
- Single bit flip can be detected
- Complies with IEC 61508 requirements

---

## 9. CI/CD Workflow

### 9.1 Complete CI/CD Pipeline

```mermaid
flowchart TB
    subgraph Trigger["Triggers"]
        PUSH["Git Push"]
        PR["Pull Request"]
        TAG["Git Tag"]
        MANUAL["Manual Trigger"]
    end

    subgraph CI["CI Pipeline"]
        VERSION["Generate Version<br/>generate_version.ps1"]
        BUILD_APP["Build Application<br/>build.ps1"]
        BUILD_BOOT["Build Bootloader<br/>build_bootloader.ps1"]
        CSTAT["C-STAT Analysis<br/>cstat_analyze.ps1"]
        ARTIFACTS["Upload Artifacts"]
    end

    subgraph Checks["Quality Gates"]
        SIZE_CHECK["Size Check<br/>App < 448KB<br/>Boot < 48KB"]
        MISRA_CHECK["MISRA Check<br/>High = 0"]
    end

    subgraph Release["Release"]
        PACKAGE["Package Firmware"]
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

### 9.2 Local Development Workflow

```mermaid
flowchart LR
    subgraph Local["Local Development"]
        CODE["Write Code"]
        IAR_BUILD["IAR Build (F7)"]
        CRUN["C-RUN Debug"]
        FIX["Fix Issues"]
    end

    subgraph Git["Git Operations"]
        ADD["git add"]
        COMMIT["git commit"]
        HOOK["pre-commit hook"]
        PUSH["git push"]
    end

    CODE --> IAR_BUILD
    IAR_BUILD -->|Build Error| FIX
    IAR_BUILD -->|Success| CRUN
    CRUN -->|Runtime Error| FIX
    CRUN -->|Pass| ADD
    FIX --> CODE

    ADD --> COMMIT
    COMMIT --> HOOK
    HOOK -->|Check Failed| FIX
    HOOK -->|Pass| PUSH
```

### 9.3 Data Flow

```mermaid
flowchart LR
    subgraph External["External"]
        HOST["Host PC"]
    end

    subgraph MCU["MCU"]
        COMM["Communication Module"]
        SVC["svc_params"]
        VALID["safety_params<br/>Validation"]
        RAM_CACHE["RAM Cache"]
    end

    subgraph Storage["Storage"]
        FLASH_INT["Internal Flash"]
        FLASH_EXT["W25Q128<br/>External Flash"]
    end

    HOST -->|"Write Request"| COMM
    COMM -->|"Raw Data"| SVC
    SVC -->|"Validation"| VALID
    VALID -->|"Valid Parameters"| RAM_CACHE
    RAM_CACHE -->|"Persist"| FLASH_INT
    RAM_CACHE -->|"Backup"| FLASH_EXT

    FLASH_INT -->|"Boot Load"| RAM_CACHE
```

---

## Appendix: Mermaid Usage Guide

### Viewing Options

1. **GitHub** - View directly in GitHub repository, automatically rendered
2. **VS Code** - Install "Markdown Preview Mermaid Support" extension
3. **Online Editor** - https://mermaid.live/

### Local Preview

```bash
# Install mermaid-cli
npm install -g @mermaid-js/mermaid-cli

# Generate PNG
mmdc -i ARCHITECTURE.md -o output.png

# Generate SVG
mmdc -i ARCHITECTURE.md -o output.svg -f svg
```

---

## Version History

| Version | Date | Description |
|------|------|------|
| 1.0.0 | 2025-12-10 | Initial version (ASCII diagrams) |
| 1.0.1 | 2025-12-10 | Integrated Mermaid diagrams, removed redundant files |
