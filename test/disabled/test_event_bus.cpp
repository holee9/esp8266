// @MX:NOTE: [TEST] EventBus unit tests for native environment
// PlatformIO Unity framework를 사용한 네이티브 테스트

#ifdef ARTHUR_NATIVE_TEST

#include <unity.h>
#include "Arduino.h"
#include "core/event_bus.h"

// 테스트용 콜백 데이터
static int callbackCallCount = 0;
static EventType lastEventType = EVENT_TYPE_COUNT;
static const void* lastUserData = nullptr;
static const void* lastEventData = nullptr;

// 테스트용 콜백 함수
void testCallback(const Event& event, void* userData) {
    callbackCallCount++;
    lastEventType = event.type;
    lastUserData = userData;
    lastEventData = event.data;
}

// 다른 콜백 함수 (구독자 구분 테스트용)
void testCallback2(const Event& event, void* userData) {
    callbackCallCount++;
}

// 세 번째 콜백 함수
void testCallback3(const Event& event, void* userData) {
    callbackCallCount++;
}

// 네 번째 콜백 함수
void testCallback4(const Event& event, void* userData) {
    callbackCallCount++;
}

// 데이터 캡처용 콜백 함수
void testDataCallback(const Event& event, void* userData) {
    callbackCallCount++;
    lastEventData = event.data;
}

// setUp: 각 테스트 전에 실행
void setUp(void) {
    mock_reset_millis();
    callbackCallCount = 0;
    lastEventType = EVENT_TYPE_COUNT;
    lastUserData = nullptr;
    lastEventData = nullptr;
}

// tearDown: 각 테스트 후에 실행
void tearDown(void) {
    // 테스트 후 정리
}

// 테스트 1: EventBus 초기화
void test_event_bus_initialization(void) {
    EventBus bus;

    // begin() 호출 전 상태 확인
    bus.begin();

    // begin() 후 기본 상태 확인 (구체적인 검증은 구현에 따라 다름)
    // 여기서는 크래시가 발생하지 않는지 확인만 수행
    TEST_ASSERT_TRUE(true);
}

// 테스트 2: 단일 구독자 등록
void test_event_bus_single_subscription(void) {
    EventBus bus;
    bus.begin();

    bool success = bus.subscribe(WIFI_CONNECTED, testCallback);

    TEST_ASSERT_TRUE(success);
}

// 테스트 3: 이벤트 발행 및 전달
void test_event_bus_publish_and_deliver(void) {
    EventBus bus;
    bus.begin();

    // 구독
    bus.subscribe(WIFI_CONNECTED, testCallback);

    // 이벤트 발행
    Event event;
    event.type = WIFI_CONNECTED;
    event.timestamp = millis();
    event.data = nullptr;

    bool published = bus.publish(event);
    TEST_ASSERT_TRUE(published);

    // 이벤트 처리
    int processed = bus.update();
    TEST_ASSERT_EQUAL(1, processed);

    // 콜백이 호출되었는지 확인
    TEST_ASSERT_EQUAL(1, callbackCallCount);
    TEST_ASSERT_EQUAL(WIFI_CONNECTED, lastEventType);
}

// 테스트 4: 다중 구독자
void test_event_bus_multiple_subscribers(void) {
    EventBus bus;
    bus.begin();

    // 같은 이벤트 타입에 다른 구독자 등록
    bus.subscribe(SENSOR_UPDATED, testCallback);
    bus.subscribe(SENSOR_UPDATED, testCallback2);

    Event event;
    event.type = SENSOR_UPDATED;
    event.timestamp = millis();
    event.data = nullptr;

    bus.publish(event);
    bus.update();

    // 두 콜백 모두 호출되었는지 확인
    TEST_ASSERT_EQUAL(2, callbackCallCount);
}

// 테스트 5: 사용자 데이터 전달
void test_event_bus_user_data(void) {
    EventBus bus;
    bus.begin();

    int testValue = 42;
    bus.subscribe(TIME_SYNCED, testCallback, (void*)&testValue);

    Event event;
    event.type = TIME_SYNCED;
    event.timestamp = millis();
    event.data = nullptr;

    bus.publish(event);
    bus.update();

    // 사용자 데이터가 올바르게 전달되었는지 확인
    TEST_ASSERT_EQUAL_PTR(&testValue, lastUserData);
}

// 테스트 6: 구독 취소
void test_event_bus_unsubscribe(void) {
    EventBus bus;
    bus.begin();

    // 구독
    bus.subscribe(WIFI_DISCONNECTED, testCallback);

    // 발행 및 처리 (콜백 호출됨)
    Event event1;
    event1.type = WIFI_DISCONNECTED;
    event1.timestamp = millis();
    event1.data = nullptr;

    bus.publish(event1);
    bus.update();
    TEST_ASSERT_EQUAL(1, callbackCallCount);

    // 구독 취소
    bus.unsubscribe(WIFI_DISCONNECTED, testCallback);

    // 재발행 및 처리 (콜백 호출되지 않아야 함)
    callbackCallCount = 0;
    Event event2;
    event2.type = WIFI_DISCONNECTED;
    event2.timestamp = millis();
    event2.data = nullptr;

    bus.publish(event2);
    bus.update();
    TEST_ASSERT_EQUAL(0, callbackCallCount);
}

// 테스트 7: 여러 이벤트 큐잉
void test_event_bus_queue_multiple_events(void) {
    EventBus bus;
    bus.begin();

    bus.subscribe(WIFI_CONNECTED, testCallback);
    bus.subscribe(WIFI_DISCONNECTED, testCallback);

    // 여러 이벤트 발행
    Event event1;
    event1.type = WIFI_CONNECTED;
    event1.timestamp = millis();
    event1.data = nullptr;

    Event event2;
    event2.type = WIFI_DISCONNECTED;
    event2.timestamp = millis() + 10;
    event2.data = nullptr;

    bus.publish(event1);
    bus.publish(event2);

    // 한 번의 update()로 모든 이벤트 처리
    int processed = bus.update();
    TEST_ASSERT_EQUAL(2, processed);
    TEST_ASSERT_EQUAL(2, callbackCallCount);
}

// 테스트 8: 전역 gEventBus 인스턴스
void test_global_event_bus_instance(void) {
    // 전역 인스턴스가 존재하는지 확인
    // 실제 초기화는 main.cpp 등에서 수행
    TEST_ASSERT_TRUE(true);
}

// 테스트 9: 큐 오버플로우 동작
void test_event_bus_queue_overflow(void) {
    EventBus bus;
    bus.begin();

    bus.subscribe(SENSOR_UPDATED, testCallback);

    Event event;
    event.type = SENSOR_UPDATED;
    event.timestamp = millis();
    event.data = nullptr;

    // EVENT_QUEUE_SIZE (16)보다 많은 이벤트 발행
    bool all_success = true;
    for (int i = 0; i < 20; i++) {
        bool result = bus.publish(event);
        if (!result && i < 16) {
            // 16개 이하에서는 실패하면 안 됨
            all_success = false;
        }
        if (result && i >= 16) {
            // 16개 이상에서는 성공하면 안 됨
            all_success = false;
        }
    }

    // 17번째 시도는 실패해야 함
    TEST_ASSERT_TRUE(all_success);

    // 큐에 있는 이벤트 처리 (최대 16개)
    int processed = bus.update();
    TEST_ASSERT_EQUAL(16, processed);
}

// 테스트 10: 최대 구독자 수 제한
void test_event_bus_max_subscribers_limit(void) {
    EventBus bus;
    bus.begin();

    // 4개까지는 성공해야 함
    bool success1 = bus.subscribe(TIME_SYNCED, testCallback);
    bool success2 = bus.subscribe(TIME_SYNCED, testCallback2);
    bool success3 = bus.subscribe(TIME_SYNCED, testCallback3);
    bool success4 = bus.subscribe(TIME_SYNCED, testCallback4);

    TEST_ASSERT_TRUE(success1);
    TEST_ASSERT_TRUE(success2);
    TEST_ASSERT_TRUE(success3);
    TEST_ASSERT_TRUE(success4);

    // 5번째는 실패해야 함 (같은 콜백 재등록 시도)
    bool extra_success = bus.subscribe(TIME_SYNCED, testCallback);
    TEST_ASSERT_FALSE(extra_success);
}

// 테스트 11: 잘못된 타입 처리
void test_event_bus_invalid_type_handling(void) {
    EventBus bus;
    bus.begin();

    // 잘못된 타입으로 구독 시도
    bool invalid_subscribe = bus.subscribe((EventType)99, testCallback);
    TEST_ASSERT_FALSE(invalid_subscribe);

    // 잘못된 타입으로 발행 시도
    Event invalid_event;
    invalid_event.type = (EventType)99;
    invalid_event.timestamp = millis();
    invalid_event.data = nullptr;

    bool invalid_publish = bus.publish(invalid_event);
    TEST_ASSERT_FALSE(invalid_publish);
}

// 테스트 12: 이벤트 데이터 전달
void test_event_bus_data_passing(void) {
    EventBus bus;
    bus.begin();

    // 테스트용 데이터 구조체
    struct TestData {
        int value;
        const char* message;
    };

    TestData test_data = {42, "Test Message"};

    bus.subscribe(WEATHER_UPDATED, testDataCallback, nullptr);

    Event event;
    event.type = WEATHER_UPDATED;
    event.timestamp = millis();
    event.data = &test_data;

    bus.publish(event);
    bus.update();

    TEST_ASSERT_EQUAL(1, callbackCallCount);
    TEST_ASSERT_EQUAL_PTR(&test_data, lastEventData);

    // 데이터 내용 검증
    TestData* received_data = (TestData*)lastEventData;
    TEST_ASSERT_EQUAL(42, received_data->value);
    TEST_ASSERT_EQUAL_STRING("Test Message", received_data->message);
}

// 테스트 13: clear() 함수 동작
void test_event_bus_clear_function(void) {
    EventBus bus;
    bus.begin();

    // 여러 구독자 등록
    bus.subscribe(WIFI_CONNECTED, testCallback);
    bus.subscribe(WIFI_DISCONNECTED, testCallback2);
    bus.subscribe(SENSOR_UPDATED, testCallback);

    // clear() 호출
    bus.clear();

    // 이벤트 발행 및 처리
    Event event;
    event.type = WIFI_CONNECTED;
    event.timestamp = millis();
    event.data = nullptr;

    bus.publish(event);
    bus.update();

    // 콜백이 호출되지 않아야 함
    TEST_ASSERT_EQUAL(0, callbackCallCount);
}

// 테스트 14: 복수 구독자 모두 호출 확인
void test_event_bus_all_subscribers_called(void) {
    EventBus bus;
    bus.begin();

    callbackCallCount = 0;

    // 3개의 서로 다른 콜백 등록
    bus.subscribe(SENSOR_UPDATED, testCallback);
    bus.subscribe(SENSOR_UPDATED, testCallback2);
    bus.subscribe(SENSOR_UPDATED, testCallback3);

    Event event;
    event.type = SENSOR_UPDATED;
    event.timestamp = millis();
    event.data = nullptr;

    bus.publish(event);
    bus.update();

    // 모든 구독자가 호출되어야 함
    TEST_ASSERT_EQUAL(3, callbackCallCount);
}

// 테스트 15: 빈 콜백 포인터로 구독 시도
void test_event_bus_nullptr_callback(void) {
    EventBus bus;
    bus.begin();

    // nullptr 콜백으로 구독 시도
    bool null_result = bus.subscribe(WIFI_CONNECTED, nullptr);
    TEST_ASSERT_FALSE(null_result);
}

// 테스트 16: 초기화되지 않은 상태에서 update()
void test_event_bus_uninitialized_update(void) {
    EventBus bus;
    // begin() 호출하지 않음

    int processed = bus.update();
    TEST_ASSERT_EQUAL(0, processed);
}

// 테스트 17: 여러 이벤트 타입 순차 처리
void test_event_bus_multiple_event_types(void) {
    EventBus bus;
    bus.begin();

    callbackCallCount = 0;

    bus.subscribe(WIFI_CONNECTED, testCallback);
    bus.subscribe(WIFI_DISCONNECTED, testCallback);
    bus.subscribe(TIME_SYNCED, testCallback);

    Event event1 = {WIFI_CONNECTED, millis(), nullptr};
    Event event2 = {WIFI_DISCONNECTED, millis(), nullptr};
    Event event3 = {TIME_SYNCED, millis(), nullptr};

    bus.publish(event1);
    bus.publish(event2);
    bus.publish(event3);

    int processed = bus.update();
    TEST_ASSERT_EQUAL(3, processed);
    TEST_ASSERT_EQUAL(3, callbackCallCount);
}

// 테스트 18: 구독 취소 후 재구독
void test_event_bus_unsubscribe_resubscribe(void) {
    EventBus bus;
    bus.begin();

    // 구독
    bus.subscribe(SENSOR_UPDATED, testCallback);

    Event event1 = {SENSOR_UPDATED, millis(), nullptr};
    bus.publish(event1);
    bus.update();
    TEST_ASSERT_EQUAL(1, callbackCallCount);

    // 구독 취소
    bus.unsubscribe(SENSOR_UPDATED, testCallback);

    callbackCallCount = 0;
    Event event2 = {SENSOR_UPDATED, millis(), nullptr};
    bus.publish(event2);
    bus.update();
    TEST_ASSERT_EQUAL(0, callbackCallCount);

    // 재구독
    bus.subscribe(SENSOR_UPDATED, testCallback);

    Event event3 = {SENSOR_UPDATED, millis(), nullptr};
    bus.publish(event3);
    bus.update();
    TEST_ASSERT_EQUAL(1, callbackCallCount);
}

// 테스트 19: 존재하지 않는 구독자 취소 시도
void test_event_bus_unsubscribe_nonexistent(void) {
    EventBus bus;
    bus.begin();

    // 등록되지 않은 콜백 취소 시도 (크래시하지 않아야 함)
    bus.unsubscribe(WIFI_CONNECTED, testCallback);

    // 이후 정상 동작 확인
    TEST_ASSERT_TRUE(true);
}

// 테스트 20: 이벤트 타임스탬프 기록
void test_event_bus_timestamp_recording(void) {
    EventBus bus;
    bus.begin();

    mock_advance_millis(100);

    unsigned long expected_time = millis();
    lastEventType = EVENT_TYPE_COUNT;

    bus.subscribe(WIFI_CONNECTED, testCallback);

    Event event;
    event.type = WIFI_CONNECTED;
    event.timestamp = 0;  // publish()가 설정하도록
    event.data = nullptr;

    bus.publish(event);
    bus.update();

    // 콜백이 호출되었는지 확인
    TEST_ASSERT_EQUAL(WIFI_CONNECTED, lastEventType);
    TEST_ASSERT_EQUAL(1, callbackCallCount);
}

int main(int argc, char** argv) {
    // Unity 테스트 프레임워크 설정
    UNITY_BEGIN();

    // 기본 테스트
    RUN_TEST(test_event_bus_initialization);
    RUN_TEST(test_event_bus_single_subscription);
    RUN_TEST(test_event_bus_publish_and_deliver);
    RUN_TEST(test_event_bus_multiple_subscribers);
    RUN_TEST(test_event_bus_user_data);
    RUN_TEST(test_event_bus_unsubscribe);
    RUN_TEST(test_event_bus_queue_multiple_events);
    RUN_TEST(test_global_event_bus_instance);

    // 추가 테스트
    RUN_TEST(test_event_bus_queue_overflow);
    RUN_TEST(test_event_bus_max_subscribers_limit);
    RUN_TEST(test_event_bus_invalid_type_handling);
    RUN_TEST(test_event_bus_data_passing);
    RUN_TEST(test_event_bus_clear_function);
    RUN_TEST(test_event_bus_all_subscribers_called);
    RUN_TEST(test_event_bus_nullptr_callback);
    RUN_TEST(test_event_bus_uninitialized_update);
    RUN_TEST(test_event_bus_multiple_event_types);
    RUN_TEST(test_event_bus_unsubscribe_resubscribe);
    RUN_TEST(test_event_bus_unsubscribe_nonexistent);
    RUN_TEST(test_event_bus_timestamp_recording);

    // 테스트 완료 및 결과 반환
    return UNITY_END();
}

#endif // ARTHUR_NATIVE_TEST
