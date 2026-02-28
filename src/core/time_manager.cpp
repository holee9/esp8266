// @MX:NOTE: [AUTO] TimeManager 구현 - NTP 기반 시간 동기화

#include "time_manager.h"
#include "event_bus.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// 전역 인스턴스 정의
TimeManager gTimeManager;

// NTP 패킷 크기 (48 바이트)
static const int NTP_PACKET_SIZE = 48;

// NTP 전용 버퍼 (정적 할당)
static byte _ntpPacketBuffer[NTP_PACKET_SIZE];

// UDP 인스턴스 (전역 - WiFiUDP 내부 버퍼 관리)
static WiFiUDP _ntpUdp;

TimeManager::TimeManager()
    : _initialized(false)
    , _isSynced(false)
    , _isSyncing(false)
    , _lastSyncTime(0)
    , _lastSyncAttempt(0)
{
}

void TimeManager::begin() {
    Serial.println(F("TimeManager: Initializing..."));

    _initialized = true;
    _isSynced = false;
    _isSyncing = false;
    _lastSyncTime = 0;
    _lastSyncAttempt = 0;

    Serial.println(F("TimeManager: Ready (will sync on WiFi connection)"));
}

void TimeManager::update() {
    if (!_initialized) {
        return;
    }

    unsigned long now = millis();

    // 동기화 필요 여부 확인
    bool needsSync = false;

    if (!_isSynced && _lastSyncAttempt == 0) {
        // 아직 동기화된 적 없음
        needsSync = true;
    } else if (!_isSynced && (now - _lastSyncAttempt >= SYNC_RETRY_INTERVAL_MS)) {
        // 동기화 실패 후 재시도 시간 도달
        needsSync = true;
    } else if (_isSynced && (now - _lastSyncTime >= SYNC_INTERVAL_MS)) {
        // 정기 재동기화 시간 도달
        needsSync = true;
    }

    if (needsSync && WiFi.status() == WL_CONNECTED) {
        syncNow();
    }

    // 타임아웃 처리
    if (_isSyncing && (now - _lastSyncAttempt >= SYNC_TIMEOUT_MS)) {
        Serial.println(F("TimeManager: Sync timeout"));
        _isSyncing = false;
    }
}

bool TimeManager::syncNow() {
    if (!_initialized) {
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("TimeManager: WiFi not connected, skipping sync"));
        return false;
    }

    if (_isSyncing) {
        Serial.println(F("TimeManager: Already syncing"));
        return false;
    }

    Serial.println(F("TimeManager: Starting NTP sync..."));
    _isSyncing = true;
    _lastSyncAttempt = millis();

    return performSync();
}

bool TimeManager::performSync() {
    // NTP UDP 시작 (포트 123)
    if (!_ntpUdp.begin(123)) {
        Serial.println(F("TimeManager: UDP begin failed"));
        _isSyncing = false;
        return false;
    }

    // NTP 서버 주소 설정
    _ntpUdp.flush();

    // NTP 요청 패킷 구성
    memset(_ntpPacketBuffer, 0, NTP_PACKET_SIZE);
    _ntpPacketBuffer[0] = 0b11100011;  // LI, Version, Mode
    _ntpPacketBuffer[1] = 0;           // Stratum
    _ntpPacketBuffer[2] = 6;           // Polling Interval
    _ntpPacketBuffer[3] = 0xEC;        // Peer Clock Precision
    // 8바이트 Zero (Root Delay & Root Dispersion)
    // 8바이트 Zero (Reference ID)
    // NTP 서버 주소 (전송 시 설정됨)

    // NTP 요청 전송
    if (!_ntpUdp.beginPacket(NTP_SERVER, 123)) {
        Serial.println(F("TimeManager: beginPacket failed"));
        _ntpUdp.stop();
        _isSyncing = false;
        return false;
    }

    _ntpUdp.write(_ntpPacketBuffer, NTP_PACKET_SIZE);

    if (!_ntpUdp.endPacket()) {
        Serial.println(F("TimeManager: endPacket failed"));
        _ntpUdp.stop();
        _isSyncing = false;
        return false;
    }

    // 응답 대기 (최대 1초)
    int timeout = 0;
    const int MAX_TIMEOUT = 10;  // 100ms * 10 = 1초

    while (timeout < MAX_TIMEOUT) {
        delay(100);

        int packetSize = _ntpUdp.parsePacket();
        if (packetSize >= NTP_PACKET_SIZE) {
            // NTP 응답 수신
            _ntpUdp.read(_ntpPacketBuffer, NTP_PACKET_SIZE);

            // 타임스탬프 추출 (전송 시각: bytes 40-43)
            unsigned long secsSince1900;
            secsSince1900 = (unsigned long)_ntpPacketBuffer[40] << 24;
            secsSince1900 |= (unsigned long)_ntpPacketBuffer[41] << 16;
            secsSince1900 |= (unsigned long)_ntpPacketBuffer[42] << 8;
            secsSince1900 |= (unsigned long)_ntpPacketBuffer[43];

            // Unix 타임으로 변환 (1900년 → 1970년: 70년 + 17 leap days)
            const unsigned long SEVENTY_YEARS = 2208988800UL;
            unsigned long epoch = secsSince1900 - SEVENTY_YEARS;

            // 시간 설정 (timezone 적용: KST = UTC + 9시간)
            time_t localTime = epoch + NTP_TIMEZONE_OFFSET_SEC;
            struct timeval tv = { .tv_sec = localTime, .tv_usec = 0 };
            settimeofday(&tv, nullptr);

            _isSynced = true;
            _isSyncing = false;
            _lastSyncTime = millis();

            // 현재 시간 출력
            char timeBuf[32];
            struct tm* tmInfo = localtime(&localTime);
            strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", tmInfo);

            Serial.printf("TimeManager: Synced! Local time: %s\n", timeBuf);

            _ntpUdp.stop();

            // 이벤트 발행
            notifyTimeSynced();

            return true;
        }

        timeout++;
    }

    // 타임아웃
    Serial.println(F("TimeManager: No NTP response"));
    _ntpUdp.stop();
    _isSyncing = false;

    return false;
}

void TimeManager::getFormattedTime(char* timeBuf, size_t bufSize) {
    if (timeBuf == nullptr || bufSize < 9) {
        return;
    }

    time_t now = time(nullptr);
    struct tm* tmInfo = localtime(&now);

    snprintf(timeBuf, bufSize, "%02d:%02d:%02d",
             tmInfo->tm_hour, tmInfo->tm_min, tmInfo->tm_sec);
}

void TimeManager::getFormattedDate(char* dateBuf, size_t bufSize) {
    if (dateBuf == nullptr || bufSize < 11) {
        return;
    }

    time_t now = time(nullptr);
    struct tm* tmInfo = localtime(&now);

    snprintf(dateBuf, bufSize, "%04d-%02d-%02d",
             tmInfo->tm_year + 1900, tmInfo->tm_mon + 1, tmInfo->tm_mday);
}

void TimeManager::getFormattedDateTime(char* dateTimeBuf, size_t bufSize) {
    if (dateTimeBuf == nullptr || bufSize < 32) {
        return;
    }

    time_t now = time(nullptr);
    struct tm* tmInfo = localtime(&now);

    // 한국어 형식: "2024년 2월 28일 수요일 14:30:00"
    const char* weekDays[] = {"일", "월", "화", "수", "목", "금", "토"};

    snprintf(dateTimeBuf, bufSize, "%04d년 %2d월 %2d일 (%s요일) %02d:%02d:%02d",
             tmInfo->tm_year + 1900,
             tmInfo->tm_mon + 1,
             tmInfo->tm_mday,
             weekDays[tmInfo->tm_wday],
             tmInfo->tm_hour,
             tmInfo->tm_min,
             tmInfo->tm_sec);
}

unsigned long TimeManager::getTimestamp() {
    return (unsigned long)time(nullptr);
}

bool TimeManager::isSynced() {
    return _isSynced;
}

unsigned long TimeManager::getLastSyncTime() {
    return _lastSyncTime;
}

void TimeManager::notifyTimeSynced() {
    // TIME_SYNCED 이벤트 발행
    Event event;
    event.type = TIME_SYNCED;
    event.timestamp = millis();
    event.data = nullptr;

    gEventBus.publish(event);
}
