#ifndef ARTHUR_CONFIG_MANAGER_H
#define ARTHUR_CONFIG_MANAGER_H

#include <Arduino.h>
#include <FS.h>
#include "arthur_littlefs.h"

/**
 * @brief ConfigManager
 *
 * LittleFS 기반 JSON 설정 관리자
 * - ArduinoJson 7.0.0 사용
 * - 원자적 쓰기 (temp 파일 + rename)
 * - 정적 할당만 사용 (new/malloc 금지)
 * - String 클래스 미사용 (char[] + F() 매크로)
 *
 * @MX:NOTE: [자동 마운트] 첫 begin() 호출 시 LittleFS 자동 마운트
 * @MX:ANCHOR: [설정 저장 인터페이스] 모듈 전체에서 설정 저장/로드에 사용
 * @MX:REASON: fan_in >= 3 (WeatherModule, TimeManager, SensorModule 등)
 */
class ConfigManager {
public:
    // JSON 문서 크기 (ESP8266 제한된 RAM 고려)
    static const size_t JSON_DOC_SIZE = 1024;

    // 키/값 최대 길이
    static const size_t MAX_KEY_LEN = 32;
    static const size_t MAX_VALUE_STR_LEN = 64;

    /**
     * @brief 생성자
     */
    ConfigManager();

    /**
     * @brief LittleFS 초기화 및 설정 로드
     *
     * @return true 초기화 성공
     * @return false 초기화 실패
     */
    bool begin();

    /**
     * @brief 문자열 값 가져오기
     *
     * @param key 설정 키
     * @param outValue 출력 버퍼
     * @param maxLen 버퍼 크기
     * @param defaultValue 기본값 (키가 없을 때)
     * @return true 값 찾음
     * @return false 값 없음 (기본값 사용)
     */
    bool get(const char* key, char* outValue, size_t maxLen, const char* defaultValue);

    /**
     * @brief 정수 값 가져오기
     *
     * @param key 설정 키
     * @param defaultValue 기본값
     * @return int 값
     */
    int getInt(const char* key, int defaultValue);

    /**
     * @brief 불리언 값 가져오기
     *
     * @param key 설정 키
     * @param defaultValue 기본값
     * @return true 값이 true/"1"/"yes"
     * @return false 값이 false/"0"/"no" 또는 키 없음
     */
    bool getBool(const char* key, bool defaultValue);

    /**
     * @brief 문자열 값 설정
     *
     * @param key 설정 키
     * @param value 설정 값
     * @return true 설정 성공
     * @return false 설정 실패
     */
    bool set(const char* key, const char* value);

    /**
     * @brief 정수 값 설정
     *
     * @param key 설정 키
     * @param value 설정 값
     * @return true 설정 성공
     * @return false 설정 실패
     */
    bool setInt(const char* key, int value);

    /**
     * @brief 불리언 값 설정
     *
     * @param key 설정 키
     * @param value 설정 값
     * @return true 설정 성공
     * @return false 설정 실패
     */
    bool setBool(const char* key, bool value);

    /**
     * @brief 설정을 파일에 저장 (원자적 쓰기)
     *
     * @return true 저장 성공
     * @return false 저장 실패
     */
    bool save();

    /**
     * @brief 설정이 변경되었는지 확인
     *
     * @return true 변경됨
     * @return false 변경 없음
     */
    bool isDirty() const { return _dirty; }

    /**
     * @brief 설정 파일 다시 로드
     *
     * @return true 로드 성공
     * @return false 로드 실패
     */
    bool reload();

    /**
     * @brief 설정 초기화 (기본값 복원)
     *
     * @return true 초기화 성공
     * @return false 초기화 실패
     */
    bool reset();

    /**
     * @brief 현재 사용 중인 힙 메모리 양
     *
     * @return size_t 남은 힙 (바이트)
     */
    size_t getFreeHeap() const;

private:
    bool _mounted;
    bool _dirty;
    bool _loaded;

    // LittleFS 마운트 (내부 사용)
    bool mount();

    // 설정 파일 로드 (내부 사용)
    bool load();

    // 디렉토리 생성 (내부 사용)
    bool ensureDirectories();
};

// 전역 인스턴스 (main.cpp에서 초기화)
extern ConfigManager ConfigMgr;

#endif // ARTHUR_CONFIG_MANAGER_H
