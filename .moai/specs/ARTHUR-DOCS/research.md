# ARTHUR ESP8266 프로젝트 코드베이스 연구 보고서

## 개요

ARTHUR은 ESP8266 HW-364 보드를 기반으로 한 Autonomous Real-Time Home Utility Responder (AttoClaw) 프로젝트입니다. 극도로 제한된 메모리 환경에서 동작하도록 설계되었으며, 현재 Phase 0 구현이 완료되었습니다.

---

## 1. 프로젝트 구조

### 디렉토리 구조
```
/home/drake/workspace/esp8266/
├── src/
│   └── main.cpp                 # (314 lines) 메인 응용 프로그램
├── include/
│   ├── arthur_config.h         # (48 lines) 컴파일 타임 설정
│   └── arthur_pins.h           # (43 lines) 하드웨어 핀 정의
├── data/                       # 데이터 저장소 (현재 비어 있음)
├── docs/                       # 문서 디렉토리
├── test/                       # 테스트 디렉토리 (현재 비어 있음)
├── .pio/                      # PlatformIO 빌드 결과
├── platformio.ini              # PlatformIO 설정
└── README.md                  # 프로젝트 문서
```

### 파일 크기 및 복잡도
- **main.cpp**: 314 lines (함수 12개, 클래스 1개)
- **arthur_config.h**: 48 lines (매크로 정의)
- **arthur_pins.h**: 43 lines (핀 정의 및 주석)

---

## 2. 핵심 아키텍처 분석

### 2.1 메인 아키텍처 (main.cpp)

**설계 패턴**: 단일 파일 모놀리식 구조 (Phase 0)
- **장점**: ESP8266의 제한된 환경에서 초기 개발 속도
- **단점**: 확장성 제한 (향후 모듈 분리 필요)

**주요 컴포넌트**:
1. **OLED 디스플레이 관리자** (lines 11, 69-92)
   - Adafruit_SSD1306 래퍼
   - 2색 영역 구분 (노랑 상단/파랑 하단)
   - 상태 표시 시스템

2. **IotWebConf 통합** (lines 13-62)
   - Captive Portal 구현
   - 커스텀 HTML/JS 통합 (WiFi 스캔 드롭다운)
   - AP 모드 강제 진입 메커니즘

3. **상태 기반 화면 시스템** (lines 93-142)
   - Boot, AP Mode, Connecting, Connected, OffLine
   - 상태 전환 시 화면 업데이트

4. **웹 핸들러** (lines 163-204)
   - WiFi 스캔 (/scan)
   - Captive Portal 처리
   - 간단한 상태 페이지

### 2.2 하드웨어 추상화 (arthur_pins.h)

**핀 할당 전략**:
- **OLED I2C**: GPIO14 (SDA), GPIO12 (SCL) - 비표준 핀
- **외부 센서**: 동일 I2C 버스 공유 (GPIO14/12)
- **제어 핀**: BUTTON_PIN=0, LED_PIN=2, BUZZER_PIN=13

**중요 고려사항**:
- OLED 핀 할당으로 SPI 사용 불가 (GPIO12/14 점유)
- I2C 버스 공유로 BME280 연결 가능 (0x76 주소)
- FLASH 버튼을 통한 설정 리셋 메커니즘

### 2.3 컴파일 타임 설정 (arthur_config.h)

**메모리 관리 설정**:
- **HEAP_SAFETY_MARGIN**: 9KB (ESP8266 가용 힙 10-18KB 대비)
- **버퍼 크기 계층화**: SMALL_BUF=32, MEDIUM_BUF=64, LARGE_BUF=128
- **String 클래스 대신 고정 버퍼 사용**

**타이밍 설정**:
- **WiFi 연결 타임아웃**: 15초
- **센서 읽기 주기**: 5초
- **날씨 업데이트 주기**: 10분
- **MQTT 킵얼라이브**: 30초

---

## 3. 기술 스택 및 의존성

### 3.1 PlatformIO 설정 (platformio.ini)

**핵심 설정**:
- **플랫폼**: espressif8266
- **보드**: nodemcuv2
- **프레임워크**: Arduino
- **C++ 표준**: gnu++14 (ESP8266 툴체인 안정성)
- **Flash**: 1MB (HW-364 실제 용량)
- **lwIP**: v2 Lower Memory (메모리 최적화)

**라이브러리 의존성**:
1. **TaskScheduler@^3.7.0**: 협력적 멀티태스킹
2. **ArduinoJson@^7.0.0**: JSON 처리
3. **MQTT@^2.5.0**: MQTT 통신
4. **IotWebConf@^3.2.1**: WiFi 설정 관리
5. **Adafruit SSD1306@^2.5.0**: OLED 디스플레이
6. **Adafruit BME280 Library@^2.2.0**: 환경 센서

### 3.2 메모리 사용 현황 (Build 결과)

| 구성 요소 | Flash 사용량 | RAM 사용량 | 메모리 비율 |
|-----------|-------------|------------|-------------|
| 전체 프로그램 | 365KB (38.1%) | 31.7KB (38.8%) | - |
| 가용 힙 | - | ~45KB (연결 후) | 55% |

---

## 4. 구현된 기능 (Phase 0 완료)

### 4.1 WiFi 관리 시스템

**IotWebConf 통합 특징**:
- **Captive Portal**: 자동 AP 생성 (ARTHUR-Setup)
- **WiFi 스캔 드롭다운**: 설정 페이지 내 JavaScript 구현
- **상태 콜백**: WiFi 연결 시점의 OLED 업데이트
- **AP 모드 강제 진입**: GPIO0 버튼을 통한 설정 리셋

**메모리 최적화 기법**:
- WiFi 스캔 결과 직접 JSON 생성 (ArduinoJson 미사용)
- PROGMEM으로 JavaScript 코드 저장
- 상태 변경 시만 OLED 업데이트

### 4.2 OLED 디스플레이 시스템

**2색 영역 구조**:
```cpp
// 상단 16px: 노랑 (상태 정보)
#define OLED_YELLOW_BOTTOM 15
// 하단 48px: 파랑 (콘텐츠)
#define OLED_BLUE_TOP 16
```

**화면 상태 시스템**:
1. **Boot Screen**: 프로젝트 정보 표시
2. **AP Mode Screen**: 설정 모드 안내
3. **Connecting Screen**: WiFi 연결 중
4. **Connected Screen**: IP/SSID/힙 정보
5. **OffLine Screen**: 재연결 시도

### 4.3 에러 핸들링

**주요 에러 상황**:
1. **OLED 초기화 실패**: SDA/SCL 교체 안내 (line 221-225)
2. **WiFi 스캔 실패**: 15개 네트워크 제한 (line 169)
3. **메모리 부족 힙 로깅**: 30초 간격 (line 304)

---

## 5. 메모리 제약 및 최적화

### 5.1 ESP8266 하드웨어 제약

**기사양**:
- SoC: ESP8266EX (Tensilica LX106, 80/160MHz)
- RAM: ~50KB SRAM (WiFi + 라이브러리 로드 후 가용 10-18KB)
- Flash: 1MB (HW-364 실제 용량)
- GPIO: 17개 (일부는 특별용도로 제한됨)

### 5.2 메모리 최적화 전략

**1. 힙 관리**:
- String 클래스 사용 금지 (힙 파편화 방지)
- 고정 크기 버퍼 사용 (char[] 배열)
- 동적 할당(new/malloc) 금지

**2. 코드 최적화**:
- F() 매크로로 Flash 메모리 이동
- 프로시저 호출 최소화
- 상태 기반 렌더링 (불필요한 갱신 방지)

**3. 라이브러리 선택**:
- lwIP v2 Lower Memory (대표 메모리 절약)
- 경량 MQTT 클라이언트 (256dpi 버전)
- 검증된 OLED 라이브러리 (Adafruit SSD1306)

---

## 6. 확장성을 위한 아키텍처 고려사항

### 6.1 현재 한계점

**Phase 0 한계**:
- 단일 파일 구조로 확장성 제한
- 모듈 간 결합도 높음
- 테스트 코드 부재
- 에러 핸들링 부족

### 6.2 향후 아키텍처 방향

**계획된 모듈화**:
```
App (main.cpp)
├── Core Modules
│   ├── WiFiManager (IotWebConf 기반)
│   ├── ConfigManager (EEPROM)
│   ├── TimeManager (NTP)
│   └── TaskScheduler (기반)
├── Feature Modules
│   ├── ClockModule
│   ├── WeatherModule (OpenWeatherMap)
│   ├── SensorModule (BME280)
│   ├── MqttModule
│   ├── NotificationModule
│   └── AIModule
└── UI Layer
    ├── ScreenManager (FSM)
    └── Screens (각 화면 클래스)
```

### 6.3 참고 구현 예시

**TaskScheduler 통합 패턴**:
```cpp
// 예시: 센서 읽기 태스크
Scheduler sensorTask(5000);  // 5초 주기
void sensorReadCallback() {
    // BME280 데이터 읽기
    // 데이터 버스 업데이트
}
```

**MQTT 통합 패턴**:
```cpp
// 예시: MQTT 클라이언트 초기화
MQTTClient mqttClient(256);  // 256바uffer 크기
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // 메시지 처리
}
```

---

## 7. 테스트 및 검증

### 7.1 현재 테스트 상태

- **단위 테스트**: 없음
- **통합 테스트**: 없음
- **하드웨어 테스트**: OLED, WiFi, 버튼 동작 확인
- **빌드 테스트**: PlatformIO 통과

### 7.2 권장 테스트 전략

**TDD 모드 적용** (quality.yaml 참조):
1. **RED**: 메모리 부족 상태 테스트
2. **GREEN**: 최소 기능 구현
3. **REFACTOR**: 메모리 최적화

**테스트 범위**:
- WiFi 연결/연결 끊김 상태 전환
- OLED 화면 업데이트 성능
- 메모리 누수 테스트
- I2C 버스 충돌 시나리오

---

## 8. 주요 발견 및 권장사항

### 8.1 강점

1. **메모리 효율성**: ESP8266 제약을 고려한 설계
2. **직관적 UI**: 2색 OLED로 상태 표시 명확
3. **실용적 WiFi 설정**: Captive Portal + 스캔 드롭다운
4. **검증된 라이브러리 선택**: 안정성 보장

### 8.2 개선 영역

1. **코드 구조**: 모듈화 필요 (향후 Phase 1+)
2. **테스트 커버리지**: TDD 적용이 시급
3. **에러 핸들링**: 상세한 에러 코드 시스템 필요
4. **문서화**: API 문서 및 주석 보강

### 8.3 잠재적 위험

1. **메모리 부족**: 10-18KB 가용 힙에서 확장 시 리스크
2. **I2C 버스 충돌**: 여러 센서 연결 시 주소 관리 필요
3. **ESP8266 제약**: Deep Sleep, WiFi 간섭 문제

---

## 9. 참조 구현

### 9.1 IotWebConf 통합 패턴

**커스텀 HTML 제공자** (lines 54-62):
```cpp
class ArthurHtmlFormatProvider : public iotwebconf::HtmlFormatProvider {
    String getScriptInner() override {
        return HtmlFormatProvider::getScriptInner() + FPSTR(SCAN_SCRIPT);
    }
};
```

### 9.2 메모리 안전 마진

**HEAP_SAFETY_MARGIN**: 9KB 설정 (line 15, arthur_config.h)
- ESP8266 가용 힙 10-18KB 대비 50% 할당
- MQTT, TLS 등 임시 사용 고려

---

## 결론

ARTHUR 프로젝트는 ESP8266의 엄격한 제약 조건에서 안정적인 Phase 0 구현을 완료했습니다. 메모리 효율적인 설계와 실용적인 WiFi 설정 시스템이 돋보이며, 향후 모듈화를 통한 기능 확장이 가능한 구조입니다. TDD 적용과 테스트 커버리지 확보가 다음 단계의 핵심 과제입니다.

---
**문서 생성**: 2026-02-28
**상태**: Phase 0 완료
**다음 단계**: 모듈화 (Phase 1), 테스트 커버리지 확보