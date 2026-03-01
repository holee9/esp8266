#!/bin/bash
# @MX:NOTE: [HELPER] 네이티브 테스트 설정 및 실행 헬퍼 스크립트
# PlatformIO가 설치되지 않은 환경에서도 테스트 인프라 검증용

set -e

# 색상 정의
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 프로젝트 루트 디렉토리
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$PROJECT_ROOT"

echo -e "${GREEN}=== ARTHUR Native Test Infrastructure Setup ===${NC}\n"

# 1. 디렉토리 구조 확인
echo -e "${YELLOW}[1/5] Checking directory structure...${NC}"
required_dirs=(
    "test/native/mocks"
    "src/core"
    "include"
)

for dir in "${required_dirs[@]}"; do
    if [ -d "$dir" ]; then
        echo -e "  ${GREEN}✓${NC} $dir exists"
    else
        echo -e "  ${RED}✗${NC} $dir missing"
        exit 1
    fi
done

# 2. Mock 헤더 파일 확인
echo -e "\n${YELLOW}[2/5] Checking mock header files...${NC}"
mock_headers=(
    "test/native/mocks/Arduino.h"
    "test/native/mocks/Wire.h"
    "test/native/mocks/WiFi.h"
    "test/native/mocks/HTTPClient.h"
    "test/native/mocks/Adafruit_SSD1306.h"
    "test/native/mocks/Adafruit_BME280.h"
)

for header in "${mock_headers[@]}"; do
    if [ -f "$header" ]; then
        echo -e "  ${GREEN}✓${NC} $header"
    else
        echo -e "  ${RED}✗${NC} $header missing"
        exit 1
    fi
done

# 3. Mock 구현 파일 확인
echo -e "\n${YELLOW}[3/5] Checking mock implementation files...${NC}"
mock_impls=(
    "test/native/mocks/Arduino.cpp"
    "test/native/mocks/Wire.cpp"
    "test/native/mocks/WiFi.cpp"
)

for impl in "${mock_impls[@]}"; do
    if [ -f "$impl" ]; then
        echo -e "  ${GREEN}✓${NC} $impl"
    else
        echo -e "  ${RED}✗${NC} $impl missing"
        exit 1
    fi
done

# 4. 테스트 파일 확인
echo -e "\n${YELLOW}[4/5] Checking test files...${NC}"
test_files=(
    "test/native/test_event_bus.cpp"
)

for test_file in "${test_files[@]}"; do
    if [ -f "$test_file" ]; then
        echo -e "  ${GREEN}✓${NC} $test_file"
    else
        echo -e "  ${RED}✗${NC} $test_file missing"
        exit 1
    fi
done

# 5. PlatformIO 확인
echo -e "\n${YELLOW}[5/5] Checking PlatformIO installation...${NC}"
if command -v pio &> /dev/null; then
    echo -e "  ${GREEN}✓${NC} PlatformIO (pio) found"
    pio --version
    echo -e "\n  ${GREEN}Run tests with: ${NC}pio test -e native_test"
elif command -v platformio &> /dev/null; then
    echo -e "  ${GREEN}✓${NC} PlatformIO found"
    platformio --version
    echo -e "\n  ${GREEN}Run tests with: ${NC}platformio test -e native_test"
else
    echo -e "  ${YELLOW}⚠${NC} PlatformIO not found"
    echo -e "  ${YELLOW}Install with: ${NC}pip install platformio"
    echo -e "  ${YELLOW}Or use: ${NC}pip install -U platformio"
    echo -e "\n  ${YELLOW}Alternative: ${NC}Install PlatformIO IDE extension"
fi

# 요약
echo -e "\n${GREEN}=== Setup Check Complete ===${NC}"
echo -e "\n${GREEN}All required files are present!${NC}"
echo -e "\n${YELLOW}Next steps:${NC}"
echo -e "  1. ${GREEN}pio test -e native_test${NC} - Run all native tests"
echo -e "  2. ${GREEN}pio test -e native_test -f test_event_bus${NC} - Run specific test"
echo -e "  3. ${GREEN}pio test -e native_test -v${NC} - Verbose output"
echo -e "\n${YELLOW}Documentation:${NC} test/native/README.md"
