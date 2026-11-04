#include <Arduino.h>

const int FIRST_PIN = 2;
const int LED_COUNT = 8;

const byte ALL_ON = 0xFF;
const byte ALL_OFF = 0x00;

void led_write(byte value)
{
  value = ~value;

  for (int i = 0; i < LED_COUNT; i++)
  {
    digitalWrite(FIRST_PIN + i, (value >> i) & 1);
  }
}

// 켜진 LED 한 개가 왼쪽 끝까지 갔다가 되돌아온다
// 1 << i 는 1을 왼쪽으로 i칸 민 값이다.
// 1<<0 = 0000 0001, 1<<3 = 0000 1000
void led_shift(int count, int speed)
{
  for (int c = 0; c < count; c++) {
    for (int i = 0; i < LED_COUNT; i++) { // D2 -> D9
      led_write(1 << i);
      delay(speed);
    }

    for (int i = LED_COUNT - 2; i > 0; i--) { // D9 -> D2
      led_write(1 << i);
      delay(speed);
    }
  }
}

void setup() {
  for (int i = 0; i < LED_COUNT; i++) {
    pinMode(FIRST_PIN + i, OUTPUT);
  }
}

void loop() {
  led_shift(5, 100);
}
