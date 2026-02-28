# Entry Points

## Application Entry Points

### Primary Entry Point

**Function**: `setup()` / `loop()`
**File**: `src/main.cpp`
**Description**: Standard Arduino entry points for initialization and main loop

### Setup Sequence

```cpp
void setup() {
    // 1. Serial initialization
    Serial.begin(115200);

    // 2. I2C initialization (non-standard pins)
    Wire.begin(OLED_SDA, OLED_SCL);  // GPIO14, GPIO12

    // 3. OLED initialization
    display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

    // 4. IotWebConf configuration
    iotWebConf.setHtmlFormatProvider(&arthurHtmlFormatProvider);
    iotWebConf.setConfigPin(BUTTON_PIN);
    iotWebConf.setStatusPin(LED_PIN, LOW);
    iotWebConf.setWifiConnectionCallback(wifiConnectedCallback);
    iotWebConf.setConfigSavedCallback(configSavedCallback);
    iotWebConf.init();

    // 5. Web handler registration
    webServer.on("/", handleRoot);
    webServer.on("/config", [] { iotWebConf.handleConfig(); });
    webServer.on("/scan", handleScan);
    webServer.onNotFound([] { iotWebConf.handleNotFound(); });
}
```

### Loop Execution

```cpp
void loop() {
    // 1. IotWebConf state machine (non-blocking)
    iotWebConf.doLoop();

    // 2. State change detection
    iotwebconf::NetworkState state = iotWebConf.getState();
    if (state != lastDisplayedState) {
        // Update OLED based on state
    }

    // 3. Periodic heap logging (30s interval)
    if (state == iotwebconf::OnLine && millis() - lastHeapLog > 30000) {
        // Log heap + refresh display
    }

    delay(1);  // yield to WiFi stack
}
```

## Web API Endpoints

| Endpoint | Method | Handler | Description |
|----------|--------|---------|-------------|
| `/` | GET | `handleRoot()` | Status page with heap info |
| `/config` | GET/POST | `iotWebConf.handleConfig()` | WiFi configuration page |
| `/scan` | GET | `handleScan()` | WiFi scan JSON API |

### WiFi Scan API

**Endpoint**: `GET /scan`

**Response**:
```json
[
    {"s": "NetworkName", "r": -45},
    {"s": "AnotherNet", "r": -72}
]
```

**Implementation**:
```cpp
void handleScan() {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n && i < 15; i++) {
        if (i > 0) json += ',';
        json += "{\"s\":\"" + WiFi.SSID(i) + "\",\"r\":" + WiFi.RSSI(i) + "}";
    }
    json += ']';
    WiFi.scanDelete();
    webServer.send(200, "application/json", json);
}
```

## Callbacks

### WiFi Connected Callback

**Trigger**: WiFi connection established
**Function**: `wifiConnectedCallback()`

```cpp
void wifiConnectedCallback() {
    Serial.println(WiFi.localIP());
    showConnectedScreen();
    lastDisplayedState = iotwebconf::OnLine;
}
```

### Config Saved Callback

**Trigger**: Configuration saved to EEPROM
**Function**: `configSavedCallback()`

```cpp
void configSavedCallback() {
    Serial.println("Config saved. Restarting...");
    // IotWebConf handles restart automatically
}
```

## State Machine Entry Points

| State | Entry Condition | Display Function |
|-------|-----------------|------------------|
| Boot | Power on / reset | `showBootScreen()` |
| ApMode | No config / FLASH button held | `showApModeScreen()` |
| Connecting | WiFi credentials saved | `showConnectingScreen()` |
| OnLine | WiFi connected | `showConnectedScreen()` |
| OffLine | WiFi lost | Show reconnecting message |

## Hardware Interrupts

| Pin | Interrupt | Action |
|-----|-----------|--------|
| GPIO0 (BUTTON_PIN) | Falling edge | Force AP mode on boot |
| GPIO2 (LED_PIN) | Status | IotWebConf status blink |

## Future Entry Points (Planned)

### EventBus (Phase 1)

```cpp
// Subscribe to events
EventBus.subscribe("sensor/reading", onSensorReading);
EventBus.subscribe("weather/update", onWeatherUpdate);

// Publish events
EventBus.publish("sensor/reading", sensorData);
```

### TaskScheduler Tasks (Phase 1)

```cpp
// Scheduled tasks
Task sensorTask(5000, TASK_FOREVER, &readSensor);
Task weatherTask(600000, TASK_FOREVER, &fetchWeather);
Task ntpTask(3600000, TASK_FOREVER, &syncNTP);
```

---

Generated: 2026-02-28
