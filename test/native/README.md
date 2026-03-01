# Native Test Infrastructure

이 디렉토리는 ESP8266 Arduino 프로젝트의 네이티브 테스트 인프라를 포함합니다. 호스트 시스템에서 단위 테스트를 실행하여 빠른 테스트 피드백 루프를 제공합니다.

## 디렉토리 구조

```
test/native/
├── mocks/              # Arduino/ESP8266 라이브러리 모의 객체
│   ├── Arduino.h       # 코어 Arduino 함수 모의
│   ├── Arduino.cpp     # 전역 Serial 인스턴스
│   ├── Wire.h          # I2C 라이브러리 모의
│   ├── Wire.cpp        # 전역 Wire 인스턴스
│   ├── WiFi.h          # WiFi 라이브러리 모의
│   ├── WiFi.cpp        # 전역 WiFi 인스턴스
│   ├── HTTPClient.h    # HTTP 클라이언트 모의
│   ├── Adafruit_SSD1306.h  # OLED 디스플레이 모의
│   └── Adafruit_BME280.h   # BME280 센서 모의
└── test_*.cpp          # Unity 프레임워크 테스트 파일
```

## 사용 방법

### PlatformIO를 통한 테스트 실행

```bash
# 모든 네이티브 테스트 실행
pio test -e native_test

# 특정 테스트 파일만 실행
pio test -e native_test -f test_event_bus

# 상세 출력
pio test -e native_test -v
```

### 테스트 작성

새 테스트 파일을 만들려면:

1. `test/native/test_*.cpp` 파일 생성
2. 테스트 파일에 다음 포함:

```cpp
#ifdef ARTHUR_NATIVE_TEST

#include <unity.h>
#include "Arduino.h"
#include "path/to/module_under_test.h"

// setUp: 각 테스트 전 실행
void setUp(void) {
    // 초기화 코드
}

// tearDown: 각 테스트 후 실행
void tearDown(void) {
    // 정리 코드
}

// 테스트 케이스
void test_feature_x(void) {
    // Arrange-Act-Assert 패턴
    TEST_ASSERT_TRUE(condition);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_feature_x);
    return UNITY_END();
}

#endif // ARTHUR_NATIVE_TEST
```

## Mock 라이브러리 기능

### Arduino.h

- **시간 함수**: `millis()`, `micros()`, `delay()` - 시뮬레이션된 시간
- **Serial 클래스**: `print()`, `println()`, `printf()` - stdout으로 출력
- **String 클래스**: 기본 문자열 래퍼
- **수학 매크로**: `min()`, `max()`, `abs()`, `constrain()`

**테스트 헬퍼**:
```cpp
mock_advance_millis(1000);  // 시간 1초 진행
mock_reset_millis();        // 시간 리셋
```

### Wire.h (I2C)

- **마스터 모드**: `begin()`, `beginTransmission()`, `endTransmission()`
- **데이터 전송**: `write()`, `requestFrom()`, `read()`
- **테스트 헬퍼**:
```cpp
wire.mock_set_rx_buffer(data, length);  // 수신 버퍼 설정
wire.mock_get_tx_buffer();              // 전송 버퍼 확인
```

### WiFi.h

- **연결 관리**: `begin()`, `disconnect()`, `status()`
- **네트워크 정보**: `SSID()`, `RSSI()`, `localIP()`
- **WiFi 스캔**: `scanNetworks()`, `scanSSID()`, `scanDelete()`
- **테스트 헬퍼**:
```cpp
WiFi.mock_set_status(WL_CONNECTED);
WiFi.mock_set_local_ip(0xC0A80101);
```

### HTTPClient.h

- **HTTP 메서드**: `GET()`, `POST()`, `PUT()`, `DELETE()`
- **헤더 관리**: `addHeader()`, `setUserAgent()`
- **응답 처리**: `getString()`, `getResponseCode()`
- **테스트 헬퍼**:
```cpp
http.mock_set_response(HTTP_CODE_OK, "{\"status\":\"ok\"}");
http.mock_set_connected(true);
```

### Adafruit_SSD1306.h (OLED)

- **디스플레이**: `begin()`, `clearDisplay()`, `display()`
- **텍스트**: `setCursor()`, `setTextSize()`, `printf()`
- **그래픽**: `fillRect()`, `drawLine()`, `drawCircle()`
- **테스트 헬퍼**:
```cpp
display.mock_is_initialized();
display.mock_get_cursor_x();
display.mock_get_text_size();
```

### Adafruit_BME280.h (Sensor)

- **센서 읽기**: `readTemperature()`, `readHumidity()`, `readPressure()`
- **고도 계산**: `readAltitude()`, `seaLevelForAltitude()`
- **테스트 헬퍼**:
```cpp
sensor.mock_set_temperature(25.0f);
sensor.mock_set_humidity(50.0f);
sensor.mock_set_pressure(1013.25f);
```

## TDD 워크플로우

1. **RED**: 실패하는 테스트 작성
   ```bash
   pio test -e native_test  # 실패 확인
   ```

2. **GREEN**: 최소 구현으로 테스트 통과
   ```bash
   pio test -e native_test  # 성공 확인
   ```

3. **REFACTOR**: 코드 개선
   ```bash
   pio test -e native_test  # 계속 통과 확인
   ```

## PlatformIO 설정

`platformio.ini`의 `[env:native_test]` 섹션:

```ini
[env:native_test]
platform = native
build_flags =
    -std=c++14
    -DARTHUR_NATIVE_TEST=1
    -I test/native/mocks
test_filter = native/*
lib_deps =
    bblanchon/ArduinoJson@^7.0.0
```

## 현재 테스트 커버리지

- [x] `test_event_bus.cpp` - EventBus pub/sub 시스템
- [ ] `test_config_manager.cpp` - 설정 관리자
- [ ] `test_time_manager.cpp` - 시간 관리자
- [ ] `test_cache_manager.cpp` - 캐시 관리자
- [ ] `test_sensor_module.cpp` - 센서 모듈
- [ ] `test_weather_module.cpp` - 날씨 모듈
- [ ] `test_clock_module.cpp` - 시계 모듈

## 문제 해결

### 컴파일 오류: "Arduino.h not found"

**원인**: `ARTHUR_NATIVE_TEST` 매크로가 정의되지 않음

**해결**: `platformio.ini`에 `build_flags` 확인:
```ini
build_flags =
    -DARTHUR_NATIVE_TEST=1
    -I test/native/mocks
```

### 링크 오류: "undefined reference to Serial"

**원인**: `Arduino.cpp`가 링크되지 않음

**해결**: 테스트 파일에 `#include "Arduino.h"`가 있는지 확인

### 테스트가 크래시함

**원인**: 초기화되지 않은 전역 객체 접근

**해결**: 테스트의 `setUp()`에서 적절한 초기화 수행

## 추가 리소스

- [PlatformIO Unit Testing](https://docs.platformio.org/en/latest/advanced/unit-testing/index.html)
- [Unity Test Framework](https://github.com/ThrowTheSwitch/Unity)
- [MoAI TDD Workflow](/.claude/skills/moai-workflow-tdd)
