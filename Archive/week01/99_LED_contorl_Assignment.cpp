#include <Arduino.h>

const int FIRST_PIN = 2;
const int LED_COUNT = 8;

const byte ALL_ON = 0xFF;
const byte ALL_OFF = 0x00;

void led_write(byte value) {
  value = ~value;

  for (int i = 0; i < LED_COUNT; i++) {
    digitalWrite(FIRST_PIN + i, (value >> i) & 1);
  }
}

void led_all_on_off(int count, int speed) {
  for (int i = 0; i < count; i++) {
    led_write(ALL_ON);
    delay(speed);
    led_write(ALL_OFF);
    delay(speed);
  }
}

// pattern 과 그 반대 모양을 번갈아 보여준다
// ~pattern 은 0 과 1 을 통째로 뒤집은 값이다
void led_alternating_onoff(byte pattern, int count, int period)
{
  for (int i = 0; i < count; i++)
  {
    led_write(pattern);
    delay(period);

    led_write(~pattern);
    delay(period);
  }
}

void led_alternating(int count, int speed)
{
  led_alternating_onoff(0xAA, count, speed);
  led_alternating_onoff(0xF0, count, speed);
  led_alternating_onoff(0xC3, count, speed);
}

void led_shift(int count, int speed) {
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
  led_all_on_off(3, 500);
  led_alternating(3, 500);
  led_shift(5, 500);
}
