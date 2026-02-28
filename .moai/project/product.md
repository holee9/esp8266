# Product Overview

## Project Identity

| Attribute | Value |
|-----------|-------|
| Name | ARTHUR |
| Full Name | Autonomous Real-Time Home Utility Responder |
| Classification | AttoClaw - Smallest unit AI agent in Claw ecosystem |
| Platform | ESP8266 (HW-364A board with built-in OLED) |
| Version | 0.1.0 |
| Status | Phase 0 Complete |

## Description

ARTHUR is a modular personal assistant built on the ESP8266 HW-364 board with an integrated OLED display. Designed to operate within the extremely memory-constrained environment of ESP8266 (~10-18KB available heap), it serves as the smallest unit (AttoClaw) in the Claw AI agent ecosystem.

## Core Features

### Implemented (Phase 0)

- **WiFi Configuration**: IotWebConf Captive Portal for zero-serial-setup WiFi configuration
- **WiFi Scan**: Automatic AP scanning with dropdown selection on configuration page
- **OLED Display**: SSD1306 128x64 dual-color display (yellow status bar / blue content area)
- **Status Visualization**: Setup Mode -> Connecting -> WiFi OK status flow

### Planned (Phase 1-4)

- **NTP Clock**: Real-time clock with date display
- **Weather**: OpenWeatherMap API integration for weather forecasts
- **Environmental Sensing**: BME280 sensor for temperature, humidity, pressure monitoring
- **IoT Control**: MQTT-based device control
- **Notifications**: Push notification display
- **Home Assistant Integration**: Via PC proxy (Docker-based)
- **Multi-AI Subscriptions**: ChatGPT/Claude/Gemini/Ollama integration
- **Dual Mode**: PC Enhanced / Standalone operation modes
- **Deep Sleep**: Power optimization for battery operation

## Target Audience

| User Type | Use Case |
|-----------|----------|
| DIY Makers | Home automation hobbyist seeking ESP8266-based smart display |
| IoT Developers | Reference implementation for memory-constrained IoT devices |
| Home Assistant Users | Compact display device for smart home monitoring |
| AI Enthusiasts | Physical interface for multi-AI subscriptions |

## Use Cases

1. **Desk Clock with Weather**: Display time, date, and weather information
2. **Indoor Climate Monitor**: Real-time temperature/humidity/pressure display
3. **Smart Home Controller**: MQTT-based IoT device control
4. **Notification Hub**: Display alerts from connected services
5. **AI Assistant Interface**: Physical terminal for AI conversations

## Constraints

- **Memory**: Extremely limited heap (~10-18KB after WiFi and libraries)
- **Flash**: 4MB total (1MB sketch + 3MB filesystem), ~10% used in Phase 0
- **Hardware SPI**: Not available (OLED occupies HSPI pins)
- **I2C**: Shared bus for OLED and external sensors
- **C++ Standard**: C++14 only (C++17 unstable on ESP8266 toolchain)

## Success Metrics

| Phase | Success Criteria |
|-------|------------------|
| Phase 0 | WiFi setup via Captive Portal, OLED functional |
| Phase 1 | NTP sync, weather display, BME280 reading |
| Phase 2 | MQTT pub/sub working, notifications displayed |
| Phase 3 | Home Assistant integration via PC proxy |
| Phase 4 | AI chat functional, deep sleep mode working |

---

Generated: 2026-02-28
