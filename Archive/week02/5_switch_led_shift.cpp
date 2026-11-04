#include <Arduino.h>

const int FIRST_PIN = 2;
const int LED_CNT = 8;

const int FIRST_SW = 10;
const int SW_CNT = 3;

void led_write(byte value) {
  value = ~value;

  for (int i = 0; i < LED_CNT; i++) {
    digitalWrite(FIRST_PIN + i, (value >> i) & 1);
  }
}

byte sw_read() {
  byte value = 0x00;

  for (int i = 0; i < SW_CNT; i++) {
    if (digitalRead(FIRST_SW + i) == LOW) {
      value = value | (1 << i);
    }
  }

  return value;
}

byte prev_sw = 0;

byte sw_pressed() {
  byte now = sw_read();
  byte edge = now & ~prev_sw;

  prev_sw = now;
  return edge;
}

void led_shift(int count, int speed) {
  for (int i = 0; i < count; i++) {
    for (int j = 0; j < LED_CNT; j++) {
      led_write(1 << j);
      delay(speed);
    }
    for (int t = LED_CNT - 2; t > 0; t--) {
      led_write(1 << t);
      delay(speed);
    }
  }
  led_write(0x01);
  delay(speed);
}

void setup() {
  for (int i = 0; i < LED_CNT; i++) {
    pinMode(FIRST_PIN + i, OUTPUT);
  }

  for (int i = 0; i < SW_CNT; i++) {
    pinMode(FIRST_SW + i, INPUT_PULLUP);
  }

  led_write(0x00);
}

void loop() {
  if (sw_pressed() & 0x01) {
    led_shift(2, 80);
    led_write(0x00);
  }
}
