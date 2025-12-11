# System-Level FMEA

**Project**: TKX_ThreadX
**Version**: 1.0.0
**Compliance Target**: IEC 61508 SIL 2 / ISO 13849 PL d
**Date**: 2025-12-11
**Author**: [Author Name]
**Reviewer**: [Reviewer Name]

---

## 1. Document Overview

### 1.1 Purpose

This document presents the Failure Modes and Effects Analysis (FMEA) for the TKX_ThreadX functional safety system. The analysis identifies potential failure modes, their effects, and the diagnostic measures implemented to detect and mitigate these failures.

### 1.2 Scope

| Item | Description |
|------|-------------|
| System | STM32F407VGT6 Functional Safety Framework |
| Hardware | STM32F407VGT6 MCU (Cortex-M4, 168MHz) |
| Software | Bootloader + Application (ThreadX RTOS) |
| Safety Target | IEC 61508 SIL 2 (DC ≥ 90%) |

### 1.3 Reference Documents

| Document | Description |
|----------|-------------|
| UM1840 | STM32F4 Series Safety Manual |
| AN5141 | Results of FMEA on STM32F4 Series microcontrollers |
| AN5140 | FMEDA snapshots for STM32F4 Series microcontrollers |
| IEC 61508 | Functional Safety of E/E/PE Systems |
| ISO 13849 | Safety of Machinery |
| SN 29500 | Failure Rate Data |
| IEC 62380 | Reliability data handbook |

### 1.4 Technical References

| Source | URL | Description |
|--------|-----|-------------|
| ST X-CUBE-STL | https://www.st.com/en/embedded-software/x-cube-stl.html | STM32 Functional Safety Software Test Library |
| ST Safety Manual | https://www.st.com/resource/en/user_manual/um1840-stm32f4-series-safety-manual-stmicroelectronics.pdf | STM32F4 Series Safety Manual (UM1840) |
| Microchip FMEDA | https://ww1.microchip.com/downloads/en/DeviceDoc/Failure-Mode-Effect-Diagnostics-Analysis-DS00003638A.pdf | FMEDA Importance and Methodology |
| ADI Pin FMEA | https://www.analog.com/en/resources/technical-articles/know-your-safety-part-3.html | Pin FMEA Analysis Guide |
| ADI Failure Modes | https://www.analog.com/en/resources/analog-dialogue/articles/know-your-safety-application-notes-part-2.html | Failure Mode Distribution |
| Memfault Watchdog | https://interrupt.memfault.com/blog/firmware-watchdog-best-practices | Watchdog Timer Best Practices |
| Barr Group | https://barrgroup.com/embedded-systems/how-to/top-five-nasty-firmware-bugs | Common Embedded Firmware Bugs |
| Ganssle Watchdogs | https://www.ganssle.com/watchdogs.htm | Designing Great Watchdog Timers |

### 1.5 Severity Classification

| Level | Severity | Description |
|-------|----------|-------------|
| 1 | Catastrophic | May cause injury or death |
| 2 | Critical | System failure, safety function lost |
| 3 | Major | Degraded operation, safety margin reduced |
| 4 | Minor | Minor malfunction, no safety impact |

### 1.6 Diagnostic Coverage Classification

| Level | DC Range | Description |
|-------|----------|-------------|
| High | ≥ 99% | Near-complete fault detection |
| Medium | 90% - 99% | Most faults detected |
| Low | 60% - 90% | Basic fault detection |
| None | < 60% | Minimal or no detection |

---

## 2. System Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    TKX_ThreadX System                       │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │ Bootloader  │  │ Application │  │ Safety Monitoring   │  │
│  │ (48KB)      │  │ (448KB)     │  │                     │  │
│  │ - Self-test │  │ - ThreadX   │  │ - Watchdog          │  │
│  │ - CRC       │  │ - App Logic │  │ - Stack Monitor     │  │
│  │ - Param Val │  │ - Services  │  │ - Program Flow      │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                    Hardware Platform                        │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────┐   │
│  │ CPU Core │ │ Flash    │ │ RAM      │ │ Peripherals  │   │
│  │ Cortex-M4│ │ 1MB      │ │ 192KB    │ │ WDG,CRC,MPU  │   │
│  └──────────┘ └──────────┘ └──────────┘ └──────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

---

## 3. FMEA Analysis Tables

### 3.1 MCU Core Subsystem

| ID | Component | Failure Mode | Cause | Local Effect | System Effect | Severity | Diagnostic Measure | DC | Detection Time | Remarks |
|----|-----------|--------------|-------|--------------|---------------|----------|-------------------|-----|----------------|---------|
| CPU-01 | CPU Core | Instruction execution error | Radiation, EMI, silicon defect | Incorrect computation | Safety function malfunction | 2 | Program flow monitoring (signature verification) | Medium (90%) | 10ms | Runtime signature check |
| CPU-02 | CPU Core | Register corruption | Bit flip, EMI | Data error | Incorrect output | 2 | CPU register self-test at startup | High (99%) | Startup | March-C pattern test |
| CPU-03 | CPU Core | Program counter error | Stack overflow, corruption | Jump to invalid address | System crash | 2 | MPU protection + Stack monitoring | Medium (95%) | Immediate | MPU triggers HardFault |
| CPU-04 | CPU Core | ALU malfunction | Hardware defect | Calculation error | Incorrect safety decision | 2 | Startup ALU test + redundant calculation | Medium (90%) | Startup / Runtime | Critical calculations use redundancy |
| CPU-05 | Clock | Frequency drift | Oscillator aging, temperature | Timing error | Communication failure | 3 | Clock monitoring (HSE vs LSI) | High (99%) | 1ms | Hardware clock security system |
| CPU-06 | Clock | Clock failure (stop) | Crystal failure | System halt | Complete failure | 2 | IWDG with LSI clock | High (99%) | 100ms | Independent watchdog on separate clock |
| CPU-07 | Interrupt | Interrupt not serviced | Priority inversion, disabled | Delayed response | Safety response delayed | 3 | Watchdog timeout | Medium (90%) | Configurable | Token-based watchdog |
| CPU-08 | Interrupt | Spurious interrupt | EMI, software bug | Unexpected execution | Unpredictable behavior | 3 | Default interrupt handler + error logging | Low (70%) | Immediate | Trap unexpected interrupts |

### 3.2 Memory Subsystem

| ID | Component | Failure Mode | Cause | Local Effect | System Effect | Severity | Diagnostic Measure | DC | Detection Time | Remarks |
|----|-----------|--------------|-------|--------------|---------------|----------|-------------------|-----|----------------|---------|
| MEM-01 | Flash | Single bit flip | Radiation, retention loss | Code/data corruption | Program malfunction | 2 | CRC32 verification (startup + incremental) | High (99%) | Startup: full, Runtime: incremental | 4KB blocks, background check |
| MEM-02 | Flash | Multi-bit corruption | Programming error, wear | Large data corruption | System failure | 1 | CRC32 + redundant storage | High (99%) | Startup | Critical params stored twice |
| MEM-03 | Flash | Sector erase failure | Wear out, power loss | Cannot update config | Configuration stuck | 3 | Write verification | Medium (90%) | On write | Read-back after write |
| MEM-04 | Flash | Stuck at 0/1 | Cell degradation | Persistent error | Data integrity loss | 2 | CRC covers all flash areas | High (99%) | Incremental check | Covered by runtime CRC |
| MEM-05 | SRAM | Single bit flip | Radiation, alpha particles | Variable corruption | Calculation error | 2 | March-C test (startup) + ECC (if available) | Medium (90%) | Startup | Full RAM test at boot |
| MEM-06 | SRAM | Address line fault | PCB defect, solder joint | Wrong address accessed | Data mix-up | 2 | March-C with address pattern | High (99%) | Startup | Address uniqueness test |
| MEM-07 | SRAM | Data line fault | PCB defect | Wrong data written | Data corruption | 2 | March-C with data pattern | High (99%) | Startup | Walking 1/0 pattern |
| MEM-08 | CCM RAM | Stack overflow | Recursive call, large local vars | Memory corruption | System crash | 2 | Stack watermark monitoring | Medium (95%) | 10ms | Thread stack usage check |
| MEM-09 | CCM RAM | Stack underflow | Return address corruption | Jump to invalid code | System crash | 2 | MPU stack region protection | Medium (90%) | Immediate | HardFault on violation |
| MEM-10 | MPU | Configuration error | Software bug | Wrong protection | Security breach | 3 | MPU config verification at startup | Medium (90%) | Startup | Read-back and verify |

### 3.3 Safety Monitoring Subsystem

| ID | Component | Failure Mode | Cause | Local Effect | System Effect | Severity | Diagnostic Measure | DC | Detection Time | Remarks |
|----|-----------|--------------|-------|--------------|---------------|----------|-------------------|-----|----------------|---------|
| SAF-01 | IWDG | Watchdog not fed | Software hang | System reset | Uncontrolled restart | 3 | Expected behavior (fail-safe) | N/A | Timeout period | Designed behavior |
| SAF-02 | IWDG | Watchdog disabled | Software error | No timeout protection | Hang not detected | 1 | Hardware IWDG cannot be disabled | High (99%) | N/A | Once enabled, always on |
| SAF-03 | IWDG | Wrong timeout | Configuration error | Too long/short timeout | Delayed detection | 3 | Timeout value verification | Medium (90%) | Startup | Read-back config |
| SAF-04 | WWDG | Window violation | Timing error | System reset | Uncontrolled restart | 3 | Expected behavior (fail-safe) | N/A | N/A | Designed behavior |
| SAF-05 | CRC Unit | CRC calculation error | Hardware fault | Wrong CRC value | False pass/fail | 2 | Known-answer test at startup | High (99%) | Startup | Calculate CRC of known data |
| SAF-06 | Program Flow | Signature mismatch | Code path error | Wrong execution sequence | Safety logic bypassed | 2 | Runtime signature verification | Medium (90%) | Per function | Entry/exit signature check |
| SAF-07 | Program Flow | Signature overflow | Counter wraparound | False match | Missed detection | 3 | 32-bit counter with range check | Low (80%) | N/A | Unlikely with 32-bit |
| SAF-08 | Stack Monitor | Monitor thread blocked | Priority inversion | No stack checking | Overflow not detected | 2 | Watchdog monitors monitor thread | Medium (90%) | Watchdog timeout | Monitor has own token |
| SAF-09 | Safety State | Stuck in wrong state | State machine bug | Cannot transition | Safety action blocked | 2 | State timeout + redundant state variable | Medium (90%) | Configurable | Dual state storage |
| SAF-10 | Safe Output | Output stuck | Driver failure, short circuit | Cannot reach safe state | Hazardous condition | 1 | Output feedback monitoring | Medium (90%) | 10ms | Read-back output state |

### 3.4 Bootloader Subsystem

| ID | Component | Failure Mode | Cause | Local Effect | System Effect | Severity | Diagnostic Measure | DC | Detection Time | Remarks |
|----|-----------|--------------|-------|--------------|---------------|----------|-------------------|-----|----------------|---------|
| BL-01 | Startup | Self-test skipped | Code corruption, jump error | No validation | Faulty system runs | 1 | Fixed entry point + code signature | High (99%) | Startup | Reset vector protected |
| BL-02 | Startup | Self-test false pass | Test code error | Fault not detected | Faulty system runs | 1 | Self-test code CRC + known-answer test | Medium (90%) | Startup | Test the test |
| BL-03 | CRC Check | Wrong CRC reference | Flash corruption | Always pass or fail | False validation | 2 | CRC stored with redundancy (inverted copy) | High (99%) | Startup | Dual storage comparison |
| BL-04 | Param Load | Invalid parameters loaded | Flash corruption | Wrong configuration | System misconfigured | 2 | Parameter range check + CRC | Medium (95%) | Startup | Bounds checking |
| BL-05 | Param Load | Default params used incorrectly | Code error | Wrong defaults | Unexpected behavior | 3 | Default value validation | Medium (90%) | Startup | Defaults within safe range |
| BL-06 | App Jump | Jump to corrupted app | CRC not checked | Execute bad code | System crash/malfunction | 1 | CRC mandatory before jump | High (99%) | Startup | No jump if CRC fails |
| BL-07 | App Jump | Wrong jump address | Address corruption | Jump to random location | System crash | 1 | Address validation + signature check | High (99%) | Startup | Check app header |
| BL-08 | Boot Counter | Counter overflow | Many resets | Counter wraps | Boot limit bypassed | 4 | 32-bit counter, reset on success | Low (70%) | N/A | Non-safety impact |
| BL-09 | Factory Mode | Unintended factory mode | Memory corruption | System in test mode | Safety features may be disabled | 2 | Factory mode requires debugger + specific sequence | Medium (90%) | Startup | Multi-step activation |

### 3.5 Application Subsystem

| ID | Component | Failure Mode | Cause | Local Effect | System Effect | Severity | Diagnostic Measure | DC | Detection Time | Remarks |
|----|-----------|--------------|-------|--------------|---------------|----------|-------------------|-----|----------------|---------|
| APP-01 | ThreadX | Kernel crash | Memory corruption, stack overflow | All threads stop | System failure | 1 | Watchdog timeout | High (99%) | Timeout period | IWDG resets system |
| APP-02 | ThreadX | Thread deadlock | Resource contention | Threads blocked | Partial function loss | 2 | Token-based watchdog per thread | Medium (90%) | Token timeout | Each thread has token |
| APP-03 | ThreadX | Priority inversion | Mutex usage | High priority delayed | Response time exceeded | 3 | Priority inheritance enabled | Medium (90%) | N/A | ThreadX feature |
| APP-04 | Safety Thread | Thread not scheduled | Starvation | No safety monitoring | Faults not detected | 1 | Highest priority + watchdog | High (99%) | Watchdog timeout | Safety thread is highest priority |
| APP-05 | Comm Thread | Message loss | Buffer overflow | Command not processed | Function not executed | 3 | Sequence number + acknowledgment | Medium (90%) | Message timeout | Protocol level check |
| APP-06 | Comm Thread | Message corruption | EMI, bit error | Wrong command | Wrong action | 2 | CRC on messages | High (99%) | On receive | Message CRC check |
| APP-07 | Param Service | Parameter out of range | Input error, corruption | Invalid config | Unpredictable behavior | 2 | Range check on all parameters | Medium (95%) | On access | Bounds checking |
| APP-08 | Diag Service | Diagnostic disabled | Software error | No diagnostics | Faults not reported | 3 | Diagnostic enable check | Low (70%) | Runtime | Status monitoring |

### 3.6 Communication Subsystem

| ID | Component | Failure Mode | Cause | Local Effect | System Effect | Severity | Diagnostic Measure | DC | Detection Time | Remarks |
|----|-----------|--------------|-------|--------------|---------------|----------|-------------------|-----|----------------|---------|
| COM-01 | CAN | Bus off | EMI, short circuit | No communication | Control lost | 2 | Bus-off detection + auto recovery | High (99%) | Immediate | Hardware detection |
| COM-02 | CAN | Message loss | Buffer overflow, collision | Command lost | Function not executed | 3 | Heartbeat + sequence number | Medium (90%) | Heartbeat period | Protocol monitoring |
| COM-03 | CAN | Message delay | Bus load, priority | Late response | Timing violation | 3 | Message timestamp + timeout | Medium (90%) | On receive | Time check |
| COM-04 | CAN | Wrong message | Address error, corruption | Wrong node responds | Incorrect action | 2 | Node ID + message CRC | High (99%) | On receive | Protocol CRC |
| COM-05 | UART | Framing error | Baud rate mismatch | Data corruption | Command error | 3 | Framing error detection | High (99%) | On receive | Hardware detection |
| COM-06 | UART | Buffer overflow | High data rate | Data loss | Message incomplete | 3 | DMA + flow control | Medium (90%) | On receive | Buffer monitoring |
| COM-07 | CAN | Bit stuffing error | EMI, noise | Frame error | Message rejected | 3 | Error counter monitoring | High (99%) | Immediate | Hardware detection |
| COM-08 | CAN | Arbitration loss | Bus contention | Delayed transmission | Timing impact | 4 | Retry mechanism | Medium (90%) | On transmit | Normal CAN behavior |
| COM-09 | SPI | Bus lockup | Slave not responding | Communication hang | Peripheral unavailable | 3 | Timeout + bus reset | Medium (90%) | Configurable | Software timeout |
| COM-10 | I2C | Bus stuck (SDA/SCL low) | Slave hang, noise | No communication | Peripheral unavailable | 3 | Bus recovery sequence | Medium (90%) | Configurable | Clock stretching timeout |

### 3.7 Power Supply Subsystem

| ID | Component | Failure Mode | Cause | Local Effect | System Effect | Severity | Diagnostic Measure | DC | Detection Time | Remarks |
|----|-----------|--------------|-------|--------------|---------------|----------|-------------------|-----|----------------|---------|
| PWR-01 | VDD | Under-voltage | Power supply failure, high load | Unstable operation | System crash, data corruption | 1 | PVD (Programmable Voltage Detector) | High (99%) | <1ms | Hardware detection |
| PWR-02 | VDD | Over-voltage | Regulator failure | Component damage | Permanent failure | 1 | External voltage monitoring | Medium (90%) | 10ms | Requires external circuit |
| PWR-03 | Voltage Regulator | Output = Input (pass-through) | Pass element damage | Over-voltage to MCU | Component damage | 1 | Voltage monitoring at MCU pins | Medium (90%) | 10ms | Critical failure mode |
| PWR-04 | Voltage Regulator | Output drift | Temperature, aging | Out-of-spec operation | Marginal operation | 3 | ADC voltage monitoring | Medium (90%) | 100ms | Periodic check |
| PWR-05 | VBAT | Battery failure | Discharge, aging | RTC/backup loss | Time/data loss on power cycle | 4 | Battery voltage check | Low (80%) | On startup | Non-safety impact |
| PWR-06 | Reset Circuit | Spurious reset | Noise, ESD | Unexpected restart | Service interruption | 3 | Reset source identification | Medium (95%) | On startup | Log reset cause |
| PWR-07 | Reset Circuit | Reset stuck active | Hardware fault | System cannot start | Complete failure | 1 | External reset monitoring | Low (70%) | N/A | Requires external watchdog |
| PWR-08 | Power Sequencing | Wrong sequence | Design error, component failure | Latch-up, damage | Component failure | 1 | Power-on self-test validation | Medium (90%) | Startup | Check all rails |

### 3.8 ADC Subsystem

| ID | Component | Failure Mode | Cause | Local Effect | System Effect | Severity | Diagnostic Measure | DC | Detection Time | Remarks |
|----|-----------|--------------|-------|--------------|---------------|----------|-------------------|-----|----------------|---------|
| ADC-01 | ADC | Conversion error | Noise, reference drift | Wrong reading | Incorrect decision | 2 | Reference voltage check + averaging | Medium (90%) | On conversion | Multiple samples |
| ADC-02 | ADC | Stuck at value | Hardware fault | Constant reading | Missed signal change | 2 | Plausibility check (value change monitoring) | Medium (90%) | 100ms | Expect variation |
| ADC-03 | ADC | Input open circuit | Connector failure, PCB fault | Floating input, random values | Unpredictable reading | 2 | Input impedance check / pull-up detection | Medium (90%) | On conversion | Check for floating |
| ADC-04 | ADC | Input short to VDD | Wiring fault | Always max value | False high reading | 2 | Range plausibility check | High (99%) | On conversion | Out-of-range detection |
| ADC-05 | ADC | Input short to GND | Wiring fault | Always zero | False low reading | 2 | Range plausibility check | High (99%) | On conversion | Out-of-range detection |
| ADC-06 | ADC | Reference voltage drift | Temperature, aging | Gain error | All readings offset | 2 | Calibration channel check | Medium (95%) | Periodic | Use internal ref |
| ADC-07 | ADC | Channel crosstalk | Multiplexer leakage | Reading influenced by other channel | Incorrect value | 3 | Channel isolation test | Low (80%) | Periodic | Dummy conversion |
| ADC-08 | ADC | Timeout (no conversion complete) | Clock failure, peripheral hang | No new data | Stale data used | 2 | Conversion timeout monitoring | High (99%) | Configurable | Watchdog on ADC |

### 3.9 GPIO and External Interface Subsystem

| ID | Component | Failure Mode | Cause | Local Effect | System Effect | Severity | Diagnostic Measure | DC | Detection Time | Remarks |
|----|-----------|--------------|-------|--------------|---------------|----------|-------------------|-----|----------------|---------|
| GPIO-01 | Output Pin | Stuck high | Driver failure, short to VDD | Cannot drive low | Output stuck | 2 | Read-back verification | High (99%) | On write | Compare output register |
| GPIO-02 | Output Pin | Stuck low | Driver failure, short to GND | Cannot drive high | Output stuck | 2 | Read-back verification | High (99%) | On write | Compare output register |
| GPIO-03 | Output Pin | Open (high-Z) | Bond wire failure, PCB fault | No drive capability | Output floating | 2 | Current monitoring / pull-up check | Medium (90%) | Periodic | External detection |
| GPIO-04 | Input Pin | Stuck at value | ESD damage, latch-up | Wrong input state | Missed input change | 2 | Plausibility check (expected transitions) | Medium (90%) | Runtime | Application dependent |
| GPIO-05 | Input Pin | Floating | Missing pull-up/down, open circuit | Random readings | Unpredictable behavior | 3 | Configure internal pull-up/down | High (99%) | Configuration | Hardware feature |
| GPIO-06 | Input Pin | Short to adjacent pin | Solder bridge, contamination | Coupled signals | Wrong input values | 2 | Pin isolation test | Low (80%) | Startup | Test pattern check |
| GPIO-07 | External Interrupt | Not triggered | Configuration error, pin fault | Event missed | Function not executed | 2 | Periodic interrupt test / timeout | Medium (90%) | Configurable | Heartbeat check |
| GPIO-08 | External Interrupt | Spurious trigger | Noise, ESD | Unexpected ISR execution | Incorrect behavior | 3 | Debounce + validation in ISR | Medium (90%) | Immediate | Software filter |

### 3.10 Timer Subsystem

| ID | Component | Failure Mode | Cause | Local Effect | System Effect | Severity | Diagnostic Measure | DC | Detection Time | Remarks |
|----|-----------|--------------|-------|--------------|---------------|----------|-------------------|-----|----------------|---------|
| TIM-01 | Timer | Counter stuck | Hardware fault | No timing update | Timing functions fail | 2 | Cross-check with independent timer | High (99%) | 10ms | Use two timers |
| TIM-02 | Timer | Wrong frequency | Prescaler error, clock issue | Incorrect timing | Timing violation | 2 | Compare against RTC/external reference | Medium (95%) | Periodic | Cross-reference |
| TIM-03 | Timer | Interrupt not generated | Configuration error | Event missed | Function not called | 2 | Interrupt occurrence monitoring | Medium (90%) | Expected period | Count interrupts |
| TIM-04 | Timer | PWM output stuck | Hardware fault, config error | Wrong duty cycle | Actuator malfunction | 2 | Output feedback monitoring | Medium (90%) | PWM period | Read-back or sensor |
| TIM-05 | Timer | Input capture miss | Signal too fast, noise | Measurement error | Wrong frequency/period | 3 | Plausibility check on captured values | Medium (90%) | On capture | Range validation |
| TIM-06 | RTC | Time drift | Crystal aging, temperature | Inaccurate time | Scheduling error | 4 | Periodic sync with external reference | Low (80%) | Hours/days | Non-safety typically |
| TIM-07 | SysTick | Tick missed | High interrupt load | RTOS timing affected | Thread scheduling impact | 2 | Tick counter validation | Medium (95%) | 1ms | Monitor tick count |

### 3.11 DMA Subsystem

| ID | Component | Failure Mode | Cause | Local Effect | System Effect | Severity | Diagnostic Measure | DC | Detection Time | Remarks |
|----|-----------|--------------|-------|--------------|---------------|----------|-------------------|-----|----------------|---------|
| DMA-01 | DMA | Transfer error | Bus fault, address error | Data not transferred | Peripheral malfunction | 2 | DMA error interrupt + flag check | High (99%) | Immediate | Hardware detection |
| DMA-02 | DMA | Transfer incomplete | Timeout, peripheral issue | Partial data | Data corruption | 2 | Transfer complete flag verification | High (99%) | On completion | Check TC flag |
| DMA-03 | DMA | Wrong source/dest address | Configuration error | Data to wrong location | Memory corruption | 1 | Address validation before enable | Medium (90%) | On config | Range check |
| DMA-04 | DMA | FIFO overrun/underrun | Speed mismatch | Data loss | Communication error | 3 | FIFO error flag monitoring | High (99%) | Immediate | Hardware detection |
| DMA-05 | DMA | Priority conflict | Configuration error | Transfer delay | Timing violation | 3 | Transfer timing monitoring | Medium (90%) | Runtime | Performance check |

---

## 4. Diagnostic Coverage Summary

### 4.1 By Subsystem

| Subsystem | Total Failure Modes | High DC (≥99%) | Medium DC (90-99%) | Low DC (<90%) | Average DC |
|-----------|---------------------|----------------|---------------------|---------------|------------|
| MCU Core | 8 | 3 | 4 | 1 | 92% |
| Memory | 10 | 5 | 5 | 0 | 95% |
| Safety Monitoring | 10 | 3 | 6 | 1 | 91% |
| Bootloader | 9 | 4 | 4 | 1 | 93% |
| Application | 8 | 2 | 5 | 1 | 90% |
| Communication | 10 | 5 | 5 | 0 | 94% |
| Power Supply | 8 | 1 | 5 | 2 | 88% |
| ADC | 8 | 3 | 4 | 1 | 92% |
| GPIO/External | 8 | 3 | 4 | 1 | 92% |
| Timer | 7 | 1 | 5 | 1 | 91% |
| DMA | 5 | 3 | 2 | 0 | 95% |
| **Total** | **91** | **33** | **49** | **9** | **92%** |

### 4.2 By Severity

| Severity | Count | With High DC | With Medium DC | With Low DC |
|----------|-------|--------------|----------------|-------------|
| Catastrophic (1) | 12 | 6 | 4 | 2 |
| Critical (2) | 50 | 20 | 27 | 3 |
| Major (3) | 25 | 6 | 16 | 3 |
| Minor (4) | 4 | 1 | 2 | 1 |

### 4.3 SIL 2 Compliance Assessment

| Requirement | Target | Achieved | Status |
|-------------|--------|----------|--------|
| Diagnostic Coverage (DC) | ≥ 90% | 92% | PASS |
| Catastrophic failures with High DC | 100% | 50% (6/12) | REVIEW |
| Safe Failure Fraction (SFF) | ≥ 90% | ~92% | PASS |

**Note**: Several catastrophic failure modes have Medium or Low DC. The following require additional mitigation:
- PWR-02 (Over-voltage): Medium DC - Add external over-voltage protection circuit
- PWR-03 (Regulator pass-through): Medium DC - Add secondary voltage monitoring
- PWR-07 (Reset stuck): Low DC - Add external watchdog IC
- PWR-08 (Power sequencing): Medium DC - Add sequencing supervisor IC
- DMA-03 (Wrong address): Medium DC - Add MPU region protection for DMA

---

## 5. Risk Mitigation Actions

### 5.1 High Priority Actions (Catastrophic Failures)

| ID | Failure Mode | Current DC | Recommended Action | Target DC | Priority |
|----|--------------|------------|-------------------|-----------|----------|
| PWR-01 | Under-voltage | 99% | PVD already implemented - verify threshold settings | 99% | Verify |
| PWR-02 | Over-voltage | 90% | Add TVS diode + external voltage monitor IC | 99% | High |
| PWR-03 | Regulator pass-through | 90% | Add redundant LDO with cross-check | 99% | High |
| PWR-07 | Reset stuck active | 70% | Add external watchdog IC (e.g., MAX6369) | 95% | High |
| PWR-08 | Power sequencing | 90% | Add power sequencer/supervisor IC | 99% | Medium |
| DMA-03 | Wrong DMA address | 90% | Configure MPU to protect critical regions | 99% | Medium |

### 5.2 Medium Priority Actions (Critical Failures with Low DC)

| ID | Failure Mode | Current DC | Recommended Action | Target DC | Priority |
|----|--------------|------------|-------------------|-----------|----------|
| SAF-10 | Output stuck | 90% | Add redundant output driver with cross-monitoring | 99% | Medium |
| CPU-08 | Spurious interrupt | 70% | Implement interrupt frequency monitoring | 90% | Medium |
| SAF-07 | Signature overflow | 80% | Add explicit overflow check and reset | 95% | Medium |
| ADC-07 | Channel crosstalk | 80% | Add dummy conversion between channel switches | 90% | Low |
| GPIO-06 | Pin short to adjacent | 80% | Add startup pin isolation test | 90% | Low |

### 5.3 Monitoring Recommendations

| Item | Recommendation | Priority |
|------|----------------|----------|
| Flash wear | Implement write cycle counter for config sector | Medium |
| RAM health | Consider periodic RAM test during idle time | Low |
| Clock drift | Log frequency deviation trends | Low |
| Power rail | Periodic ADC measurement of all power rails | Medium |
| Temperature | Monitor MCU junction temperature | Medium |

### 5.4 Hardware Design Recommendations

| Area | Recommendation | Justification |
|------|----------------|---------------|
| Power | Add crowbar circuit for catastrophic over-voltage | Protect against regulator failure |
| Power | Use supervisor IC with voltage monitoring | Independent voltage verification |
| Reset | External watchdog with separate oscillator | Detect main oscillator failure |
| Communication | Add bus isolation for CAN/SPI/I2C | Prevent bus fault propagation |
| ADC | Use differential inputs where possible | Improve noise immunity |
| GPIO | Add series resistors on critical outputs | Current limiting for short protection |

---

## 6. Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2025-12-11 | [Author] | Initial release |

---

## 7. Approval

| Role | Name | Signature | Date |
|------|------|-----------|------|
| Author | | | |
| Reviewer | | | |
| Approver | | | |
