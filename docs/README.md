# STM32F407 功能安全框架

**项目 / Project**: TKX_ThreadX
**版本 / Version**: 1.0.1
**合规标准 / Compliance**: IEC 61508 SIL 2 / ISO 13849 PL d

---

基于 STM32F407VGT6 + Azure RTOS ThreadX 的功能安全框架实现。

## 概述

本项目实现了符合 IEC 61508 SIL 2 / ISO 13849 PL d 要求的功能安全框架，包含：

- **安全启动引导程序 (Bootloader)**: 启动自检、参数验证、应用程序完整性校验
- **运行时安全监控**: 多线程看门狗、栈监控、程序流监控
- **轻量级自检机制**: 增量式 Flash CRC 校验
- **MPU 内存保护**: 防止非法内存访问

## 硬件平台

| 项目 | 规格 |
|------|------|
| MCU | STM32F407VGT6 (Cortex-M4, 168MHz) |
| Flash | 1MB (内部) |
| RAM | 192KB (128KB SRAM + 64KB CCM) |
| 外设 | CAN, UART, SPI, I2C, ADC |

## 软件环境

| 项目 | 版本 |
|------|------|
| IDE | IAR EWARM V8.50+ |
| RTOS | Azure RTOS ThreadX 6.1.10 |
| HAL | STM32Cube HAL |
| 代码生成 | STM32CubeMX |

## 目录结构

```
TKX_ThreadX/
├── App/                    # 应用层代码
│   ├── Inc/               # 头文件
│   └── Src/               # 源文件
├── Bootloader/            # 安全启动引导程序
│   ├── Core/              # Bootloader 核心代码
│   ├── Drivers/           # HAL 驱动
│   └── EWARM/             # IAR 工程文件
├── Core/                  # CubeMX 生成的核心代码
├── Drivers/               # STM32 HAL 驱动
├── EWARM/                 # 应用程序 IAR 工程文件
├── FATFS/                 # 文件系统 (可选)
├── Middlewares/           # 中间件 (ThreadX, FatFs)
├── Safety/                # 功能安全模块
│   ├── Inc/               # 安全模块头文件
│   └── Src/               # 安全模块实现
├── Services/              # 服务层
│   ├── Inc/               # 服务层头文件
│   └── Src/               # 服务层实现
├── Shared/                # Bootloader/App 共享配置
│   └── Inc/               # 共享头文件
└── docs/                  # 文档
```

## 内存布局

| 区域 | 起始地址 | 大小 | 用途 |
|------|----------|------|------|
| Bootloader | 0x08000000 | 48KB | 安全引导程序 |
| Config | 0x0800C000 | 16KB | 配置/校准参数 |
| Application | 0x08010000 | 448KB | 主应用程序 |
| SRAM | 0x20000000 | 128KB | 运行时数据 |
| CCM RAM | 0x10000000 | 64KB | 线程栈/关键数据 |

## 安全机制

### 启动时检测
- CPU 寄存器测试
- RAM March-C 测试
- Flash CRC32 校验
- 时钟频率验证

### 运行时监控
- 基于令牌的多线程看门狗
- 线程栈使用率监控
- 程序流签名验证
- 增量式 Flash CRC 校验

### 保护机制
- MPU 内存保护 (6 个区域)
- 参数冗余校验 (位反转)
- 安全/降级/安全停止状态机

## 快速开始

### 1. 环境准备

1. 安装 IAR EWARM V8.50+
2. 安装 STM32CubeMX (可选，用于重新生成代码)
3. 克隆本仓库

### 2. 编译 Bootloader

1. 打开 `Bootloader/EWARM/Bootloader.eww`
2. 选择 `Release` 配置
3. 编译项目 (F7)

### 3. 编译应用程序

1. 打开 `EWARM/TKX_ThreadX.eww`
2. 选择 `Debug` 或 `Release` 配置
3. 编译项目 (F7)

### 4. 烧录顺序

1. 首先烧录 Bootloader (地址 0x08000000)
2. 然后烧录应用程序 (地址 0x08010000)
3. 可选：烧录校准参数 (地址 0x0800C000)

## 相关文档

- [架构设计](ARCHITECTURE.md) - 整体架构与设计决策
- [Bootloader 设计](BOOTLOADER.md) - 安全引导程序详解
- [安全模块](SAFETY_MODULES.md) - 安全模块 API 参考
- [服务层](SERVICES.md) - 参数服务等
- [应用层](APP_LAYER.md) - 应用开发指南

## 功能安全标准

本框架设计符合以下标准要求：

- **IEC 61508**: 安全完整性等级 SIL 2
- **ISO 13849**: 性能等级 PL d

诊断覆盖率目标：
- 启动自检: DC > 99%
- 运行时监控: DC > 90%

## 许可证

MIT License
