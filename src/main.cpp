#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>
#include <LittleFS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFiManager.h>  // WiFiManager - 더 안정적인 WiFi 설정 라이브러리
#include "arthur_pins.h"
#include "arthur_config.h"

// --- OLED 디스플레이 (1KB 프레임버퍼) ---
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// --- WiFiManager ---
WiFiManager wifiManager;

// OLED 화면 갱신 추적
static int lastDisplayedState = -1;
static unsigned long lastHeapLog = 0;

// --- OLED 헬퍼 ---

void drawStatusBar(const char* text) {
    display.fillRect(0, 0, OLED_WIDTH, OLED_YELLOW_BOTTOM + 1, SSD1306_BLACK);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 4);
    display.print(text);
}

void drawContent(const char* line1, const char* line2, const char* line3) {
    display.fillRect(0, OLED_BLUE_TOP, OLED_WIDTH, OLED_HEIGHT - OLED_BLUE_TOP, SSD1306_BLACK);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    if (line1) { display.setCursor(0, 20); display.print(line1); }
    if (line2) { display.setCursor(0, 34); display.print(line2); }
    if (line3) { display.setCursor(0, 48); display.print(line3); }
}

void showScreen() {
    display.display();
}

// --- OLED 상태별 화면 ---

void showBootScreen() {
    display.clearDisplay();
    drawStatusBar("ARTHUR v" ARTHUR_VERSION);

    display.setTextSize(2);
    display.setCursor(16, 24);
    display.print(F("ARTHUR"));

    display.setTextSize(1);
    display.setCursor(0, 48);
    display.print(F("AttoClaw ESP8266"));
    showScreen();
}

void showApModeScreen() {
    display.clearDisplay();
    drawStatusBar("Setup Mode");
    drawContent(
        "Connect to WiFi:",
        "ARTHUR",
        "Open 192.168.4.1"
    );
    showScreen();
}

void showConnectingScreen() {
    display.clearDisplay();
    drawStatusBar("Connecting...");
    drawContent("WiFi connecting", "Please wait...", nullptr);
    showScreen();
}

void showConnectedScreen() {
    char ipBuf[SMALL_BUF];
    WiFi.localIP().toString().toCharArray(ipBuf, sizeof(ipBuf));

    char ssidBuf[MEDIUM_BUF];
    snprintf(ssidBuf, sizeof(ssidBuf), "SSID: %s", WiFi.SSID().c_str());

    char heapBuf[SMALL_BUF];
    snprintf(heapBuf, sizeof(heapBuf), "Heap: %u B", ESP.getFreeHeap());

    display.clearDisplay();
    drawStatusBar("WiFi OK");
    drawContent(ssidBuf, ipBuf, heapBuf);
    showScreen();
}

// --- 메인 ---

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println(F("=== ARTHUR v" ARTHUR_VERSION " ==="));
    Serial.println(F("AttoClaw - ESP8266 Personal Assistant"));

    Serial.print(F("Free heap: "));
    Serial.print(ESP.getFreeHeap());
    Serial.println(F(" bytes"));

    // I2C + OLED
    Wire.begin(OLED_SDA, OLED_SCL);

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println(F("SSD1306 init FAILED"));
        Serial.println(F("Try swapping SDA/SCL: Wire.begin(12, 14)"));
        while (1) { delay(1000); }
    }
    Serial.println(F("OLED OK"));
    showBootScreen();

    // WiFiManager 설정
    wifiManager.setDebugOutput(true);  // 디버그 출력 활성화
    wifiManager.setMinimumSignalQuality(10);  // 신호 품질 최소 10%

    Serial.println(F(""));
    Serial.println(F("--- WiFiManager Starting ---"));
    Serial.println(F("Connect to AP: ARTHUR"));
    Serial.println(F("Password: arthur123"));
    Serial.println(F("Then open http://192.168.4.1"));
    Serial.println(F(""));

    showApModeScreen();
}

void loop() {
    // WiFiManager autoConnect 실행 (비차단 상태)
    static bool firstRun = true;
    static unsigned long lastStateCheck = 0;

    if (firstRun) {
        firstRun = false;
        // autoConnect 실행
        wifiManager.autoConnect("ARTHUR", "arthur123");
    }

    // WiFi 상태 확인 (1초마다)
    if (millis() - lastStateCheck > 1000) {
        lastStateCheck = millis();

        if (WiFi.status() == WL_CONNECTED) {
            // WiFi 연결됨
            if (lastDisplayedState != 1) {
                lastDisplayedState = 1;
                Serial.println(F(""));
                Serial.println(F("=== WiFi Connected! ==="));
                Serial.print(F("IP: "));
                Serial.println(WiFi.localIP());
                Serial.print(F("SSID: "));
                Serial.println(WiFi.SSID());
                Serial.print(F("RSSI: "));
                Serial.print(WiFi.RSSI());
                Serial.println(F(" dBm"));
                Serial.print(F("Free heap: "));
                Serial.print(ESP.getFreeHeap());
                Serial.println(F(" bytes"));
                showConnectedScreen();
            }

            // 30초마다 힙 로깅
            if (millis() - lastHeapLog > 30000) {
                lastHeapLog = millis();
                Serial.print(F("[Heap] "));
                Serial.print(ESP.getFreeHeap());
                Serial.println(F(" bytes"));
            }
        } else if (WiFi.status() == WL_DISCONNECTED || WiFi.status() == WL_IDLE_STATUS) {
            // WiFi 연결 끊김
            if (lastDisplayedState != 0) {
                lastDisplayedState = 0;
                Serial.println(F(""));
                Serial.println(F("=== WiFi Disconnected ==="));
                Serial.println(F("Restarting WiFiManager..."));
                showApModeScreen();
                // WiFiManager 재시작
                wifiManager.startConfigPortal();
            }
        }
    } else if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL) {
        // WiFi 연결 실패
        if (lastDisplayedState != 2) {
            lastDisplayedState = 2;
            Serial.println(F(""));
            Serial.println(F("=== WiFi Connection Failed ==="));
            Serial.println(F("Starting AP mode..."));
            showApModeScreen();
            // AP 모드 시작
            wifiManager.startConfigPortal();
        }
    }

    delay(1);  // yield
}
