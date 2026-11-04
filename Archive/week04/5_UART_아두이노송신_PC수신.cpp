#include <Arduino.h>
#include <avr/interrupt.h>
#include <stdarg.h>
#include <string.h>

const int FIRST_PIN = 2;
const int LED_CNT = 8;

const int SW1_PIN = 10;
const int SW2_PIN = 11;
const int SW3_PIN = 12;

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

// 수신 버퍼
// 한 글자가 아니라 문자열을 받으려면, 끝 문자가 올 때까지 모아 둬야 한다.
const int MAX_BUF = 48;

char rx_buf[MAX_BUF];
int buf_len = 0;
bool line_ready = false; // 한 줄이 완성됐는가

// Arduino는 RX 인터럽트와 수신 버퍼를 이미 제공한다.
// 자료의 ISR(USART_RX_vect) 는 HardwareSerial이 이미 쓰고 있어
// 직접 정의하면 중복 정의 오류가 난다. 대신 받은 것을 여기서 모은다.

void rx_collect()
{
  while(Serial.available() > 0) // Serial.write()가 데이터를 송신(TX) 하는 함수라면, Serial.available()은 수신(RX) 버퍼에 읽을 데이터가 있는지 확인하는 함수
  {
    char c = Serial.read();

    if(c == '\n') // 끝 글자 -> 한 줄 완성
    {
      rx_buf[buf_len] = '\0';
      line_ready = true;
      return;
    }

    if(c == '\r') // 윈도우 줄바꿈의 앞 글자는 버린다
    {
      continue;
    }

    if(buf_len < MAX_BUF - 1)
    {
      rx_buf[buf_len] = c;
      buf_len++;
    }
  }
}

void rx_clear()
{
  buf_len = 0;
  line_ready = false;
}

// 받은 줄에 명령이 들어있으면 그 동작을 한다
void command(char* buff, int len)
{
  if(strstr(buff, "LED_ALL_ONOFF") != NULL)
  {
    tx_str(buff, len);
    tx_char('\n');
    led_all_onoff(3);
  }
  else if(strstr(buff, "LED_ALT") != NULL)
  {
    tx_str(buff, len);
    tx_char('\n');
    led_alternating(0xAA, 3);
  }
  else if(strstr(buff, "LED_SHIFT") != NULL)
  {
    tx_str(buff, len);
    tx_char('\n');
    led_shift(3);
  }
  else
  {
    tx_str(buff, len); // 모르는 명령은 그대로 돌려보낸다.
    tx_char('\n');
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
  rx_collect();

  if(line_ready == true)
  {
    command(rx_buf, buf_len);
    rx_clear();

    led_write(0x00);
  }
}
