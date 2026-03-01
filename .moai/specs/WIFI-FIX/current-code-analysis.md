# WiFi Connection Bug Analysis

## Executive Summary

The ARTHUR ESP8266 project has a critical bug where WiFi credentials can be saved via the configuration portal, but the device never successfully connects to WiFi. The device remains in AP mode or shows "not connected" state indefinitely.

**Project Context**: The README states this project was originally using IotWebConf for WiFi management, but the current `main.cpp` has been converted to use WiFiManager (tzapu/WiFiManager@^2.0.17). The conversion appears incomplete and contains several critical bugs.

---

## Critical Bugs Identified

### 1. Dual Web Server Conflict (CRITICAL)

**Location**: `src/main.cpp:27-28`

```cpp
WiFiManager wm;
ESP8266WebServer webServer(80);
```

**Problem**: Both WiFiManager and the custom `webServer` are trying to use port 80. According to WiFiManager.h:509, WiFiManager has an internal `std::unique_ptr<WM_WebServer> server` that also uses port 80 by default.

**Impact**: This creates a port conflict. When WiFiManager starts its config portal, it creates its own web server on port 80. The custom `webServer` on port 80 interferes with WiFiManager's operations, particularly during the credential save process.

**Evidence**:
- WiFiManager library creates its own ESP8266WebServer internally
- WiFiManager.h:586 shows `_httpPort = 80` by default
- Lines 569-576 only start the custom webServer when WiFi is connected, but the webServer object is still created and may conflict

---

### 2. Incorrect WiFi Connection Method in Custom Save Handler (CRITICAL)

**Location**: `src/main.cpp:524-567` - `/save-wifi` handler

```cpp
void handleSaveWifi() {
    ...
    // WiFiManager에 설정
    WiFi.begin(ssid.c_str(), pass.c_str());  // LINE 542 - BUG!
    ...
}
```

**Problem**: The handler manually calls `WiFi.begin()` instead of using WiFiManager's connection methods. This bypasses WiFiManager's credential storage and connection management.

**Expected Pattern**: WiFiManager provides `setSaveConfigCallback` and `setSaveParamsCallback` for this purpose. The credentials should be saved to WiFiManager's internal parameter storage, then WiFiManager should be asked to initiate the connection.

**Impact**:
- Credentials are not persisted to WiFiManager's EEPROM storage
- WiFiManager doesn't know about the new credentials
- On next reboot, WiFiManager will autoConnect with old (or no) credentials
- The manual `WiFi.begin()` call may conflict with WiFiManager's state machine

---

### 3. Custom Web Form Bypasses WiFiManager's Credential Storage

**Location**: `src/main.cpp:498-522` - `/wifimanager` handler

**Problem**: The code implements a custom HTML form at `/wifimanager` instead of using WiFiManager's built-in configuration portal. This custom form:

1. Collects credentials via HTML form
2. Saves them to LittleFS for weather config (lines 532-539)
3. Manually calls `WiFi.begin()` (line 542)
4. Never calls WiFiManager's credential saving methods

**WiFiManager's Expected Usage**:
```cpp
// WiFiManager manages its own web interface
wm.autoConnect(AP_NAME, AP_PASSWORD);  // This creates the portal
// Or use wm.startConfigPortal() for manual trigger
```

**Current Code Problem**: The custom `/wifimanager` route creates a parallel configuration path that doesn't integrate with WiFiManager's credential storage system.

---

### 4. WiFiManager Parameter Integration Issue

**Location**: `src/main.cpp:39-40, 444-445`

```cpp
WiFiManagerParameter paramApiKey("apiKey", "OpenWeatherMap API Key", weatherApiKey, sizeof(weatherApiKey));
WiFiManagerParameter paramLocation("location", "Location (city,CC)", weatherLocation, sizeof(weatherLocation), "placeholder='Seoul,KR'");

// Later in setup():
wm.addParameter(&paramApiKey);
wm.addParameter(&paramLocation);
```

**Problem**: The parameters are added to WiFiManager, but the custom `/save-wifi` handler doesn't use WiFiManager's parameter save mechanism. Instead, it manually saves to LittleFS (lines 532-539).

**Impact**: The dual storage (WiFiManager params vs LittleFS) creates inconsistency. Weather settings are saved twice in different places, but WiFi credentials are only manually started with `WiFi.begin()`.

---

### 5. Missing WiFiManager Save Callback

**Location**: `src/main.cpp:439-454` - WiFiManager setup

**Problem**: No `setSaveConfigCallback` or `setSaveParamsCallback` is registered. WiFiManager provides these callbacks to notify when credentials are saved through its portal.

**Expected Pattern**:
```cpp
wm.setSaveConfigCallback([]() {
    // Called when WiFiManager saves credentials
    // Update local state, restart modules, etc.
});
```

---

### 6. State Machine Logic Issue

**Location**: `src/main.cpp:216-240` - `updateWifiState()`

```cpp
void updateWifiState() {
    ...
    if (WiFi.getMode() == WIFI_AP) {
        currentWifiState = WS_AP_MODE;
    } else if (WiFi.status() == WL_CONNECTED) {
        currentWifiState = WS_CONNECTED;
    } ...
}
```

**Problem**: The state machine doesn't properly handle the transition from WiFiManager's AP+STA mode during configuration to pure STA mode after connection.

**WiFiManager Behavior**: During config portal operation, WiFiManager operates in `WIFI_AP_STA` mode (both AP and Station active). After successful connection, it should switch to `WIFI_STA` mode only.

**Issue**: The state machine checks `WiFi.getMode() == WIFI_AP` but doesn't account for `WIFI_AP_STA` mode, which is what WiFiManager uses during configuration.

---

## WiFiManager Library Usage Analysis

### Correct WiFiManager Usage Pattern

Based on WiFiManager.h:256-257 and standard usage:

```cpp
// Standard pattern:
WiFiManager wm;
wm.setAPCallback([](WiFiManager* myWiFiManager) {
    // Called when config portal starts
});

// Method 1: Auto-connect (starts portal if no saved creds)
bool res = wm.autoConnect("AP_NAME", "AP_PASSWORD");

// Method 2: Manual config portal trigger
bool res = wm.startConfigPortal("AP_NAME", "AP_PASSWORD");

if (res) {
    // Connected successfully
    // WiFiManager has saved credentials to its storage
} else {
    // Failed to connect or timeout
}
```

### Current Code Deviations

1. **Dual web servers**: Custom ESP8266WebServer conflicts with WiFiManager's internal server
2. **Manual credential handling**: `/save-wifi` bypasses WiFiManager's credential storage
3. **Custom UI instead of WiFiManager portal**: Creates fragmentation in user experience
4. **Missing callbacks**: No integration with WiFiManager's save/complete events

---

## Root Cause Summary

The root cause is **architectural mismatch**: The code was converted from IotWebConf to WiFiManager, but the conversion was incomplete. The code mixes:

1. WiFiManager's built-in credential management and portal
2. Custom web server and HTML forms
3. Manual WiFi.begin() calls that bypass WiFiManager

This creates a situation where:
- User enters credentials in the custom form
- Credentials trigger a manual WiFi.begin() call
- WiFiManager never learns about or stores these credentials properly
- On next reboot, WiFiManager autoConnect fails because credentials weren't saved correctly

---

## Recommended Fix Approach

### Option A: Pure WiFiManager (Recommended)

1. Remove the custom `webServer` object entirely
2. Remove custom `/wifimanager` and `/save-wifi` handlers
3. Let WiFiManager handle all WiFi configuration through its built-in portal
4. Use WiFiManager's `setSaveParamsCallback` to handle weather config
5. Create a separate minimal web server on a different port (e.g., 8080) for application-specific routes

### Option B: Hybrid with Proper Integration

1. Keep custom web server but on different port
2. Use WiFiManager's API to programmatically save credentials:
   - Use `wm.setWiFiAutoReconnect(true)`
   - Store credentials properly before calling WiFi.begin()
3. Register proper callbacks with WiFiManager
4. Ensure state machine handles WIFI_AP_STA mode

---

## Additional Issues

### Memory Concerns

The project already shows significant memory usage (RAM: 38.8%, ~45KB free heap). Running dual web servers simultaneously may cause heap exhaustion.

### ESP8266WiFi Channel Conflict

WiFiManager.h:556 mentions `_channelSync` - ESP8266 cannot be AP and STA on different channels simultaneously. The current AP mode setup may be forcing a channel that doesn't match the target WiFi network.

---

## References

- WiFiManager library: tzapu/WiFiManager@^2.0.17
- Header analysis: `/home/drake/workspace/esp8266/.pio/libdeps/release/WiFiManager/WiFiManager.h`
- Main code: `/home/drake/workspace/esp8266/src/main.cpp`
- Project README shows original intent was IotWebConf
