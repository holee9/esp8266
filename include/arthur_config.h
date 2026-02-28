#ifndef ARTHUR_CONFIG_H
#define ARTHUR_CONFIG_H

// 프로젝트 정보
#ifndef ARTHUR_VERSION
#define ARTHUR_VERSION "0.1.0"
#endif

// 디버그 레벨 (0=OFF, 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG)
#ifndef ARTHUR_LOG_LEVEL
#define ARTHUR_LOG_LEVEL 3
#endif

// 메모리 안전 마진 (바이트)
#define HEAP_SAFETY_MARGIN 9216  // 9KB

// WiFi 설정
#define WIFI_CONNECT_TIMEOUT_MS 15000
#define WIFI_AP_SSID "ARTHUR-Setup"

// NTP 설정
#define NTP_SERVER "pool.ntp.org"
#define NTP_TIMEZONE_OFFSET 9  // KST (UTC+9)
#define NTP_SYNC_INTERVAL_MS 3600000  // 1시간

// OLED 업데이트 간격
#define DISPLAY_UPDATE_INTERVAL_MS 1000

// 센서 읽기 간격
#define SENSOR_READ_INTERVAL_MS 5000

// 날씨 업데이트 간격
#define WEATHER_UPDATE_INTERVAL_MS 600000  // 10분

// MQTT 설정
#define MQTT_BUFFER_SIZE 256
#define MQTT_KEEPALIVE_SEC 30

// 프록시 heartbeat 간격
#define PROXY_HEARTBEAT_INTERVAL_MS 30000  // 30초

// 문자열 버퍼 크기 (String 클래스 대신 고정 버퍼)
#define SMALL_BUF  32
#define MEDIUM_BUF 64
#define LARGE_BUF  128

#endif // ARTHUR_CONFIG_H
