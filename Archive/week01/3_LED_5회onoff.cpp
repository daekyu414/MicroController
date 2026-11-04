
#include <Arduino.h>

const int FIRST_PIN = 2;
const int LED_COUNT = 8;

const byte ALL_ON  = 0xFF;  // 1111 1111
const byte ALL_OFF = 0x00;  // 0000 0000

void led_write(byte value)
{
    value = ~value;

    for (int i = 0; i < LED_COUNT; i++)
    {
        digitalWrite(FIRST_PIN + i, (value >> i) & 1);
    }
}

void setup()
{
    for (int i = 0; i < LED_COUNT; i++)
    {
        pinMode(FIRST_PIN + i, OUTPUT);
    }

    // 5번만 하고 끝내려면 loop 가 아니라 setup 에 쓴다
    for (int i = 0; i < 5; i++)
    {
        led_write(ALL_ON);
        delay(500);
        led_write(ALL_OFF);
        delay(500);
    }
}

void loop()
{

}
