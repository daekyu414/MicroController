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

void setup() {
  for (int i = 0; i < LED_CNT; i++) {
    pinMode(FIRST_PIN + i, OUTPUT);
  }

  for (int i = 0; i < SW_CNT; i++) {
    pinMode(FIRST_SW + i, INPUT_PULLUP);
  }

  led_write(0x00);
}

int pos_r = 0;
int pos_l = LED_CNT - 1;

void loop() {
  byte pressed = sw_pressed();

  // 1번 버튼 : 0 → 1 → 2 → ... → 7 → 0
  if (pressed & 0x01) {

    led_write(1 << pos_r);

    pos_r++;

    if (pos_r >= LED_CNT) {
      pos_r = 0;
    }
  }

  // 2번 버튼 : 7 → 6 → 5 → ... → 0 → 7
  if (pressed & 0x02) {

    led_write(1 << pos_l);

    pos_l--;

    if (pos_l < 0) {
      pos_l = LED_CNT - 1;
    }
  }

  delay(10);
}
