// @MX:NOTE: [MOCK] Wire I2C library mock implementation
// 네이티브 테스트 환경을 위한 전역 Wire 인스턴스 정의

#ifdef ARTHUR_NATIVE_TEST

#include "Wire.h"

// 전역 Wire 인스턴스
TwoWire Wire;

#endif // ARTHUR_NATIVE_TEST
