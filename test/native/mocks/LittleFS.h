// @MX:NOTE: [MOCK] LittleFS.h mock for native testing
// ESP8266 LittleFS 파일시스템 모의

#ifndef ARTHUR_LITTLEFS_MOCK_H
#define ARTHUR_LITTLEFS_MOCK_H

#include "FS.h"

#ifdef ARTHUR_NATIVE_TEST

// LittleFS는 FS.h에서 정의된 전역 인스턴스를 사용
// extern FS LittleFS;

#endif // ARTHUR_NATIVE_TEST
#endif // ARTHUR_LITTLEFS_MOCK_H
