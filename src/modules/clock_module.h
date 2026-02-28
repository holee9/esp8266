// @MX:NOTE: [AUTO] ClockModule - OLED 시계 표시 모듈
// TimeManager의 시간 정보를 OLED 디스플레이에 표시

#ifndef ARTHUR_CLOCK_MODULE_H
#define ARTHUR_CLOCK_MODULE_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "../core/event_bus.h"  // Event 타입 사용

// 전방 선언 (의존성 최소화)
class TimeManager;

/**
 * @brief ClockModule 클래스
 *
 * OLED 디스플레이에 현재 시간을 표시
 * - TimeManager의 TIME_SYNCED 이벤트를 구독
 * - 1초마다 화면 갱신
 * - 2색 OLED 지원 (노랑 상단바 + 파랑 내용)
 * - String 클래스 미사용
 */
class ClockModule {
public:
    ClockModule(Adafruit_SSD1306& display);
    ~ClockModule() = default;

    /**
     * @brief 모듈 초기화
     */
    void begin();

    /**
     * @brief 정기 업데이트 (loop에서 호출)
     */
    void update();

    /**
     * @brief 시계 화면 표시
     */
    void show();

    /**
     * @brief 시계 화면 숨김 (다른 화면 전환 시)
     */
    void hide();

    /**
     * @brief 표시 활성화 상태 확인
     */
    bool isVisible();

private:
    Adafruit_SSD1306& _display;
    bool _initialized;
    bool _visible;
    unsigned long _lastUpdate;
    bool _timeSynced;

    static const unsigned long UPDATE_INTERVAL_MS = 1000;  // 1초

    /**
     * @brief 시계 화면 그리기
     */
    void drawClockScreen();

    /**
     * @brief 상태바 그리기
     */
    void drawStatusBar(const char* text);

    /**
     * @brief 시간 표시 영역 그리기 (큰 글씨)
     */
    void drawTimeDisplay(const char* timeStr);

    /**
     * @brief 날짜 표시 영역 그리기
     */
    void drawDateDisplay(const char* dateStr);

    /**
     * @brief TIME_SYNCED 이벤트 콜백 (정적 함수)
     */
    static void onTimeSynced(const Event& event, void* userData);
};

// 전역 인스턴스 포인터 (콜백용)
extern ClockModule* gClockModulePtr;

#endif // ARTHUR_CLOCK_MODULE_H
