#include <Arduino.h>

const int FIRST_LED = 2;
const int LED_CNT = 8;

const int FIRST_SW = 10;
const int SW_CNT = 3;

void led_write(byte value)
{
  value = ~value;

  for(int i = 0; i < LED_CNT; i++)
  {
    digitalWrite(FIRST_LED + i, (value >> i) & 0x01);
  }
}

byte sw_read()
{
  byte value = 0x00;

  for(int i = 0; i < SW_CNT; i++)
  {
    if(digitalRead(FIRST_SW + i) == LOW)
    {
      value = value | (1 << i);
    }
  }

  return value;
}

byte prev_sw = 0;

byte sw_pressed()
{
  byte now = sw_read();
  byte edge = now & ~prev_sw;

  prev_sw = now;
  return edge;
}

void led_shift(int count, int speed)
{
  for(int i = 0; i < count; i++)
  {
    int start;
    if(i == 0)
    {
      start = 0;
    }
    else
    {
      start = 1;
    }
    
    for(int j = start; j < LED_CNT; j++)
    {
      led_write(1 << j);
      delay(speed);
    }

    for(int j = LED_CNT - 2; j >= 0; j--)
    {
      led_write(1 << j);
      delay(speed);
    }
  }
}

void led_alternating(int count, int speed)
{
  for(int i = 0; i < count; i++)
  {
    led_write(0xAA);
    delay(speed);
    led_write(0x55);
    delay(speed);
  }
}

void led_inward_pair(int count, int speed)
{
  for(int i = 0; i < count; i++)
  {
    int start;
    if(i == 0)
    {
      start = 0;
    }
    else
    {
      start = 1;
    }
    
    for(int j = start; j < LED_CNT / 2; j++)
    {
      led_write((1 << j) | (1 << (LED_CNT - 1 - j)));
      delay(speed);
    }
    for(int j = (LED_CNT / 2) - 2; j >= 0; j--)
    {
      led_write((1 << j) | (1 << (LED_CNT - 1 - j)));
      delay(speed);
    }
  }
}

void setup()
{
  for(int i = 0; i < LED_CNT; i++)
  {
    pinMode(FIRST_LED + i, OUTPUT);
  }
  for(int i = 0; i < SW_CNT; i++)
  {
    pinMode(FIRST_SW + i, INPUT_PULLUP);
  }

  led_write(0x00);
}

void loop()
{
  byte edge = sw_pressed();

  if(edge & 0x01)
  {
    led_shift(2, 100);
    led_write(0x00);
  }
  else if(edge & 0x02)
  {
    led_alternating(2, 300);
    led_write(0x00);
  }
  else if(edge & 0x04)
  {
    led_inward_pair(2, 100);
    led_write(0x00);
  }
}
