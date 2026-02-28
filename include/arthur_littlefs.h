#ifndef ARTHUR_LITTLEFS_H
#define ARTHUR_LITTLEFS_H

// LittleFS 파일 시스템 경로 상수
// ESP8266 4MB Flash (1MB sketch + 3MB LittleFS)

// 최대 경로 길이 (파일 시스템 제한 고려)
#define LITTLEFS_MAX_PATH_LEN 64

// 기본 디렉토리 경로
#define LITTLEFS_DIR_CONFIG  "/config"
#define LITTLEFS_DIR_CACHE   "/cache"
#define LITTLEFS_DIR_LOGS    "/logs"
#define LITTLEFS_DIR_CERTS   "/certs"
#define LITTLEFS_DIR_ASSETS  "/assets"

// 설정 파일 경로
#define LITTLEFS_CONFIG_FILE      "/config/device.json"
#define LITTLEFS_CONFIG_TEMP_FILE "/config/device.tmp"

// 캐시 파일 경로 (key는 파일명으로 사용)
#define LITTLEFS_CACHE_PREFIX     "/cache/"

// 로그 파일 경로
#define LITTLEFS_LOG_FILE         "/logs/app.log"

// 캐시 관련 상수
#define LITTLEFS_CACHE_DEFAULT_TTL 3600000  // 1시간 (밀리초)
#define LITTLEFS_MAX_CACHE_SIZE   4096      // 4KB (단일 캐시 항목)
#define LITTLEFS_MAX_KEY_LEN      32        // 캐시 키 최대 길이

#endif // ARTHUR_LITTLEFS_H
