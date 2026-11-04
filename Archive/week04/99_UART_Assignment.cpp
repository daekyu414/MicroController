#include <Arduino.h>
#include <stdarg.h>
#include <string.h>

const int FIRST_PIN = 2;
const int LED_CNT = 8;

void led_write(byte value)
{
  value = ~value;

  for(int i = 0; i < LED_CNT; i++)
  {
    digitalWrite(FIRST_PIN + i, (value >> i) & 1);
  }
}

// 한 바이트 보내기
//
// 자료의 레지스터 방식과 하는 일이 같다
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

void Printf(const char* fmt, ...)
{
  char text[96];
  va_list args;

  va_start(args, fmt);
  vsnprintf(text, sizeof(text), fmt, args);
  va_end(args);

  tx_str(text, strlen(text));
}

void led_all_onoff(int cnt)
{
  for(int i = 0; i < cnt; i++)
  {
    led_write(0xFF);
    delay(300);
    led_write(0x00);
    delay(300);
  }
}

void led_alternating(byte pattern, int cnt)
{
  for(int i = 0; i < cnt; i++)
  {
    led_write(pattern);
    delay(300);
    led_write(~pattern);
    delay(300);
  }
}

void led_shift(int cnt)
{
  for(int c = 0; c < cnt; c++)
  {
    for(int i = 0; i < LED_CNT; i++)
    {
      led_write(1 << i);
      delay(80);
    }
    for(int i = LED_CNT - 2; i > 0; i--)
    {
      led_write(1 << i);
      delay(80);
    }
  }

  led_write(1 << 0);
  delay(80);
}

// -- 프로토콜--------------
// 자료와 값만 다르고 구조는 똑같다.
const byte STX = '<';
const byte ETX = '>';

const byte CMD_ALL_ONOFF = '1';
const byte CMD_ALT = '2';
const byte CMD_SHIFT = '4';

// 0 : 보통, 1: command 수신, 2 : data 수신 3 : 끝 기다림
int decode = 0;
byte command = 0;
char data[3];
int data_len = 0;
int expected_len = 0;
bool ready = false;

void rx_decode()
{
  // 패킷 하나가 완성되면 멈춘다. 나머지는 실행이 끝난 뒤 버퍼에서 이어 읽는다.
  while(Serial.available() > 0 && ready != 1)
  {
    byte c = Serial.read();

    if(decode == 0)
    {
      if(c == STX) // 시작 표시를 만나면 다음 상태로
      {
        decode = 1;
      }
    }
    else if(decode == 1)
    {
      command = c; // 한 바이트를 명령으로 저장

      if(command == CMD_ALT)
      {
        expected_len = 2;
      }
      else if(command == CMD_ALL_ONOFF || command == CMD_SHIFT)
      {
        expected_len = 1;
      }
      else
      {
        Printf("Invalid Command\n");
        decode = 0;
        continue;
      }
      data_len = 0;
      decode = 2;
    }
    else if(decode == 2)
    {
      data[data_len] = c;
      data_len++;

      if(data_len == expected_len)
      {
        data[data_len] = '\0';
        decode = 3;
      }
    }
    else
    {
      if(c == ETX) // 끝 표시면 실행 준비
      {
        ready = true;
      }
      else
      {
        Printf("RX error\n");
      }
      decode = 0; // 어느 쪽이든 보통 상태로 돌아간다
    }
  }
}

int hex_to_int(char c)
{
    if(c >= '0' && c <= '9')
    {
        return c - '0';
    }
    else if(c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    else if(c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }

    return -1;
}

void run_command(byte cd, char* dt)
{
  if(cd == CMD_ALL_ONOFF)
  {
    if(dt[0] < '0' || dt[0] > '9')
    {
        Printf("Invalid DATA\n");
        return;
    }
    int cnt = dt[0] - '0';
    Printf("CMD 1 : LED ALL ON OFF\n");
    led_all_onoff(cnt);
  }
  else if(cd == CMD_ALT)
  {
    int high = hex_to_int(dt[0]);
    int low = hex_to_int(dt[1]);

    if(high == -1 || low == -1)
    {
        Printf("Invalid DATA\n");
        return;
    }

    byte pattern = (high << 4) | low;

    Printf("CMD 2 : LED ALTERNATING ON OFF\n");
    led_alternating(pattern, 3);
  }
  else if(cd == CMD_SHIFT)
  {
    if(dt[0] < '0' || dt[0] > '9')
    {
      Printf("Invalid DATA\n");
      return;
    }
    int cnt = dt[0] - '0';
    Printf("CMD 4 : LED SHIFT\n");
    led_shift(cnt);
  }
  else
  {
    Printf("Invalid Command %c\n", cd);
  }
}

void setup()
{
  for(int i = 0; i < LED_CNT; i++)
  {
    pinMode(FIRST_PIN + i, OUTPUT);
  }

  led_write(0x00);
  Serial.begin(9600);
}

void loop()
{
  rx_decode();

  if(ready == true)
  {
    run_command(command, data);

    ready = false;
    command = 0;
    data_len = 0;

    led_write(0x00);
  }
}
