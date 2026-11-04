#include <Arduino.h>

const int FIRST_PIN = 2;
const int LED_COUNT = 8;

const byte ALL_ON = 0xFF;
const byte ALL_OFF = 0x00;

void led_write(byte value)
{
  value = ~value;

  for (int i = 0; i < LED_COUNT; i++) {
    digitalWrite(FIRST_PIN + i, (value >> i) & 1);
  }
}

// 전체 켜고 끄기를 count 번 반복, period 는 ms 단위
void led_all_onoff(int count, int period)
{
  for (int i = 0; i < count; i++) {
    led_write(ALL_ON);
    delay(period);

    led_write(ALL_OFF);
    delay(period);
  }
}

void setup() {
  for (int i = 0; i < LED_COUNT; i++)
  {
    pinMode(FIRST_PIN + i, OUTPUT);
  }

  led_all_onoff(5, 500);
}

void loop() {}
