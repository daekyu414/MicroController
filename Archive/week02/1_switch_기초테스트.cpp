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
// 1 << i  : i 번째 자리에 1을 세운다
// value | : 그 자리를 켜서 모은다
byte sw_read() {
    byte value = 0;

    for (int i = 0; i < SW_COUNT; i++) {
        if (digitalRead(FIRST_SW + i) == LOW) {
            value = value | (1 << i);
        }
    }

    return value;
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

// 누른 스위치에 해당하는 LED 만 켜진다
// 자료의 PORTD = PINC; 에 해당한다
void loop() {
    led_write(sw_read());
}
