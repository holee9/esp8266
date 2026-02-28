# Technology Stack

## Platform

| Component | Technology | Details |
|-----------|------------|---------|
| SoC | ESP8266EX | Tensilica LX106, 80/160MHz |
| Module | ESP-12E/F | 4MB Flash, ~50KB SRAM |
| Board | HW-364A | Built-in SSD1306 OLED, CH340G USB-Serial, USB-C |
| Framework | Arduino | ESP8266 Arduino Core |
| Build System | PlatformIO | Multi-environment support |

## Development Environment

### Prerequisites

- Python 3.12+
- PlatformIO Core CLI
- USB-Serial driver (CH340G)

### Build Environments

```ini
[env:debug]      # Debug build with ARTHUR_DEBUG=1
[env:release]    # Optimized release (-Os)
[env:native_test]   # PC-based unit tests
[env:embedded_test] # Device-based tests
```

### Compiler Settings

| Setting | Value | Reason |
|---------|-------|--------|
| C++ Standard | gnu++14 | C++17 unstable on ESP8266 toolchain |
| CPU Frequency | 160MHz | Performance boost |
| lwIP Variant | v2 Lower Memory | Memory optimization |

## Core Libraries

### WiFi Management (IotWebConf 3.2.1)

- Non-blocking WiFi configuration
- Captive Portal for zero-serial setup
- EEPROM-based configuration persistence
- Custom HTML format provider for WiFi scan dropdown

### Task Scheduling (TaskScheduler 3.7.0)

- Cooperative multitasking (no RTOS overhead)
- ESP8266 yield() compatible
- 15-18 microsecond overhead per task
- Status request pattern for inter-task communication

### Display (Adafruit SSD1306 2.5.0)

- 1KB framebuffer (128x64 / 8 bits per byte)
- I2C interface at 0x3C
- Non-standard pins: SDA=GPIO14, SCL=GPIO12
- Dual-color display: Yellow (0-15px) / Blue (16-63px)

### JSON Processing (ArduinoJson 7.0.0)

- Memory-efficient JSON serialization
- Used for weather API responses
- Avoided in WiFi scan to save memory (direct String concatenation)

### MQTT (256dpi/MQTT 2.5.0)

- Selected over PubSubClient due to IotWebConf conflicts
- Exception 28/29 issues with PubSubClient
- ~1.5-3KB memory footprint

### Sensors (Adafruit BME280 2.2.0)

- Temperature, humidity, pressure sensing
- I2C address: 0x76 (SDO=GND)
- Shared I2C bus with OLED

## Memory Budget

| Component | Memory Usage |
|-----------|--------------|
| WiFi STA (connected) | 20-25KB |
| IotWebConf (webserver + config) | 4-8KB |
| SSD1306 Framebuffer | 1KB (fixed) |
| MQTT Client | 1.5-3KB |
| ArduinoJson (temporary) | 1KB/block |
| TaskScheduler (10 tasks) | ~500B |
| BME280 Driver | ~300B |
| TLS Handshake (temporary) | 15KB (HTTPS only) |
| **Available Heap** | **10-18KB** |

## Hardware Constraints

### GPIO Availability

| Pin | GPIO | Status | Notes |
|-----|------|--------|-------|
| D1 | GPIO5 | Available | I2C SCL (external), interrupts OK |
| D2 | GPIO4 | Available | I2C SDA (external), interrupts OK |
| D5 | GPIO14 | Occupied | OLED SDA |
| D6 | GPIO12 | Occupied | OLED SCL |
| D7 | GPIO13 | Available | Digital I/O, buzzer |
| D3 | GPIO0 | Button | FLASH button (config reset) |
| D4 | GPIO2 | LED | Built-in LED (status) |
| D0 | GPIO16 | Limited | PWM/I2C/interrupts unavailable, deep sleep wake |
| A0 | ADC0 | Available | Analog input (may interfere with WiFi) |

### I2C Bus

- Primary bus: GPIO14 (SDA) / GPIO12 (SCL)
- Devices: OLED (0x3C), BME280 (0x76)
- Note: Some HW-364 units have SDA/SCL swapped

### SPI

- Hardware SPI unavailable (OLED occupies HSPI pins)
- Software SPI possible but slower

## Design Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| C++ Standard | C++14 | C++17 unstable on ESP8266 toolchain |
| MQTT Library | 256dpi/MQTT | PubSubClient conflicts with IotWebConf |
| String Handling | char[] + F() macro | Avoid String class heap fragmentation |
| Memory Allocation | Static only | No runtime new/malloc |
| Filesystem | LittleFS | SPIFFS deprecated |
| lwIP Variant | v2 Lower Memory | Memory optimization for IotWebConf |

---

Generated: 2026-02-28
