#ifndef ARTHUR_CACHE_MANAGER_H
#define ARTHUR_CACHE_MANAGER_H

#include <Arduino.h>
#include <FS.h>
#include "arthur_littlefs.h"

/**
 * @brief CacheManager
 *
 * LittleFS 기반 TTL 캐시 관리자
 * - 키-값 저장소
 * - TTL (Time To Live) 기반 만료
 * - 정적 할당만 사용 (new/malloc 금지)
 * - String 클래스 미사용 (char[] + F() 매크로)
 *
 * @MX:NOTE: [파일 기반 캐시] 각 캐시 항목은 별도 파일로 저장
 * @MX:ANCHOR: [캐시 인터페이스] 모듈 전체에서 데이터 캐싱에 사용
 * @MX:REASON: fan_in >= 3 (WeatherModule, TimeManager 등)
 */
class CacheManager {
public:
    /**
     * @brief 생성자
     */
    CacheManager();

    /**
     * @brief LittleFS 초기화 및 캐시 디렉토리 확인
     *
     * @return true 초기화 성공
     * @return false 초기화 실패
     */
    bool begin();

    /**
     * @brief 캐시에서 값 가져오기
     *
     * @param key 캐시 키
     * @param outValue 출력 버퍼
     * @param maxLen 버퍼 크기
     * @return true 값 찾음 (만료되지 않음)
     * @return false 값 없음 또는 만료됨
     */
    bool get(const char* key, char* outValue, size_t maxLen);

    /**
     * @brief 캐시에 값 저장
     *
     * @param key 캐시 키
     * @param value 저장할 값
     * @param ttlMillis TTL (밀리초), 0이면 기본값 사용
     * @return true 저장 성공
     * @return false 저장 실패
     */
    bool set(const char* key, const char* value, unsigned long ttlMillis = 0);

    /**
     * @brief 캐시 항목 존재 확인 (만료 체크 포함)
     *
     * @param key 캐시 키
     * @return true 존재하고 만료되지 않음
     * @return false 없음 또는 만료됨
     */
    bool has(const char* key);

    /**
     * @brief 캐시 항목 삭제
     *
     * @param key 캐시 키
     * @return true 삭제 성공
     * @return false 삭제 실패 (없음 등)
     */
    bool remove(const char* key);

    /**
     * @brief 캐시 항목의 남은 TTL 확인
     *
     * @param key 캐시 키
     * @return long 남은 TTL (밀리초), -1이면 없음 또는 만료됨
     */
    long getTTL(const char* key);

    /**
     * @brief 모든 만료된 캐시 정리
     *
     * @return int 정리된 항목 수
     */
    int cleanup();

    /**
     * @brief 모든 캐시 비우기
     *
     * @return true 성공
     * @return false 실패
     */
    bool clear();

    /**
     * @brief 캐시 항목 수 확인
     *
     * @return int 캐시 항목 수
     */
    int count();

    /**
     * @brief 기본 TTL 설정
     *
     * @param ttlMillis 기본 TTL (밀리초)
     */
    void setDefaultTTL(unsigned long ttlMillis) {
        _defaultTTL = ttlMillis;
    }

    /**
     * @brief 현재 사용 중인 힙 메모리 양
     *
     * @return size_t 남은 힙 (바이트)
     */
    size_t getFreeHeap() const;

private:
    bool _mounted;
    unsigned long _defaultTTL;

    // 키로 파일 경로 생성
    void getFilePath(const char* key, char* outPath, size_t maxLen);

    // 메타데이터 파일 경로 생성
    void getMetaPath(const char* key, char* outPath, size_t maxLen);

    // 만료 시간 계산
    unsigned long getExpiryTime(unsigned long ttlMillis);

    // LittleFS 마운트 (내부 사용)
    bool mount();

    // 만료 체크
    bool isExpired(const char* key);
};

// 전역 인스턴스
extern CacheManager CacheMgr;

#endif // ARTHUR_CACHE_MANAGER_H
