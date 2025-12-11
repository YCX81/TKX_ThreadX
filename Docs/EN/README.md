# STM32F407 Functional Safety Framework

**Project**: TKX_ThreadX
**Version**: 1.0.1
**Compliance**: IEC 61508 SIL 2 / ISO 13849 PL d

---

Functional safety framework implementation based on STM32F407VGT6 + Azure RTOS ThreadX.

## Overview

This project implements a functional safety framework compliant with IEC 61508 SIL 2 / ISO 13849 PL d requirements, including:

- **Safe Boot Bootloader**: Startup self-test, parameter validation, application integrity verification
- **Runtime Safety Monitoring**: Multi-threaded watchdog, stack monitoring, program flow monitoring
- **Lightweight Self-Test Mechanism**: Incremental Flash CRC verification
- **MPU Memory Protection**: Prevention of illegal memory access

## Hardware Platform

| Item | Specification |
|------|---------------|
| MCU | STM32F407VGT6 (Cortex-M4, 168MHz) |
| Flash | 1MB (internal) |
| RAM | 192KB (128KB SRAM + 64KB CCM) |
| Peripherals | CAN, UART, SPI, I2C, ADC |

## Software Environment

| Item | Version |
|------|---------|
| IDE | IAR EWARM V8.50+ |
| RTOS | Azure RTOS ThreadX 6.1.10 |
| HAL | STM32Cube HAL |
| Code Generation | STM32CubeMX |

## Directory Structure

```
TKX_ThreadX/
├── App/                    # Application layer code
│   ├── Inc/               # Header files
│   └── Src/               # Source files
├── Bootloader/            # Safe boot bootloader
│   ├── Core/              # Bootloader core code
│   ├── Drivers/           # HAL drivers
│   └── EWARM/             # IAR project files
├── Core/                  # CubeMX generated core code
├── Drivers/               # STM32 HAL drivers
├── EWARM/                 # Application IAR project files
├── FATFS/                 # File system (optional)
├── Middlewares/           # Middlewares (ThreadX, FatFs)
├── Safety/                # Functional safety modules
│   ├── Inc/               # Safety module header files
│   └── Src/               # Safety module implementation
├── Services/              # Service layer
│   ├── Inc/               # Service layer header files
│   └── Src/               # Service layer implementation
├── Shared/                # Bootloader/App shared configuration
│   └── Inc/               # Shared header files
├── Docs/                  # Documentation
└── CI/                    # CI/CD scripts
```

## Memory Layout

| Region | Start Address | Size | Purpose |
|--------|---------------|------|---------|
| Bootloader | 0x08000000 | 48KB | Safe boot program |
| Config | 0x0800C000 | 16KB | Configuration/calibration parameters |
| Application | 0x08010000 | 448KB | Main application |
| SRAM | 0x20000000 | 128KB | Runtime data |
| CCM RAM | 0x10000000 | 64KB | Thread stacks/critical data |

## Safety Mechanisms

### Startup Detection
- CPU register test
- RAM March-C test
- Flash CRC32 verification
- Clock frequency validation

### Runtime Monitoring
- Token-based multi-threaded watchdog
- Thread stack usage monitoring
- Program flow signature verification
- Incremental Flash CRC verification

### Protection Mechanisms
- MPU memory protection (6 regions)
- Parameter redundancy verification (bit inversion)
- Safe/degraded/safe-stop state machine

## Quick Start

### 1. Environment Setup

1. Install IAR EWARM V8.50+
2. Install STM32CubeMX (optional, for code regeneration)
3. Clone this repository

### 2. Build Bootloader

1. Open `Bootloader/EWARM/Bootloader.eww`
2. Select `Release` configuration
3. Build project (F7)

### 3. Build Application

1. Open `EWARM/TKX_ThreadX.eww`
2. Select `Debug` or `Release` configuration
3. Build project (F7)

### 4. Flash Sequence

1. First flash Bootloader (address 0x08000000)
2. Then flash application (address 0x08010000)
3. Optional: Flash calibration parameters (address 0x0800C000)

## Documentation Navigation

| Document | Description | Target Audience |
|----------|-------------|-----------------|
| [Architecture Design](ARCHITECTURE.md) | System architecture, memory layout, state machine | Architects, Developers |
| [Bootloader](BOOTLOADER.md) | Safe boot bootloader design and implementation | Low-level Developers |
| [Safety Modules](SAFETY_MODULES.md) | Safety module API and usage guide | Safety Developers |
| [Service Layer](SERVICES.md) | Parameter service, diagnostic service | Application Developers |
| [Application Layer](APP_LAYER.md) | Application thread development guide | Application Developers |
| [Coding Standard](CODING_STANDARD.md) | C code coding standard | All Developers |
| [CI/CD Workflow](CI_CD_WORKFLOW.md) | Continuous integration and automated build | DevOps |
| [Safety Checklist](SAFETY_CHECKLIST.md) | Pre-release safety checklist | Testing, Release |
| [Documentation Style Guide](DOC_STYLE_GUIDE.md) | Documentation writing style guide | Documentation Writers |

## Functional Safety Standards

This framework is designed to meet the following standard requirements:

- **IEC 61508**: Safety Integrity Level SIL 2
- **ISO 13849**: Performance Level PL d

Diagnostic Coverage targets:
- Startup self-test: DC > 99%
- Runtime monitoring: DC > 90%

## License

MIT License
