#ifndef ARTHUR_WEATHER_MODULE_H
#define ARTHUR_WEATHER_MODULE_H

#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "../core/config_manager.h"
#include "../core/cache_manager.h"
#include "../core/event_bus.h"

/**
 * @brief WeatherModule
 *
 * OpenWeatherMap API 기반 날씨 모듈
 * - ConfigManager: API key 저장
 * - CacheManager: 날씨 데이터 캐싱 (2시간 TTL)
 * - EventBus: 날씨 업데이트 이벤트 발행
 * - 정적 할당만 사용 (new/malloc 금지)
 * - String 클래스 미사용 (char[] + F() 매크로)
 *
 * @MX:NOTE: [오프라인 지원] WiFi 연결 없으면 캐시 데이터 반환
 * @MX:ANCHOR: [날씨 인터페이스] UI 및 다른 모듈에서 날씨 정보 조회
 * @MX:REASON: fan_in >= 3 (ClockModule, UIManager 등)
 */
class WeatherModule {
public:
    // 날씨 조건 코드
    enum WeatherCondition {
        CLEAR,          // 맑음
        CLOUDY,         // 구름
        RAIN,           // 비
        SNOW,           // 눈
        THUNDERSTORM,   // 천둥
        MIST,           // 안개
        UNKNOWN         // 알 수 없음
    };

    // 날씨 데이터 구조체
    struct WeatherData {
        float temperature;      // 온도 (섭씨)
        float humidity;         // 습도 (%)
        float windSpeed;        // 풍속 (m/s)
        int pressure;           // 기압 (hPa)
        WeatherCondition condition;  // 날씨 상태
        char description[32];   // 날씨 설명 (예: "light rain")
        char location[32];      // 위치 (예: "Seoul")
        unsigned long timestamp;  // 업데이트 시각 (millis)

        // 기본 생성자
        WeatherData()
            : temperature(0)
            , humidity(0)
            , windSpeed(0)
            , pressure(0)
            , condition(UNKNOWN)
            , timestamp(0)
        {
            description[0] = '\0';
            location[0] = '\0';
        }
    };

    // 버퍼 크기 상수
    static const size_t API_URL_BUF_SIZE = 256;
    static const size_t WEATHER_JSON_BUF_SIZE = 1024;
    static const size_t LOCATION_BUF_SIZE = 32;

    // 업데이트 간격 (밀리초)
    static const unsigned long UPDATE_INTERVAL_MS = 600000;  // 10분
    // 캐시 TTL (밀리초)
    static const unsigned long CACHE_TTL_MS = 7200000;       // 2시간

    /**
     * @brief 생성자
     */
    WeatherModule();

    /**
     * @brief 모듈 초기화
     *
     * @return true 초기화 성공
     * @return false 초기화 실패
     */
    bool begin();

    /**
     * @brief 메인 루프에서 호출 (주기적 업데이트)
     *
     * @return int 처리된 작업 수 (0 또는 1)
     */
    int update();

    /**
     * @brief 현재 날씨 데이터 가져오기
     *
     * @return WeatherData const* 날씨 데이터 포인터 (nullptr 유효하지 않음)
     */
    const WeatherData* getWeatherData() const { return &_currentData; }

    /**
     * @brief 날씨 강제 업데이트
     *
     * @return true 업데이트 성공
     * @return false 업데이트 실패
     */
    bool refresh();

    /**
     * @brief API 키 설정
     *
     * @param apiKey OpenWeatherMap API 키
     * @return true 설정 성공
     * @return false 설정 실패
     */
    bool setApiKey(const char* apiKey);

    /**
     * @brief API 키 가져오기
     *
     * @param outBuf 출력 버퍼
     * @param maxLen 버퍼 크기
     * @return true 키 찾음
     * @return false 키 없음
     */
    bool getApiKey(char* outBuf, size_t maxLen);

    /**
     * @brief 위치 설정
     *
     * @param location 도시 이름 (예: "Seoul,KR" 또는 "Tokyo,JP")
     * @return true 설정 성공
     * @return false 설정 실패
     */
    bool setLocation(const char* location);

    /**
     * @brief 위치 가져오기
     *
     * @param outBuf 출력 버퍼
     * @param maxLen 버퍼 크기
     * @return true 위치 찾음
     * @return false 위치 없음
     */
    bool getLocation(char* outBuf, size_t maxLen);

    /**
     * @brief WiFi 연결 상태 설정 (이벤트 콜백용)
     *
     * @param connected true 연결됨, false 연결 끊김
     */
    void setWiFiConnected(bool connected) { _wifiConnected = connected; }

    /**
     * @brief 날씨 조건 코드를 문자열로 변환
     *
     * @param condition 날씨 조건
     * @return const char* 조건 설명
     */
    static const char* conditionToString(WeatherCondition condition);

    /**
     * @brief OpenWeatherMap 코드를 WeatherCondition으로 변환
     *
     * @param code OpenWeatherMap 코드
     * @return WeatherCondition 변환된 조건
     */
    static WeatherCondition parseWeatherCondition(int code);

private:
    WiFiClient _wifiClient;
    HTTPClient _httpClient;

    WeatherData _currentData;
    unsigned long _lastUpdate;
    bool _wifiConnected;
    bool _initialized;

    char _apiKey[64];
    char _location[LOCATION_BUF_SIZE];

    // OpenWeatherMap API 호출
    bool fetchWeatherFromAPI();

    // 캐시에서 날씨 데이터 로드
    bool loadFromCache();

    // 캐시에 날씨 데이터 저장
    bool saveToCache();

    // 날씨 데이터를 JSON 문자열로 변환
    bool weatherDataToJson(char* outBuf, size_t maxLen);

    // JSON 문자열을 날씨 데이터로 파싱
    bool jsonToWeatherData(const char* jsonStr);

    // URL 이스케이프 (공백을 %20으로 변환 등)
    void urlEncode(char* dest, const char* src, size_t maxLen);

    // 이벤트 버스 콜백 (정적 함수)
    static void onWiFiEvent(const Event& event, void* userData);
};

// 전역 인스턴스
extern WeatherModule gWeatherModule;

#endif // ARTHUR_WEATHER_MODULE_H
