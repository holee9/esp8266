# Memory Budget (Memory-Map)

## Overview

ESP8266 has extremely limited memory resources. This document tracks the memory budget allocation for ARTHUR project.

## Total Memory

| Resource | Total | Phase 0 Used | Available |
|----------|-------|--------------|-----------|
| Flash | **4MB** | 365KB (9.5%) | ~3.6MB |
| SRAM | ~50KB | 31.7KB (38.8%) | ~18KB |

### Flash Layout (4MB)

| Region | Size | Purpose |
|--------|------|---------|
| Sketch | 1MB | Application code |
| LittleFS | 3MB | Configuration, logs, assets |
| OTA Reserve | (included in sketch) | Over-the-air updates |

## RAM Budget Breakdown

### Fixed Allocations (Always Resident)

| Component | Allocation | Notes |
|-----------|------------|-------|
| WiFi STA (connected) | 20-25KB | lwIP stack, WiFi buffers |
| IotWebConf | 4-8KB | WebServer, DNSServer, config |
| SSD1306 Framebuffer | 1KB | 128x64 / 8 = 1024 bytes |
| TaskScheduler | ~500B | Task control blocks |

### Conditional Allocations (When Active)

| Component | Allocation | Trigger |
|-----------|------------|---------|
| MQTT Client | 1.5-3KB | When MQTT enabled |
| ArduinoJson | 1KB/block | During JSON parsing |
| TLS Handshake | 15KB | During HTTPS (temporary spike) |
| BME280 Driver | ~300B | When sensor initialized |

### Safety Margin

```cpp
#define HEAP_SAFETY_MARGIN 9216  // 9KB minimum free heap
```

**Rationale**: 9KB buffer for:
- Stack growth during interrupts
- Temporary allocations during WiFi operations
- Emergency heap for error recovery

## Memory Tracking

### Current State (Phase 0)

| Metric | Value | Status |
|--------|-------|--------|
| Free Heap (after boot) | ~45KB | ✅ Good |
| Free Heap (after WiFi connect) | ~45KB | ✅ Good |
| Free Heap (during web request) | ~35KB | ⚠️ Monitor |
| Minimum Free Heap | >10KB | ✅ Within margin |

### Heap Monitoring

```cpp
// Logged every 30 seconds in OnLine state
if (state == iotwebconf::OnLine && millis() - lastHeapLog > 30000) {
    Serial.print(F("Heap: "));
    Serial.print(ESP.getFreeHeap());
    Serial.println(F(" B"));
}
```

## Memory Budget by Phase

| Phase | Expected Heap Usage | Remaining |
|-------|---------------------|-----------|
| Phase 0 | ~25KB | ~25KB (50% free) |
| Phase 1 | ~30KB | ~20KB (40% free) |
| Phase 2 | ~33KB | ~17KB (34% free) |
| Phase 3 | ~36KB | ~14KB (28% free) |
| Phase 4 | ~40KB | ~10KB (20% free) |

## Memory Anti-Patterns (Avoid)

### ❌ String Class

```cpp
// BAD: Causes heap fragmentation
String message = "Hello";
message += " World";
```

### ✅ Static Buffers

```cpp
// GOOD: Fixed allocation
char message[64];
snprintf(message, sizeof(message), "Hello %s", name);
```

### ❌ Dynamic Allocation

```cpp
// BAD: Runtime new/malloc
char* buffer = new char[256];
```

### ✅ Static Allocation

```cpp
// GOOD: Compile-time allocation
static char buffer[256];
```

### ❌ Large JSON in Memory

```cpp
// BAD: Entire JSON in memory
String json = "{\"temperature\":25.5,\"humidity\":60}";
```

### ✅ Stream Processing

```cpp
// GOOD: Stream JSON directly
String json = "[";
for (int i = 0; i < n; i++) {
    if (i > 0) json += ',';
    json += "{\"s\":\"" + WiFi.SSID(i) + "\",\"r\":" + WiFi.RSSI(i) + "}";
}
json += "]";
```

## Memory Optimization Techniques

### 1. F() Macro for String Literals

```cpp
// Stores string in flash, not RAM
Serial.println(F("WiFi connected"));
```

### 2. PROGMEM for Large Data

```cpp
// WiFi scan script stored in flash
static const char SCAN_SCRIPT[] PROGMEM = R"(
function scanWifi(){...}
)";
```

### 3. Reuse Buffers

```cpp
// Shared buffer for temporary operations
static char sharedBuf[128];

void function1() {
    snprintf(sharedBuf, sizeof(sharedBuf), "...");  // message 1
}

void function2() {
    snprintf(sharedBuf, sizeof(sharedBuf), "...");  // message 2
}
```

### 4. Avoid STL Containers

```cpp
// BAD: std::vector, std::string cause heap fragmentation
std::vector<int> items;

// GOOD: Fixed-size arrays
int items[MAX_ITEMS];
int itemCount = 0;
```

## Memory Debugging

### Enable Heap Logging

```cpp
// In platformio.ini debug environment
-DARTHUR_DEBUG=1
-DARTHUR_LOG_LEVEL=3
```

### Runtime Heap Check

```cpp
void checkHeap(const char* label) {
    uint32_t heap = ESP.getFreeHeap();
    Serial.printf("[%s] Heap: %u bytes\n", label, heap);
    if (heap < HEAP_SAFETY_MARGIN) {
        Serial.println("WARNING: Low heap!");
    }
}
```

## Emergency Recovery

If heap drops below safety margin:

1. Log warning to Serial
2. Display warning on OLED
3. Consider graceful degradation (disable non-essential features)
4. Reset as last resort

```cpp
void emergencyReset(const char* reason) {
    Serial.printf("EMERGENCY RESET: %s\n", reason);
    ESP.restart();
}
```

---

Generated: 2026-02-28
Source: architect recommendation + README.md memory budget table
