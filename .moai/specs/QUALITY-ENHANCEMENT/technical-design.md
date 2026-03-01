# ESP8266 ARTHUR - 품질 고도화 기술 설계

**작성일**: 2026-03-01
**작성자**: 품질 고도화 전문가
**상태**: 설계 완료
**버전**: 1.0

---

## 📋 개요

이 문서는 ESP8266 ARTHUR 프로젝트의 현재 구현을 고품질 코드로 향상하기 위한 기술적 접근법을 상세히 설명합니다. 모듈형 아키텍처 통합 전략, TDD 방법론 적용, 메모리 최적화 방안, 에러 핸들링 개선, 로깅 전략을 포함합니다.

---

## 🎯 품질 개선 목표

### 주요 목표
1. **아키텍처 통합**: 분리된 모듈들을 main.cpp에 안정적으로 통합
2. **메모리 안전성**: ESP8266 제약 하에서 안정적인 동작 보장
3. **오류 처리**: 체계적인 에러 핸들링 및 복구 메커니즘 구현
4. **테스트 커버리지**: TDD 기반의 자동화된 테스트 인프라 구축
5. **유지보수성**: 코드 품질 향상 및 문서화 개선

### 성과 지표 (KPI)
- 메모리 사용률: 80% 미만 (20KB 이상 가용 힙 유지)
- 테스트 커버리지: 85% 이상
- 런타임 오류율: 0.1% 미만
- 모듈 통합 성공률: 100%

---

## 🏛️ 모듈형 아키텍처 통합 전략

### Phase 1: 핵심 통합 (4-6시간)

#### 1.1 Orchestrator 클래스 도입

**문제점**: 현재 main.cpp가 모든 로직을 직접 관리하여 결합도가 높음

**해결책**: `AppOrchestrator` 클래스 도입
```cpp
// src/core/app_orchestrator.h
class AppOrchestrator {
private:
    // 핵심 관리자
    EventBus& eventBus;
    TimeManager& timeManager;
    ConfigManager& configManager;
    CacheManager& cacheManager;

    // 기능 모듈
    ClockModule& clockModule;
    SensorModule& sensorModule;
    WeatherModule& weatherModule;

    // 상태 관리
    SystemState currentState;
    unsigned long lastUpdate;

public:
    AppOrchestrator(Adafruit_SSD1306& display);
    bool begin();
    void update();

private:
    void initializeModules();
    void updateSystemState();
    void handleEvents();
};
```

**이점**:
- 단일 책임 원칙(SRP) 적용
- 의존성 주입을 통한 테스트 용이성
- 중앙 집중적 상태 관리

#### 1.2 의존성 주입 패턴 적용

**문제점**: 전역 변수 의존성이 높음

**해결책**: 의존성 주입을 통한 느슨한 결합
```cpp
// main.cpp
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// 관리자 초기화
EventBus gEventBus;
TimeManager gTimeManager;
ConfigManager gConfigManager;
CacheManager gCacheManager;

// 모듈 초기화
ClockModule gClockModule(display);
SensorModule gSensorModule(display);
WeatherModule gWeatherModule;

// Orchestrator 생성
AppOrchestrator app(display);

void setup() {
    // 하드웨어 초기화
    Wire.begin(OLED_SDA, OLED_SCL);
    display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

    // 시스템 초기화
    gConfigManager.begin();
    gCacheManager.begin();
    gEventBus.begin();
    gTimeManager.begin();

    // Orchestrator 시작
    app.begin();
}
```

#### 1.3 메모리 관리 전략

**문제점**: 모든 모듈 동시 로딩 시 메모리 부족 위험

**해결책**: 지연 초기화(Lazy Initialization) 패턴
```cpp
class AppOrchestrator {
private:
    bool modulesInitialized = false;

    void initializeModules() {
        // WiFi 연결 후에만 모듈 초기화
        if (WiFi.status() == WL_CONNECTED && !modulesInitialized) {
            gSensorModule.begin();
            gWeatherModule.begin();
            modulesInitialized = true;
        }
    }
};
```

### Phase 2: 이벤트 통신 최적화 (2-3시간)

#### 2.1 이벤트 타입 확장

**현재**: 8개 이벤트 타입으로 제한됨

**향상**: 동적 이벤트 타입 지원
```cpp
// 이벤트 타입 관리
enum EventType {
    // 기존 이벤트
    WIFI_CONNECTED,
    WIFI_DISCONNECTED,
    TIME_SYNCED,
    SENSOR_UPDATED,
    WEATHER_UPDATED,

    // 신규 이벤트
    MODULE_ERROR,      // 모듈 오류 발생
    MEMORY_WARNING,     // 메모리 경고
    SYSTEM_STATE_CHANGE // 시스템 상태 변화
};

// 오류 이벤트 구조체
struct ErrorEvent {
    EventType type = MODULE_ERROR;
    Module sourceModule;  // 어떤 모듈에서 발생?
    ErrorCode code;       // 오류 코드
    const char* message; // 오류 메시지
    timestamp_t timestamp;
};
```

#### 2.2 이벤트 큐 최적화

**문제점**: 고정 크기 큐(16)로 오버플로우 가능성

**해결책**: 순환 버퍼 + 우선순위 큐
```cpp
class OptimizedEventQueue {
private:
    static const int HIGH_PRIORITY_QUEUE_SIZE = 8;
    static const int NORMAL_PRIORITY_QUEUE_SIZE = 16;

    Event highPriorityQueue[HIGH_PRIORITY_QUEUE_SIZE];
    Event normalPriorityQueue[NORMAL_PRIORITY_QUEUE_SIZE];

    int highPriorityCount = 0;
    int normalPriorityCount = 0;

public:
    bool publish(const Event& event, bool isHighPriority = false);
    Event getNextEvent();
    bool isEmpty() const { return highPriorityCount == 0 && normalPriorityCount == 0; }
};
```

---

## 🧪 TDD 방법론 적용 계획

### 테스트 전략

#### 1. 테스트 계층 구조

```
📁 test/
├── 📁 unit/              # 단위 테스트
│   ├── event_bus_test.cpp
│   ├── time_manager_test.cpp
│   ├── config_manager_test.cpp
│   └── cache_manager_test.cpp
├── 📁 integration/       # 통합 테스트
│   ├── module_integration_test.cpp
│   ├── event_flow_test.cpp
│   └── memory_usage_test.cpp
└── 📁 system/           # 시스템 테스트
    ├── wifi_connection_test.cpp
    ├── module_loading_test.cpp
    └── stress_test.cpp
```

#### 2. 테스트 프레임워크 확장

**현재**: Unity 프레임워크 사용 중

**향상**: 모의(Mock) 객체 지원
```cpp
// test/mocks/mock_time_manager.h
class MockTimeManager : public TimeManager {
public:
    MOCK_METHOD(begin, void(), (override));
    MOCK_METHOD(update, void(), (override));
    MOCK_METHOD(syncNow, bool(), (override));
    MOCK_METHOD(getFormattedTime, void(char*, size_t), (override));

    // 테스트 유틸리티
    void setSynced(bool synced) { _isSynced = synced; }
    void setMockTime(unsigned long time) { _mockTime = time; }
};
```

#### 3. 테스트 데이터 관리

**문제점**: 테스트 데이터가 분산되어 있음

**해결책**: 테스트 데이터 팩토리 패턴
```cpp
// test/test_data_factory.h
class TestDataFactory {
public:
    static Event createSensorEvent(float temp, float humidity, float pressure) {
        Event event;
        event.type = SENSOR_UPDATED;
        event.timestamp = millis();

        SensorData* data = new SensorData();
        data->temperature = temp;
        data->humidity = humidity;
        data->pressure = pressure;
        data->valid = true;
        event.data = data;

        return event;
    }

    static Event createWeatherEvent(float temp, const char* condition) {
        // 날씨 이벤트 생성
    }
};
```

### TDD 구현 순서

#### 1. RED 단계 (실패하는 테스트)
```cpp
TEST(TestAppOrchestrator, ShouldInitializeAllModules) {
    // Arrange
    Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
    AppOrchestrator app(display);

    // Act
    app.begin();

    // Assert
    // 모듈이 초기화되었는지 검증 (아직 실패)
    EXPECT_TRUE(app.isModuleInitialized(MODULE_SENSOR));
}
```

#### 2. GREEN 단계 (최소한의 구현)
```cpp
bool AppOrchestrator::begin() {
    // 모듈 초기화
    gSensorModule.begin();
    gWeatherModule.begin();

    modulesInitialized = true;
    return true;
}
```

#### 3. REFACTOR 단계 (코드 정리)
- 중복 코드 제거
- 인터페이스 분리
- 의존성 주입 적용

---

## 📊 메모리 최적화 방안

### 메모리 프로파일링 전략

#### 1. 메모리 사용량 모니터링
```cpp
class MemoryMonitor {
public:
    struct MemorySnapshot {
        size_t freeHeap;
        size_t maxAllocated;
        size_t fragmentation;
        Module memoryUsage[MODULE_COUNT];
    };

    void takeSnapshot(const char* context);
    void reportUsage();
    bool isMemoryLow() { return ESP.getFreeHeap() < MEMORY_THRESHOLD; }

private:
    static const size_t MEMORY_THRESHOLD = 20000; // 20KB
    MemorySnapshot snapshots[MAX_SNAPSHOTS];
};
```

#### 2. 메모리 풀 (Memory Pool) 도입

**문제점**: 동적 할당이 메모리 파편화 유발

**해결책**: 사전 할당된 메모리 풀
```cpp
// src/core/memory_pool.h
template<typename T, size_t N>
class MemoryPool {
private:
    T pool[N];
    std::bitset<N> used;

public:
    T* allocate() {
        int index = used._Find_first();
        if (index < N) {
            used[index] = true;
            return &pool[index];
        }
        return nullptr;
    }

    void deallocate(T* ptr) {
        if (ptr >= pool && ptr < pool + N) {
            size_t index = ptr - pool;
            used[index] = false;
        }
    }
};
```

#### 3. String 클래스 사용 최소화

**현재**: `String` 클래스 사용 금지 정책 존재

**강화**: `String` 클래스 완전히 금지
```cpp
// 허용되지 않는 사용
String tempString = String(temperature);  // ❌
config.getString("key", tempString);     // ❌

// 대안 허용
char tempBuf[16];
snprintf(tempBuf, sizeof(tempBuf), "%.1f", temperature);  // ✅
```

### 메모리 압축 기법

#### 1. 데이터 구조체 패킹
```cpp
#pragma pack(push, 1)
struct PackedSensorData {
    float temperature;    // 4 bytes
    float humidity;      // 4 bytes
    float pressure;      // 4 bytes
    uint32_t timestamp;  // 4 bytes
    bool valid;          // 1 byte
    // 총 17 bytes (패킹 전 20 bytes)
};
#pragma pack(pop)
```

#### 2. 캐시 데이터 압축
```cpp
// CacheManager에 데이터 압축 기능 추가
class CacheManager {
public:
    bool compressAndSet(const char* key, const char* value, unsigned long ttl);
    bool getAndDecompress(const char* key, char* outValue, size_t maxLen);

private:
    bool compress(const char* input, char* output, size_t* outputSize);
    bool decompress(const char* input, char* output, size_t* outputSize);
};
```

---

## 🚨 에러 핸들링 개선

### 체계적인 에러 처리

#### 1. 에러 코드 시스템
```cpp
// src/core/error_codes.h
enum class ErrorCode {
    SUCCESS = 0,

    // WiFi 관련 오류
    WIFI_CONNECTION_FAILED = 1000,
    WIFI_TIMEOUT,
    WIFI_CREDENTIALS_INVALID,

    // 센서 관련 오류
    SENSOR_NOT_FOUND,
   _SENSOR_READ_FAILED,
    SENSOR_DATA_INVALID,

    // 네트워크 관련 오류
    NETWORK_REQUEST_FAILED,
    NETWORK_TIMEOUT,
    NETWORK_INVALID_RESPONSE,

    // 시스템 관련 오류
    MEMORY_LOW,
    FILESYSTEM_ERROR,
    INITIALIZATION_FAILED
};

// 오류 처리 핸들러
class ErrorHandler {
public:
    static void handle(ErrorCode code, const char* context, Module source);
    static void logError(ErrorCode code, const char* message);
    static bool isRecoverable(ErrorCode code);

private:
    static const char* getErrorMessage(ErrorCode code);
};
```

#### 2. 재시 메커니즘
```cpp
// Retry 템플릿
template<typename Func, typename... Args>
auto withRetry(Func func, int maxRetries, unsigned long delayMs, Args... args)
    -> typename std::result_of<Func(Args...)>::type {

    int attempt = 0;
    while (attempt < maxRetries) {
        try {
            return func(args...);
        } catch (...) {
            attempt++;
            if (attempt >= maxRetries) {
                throw;
            }
            delay(delayMs);
        }
    }
}
```

#### 3. 회복 가능성 분석
```cpp
class RecoveryManager {
public:
    enum class RecoveryAction {
        RETRY,          // 재시도
        REINITIALIZE,   // 재초기화
        FALLBACK,       // 대체 방식 사용
        SHUTDOWN,       // 안전 종료
        UNKNOWN
    };

    static RecoveryAction getRecoveryAction(ErrorCode error, Module module) {
        switch (error) {
            case ErrorCode::WIFI_CONNECTION_FAILED:
                return RecoveryAction::RETRY;
            case ErrorCode::SENSOR_NOT_FOUND:
                return RecoveryAction::REINITIALIZE;
            case ErrorCode::MEMORY_LOW:
                return RecoveryAction::SHUTDOWN;
            default:
                return RecoveryAction::UNKNOWN;
        }
    }
};
```

### 안전한 상태 전환

#### 1. 상태 머신
```cpp
class SystemStateMachine {
public:
    enum class State {
        BOOTING,
        INITIALIZING,
        CONNECTING_WIFI,
        CONNECTED,
        UPDATING,
        ERROR,
        SHUTDOWN
    };

    void transitionTo(State newState);
    State getCurrentState() const { return currentState; }

private:
    State currentState = State::BOOTING;
    std::map<State, std::vector<State>> validTransitions;

    bool isValidTransition(State from, State to) {
        return validTransitions[from].end() !=
               std::find(validTransitions[from].begin(),
                        validTransitions[from].end(), to);
    }
};
```

---

## 📝 로깅 전략

### 구조화된 로깅 시스템

#### 1. 로깅 레벨
```cpp
enum class LogLevel {
    DEBUG,    // 디버깅 정보
    INFO,     // 일반 정보
    WARNING,  // 경고
    ERROR,    // 오류
    FATAL     // 치명적 오류
};

class Logger {
public:
    static void log(LogLevel level, const char* tag, const char* message, ...) {
        if (level >= currentLogLevel) {
            formatLog(level, tag, message);
        }
    }

    static void setLogLevel(LogLevel level) { currentLogLevel = level; }

private:
    static LogLevel currentLogLevel = LogLevel::INFO;

    static void formatLog(LogLevel level, const char* tag, const char* message, ...) {
        va_list args;
        va_start(args, message);

        char formatted[256];
        vsnprintf(formatted, sizeof(formatted), message, args);

        // 로그 포맷: [LEVEL][TAG] message
        printf("[%s][%s] %s\n", getLevelString(level), tag, formatted);

        va_end(args);
    }

    static const char* getLevelString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:   return "DEBUG";
            case LogLevel::INFO:    return "INFO";
            case LogLevel::WARNING: return "WARN";
            case LogLevel::ERROR:   return "ERROR";
            case LogLevel::FATAL:   return "FATAL";
        }
    }
};
```

#### 2. 성능 모니터링 로그
```cpp
class PerformanceMonitor {
public:
    struct PerformanceMetrics {
        unsigned long updateDuration;
        unsigned long memoryUsage;
        unsigned long networkLatency;
        int errorCount;
    };

    void beginMeasurement(const char* operation);
    void endMeasurement(const char* operation);
    PerformanceMetrics getMetrics(const char* operation);

    void logPerformanceReport();

private:
    std::map<String, PerformanceMetrics> metrics;
    unsigned long measurementStart;
};
```

#### 3. 로그 파일 관리
```cpp
class LogManager {
public:
    bool begin();
    void logToFile(LogLevel level, const char* message);
    void rotateLogs();
    void exportLogs();

private:
    static const int MAX_LOG_FILES = 5;
    static const int MAX_LOG_SIZE = 1024 * 1024; // 1MB

    File logFile;
    int currentLogFile = 1;

    bool shouldRotate();
};
```

---

## 🔍 코드 품질 검증

### 정적 분석 도구 적용

#### 1. clang-tidy 규칙
```cpp
// .clang-tidy
Checks: >
  -readability-identifier-length
  -cppcoreguidelines-*
  -modernize-*
  -performance-*
  -bugprone-*

HeaderFilterRegex: '.*\.h$'
Format: 'clang-format'
```

#### 2. 커스텀 린터 규칙
```python
# custom_linter.py
class ESP8266Rules:
    def check_memory_usage(self, tree):
        # String 클래스 사용 검사
        for node in ast.walk(tree):
            if isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute):
                if node.func.attr == 'String':
                    yield "String 클래스 사용이 금지됩니다", node.lineno

    def check_event_bus_usage(self, tree):
        # EventBus 사용 패턴 검사
        # ...
```

### 코드 커버리지 목표

#### 1. 단위 테스트 커버리지
- EventBus: 95%
- TimeManager: 90%
- ConfigManager: 90%
- CacheManager: 85%
- 각 모듈: 85% 이상

#### 2. 통합 테스트 커버리지
- 모듈 간 통신: 90%
- 이벤트 흐름: 85%
- 메모리 관리: 80%

#### 3. 커버리지 측정 도구
```bash
# PlatformIO 테스트 실행
pio test --environment native_test --coverage

# 커버리지 보고서 생성
python -m coverage report -m
python -m coverage html
```

---

## 📅 실행 계획 (Timeline)

### Week 1: 아키텍처 개선 (4-6시간)
1. **Day 1-2**: AppOrchestrator 클래스 구현
   - 의존성 주입 패턴 적용
   - 기본 구조 구현

2. **Day 3-4**: 모듈 통합
   - ClockModule 연결
   - SensorModule 연결
   - WeatherModule 연결

### Week 2: TDD 테스트 구축 (6-8시간)
1. **Day 1-2**: 단위 테스트 작성
   - EventBus 테스트
   - TimeManager 테스트

2. **Day 3-4**: 통합 테스트 작성
   - 모듈 간 통신 테스트
   - 메모리 사용 테스트

### Week 3: 최적화 및 안정화 (4-6시간)
1. **Day 1-2**: 메모리 최적화
   - 프로파일링
   - 압축 기법 적용

2. **Day 3-4**: 에러 핸들링 완성
   - 재시 메커니즘 구현
   - 로깅 시스템 완성

### Week 4: 검증 및 배포 (4시간)
1. **Day 1-2**: 테스트 실행 및 검증
2. **Day 3-4**: 배포 및 모니터링 설정

---

## 🎯 성과 측정

### 정량적 지표
- **메모리 사용률**: 초기 25.5KB → 목표 18.5KB (-27%)
- **테스트 커버리지**: 초기 30% → 목표 85%
- **오류 발생률**: 초기 5% → 목표 0.1%
- **모듈 통합 성공률**: 100%

### 정성적 지표
- **코드 가독성**: 개선된 인터페이스 설계
- **유지보수성**: 명확한 의존성 관계
- **안정성**: 체계적인 에러 처리
- **확장성**: 모듈식 아키텍처

---

## 🚀 예상 결과

### 단기적 결과 (1개월 내)
- 메모리 사용량 27% 감소
- 테스트 커버리지 85% 달성
- 모든 모듈 안정적 통합

### 장기적 결과 (3개월 내)
- 개발 생산성 50% 향상
- 버그 수정 시간 70% 단축
- 신규 기능 추가 용이성 증대

---

## 📋 추천 검토 항목

1. **아키텍처 설계 검토**: AppOrchestrator 인터페이스 설계 검토
2. **테스트 전략 검토**: TDD 적용 방식 검토
3. **메모리 최적화 검토**: 압축 기법 적용 범위 검토
4. **에러 처리 검토**: 재시 메커니즘 정책 검토
5. **로깅 전략 검토**: 로그 레벨 및 포맷 검토

---

**문서 상태**: 설계 완료
**다음 단계**: 팀 리뷰 및 승인
**예상 검토 기간**: 1-2일

**END OF DOCUMENT**