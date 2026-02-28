# Module Descriptions

## Core Modules

### WiFiManager (IotWebConf Wrapper)

**Status**: Implemented (Phase 0)

**Responsibility**:
- WiFi connection management
- Captive Portal for initial configuration
- Configuration persistence in EEPROM
- WiFi scan functionality

**Key Functions**:
| Function | Description |
|----------|-------------|
| `iotWebConf.init()` | Initialize and load saved config |
| `iotWebConf.doLoop()` | Non-blocking WiFi state machine |
| `iotWebConf.getState()` | Get current network state |
| `handleScan()` | WiFi scan API endpoint |

**State Transitions**:
```
Boot → NotConfigured → ApMode
     → Connecting → OnLine
                  → OffLine → Connecting (auto-reconnect)
```

---

### DisplayManager (SSD1306 Wrapper)

**Status**: Implemented (Phase 0)

**Responsibility**:
- OLED display initialization
- Screen rendering (status bar + content)
- State-specific screen templates

**Key Functions**:
| Function | Description |
|----------|-------------|
| `drawStatusBar()` | Render yellow status bar (rows 0-15) |
| `drawContent()` | Render blue content area (rows 16-63) |
| `showBootScreen()` | Display boot splash |
| `showApModeScreen()` | Display setup mode instructions |
| `showConnectingScreen()` | Display connecting status |
| `showConnectedScreen()` | Display WiFi info + heap |

**Hardware Configuration**:
- I2C Address: 0x3C
- SDA: GPIO14 (D5)
- SCL: GPIO12 (D6)
- Resolution: 128x64 pixels

---

### ConfigManager

**Status**: Planned (Phase 1)

**Responsibility**:
- Application configuration persistence
- EEPROM/LittleFS storage abstraction
- Configuration validation

---

### TimeManager (NTP)

**Status**: Planned (Phase 1)

**Responsibility**:
- NTP time synchronization
- Timezone handling (KST: UTC+9)
- Time provision to other modules

**Configuration**:
- Server: pool.ntp.org
- Sync interval: 1 hour
- Timezone offset: +9 hours

---

## Feature Modules (Planned)

### ClockModule (Phase 1)

**Responsibility**:
- Time display formatting
- Date display (Korean format)
- Clock screen management

---

### WeatherModule (Phase 1)

**Responsibility**:
- OpenWeatherMap API integration
- Weather data caching
- Weather screen management

**Configuration**:
- Update interval: 10 minutes
- API: OpenWeatherMap

---

### SensorModule (Phase 1)

**Responsibility**:
- BME280 sensor reading
- Temperature/humidity/pressure monitoring
- Sensor data provision via EventBus

**Configuration**:
- Read interval: 5 seconds
- I2C address: 0x76

---

### MqttModule (Phase 2)

**Responsibility**:
- MQTT connection management
- Topic subscription/publication
- IoT device control

**Configuration**:
- Buffer size: 256 bytes
- Keepalive: 30 seconds

---

### NotificationModule (Phase 2)

**Responsibility**:
- Push notification handling
- Notification queue management
- Alert display

---

### ProxyManager (Phase 3)

**Responsibility**:
- PC proxy discovery (mDNS)
- Dual mode management (PC Enhanced / Standalone)
- Communication protocol handling

---

### HomeAssistantModule (Phase 3)

**Responsibility**:
- Home Assistant API integration
- Entity state monitoring
- Device control delegation

---

### AIModule (Phase 4)

**Responsibility**:
- Multi-AI subscription management
- ChatGPT/Claude/Gemini/Ollama integration
- Conversation handling

---

## UI Layer (Planned)

### ScreenManager

**Responsibility**:
- Screen state management (FSM)
- Screen transition handling
- Button input routing

### Screens

| Screen | Trigger | Content |
|--------|---------|---------|
| ClockScreen | Default | Time, date, indoor climate |
| WeatherScreen | Button | Weather forecast |
| SensorScreen | Button | BME280 detailed readings |
| MqttScreen | Button | MQTT status, devices |
| NotificationScreen | Alert | Incoming notifications |
| AIScreen | Button | AI chat interface |
| SetupScreen | Boot (no config) | WiFi setup instructions |

---

Generated: 2026-02-28
