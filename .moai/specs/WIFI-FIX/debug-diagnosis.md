# WiFi Connection Failure - Debug Diagnosis

## Problem Summary
Device enters AP mode correctly, user can connect to configuration portal, but WiFi never connects after saving credentials.

## Root Cause Analysis

### Critical Bug #1: Missing `wm.process()` in loop()
**Location**: `/home/drake/workspace/esp8266/src/main.cpp` line 623-627

**Current Code**:
```cpp
void loop() {
    // 웹 서버 처리 (WiFi 연결 시에만)
    if (currentWifiState == WS_CONNECTED) {
        webServer.handleClient();
    }
    // ... rest of loop
}
```

**Problem**: WiFiManager의 `wm.process()`가 호출되지 않아, AP 모드에서 사용자 입력이 처리되지 않습니다. WiFiManager는 자체 웹 서버를 실행하며, `process()`를 통해 DNS 캡처와 요청 처리를 수행합니다.

**Impact**: 사용자가 자격 증명을 저장해도 WiFiManager가 이를 처리하지 못해 연결이 불가능합니다.

---

### Critical Bug #2: Web Server Port Conflict
**Location**: `/home/drake/workspace/esp8266/src/main.cpp` line 28, 570-571

**Current Code**:
```cpp
// Line 28
ESP8266WebServer webServer(80);

// Lines 570-571
if (currentWifiState == WS_CONNECTED) {
    webServer.begin();
}
```

**Problem**: ESP8266WebServer가 포트 80을 사용하지만, WiFiManager도 포트 80을 사용합니다. setup()에서 `wm.autoConnect()`를 실행하면 WiFiManager가 이미 포트 80을 바인딩합니다. 이후 custom webServer를 시작하려고 시도하면 포트 충돌이 발생합니다.

**Impact**: AP 모드에서 WiFiManager 포털과 custom 웹 서버가 충돌하여 요청 처리가 실패할 수 있습니다.

---

### Critical Bug #3: WiFiManager Save Handler Not Called
**Location**: `/home/drake/workspace/esp8266/src/main.cpp` line 524-567

**Current Code**: Custom `/save-wifi` 핸들러에서 직접 `WiFi.begin()` 호출:
```cpp
webServer.on("/save-wifi", HTTP_POST, []() {
    // ...
    WiFi.begin(ssid.c_str(), pass.c_str());  // Line 542
    // ...
});
```

**Problem**: WiFiManager의 자격 증명 저장 메커니즘을 우회하고 있습니다. WiFiManager는 `wm.setSaveConfigCallback()`를 통해 자격 증명을 EEPROM에 저장해야 합니다. 현재 코드는 WiFiManager 내부 상태를 업데이트하지 않아 재부팅 시 설정이 손실됩니다.

**Impact**: 자격 증명이 WiFiManager의 내부 저장소에 저장되지 않아, 재부팅 후 연결이 유지되지 않습니다.

---

### Critical Bug #4: WebServer Started Too Late for AP Mode
**Location**: `/home/drake/workspace/esp8266/src/main.cpp` line 569-575

**Current Code**:
```cpp
// 웹서버는 WiFi 연결 후에만 시작 (AP 모드에서는 WiFiManager가 처리)
if (currentWifiState == WS_CONNECTED) {
    webServer.begin();
    Serial.println(F("[Web] Server started"));
} else {
    Serial.println(F("[Web] WiFi not connected, using WiFiManager portal"));
}
```

**Problem**: AP 모드에서 custom 웹 핸들러(`/wifimanager`, `/save-wifi`)가 등록되었지만, webServer가 시작되지 않습니다. 즉, 사용자가 정의한 HTML 폼이 접근 불가능합니다.

**Impact**: 사용자가 custom 설정 페이지(`/wifimanager`)에 접근할 수 없습니다.

---

### Critical Bug #5: Custom Parameter Not Saved Correctly
**Location**: `/home/drake/workspace/esp8266/src/main.cpp` line 39-40, 544-552

**Current Code**: Custom 파라미터가 WiFiManager에 추가되었지만, 저장 콜백이 없습니다:
```cpp
WiFiManagerParameter paramApiKey("apiKey", "OpenWeatherMap API Key", weatherApiKey, sizeof(weatherApiKey));
WiFiManagerParameter paramLocation("location", "Location (city,CC)", weatherLocation, sizeof(weatherLocation), "placeholder='Seoul,KR'");
```

**Problem**: WiFiManager에 파라미터를 추가했지만 `setSaveConfigCallback()`을 설정하지 않아, 사용자 입력값이 `weatherApiKey`와 `weatherLocation` 변수로 복사되지 않습니다.

**Impact**: 날씨 API 설정이 WiFiManager 포털에서 저장되지 않습니다.

---

## Recommended Fix Approach

### Solution 1: Add `wm.process()` to loop()
```cpp
void loop() {
    // WiFiManager 처리 (항상 - AP 모드와 STA 모드 모두)
    wm.process();

    // 웹 서버 처리 (WiFi 연결 시에만)
    if (currentWifiState == WS_CONNECTED) {
        webServer.handleClient();
    }
    // ... rest of loop
}
```

### Solution 2: Use WiFiManager's Built-in Portal
WiFiManager가 자체적으로 포털을 제공하므로, custom `/wifimanager` 핸들러를 제거하고 WiFiManager의 내장 포털을 사용합니다.

### Solution 3: Add Save Config Callback
```cpp
// setup()에 추가
wm.setSaveConfigCallback([]() {
    Serial.println(F("[WiFi] Config saved, reading custom parameters..."));
    // WiFiManagerParameter에서 값 읽기
    strcpy(weatherApiKey, paramApiKey.getValue());
    strcpy(weatherLocation, paramLocation.getValue());

    // LittleFS에 저장
    if (LittleFS.begin()) {
        File f = LittleFS.open("/weather.cfg", "w");
        if (f) {
            f.println(weatherApiKey);
            f.println(weatherLocation);
            f.close();
        }
    }
});
```

### Solution 4: Remove Custom WebServer for WiFi Configuration
WiFiManager가 포트 80에서 이미 실행 중이므로, custom 웹 서버를 다른 포트(예: 8080)로 이동하거나 완전히 제거합니다.

### Solution 5: Start WebServer Only in STA Mode
```cpp
// 웹 서버 시작 (포트 8080로 변경)
if (currentWifiState == WS_CONNECTED) {
    webServer.begin();
    Serial.println(F("[Web] Server started on port 8080"));
}
```

---

## Summary of Fixes Required

| Bug | Fix | Priority |
|-----|-----|----------|
| Missing `wm.process()` | Add to loop() | CRITICAL |
| Port conflict | Remove custom webServer or change port | CRITICAL |
| No save callback | Add `setSaveConfigCallback()` | HIGH |
| Custom parameter not saved | Implement callback handler | HIGH |
| WebServer not started in AP | Remove custom handlers from AP mode | MEDIUM |

---

## Expected Behavior After Fix

1. Device boots → WiFiManager autoConnect attempts
2. No saved credentials → Enters AP mode (192.168.4.1)
3. User connects to "ARTHUR" AP
4. User opens 192.168.4.1 → WiFiManager portal appears
5. User enters SSID/password → WiFiManager processes via `wm.process()`
6. WiFi connects → `onWifiConnected()` callback fires
7. Custom webServer starts on port 8080 for additional features
