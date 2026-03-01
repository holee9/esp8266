# WiFi 구현 역사 분석 및 손상 원인 조사

## 개요

본 문서는 ARTHUR ESP8266 프로젝트의 WiFi 구현 역사를 분석하고, 현재 손상된 WiFi 연결 문제의 원인을 파악하기 위한 연구 결과를 정리합니다.

## 커밋 히스토리 분석

### 주요 WiFi 관련 커밋

1. **961fd49** - `feat: implement Phase 0 - OLED display, IotWebConf WiFi setup with scan`
   - WiFi가 처음 구현된 시점
   - IotWebConf 라이브러리 사용
   - AP 모드 및 WiFi 스캔 기능 구현

2. **c04d172** - `feat: update Flash spec to 4MB and add MoAI-ADK setup (#1)`
   - 최신 커밋 (현재 버전)
   - WiFiManager로 라이브러리 변경

## 두 버전 비교

### 작동 버전 (961fd49)

#### 라이브러리
- **IotWebConf** 사용
  - 완전 통합된 WiFi 설정 솔루션
  - Captive Portal 자동 처리
  - 내장된 설정 페이지 제공

#### WiFi 초기화 흐름
```cpp
// 1. IotWebConf 초기화
IotWebConf iotWebConf("ARTHUR", &dnsServer, &webServer, "arthur123", "v1");

// 2. 콜백 등록
iotWebConf.setWifiConnectionCallback(wifiConnectedCallback);

// 3. 설정 핀 지정 (GPIO0)
iotWebConf.setConfigPin(BUTTON_PIN);

// 4. 초기화 실행
bool hasConfig = iotWebConf.init();

// 5. autoConnect 호출 (없음 - init() 자동 처리)
```

#### 특징
- 단순한 초기화 흐름
- EEPROM에 설정 자동 저장/로드
- AP 모드 자동 진입 로직 내장
- WiFi 스캔 기능 제공 (`/scan` 엔드포인트)
- Captive Portal 완벽 지원

### 손상 버전 (현재)

#### 라이브러리
- **WiFiManager** (tzapu/WiFiManager) 사용
  - 분리된 라이브러리
  - 별도의 설정 페이지 필요
  - LittleFS 통합

#### WiFi 초기화 흐름
```cpp
// 1. WiFiManager 인스턴스 생성
WiFiManager wm;

// 2. 콜백 설정
wm.setAPCallback([](WiFiManager* myWiFiManager) {
    // AP 모드 진입 시 콜백
});

// 3. 설정 포털 타임아웃 설정
wm.setConfigPortalTimeout(300);

// 4. autoConnect 호출
bool res = wm.autoConnect(AP_NAME, AP_PASSWORD);
```

#### 변경 사항
1. **라이브러리 변경**: IotWebConf → WiFiManager
2. **파일 시스템 추가**: LittleFS 통합 (설정 저장용)
3. **복잡한 웹 서버 구현**: 커스텀 핸들러 추가
4. **모듈 시스템 도입**: core/modules 구조로 변경
5. **이벤트 버스 시스템 추가**: 복잡한 상태 관리

## 문제점 분석

### 1. WiFiManager의 자동 연동 문제
- `wm.autoConnect()`가 제대로 동작하지 않음
- AP 모드 진입 시 타이밍 문제 발생
- WiFi 상태 추적 로직이 복잡하고 오류 발생

### 2. 설정 저장/로드 오류
- LittleFS 초기화 실패 가능성
- 설정 파일 형식 변경으로 인한 호환성 문제
- EEPROM 대신 LittleFS로 변경되면서 발생한 문제

### 3. 시퀀싱 문제
- 웹 서버 시작 타이밅 문제
- 모듈 초기화 순서 문제
- WiFi 연결 전에 모듈이 초기화되는 경우 발생

### 4. 메모리 문제
- 복잡한 이벤트 버스 및 모듈 시스템으로 인한 메모리 부족
- 힙 사용량이 과도하여 WiFi 연결 불안정

## 원인 요약

### 주요 원인
1. **라이브러리 변경**: 작동하던 IotWebConf에서 WiFiManager로 변경
2. **아키텍처 변경**: 단순한 구조에서 복잡한 모듈 시스템으로 변경
3. **타이밍 문제**: WiFi 연결과 웹 서버 시작의 순서 문제
4. **메모리 압박**: 너무 많은 기능 추가로 인한 리소스 부족

### 솔루션 방향
1. IotWebConf로 롤백 또는 두 라이브러리 비교
2. WiFiManager의 autoConnect 동작 원리 분석
3. LittleFS 통합 문제 해결
4. 모듈 초기화 순서 최적화
5. 메모리 사용량 절약 방안 모색

## 추가 조사 필요 사항

1. `wm.autoConnect()`의 실제 동작 방식 문서화
2. IotWebConf와 WiFiManager의 차이점 상세 비교
3. LittleFS 초기화 실패 시 처리 방안
4. 메모리 프로파일링으로 병목 현상 확인
5. WiFi 연결 상태 추적 로직 개선