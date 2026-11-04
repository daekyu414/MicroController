
#include <Arduino.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

// UART 송신 버퍼
#define TX_SIZE 128

volatile char tx_buffer[TX_SIZE];
volatile int tx_head = 0;
volatile int tx_tail = 0;

// UART 초기화 (9600bps)
void uart_init()
{
  UBRR0H = 0;
  UBRR0L = 103;  // 16MHz, 9600bps

  UCSR0A = 0x00;
  UCSR0B = (1 << TXEN0);  // 송신 활성화
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8N1

  sei(); // 전역 인터럽트 허용
}

// 송신 데이터 레지스터가 비면 실행
ISR(USART_UDRE_vect)
{
  if (tx_head != tx_tail)
  {
    UDR0 = tx_buffer[tx_tail];

    tx_tail = (tx_tail + 1) % TX_SIZE;
  }
  else
  {
    // 보낼 데이터가 없으면 인터럽트 비활성화
    UCSR0B &= ~(1 << UDRIE0);
  }
}

// 문자 하나를 송신 버퍼에 저장
void tx_char(char txChar)
{
  int next_head = (tx_head + 1) % TX_SIZE;

  // 버퍼가 가득 차면 공간이 생길 때까지 대기
  while (next_head == tx_tail)
  {
  }

  // 버퍼 갱신 중 인터럽트 잠시 비활성화
  uint8_t oldSREG = SREG;
  cli();

  tx_buffer[tx_head] = txChar;
  tx_head = next_head;

  // UART 송신 인터럽트 활성화
  UCSR0B |= (1 << UDRIE0);

  SREG = oldSREG;
}

// 문자열 송신
void tx_str(char* txStr, int len)
{
  for (int i = 0; i < len; i++)
  {
    tx_char(txStr[i]);
  }
}

// printf 형식 출력
void Printf(const char* fmt, ...)
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
  uart_init();
  delay(100);

  Printf("********************************************\n");
  Printf("Clock Speed = %lu\n", (unsigned long)F_CPU);
  Printf("PCMSK0     = 0x%02X\n", PCMSK0);
  Printf("********************************************\n");
}

void loop()
{

}
