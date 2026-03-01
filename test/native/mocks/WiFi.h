// @MX:NOTE: [MOCK] ESP8266 WiFi library mock for native testing
// PlatformIO native environment에서 WiFi 기능 모방

#ifndef ARTHUR_WIFI_MOCK_H
#define ARTHUR_WIFI_MOCK_H

#include <cstdint>
#include <cstring>

#ifdef ARTHUR_NATIVE_TEST

// WiFi 상태 코드
enum wl_status_t {
    WL_NO_SHIELD = 255,
    WL_IDLE_STATUS = 0,
    WL_NO_SSID_AVAIL = 1,
    WL_SCAN_COMPLETED = 2,
    WL_CONNECTED = 3,
    WL_CONNECT_FAILED = 4,
    WL_CONNECTION_LOST = 5,
    WL_DISCONNECTED = 6
};

// WiFi 네트워크 정보 구조체
struct WiFiNetworkInfo {
    char ssid[33];
    int8_t rssi;
    uint8_t encryption;
};

// WiFi 클래스 모의
class WiFiClass {
public:
    WiFiClass() {
        _status = WL_DISCONNECTED;
        _localIP = 0;
        _subnetMask = 0;
        _gatewayIP = 0;
        _scanCount = 0;
        _scanResults = nullptr;
        memset(_ssid, 0, sizeof(_ssid));
    }

    ~WiFiClass() {
        mock_clear_scan_results();
    }

    // WiFi 연결 시작
    int begin(const char* ssid, const char* passphrase = nullptr) {
        strncpy(_ssid, ssid, sizeof(_ssid) - 1);
        if (passphrase) {
            strncpy(_passphrase, passphrase, sizeof(_passphrase) - 1);
        }
        // 테스트에서는 즉시 연결 성공으로 설정
        _status = WL_CONNECTED;
        _localIP = 0xC0A80101; // 192.168.1.1
        _subnetMask = 0xFFFFFF00; // 255.255.255.0
        _gatewayIP = 0xC0A80101; // 192.168.1.1
        return WL_CONNECTED;
    }

    int begin(const char* ssid, uint8_t key_idx, const char* key) {
        return begin(ssid, key);
    }

    // WiFi 연결 해제
    int disconnect() {
        _status = WL_DISCONNECTED;
        _localIP = 0;
        memset(_ssid, 0, sizeof(_ssid));
        return WL_DISCONNECTED;
    }

    // 현재 상태 반환
    wl_status_t status() {
        return _status;
    }

    // MAC 주소 설정/반환
    uint8_t* macAddress(uint8_t* mac) {
        if (mac) {
            // 테스트용 가상 MAC 주소
            mac[0] = 0xAA;
            mac[1] = 0xBB;
            mac[2] = 0xCC;
            mac[3] = 0xDD;
            mac[4] = 0xEE;
            mac[5] = 0xFF;
        }
        return mac;
    }

    // IP 주소 관련 (32비트 정수 표현)
    uint32_t localIP() {
        return _localIP;
    }

    uint32_t subnetMask() {
        return _subnetMask;
    }

    uint32_t gatewayIP() {
        return _gatewayIP;
    }

    // SSID 반환
    char* SSID() {
        return _ssid;
    }

    // RSSI (신호 강도) 반환
    int32_t RSSI() {
        return -50; // 테스트용 고정값 (좋은 신호)
    }

    // WiFi 스캔 시작
    int8_t scanNetworks() {
        mock_clear_scan_results();
        // 테스트용 가상 네트워크 생성
        _scanCount = 2;
        _scanResults = new WiFiNetworkInfo[_scanCount];

        strncpy(_scanResults[0].ssid, "TestNetwork1", sizeof(_scanResults[0].ssid) - 1);
        _scanResults[0].rssi = -50;
        _scanResults[0].encryption = 3;

        strncpy(_scanResults[1].ssid, "TestNetwork2", sizeof(_scanResults[1].ssid) - 1);
        _scanResults[1].rssi = -70;
        _scanResults[1].encryption = 2;

        return _scanCount;
    }

    // 스캔 결과 SSID 반환
    char* scanSSID(int8_t index) {
        if (index < 0 || index >= _scanCount) return nullptr;
        return _scanResults[index].ssid;
    }

    // 스캔 결과 RSSI 반환
    int8_t scanRSSI(int8_t index) {
        if (index < 0 || index >= _scanCount) return 0;
        return _scanResults[index].rssi;
    }

    // 스캔 결과 암호화 타입 반환
    uint8_t scanEncryptionType(int8_t index) {
        if (index < 0 || index >= _scanCount) return 0;
        return _scanResults[index].encryption;
    }

    // 스캔 결과 메모리 해제
    void scanDelete() {
        mock_clear_scan_results();
    }

    // 테스트 헬퍼: 연결 상태 설정
    void mock_set_status(wl_status_t status) {
        _status = status;
    }

    // 테스트 헬퍼: IP 주소 설정
    void mock_set_local_ip(uint32_t ip) {
        _localIP = ip;
    }

    // 테스트 헬퍼: 스캔 결과 설정
    void mock_set_scan_results(WiFiNetworkInfo* results, int8_t count) {
        mock_clear_scan_results();
        _scanCount = count;
        if (count > 0 && results) {
            _scanResults = new WiFiNetworkInfo[count];
            memcpy(_scanResults, results, sizeof(WiFiNetworkInfo) * count);
        }
    }

private:
    wl_status_t _status;
    char _ssid[33];
    char _passphrase[64];
    uint32_t _localIP;
    uint32_t _subnetMask;
    uint32_t _gatewayIP;

    int8_t _scanCount;
    WiFiNetworkInfo* _scanResults;

    void mock_clear_scan_results() {
        if (_scanResults) {
            delete[] _scanResults;
            _scanResults = nullptr;
        }
        _scanCount = 0;
    }
};

// 전역 WiFi 인스턴스
extern WiFiClass WiFi;

// WiFi 편의 함수
inline int WiFiScanComplete() {
    return WiFi.scanComplete();
}

#endif // ARTHUR_NATIVE_TEST
#endif // ARTHUR_WIFI_MOCK_H
