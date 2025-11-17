# PTX Oven Controller - System Design Document

## 1. System Overview

PTX Oven Controller is a commercial oven control system with advanced safety features and high-level automation.

### Key Features
- ✅ Hysteresis temperature control (175°C - 185°C)
- ✅ Multi-layer safety (door, sensor faults, ignition retry)
- ✅ Median filter for sensor noise reduction
- ✅ Ignition safety with retry and lockout
- ✅ Runtime configurable parameters
- ✅ Comprehensive logging

---

## 2. System Architecture

```mermaid
graph TB
    subgraph "Hardware Layer"
        H1[Temperature Sensor<br/>Analog Input]
        H2[Door Switch<br/>Digital Input]
        H3[Gas Valve<br/>Digital Output]
        H4[Igniter<br/>Digital Output]
    end
    
    subgraph "HAL - api.h/cpp"
        API[Hardware Abstraction<br/>read_voltage<br/>read_input<br/>set_output<br/>read_output]
    end
    
    subgraph "Sensor Processing"
        FILTER[Median Filter<br/>ptx_sensor_filter]
        FAULT[Fault Detection<br/>Timed Latching]
    end
    
    subgraph "Control Logic"
        SM[State Machine<br/>3 States: IDLE, IGNITING, HEATING]
        CONFIG[Runtime Config<br/>ptx_oven_config]
    end
    
    subgraph "Output Control"
        ACT[Actuator Wrapper<br/>ptx_actuator]
    end
    
    subgraph "Logging"
        LOG[PTX Logging<br/>Timestamped]
    end
    
    H1 --> API
    H2 --> API
    API --> FILTER
    FILTER --> FAULT
    FAULT --> SM
    CONFIG --> SM
    SM --> ACT
    ACT --> API
    API --> H3
    API --> H4
    SM --> LOG
    FAULT --> LOG
```

---

## 3. State Machine Diagram

**Note:** This diagram shows the system with **flame detection disabled** (default mode).

```mermaid
stateDiagram-v2
    [*] --> IDLE: Init / Boot
    
    IDLE --> IGNITING: temp ≤ 175°C<br/>AND no faults<br/>AND uptime > 2s
    
    IGNITING --> HEATING: 5s elapsed<br/>(assume ignition success)
    
    HEATING --> IDLE: temp ≥ 185°C
    
    note right of IGNITING
        Flame Detection = OFF (default)
        Ignition always assumed successful
        after 5 second timer
    end note
    
    IDLE --> IDLE: Door open / Sensor fault<br/>→ shutdown
    IGNITING --> IDLE: Door open / Sensor fault<br/>→ shutdown
    HEATING --> IDLE: Door open / Sensor fault<br/>→ shutdown
```

---

## 4. Control Flow Diagram

```mermaid
flowchart TD
    START([ptx_oven_control_update])
    
    READ[Read & Filter Sensors<br/>vref, signal]
    EVAL[Evaluate Faults<br/>with timing window]
    DOOR[Check Door State]
    TEMP[Compute Temperature]
    
    FAULT_CHECK{Door open OR<br/>Sensor fault?}
    SHUTDOWN[Shutdown:<br/>gas=OFF, ign=OFF<br/>state=IDLE]
    
    TIME_CHECK{uptime < 2s?}
    WAIT[Keep OFF<br/>sensor stabilization]
    
    STATE[State Machine Logic]
    APPLY[Apply Outputs]
    LOG[Periodic Logging]
    
    END([Return])
    
    START --> READ
    READ --> EVAL
    EVAL --> DOOR
    DOOR --> TEMP
    TEMP --> FAULT_CHECK
    
    FAULT_CHECK -->|YES| SHUTDOWN
    SHUTDOWN --> APPLY
    
    FAULT_CHECK -->|NO| TIME_CHECK
    TIME_CHECK -->|YES| WAIT
    WAIT --> APPLY
    
    TIME_CHECK -->|NO| STATE
    STATE --> APPLY
    APPLY --> LOG
    LOG --> END
```

---

## 5. Component Architecture

```mermaid
classDiagram
    class ptx_oven_control {
        +ptx_oven_control_init()
        +ptx_oven_control_update()
        +ptx_oven_get_status()
        +ptx_oven_set_door_state()
        +ptx_oven_reset_ignition_lockout()
        -ptx_update_heating()
        -ptx_eval_sensor_faults_with_timing()
        -ptx_compute_temperature()
    }
    
    class ptx_sensor_filter {
        +ptx_sensor_filter_init(window_size)
        +ptx_sensor_filter_read_and_update()
        -median_filter_vref[]
        -median_filter_signal[]
    }
    
    class ptx_actuator {
        +ptx_actuator_init()
        +ptx_actuator_set_gas(on)
        +ptx_actuator_set_igniter(on)
        +ptx_actuator_get_gas_state()
        +ptx_actuator_get_igniter_state()
    }
    
    class ptx_oven_config {
        +ptx_oven_get_config()
        +ptx_oven_set_config()
        +ptx_oven_reset_config_to_defaults()
        +Individual getters/setters
        -ptx_oven_config_t
    }
    
    class api {
        +read_voltage(pin)
        +read_input(pin)
        +set_output(pin, value)
        +read_output(pin)
        +millis()
    }
    
    class ptx_logging {
        +PTX_LOG(msg)
        +PTX_LOGF(format, ...)
        +ptx_log_init()
    }
    
    ptx_oven_control --> ptx_sensor_filter
    ptx_oven_control --> ptx_actuator
    ptx_oven_control --> ptx_oven_config
    ptx_oven_control --> ptx_logging
    ptx_sensor_filter --> api
    ptx_actuator --> api
```

---

## 6. Sensor Fault Detection Timing

```mermaid
sequenceDiagram
    participant Sensor
    participant Filter
    participant Fault Logic
    participant Control
    
    Note over Sensor: Normal readings
    Sensor->>Filter: vref=5000mV, signal=1500mV
    Filter->>Fault Logic: filtered values
    Fault Logic->>Control: vref_fault=0, signal_fault=0, sensor_fault=0
    
    Note over Sensor: Fault occurs
    Sensor->>Filter: vref=5000mV, signal=0mV
    Filter->>Fault Logic: filtered values
    Note over Fault Logic: Start timer<br/>pti_out_of_range_since_ms = now
    Fault Logic->>Control: vref_fault=0, signal_fault=1, sensor_fault=0
    
    Note over Fault Logic: Wait 1000ms...
    
    Sensor->>Filter: vref=5000mV, signal=0mV (still bad)
    Filter->>Fault Logic: filtered values
    Note over Fault Logic: Elapsed > 1000ms<br/>LATCH FAULT
    Fault Logic->>Control: sensor_fault=1 (latched)<br/>→ SHUTDOWN
    
    Note over Sensor: Readings restore
    Sensor->>Filter: vref=5000mV, signal=1500mV
    Filter->>Fault Logic: filtered values
    Note over Fault Logic: Start valid timer<br/>pti_valid_since_ms = now
    
    Note over Fault Logic: Wait 3000ms...
    
    Sensor->>Filter: vref=5000mV, signal=1500mV (continuous valid)
    Filter->>Fault Logic: filtered values
    Note over Fault Logic: Elapsed ≥ 3000ms<br/>CLEAR FAULT
    Fault Logic->>Control: sensor_fault=0<br/>→ AUTO-RESUME
```

---

## 7. Configuration Parameters

| Parameter | Type | Default | Range | Description |
|-----------|------|---------|-------|-------------|
| temp_target_c | float | 180.0 | 0-300 | Target temperature (°C) |
| temp_delta_c | float | 2.0 | 0.1-50 | Hysteresis half-band (°C) |
| ignition_duration_ms | uint32 | 5000 | 1000-30000 | Igniter ON time (ms) |
| max_ignition_attempts | uint8 | 3 | 1-10 | Max retry before lockout |
| purge_time_ms | uint32 | 2500 | 1000-10000 | Gas purge after fail (ms) |
| flame_detect_temp_rise_c | float | 2.0 | 0-50 | Temp rise for flame detect (°C) |
| sensor_fault_window_ms | uint32 | 1000 | 100-10000 | Fault latch delay (ms) |
| auto_resume_delay_ms | uint32 | 3000 | 1000-30000 | Valid readings before resume (ms) |
| vref_min_v | float | 4.5 | 0-10 | Min vref voltage (V) |
| vref_max_v | float | 5.5 | 0-10 | Max vref voltage (V) |
| periodic_log_ms | uint32 | 1000 | 100-60000 | Log interval (ms) |

---

## 8. Safety Features

### 8.1 Multi-Layer Fault Detection

```mermaid
graph TB
    INPUT[Sensor Readings]
    
    F1{Vref<br/>4.5-5.5V?}
    F2{Signal<br/>10-90% vref?}
    F3{Persist<br/>> 1s?}
    F4{Door<br/>Closed?}
    
    LATCH[Latch sensor_fault]
    IMMEDIATE[Immediate Shutdown]
    HEATING[Continue Heating]
    
    INPUT --> F1
    INPUT --> F2
    INPUT --> F4
    
    F1 -->|NO| F3
    F2 -->|NO| F3
    F3 -->|YES| LATCH
    LATCH --> IMMEDIATE
    
    F4 -->|NO| IMMEDIATE
    
    F1 -->|YES| F2
    F2 -->|YES| F4
    F4 -->|YES| HEATING
```

### 8.2 Ignition Safety Chain

**Current Configuration: Flame Detection = OFF (Default)**

1. **Pre-ignition checks:**
   - ✅ No door open
   - ✅ No sensor faults
   - ✅ System uptime > 2s (sensor stabilized)
   - ✅ Temperature ≤ 175°C

2. **During ignition (5s):**
   - Monitor door state → immediate shutdown if opened
   - Monitor sensor faults → immediate shutdown if detected

3. **Post-ignition:**
   - After 5 seconds → assume ignition successful
   - Igniter turns OFF
   - Continue heating with gas ON
   
**Note:** When `PTX_FLAME_DETECT_ENABLED` is set to 1, the system will check for temperature rise to confirm flame presence and implement retry/purge/lockout logic.

---

## 9. Testing Architecture

```mermaid
graph TB
    subgraph "Production Code"
        CTRL[ptx_oven_control.cpp]
        SENS[ptx_sensor_filter.cpp]
        ACT[ptx_actuator.cpp]
        CFG[ptx_oven_config.cpp]
    end
    
    subgraph "Test Mocks"
        MOCK_API[mock_api.cpp<br/>Simulated HW]
        MOCK_LOG[mock_logging.cpp<br/>Silent logging]
    end
    
    subgraph "Test Framework"
        GTEST[Google Test<br/>8 test cases]
        CMAKE[CMake Build<br/>CTest Runner]
    end
    
    subgraph "CI/CD"
        GH[GitHub Actions<br/>Ubuntu runner]
    end
    
    CTRL --> MOCK_API
    SENS --> MOCK_API
    ACT --> MOCK_API
    CTRL --> MOCK_LOG
    
    GTEST --> CTRL
    GTEST --> SENS
    GTEST --> ACT
    GTEST --> CFG
    
    CMAKE --> GTEST
    GH --> CMAKE
```

### Test Coverage

| Feature | Test Case | Status |
|---------|-----------|--------|
| Door safety | DoorOpenShutdown | ✅ |
| Ignition timing | IgnitionTiming | ✅ |
| Hysteresis control | HysteresisControl | ✅ |
| Sensor fault timing | SensorFaultTimedDetection | ✅ |
| Auto-resume | AutoResumeAfterValidWindow | ✅ |
| Ignition retry | IgnitionRetryAfterFailure | ✅ |
| Ignition lockout | IgnitionLockoutAfterMaxAttempts | ✅ |
| Manual reset | ManualResetFromLockout | ✅ |

---

## 10. Deployment Diagram

```mermaid
graph TB
    subgraph "Arduino MCU"
        APP[sketch.ino<br/>Main Loop]
        
        subgraph "Control Layer"
            CTRL[ptx_oven_control]
            SENS[ptx_sensor_filter]
            ACT[ptx_actuator]
            CFG[ptx_oven_config]
            LOG[ptx_logging]
        end
        
        subgraph "HAL"
            API[api.cpp<br/>Hardware Access]
        end
    end
    
    subgraph "Hardware"
        SENSOR[PT100 Sensor<br/>Analog ADC]
        DOOR[Door Switch<br/>GPIO Interrupt]
        GAS[Gas Valve<br/>GPIO Output]
        IGN[Igniter<br/>GPIO Output]
    end
    
    APP --> CTRL
    CTRL --> SENS
    CTRL --> ACT
    CTRL --> CFG
    CTRL --> LOG
    
    SENS --> API
    ACT --> API
    LOG --> API
    
    API --> SENSOR
    API --> DOOR
    API --> GAS
    API --> IGN
```

---

## 11. Future Enhancements

### Planned Features
- [ ] **PID temperature control** (replace simple hysteresis)
- [ ] **WiFi monitoring** (web dashboard)
- [ ] **Data logging to SD card** (temperature history)
- [ ] **Multiple temperature zones** (top/bottom heating)
- [ ] **Recipe management** (time/temp profiles)
- [ ] **OTA firmware updates**

### Scalability Considerations
- Modular architecture allows easy addition of new sensors
- Runtime config system supports dynamic parameter tuning
- State machine design scales to more complex heating profiles
- HAL abstraction enables porting to different hardware

---

## 12. How to Import to Lucidchart

### Option 1: Manual Recreation
1. Open this document in GitHub (Mermaid renders automatically)
2. Take screenshots of diagrams
3. Import to Lucidchart as image templates
4. Redraw using Lucidchart shapes

### Option 2: PlantUML Export
```bash
# Install PlantUML
npm install -g node-plantuml

# Convert Mermaid to PlantUML (manual conversion needed)
# Then generate PNG:
plantuml system_design.puml
```

### Option 3: Use Mermaid Live Editor
1. Go to https://mermaid.live
2. Copy-paste Mermaid code
3. Export as PNG/SVG
4. Import to Lucidchart

---

## 13. Contact & Maintenance

**Project Repository:** https://github.com/ngvanhak49/ptx_oven_controller_demo

**Documentation Updates:** Keep this file in sync with code changes

**Review Schedule:** Update diagrams after major architectural changes