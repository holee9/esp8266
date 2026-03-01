// @MX:NOTE: [MOCK] WiFi library mock implementation
// 네이티브 테스트 환경을 위한 전역 WiFi 인스턴스 정의

#ifdef ARTHUR_NATIVE_TEST

#include "WiFi.h"

// 전역 WiFi 인스턴스
WiFiClass WiFi;

#endif // ARTHUR_NATIVE_TEST
