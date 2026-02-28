#include "cache_manager.h"
#include <LittleFS.h>

// 전역 인스턴스
CacheManager CacheMgr;

// @MX:ANCHOR: [캐시 관리자 초기화] 부팅 시 캐시 시스템 초기화
// @MX:REASON: 시스템 진입점, begin()에서 LittleFS 마운트
CacheManager::CacheManager()
    : _mounted(false)
    , _defaultTTL(LITTLEFS_CACHE_DEFAULT_TTL)
{
}

bool CacheManager::begin() {
    if (!mount()) {
        Serial.println(F("[CacheMgr] LittleFS mount FAILED"));
        return false;
    }

    // 캐시 디렉토리 확인
    if (!LittleFS.exists(LITTLEFS_DIR_CACHE)) {
        LittleFS.mkdir(LITTLEFS_DIR_CACHE);
    }

    Serial.println(F("[CacheMgr] Cache initialized"));
    return true;
}

bool CacheManager::mount() {
    if (_mounted) {
        return true;
    }

    if (!LittleFS.begin()) {
        Serial.println(F("[CacheMgr] LittleFS.begin() failed"));
        return false;
    }

    _mounted = true;
    return true;
}

void CacheManager::getFilePath(const char* key, char* outPath, size_t maxLen) {
    // 캐시 파일 경로: /cache/{key}
    snprintf(outPath, maxLen, "%s%s", LITTLEFS_CACHE_PREFIX, key);
}

void CacheManager::getMetaPath(const char* key, char* outPath, size_t maxLen) {
    // 메타데이터 파일 경로: /cache/.{key}.meta
    snprintf(outPath, maxLen, "%s.%s.meta", LITTLEFS_CACHE_PREFIX, key);
}

unsigned long CacheManager::getExpiryTime(unsigned long ttlMillis) {
    if (ttlMillis == 0) {
        ttlMillis = _defaultTTL;
    }
    return millis() + ttlMillis;
}

bool CacheManager::isExpired(const char* key) {
    char metaPath[LITTLEFS_MAX_PATH_LEN];
    getMetaPath(key, metaPath, sizeof(metaPath));

    File metaFile = LittleFS.open(metaPath, "r");
    if (!metaFile) {
        // 메타데이터 없음 = 만료됨
        return true;
    }

    // 만료 시간 읽기 (8바이트 unsigned long)
    unsigned long expiryTime = 0;
    size_t bytesRead = metaFile.read((uint8_t*)&expiryTime, sizeof(expiryTime));
    metaFile.close();

    if (bytesRead != sizeof(expiryTime)) {
        return true;  // 읽기 실패 = 만료됨
    }

    // @MX:NOTE: [millis 오버플로우] millis()는 약 49일 후 0으로 되돌아감
    // 오버플로우 고려: 만료 시간이 현재 시간보다 작으면 만료로 간주
    // 오버플로우 시나리오: expiryTime=0xFFFFFFFF, millis()=0x00000100
    // (expiryTime - millis())이 큰 양수이면 아직 유효

    unsigned long now = millis();
    unsigned long remaining = expiryTime - now;

    // remaining이 0xFFFFFFFF보다 크면 오버플로우 발생 (음수로 계산됨)
    // 즉, remaining > 0x7FFFFFFF 이면 만료로 간주 (대략 24일 이상 차이)
    if (remaining > 0x7FFFFFFFUL) {
        return true;
    }

    return false;
}

bool CacheManager::get(const char* key, char* outValue, size_t maxLen) {
    // 만료 체크
    if (isExpired(key)) {
        remove(key);  // 만료된 항목 삭제
        return false;
    }

    char filePath[LITTLEFS_MAX_PATH_LEN];
    getFilePath(key, filePath, sizeof(filePath));

    File file = LittleFS.open(filePath, "r");
    if (!file) {
        return false;
    }

    // 값 읽기
    size_t bytesRead = file.read((uint8_t*)outValue, maxLen - 1);
    file.close();

    if (bytesRead == 0) {
        return false;
    }

    outValue[bytesRead] = '\0';
    return true;
}

bool CacheManager::set(const char* key, const char* value, unsigned long ttlMillis) {
    char filePath[LITTLEFS_MAX_PATH_LEN];
    char metaPath[LITTLEFS_MAX_PATH_LEN];

    getFilePath(key, filePath, sizeof(filePath));
    getMetaPath(key, metaPath, sizeof(metaPath));

    // 값 파일 쓰기
    File dataFile = LittleFS.open(filePath, "w");
    if (!dataFile) {
        Serial.print(F("[CacheMgr] Failed to open data file: "));
        Serial.println(filePath);
        return false;
    }

    size_t valueLen = strlen(value);
    size_t written = dataFile.write((const uint8_t*)value, valueLen);
    dataFile.close();

    if (written != valueLen) {
        Serial.print(F("[CacheMgr] Write mismatch: "));
        Serial.print(written);
        Serial.print(F(" vs "));
        Serial.println(valueLen);
        return false;
    }

    // 메타데이터 파일 쓰기 (만료 시간)
    File metaFile = LittleFS.open(metaPath, "w");
    if (!metaFile) {
        Serial.print(F("[CacheMgr] Failed to open meta file: "));
        Serial.println(metaPath);
        return false;
    }

    unsigned long expiryTime = getExpiryTime(ttlMillis);
    written = metaFile.write((const uint8_t*)&expiryTime, sizeof(expiryTime));
    metaFile.close();

    if (written != sizeof(expiryTime)) {
        Serial.println(F("[CacheMgr] Meta write failed"));
        return false;
    }

    return true;
}

bool CacheManager::has(const char* key) {
    // 만료 체크 포함
    if (isExpired(key)) {
        remove(key);
        return false;
    }

    char filePath[LITTLEFS_MAX_PATH_LEN];
    getFilePath(key, filePath, sizeof(filePath));
    return LittleFS.exists(filePath);
}

bool CacheManager::remove(const char* key) {
    char filePath[LITTLEFS_MAX_PATH_LEN];
    char metaPath[LITTLEFS_MAX_PATH_LEN];

    getFilePath(key, filePath, sizeof(filePath));
    getMetaPath(key, metaPath, sizeof(metaPath));

    bool result = true;

    if (LittleFS.exists(filePath)) {
        if (!LittleFS.remove(filePath)) {
            result = false;
        }
    }

    if (LittleFS.exists(metaPath)) {
        if (!LittleFS.remove(metaPath)) {
            result = false;
        }
    }

    return result;
}

long CacheManager::getTTL(const char* key) {
    char metaPath[LITTLEFS_MAX_PATH_LEN];
    getMetaPath(key, metaPath, sizeof(metaPath));

    File metaFile = LittleFS.open(metaPath, "r");
    if (!metaFile) {
        return -1;
    }

    unsigned long expiryTime = 0;
    size_t bytesRead = metaFile.read((uint8_t*)&expiryTime, sizeof(expiryTime));
    metaFile.close();

    if (bytesRead != sizeof(expiryTime)) {
        return -1;
    }

    unsigned long now = millis();
    long remaining = (long)(expiryTime - now);

    // 오버플로우 체크
    if (remaining < 0 || (unsigned long)remaining > 0x7FFFFFFFUL) {
        return -1;  // 만료됨
    }

    return remaining;
}

int CacheManager::cleanup() {
    int cleaned = 0;
    Dir dir = LittleFS.openDir(LITTLEFS_DIR_CACHE);

    while (dir.next()) {
        // 메타데이터 파일만 체크
        const char* fileName = dir.fileName().c_str();

        // .key.meta 형식 확인
        if (fileName[0] == '.' && strstr(fileName, ".meta") != nullptr) {
            // 키 추출: ".key.meta" -> "key"
            char key[LITTLEFS_MAX_KEY_LEN];
            strncpy(key, fileName + 1, sizeof(key) - 1);
            key[sizeof(key) - 1] = '\0';

            // ".meta" 제거
            char* dotMeta = strstr(key, ".meta");
            if (dotMeta) {
                *dotMeta = '\0';
            }

            // 만료 체크
            if (isExpired(key)) {
                remove(key);
                cleaned++;
            }
        }
    }

    if (cleaned > 0) {
        Serial.print(F("[CacheMgr] Cleaned "));
        Serial.print(cleaned);
        Serial.println(F(" expired items"));
    }

    return cleaned;
}

bool CacheManager::clear() {
    Dir dir = LittleFS.openDir(LITTLEFS_DIR_CACHE);

    while (dir.next()) {
        const char* fileName = dir.fileName().c_str();
        char fullPath[LITTLEFS_MAX_PATH_LEN];
        snprintf(fullPath, sizeof(fullPath), "%s%s", LITTLEFS_CACHE_PREFIX, fileName);
        LittleFS.remove(fullPath);
    }

    Serial.println(F("[CacheMgr] All cache cleared"));
    return true;
}

int CacheManager::count() {
    int count = 0;
    Dir dir = LittleFS.openDir(LITTLEFS_DIR_CACHE);

    while (dir.next()) {
        const char* fileName = dir.fileName().c_str();
        // 데이터 파일만 카운트 (.meta 제외)
        if (fileName[0] != '.') {
            count++;
        }
    }

    return count / 2;  // 데이터 + 메타데이터 쌍이므로 2로 나눔
}

size_t CacheManager::getFreeHeap() const {
    return ESP.getFreeHeap();
}
