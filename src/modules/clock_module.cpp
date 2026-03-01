// @MX:NOTE: [AUTO] ClockModule 구현 - OLED 시계 표시

#include "clock_module.h"
#include "../core/time_manager.h"
#include "../core/event_bus.h"
#include "../include/arthur_pins.h"
#include "../include/arthur_config.h"
#include "weather_module.h"  // @MX:NOTE: WeatherData 구조체 사용을 위해 포함
#include "sensor_module.h"   // @MX:NOTE: SensorData 구조체 사용을 위해 포함

// 전역 포인터 정의 (이벤트 콜백용)
ClockModule* gClockModulePtr = nullptr;

ClockModule::ClockModule(Adafruit_SSD1306& display)
    : _display(display)
    , _initialized(false)
    , _visible(false)
    , _lastUpdate(0)
    , _timeSynced(false)
    , _lastSensorTemp(0)
    , _sensorDataValid(false)
    , _lastWeatherTemp(0)
    , _weatherDataValid(false)
{
}

void ClockModule::begin() {
    Serial.println(F("ClockModule: Initializing..."));

    // 전역 포인터 설정 (콜백용)
    gClockModulePtr = this;

    // 이벤트 구독
    gEventBus.subscribe(TIME_SYNCED, onTimeSynced, nullptr);
    gEventBus.subscribe(SENSOR_UPDATED, onSensorUpdated, nullptr);
    gEventBus.subscribe(WEATHER_UPDATED, onWeatherUpdated, nullptr);

    _initialized = true;
    _visible = true;
    _lastUpdate = 0;

    Serial.println(F("ClockModule: Ready"));
}

void ClockModule::update() {
    if (!_initialized || !_visible) {
        return;
    }

    unsigned long now = millis();

    // 1초마다 갱신
    if (now - _lastUpdate >= UPDATE_INTERVAL_MS) {
        _lastUpdate = now;

        // 시간 동기화 상태 확인
        _timeSynced = gTimeManager.isSynced();

        // 시계 화면 그리기
        drawClockScreen();
    }
}

void ClockModule::show() {
    _visible = true;
    _lastUpdate = 0;  // 즉시 갱신
}

void ClockModule::hide() {
    _visible = false;
}

bool ClockModule::isVisible() {
    return _visible;
}

void ClockModule::drawClockScreen() {
    _display.clearDisplay();

    // 상태바 (노랑 영역, 0-15행)
    if (_timeSynced) {
        // 센서 데이터가 있으면 온도 표시
        if (_sensorDataValid) {
            char statusBuf[32];
            snprintf(statusBuf, sizeof(statusBuf), "ARTHUR %.1fC", _lastSensorTemp);
            drawStatusBar(statusBuf);
        } else {
            drawStatusBar("ARTHUR");
        }
    } else {
        drawStatusBar("Syncing...");
    }

    // 시간 버퍼
    char timeBuf[16];
    if (_timeSynced) {
        gTimeManager.getFormattedTime(timeBuf, sizeof(timeBuf));
    } else {
        strcpy(timeBuf, "--:--:--");
    }

    // 날짜 버퍼
    char dateBuf[32];
    if (_timeSynced) {
        gTimeManager.getFormattedDateTime(dateBuf, sizeof(dateBuf));
    } else {
        strcpy(dateBuf, "Wait for NTP sync");
    }

    // 시간 표시 (큰 글씨)
    drawTimeDisplay(timeBuf);

    // 날짜 표시
    drawDateDisplay(dateBuf);

    _display.display();
}

void ClockModule::drawStatusBar(const char* text) {
    // 상단 노랑 영역 (0~15)
    _display.fillRect(0, 0, OLED_WIDTH, OLED_YELLOW_BOTTOM + 1, SSD1306_BLACK);
    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(0, 4);
    _display.print(text);
}

void ClockModule::drawTimeDisplay(const char* timeStr) {
    // 하단 파랑 영역 (16~63)
    _display.setTextSize(2);
    _display.setTextColor(SSD1306_WHITE);

    // 시간 중앙 정렬 계산
    int16_t x1, y1;
    uint16_t w, h;
    _display.getTextBounds((char*)timeStr, 0, 0, &x1, &y1, &w, &h);

    int x = (OLED_WIDTH - w) / 2;
    int y = 22;

    _display.setCursor(x, y);
    _display.print(timeStr);
}

void ClockModule::drawDateDisplay(const char* dateStr) {
    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);

    // 날짜 중앙 정렬
    int16_t x1, y1;
    uint16_t w, h;
    _display.getTextBounds((char*)dateStr, 0, 0, &x1, &y1, &w, &h);

    int x = (OLED_WIDTH - w) / 2;
    int y = 48;

    _display.setCursor(x, y);
    _display.print(dateStr);
}

// 정적 콜백 함수
void ClockModule::onTimeSynced(const Event& event, void* userData) {
    (void)event;    // 미사용 경고 방지
    (void)userData;

    if (gClockModulePtr != nullptr) {
        Serial.println(F("ClockModule: Time synced event received"));
        gClockModulePtr->_timeSynced = true;
        gClockModulePtr->_lastUpdate = 0;  // 즉시 갱신 트리거
    }
}

// SENSOR_UPDATED 이벤트 콜백
void ClockModule::onSensorUpdated(const Event& event, void* userData) {
    (void)userData;

    if (gClockModulePtr != nullptr && event.data != nullptr) {
        const SensorData* data = static_cast<const SensorData*>(event.data);
        if (data != nullptr && data->valid) {
            gClockModulePtr->_lastSensorTemp = data->temperature;
            gClockModulePtr->_sensorDataValid = true;
            gClockModulePtr->_lastUpdate = 0;  // 즉시 갱신 트리거
            Serial.println(F("ClockModule: Sensor data received"));
        }
    }
}

// WEATHER_UPDATED 이벤트 콜백
void ClockModule::onWeatherUpdated(const Event& event, void* userData) {
    (void)userData;

    if (gClockModulePtr != nullptr && event.data != nullptr) {
        // WeatherModule::WeatherData 구조체 사용
        const WeatherModule::WeatherData* data =
            static_cast<const WeatherModule::WeatherData*>(event.data);
        if (data != nullptr) {
            gClockModulePtr->_lastWeatherTemp = data->temperature;
            gClockModulePtr->_weatherDataValid = true;
            gClockModulePtr->_lastUpdate = 0;  // 즉시 갱신 트리거
            Serial.println(F("ClockModule: Weather data received"));
        }
    }
}
