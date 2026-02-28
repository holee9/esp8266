# ARTHUR - ESP8266 개인비서

**Autonomous Real-Time Home Utility Responder**

ESP8266 HW-364 보드(OLED 내장)를 기반으로 한 모듈형 개인비서. ESP8266의 극도로 제한된 메모리(가용 힙 ~10-18KB) 환경에서 동작하도록 설계되었다.

**Claw 분류**: AttoClaw — Claw 생태계에서 가장 극소 단위의 AI 에이전트

## 현재 상태: Phase 0 완료

### 구현 완료

- PlatformIO 개발 환경 구축 (ESP8266 툴체인, 1MB flash 설정)
- OLED 디스플레이 동작 확인 (SSD1306, I2C, GPIO14/12)
- IotWebConf Captive Portal WiFi 설정 (시리얼/PC 불필요)
- WiFi 스캔 드롭다운 (설정 페이지에서 AP 목록 자동 표시)
- OLED 상태 표시 (Setup Mode → Connecting → WiFi OK)
- 2색 OLED 레이아웃 (상단 노랑 상태바 / 하단 파랑 콘텐츠)

### 빌드 현황

| 항목 | 값 |
|------|-----|
| Flash | 9.5% (365KB / 4MB) |
| RAM | 38.8% (31.7KB / 81.9KB) |
| Free Heap (WiFi 연결 후) | ~45KB |

## 기능 (계획)

- NTP 시계 및 날짜 표시
- 날씨 예보 (OpenWeatherMap API)
- 실내 온도/습도/기압 모니터링 (BME280)
- MQTT IoT 기기 제어
- 푸시 알림 표시
- Home Assistant 연동 (PC 프록시 경유)
- 구독형 멀티 AI 에이전트 (ChatGPT/Claude/Gemini/Ollama)
- Dual Mode (PC Enhanced / Standalone)

## WiFi 설정 방법

1. 최초 부팅 시 OLED에 "Setup Mode" 표시
2. 스마트폰 WiFi에서 **ARTHUR** AP에 연결 (패스워드: `arthur123`)
3. 자동으로 설정 페이지 열림 (또는 브라우저에서 `http://192.168.4.1`)
4. 로그인: `admin` / `arthur123`
5. **Scan WiFi** 버튼으로 주변 AP 스캔 → 드롭다운에서 선택
6. 패스워드 입력 → Apply → 자동 재부팅 후 WiFi 연결
7. 이후 재부팅 시 자동 연결

> **WiFi 재설정**: 부팅 시 FLASH 버튼을 누르고 있으면 AP 모드 강제 진입

## 하드웨어

### 보드: HW-364A/B

| 항목 | 값 |
|------|-----|
| SoC | ESP8266EX (Tensilica LX106, 80/160MHz) |
| 모듈 | ESP-12E / ESP-12F |
| Flash | **4MB** (1MB sketch + 3MB filesystem) |
| RAM | ~50KB SRAM (WiFi + 라이브러리 로드 후 가용 10-18KB) |
| USB-Serial | CH340G |
| 내장 OLED | SSD1306 128x64, I2C @ 0x3C |
| OLED 색상 | 상단 16px 노랑 / 하단 48px 파랑 (2색) |
| OLED 핀 | **SDA=GPIO14, SCL=GPIO12** (비표준!) |
| USB | HW-364A=USB-C, HW-364B=Micro-USB (핀 호환) |

### GPIO 가용성 (OLED 점유 후)

| 핀 | GPIO | 상태 | 비고 |
|-----|------|------|------|
| D1 | GPIO5 | **사용 가능** | I2C SCL (외부 센서), 인터럽트 OK |
| D2 | GPIO4 | **사용 가능** | I2C SDA (외부 센서), 인터럽트 OK |
| D7 | GPIO13 | **사용 가능** | Digital I/O, 부저 |
| D0 | GPIO16 | **제한적** | PWM/I2C/인터럽트 불가, Deep Sleep 웨이크업 전용 |
| A0 | ADC0 | **사용 가능** | 아날로그 입력 (빈번한 읽기 시 WiFi 간섭 가능) |

**하드웨어 SPI 사용 불가**: GPIO14 (HSPI_CLK), GPIO12 (HSPI_MISO)를 OLED이 점유.

### 배선도 (BME280, I2C 버스 공유)

```
HW-364 보드 (OLED 내장)          BME280 센서 (I2C)
─────────────────────────────    ───────────────────
3V3 ──────────────────────────── VCC
GND ──────────────────────────── GND
GPIO14 (D5) ── OLED SDA ──────── BME280 SDA (버스 공유)
GPIO12 (D6) ── OLED SCL ──────── BME280 SCL (버스 공유)

BME280 I2C 주소: 0x76 (SDO=GND) — OLED 0x3C와 충돌 없음
```

> **주의**: 일부 HW-364 유닛은 SDA/SCL이 뒤바뀌어 있음. 디스플레이가 동작하지 않으면 `Wire.begin(12, 14)` 시도.

## 소프트웨어 아키텍처

```
[main.cpp]
  └── App (Orchestrator)
       ├── TaskScheduler (협력적 멀티태스킹)
       ├── EventBus (pub/sub 모듈간 통신)
       ├── Core Modules
       │    ├── WiFiManager (IotWebConf 3.2.1 captive portal + WiFi scan)
       │    ├── ConfigManager (EEPROM via IotWebConf)
       │    ├── TimeManager (NTP 동기화)
       │    └── OTAManager (무선 펌웨어 업데이트)
       ├── Feature Modules
       │    ├── ClockModule
       │    ├── WeatherModule (OpenWeatherMap)
       │    ├── SensorModule (BME280)
       │    ├── MqttModule (arduino-mqtt / 256dpi)
       │    ├── NotificationModule
       │    ├── ProxyManager (PC 프록시 mDNS 탐색, Dual Mode)
       │    ├── HomeAssistantModule
       │    └── AIModule (구독형 멀티 AI)
       └── UI Layer
            ├── ScreenManager (SimpleFSM)
            └── Screens (Clock, Weather, Sensor, MQTT, Notification, AI, Setup)
```

### 주요 설계 결정

| 결정 | 선택 | 근거 |
|------|------|------|
| C++ 표준 | **C++14** (gnu++14) | C++17은 ESP8266 툴체인에서 불안정 |
| MQTT 라이브러리 | **256dpi/MQTT** | PubSubClient는 IotWebConf와 충돌 (exception 28/29) |
| WiFi 관리 | IotWebConf 3.2.1 | 비차단, TaskScheduler 호환, Captive Portal |
| 디스플레이 라이브러리 | Adafruit SSD1306 | 검증된 호환성 (1KB 프레임버퍼) |
| 멀티태스킹 | TaskScheduler | ESP8266 yield() 호환, 15-18us 오버헤드 |
| 문자열 처리 | char[] + F() 매크로 | String 클래스의 힙 파편화 방지 |
| 메모리 할당 | 정적 할당 only | 런타임 new/malloc 금지 |
| 파일시스템 | LittleFS | SPIFFS deprecated |
| lwIP 변형 | v2 Lower Memory | 메모리 절약, IotWebConf 권장 |

### 메모리 예산

| 구성 요소 | 소비량 |
|-----------|--------|
| WiFi STA (연결 상태) | 20-25KB |
| IotWebConf (웹서버+설정) | 4-8KB |
| SSD1306 프레임버퍼 | 1KB (고정) |
| MQTT 클라이언트 | 1.5-3KB |
| ArduinoJson (일시적) | 1KB/블록 |
| TaskScheduler (10개 태스크) | ~500B |
| BME280 드라이버 | ~300B |
| TLS Handshake (일시적) | 15KB (HTTPS 전용) |
| **남은 가용 힙** | **10-18KB** |

## 의존성

```ini
lib_deps =
    arkhipenko/TaskScheduler@^3.7.0
    bblanchon/ArduinoJson@^7.0.0
    256dpi/MQTT@^2.5.0
    prampec/IotWebConf@^3.2.1
    adafruit/Adafruit SSD1306@^2.5.0
    adafruit/Adafruit BME280 Library@^2.2.0
```

## 개발 환경 설정

### 사전 요구사항

- Python 3.12+
- PlatformIO Core CLI

### 설치

```bash
# PlatformIO 설치 (공식 인스톨러, 격리된 venv 자동 생성)
curl -fsSL -o /tmp/get-platformio.py \
  https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 /tmp/get-platformio.py

# PATH 추가 (zsh)
echo 'export PATH="$HOME/.platformio/penv/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc

# udev 규칙 설치 (ESP8266/CH340)
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/develop/platformio/assets/system/99-platformio-udev.rules \
  | sudo tee /etc/udev/rules.d/99-platformio-udev.rules
sudo udevadm control --reload-rules && sudo udevadm trigger

# 확인
pio --version
pio device list
```

### 빌드 & 업로드

```bash
# Debug 빌드
pio run

# 보드에 업로드
pio run --target upload

# 시리얼 모니터
pio device monitor --baud 115200

# Native 테스트 (PC 실행)
pio test -e native_test

# Embedded 테스트 (실제 장치)
pio test -e embedded_test
```

## 디스플레이 UI (2색 OLED)

노랑 영역(행 0-15) = 상태바, 파랑 영역(행 16-63) = 콘텐츠.

```
시계:                          날씨:
┌────────────────────────┐     ┌────────────────────────┐
│ 22.5C 45% WiFi  [노랑] │     │ Seoul   3C      [노랑] │
├────────────────────────┤     ├────────────────────────┤
│                        │     │                        │
│      14:35:28          │     │  맑음                  │
│    2026-02-28 토       │     │  체감:-1C 습도:35%     │
│                 [파랑] │     │  풍속:3m/s      [파랑] │
└────────────────────────┘     └────────────────────────┘
```

## 로드맵

- **Phase 0**: 개발 환경 + OLED + WiFi 설정 (Captive Portal + WiFi Scan)
- **Phase 1 (MVP)**: EventBus + NTP 시계 + 날씨 + BME280 센서
- **Phase 2**: MQTT 통신 + 알림
- **Phase 3**: PC 프록시 Docker + Home Assistant 연동
- **Phase 4**: 구독형 멀티 AI + Deep Sleep

## 라이선스

MIT

## 참고 자료

- [peff74/esp8266_OLED_HW-364A](https://github.com/peff74/esp8266_OLED_HW-364A) - HW-364A 레퍼런스 구현
- [ESP8266 Pinout Reference](https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/)
- [IotWebConf](https://github.com/prampec/IotWebConf) - WiFi 설정 포털
- [TaskScheduler](https://github.com/arkhipenko/TaskScheduler) - 협력적 멀티태스킹
