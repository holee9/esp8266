#ifndef ARTHUR_PINS_H
#define ARTHUR_PINS_H

// HW-364 OLED (SSD1306 128x64, I2C)
// 주소: 0x3C
// 상단 16px 노랑 / 하단 48px 파랑 (2색)
#ifndef OLED_SDA
#define OLED_SDA 14  // GPIO14 (D5)
#endif

#ifndef OLED_SCL
#define OLED_SCL 12  // GPIO12 (D6)
#endif

#ifndef OLED_ADDR
#define OLED_ADDR 0x3C
#endif

#define OLED_WIDTH  128
#define OLED_HEIGHT 64

// 2색 OLED 영역 경계
#define OLED_YELLOW_TOP    0
#define OLED_YELLOW_BOTTOM 15  // 상단 16px (노랑)
#define OLED_BLUE_TOP      16
#define OLED_BLUE_BOTTOM   63  // 하단 48px (파랑)

// 외부 센서용 I2C (OLED과 동일 버스 공유)
#define SENSOR_SDA OLED_SDA
#define SENSOR_SCL OLED_SCL
#define BME280_ADDR 0x76  // SDO=GND

// FLASH 버튼 (화면 전환용)
#define BUTTON_PIN 0  // GPIO0 (D3)

// 부저 (선택)
#define BUZZER_PIN 13  // GPIO13 (D7)

// 내장 LED
#define LED_PIN 2  // GPIO2 (D4)

#endif // ARTHUR_PINS_H
