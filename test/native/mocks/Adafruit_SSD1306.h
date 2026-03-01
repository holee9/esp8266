// @MX:NOTE: [MOCK] Adafruit SSD1306 OLED display library mock for native testing
// PlatformIO native environment에서 OLED 디스플레이 기능 모방

#ifndef ARTHUR_ADAFRUIT_SSD1306_MOCK_H
#define ARTHUR_ADAFRUIT_SSD1306_MOCK_H

#include <cstdint>
#include <cstring>

#ifdef ARTHUR_NATIVE_TEST

// OLED 색상 상수
#define SSD1306_BLACK 0
#define SSD1306_WHITE 1
#define SSD1306_INVERSE 2

// OLED 전압 선택 상수
#define SSD1306_SWITCHCAPVCC 1
#define SSD1306_EXTERNALVCC 2

// FlashStringHelper 타입 (Arduino 호환)
class __FlashStringHelper;
#define F(string_literal) (reinterpret_cast<const __FlashStringHelper *>(PSTR(string_literal)))
#define PSTR(string_literal) (string_literal)

// 해상도 정의
#define SSD1306_128_64 0
#define SSD1306_128_32 1
#define SSD1306_96_16 2

// Adafruit_GFX 베이스 클래스 (최소한)
class Adafruit_GFX {
public:
    Adafruit_GFX(int16_t w, int16_t h) : _width(w), _height(h) {}
    virtual ~Adafruit_GFX() = default;

    int16_t width() const { return _width; }
    int16_t height() const { return _height; }

    void drawPixel(int16_t x, int16_t y, uint16_t color) {
        // 테스트용 stub
    }

    virtual void fillScreen(uint16_t color) {
        // 테스트용 stub
    }

protected:
    int16_t _width;
    int16_t _height;
};

// Adafruit_SSD1306 클래스 모의
class Adafruit_SSD1306 : public Adafruit_GFX {
public:
    Adafruit_SSD1306(int16_t w = 128, int16_t h = 64)
        : Adafruit_GFX(w, h), _initialized(false), _cursorX(0), _cursorY(0),
          _textSize(1), _textColor(SSD1306_WHITE), _textWrap(true) {
        memset(_buffer, 0, sizeof(_buffer));
    }

    ~Adafruit_SSD1306() = default;

    // 디스플레이 초기화
    bool begin(uint8_t vccstate = SSD1306_SWITCHCAPVCC, uint8_t i2caddr = 0x3C, bool reset = true, bool periphBegin = true) {
        _initialized = true;
        return true;
    }

    // 디스플레이 버퍼 및 화면 지우기
    void clearDisplay() {
        memset(_buffer, 0, sizeof(_buffer));
    }

    // 버퍼 내용을 실제 디스플레이에 전송 (테스트에서는 no-op)
    void display() {
        // 네이티브 환경에서는 no-op
    }

    // 커서 위치 설정
    void setCursor(int16_t x, int16_t y) {
        _cursorX = x;
        _cursorY = y;
    }

    // 텍스트 색상 설정
    void setTextColor(uint16_t c) {
        _textColor = c;
    }

    void setTextColor(uint16_t c, uint16_t bg) {
        _textColor = c;
        // 배경색은 테스트에서 무시
    }

    // 텍스트 크기 설정 (확대 배율)
    void setTextSize(uint8_t s) {
        _textSize = s;
    }

    // 텍스트 랩 설정
    void setTextWrap(bool w) {
        _textWrap = w;
    }

    // 텍스트 그리기
    size_t write(uint8_t c) {
        // 테스트에서는 개수만 반환
        return 1;
    }

    size_t write(const uint8_t* buffer, size_t size) {
        // 테스트에서는 size만 반환
        return size;
    }

    // 문자열 출력 (printf 스타일)
    size_t printf(const char* format, ...) {
        // 테스트에서는 형식 문자열 길이만 반환
        return strlen(format);
    }

    // 직사각형 채우기
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        // 테스트용 stub
    }

    // 선 그리기
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
        // 테스트용 stub
    }

    // 직사각형 외곽선 그리기
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        // 테스트용 stub
    }

    // 원 그리기
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
        // 테스트용 stub
    }

    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
        // 테스트용 stub
    }

    // 텍스트 경계 계산
    void getTextBounds(const char* str, int16_t x, int16_t y,
                       int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
        if (x1) *x1 = x;
        if (y1) *y1 = y;
        if (w) *w = strlen(str) * 6 * _textSize; // 대략적인 계산
        if (h) *h = 8 * _textSize;
    }

    void getTextBounds(const __FlashStringHelper* str, int16_t x, int16_t y,
                       int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
        getTextBounds((const char*)str, x, y, x1, y1, w, h);
    }

    // 화면 크기 설정 (다양한 해상도 지원)
    void setBufferSize(uint8_t w, uint8_t h) {
        _width = w;
        _height = h;
    }

    // 디스플레이 끄기/켜기
    void dim(bool dim) {
        // 테스트용 stub
    }

    // 테스트 헬퍼: 초기화 상태 확인
    bool mock_is_initialized() const {
        return _initialized;
    }

    // 테스트 헬퍼: 커서 위치 확인
    int16_t mock_get_cursor_x() const { return _cursorX; }
    int16_t mock_get_cursor_y() const { return _cursorY; }

    // 테스트 헬퍼: 텍스트 설정 확인
    uint8_t mock_get_text_size() const { return _textSize; }
    uint16_t mock_get_text_color() const { return _textColor; }

private:
    bool _initialized;
    int16_t _cursorX;
    int16_t _cursorY;
    uint8_t _textSize;
    uint16_t _textColor;
    bool _textWrap;

    // 디스플레이 버퍼 (128x64 monochrome = 1024 bytes)
    static const int BUFFER_SIZE = 1024;
    uint8_t _buffer[BUFFER_SIZE];
};

#endif // ARTHUR_NATIVE_TEST
#endif // ARTHUR_ADAFRUIT_SSD1306_MOCK_H
