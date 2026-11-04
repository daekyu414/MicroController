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

void setup()
{
  for (int i = 0; i < LED_COUNT; i++)
  {
    pinMode(FIRST_PIN + i, OUTPUT);
  }
}

void loop()
{
  led_alternating_onoff(0xAA, 3, 500); // 1010 1010
  led_alternating_onoff(0xF0, 4, 500); // 1111 0000
  led_alternating_onoff(0xC3, 5, 500); // 1100 0011
}
