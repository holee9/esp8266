#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <IotWebConf.h>
#include <IotWebConfUsing.h>
#include "arthur_pins.h"
#include "arthur_config.h"

// --- OLED 디스플레이 (1KB 프레임버퍼) ---
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// --- IotWebConf ---
// AP 초기 패스워드: 최초 부팅 시 "ARTHUR-Setup" AP에 이 패스워드로 접속
static const char AP_DEFAULT_PASSWORD[] = "arthur123";

DNSServer dnsServer;
WebServer webServer(80);
IotWebConf iotWebConf("ARTHUR", &dnsServer, &webServer, AP_DEFAULT_PASSWORD, "v1");

// WiFi 스캔용 HTML/JS (PROGMEM으로 Flash에 저장)
// 설정 페이지에 "Scan WiFi" 버튼 + 드롭다운 삽입
static const char SCAN_SCRIPT[] PROGMEM = R"(
function scanWifi(){
var b=document.getElementById('sb');
b.textContent='Scanning...';b.disabled=true;
fetch('/scan').then(r=>r.json()).then(d=>{
var s=document.getElementById('sl');
s.innerHTML='<option value="">-- Select --</option>';
d.forEach(function(n){
var o=document.createElement('option');
o.value=n.s;o.textContent=n.s+' ('+n.r+'dBm)';
s.appendChild(o);});
s.style.display='block';b.textContent='Scan WiFi';b.disabled=false;
}).catch(function(){b.textContent='Scan WiFi';b.disabled=false;});
}
window.addEventListener('load',function(){
var f=document.querySelector('input[name="iwcWifiSsid"]');
if(!f)return;
var p=f.parentNode;
var btn=document.createElement('button');
btn.type='button';btn.id='sb';btn.textContent='Scan WiFi';
btn.style.cssText='margin:5px 0;width:100%;';
btn.onclick=scanWifi;
var sel=document.createElement('select');
sel.id='sl';sel.style.cssText='display:none;margin:5px 0;width:100%;';
sel.onchange=function(){if(this.value)f.value=this.value;};
p.insertBefore(btn,f.nextSibling);
p.insertBefore(sel,btn.nextSibling);
});
)";

// 커스텀 HTML 포맷: 스캔 JS 삽입
class ArthurHtmlFormatProvider : public iotwebconf::HtmlFormatProvider {
protected:
    String getScriptInner() override {
        return HtmlFormatProvider::getScriptInner() + FPSTR(SCAN_SCRIPT);
    }
};

static ArthurHtmlFormatProvider arthurHtmlFormatProvider;

// OLED 화면 갱신 추적
static iotwebconf::NetworkState lastDisplayedState = iotwebconf::Boot;
static unsigned long lastHeapLog = 0;

// --- OLED 헬퍼 ---

void drawStatusBar(const char* text) {
    // 상단 노랑 영역 (0~15)
    display.fillRect(0, 0, OLED_WIDTH, OLED_YELLOW_BOTTOM + 1, SSD1306_BLACK);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 4);
    display.print(text);
}

void drawContent(const char* line1, const char* line2, const char* line3) {
    // 하단 파랑 영역 (16~63)
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

// --- IotWebConf 콜백 ---

void wifiConnectedCallback() {
    Serial.print(F("WiFi connected! IP: "));
    Serial.println(WiFi.localIP());
    Serial.print(F("Free heap: "));
    Serial.print(ESP.getFreeHeap());
    Serial.println(F(" bytes"));
    // 즉시 OLED 갱신
    showConnectedScreen();
    lastDisplayedState = iotwebconf::OnLine;
}

void configSavedCallback() {
    Serial.println(F("Config saved. Restarting..."));
    // IotWebConf가 자동으로 재시작 처리
}

// --- 웹 핸들러 ---

void handleScan() {
    Serial.println(F("WiFi scan requested"));
    int n = WiFi.scanNetworks();

    // JSON 배열 직접 생성 (ArduinoJson 없이 — 메모리 절약)
    String json = "[";
    for (int i = 0; i < n && i < 15; i++) {
        if (i > 0) json += ',';
        json += "{\"s\":\"";
        // SSID에 " 문자 이스케이프
        String ssid = WiFi.SSID(i);
        ssid.replace("\"", "\\\"");
        json += ssid;
        json += "\",\"r\":";
        json += WiFi.RSSI(i);
        json += '}';
    }
    json += ']';

    WiFi.scanDelete();

    webServer.send(200, "application/json", json);
    Serial.printf("Scan done: %d networks\n", n);
}

void handleRoot() {
    // Captive Portal 리다이렉트 처리
    if (iotWebConf.handleCaptivePortal()) return;

    // 간단한 상태 페이지
    char buf[256];
    snprintf(buf, sizeof(buf),
        "<!DOCTYPE html><html><body>"
        "<h1>ARTHUR</h1>"
        "<p>v" ARTHUR_VERSION "</p>"
        "<p>Heap: %u B</p>"
        "<p><a href='config'>Settings</a></p>"
        "</body></html>",
        ESP.getFreeHeap());

    webServer.send(200, "text/html", buf);
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
        while (true) { delay(1000); }
    }
    Serial.println(F("OLED OK"));
    showBootScreen();

    // IotWebConf 설정
    // WiFi 스캔 드롭다운 JS 삽입
    iotWebConf.setHtmlFormatProvider(&arthurHtmlFormatProvider);

    // FLASH 버튼(GPIO0)을 설정 리셋 핀으로 사용
    // 부팅 시 FLASH 버튼을 누르고 있으면 → AP 모드 강제 진입 (설정 초기화)
    iotWebConf.setConfigPin(BUTTON_PIN);

    // 내장 LED를 상태 표시로 사용
    iotWebConf.setStatusPin(LED_PIN, LOW);

    // 콜백 등록
    iotWebConf.setWifiConnectionCallback(wifiConnectedCallback);
    iotWebConf.setConfigSavedCallback(configSavedCallback);

    // IotWebConf 초기화 (EEPROM에서 설정 로드)
    bool hasConfig = iotWebConf.init();

    Serial.print(F("Has saved config: "));
    Serial.println(hasConfig ? F("YES") : F("NO"));

    // 웹 핸들러 등록
    webServer.on("/", handleRoot);
    webServer.on("/config", [] { iotWebConf.handleConfig(); });
    webServer.on("/scan", handleScan);
    webServer.onNotFound([] { iotWebConf.handleNotFound(); });

    Serial.print(F("Free heap after init: "));
    Serial.print(ESP.getFreeHeap());
    Serial.println(F(" bytes"));

    Serial.println(F("---"));
    Serial.println(F("First boot? Connect to WiFi 'ARTHUR'"));
    Serial.println(F("Password: arthur123"));
    Serial.println(F("Then open http://192.168.4.1"));
    Serial.println(F("---"));
}

void loop() {
    iotWebConf.doLoop();

    // IotWebConf 상태에 따라 OLED 업데이트
    iotwebconf::NetworkState state = iotWebConf.getState();

    // 상태가 바뀔 때만 화면 갱신
    if (state != lastDisplayedState) {
        lastDisplayedState = state;

        switch (state) {
            case iotwebconf::ApMode:
            case iotwebconf::NotConfigured:
                showApModeScreen();
                break;

            case iotwebconf::Connecting:
                showConnectingScreen();
                break;

            case iotwebconf::OnLine:
                showConnectedScreen();
                break;

            case iotwebconf::OffLine:
                display.clearDisplay();
                drawStatusBar("WiFi Lost");
                drawContent("Reconnecting...", nullptr, nullptr);
                showScreen();
                break;

            default:
                break;
        }
    }

    // OnLine 상태: 30초마다 힙 로깅 + OLED 갱신
    if (state == iotwebconf::OnLine && millis() - lastHeapLog > 30000) {
        lastHeapLog = millis();
        Serial.print(F("Heap: "));
        Serial.print(ESP.getFreeHeap());
        Serial.println(F(" B"));
        showConnectedScreen();
    }

    delay(1);  // yield
}
