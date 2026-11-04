#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define MAX 255

const int FIRST_PIN = 2;  // LED : D2 ~ D9 (Active Low)
const int LED_COUNT = 8;

const int CDS_PIN = A3;   // PC3 - ADC3
const int THRESHOLD = 400; // 이보다 작으면 어둡다고 본다

// 비트를 LED 핀으로 뿌린다. bit0 = D2
// 회로가 Active Low 라서 뒤집어 내보낸다
void led_write(byte value)
{
    value = ~value;

    for (int i = 0; i < LED_COUNT; i++)
    {
        digitalWrite(FIRST_PIN + i, (value >> i) & 1);
    }
}

// --- 4차시에서 만든 Printf ---
void tx_char(char txChar)
{
    Serial.write(txChar);
}

void tx_str(char* txStr, int len)
{
    for (int i = 0; i < len; i++)
    {
        tx_char(txStr[i]);
    }
}

void Printf(const char* fmt, ...)
{
    char text[96];
    va_list args;

    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);

    tx_str(text, strlen(text));
}

bool prev_dark = false;
int cnt = 0;

void setup()
{
    for (int i = 0; i < LED_COUNT; i++)
    {
        pinMode(FIRST_PIN + i, OUTPUT);
    }
    led_write(0x00);

    Serial.begin(9600);
}

void loop()
{
    int adc = analogRead(CDS_PIN); // 0 ~ 1023

    bool now_dark = (adc < THRESHOLD);

    if(now_dark && !prev_dark)
    {
      cnt++;

      if(cnt > MAX)
      {
        cnt = 0;
      }
      led_write(cnt);
      Printf("Count : %4d\n", cnt);
    }

    prev_dark = now_dark;

    delay(50);
}
