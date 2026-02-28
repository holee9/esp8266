# Project Structure

## Directory Layout

```
esp8266/
├── .claude/              # Claude Code configuration
│   ├── rules/            # MoAI-ADK rules
│   └── skills/           # MoAI skills
├── .moai/                # MoAI-ADK project files
│   ├── config/           # Project configuration
│   ├── project/          # Generated documentation
│   └── specs/            # SPEC documents
├── .pio/                 # PlatformIO build artifacts
│   └── libdeps/          # Dependency libraries
├── data/                 # LittleFS filesystem data
├── docs/                 # Additional documentation
├── include/              # Header files
│   ├── arthur_config.h   # Configuration constants
│   └── arthur_pins.h     # GPIO pin definitions
├── src/                  # Source code
│   └── main.cpp          # Main application entry point
├── test/                 # Test files
│   ├── native/           # Native (PC) tests
│   └── embedded/         # Embedded (device) tests
├── .gitignore
├── .mcp.json             # MCP server configuration
├── CLAUDE.md             # Claude Code project instructions
├── platformio.ini        # PlatformIO build configuration
└── README.md             # Project documentation
```

## Key Files

### Source Files

| File | Purpose | Lines |
|------|---------|-------|
| `src/main.cpp` | Main application entry point, WiFi/OLED management | ~314 |
| `include/arthur_config.h` | Configuration constants (version, timeouts, buffer sizes) | ~47 |
| `include/arthur_pins.h` | GPIO pin definitions for HW-364 board | ~43 |

### Configuration Files

| File | Purpose |
|------|---------|
| `platformio.ini` | Build configuration, environment definitions, dependencies |
| `.moai/config/sections/quality.yaml` | Development mode (TDD/DDD), test coverage targets |
| `.moai/config/sections/language.yaml` | Language settings (Korean user, English code) |

### Build Environments

| Environment | Description |
|-------------|-------------|
| `debug` | Debug build with exception decoder |
| `release` | Optimized release build (-Os) |
| `native_test` | PC-based unit tests |
| `embedded_test` | Device-based tests |

## Module Organization

### Current Implementation (Phase 0)

```
main.cpp
├── OLED Display (Adafruit_SSD1306)
│   ├── drawStatusBar()    - Yellow status bar
│   ├── drawContent()      - Blue content area
│   └── showXxxScreen()    - State-specific displays
├── WiFi Manager (IotWebConf)
│   ├── Captive Portal
│   ├── WiFi Scan API
│   └── Custom HTML (WiFi scan dropdown)
└── Web Server (ESP8266WebServer)
    ├── handleRoot()       - Status page
    ├── handleScan()       - WiFi scan JSON API
    └── handleConfig()     - Configuration page
```

### Planned Architecture

```
App (Orchestrator)
├── TaskScheduler (Cooperative multitasking)
├── EventBus (pub/sub module communication)
├── Core Modules
│   ├── WiFiManager
│   ├── ConfigManager
│   ├── TimeManager
│   └── OTAManager
├── Feature Modules
│   ├── ClockModule
│   ├── WeatherModule
│   ├── SensorModule
│   ├── MqttModule
│   ├── NotificationModule
│   ├── ProxyManager
│   ├── HomeAssistantModule
│   └── AIModule
└── UI Layer
    ├── ScreenManager
    └── Screens (Clock, Weather, etc.)
```

## Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| TaskScheduler | 3.7.0 | Cooperative multitasking |
| ArduinoJson | 7.0.0 | JSON parsing/serialization |
| MQTT (256dpi) | 2.5.0 | MQTT client (PubSubClient alternative) |
| IotWebConf | 3.2.1 | WiFi configuration portal |
| Adafruit SSD1306 | 2.5.0 | OLED display driver |
| Adafruit BME280 | 2.2.0 | Environmental sensor driver |

---

Generated: 2026-02-28
