#include "config_manager.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

// 전역 인스턴스
ConfigManager ConfigMgr;

// @MX:ANCHOR: [설정 관리자 초기화] 부팅 시 첫 호출점
// @MX:REASON: 시스템 진입점, begin()에서 마운트 및 설정 로드
ConfigManager::ConfigManager()
    : _mounted(false)
    , _dirty(false)
    , _loaded(false)
{
}

bool ConfigManager::begin() {
    // LittleFS 마운트 시도
    if (!mount()) {
        Serial.println(F("[ConfigMgr] LittleFS mount FAILED"));
        return false;
    }

    // 디렉토리 구조 확인
    ensureDirectories();

    // 설정 파일 로드
    if (load()) {
        Serial.println(F("[ConfigMgr] Config loaded"));
        return true;
    }

    // 설정 파일 없음 - 기본 설정으로 시작
    Serial.println(F("[ConfigMgr] No config found, using defaults"));
    _loaded = true;  // 빈 설정으로 로드 완료 표시
    return true;
}

bool ConfigManager::mount() {
    if (_mounted) {
        return true;  // 이미 마운트됨
    }

    // LittleFS 마운트 시도
    if (!LittleFS.begin()) {
        Serial.println(F("[ConfigMgr] LittleFS.begin() failed"));

        // 포맷 시도 (선택적 - 주석 처리)
        // if (LittleFS.format()) {
        //     Serial.println(F("[ConfigMgr] Formatted, retrying mount"));
        //     return LittleFS.begin();
        // }
        return false;
    }

    _mounted = true;

    // ESP8266 LittleFS info API
    FSInfo fs_info;
    if (LittleFS.info(fs_info)) {
        Serial.print(F("[ConfigMgr] LittleFS mounted ("));
        Serial.print(fs_info.totalBytes);
        Serial.print(F(" total, "));
        Serial.print(fs_info.usedBytes);
        Serial.println(F(" used)"));
    } else {
        Serial.println(F("[ConfigMgr] LittleFS mounted"));
    }

    return true;
}

bool ConfigManager::ensureDirectories() {
    // 필요한 디렉토리 생성
    const char* dirs[] = {
        LITTLEFS_DIR_CONFIG,
        LITTLEFS_DIR_CACHE,
        LITTLEFS_DIR_LOGS,
        LITTLEFS_DIR_CERTS,
        LITTLEFS_DIR_ASSETS
    };

    for (size_t i = 0; i < 5; i++) {
        if (!LittleFS.exists(dirs[i])) {
            if (LittleFS.mkdir(dirs[i])) {
                // Serial.printf("[ConfigMgr] Created dir: %s\n", dirs[i]);
            } else {
                Serial.print(F("[ConfigMgr] FAILED to create dir: "));
                Serial.println(dirs[i]);
            }
        }
    }

    return true;
}

bool ConfigManager::load() {
    File file = LittleFS.open(LITTLEFS_CONFIG_FILE, "r");
    if (!file) {
        // 파일 없음 = 첫 부팅
        return false;
    }

    // JSON 문서 파싱
    StaticJsonDocument<JSON_DOC_SIZE> doc;

    // deserializeJson의 DeserializationError 반환
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.print(F("[ConfigMgr] JSON parse error: "));
        Serial.println(error.f_str());
        return false;
    }

    // 성공적으로 로드됨
    // 내부적으로 doc의 내용은 get() 호출 시 사용됨
    // 여기서는 별도 캐싱하지 않고 필요시 파일에서 다시 읽거나,
    // 더 나은 방법은 JSON 문서를 멤버 변수로 보관하는 것

    // @MX:NOTE: [메모리 최적화] JSON 문서를 멤버로 보관하지 않음
    // 필요할 때마다 파일에서 읽어서 메모리 절약 ( trade-off: 성능 vs 메모리)

    _loaded = true;
    _dirty = false;
    return true;
}

bool ConfigManager::get(const char* key, char* outValue, size_t maxLen, const char* defaultValue) {
    if (!_loaded && !load()) {
        // 로드 실패 시 기본값 사용
        strncpy(outValue, defaultValue, maxLen - 1);
        outValue[maxLen - 1] = '\0';
        return false;
    }

    File file = LittleFS.open(LITTLEFS_CONFIG_FILE, "r");
    if (!file) {
        strncpy(outValue, defaultValue, maxLen - 1);
        outValue[maxLen - 1] = '\0';
        return false;
    }

    StaticJsonDocument<JSON_DOC_SIZE> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        strncpy(outValue, defaultValue, maxLen - 1);
        outValue[maxLen - 1] = '\0';
        return false;
    }

    // 키로 값 검색
    JsonVariant value = doc[key];
    if (!value.is<const char*>()) {
        strncpy(outValue, defaultValue, maxLen - 1);
        outValue[maxLen - 1] = '\0';
        return false;
    }

    const char* strValue = value.as<const char*>();
    strncpy(outValue, strValue, maxLen - 1);
    outValue[maxLen - 1] = '\0';
    return true;
}

int ConfigManager::getInt(const char* key, int defaultValue) {
    char buf[MAX_VALUE_STR_LEN];
    if (get(key, buf, sizeof(buf), "")) {
        return atoi(buf);
    }
    return defaultValue;
}

bool ConfigManager::getBool(const char* key, bool defaultValue) {
    char buf[MAX_VALUE_STR_LEN];
    if (get(key, buf, sizeof(buf), "")) {
        // true/"1"/"yes" 판별
        if (strcmp(buf, "true") == 0 ||
            strcmp(buf, "1") == 0 ||
            strcmp(buf, "yes") == 0 ||
            strcmp(buf, "YES") == 0) {
            return true;
        }
        if (strcmp(buf, "false") == 0 ||
            strcmp(buf, "0") == 0 ||
            strcmp(buf, "no") == 0 ||
            strcmp(buf, "NO") == 0) {
            return false;
        }
    }
    return defaultValue;
}

bool ConfigManager::set(const char* key, const char* value) {
    // 먼저 기존 설정 로드
    StaticJsonDocument<JSON_DOC_SIZE> doc;

    File file = LittleFS.open(LITTLEFS_CONFIG_FILE, "r");
    if (file) {
        deserializeJson(doc, file);
        file.close();
    }

    // 값 설정
    doc[key] = value;

    // 원자적 쓰기: 임시 파일에 쓰고 rename
    File tempFile = LittleFS.open(LITTLEFS_CONFIG_TEMP_FILE, "w");
    if (!tempFile) {
        Serial.println(F("[ConfigMgr] Failed to create temp file"));
        return false;
    }

    // JSON 직렬화 (Compact)
    if (serializeJson(doc, tempFile) == 0) {
        Serial.println(F("[ConfigMgr] Failed to write JSON"));
        tempFile.close();
        return false;
    }
    tempFile.close();

    // 기존 파일 삭제 후 임시 파일 이름 변경
    if (LittleFS.exists(LITTLEFS_CONFIG_FILE)) {
        LittleFS.remove(LITTLEFS_CONFIG_FILE);
    }

    if (!LittleFS.rename(LITTLEFS_CONFIG_TEMP_FILE, LITTLEFS_CONFIG_FILE)) {
        Serial.println(F("[ConfigMgr] Failed to rename temp file"));
        return false;
    }

    _dirty = false;
    _loaded = true;
    return true;
}

bool ConfigManager::setInt(const char* key, int value) {
    char buf[16];
    itoa(value, buf, 10);
    return set(key, buf);
}

bool ConfigManager::setBool(const char* key, bool value) {
    return set(key, value ? "true" : "false");
}

bool ConfigManager::save() {
    // set()에서 이미 자동 저장됨
    // dirty 상태일 때만 저장하도록 변경 가능
    return !_dirty;
}

bool ConfigManager::reload() {
    _loaded = false;
    return load();
}

bool ConfigManager::reset() {
    // 설정 파일 삭제
    if (LittleFS.exists(LITTLEFS_CONFIG_FILE)) {
        LittleFS.remove(LITTLEFS_CONFIG_FILE);
    }
    _loaded = false;
    _dirty = true;
    return true;
}

size_t ConfigManager::getFreeHeap() const {
    return ESP.getFreeHeap();
}
