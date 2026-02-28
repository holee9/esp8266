# ARTHUR - ESP8266 Personal Assistant

**Autonomous Real-Time Home Utility Responder**

A modular personal assistant built on ESP8266 HW-364 (with built-in OLED), designed to run within the extreme memory constraints (~10-18KB usable heap) of the ESP8266 platform.

## Features (Planned)

- NTP Clock with date display
- Weather forecast (OpenWeatherMap API)
- Indoor temperature/humidity/pressure monitoring (BME280)
- MQTT IoT device control
- Push notification display
- Home Assistant integration
- AI API integration (OpenAI, single-shot queries)

## Hardware

### Board: HW-364A/B

| Spec | Value |
|------|-------|
| SoC | ESP8266EX (Tensilica LX106, 80/160MHz) |
| Module | ESP-12E / ESP-12F |
| Flash | **1MB** |
| RAM | ~50KB SRAM (10-18KB usable after WiFi + libs) |
| USB-Serial | CH340G |
| Built-in OLED | SSD1306 128x64, I2C @ 0x3C |
| OLED Colors | Top 16px yellow / Bottom 48px blue (dual-color) |
| OLED Pins | **SDA=GPIO14, SCL=GPIO12** (non-standard!) |
| USB | HW-364A=USB-C, HW-364B=Micro-USB (pin-compatible) |

### GPIO Availability (after OLED)

| Pin | GPIO | Status | Notes |
|-----|------|--------|-------|
| D1 | GPIO5 | **Free** | I2C SCL (external sensors), interrupt OK |
| D2 | GPIO4 | **Free** | I2C SDA (external sensors), interrupt OK |
| D7 | GPIO13 | **Free** | Digital I/O, buzzer |
| D0 | GPIO16 | **Limited** | No PWM/I2C/interrupt, deep sleep wake only |
| A0 | ADC0 | **Free** | Analog input (frequent reads may interfere with WiFi) |

**Hardware SPI unavailable**: GPIO14 (HSPI_CLK) and GPIO12 (HSPI_MISO) occupied by OLED.

### Wiring (BME280 via shared I2C bus)

```
HW-364 Board (built-in OLED)     BME280 Sensor (I2C)
─────────────────────────────    ───────────────────
3V3 ──────────────────────────── VCC
GND ──────────────────────────── GND
GPIO14 (D5) ── OLED SDA ──────── BME280 SDA (shared bus)
GPIO12 (D6) ── OLED SCL ──────── BME280 SCL (shared bus)

BME280 I2C address: 0x76 (SDO=GND) — no conflict with OLED 0x3C
```

> **Note**: Some HW-364 units have SDA/SCL swapped. If display doesn't work, try `Wire.begin(12, 14)`.

## Software Architecture

```
[main.cpp]
  └── App (Orchestrator)
       ├── TaskScheduler (cooperative multitasking)
       ├── EventBus (pub/sub inter-module communication)
       ├── Core Modules
       │    ├── ConfigManager (LittleFS + ArduinoJson v7)
       │    ├── WiFiManager (IotWebConf 3.2.2 captive portal)
       │    ├── TimeManager (NTP sync)
       │    └── OTAManager (wireless firmware update)
       ├── Feature Modules
       │    ├── ClockModule
       │    ├── WeatherModule (OpenWeatherMap)
       │    ├── SensorModule (BME280)
       │    ├── MqttModule (arduino-mqtt / 256dpi)
       │    ├── NotificationModule
       │    └── HomeAssistantModule
       └── UI Layer
            ├── ScreenManager (SimpleFSM)
            └── Screens (Clock, Weather, Sensor, MQTT, Notification, Setup)
```

### Key Design Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| C++ Standard | **C++14** (gnu++14) | C++17 unstable on ESP8266 toolchain |
| MQTT Library | **256dpi/MQTT** | PubSubClient crashes with IotWebConf (exception 28/29) |
| WiFi Manager | IotWebConf 3.2.2 | Non-blocking, TaskScheduler compatible |
| Display Library | Adafruit SSD1306 | Proven compatibility (1KB framebuffer) |
| Multitasking | TaskScheduler | ESP8266 yield() compatible, 15-18us overhead |
| String Handling | char[] + F() macro | Prevent heap fragmentation from String class |
| Memory Allocation | Static only | No runtime new/malloc |
| Filesystem | LittleFS | SPIFFS deprecated |
| lwIP Variant | v2 Lower Memory | Memory savings, IotWebConf recommended |

### Memory Budget

| Component | Usage |
|-----------|-------|
| WiFi STA (connected) | 20-25KB |
| IotWebConf (webserver+config) | 4-8KB |
| SSD1306 framebuffer | 1KB (fixed) |
| MQTT client | 1.5-3KB |
| ArduinoJson (temporary) | 1KB/block |
| TaskScheduler (10 tasks) | ~500B |
| BME280 driver | ~300B |
| TLS Handshake (temporary) | 15KB (HTTPS only) |
| **Remaining usable heap** | **10-18KB** |

## Dependencies

```ini
lib_deps =
    arkhipenko/TaskScheduler@^3.7.0
    bblanchon/ArduinoJson@^7.0.0
    256dpi/MQTT@^2.5.0
    prampec/IotWebConf@^3.2.2
    adafruit/Adafruit SSD1306@^2.5.0
    adafruit/Adafruit BME280 Library@^2.2.0
```

## Development Setup

### Prerequisites

- Python 3.12+
- PlatformIO Core CLI

### Installation

```bash
# Install system packages
sudo apt update && sudo apt install -y python3-venv python3.12-venv curl git

# Install PlatformIO (official installer, creates isolated venv)
curl -fsSL -o /tmp/get-platformio.py \
  https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 /tmp/get-platformio.py

# Add to PATH (zsh)
echo 'export PATH="$HOME/.platformio/penv/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc

# Install udev rules for ESP8266/CH340
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/develop/platformio/assets/system/99-platformio-udev.rules \
  | sudo tee /etc/udev/rules.d/99-platformio-udev.rules
sudo udevadm control --reload-rules && sudo udevadm trigger

# Verify
pio --version
pio device list
```

### Build & Upload

```bash
# Debug build
pio run

# Upload to board
pio run --target upload

# Serial monitor
pio device monitor --baud 115200

# Run native tests (PC)
pio test -e native_test

# Run embedded tests (on device)
pio test -e embedded_test
```

## Display UI (Dual-Color OLED)

Yellow zone (rows 0-15) = status bar, Blue zone (rows 16-63) = content.

```
Clock:                         Weather:
┌────────────────────────┐     ┌────────────────────────┐
│ 22.5C 45% WiFi [yellow]│     │ Seoul   3C    [yellow] │
├────────────────────────┤     ├────────────────────────┤
│                        │     │                        │
│      14:35:28          │     │  Clear                 │
│    2026-02-28 Sat      │     │  Feels:-1C Hum:35%    │
│                 [blue] │     │  Wind:3m/s      [blue] │
└────────────────────────┘     └────────────────────────┘
```

## Roadmap

- **Phase 0**: Development environment setup
- **Phase 1 (MVP)**: Clock + Weather + Sensor display
- **Phase 2**: MQTT communication + Notifications
- **Phase 3**: Home Assistant integration
- **Phase 4**: AI API + Deep Sleep

## License

MIT

## References

- [peff74/esp8266_OLED_HW-364A](https://github.com/peff74/esp8266_OLED_HW-364A) - HW-364A reference implementation
- [ESP8266 Pinout Reference](https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/)
- [IotWebConf](https://github.com/prampec/IotWebConf) - WiFi configuration portal
- [TaskScheduler](https://github.com/arkhipenko/TaskScheduler) - Cooperative multitasking
