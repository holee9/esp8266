// @MX:NOTE: [AUTO] 이벤트 버스 - pub/sub 패턴 구현
// @MX:ANCHOR: [AUTO] 코어 모듈 간 통신 허브
// @MX:REASON: fan_in >= 3 (TimeManager, SensorModule, WeatherModule 등에서 구독)

#ifndef ARTHUR_EVENT_BUS_H
#define ARTHUR_EVENT_BUS_H

#include <Arduino.h>

// 최대 이벤트 타입 수
#define MAX_EVENT_TYPES 8

// 최대 구독자 수 (이벤트 타입당)
#define MAX_SUBSCRIBERS 4

// 이벤트 타입 열거형
enum EventType {
    WIFI_CONNECTED,      // WiFi 연결 완료
    WIFI_DISCONNECTED,   // WiFi 연결 끊김
    TIME_SYNCED,         // NTP 시간 동기화 완료
    SENSOR_UPDATED,      // 센서 데이터 업데이트
    WEATHER_UPDATED,     // 날씨 정보 업데이트
    // 예비 타입 (최대 8개)
    EVENT_TYPE_RESERVED_1,
    EVENT_TYPE_RESERVED_2,
    EVENT_TYPE_COUNT     // 항상 마지막에 위치
};

// 이벤트 데이터 구조체
struct Event {
    EventType type;              // 이벤트 타입
    unsigned long timestamp;     // 발생 시각 (millis())
    const void* data;            // 부가 데이터 (선택 사항)
};

// 이벤트 콜백 함수 포인터 타입
// event: 발생한 이벤트
// userData: 구독 시 등록한 사용자 데이터
typedef void (*EventCallback)(const Event& event, void* userData);

// 구독자 정보 구조체
struct Subscriber {
    EventCallback callback;      // 콜백 함수
    void* userData;              // 사용자 데이터
};

/**
 * @brief EventBus 클래스
 *
 * ESP8266의 제한된 메모리 환경에서 동작하는 경량 pub/sub 이벤트 시스템
 * - 정적 할당만 사용 (new/malloc 금지)
 * - String 클래스 미사용
 * - 최대 8개 이벤트 타입, 타입당 4개 구독자 지원
 */
class EventBus {
public:
    EventBus();
    ~EventBus() = default;

    /**
     * @brief 이벤트 버스 초기화
     */
    void begin();

    /**
     * @brief 이벤트 구독
     *
     * @param type 구독할 이벤트 타입
     * @param callback 이벤트 발생 시 호출될 함수
     * @param userData 콜백에 전달할 사용자 데이터 (선택 사항)
     * @return true 구독 성공
     * @return false 구독 실패 (구독자 만료 또는 잘못된 타입)
     */
    bool subscribe(EventType type, EventCallback callback, void* userData = nullptr);

    /**
     * @brief 이벤트 발행 (비동기)
     *
     * 이벤트를 큐에 추가하고 즉시 반환
     * 실제 콜백은 update() 호출 시 실행
     *
     * @param event 발행할 이벤트
     * @return true 이벤트 큐에 추가 성공
     * @return false 큐가 가득 참
     */
    bool publish(const Event& event);

    /**
     * @brief 대기 중인 이벤트 처리
     *
     * 큐에 있는 이벤트를 순차적으로 처리하고 등록된 콜백 호출
     * loop() 함수에서 주기적으로 호출해야 함
     *
     * @return int 처리된 이벤트 개수
     */
    int update();

    /**
     * @brief 구독 취소
     *
     * @param type 구독 취소할 이벤트 타입
     * @param callback 취소할 콜백 함수
     */
    void unsubscribe(EventType type, EventCallback callback);

    /**
     * @brief 모든 구독자 초기화
     */
    void clear();

private:
    // 구독자 배열 [이벤트 타입][구독자 인덱스]
    Subscriber _subscribers[MAX_EVENT_TYPES][MAX_SUBSCRIBERS];

    // 이벤트 큐 (대기열)
    static const int EVENT_QUEUE_SIZE = 16;
    Event _eventQueue[EVENT_QUEUE_SIZE];
    int _queueHead;      // 큐의 읽기 위치
    int _queueTail;      // 큐의 쓰기 위치
    int _queueCount;     // 큐에 있는 이벤트 수

    bool _initialized;

    /**
     * @brief 콜백 호출 (내부용)
     */
    void dispatchEvent(const Event& event);
};

// 전역 인스턴스 (extern)
extern EventBus gEventBus;

#endif // ARTHUR_EVENT_BUS_H
