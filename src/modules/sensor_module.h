// @MX:NOTE: [AUTO] SensorModule - BME280 온습도/기압 센서 모듈
// I2C 공유 버스 (OLED와 동일) 사용

#ifndef ARTHUR_SENSOR_MODULE_H
#define ARTHUR_SENSOR_MODULE_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>

// BME280 라이브러리
#include <Adafruit_BME280.h>

// 전방 선언 (의존성 최소화)
class TimeManager;

/**
 * @brief 센서 데이터 구조체
 *
 * BME280에서 읽은 센서 값
 */
struct SensorData {
    float temperature;    // 온도 (섭씨)
    float humidity;       // 습도 (%)
    float pressure;       // 기압 (hPa)
    unsigned long timestamp;  // 측정 시각 (millis)
    bool valid;           // 데이터 유효 여부

    // 기본 생성자
    SensorData() : temperature(0), humidity(0), pressure(0), timestamp(0), valid(false) {}
};

/**
 * @brief SensorModule 클래스
 *
 * BME280 온습도/기압 센서를 읽고 데이터를 관리
 * - I2C 주소: 0x76 (SDO=GND 설정)
 * - I2C 버스: OLED와 공유 (GPIO14/GPIO12)
 * - 읽기 간격: 5초
 * - CacheManager에 데이터 캐싱
 * - SENSOR_UPDATED 이벤트 발행
 * - String 클래스 미사용
 */
class SensorModule {
public:
    SensorModule(Adafruit_SSD1306& display);
    ~SensorModule() = default;

    /**
     * @brief 모듈 초기화
     *
     * @return true 초기화 성공
     * @return false 초기화 실패
     */
    bool begin();

    /**
     * @brief 정기 업데이트 (loop에서 호출)
     *
     * 설정된 간격으로 센서 읽기 및 이벤트 발행
     */
    void update();

    /**
     * @brief 센서 데이터 수동 읽기
     *
     * @param outData 출력 버퍼
     * @return true 읽기 성공
     * @return false 읽기 실패
     */
    bool readSensor(SensorData& outData);

    /**
     * @brief 마지막으로 읽은 데이터 가져오기
     *
     * @return const SensorData& 마지막 센서 데이터 (참조)
     */
    const SensorData& getLastData() const { return _lastData; }

    /**
     * @brief 센서 초기화 여부 확인
     */
    bool isInitialized() const { return _initialized; }

    /**
     * @brief 현재 데이터 표시 모드 설정
     *
     * @param visible true면 OLED에 센서 데이터 표시
     */
    void setVisible(bool visible) { _visible = visible; }

    /**
     * @brief 읽기 간격 설정 (밀리초)
     */
    void setReadInterval(unsigned long intervalMs) { _readInterval = intervalMs; }

    /**
     * @brief 센서 데이터를 OLED에 표시
     */
    void displaySensorData();

private:
    Adafruit_SSD1306& _display;
    Adafruit_BME280 _bme;  // BME280 인스턴스

    bool _initialized;
    bool _visible;
    unsigned long _lastReadTime;
    unsigned long _readInterval;

    SensorData _lastData;

    // 캐시 키 (정적 상수)
    static const char* const CACHE_KEY_TEMP;
    static const char* const CACHE_KEY_HUMID;
    static const char* const CACHE_KEY_PRESS;

    /**
     * @brief 캐시에 센서 데이터 저장
     */
    void cacheSensorData(const SensorData& data);

    /**
     * @brief 센서 데이터 유효성 확인
     */
    bool isDataValid(const SensorData& data);

    /**
     * @brief SENSOR_UPDATED 이벤트 발행
     */
    void publishSensorEvent(const SensorData& data);

    /**
     * @brief OLED 화면 그리기 (센서 데이터)
     */
    void drawSensorScreen(const SensorData& data);

    /**
     * @brief 상태바 그리기
     */
    void drawStatusBar(const char* text);

    /**
     * @brief 센서 값 포맷팅 (소수점 1자리)
     */
    void formatFloat(char* buf, size_t bufSize, float value, const char* unit);
};

// 전역 인스턴스 포인터
extern SensorModule* gSensorModulePtr;

#endif // ARTHUR_SENSOR_MODULE_H
