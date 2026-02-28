#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <IotWebConf.h>
#include <IotWebConfUsing.h>
#include "arthur_pins.h"
#include "arthur_config.h"
#include "arthur_littlefs.h"

// 코어 모듈
#include "core/event_bus.h"
#include "core/config_manager.h"
#include "core/cache_manager.h"
#include "core/time_manager.h"

// 기능 모듈
#include "modules/clock_module.h"
#include "modules/sensor_module.h"
#include "modules/weather_module.h"

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

// --- Phase 1 모듈 인스턴스 ---
// 포인터는 각 모듈 cpp에서 정의됨
// extern ClockModule* gClockModulePtr;
// extern SensorModule* gSensorModulePtr;

// WeatherModule은 전역 인스턴스 (weather_module.cpp에서 정의)
// extern WeatherModule gWeatherModule;

// --- Phase 1 모듈 초기화 상태 ---
static bool modulesInitialized = false;

// --- 화면 모드 ---
enum DisplayMode {
    DM_BOOT,      // 부팅 화면
    DM_WIFI,      // WiFi 상태 화면
    DM_CLOCK,     // 시계 화면 (기본)
    DM_SENSOR,    // 센서 데이터 화면
    DM_WEATHER    // 날씨 정보 화면
};
static DisplayMode currentDisplayMode = DM_BOOT;
static unsigned long lastDisplaySwitch = 0;
static const unsigned long DISPLAY_SWITCH_INTERVAL_MS = 5000;  // 5초마다 화면 전환

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

void showWeatherScreen() {
    const WeatherModule::WeatherData* weather = gWeatherModule.getWeatherData();

    display.clearDisplay();

    // 상태바
    char statusBuf[32];
    snprintf(statusBuf, sizeof(statusBuf), "Weather: %s", weather->location);
    drawStatusBar(statusBuf);

    // 하단 영역 (파랑)
    display.fillRect(0, OLED_BLUE_TOP, OLED_WIDTH, OLED_HEIGHT - OLED_BLUE_TOP, SSD1306_BLACK);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // 온도
    display.setCursor(0, 20);
    display.print(F("Temp: "));
    display.print(weather->temperature, 1);
    display.println(F(" C"));

    // 습도
    display.setCursor(0, 34);
    display.print(F("Humid: "));
    display.print((int)weather->humidity);
    display.println(F(" %"));

    // 상태 설명
    display.setCursor(0, 48);
    display.print(weather->description);

    showScreen();
}

// --- IotWebConf 콜백 ---

void wifiConnectedCallback() {
    Serial.print(F("WiFi connected! IP: "));
    Serial.println(WiFi.localIP());
    Serial.print(F("Free heap: "));
    Serial.print(ESP.getFreeHeap());
    Serial.println(F(" bytes"));

    // EventBus에 WiFi 연결 이벤트 발행
    Event event;
    event.type = WIFI_CONNECTED;
    event.timestamp = millis();
    event.data = nullptr;
    gEventBus.publish(event);

    // 즉시 OLED 갱신
    showConnectedScreen();
    lastDisplayedState = iotwebconf::OnLine;
}

void configSavedCallback() {
    Serial.println(F("Config saved. Restarting..."));
    // IotWebConf가 자동으로 재시작 처리
}

void wifiDisconnectedCallback() {
    Serial.println(F("WiFi disconnected"));

    // EventBus에 WiFi 연결 끊김 이벤트 발행
    Event event;
    event.type = WIFI_DISCONNECTED;
    event.timestamp = millis();
    event.data = nullptr;
    gEventBus.publish(event);
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

    // 상태 페이지 (모듈 정보 포함)
    char timeBuf[32];
    gTimeManager.getFormattedDateTime(timeBuf, sizeof(timeBuf));

    const WeatherModule::WeatherData* weather = gWeatherModule.getWeatherData();

    char buf[512];
    snprintf(buf, sizeof(buf),
        "<!DOCTYPE html><html><body>"
        "<h1>ARTHUR v" ARTHUR_VERSION "</h1>"
        "<h2>System Status</h2>"
        "<p>Heap: %u B</p>"
        "<p>Uptime: %lu sec</p>"
        "<h2>Time</h2>"
        "<p>%s</p>"
        "<h2>Weather</h2>"
        "<p>%s: %.1fC, %d%%</p>"
        "<p>%s</p>"
        "<h2>Sensor</h2>",  // 센서 데이터 추가 필요 시
        ESP.getFreeHeap(),
        millis() / 1000,
        timeBuf,
        weather->location,
        weather->temperature,
        (int)weather->humidity,
        weather->description);

    // 센서 데이터 추가
    size_t len = strlen(buf);
    if (gSensorModulePtr) {
        const SensorData& sensor = gSensorModulePtr->getLastData();
        if (sensor.valid) {
            snprintf(buf + len, sizeof(buf) - len,
                "<p>Temp: %.1fC, Humid: %d%%</p>"
                "<p>Press: %d hPa</p>",
                sensor.temperature,
                (int)sensor.humidity,
                (int)sensor.pressure);
        } else {
            snprintf(buf + len, sizeof(buf) - len,
                "<p>Sensor not available</p>");
        }
    } else {
        snprintf(buf + len, sizeof(buf) - len,
            "<p>Sensor not initialized</p>");
    }

    len = strlen(buf);
    snprintf(buf + len, sizeof(buf) - len,
        "<p><a href='config'>Settings</a></p>"
        "</body></html>");

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

    // Phase 1 모듈 초기화
    Serial.println(F("--- Phase 1 Module Init ---"));

    // EventBus 초기화 (먼저 초기화)
    gEventBus.begin();
    Serial.println(F("[Init] EventBus OK"));

    // ConfigManager 초기화 (LittleFS 마운트)
    if (ConfigMgr.begin()) {
        Serial.println(F("[Init] ConfigManager OK"));
    } else {
        Serial.println(F("[Init] ConfigManager FAILED"));
    }

    // CacheManager 초기화
    if (CacheMgr.begin()) {
        Serial.println(F("[Init] CacheManager OK"));
    } else {
        Serial.println(F("[Init] CacheManager FAILED"));
    }

    // TimeManager 초기화
    gTimeManager.begin();
    Serial.println(F("[Init] TimeManager OK"));

    // ClockModule 초기화 (display 참조 필요)
    static ClockModule sClockModule(display);
    gClockModulePtr = &sClockModule;
    sClockModule.begin();
    Serial.println(F("[Init] ClockModule OK"));

    // SensorModule 초기화
    static SensorModule sSensorModule(display);
    gSensorModulePtr = &sSensorModule;
    if (sSensorModule.begin()) {
        Serial.println(F("[Init] SensorModule OK"));
    } else {
        Serial.println(F("[Init] SensorModule FAILED (BME280 not found)"));
    }

    // WeatherModule 초기화 (전역 인스턴스)
    if (gWeatherModule.begin()) {
        Serial.println(F("[Init] WeatherModule OK"));
    } else {
        Serial.println(F("[Init] WeatherModule FAILED"));
    }

    Serial.print(F("Free heap after modules: "));
    Serial.print(ESP.getFreeHeap());
    Serial.println(F(" bytes"));

    modulesInitialized = true;
    currentDisplayMode = DM_CLOCK;  // 기본 화면: 시계

    Serial.println(F("---"));
    Serial.println(F("First boot? Connect to WiFi 'ARTHUR'"));
    Serial.println(F("Password: arthur123"));
    Serial.println(F("Then open http://192.168.4.1"));
    Serial.println(F("---"));
}

void loop() {
    iotWebConf.doLoop();

    // Phase 1 모듈 업데이트
    if (modulesInitialized) {
        // EventBus 이벤트 처리
        gEventBus.update();

        // TimeManager 업데이트 (NTP 동기화)
        gTimeManager.update();

        // ClockModule 업데이트 (시계 화면)
        if (gClockModulePtr) gClockModulePtr->update();

        // SensorModule 업데이트 (센서 읽기)
        if (gSensorModulePtr) gSensorModulePtr->update();

        // WeatherModule 업데이트 (날씨 API)
        gWeatherModule.update();
    }

    // IotWebConf 상태에 따라 OLED 업데이트
    iotwebconf::NetworkState state = iotWebConf.getState();

    // WiFi가 연결되면 모듈 화면 표시, 아니면 WiFi 상태 화면
    if (state == iotwebconf::OnLine && modulesInitialized) {
        // 화면 자동 전환 (5초마다)
        unsigned long now = millis();
        if (now - lastDisplaySwitch >= DISPLAY_SWITCH_INTERVAL_MS) {
            lastDisplaySwitch = now;

            // 화면 모드 순환: CLOCK -> SENSOR -> WEATHER -> CLOCK ...
            switch (currentDisplayMode) {
                case DM_CLOCK:
                    currentDisplayMode = DM_SENSOR;
                    if (gSensorModulePtr) gSensorModulePtr->setVisible(true);
                    if (gClockModulePtr) gClockModulePtr->hide();
                    break;
                case DM_SENSOR:
                    currentDisplayMode = DM_WEATHER;
                    if (gSensorModulePtr) gSensorModulePtr->setVisible(false);
                    showWeatherScreen();
                    break;
                case DM_WEATHER:
                    currentDisplayMode = DM_CLOCK;
                    if (gClockModulePtr) gClockModulePtr->show();
                    break;
                default:
                    currentDisplayMode = DM_CLOCK;
                    if (gClockModulePtr) gClockModulePtr->show();
                    break;
            }
        }
    } else {
        // WiFi 연결 전에는 WiFi 상태 화면 표시
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
    }

    // OnLine 상태: 30초마다 힙 로깅
    if (state == iotwebconf::OnLine && millis() - lastHeapLog > 30000) {
        lastHeapLog = millis();
        Serial.print(F("Heap: "));
        Serial.print(ESP.getFreeHeap());
        Serial.println(F(" B"));
    }

    delay(1);  // yield
}
