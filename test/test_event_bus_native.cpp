// @MX:NOTE: [TEST] Native EventBus tests with direct source inclusion
// PlatformIO Unity framework를 사용한 네이티브 테스트

#include <unity.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdarg>

// 네이티브 테스트용 Arduino 모의 정의
#define ARTHUR_NATIVE_TEST 1

// mock_millis 카운터 정의
unsigned long mock_millis_counter = 0;
unsigned long mock_micros_counter = 0;

// Arduino 기본 타입 정의
using byte = uint8_t;
using boolean = bool;

// Arduino 매크로
#define F(string_literal) (string_literal)
#define PROGMEM

// Serial 클래스 모의
class HardwareSerial {
public:
    void begin(unsigned long) {}
    void print(const char* str) { if (str) printf("%s", str); }
    void print(int val) { printf("%d", val); }
    void print(unsigned int val) { printf("%u", val); }
    void print(long val) { printf("%ld", val); }
    void print(unsigned long val) { printf("%lu", val); }
    void println(const char* str) { printf("%s\n", str ? str : ""); }
    void println(int val) { printf("%d\n", val); }
    void println(unsigned int val) { printf("%u\n", val); }
    void println(long val) { printf("%ld\n", val); }
    void println(unsigned long val) { printf("%lu\n", val); }
    void println() { printf("\n"); }
    void printf(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
    int available() { return 0; }
    int read() { return -1; }
};
HardwareSerial Serial;

inline unsigned long millis() { return mock_millis_counter; }
inline void mock_reset_millis() { mock_millis_counter = 0; mock_micros_counter = 0; }
inline void mock_advance_millis(unsigned long ms) { mock_millis_counter += ms; }

// ==========================================
// EventBus 소스 직접 포함 (event_bus.h 내용)
// ==========================================

// 최대 이벤트 타입 수
#define MAX_EVENT_TYPES 8

// 최대 구독자 수 (이벤트 타입당)
#define MAX_SUBSCRIBERS 4

// 이벤트 타입 열거형
enum EventType {
    WIFI_CONNECTED,
    WIFI_DISCONNECTED,
    TIME_SYNCED,
    SENSOR_UPDATED,
    WEATHER_UPDATED,
    EVENT_TYPE_RESERVED_1,
    EVENT_TYPE_RESERVED_2,
    EVENT_TYPE_COUNT
};

// 이벤트 데이터 구조체
struct Event {
    EventType type;
    unsigned long timestamp;
    const void* data;
};

// 이벤트 콜백 함수 포인터 타입
typedef void (*EventCallback)(const Event& event, void* userData);

// 구독자 정보 구조체
struct Subscriber {
    EventCallback callback;
    void* userData;
};

// EventBus 클래스
class EventBus {
public:
    EventBus();
    ~EventBus() = default;

    void begin();
    bool subscribe(EventType type, EventCallback callback, void* userData = nullptr);
    bool publish(const Event& event);
    int update();
    void unsubscribe(EventType type, EventCallback callback);
    void clear();

private:
    Subscriber _subscribers[MAX_EVENT_TYPES][MAX_SUBSCRIBERS];
    static const int EVENT_QUEUE_SIZE = 16;
    Event _eventQueue[EVENT_QUEUE_SIZE];
    int _queueHead;
    int _queueTail;
    int _queueCount;
    bool _initialized;

    void dispatchEvent(const Event& event);
};

// 전역 인스턴스
EventBus gEventBus;

// ==========================================
// EventBus 구현 (event_bus.cpp 내용)
// ==========================================

EventBus::EventBus()
    : _queueHead(0)
    , _queueTail(0)
    , _queueCount(0)
    , _initialized(false)
{
    for (int type = 0; type < MAX_EVENT_TYPES; type++) {
        for (int i = 0; i < MAX_SUBSCRIBERS; i++) {
            _subscribers[type][i].callback = nullptr;
            _subscribers[type][i].userData = nullptr;
        }
    }
}

void EventBus::begin() {
    _queueHead = 0;
    _queueTail = 0;
    _queueCount = 0;
    _initialized = true;
    Serial.println(F("EventBus initialized"));
}

bool EventBus::subscribe(EventType type, EventCallback callback, void* userData) {
    if (type < 0 || type >= EVENT_TYPE_COUNT) return false;
    if (callback == nullptr) return false;

    for (int i = 0; i < MAX_SUBSCRIBERS; i++) {
        if (_subscribers[type][i].callback == nullptr) {
            _subscribers[type][i].callback = callback;
            _subscribers[type][i].userData = userData;
            return true;
        }
    }

    Serial.print(F("EventBus: MAX_SUBSCRIBERS reached for type "));
    Serial.println(type);
    return false;
}

bool EventBus::publish(const Event& event) {
    if (event.type < 0 || event.type >= EVENT_TYPE_COUNT) return false;
    if (_queueCount >= EVENT_QUEUE_SIZE) {
        Serial.println(F("EventBus: Event queue full"));
        return false;
    }

    _eventQueue[_queueTail] = event;
    _eventQueue[_queueTail].timestamp = millis();
    _queueTail = (_queueTail + 1) % EVENT_QUEUE_SIZE;
    _queueCount++;

    return true;
}

int EventBus::update() {
    if (!_initialized) return 0;

    int processed = 0;
    while (_queueCount > 0) {
        Event event = _eventQueue[_queueHead];
        _queueHead = (_queueHead + 1) % EVENT_QUEUE_SIZE;
        _queueCount--;
        dispatchEvent(event);
        processed++;
    }

    return processed;
}

void EventBus::unsubscribe(EventType type, EventCallback callback) {
    if (type < 0 || type >= EVENT_TYPE_COUNT) return;

    for (int i = 0; i < MAX_SUBSCRIBERS; i++) {
        if (_subscribers[type][i].callback == callback) {
            _subscribers[type][i].callback = nullptr;
            _subscribers[type][i].userData = nullptr;
            return;
        }
    }
}

void EventBus::clear() {
    for (int type = 0; type < MAX_EVENT_TYPES; type++) {
        for (int i = 0; i < MAX_SUBSCRIBERS; i++) {
            _subscribers[type][i].callback = nullptr;
            _subscribers[type][i].userData = nullptr;
        }
    }
    _queueHead = 0;
    _queueTail = 0;
    _queueCount = 0;
}

void EventBus::dispatchEvent(const Event& event) {
    for (int i = 0; i < MAX_SUBSCRIBERS; i++) {
        Subscriber& sub = _subscribers[event.type][i];
        if (sub.callback != nullptr) {
            sub.callback(event, sub.userData);
        }
    }
}

// ==========================================
// 테스트 코드
// ==========================================

// 테스트용 전역 변수
static int callbackCallCount = 0;
static EventType lastEventType = EVENT_TYPE_COUNT;
static const void* lastUserData = nullptr;

// 테스트용 콜백
void testCallback(const Event& event, void* userData) {
    callbackCallCount++;
    lastEventType = event.type;
    lastUserData = userData;
}

void setUp(void) {
    mock_reset_millis();
    callbackCallCount = 0;
    lastEventType = EVENT_TYPE_COUNT;
    lastUserData = nullptr;
}

void tearDown(void) {}

// 테스트 케이스
void test_event_bus_initialization(void) {
    EventBus bus;
    bus.begin();
    TEST_ASSERT_TRUE(bus.update() >= 0);
}

void test_event_bus_subscribe_publish(void) {
    EventBus bus;
    bus.begin();

    bus.subscribe(WIFI_CONNECTED, testCallback, nullptr);

    Event e;
    e.type = WIFI_CONNECTED;
    e.data = nullptr;
    bus.publish(e);
    bus.update();

    TEST_ASSERT_EQUAL_INT(1, callbackCallCount);
    TEST_ASSERT_EQUAL_INT(WIFI_CONNECTED, lastEventType);
}

void test_event_bus_multiple_subscribers(void) {
    EventBus bus;
    bus.begin();

    bus.subscribe(WIFI_CONNECTED, testCallback, nullptr);
    bus.subscribe(WIFI_CONNECTED, testCallback, (void*)1);
    bus.subscribe(WIFI_CONNECTED, testCallback, (void*)2);

    Event e;
    e.type = WIFI_CONNECTED;
    e.data = nullptr;
    bus.publish(e);
    bus.update();

    TEST_ASSERT_EQUAL_INT(3, callbackCallCount);
}

void test_event_bus_queue_overflow(void) {
    EventBus bus;
    bus.begin();
    bus.subscribe(WIFI_CONNECTED, testCallback, nullptr);

    Event e;
    e.type = WIFI_CONNECTED;
    e.data = nullptr;

    // 큐 크기(16)보다 많은 이벤트 발행
    for (int i = 0; i < 20; i++) {
        bool result = bus.publish(e);
        if (i < 16) {
            TEST_ASSERT_TRUE(result);
        } else {
            TEST_ASSERT_FALSE(result);
        }
    }
}

void test_event_bus_max_subscribers(void) {
    EventBus bus;
    bus.begin();

    // MAX_SUBSCRIBERS(4)만큼 구독 성공
    TEST_ASSERT_TRUE(bus.subscribe(WIFI_CONNECTED, testCallback, nullptr));
    TEST_ASSERT_TRUE(bus.subscribe(WIFI_CONNECTED, testCallback, (void*)1));
    TEST_ASSERT_TRUE(bus.subscribe(WIFI_CONNECTED, testCallback, (void*)2));
    TEST_ASSERT_TRUE(bus.subscribe(WIFI_CONNECTED, testCallback, (void*)3));

    // 5번째 구독은 실패해야 함
    TEST_ASSERT_FALSE(bus.subscribe(WIFI_CONNECTED, testCallback, (void*)4));
}

void test_event_bus_clear(void) {
    EventBus bus;
    bus.begin();

    bus.subscribe(WIFI_CONNECTED, testCallback, nullptr);
    bus.clear();

    Event e;
    e.type = WIFI_CONNECTED;
    e.data = nullptr;
    bus.publish(e);
    bus.update();

    // clear() 후에는 콜백이 호출되지 않아야 함
    TEST_ASSERT_EQUAL_INT(0, callbackCallCount);
}

void test_event_bus_unsubscribe(void) {
    EventBus bus;
    bus.begin();

    bus.subscribe(WIFI_CONNECTED, testCallback, nullptr);
    bus.unsubscribe(WIFI_CONNECTED, testCallback);

    Event e;
    e.type = WIFI_CONNECTED;
    e.data = nullptr;
    bus.publish(e);
    bus.update();

    TEST_ASSERT_EQUAL_INT(0, callbackCallCount);
}

void test_event_bus_user_data(void) {
    EventBus bus;
    bus.begin();

    int userData = 42;
    bus.subscribe(WIFI_CONNECTED, testCallback, &userData);

    Event e;
    e.type = WIFI_CONNECTED;
    e.data = nullptr;
    bus.publish(e);
    bus.update();

    TEST_ASSERT_EQUAL_PTR(&userData, lastUserData);
}

void test_event_bus_invalid_type(void) {
    EventBus bus;
    bus.begin();

    // 잘못된 타입으로 구독 시도
    TEST_ASSERT_FALSE(bus.subscribe((EventType)-1, testCallback, nullptr));
    TEST_ASSERT_FALSE(bus.subscribe(EVENT_TYPE_COUNT, testCallback, nullptr));

    // 잘못된 타입으로 발행 시도
    Event e;
    e.type = (EventType)-1;
    e.data = nullptr;
    TEST_ASSERT_FALSE(bus.publish(e));
}

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_event_bus_initialization);
    RUN_TEST(test_event_bus_subscribe_publish);
    RUN_TEST(test_event_bus_multiple_subscribers);
    RUN_TEST(test_event_bus_queue_overflow);
    RUN_TEST(test_event_bus_max_subscribers);
    RUN_TEST(test_event_bus_clear);
    RUN_TEST(test_event_bus_unsubscribe);
    RUN_TEST(test_event_bus_user_data);
    RUN_TEST(test_event_bus_invalid_type);

    return UNITY_END();
}
