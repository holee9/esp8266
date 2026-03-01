# SPEC-CODE-QUALITY: ESP8266 ARTHUR 코드품질 고도화

**SPEC ID**: CODE-QUALITY
**작성일**: 2026-03-01
**우선순위**: P0 (긴급)
**개발 모드**: TDD (Test-Driven Development)

---

## 1. 개요

ESP8266 ARTHUR 프로젝트의 현재 코드품질 상태를 TRUST 5 프레임워크 기반으로 고도화합니다. 모듈형 아키텍처 통합, 테스트 커버리지 향상, 메모리 안전성 강화를 통해 프로덕션 레디 코드베이스를 구축합니다.

### 현재 상태

| 항목 | 현재 값 | 목표 값 | 갭 |
|------|---------|---------|-----|
| TRUST 5 점수 | 15/25 (60%) | 23/25 (92%) | +8 |
| 테스트 커버리지 | 15% | 85% | +70% |
| Free Heap | 45KB | 30KB+ (안정 마진) | -15KB |
| String 위반 | 1건 | 0건 | -1 |
| 모듈 통합 | 0% | 100% | +100% |

---

## 2. EARS 요구사항

### 2.1 WHIERE (시스템이 제공해야 하는 기능)

**REQ-CQ-001**: 시스템은 모듈형 아키텍처를 main.cpp에 통합해야 한다.
- EventBus, TimeManager, ConfigManager, CacheManager를 초기화해야 한다
- ClockModule, SensorModule, WeatherModule을 인스턴스화해야 한다
- 모든 모듈의 update() 메서드를 루프에서 호출해야 한다

**REQ-CQ-002**: 시스템은 ESP8266 메모리 제약을 준수해야 한다.
- 가용 힙을 30KB 이상 유지해야 한다 (안정 마진)
- String 클래스 사용을 완전히 제거해야 한다
- 정적 할당만 사용해야 한다 (new/malloc 금지)

**REQ-CQ-003**: 시스템은 85% 이상의 테스트 커버리지를 달성해야 한다.
- 모든 코어 모듈에 단위 테스트가 있어야 한다
- 모든 공개 API에 테스트 케이스가 있어야 한다
- 통합 테스트가 모듈 간 통신을 검증해야 한다

**REQ-CQ-004**: 시스템은 TDD 방법론을 준수해야 한다.
- RED-GREEN-REFACTOR 사이클을 따라야 한다
- 테스트가 실패한 상태(RED)에서 구현을 시작해야 한다
- 구현 후 리팩토링을 통해 코드 품질을 개선해야 한다

### 2.2 WHEN (이벤트 발생 조건)

**REQ-CQ-005**: WiFi 연결 시 시스템은 WIFI_CONNECTED 이벤트를 발행해야 한다.
- EventBus를 통해 모든 구독자에게 알려야 한다
- 연결 성공 후 TimeManager NTP 동기화를 시작해야 한다

**REQ-CQ-006**: WiFi 연결 해제 시 시스템은 WIFI_DISCONNECTED 이벤트를 발행해야 한다.
- 모든 모듈이 연결 손실을 처리할 수 있어야 한다
- 재연결 시도를 해야 한다

**REQ-CQ-007**: NTP 동기화 완료 시 TimeManager는 TIME_SYNCED 이벤트를 발행해야 한다.
- ClockModule이 시간 표시를 갱신할 수 있어야 한다
- 로컬 캐시에 타임스탬프를 저장해야 한다

### 2.3 IF (조건부 동작)

**REQ-CQ-008**: 메모리 부족 경고 시 시스템은 선택적 모듈 비활성화를 수행해야 한다.
- 가용 힙이 20KB 미만일 경우 WeatherModule 일시 중지
- 가용 힙이 15KB 미만일 경우 SensorModule 일시 중지
- 최소 10KB 이상의 가용 힙을 유지해야 한다

**REQ-CQ-009**: 모듈 초기화 실패 시 시스템은 에러 로그를 출력하고 계속 실행해야 한다.
- 실패한 모듈을 건너뛰고 다른 모듈을 초기화해야 한다
- Serial에 에러 메시지를 출력해야 한다

### 2.4 WHILE (지속적 동작)

**REQ-CQ-010**: 루프 동안 시스템은 정해진 순서로 모듈을 업데이트해야 한다.
1. TimeManager.update()
2. EventBus.update()
3. ClockModule.update()
4. SensorModule.update()
5. WeatherModule.update()

**REQ-CQ-011**: 루프 동안 시스템은 30초마다 힙 메모리를 로깅해야 한다.
- "[Heap] XXXXX bytes" 형식으로 출력해야 한다
- 메모리 누수 조기 발견을 위해 로깅해야 한다

---

## 3. 인수 기준 (Acceptance Criteria)

### AC-CQ-001: 모듈 통합 완료
- [ ] main.cpp에 EventBus, TimeManager, ConfigManager, CacheManager 초기화 코드 추가
- [ ] ClockModule, SensorModule, WeatherModule 인스턴스화
- [ ] 모든 update() 메서드가 loop()에서 호출됨
- [ ] WiFi 연결/해제 이벤트 발행 기능 구현

### AC-CQ-002: String 클래스 제거
- [ ] weather_module.cpp:112의 String 사용을 char[] 버퍼로 변경
- [ ] 모든 테스트 통과
- [ ] 메모리 사용량 감소 확인

### AC-CQ-003: 테스트 커버리지 85%
- [ ] TimeManager 단위 테스트 작성 (최소 5개 케이스)
- [ ] CacheManager 단위 테스트 작성 (최소 5개 케이스)
- [ ] ConfigManager 단위 테스트 작성 (최소 3개 케이스)
- [ ] SensorModule 단위 테스트 작성 (최소 5개 케이스)
- [ ] WeatherModule 단위 테스트 작성 (최소 5개 케이스)
- [ ] ClockModule 단위 테스트 작성 (최소 3개 케이스)
- [ ] 통합 테스트 작성 (최소 3개 케이스)
- [ ] 전체 커버리지 85% 이상 달성

### AC-CQ-004: 메모리 안정성
- [ ] 가용 힙이 30KB 이상 유지됨
- [ ] 메모리 누수 없음 (24시간 연속 동작 테스트)
- [ ] 정적 할당만 사용됨 (new/malloc 0건)

### AC-CQ-005: TDD 준수
- [ ] 모든 구현이 RED-GREEN-REFACTOR 사이클을 따름
- [ ] 테스트가 구현보다 먼저 작성됨
- [ ] 커밋마다 테스트 통과

---

## 4. 기술 설계

### 4.1 아키텍처 통합 전략

```
[main.cpp]
  |
  +-- setup()
  |     |
  |     +-- LittleFS.begin()
  |     +-- gEventBus.begin()
  |     +-- gTimeManager.begin()
  |     +-- ConfigMgr.begin()
  |     +-- CacheMgr.begin()
  |     +-- gClockModule.begin()
  |     +-- gSensorModule.begin()
  |     +-- gWeatherModule.begin()
  |
  +-- loop()
        |
        +-- wifiManager.autoConnect() (비차단)
        +-- WiFi 상태 확인 → 이벤트 발행
        +-- gTimeManager.update()
        +-- gEventBus.update()
        +-- gClockModule.update()
        +-- gSensorModule.update()
        +-- gWeatherModule.update()
        +-- 30초마다 힙 로깅
```

### 4.2 파일 변경 목록

**수정 파일:**
1. `src/main.cpp` - 모듈 통합
2. `src/modules/weather_module.cpp` - String 제거

**생성 파일:**
1. `test/native/test_time_manager_native.cpp`
2. `test/native/test_cache_manager_native.cpp`
3. `test/native/test_config_manager_native.cpp`
4. `test/native/test_sensor_module_native.cpp`
5. `test/native/test_weather_module_native.cpp`
6. `test/native/test_clock_module_native.cpp`
7. `test/native/test_integration_native.cpp`

### 4.3 단계별 구현 계획

#### Phase 1: 핵심 모듈 테스트 (RED) - 3시간
1. TimeManager 테스트 작성 (5 케이스)
2. CacheManager 테스트 작성 (5 케이스)
3. ConfigManager 테스트 작성 (3 케이스)
4. 테스트 실패 확인 (RED 상태)

#### Phase 2: 모듈 통합 (GREEN) - 2시간
1. main.cpp에 EventBus, TimeManager, ConfigManager, CacheManager 통합
2. 테스트 통과 확인
3. 기존 기능 동작 확인

#### Phase 3: 기능 모듈 테스트 (RED) - 3시간
1. SensorModule 테스트 작성 (5 케이스)
2. WeatherModule 테스트 작성 (5 케이스)
3. ClockModule 테스트 작성 (3 케이스)
4. String 제거 테스트 추가
5. 테스트 실패 확인 (RED 상태)

#### Phase 4: 기능 모듈 통합 (GREEN) - 2시간
1. main.cpp에 기능 모듈 통합
2. WiFi 이벤트 발행 구현
3. 테스트 통과 확인

#### Phase 5: 리팩토링 (REFACTOR) - 2시간
1. 코드 정리 및 중복 제거
2. @MX 태그 업데이트
3. 문서화 개선
4. 통합 테스트 작성 (3 케이스)

#### Phase 6: 검증 - 1시간
1. 커버리지 측정 (목표: 85%)
2. 메모리 사용량 확인
3. 실제 장치 테스트

### 4.4 위험 완화

| 위험 | 확률 | 영향 | 완화 전략 |
|------|------|------|-----------|
| 메모리 부족 | 중 | 높 | 지연 초기화, 선택적 모듈 비활성화 |
| 테스트 실패 | 중 | 중 | Mock 고도화, 점진적 구현 |
| 이벤트 순환 참조 | 낮 | 중 | 이벤트 처리 순서 고정 |

---

## 5. 테스트 전략

### 5.1 단위 테스트

**TimeManager (5 케이스):**
1. 초기화 상태 확인
2. NTP 동기화 성공
3. NTP 동기화 실패 (타임아웃)
4. 시간 포맷팅
5. 날짜 포맷팅

**CacheManager (5 케이스):**
1. 초기화 및 파일 시스템 마운트
2. 데이터 저장 및 조회
3. TTL 만료 확인
4. 캐시容量 초과 처리
5. 캐시 삭제

**ConfigManager (3 케이스):**
1. JSON 설정 로드
2. JSON 설정 저장
3. 기본값 처리

**SensorModule (5 케이스):**
1. BME280 초기화
2. 온도 읽기
3. 습도 읽기
4. 기압 읽기
5. SENSOR_UPDATED 이벤트 발행

**WeatherModule (5 케이스):**
1. API 호출 성공
2. API 호출 실패 (네트워크 오류)
3. JSON 파싱
4. 캐싱 동작
5. char[] 버퍼 사용 (String 미사용 확인)

**ClockModule (3 케이스):**
1. 시계 화면 표시
2. TIME_SYNCED 이벤트 처리
3. SENSOR_UPDATED 이벤트 처리

### 5.2 통합 테스트 (3 케이스)

1. **WiFi 연결 시나리오**
   - WiFi 연결 → WIFI_CONNECTED 이벤트
   - TimeManager NTP 동기화
   - ClockModule 시간 표시

2. **센서 데이터 흐름**
   - SensorModule.readSensor()
   - SENSOR_UPDATED 이벤트
   - ClockModule 센서 온도 표시

3. **날씨 데이터 흐름**
   - WeatherModule API 호출
   - WEATHER_UPDATED 이벤트
   - ClockModule 날씨 온도 표시

---

## 6. 성공 지표

### 정량적 지표
- 테스트 커버리지: 85% 이상
- TRUST 5 점수: 23/25 (92%) 이상
- Free Heap: 30KB 이상
- String 위반: 0건
- 테스트 케이스: 29개 이상

### 정성적 지표
- 모든 모듈이 main.cpp에 통합됨
- TDD 사이클이 준수됨
- 코드가 자체 문서화됨 (@MX 태그)
- 실제 장치에서 안정적 동작

---

## 7. 참고 자료

- `.moai/specs/CODE-QUALITY/research.md` - 아키텍처 분석
- `.moai/specs/WIFI-REDESIGN/architecture-analysis.md` - WiFi 설계
- `.moai/config/sections/quality.yaml` - TDD 설정
- `test/test_event_bus_native.cpp` - 레퍼런스 테스트 구현

---

**SPEC 상태**: DRAFT
**승인 상태**: PENDING
**다음 단계**: /moai run CODE-QUALITY
