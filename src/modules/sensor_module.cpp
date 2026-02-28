// @MX:NOTE: [AUTO] SensorModule 구현 - BME280 센서 읽기 및 캐싱

#include "sensor_module.h"
#include "../core/time_manager.h"
#include "../core/event_bus.h"
#include "../core/cache_manager.h"
#include "../include/arthur_pins.h"
#include "../include/arthur_config.h"

// 캐시 키 정의
const char* const SensorModule::CACHE_KEY_TEMP = "sensor_temp";
const char* const SensorModule::CACHE_KEY_HUMID = "sensor_humid";
const char* const SensorModule::CACHE_KEY_PRESS = "sensor_press";

// 전역 포인터 정의
SensorModule* gSensorModulePtr = nullptr;

SensorModule::SensorModule(Adafruit_SSD1306& display)
    : _display(display)
    , _initialized(false)
    , _visible(false)
    , _lastReadTime(0)
    , _readInterval(SENSOR_READ_INTERVAL_MS)
{
}

bool SensorModule::begin() {
    Serial.println(F("SensorModule: Initializing BME280..."));

    // I2C 버스는 이미 main.cpp에서 초기화됨 (Wire.begin)
    // BME280 초기화 (주소: 0x76)
    if (!_bme.begin(BME280_ADDR, &Wire)) {
        Serial.println(F("SensorModule: BME280 not found at 0x76"));
        Serial.println(F("  Check wiring: SDA=GPIO14, SCL=GPIO12"));
        Serial.println(F("  SDO pin should be GND for 0x76"));
        return false;
    }

    // BME280 설정
    // 일반 모드, 오버샘플링 설정
    _bme.setSampling(
        Adafruit_BME280::MODE_NORMAL,
        Adafruit_BME280::SAMPLING_X16,  // 온도 오버샘플링
        Adafruit_BME280::SAMPLING_X16,  // 습도 오버샘플링
        Adafruit_BME280::SAMPLING_X16,  // 기압 오버샘플링
        Adafruit_BME280::FILTER_X16,    // 필터
        Adafruit_BME280::STANDBY_MS_500 // 대기 시간
    );

    // 전역 포인터 설정
    gSensorModulePtr = this;

    _initialized = true;

    Serial.println(F("SensorModule: BME280 initialized"));
    Serial.printf("SensorModule: Read interval: %lu ms\n", _readInterval);

    // 첫 읽기 시도
    SensorData data;
    if (readSensor(data)) {
        Serial.printf("SensorModule: T=%.1fC, H=%.0f%%, P=%.0fhPa\n",
                     data.temperature, data.humidity, data.pressure);
    }

    return true;
}

void SensorModule::update() {
    if (!_initialized) {
        return;
    }

    unsigned long now = millis();

    // 읽기 간격 확인
    if (now - _lastReadTime < _readInterval) {
        return;
    }

    _lastReadTime = now;

    // 센서 읽기
    SensorData data;
    if (readSensor(data)) {
        // 캐시에 저장
        cacheSensorData(data);

        // 이벤트 발행
        publishSensorEvent(data);

        // OLED 표시 (활성화된 경우)
        if (_visible) {
            displaySensorData();
        }

        // 시리얼 로그
        Serial.printf("SensorModule: T=%.1fC, H=%.0f%%, P=%.0fhPa\n",
                     data.temperature, data.humidity, data.pressure);
    }
}

bool SensorModule::readSensor(SensorData& outData) {
    if (!_initialized) {
        return false;
    }

    // BME280에서 값 읽기
    outData.temperature = _bme.readTemperature();
    outData.humidity = _bme.readHumidity();
    outData.pressure = _bme.readPressure() / 100.0f;  // Pa -> hPa 변환
    outData.timestamp = millis();
    outData.valid = isDataValid(outData);

    if (!outData.valid) {
        Serial.println(F("SensorModule: Invalid sensor reading"));
    }

    return outData.valid;
}

void SensorModule::cacheSensorData(const SensorData& data) {
    char buf[16];

    // 온도 캐싱 (TTL: 10분)
    snprintf(buf, sizeof(buf), "%.1f", data.temperature);
    CacheMgr.set(CACHE_KEY_TEMP, buf, 600000);

    // 습도 캐싱
    snprintf(buf, sizeof(buf), "%.0f", data.humidity);
    CacheMgr.set(CACHE_KEY_HUMID, buf, 600000);

    // 기압 캐싱
    snprintf(buf, sizeof(buf), "%.0f", data.pressure);
    CacheMgr.set(CACHE_KEY_PRESS, buf, 600000);
}

bool SensorModule::isDataValid(const SensorData& data) {
    // BME280 유효 범위 확인
    // 온도: -40 ~ 85 °C
    // 습도: 0 ~ 100 %
    // 기압: 300 ~ 1100 hPa

    if (data.temperature < -40.0f || data.temperature > 85.0f) {
        return false;
    }

    if (data.humidity < 0.0f || data.humidity > 100.0f) {
        return false;
    }

    if (data.pressure < 300.0f || data.pressure > 1100.0f) {
        return false;
    }

    // NaN 확인
    if (isnan(data.temperature) || isnan(data.humidity) || isnan(data.pressure)) {
        return false;
    }

    return true;
}

void SensorModule::publishSensorEvent(const SensorData& data) {
    // SENSOR_UPDATED 이벤트 발행
    Event event;
    event.type = SENSOR_UPDATED;
    event.timestamp = millis();
    event.data = &data;  // SensorData 구조체 포인터

    gEventBus.publish(event);
}

void SensorModule::displaySensorData() {
    drawSensorScreen(_lastData);
}

void SensorModule::drawSensorScreen(const SensorData& data) {
    _display.clearDisplay();

    // 상태바 (노랑 영역)
    drawStatusBar("Environment");

    // 온도 표시 (큰 글씨)
    char tempBuf[16];
    formatFloat(tempBuf, sizeof(tempBuf), data.temperature, "C");

    _display.setTextSize(2);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(0, 20);
    _display.print(F("Temp: "));
    _display.println(tempBuf);

    // 습도 표시
    char humidBuf[16];
    formatFloat(humidBuf, sizeof(humidBuf), data.humidity, "%");

    _display.setTextSize(1);
    _display.setCursor(0, 42);
    _display.print(F("Humidity: "));
    _display.println(humidBuf);

    // 기압 표시
    char pressBuf[16];
    formatFloat(pressBuf, sizeof(pressBuf), data.pressure, "hPa");

    _display.setCursor(0, 54);
    _display.print(F("Pressure: "));
    _display.println(pressBuf);

    _display.display();
}

void SensorModule::drawStatusBar(const char* text) {
    _display.fillRect(0, 0, OLED_WIDTH, OLED_YELLOW_BOTTOM + 1, SSD1306_BLACK);
    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(0, 4);
    _display.print(text);
}

void SensorModule::formatFloat(char* buf, size_t bufSize, float value, const char* unit) {
    if (buf == nullptr || bufSize < 8) {
        return;
    }

    // 소수점 1자리까지 포맷
    snprintf(buf, bufSize, "%.1f%s", value, unit);
}
