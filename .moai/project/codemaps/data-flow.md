# Data Flow

## WiFi Configuration Flow

```
┌─────────────────────────────────────────────────────────────────────┐
│                      First Boot / Reset                              │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│  OLED: "Setup Mode"                                                  │
│  AP: "ARTHUR" (password: arthur123)                                 │
│  IP: 192.168.4.1                                                     │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                    ┌───────────────┴───────────────┐
                    │    User connects smartphone   │
                    │    to ARTHUR AP               │
                    └───────────────┬───────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│  Captive Portal → http://192.168.4.1/config                         │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │  [Scan WiFi] → GET /scan → WiFi.scanNetworks()             │   │
│  │       ↓                                                      │   │
│  │  [{"s":"SSID","r":-45},...] → Dropdown populated            │   │
│  │       ↓                                                      │   │
│  │  User selects SSID + enters password                         │   │
│  │       ↓                                                      │   │
│  │  [Apply] → POST /config → IotWebConf saves to EEPROM        │   │
│  └─────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│  Restart → Connecting → OnLine                                       │
│  OLED: "WiFi OK" + SSID + IP + Heap                                 │
└─────────────────────────────────────────────────────────────────────┘
```

## Main Loop Data Flow

```
┌─────────────────┐
│     loop()      │
└────────┬────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────────────┐
│  iotWebConf.doLoop()                                                 │
│  ├── WiFi state machine update                                       │
│  ├── Web server request handling                                     │
│  └── DNS server (AP mode)                                           │
└─────────────────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────────────┐
│  State Change Detection                                              │
│  if (state != lastDisplayedState)                                    │
│  ├── ApMode → showApModeScreen()                                    │
│  ├── Connecting → showConnectingScreen()                            │
│  ├── OnLine → showConnectedScreen()                                 │
│  └── OffLine → show reconnecting message                            │
└─────────────────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────────────┐
│  Periodic Tasks (OnLine only)                                        │
│  if (millis() - lastHeapLog > 30000)                                │
│  ├── Serial.print(ESP.getFreeHeap())                                │
│  └── showConnectedScreen() (refresh IP/Heap display)               │
└─────────────────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────────────┐
│  delay(1) → yield to WiFi stack                                      │
└─────────────────────────────────────────────────────────────────────┘
```

## HTTP Request Flow

```
┌──────────┐     GET /scan      ┌─────────────────┐
│  Client  │ ────────────────── │   WebServer     │
│(Browser) │                    │                 │
└──────────┘                    └────────┬────────┘
                                         │
                                         ▼
                                ┌─────────────────┐
                                │  handleScan()   │
                                │                 │
                                │  WiFi.scanNetworks()
                                │       ↓         │
                                │  Build JSON     │
                                │       ↓         │
                                │  WiFi.scanDelete()
                                └────────┬────────┘
                                         │
                                         ▼
┌──────────┐   application/json   ┌─────────────────┐
│  Client  │ ◄────────────────── │  webServer.send │
└──────────┘   [{"s":...,"r":...}]│                 │
```

## OLED Display Flow

```
┌─────────────────────────────────────────────────────────────────────┐
│                        Display Update                                │
└─────────────────────────────────────────────────────────────────────┘
                                │
                ┌───────────────┴───────────────┐
                │                               │
                ▼                               ▼
    ┌─────────────────────┐         ┌─────────────────────┐
    │   drawStatusBar()   │         │    drawContent()    │
    │   (Yellow: 0-15px)  │         │   (Blue: 16-63px)   │
    │                     │         │                     │
    │ display.fillRect()  │         │ display.fillRect()  │
    │ display.setTextSize(1)│       │ display.setTextSize(1)│
    │ display.setCursor() │         │ display.setCursor() │
    │ display.print()     │         │ display.print()     │
    └──────────┬──────────┘         └──────────┬──────────┘
               │                               │
               └───────────────┬───────────────┘
                               │
                               ▼
                    ┌─────────────────────┐
                    │   display.display() │
                    │   (I2C transfer)    │
                    └─────────────────────┘
```

## Future Data Flows (Planned)

### EventBus Flow (Phase 1)

```
┌──────────────┐     publish("sensor/reading")     ┌──────────────┐
│ SensorModule │ ─────────────────────────────────► │  EventBus    │
└──────────────┘                                    └──────┬───────┘
                                                          │
                        ┌─────────────────────────────────┤
                        │                                 │
                        ▼                                 ▼
            ┌──────────────────┐             ┌──────────────────┐
            │   ClockScreen    │             │  MqttModule      │
            │ (subscribe)      │             │ (subscribe)      │
            └──────────────────┘             └──────────────────┘
```

### MQTT Communication Flow (Phase 2)

```
┌──────────────┐     publish     ┌──────────────┐     publish     ┌──────────────┐
│ ARTHUR       │ ──────────────► │ MQTT Broker  │ ──────────────► │ Home         │
│ (MqttModule) │                 │              │                 │ Assistant    │
└──────────────┘ ◄────────────── └──────────────┘ ◄────────────── └──────────────┘
                   subscribe                   subscribe
```

### Weather API Flow (Phase 1)

```
┌──────────────┐     GET /weather     ┌──────────────┐
│ ARTHUR       │ ───────────────────► │ OpenWeather  │
│ (WeatherModule)│                     │ Map API      │
└──────────────┘ ◄────────────────── └──────────────┘
                   JSON response
                   {temp, humidity, ...}
```

---

Generated: 2026-02-28
