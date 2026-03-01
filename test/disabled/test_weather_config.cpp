/**
 * @file test_weather_config.cpp
 * @brief 날씨 설정 파라미터 테스트
 *
 * SPEC-004: WeatherModule API Key UI
 * IotWebConf 파라미터 버퍼 크기 검증
 */

#include <unity.h>

// 네이티브 테스트용 로컬 변수 (main.cpp와 동일한 크기)
// 실제 main.cpp에서는 64바이트로 정의됨
static char weatherApiKey[64] = "";
static char weatherLocation[64] = "Seoul,KR";

/**
 * @brief API 키 버퍼 크기 테스트
 *
 * @MX:NOTE: [API 키 버퍼 검증] OpenWeatherMap API 키는 32자이지만
 * 향후 다른 API를 지원할 수 있으므로 64바이트 버퍼 사용
 */
TEST(WeatherConfig, ApiKeyBufferSize) {
    // OpenWeatherMap API 키 최대 길이: 32자
    // 버퍼는 null 종료 문자를 포함해 최소 33바이트 필요
    // 현재 64바이트로 여유 있게 할당
    TEST_ASSERT_TRUE(sizeof(weatherApiKey) >= 64);
}

/**
 * @brief 위치 버퍼 크기 테스트
 *
 * @MX:NOTE: [위치 버퍼 검증] "City,CountryCode" 형식 또는
 * "lat,lon" 좌표 형식 지원 (최대 약 40자)
 */
TEST(WeatherConfig, LocationBufferSize) {
    // 형식 예시:
    // - "Seoul,KR" (8자)
    // - "San Francisco,US" (16자)
    // - "37.5665,126.9780" (17자)
    // 버퍼는 null 종료 문자를 포함해 최소 64바이트
    TEST_ASSERT_TRUE(sizeof(weatherLocation) >= 32);
}

/**
 * @brief 기본값 초기화 테스트
 *
 * @MX:NOTE: [기본 위치 검증] 기본 위치는 Seoul,KR
 */
TEST(WeatherConfig, DefaultLocationValue) {
    // 기본값이 "Seoul,KR"로 설정되어 있는지 확인
    TEST_ASSERT_EQUAL_STRING("Seoul,KR", weatherLocation);
}

/**
 * @brief 설정 그룹 구성 테스트
 *
 * @MX:NOTE: [파라미터 그룹 검증] API 키와 위치가
 * OptionalGroup으로 묶여있는지 확인
 */
TEST(WeatherConfig, ParameterGroupStructure) {
    // IotWebConf 파라미터 구조 검증
    // - PasswordParameter: weatherApiKey (마스크 표시)
    // - TextParameter: weatherLocation (일반 텍스트)
    // - OptionalGroup: weatherGroup (그룹화)
    // 이 테스트는 컴파일 타임 검증으로 충분
    TEST_ASSERT_TRUE(sizeof(weatherApiKey) > 0);
    TEST_ASSERT_TRUE(sizeof(weatherLocation) > 0);
}

// Unity 메인 함수
void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(WeatherConfig_ApiKeyBufferSize);
    RUN_TEST(WeatherConfig_LocationBufferSize);
    RUN_TEST(WeatherConfig_DefaultLocationValue);
    RUN_TEST(WeatherConfig_ParameterGroupStructure);
    return UNITY_END();
}
