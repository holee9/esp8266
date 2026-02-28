// @MX:NOTE: [AUTO] NTP 시간 동기화 관리자
// @MX:ANCHOR: [AUTO] 시간 소스 제공 - 여러 모듈에서 시간 정보 사용
// @MX:REASON: fan_in >= 3 (ClockModule, SensorModule, WeatherModule 등에서 호출)

#ifndef ARTHUR_TIME_MANAGER_H
#define ARTHUR_TIME_MANAGER_H

#include <Arduino.h>
#include <time.h>

// NTP 서버 설정
#ifndef NTP_SERVER
#define NTP_SERVER "pool.ntp.org"
#endif

// 타임존 오프셋 (초 단위)
#ifndef NTP_TIMEZONE_OFFSET_SEC
#define NTP_TIMEZONE_OFFSET_SEC (9 * 3600)  // KST (UTC+9)
#endif

/**
 * @brief TimeManager 클래스
 *
 * NTP를 통한 시간 동기화 및 현재 시간 제공
 * - WiFi 연결 후 자동으로 NTP 동기화
 * - 1시간마다 재동기화
 * - timezone: KST (UTC+9)
 * - String 클래스 미사용
 */
class TimeManager {
public:
    TimeManager();
    ~TimeManager() = default;

    /**
     * @brief TimeManager 초기화
     *
     * NTP 설정을 구성하고 첫 동기화 스케줄링
     */
    void begin();

    /**
     * @brief 정기 업데이트 (loop에서 호출)
     *
     * 동기화 주기 확인 및 NTP 갱신 실행
     */
    void update();

    /**
     * @brief 수동 NTP 동기화 요청
     *
     * @return true 동기화 시작 성공
     * @return false WiFi 미연결 또는 이미 동기화 중
     */
    bool syncNow();

    /**
     * @brief 현재 시간 가져오기
     *
     * @param timeBuf 시간 문자열을 저장할 버퍼 (HH:MM:SS 형식)
     * @param bufSize 버퍼 크기 (최소 9 바이트 권장)
     */
    void getFormattedTime(char* timeBuf, size_t bufSize);

    /**
     * @brief 현재 날짜 가져오기
     *
     * @param dateBuf 날짜 문자열을 저장할 버퍼 (YYYY-MM-DD 형식)
     * @param bufSize 버퍼 크기 (최소 11 바이트 권장)
     */
    void getFormattedDate(char* dateBuf, size_t bufSize);

    /**
     * @brief 날짜+시간 가져오기 (한국 형식)
     *
     * @param dateTimeBuf 버퍼 (최소 32 바이트 권장)
     * @param bufSize 버퍼 크기
     */
    void getFormattedDateTime(char* dateTimeBuf, size_t bufSize);

    /**
     * @brief 현재 Unix 타임스탬프 가져오기
     *
     * @return unsigned long 초 단위 타임스탬프
     */
    unsigned long getTimestamp();

    /**
     * @brief 시간 동기화 완료 여부 확인
     *
     * @return true 시간이 동기화됨
     * @return false 아직 동기화되지 않음
     */
    bool isSynced();

    /**
     * @brief 마지막 동기화 시간 가져오기
     *
     * @return unsigned long 마지막 성공적 동기화의 millis() 값
     */
    unsigned long getLastSyncTime();

private:
    bool _initialized;
    bool _isSynced;
    bool _isSyncing;
    unsigned long _lastSyncTime;       // 마지막 동기화 성공 시각 (millis)
    unsigned long _lastSyncAttempt;   // 마지막 동기화 시도 시각 (millis)

    static const unsigned long SYNC_INTERVAL_MS = 3600000;  // 1시간
    static const unsigned long SYNC_RETRY_INTERVAL_MS = 30000;  // 30초 (실패 시)
    static const unsigned long SYNC_TIMEOUT_MS = 15000;  // 15초 타임아웃

    /**
     * @brief NTP 동기화 실행 (내부용)
     */
    bool performSync();

    /**
     * @brief 시간 동기화 완료 이벤트 발행
     */
    void notifyTimeSynced();
};

// 전역 인스턴스
extern TimeManager gTimeManager;

#endif // ARTHUR_TIME_MANAGER_H
