// @MX:NOTE: [MOCK] Arduino mock implementation
// 네이티브 테스트 환경을 위한 전역 인스턴스 정의

#ifdef ARTHUR_NATIVE_TEST

#include "Arduino.h"

// 전역 시뮬레이션 카운터
unsigned long mock_millis_counter = 0;
unsigned long mock_micros_counter = 0;

// 전역 Serial 인스턴스
HardwareSerial Serial;

#endif // ARTHUR_NATIVE_TEST
