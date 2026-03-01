# Native Test Quick Start

빠른 시작 가이드 - 네이티브 테스트 인프라 사용법

## 30초 요약

```bash
# 1. 설치 확인
./test/native/test_setup_helper.sh

# 2. 테스트 실행
pio test -e native_test

# 3. 결과 확인
# 테스트가 통과하면 새로운 기능 구현 가능!
```

## 새 테스트 작성법

### 1단계: 테스트 파일 생성

`test/native/test_my_module.cpp`:

```cpp
#ifdef ARTHUR_NATIVE_TEST

#include <unity.h>
#include "Arduino.h"
#include "core/my_module.h"

void setUp(void) {
    // 각 테스트 전 초기화
}

void tearDown(void) {
    // 각 테스트 후 정리
}

void test_my_feature(void) {
    // Arrange - 준비
    MyModule module;
    module.begin();

    // Act - 실행
    int result = module.doSomething(42);

    // Assert - 검증
    TEST_ASSERT_EQUAL(42, result);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_my_feature);
    return UNITY_END();
}

#endif // ARTHUR_NATIVE_TEST
```

### 2단계: 테스트 실행

```bash
pio test -e native_test -f test_my_module
```

### 3단계: 결과 확인

```
=== Native Test Environment ===
test_my_module:OK [0.123s]
-------------------
1 Tests 0 Failures 0 Ignored
OK
```

## Mock 사용 예시

### WiFi Mock

```cpp
void test_wifi_connection() {
    // WiFi 연결 상태 설정
    WiFi.mock_set_status(WL_CONNECTED);
    WiFi.mock_set_local_ip(0xC0A80101); // 192.168.1.1

    // 테스트 코드
    TEST_ASSERT_EQUAL(WL_CONNECTED, WiFi.status());
}
```

### Sensor Mock

```cpp
void test_sensor_reading() {
    Adafruit_BME280 sensor;
    sensor.begin();

    // 센서 값 설정
    sensor.mock_set_temperature(25.0f);
    sensor.mock_set_humidity(50.0f);

    // 테스트 코드
    float temp = sensor.readTemperature();
    TEST_ASSERT_EQUAL_FLOAT(25.0f, temp);
}
```

### Time Mock

```cpp
void test_timeout() {
    mock_reset_millis();

    // 초기 상태
    TEST_ASSERT_EQUAL(0, millis());

    // 시간 진행
    mock_advance_millis(1000);
    TEST_ASSERT_EQUAL(1000, millis());
}
```

## 자주 쓰는 Unity 매크로

```cpp
// 기본
TEST_ASSERT_TRUE(condition);
TEST_ASSERT_FALSE(condition);
TEST_ASSERT_EQUAL(expected, actual);
TEST_ASSERT_NOT_EQUAL(expected, actual);

// 숫자
TEST_ASSERT_EQUAL_FLOAT(expected, actual);
TEST_ASSERT_EQUAL_DOUBLE(expected, actual);
TEST_ASSERT_GREATER_THAN(threshold, value);

// 포인터
TEST_ASSERT_EQUAL_PTR(expected, actual);
TEST_ASSERT_NULL(pointer);
TEST_ASSERT_NOT_NULL(pointer);

// 문자열
TEST_ASSERT_EQUAL_STRING(expected, actual);
```

## 문제 해결

### 컴파일 오류

```
fatal error: Arduino.h: No such file or directory
```

**해결:** `#ifdef ARTHUR_NATIVE_TEST` 가 있는지 확인

### 링크 오류

```
undefined reference to `Serial'
```

**해결:** `#include "Arduino.h"`가 있는지 확인

### 테스트가 크래시함

**해결:** `setUp()`에서 적절한 초기화 수행

## 참고 자료

- [자세한 문서](test/native/README.md)
- [구현 요약](test/native/IMPLEMENTATION_SUMMARY.md)
- [TDD 워크플로우](.claude/skills/moai-workflow-tdd)

## 질문?

```bash
# 도움말 보기
pio test --help

# 상세 출력
pio test -e native_test -v
```

---

**TDD 원칙:** 테스트가 실패하기 전에 새로운 코드를 작성하지 마세요!
