#include <Arduino.h>

const int FIRST_PIN = 2;  // 첫 LED 핀 (D2 ~ D9)
const int LED_COUNT = 8;

const int FIRST_SW = 10;  // 첫 스위치 핀 (D10 ~ D12)
const int SW_COUNT = 3;

const byte ALL_ON = 0xFF;
const byte ALL_OFF = 0x00;

// 비트를 LED 핀으로 뿌린다. bit0 = D2
// 회로가 Active Low 라서 뒤집어 내보낸다
void led_write(byte value) {
    value = ~value;

    for (int i = 0; i < LED_COUNT; i++) {
        digitalWrite(FIRST_PIN + i, (value >> i) & 1);
    }
}

// 스위치 핀을 읽어 비트로 모은다. bit0 = D10
// 풀업이라 눌리면 Low 다
//
// 1 << i   : i 번째 자리에 1 을 세운다
// value |  : 그 자리를 켜서 모은다
byte sw_read() {
    byte value = 0;

    for (int i = 0; i < SW_COUNT; i++) {
        if (digitalRead(FIRST_SW + i) == LOW) {
            value = value | (1 << i);
        }
    }

    return value;
}

byte prev_sw = 0;  // 직전에 읽은 스위치 상태

// 막 눌린 순간에만 1 을 돌려준다 (누르고 있는 동안은 0)
//
// now      : 지금 눌려 있는 것
// ~prev_sw : 직전에 안 눌려 있던 것
// 둘 다 참인 비트 = 이번에 새로 눌린 것
byte sw_pressed() {
    byte now = sw_read();
    byte edge = now & ~prev_sw;

    prev_sw = now;
    return edge;
}

void setup() {
    for (int i = 0; i < LED_COUNT; i++) {
        pinMode(FIRST_PIN + i, OUTPUT);
    }

    for (int i = 0; i < SW_COUNT; i++) {
        pinMode(FIRST_SW + i, INPUT_PULLUP);
    }

    led_write(ALL_OFF);
}

int pos = 0;  // 켜질 LED 자리

// 실습1) 스위치 1 을 누를 때마다 LED 가 한 칸 이동
void loop() {
    if (sw_pressed() & 0x01) {
        led_write(1 << pos);

        if (pos <= LED_COUNT - 1) {
            pos = pos + 1;
        }
    }

    delay(10);
}
