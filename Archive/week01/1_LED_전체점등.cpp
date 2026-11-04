
#include <Arduino.h>

const int FIRST_PIN = 2; // 첫 LED 가 꽂힌 핀 (D2)
const int LED_COUNT = 8; // LED 개수 (D2 ~ D9)

// 8비트 값을 LED 8개에 한꺼번에 보낸다
// 맨 오른쪽 비트가 D2, 맨 왼쪽 비트가 D9
//
// 회로가 Active Low 라서 핀에는 반대로 내보내야 한다.
// 이 함수 안에서만 뒤집으면, 다른 곳에서는 1 = 켜짐 으로 편하게 쓸 수 있다.
//
// ~value       : 0 과 1 을 통째로 뒤집는다
// value >> i   : i 번째 비트를 맨 오른쪽으로 끌어온다
// & 1          : 그 한 칸만 남기고 나머지를 지운다 -> 0 또는 1
void led_write(byte value)
{
    value = ~value;

    for (int i = 0; i < LED_COUNT; i++) {
        digitalWrite(FIRST_PIN + i, (value >> i) & 1);
    }
}

void setup()
{
    for (int i = 0; i < LED_COUNT; i++) {
        pinMode(FIRST_PIN + i, OUTPUT);
    }

    led_write(0xff); // 1111 1111 = 모든 비트가 1 -> 모든 LED 켜짐
}

void loop() {}
