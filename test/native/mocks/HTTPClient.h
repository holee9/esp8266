// @MX:NOTE: [MOCK] HTTPClient library mock for native testing
// PlatformIO native environment에서 HTTP 클라이언트 기능 모방

#ifndef ARTHUR_HTTP_CLIENT_MOCK_H
#define ARTHUR_HTTP_CLIENT_MOCK_H

#include <cstdint>
#include <cstring>
#include <cstdio>

#ifdef ARTHUR_NATIVE_TEST

// HTTP 상태 코드
#define HTTP_CODE_OK 200
#define HTTP_CODE_BAD_REQUEST 400
#define HTTP_CODE_UNAUTHORIZED 401
#define HTTP_CODE_NOT_FOUND 404
#define HTTP_CODE_SERVER_ERROR 500

// HTTPClient 클래스 모의
class HTTPClient {
public:
    HTTPClient() {
        _connected = false;
        _responseCode = 0;
        _responseSize = 0;
        _responseBody = nullptr;
        _userAgent = nullptr;
        memset(_url, 0, sizeof(_url));
    }

    ~HTTPClient() {
        mock_clear_response();
    }

    // HTTP 연결 시작
    bool begin(const char* url) {
        if (!url) return false;
        strncpy(_url, url, sizeof(_url) - 1);
        _connected = true;
        _responseCode = 0;
        return true;
    }

    bool begin(String url) {
        return begin(url.c_str());
    }

    bool begin(const char* host, uint16_t port, const char* uri, bool https = false) {
        char url[256];
        snprintf(url, sizeof(url), "%s://%s:%d%s",
                 https ? "https" : "http", host, port, uri);
        return begin(url);
    }

    bool begin(String host, uint16_t port, String uri, bool https = false) {
        return begin(host.c_str(), port, uri.c_str(), https);
    }

    // HTTP 연결 종료
    void end() {
        _connected = false;
        mock_clear_response();
    }

    // User-Agent 설정
    void setUserAgent(const char* userAgent) {
        _userAgent = userAgent;
    }

    void setUserAgent(String userAgent) {
        _userAgent = userAgent.c_str();
    }

    // Authorization 헤더 설정
    void setAuthorization(const char* user, const char* password) {
        // 테스트에서는 저장만 수행
    }

    void setAuthorization(String user, String password) {
        setAuthorization(user.c_str(), password.c_str());
    }

    void setAuthorization(const char* auth) {
        // 테스트에서는 저장만 수행
    }

    // HTTP 헤더 추가
    void addHeader(const char* name, const char* value, bool first = false, bool replace = true) {
        // 테스트에서는 저장만 수행
    }

    void addHeader(String name, String value, bool first = false, bool replace = true) {
        addHeader(name.c_str(), value.c_str(), first, replace);
    }

    // HTTP 헤더 수집
    void collectHeaders(const char* headerKeys[], size_t headerKeysCount) {
        // 테스트에서는 저장만 수행
    }

    String header(const char* name) {
        return ""; // 테스트용 빈 문자열
    }

    String header(String name) {
        return header(name.c_str());
    }

    // GET 요청
    int GET() {
        if (!_connected) return 0;
        // 테스트용 기본 응답
        _responseCode = HTTP_CODE_OK;
        return _responseCode;
    }

    // POST 요청
    int POST(const uint8_t* data, size_t size) {
        if (!_connected) return 0;
        _responseCode = HTTP_CODE_OK;
        return _responseCode;
    }

    int POST(String data) {
        return POST((const uint8_t*)data.c_str(), data.length());
    }

    int POST(const char* data) {
        return POST((const uint8_t*)data, strlen(data));
    }

    int POST(uint8_t* data, size_t size) {
        return POST((const uint8_t*)data, size);
    }

    // PUT 요청
    int PUT(const uint8_t* data, size_t size) {
        if (!_connected) return 0;
        _responseCode = HTTP_CODE_OK;
        return _responseCode;
    }

    int PUT(String data) {
        return PUT((const uint8_t*)data.c_str(), data.length());
    }

    int PUT(const char* data) {
        return PUT((const uint8_t*)data, strlen(data));
    }

    // PATCH 요청
    int PATCH(const uint8_t* data, size_t size) {
        if (!_connected) return 0;
        _responseCode = HTTP_CODE_OK;
        return _responseCode;
    }

    int PATCH(String data) {
        return PATCH((const uint8_t*)data.c_str(), data.length());
    }

    int PATCH(const char* data) {
        return PATCH((const uint8_t*)data, strlen(data));
    }

    // DELETE 요청
    int DELETE() {
        if (!_connected) return 0;
        _responseCode = HTTP_CODE_OK;
        return _responseCode;
    }

    // 응답 코드 반환
    int getSize() {
        return _responseSize;
    }

    int getResponseCode() {
        return _responseCode;
    }

    // 응답 본문 가져오기
    String getString() {
        if (_responseBody) {
            return String(_responseBody);
        }
        return "";
    }

    String getString() const {
        if (_responseBody) {
            return String(_responseBody);
        }
        return "";
    }

    size_t getStream(uint8_t* buffer, size_t maxLen) {
        if (!_responseBody || maxLen == 0) return 0;
        size_t copyLen = (strlen(_responseBody) < maxLen) ? strlen(_responseBody) : maxLen;
        memcpy(buffer, _responseBody, copyLen);
        return copyLen;
    }

    // 연결 상태 확인
    bool connected() {
        return _connected;
    }

    // 테스트 헬퍼: 응답 설정
    void mock_set_response(int code, const char* body) {
        _responseCode = code;
        mock_clear_response();
        if (body) {
            _responseSize = strlen(body);
            _responseBody = new char[_responseSize + 1];
            strcpy(_responseBody, body);
        }
    }

    void mock_set_response(int code, String body) {
        mock_set_response(code, body.c_str());
    }

    // 테스트 헬퍼: 연결 상태 설정
    void mock_set_connected(bool connected) {
        _connected = connected;
    }

private:
    bool _connected;
    char _url[256];
    const char* _userAgent;
    int _responseCode;
    int _responseSize;
    char* _responseBody;

    void mock_clear_response() {
        if (_responseBody) {
            delete[] _responseBody;
            _responseBody = nullptr;
        }
        _responseSize = 0;
    }
};

#endif // ARTHUR_NATIVE_TEST
#endif // ARTHUR_HTTP_CLIENT_MOCK_H
