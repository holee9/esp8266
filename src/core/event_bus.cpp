// @MX:NOTE: [AUTO] EventBus 구현 - 정적 할당 기반 pub/sub 시스템

#include "event_bus.h"

// 전역 인스턴스 정의
EventBus gEventBus;

EventBus::EventBus()
    : _queueHead(0)
    , _queueTail(0)
    , _queueCount(0)
    , _initialized(false)
{
    // 구독자 배열 초기화
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
    // 타입 유효성 검사
    if (type < 0 || type >= EVENT_TYPE_COUNT) {
        return false;
    }

    // 콜백 유효성 검사
    if (callback == nullptr) {
        return false;
    }

    // 빈 슬롯 찾기
    for (int i = 0; i < MAX_SUBSCRIBERS; i++) {
        if (_subscribers[type][i].callback == nullptr) {
            _subscribers[type][i].callback = callback;
            _subscribers[type][i].userData = userData;
            return true;
        }
    }

    // 구독자가 꽉 찼음
    Serial.print(F("EventBus: MAX_SUBSCRIBERS reached for type "));
    Serial.println(type);
    return false;
}

bool EventBus::publish(const Event& event) {
    // 타입 유효성 검사
    if (event.type < 0 || event.type >= EVENT_TYPE_COUNT) {
        return false;
    }

    // 큐가 가득 찼는지 확인
    if (_queueCount >= EVENT_QUEUE_SIZE) {
        Serial.println(F("EventBus: Event queue full"));
        return false;
    }

    // 큐에 이벤트 추가
    _eventQueue[_queueTail] = event;
    _eventQueue[_queueTail].timestamp = millis();

    _queueTail = (_queueTail + 1) % EVENT_QUEUE_SIZE;
    _queueCount++;

    return true;
}

int EventBus::update() {
    if (!_initialized) {
        return 0;
    }

    int processed = 0;

    // 큐에 있는 모든 이벤트 처리
    while (_queueCount > 0) {
        // 이벤트 꺼내기
        Event event = _eventQueue[_queueHead];
        _queueHead = (_queueHead + 1) % EVENT_QUEUE_SIZE;
        _queueCount--;

        // 등록된 구독자들에게 전파
        dispatchEvent(event);
        processed++;
    }

    return processed;
}

void EventBus::unsubscribe(EventType type, EventCallback callback) {
    // 타입 유효성 검사
    if (type < 0 || type >= EVENT_TYPE_COUNT) {
        return;
    }

    // 해당 콜백 찾아서 제거
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

    // 큐 초기화
    _queueHead = 0;
    _queueTail = 0;
    _queueCount = 0;
}

void EventBus::dispatchEvent(const Event& event) {
    // 이 이벤트 타입을 구독한 모든 콜백 호출
    for (int i = 0; i < MAX_SUBSCRIBERS; i++) {
        Subscriber& sub = _subscribers[event.type][i];

        if (sub.callback != nullptr) {
            // 콜백 호출
            sub.callback(event, sub.userData);
        }
    }
}
