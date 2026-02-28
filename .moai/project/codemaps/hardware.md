# Hardware Configuration

## Board: HW-364A

### Specifications

| Attribute | Value |
|-----------|-------|
| SoC | ESP8266EX (Tensilica LX106) |
| Clock | 80MHz / 160MHz (configurable) |
| Module | ESP-12E / ESP-12F |
| Flash | **4MB** (1MB sketch + 3MB LittleFS) |
| SRAM | ~50KB (usable ~18KB after system) |
| USB-Serial | CH340G |
| USB Connector | USB-C |

### Built-in OLED

| Attribute | Value |
|-----------|-------|
| Controller | SSD1306 |
| Resolution | 128x64 pixels |
| Interface | I2C |
| Address | 0x3C |
| Colors | 2-color (Yellow top 16px / Blue bottom 48px) |
| Framebuffer | 1KB (1024 bytes) |

## GPIO Assignment

### Pin Map

| Pin | GPIO | Function | Assignment | Notes |
|-----|------|----------|------------|-------|
| D0 | GPIO16 | Wake | Deep Sleep Wake | No PWM/I2C/interrupts |
| D1 | GPIO5 | I2C SCL | **Available** | External sensors |
| D2 | GPIO4 | I2C SDA | **Available** | External sensors |
| D3 | GPIO0 | Button | FLASH Button | Config reset on boot |
| D4 | GPIO2 | LED | Built-in LED | Active LOW |
| D5 | GPIO14 | OLED SDA | Occupied | OLED I2C data |
| D6 | GPIO12 | OLED SCL | Occupied | OLED I2C clock |
| D7 | GPIO13 | Digital | **Available** | Buzzer, GPIO |
| A0 | ADC0 | Analog | **Available** | 0-1V input (may interfere with WiFi) |

### I2C Bus Configuration

```
Primary I2C Bus (GPIO14/GPIO12):
├── SSD1306 OLED @ 0x3C
└── BME280 Sensor @ 0x76 (SDO=GND)
```

**Note**: I2C pins are non-standard (typically GPIO4/GPIO5).

### SPI Availability

| SPI | Status | Reason |
|-----|--------|--------|
| HSPI | ❌ Unavailable | GPIO14 (CLK) and GPIO12 (MISO) occupied by OLED |
| Software SPI | ✅ Possible | Slower, use any GPIO |

## Peripheral Configuration

### OLED Display

```cpp
// Non-standard I2C pins
#define OLED_SDA 14  // GPIO14 (D5)
#define OLED_SCL 12  // GPIO12 (D6)
#define OLED_ADDR 0x3C

// Initialization
Wire.begin(OLED_SDA, OLED_SCL);
display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
```

**Troubleshooting**: Some HW-364 units have SDA/SCL reversed. If display doesn't initialize:

```cpp
// Try swapped pins
Wire.begin(12, 14);  // SDA=GPIO12, SCL=GPIO14
```

### BME280 Sensor

| Connection | HW-364 Pin | BME280 Pin |
|------------|------------|------------|
| VCC | 3V3 | VCC |
| GND | GND | GND |
| SDA | GPIO14 (D5) | SDA |
| SCL | GPIO12 (D6) | SCL |

**I2C Address**: 0x76 (SDO connected to GND)

```cpp
#define BME280_ADDR 0x76

// Shared I2C bus with OLED
Adafruit_BME280 bme;
bme.begin(BME280_ADDR);
```

### Button (FLASH)

| Attribute | Value |
|-----------|-------|
| Pin | GPIO0 (D3) |
| Active Level | LOW (pressed) |
| Function | Force AP mode on boot |

```cpp
#define BUTTON_PIN 0  // GPIO0

// IotWebConf config reset
iotWebConf.setConfigPin(BUTTON_PIN);
```

### Built-in LED

| Attribute | Value |
|-----------|-------|
| Pin | GPIO2 (D4) |
| Active Level | LOW (on) |
| Function | WiFi status indicator |

```cpp
#define LED_PIN 2  // GPIO2

// IotWebConf status LED
iotWebConf.setStatusPin(LED_PIN, LOW);  // Active LOW
```

### Buzzer (Optional)

| Attribute | Value |
|-----------|-------|
| Pin | GPIO13 (D7) |
| Type | Active buzzer or PWM passive |

```cpp
#define BUZZER_PIN 13  // GPIO13 (D7)

// Simple beep
digitalWrite(BUZZER_PIN, HIGH);
delay(100);
digitalWrite(BUZZER_PIN, LOW);
```

## Power Supply

### Recommended

| Source | Voltage | Current |
|--------|---------|---------|
| USB | 5V (regulated to 3.3V) | 500mA+ |
| 3.3V Pin | 3.3V direct | 500mA+ |

### Current Consumption

| Mode | Current |
|------|---------|
| Boot | ~70mA |
| WiFi Active | ~170mA |
| WiFi Sleep | ~15mA |
| Deep Sleep | ~20µA |

## Hardware Variants

### HW-364A (Current)

| Feature | Value |
|---------|-------|
| USB Connector | USB-C |
| Flash | 4MB |
| OLED | SSD1306 128x64 |
| CH340G | Yes |

### Known Issues

1. **SDA/SCL Reversal**: Some units have reversed I2C pins
   - Solution: Try `Wire.begin(12, 14)` if default fails

2. **Flash Size**: Verify with `ESP.getFlashChipSize()`
   - Expected: 4MB (4194304 bytes)

## Electrical Constraints

### GPIO Output

| Parameter | Value |
|-----------|-------|
| Voltage | 3.3V |
| Max Current | 12mA per pin |
| Total Max | 120mA all pins |

### GPIO Input

| Parameter | Value |
|-----------|-------|
| Voltage Range | 0-3.3V |
| ADC Range | 0-1V (A0 only) |

### I2C Pull-up

OLED has internal pull-ups. External sensors may need additional pull-ups (4.7KΩ).

## Wiring Diagram

```
HW-364A Board (OLED Built-in)         BME280 Sensor
─────────────────────────────────    ───────────────────
    3V3 ──────────────────────────── VCC
    GND ──────────────────────────── GND
GPIO14 (D5) ── OLED SDA ──────────── SDA (shared bus)
GPIO12 (D6) ── OLED SCL ──────────── SCL (shared bus)

BME280 I2C Address: 0x76 (SDO=GND)
OLED I2C Address: 0x3C
```

---

Generated: 2026-02-28
Source: README.md hardware section + include/arthur_pins.h
