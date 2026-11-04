#include <Arduino.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdarg.h>

void tx_char(char txChar)
{
  Serial.write(txChar);
}

void tx_str(char* txStr, int len)
{
  for(int i = 0; i < len; i++)
  {
    tx_char(txStr[i]);
  }
}

void Printf(const char* fmt, ...) // ...은 개수가 정해지지 않은 인자를 받을 수 있다는 의미
{
  char text[96];
  va_list args;

  va_start(args, fmt);
  vsnprintf(text, sizeof(text), fmt, args);
  va_end(args);

  tx_str(text, strlen(text));
}

void setup()
{
  Serial.begin(9600);
  delay(100);

  Printf("********************************************\n");
  Printf("Clock Speed = %lu\n", (unsigned long)F_CPU);
  Printf("PCMSK0     = 0x%02X\n", PCMSK0);
  Printf("********************************************\n");
}

void loop()
{

}
