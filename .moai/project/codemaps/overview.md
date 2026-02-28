# Architecture Overview

## System Summary

ARTHUR is a memory-constrained IoT application running on ESP8266 HW-364 board. It follows a modular, event-driven architecture designed for cooperative multitasking.

## High-Level Architecture

```
┌────────────────────────────────────────────────────────────────┐
│                        ARTHUR Application                       │
├────────────────────────────────────────────────────────────────┤
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                    UI Layer (Planned)                    │  │
│  │  ScreenManager → [ClockScreen, WeatherScreen, ...]      │  │
│  └──────────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                  Feature Modules (Planned)               │  │
│  │  ClockModule, WeatherModule, SensorModule, MqttModule... │  │
│  └──────────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                    Core Modules                          │  │
│  │  WiFiManager (IotWebConf)  │  DisplayManager (SSD1306)   │  │
│  │  ConfigManager             │  TimeManager (NTP, planned) │  │
│  └──────────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              Infrastructure (Planned)                    │  │
│  │  EventBus (pub/sub)  │  TaskScheduler (cooperative)      │  │
│  └──────────────────────────────────────────────────────────┘  │
├────────────────────────────────────────────────────────────────┤
│                      ESP8266 Hardware                          │
│  WiFi  │  I2C (OLED + Sensors)  │  GPIO  │  Flash/LittleFS    │
└────────────────────────────────────────────────────────────────┘
```

## Architectural Patterns

### Current (Phase 0)

| Pattern | Implementation |
|---------|---------------|
| Event Loop | `loop()` → `iotWebConf.doLoop()` → state polling |
| State Machine | `iotwebconf::NetworkState` for WiFi/OLED sync |
| Callback | `wifiConnectedCallback`, `configSavedCallback` |

### Planned (Phase 1+)

| Pattern | Implementation |
|---------|---------------|
| Cooperative Multitasking | TaskScheduler with priority-based tasks |
| Pub/Sub | EventBus for inter-module communication |
| FSM | ScreenManager for UI state management |
| Repository | ConfigManager for EEPROM/LittleFS access |

## Key Design Principles

1. **Memory-First**: Every design decision considers the 10-18KB heap constraint
2. **Non-Blocking**: All operations use async patterns (no delay() > 10ms)
3. **Static Allocation**: No runtime new/malloc to prevent fragmentation
4. **Modular**: Clear module boundaries for testability and extensibility
5. **Fail-Safe**: WiFi reset via FLASH button, automatic reconnection

## Execution Flow

```
Boot
  │
  ├─→ Serial Init
  ├─→ I2C Init (GPIO14/GPIO12)
  ├─→ OLED Begin + Boot Screen
  ├─→ IotWebConf Init (load EEPROM config)
  ├─→ Web Handlers Register
  │
  └─→ Loop (forever)
        │
        ├─→ iotWebConf.doLoop()
        │     └─→ WiFi state machine
        │
        └─→ State Check
              ├─→ ApMode → Show Setup Screen
              ├─→ Connecting → Show Connecting Screen
              ├─→ OnLine → Show Connected Screen
              └─→ OffLine → Show Reconnecting Screen
```

## Resource Utilization

| Resource | Total | Phase 0 | Target (Phase 4) |
|----------|-------|---------|------------------|
| Flash | 4MB | 9.5% (365KB) | < 25% |
| RAM | ~50KB | 38.8% | < 60% |
| Heap (free) | ~18KB | ~45KB available | > 10KB |

---

Generated: 2026-02-28
