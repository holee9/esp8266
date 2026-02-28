#include "weather_module.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

// 전역 인스턴스
WeatherModule gWeatherModule;

// @MX:ANCHOR: [날씨 모듈 초기화] 부팅 시 날씨 모듈 초기화
// @MX:REASON: 시스템 진입점, begin()에서 설정 로드 및 이벤트 구독
WeatherModule::WeatherModule()
    : _lastUpdate(0)
    , _wifiConnected(false)
    , _initialized(false)
{
    _apiKey[0] = '\0';
    _location[0] = '\0';
    // 기본 위치: Seoul
    strncpy(_location, "Seoul,KR", sizeof(_location) - 1);
    _location[sizeof(_location) - 1] = '\0';
}

bool WeatherModule::begin() {
    // ConfigManager에서 설정 로드
    getApiKey(_apiKey, sizeof(_apiKey));
    getLocation(_location, sizeof(_location));

    // EventBus에 WiFi 이벤트 구독
    gEventBus.subscribe(WIFI_CONNECTED, onWiFiEvent, this);
    gEventBus.subscribe(WIFI_DISCONNECTED, onWiFiEvent, this);

    // 캐시에서 데이터 로드 시도
    loadFromCache();

    _initialized = true;
    Serial.println(F("[WeatherModule] Initialized"));
    return true;
}

int WeatherModule::update() {
    if (!_initialized) {
        return 0;
    }

    unsigned long now = millis();

    // WiFi 연결 상태 확인
    _wifiConnected = (WiFi.status() == WL_CONNECTED);

    // 업데이트 주간 도달 시 날씨 새로고침
    if (_wifiConnected && (now - _lastUpdate >= UPDATE_INTERVAL_MS || _lastUpdate == 0)) {
        if (refresh()) {
            _lastUpdate = now;
            return 1;
        }
    }

    return 0;
}

bool WeatherModule::refresh() {
    if (!_wifiConnected) {
        // WiFi 연결 없음 - 캐시 데이터 사용
        Serial.println(F("[WeatherModule] No WiFi, using cache"));
        return loadFromCache();
    }

    // API 키 확인
    if (_apiKey[0] == '\0') {
        Serial.println(F("[WeatherModule] No API key, using cache"));
        return loadFromCache();
    }

    // OpenWeatherMap API 호출
    if (fetchWeatherFromAPI()) {
        // 성공 시 캐시에 저장
        saveToCache();

        // 이벤트 발행
        Event event;
        event.type = WEATHER_UPDATED;
        event.timestamp = millis();
        event.data = &_currentData;
        gEventBus.publish(event);

        Serial.print(F("[WeatherModule] Updated: "));
        Serial.print(_currentData.temperature, 1);
        Serial.print(F("C, "));
        Serial.print((int)_currentData.humidity);
        Serial.print(F("%, "));
        Serial.println(_currentData.description);
        return true;
    }

    // API 실패 시 캐시 사용
    Serial.println(F("[WeatherModule] API failed, using cache"));
    return loadFromCache();
}

bool WeatherModule::fetchWeatherFromAPI() {
    char urlBuf[API_URL_BUF_SIZE];

    // URL 생성: http://api.openweathermap.org/data/2.5/weather?q={city}&appid={key}&units=metric
    snprintf(urlBuf, sizeof(urlBuf),
        "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=metric",
        _location, _apiKey);

    // HTTP 요청
    if (_httpClient.begin(_wifiClient, urlBuf)) {
        int httpCode = _httpClient.GET();

        if (httpCode == HTTP_CODE_OK) {
            String payload = _httpClient.getString();
            _httpClient.end();

            // JSON 파싱
            StaticJsonDocument<WEATHER_JSON_BUF_SIZE> doc;
            DeserializationError error = deserializeJson(doc, payload);

            if (error) {
                Serial.print(F("[WeatherModule] JSON parse error: "));
                Serial.println(error.f_str());
                return false;
            }

            // 날씨 데이터 추출
            _currentData.temperature = doc["main"]["temp"].as<float>();
            _currentData.humidity = doc["main"]["humidity"].as<float>();
            _currentData.pressure = doc["main"]["pressure"].as<int>();
            _currentData.windSpeed = doc["wind"]["speed"].as<float>();

            // 날씨 상태
            int weatherCode = doc["weather"][0]["id"].as<int>();
            _currentData.condition = parseWeatherCondition(weatherCode);

            // 설명
            const char* desc = doc["weather"][0]["description"];
            if (desc) {
                strncpy(_currentData.description, desc, sizeof(_currentData.description) - 1);
                _currentData.description[sizeof(_currentData.description) - 1] = '\0';
            }

            // 위치
            const char* cityName = doc["name"];
            if (cityName) {
                strncpy(_currentData.location, cityName, sizeof(_currentData.location) - 1);
                _currentData.location[sizeof(_currentData.location) - 1] = '\0';
            }

            _currentData.timestamp = millis();
            return true;
        } else {
            Serial.print(F("[WeatherModule] HTTP error: "));
            Serial.println(httpCode);
            _httpClient.end();
            return false;
        }
    }

    Serial.println(F("[WeatherModule] HTTP begin failed"));
    return false;
}

bool WeatherModule::loadFromCache() {
    char cacheKey[64];
    snprintf(cacheKey, sizeof(cacheKey), "weather_%s", _location);

    char jsonBuf[WEATHER_JSON_BUF_SIZE];
    if (CacheMgr.get(cacheKey, jsonBuf, sizeof(jsonBuf))) {
        return jsonToWeatherData(jsonBuf);
    }

    // 캐시 없음 - 빈 데이터 반환
    Serial.println(F("[WeatherModule] No cache data"));
    return false;
}

bool WeatherModule::saveToCache() {
    char cacheKey[64];
    snprintf(cacheKey, sizeof(cacheKey), "weather_%s", _location);

    char jsonBuf[WEATHER_JSON_BUF_SIZE];
    if (weatherDataToJson(jsonBuf, sizeof(jsonBuf))) {
        return CacheMgr.set(cacheKey, jsonBuf, CACHE_TTL_MS);
    }

    return false;
}

bool WeatherModule::weatherDataToJson(char* outBuf, size_t maxLen) {
    StaticJsonDocument<WEATHER_JSON_BUF_SIZE> doc;

    doc["temp"] = _currentData.temperature;
    doc["humidity"] = _currentData.humidity;
    doc["pressure"] = _currentData.pressure;
    doc["wind"] = _currentData.windSpeed;
    doc["condition"] = (int)_currentData.condition;
    doc["desc"] = _currentData.description;
    doc["location"] = _currentData.location;
    doc["timestamp"] = _currentData.timestamp;

    size_t len = serializeJson(doc, outBuf, maxLen);
    return (len > 0 && len < maxLen);
}

bool WeatherModule::jsonToWeatherData(const char* jsonStr) {
    StaticJsonDocument<WEATHER_JSON_BUF_SIZE> doc;
    DeserializationError error = deserializeJson(doc, jsonStr);

    if (error) {
        Serial.print(F("[WeatherModule] Cache JSON parse error: "));
        Serial.println(error.f_str());
        return false;
    }

    _currentData.temperature = doc["temp"].as<float>();
    _currentData.humidity = doc["humidity"].as<float>();
    _currentData.pressure = doc["pressure"].as<int>();
    _currentData.windSpeed = doc["wind"].as<float>();
    _currentData.condition = (WeatherCondition)doc["condition"].as<int>();

    const char* desc = doc["desc"];
    if (desc) {
        strncpy(_currentData.description, desc, sizeof(_currentData.description) - 1);
        _currentData.description[sizeof(_currentData.description) - 1] = '\0';
    }

    const char* location = doc["location"];
    if (location) {
        strncpy(_currentData.location, location, sizeof(_currentData.location) - 1);
        _currentData.location[sizeof(_currentData.location) - 1] = '\0';
    }

    _currentData.timestamp = doc["timestamp"].as<unsigned long>();
    return true;
}

bool WeatherModule::setApiKey(const char* apiKey) {
    strncpy(_apiKey, apiKey, sizeof(_apiKey) - 1);
    _apiKey[sizeof(_apiKey) - 1] = '\0';

    // ConfigManager에 저장
    return ConfigMgr.set("weather_api_key", apiKey);
}

bool WeatherModule::getApiKey(char* outBuf, size_t maxLen) {
    return ConfigMgr.get("weather_api_key", outBuf, maxLen, "");
}

bool WeatherModule::setLocation(const char* location) {
    strncpy(_location, location, sizeof(_location) - 1);
    _location[sizeof(_location) - 1] = '\0';

    // ConfigManager에 저장
    return ConfigMgr.set("weather_location", location);
}

bool WeatherModule::getLocation(char* outBuf, size_t maxLen) {
    return ConfigMgr.get("weather_location", outBuf, maxLen, "Seoul,KR");
}

const char* WeatherModule::conditionToString(WeatherCondition condition) {
    switch (condition) {
        case CLEAR:        return "Clear";
        case CLOUDY:       return "Cloudy";
        case RAIN:         return "Rain";
        case SNOW:         return "Snow";
        case THUNDERSTORM: return "Thunderstorm";
        case MIST:         return "Mist";
        default:           return "Unknown";
    }
}

// @MX:NOTE: [OpenWeatherMap 코드 매핑] WMO 코드를 WeatherCondition으로 변환
// 참고: https://openweathermap.org/weather-conditions
WeatherModule::WeatherCondition WeatherModule::parseWeatherCondition(int code) {
    // 2xx: 천둥暴風
    if (code >= 200 && code < 300) {
        return THUNDERSTORM;
    }

    // 3xx: 안개
    if (code >= 300 && code < 400) {
        return MIST;
    }

    // 5xx: 비
    if (code >= 500 && code < 600) {
        return RAIN;
    }

    // 6xx: 눈
    if (code >= 600 && code < 700) {
        return SNOW;
    }

    // 7xx: 안개/연무
    if (code >= 700 && code < 800) {
        return MIST;
    }

    // 800: 맑음
    if (code == 800) {
        return CLEAR;
    }

    // 80x: 구름
    if (code > 800 && code < 900) {
        return CLOUDY;
    }

    return UNKNOWN;
}

void WeatherModule::urlEncode(char* dest, const char* src, size_t maxLen) {
    size_t i = 0;
    while (*src && i < maxLen - 1) {
        if (*src == ' ') {
            if (i < maxLen - 3) {
                *dest++ = '%';
                *dest++ = '2';
                *dest++ = '0';
                i += 3;
            } else {
                break;
            }
        } else {
            *dest++ = *src;
            i++;
        }
        src++;
    }
    *dest = '\0';
}

void WeatherModule::onWiFiEvent(const Event& event, void* userData) {
    WeatherModule* module = static_cast<WeatherModule*>(userData);

    if (event.type == WIFI_CONNECTED) {
        module->setWiFiConnected(true);
        // WiFi 연결 시 즉시 날씨 업데이트 시도
        module->refresh();
    } else if (event.type == WIFI_DISCONNECTED) {
        module->setWiFiConnected(false);
    }
}
