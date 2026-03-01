// @MX:NOTE: [MOCK] Adafruit BME280 sensor library mock for native testing
// PlatformIO native environment에서 BME280 센서 기능 모방

#ifndef ARTHUR_ADAFRUIT_BME280_MOCK_H
#define ARTHUR_ADAFRUIT_BME280_MOCK_H

#include <cstdint>

#ifdef ARTHUR_NATIVE_TEST

// 센서 모드 타입 (Adafruit_Sensor 호환)
typedef enum {
    MODE_SLEEP = 0,
    MODE_FORCED = 1,
    MODE_NORMAL = 3
} sensor_mode_t;

// 샘플링 타입
typedef enum {
    SAMPLING_NONE = 0,
    SAMPLING_X1 = 1,
    SAMPLING_X2 = 2,
    SAMPLING_X4 = 3,
    SAMPLING_X8 = 4,
    SAMPLING_X16 = 5
} sensor_sampling_t;

// 필터 타입
typedef enum {
    FILTER_OFF = 0,
    FILTER_X2 = 1,
    FILTER_X4 = 2,
    FILTER_X8 = 3,
    FILTER_X16 = 4
} sensor_filter_t;

// 대기 시간 타입
typedef enum {
    STANDBY_MS_0_5 = 0,
    STANDBY_MS_10 = 1,
    STANDBY_MS_20 = 2,
    STANDBY_MS_62_5 = 3,
    STANDBY_MS_125 = 4,
    STANDBY_MS_250 = 5,
    STANDBY_MS_500 = 6,
    STANDBY_MS_1000 = 7
} standby_duration_t;

// BME280 등록 주소
#define BME280_ADDRESS 0x77
#define BME280_ADDRESS_ALTERNATE 0x76

// Adafruit_BME280 클래스 모의
class Adafruit_BME280 {
public:
    // 내부에서 사용하는 타입 별칭
    typedef sensor_mode_t mode_t;
    typedef sensor_sampling_t sampling_t;
    typedef sensor_filter_t filter_t;
    typedef standby_duration_t standby_t;

    Adafruit_BME280()
        : _initialized(false), _temperature(25.0f), _humidity(50.0f),
          _pressure(1013.25f), _altitude(0.0f) {
    }

    ~Adafruit_BME280() = default;

    // 센서 초기화
    bool begin(uint8_t addr = BME280_ADDRESS, bool initSettings = true) {
        _initialized = true;
        // 테스트에서는 항상 성공
        return true;
    }

    // 온도 읽기 (섭씨)
    float readTemperature() {
        if (!_initialized) return NAN;
        return _temperature;
    }

    // 습도 읽기 (%)
    float readHumidity() {
        if (!_initialized) return NAN;
        return _humidity;
    }

    // 기압 읽기 (hPa)
    float readPressure() {
        if (!_initialized) return NAN;
        // Pa 단위로 반환 (hPa * 100)
        return _pressure * 100.0f;
    }

    // 해발고도 계산 (미터)
    float readAltitude(float seaLevelhPa = 1013.25f) {
        if (!_initialized) return NAN;
        // 대기압 공식에 따른 고도 계산
        // altitude = 44330 * (1 - (pressure / seaLevelhPa)^(1/5.255))
        float pressureRatio = _pressure / seaLevelhPa;
        return 44330.0f * (1.0f - pow(pressureRatio, 0.190294957f));
    }

    // 해면 기압 계산
    float seaLevelForAltitude(float altitude, float atmospheric) {
        if (!_initialized) return NAN;
        // seaLevel = pressure / pow(1 - altitude/44330, 5.255)
        float ratio = 1.0f - (altitude / 44330.0f);
        return atmospheric / pow(ratio, 5.255f);
    }

    // 센서 읽기 (모든 값 한번에)
    bool takeForcedMeasurement() {
        if (!_initialized) return false;
        // 테스트에서는 항상 성공
        return true;
    }

    // 샘플링 설정 (테스트용 stub)
    void setSampling(mode_t mode,
                     sampling_t tempSampling,
                     sampling_t pressSampling,
                     sampling_t humSampling,
                     filter_t filter,
                     standby_t duration) {
        // 테스트에서는 저장만 수행
        (void)mode;
        (void)tempSampling;
        (void)pressSampling;
        (void)humSampling;
        (void)filter;
        (void)duration;
    }

    // 테스트 헬퍼: 센서 값 설정
    void mock_set_temperature(float temp) {
        _temperature = temp;
    }

    void mock_set_humidity(float humidity) {
        _humidity = humidity;
    }

    void mock_set_pressure(float pressure) {
        _pressure = pressure;
    }

    void mock_set_altitude(float altitude) {
        _altitude = altitude;
    }

    // 테스트 헬퍼: 초기화 상태 설정
    void mock_set_initialized(bool initialized) {
        _initialized = initialized;
    }

    // 테스트 헬퍼: 초기화 상태 확인
    bool mock_is_initialized() const {
        return _initialized;
    }

private:
    bool _initialized;
    float _temperature;  // 섭씨
    float _humidity;     // %
    float _pressure;     // hPa
    float _altitude;     // 미터
};

// Adafruit_Sensor 네임스페이스 (최소한)
namespace Adafruit_Sensor {
    enum SensorMode {
        SENSOR_MODE_SLEEP = 0,
        SENSOR_MODE_FORCED = 1,
        SENSOR_MODE_NORMAL = 3
    };

    enum SensorSampling {
        SAMPLING_NONE = 0,
        SAMPLING_X1 = 1,
        SAMPLING_X2 = 2,
        SAMPLING_X4 = 3,
        SAMPLING_X8 = 4,
        SAMPLING_X16 = 5
    };

    enum SensorFilter {
        FILTER_OFF = 0,
        FILTER_X2 = 1,
        FILTER_X4 = 2,
        FILTER_X8 = 3,
        FILTER_X16 = 4
    };

    enum StandbyDuration {
        STANDBY_MS_0_5 = 0,
        STANDBY_MS_10 = 6,
        STANDBY_MS_20 = 7,
        STANDBY_MS_62_5 = 1,
        STANDBY_MS_125 = 2,
        STANDBY_MS_250 = 3,
        STANDBY_MS_500 = 4,
        STANDBY_MS_1000 = 5
    };
}

#endif // ARTHUR_NATIVE_TEST
#endif // ARTHUR_ADAFRUIT_BME280_MOCK_H
