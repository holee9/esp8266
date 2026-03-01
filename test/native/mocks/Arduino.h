// @MX:NOTE: [MOCK] Arduino core mock for native testing
// PlatformIO native environment에서 Arduino ESP8266 기능 모방

#ifndef ARTHUR_ARDUINO_MOCK_H
#define ARTHUR_ARDUINO_MOCK_H

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cstdlib>

#ifdef ARTHUR_NATIVE_TEST

// Arduino 기본 타입 정의
using byte = uint8_t;
using boolean = bool;

// GPIO 핀 상수
#define HIGH 1
#define LOW 0

#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

// 시간 관련 함수 (모의 시뮬레이션)
extern unsigned long mock_millis_counter;
extern unsigned long mock_micros_counter;

inline unsigned long millis() {
    return mock_millis_counter;
}

inline unsigned long micros() {
    return mock_micros_counter;
}

inline void delay(unsigned long ms) {
    mock_millis_counter += ms;
}

inline void delayMicroseconds(unsigned int us) {
    mock_micros_counter += us;
}

// 테스트 헬퍼: 시간 조작
inline void mock_advance_millis(unsigned long ms) {
    mock_millis_counter += ms;
}

inline void mock_reset_millis() {
    mock_millis_counter = 0;
    mock_micros_counter = 0;
}

// Serial 클래스 모의
class HardwareSerial {
public:
    void begin(unsigned long baud) {
        // 네이티브 환경에서는 baud rate 무시
    }

    void end() {
        // 리소스 정리 (필요시)
    }

    // 출력 메서드 (stdout으로 출력)
    size_t print(const char* str) {
        printf("%s", str);
        return str ? strlen(str) : 0;
    }

    size_t print(char c) {
        putchar(c);
        return 1;
    }

    size_t print(unsigned char c, int base = 10) {
        char buf[34];
        snprintf(buf, sizeof(buf), (base == 16) ? "%02x" : "%u", c);
        return print(buf);
    }

    size_t print(int num, int base = 10) {
        char buf[34];
        if (base == 16) snprintf(buf, sizeof(buf), "%x", num);
        else if (base == 2) {
            // Binary conversion
            int n = num, i = 0;
            if (n == 0) { buf[0] = '0'; buf[1] = '\0'; }
            else {
                char tmp[34];
                while (n > 0) { tmp[i++] = (n & 1) + '0'; n >>= 1; }
                for (int j = 0; j < i; j++) buf[j] = tmp[i-1-j];
                buf[i] = '\0';
            }
        }
        else snprintf(buf, sizeof(buf), "%d", num);
        return print(buf);
    }

    size_t print(unsigned int num, int base = 10) {
        char buf[34];
        if (base == 16) snprintf(buf, sizeof(buf), "%x", num);
        else snprintf(buf, sizeof(buf), "%u", num);
        return print(buf);
    }

    size_t print(long num, int base = 10) {
        char buf[34];
        if (base == 16) snprintf(buf, sizeof(buf), "%lx", num);
        else snprintf(buf, sizeof(buf), "%ld", num);
        return print(buf);
    }

    size_t print(unsigned long num, int base = 10) {
        char buf[34];
        if (base == 16) snprintf(buf, sizeof(buf), "%lx", num);
        else snprintf(buf, sizeof(buf), "%lu", num);
        return print(buf);
    }

    size_t print(double num, int digits = 2) {
        char buf[50];
        snprintf(buf, sizeof(buf), "%.*f", digits, num);
        return print(buf);
    }

    size_t println(const char* str) {
        size_t n = print(str);
        println();
        return n + 1;
    }

    size_t println(char c) {
        size_t n = print(c);
        println();
        return n + 1;
    }

    size_t println(int num, int base = 10) {
        size_t n = print(num, base);
        println();
        return n + 1;
    }

    size_t println(unsigned int num, int base = 10) {
        size_t n = print(num, base);
        println();
        return n + 1;
    }

    size_t println(long num, int base = 10) {
        size_t n = print(num, base);
        println();
        return n + 1;
    }

    size_t println(unsigned long num, int base = 10) {
        size_t n = print(num, base);
        println();
        return n + 1;
    }

    size_t println(double num, int digits = 2) {
        size_t n = print(num, digits);
        println();
        return n + 1;
    }

    size_t println() {
        print("\n");
        return 1;
    }

    size_t printf(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        size_t n = vprintf(fmt, args);
        va_end(args);
        return n;
    }

    // 입력 메서드 (테스트용 stub)
    int available() {
        return 0;
    }

    int read() {
        return -1;
    }

    int peek() {
        return -1;
    }

    void flush() {
        fflush(stdout);
    }
};

// 전역 Serial 인스턴스
extern HardwareSerial Serial;

// Arduino String 클래스 모의 (간단한 구현)
class String {
public:
    String() : _buffer(nullptr), _length(0), _capacity(0) {
    }

    String(const char* str) {
        if (str) {
            _length = strlen(str);
            _capacity = _length + 1;
            _buffer = new char[_capacity];
            strcpy(_buffer, str);
        } else {
            _buffer = nullptr;
            _length = 0;
            _capacity = 0;
        }
    }

    String(const String& other) {
        if (other._buffer) {
            _length = other._length;
            _capacity = other._capacity;
            _buffer = new char[_capacity];
            strcpy(_buffer, other._buffer);
        } else {
            _buffer = nullptr;
            _length = 0;
            _capacity = 0;
        }
    }

    ~String() {
        delete[] _buffer;
    }

    String& operator=(const String& other) {
        if (this != &other) {
            delete[] _buffer;
            if (other._buffer) {
                _length = other._length;
                _capacity = other._capacity;
                _buffer = new char[_capacity];
                strcpy(_buffer, other._buffer);
            } else {
                _buffer = nullptr;
                _length = 0;
                _capacity = 0;
            }
        }
        return *this;
    }

    const char* c_str() const {
        return _buffer ? _buffer : "";
    }

    int length() const {
        return _length;
    }

    bool operator==(const String& other) const {
        if (!_buffer || !other._buffer) {
            return !_buffer && !other._buffer;
        }
        return strcmp(_buffer, other._buffer) == 0;
    }

    bool operator==(const char* str) const {
        if (!_buffer) {
            return !str || str[0] == '\0';
        }
        return strcmp(_buffer, str) == 0;
    }

    String& operator+=(const char* str) {
        if (!str) return *this;
        int newLen = _length + strlen(str);
        if (newLen + 1 > _capacity) {
            int newCap = (newLen + 1) * 2;
            char* newBuf = new char[newCap];
            if (_buffer) {
                strcpy(newBuf, _buffer);
                delete[] _buffer;
            } else {
                newBuf[0] = '\0';
            }
            _buffer = newBuf;
            _capacity = newCap;
        }
        strcat(_buffer, str);
        _length = newLen;
        return *this;
    }

private:
    char* _buffer;
    int _length;
    int _capacity;
};

// Flash 문자열 매크로 (네이티브에서는 무시)
#define F(string_literal) (string_literal)
#define PSTR(string_literal) (string_literal)

// PROGMEM 매크로 (너이티브에서는 무시)
#define PROGMEM
#define FPSTR(string_literal) (string_literal)

// 수학 함수
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define abs(x) ((x) > 0 ? (x) : -(x))
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#define round(x) ((x) >= 0 ? (long)((x) + 0.5) : (long)((x) - 0.5))
#define radians(deg) ((deg) * DEG_TO_RAD)
#define degrees(rad) ((rad) * RAD_TO_DEG)
#define sq(x) ((x) * (x))

#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define EULER 2.7182818284590452353602874713527

// EEPROM 매크로 (너이티브에서는 stub)
#define EEPROM_SIZE 4096

#endif // ARTHUR_NATIVE_TEST
#endif // ARTHUR_ARDUINO_MOCK_H
