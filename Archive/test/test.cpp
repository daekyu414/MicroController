
#include <Arduino.h>
#include <avr/interrupt.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// LED : D2 ~ D9 (Active Low)
const int FIRST_LED = 2;
const int LED_CNT = 8;

// SW1 : D10 = PB2 = PCINT2
// SW2 : D11 = PB3 = PCINT3
// SW3 : D12 = PB4 = PCINT4
const int FIRST_SW = 10;

// ADC
const int POT_PIN = A1;
const int CDS_PIN = A3;
const int THRESHOLD = 400;

// 계산기 변수
int first_value = 0;
int second_value = 0;
int result = 0;
int current_value = 0;
int prev_value = -1;

// 0: 첫 번째 입력 대기
// 1: 두 번째 입력 대기
// 2: 계산 대기
// 3: 계산 완료
int step = 0;

// 스위치 인터럽트 변수
volatile byte sw_event = 0;
volatile byte prev_pinb = 0;

unsigned long last_press[3] = {0, 0, 0};
const unsigned long DEBOUNCE = 50;

bool dark_mode = false;

// LED 출력
void led_write(byte value)
{
  value = ~value;

  for (int i = 0; i < LED_CNT; i++)
  {
    digitalWrite(FIRST_LED + i,
                 (value >> i) & 1);
  }
}

// UART 출력
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

// PCINT 인터럽트
ISR(PCINT0_vect)
{
  byte now = PINB;

  // HIGH -> LOW로 바뀐 스위치 감지
  byte changed = prev_pinb ^ now;
  byte pressed = changed & (~now);

  // PB2, PB3, PB4만 확인
  sw_event |= (pressed >> 2) & 0x07;

  prev_pinb = now;
}

// 스위치 이벤트 읽기
byte sw_read_event()
{
  noInterrupts();

  byte event = sw_event;
  sw_event = 0;

  interrupts();

  byte valid_event = 0;

  for (int i = 0; i < 3; i++)
  {
    if (event & (1 << i))
    {
      if (millis() - last_press[i] >= DEBOUNCE)
      {
        valid_event |= (1 << i);
        last_press[i] = millis();
      }
    }
  }

  return valid_event;
}

// 계산기 초기화
void calculator_reset()
{
  first_value = 0;
  second_value = 0;
  result = 0;

  step = 0;
  prev_value = -1;

  led_write(0x00);

  noInterrupts();
  sw_event = 0;
  interrupts();

  Printf("\nCalculator RESET!\n");
}

// 첫 번째 숫자 결정
void save_first()
{
  if (step != 0)
    return;

  first_value = current_value;
  step = 1;

  Printf("\nFIRST VALUE = %d\n", first_value);
  Printf("Set SECOND value and press SW2\n");
}

// 두 번째 숫자 결정
void save_second()
{
  if (step != 1)
    return;

  second_value = current_value;
  step = 2;

  Printf("\nSECOND VALUE = %d\n", second_value);
  Printf("Press SW3 to calculate\n");
}

// 계산 실행
void calculate()
{
  if (step != 2)
    return;

  result = first_value + second_value;

  // LED는 8비트만 표현 가능
  byte led_value = (byte)(result & 0xFF);

  led_write(led_value);

  Printf("\n**********************\n");
  Printf("FIRST  = %d\n", first_value);
  Printf("SECOND = %d\n", second_value);
  Printf("RESULT = %d\n", result);
  Printf("LED    = 0x%02X\n", led_value);
  Printf("**********************\n");

  step = 3;
}

void setup()
{
  Serial.begin(9600);

  // LED 출력 설정
  for (int i = 0; i < LED_CNT; i++)
  {
    pinMode(FIRST_LED + i, OUTPUT);
  }

  led_write(0x00);

  // 스위치 INPUT_PULLUP
  for (int i = 0; i < 3; i++)
  {
    pinMode(FIRST_SW + i, INPUT_PULLUP);
  }

  // ADC 입력
  pinMode(POT_PIN, INPUT);
  pinMode(CDS_PIN, INPUT);

  // ADC 기준전압 = AVcc (5V)
  analogReference(DEFAULT);

  // PCINT 설정
  noInterrupts();

  prev_pinb = PINB;

  PCMSK0 = (1 << PCINT2) |
           (1 << PCINT3) |
           (1 << PCINT4);

  PCIFR = (1 << PCIF0);
  PCICR |= (1 << PCIE0);

  interrupts();

  Printf("Binary Calculator START\n");
  Printf("SW1: FIRST value\n");
  Printf("SW2: SECOND value\n");
  Printf("SW3: Calculate\n");
}

void loop()
{
  // 조도센서 ADC
  int cds_value = analogRead(CDS_PIN);

  // 어두우면 계산기 초기화
  if (cds_value < THRESHOLD)
  {
    if (!dark_mode)
    {
      dark_mode = true;
      calculator_reset();
      Printf("DARK MODE\n");
    }

    // 어두운 동안 입력 무시
    noInterrupts();
    sw_event = 0;
    interrupts();

    delay(10);
    return;
  }

  // 다시 밝아지면 사용 가능
  if (dark_mode)
  {
    dark_mode = false;
    Printf("LIGHT MODE\n");
  }

  // 가변저항 ADC 0~1023
  int adc_value = analogRead(POT_PIN);

  // 0~255로 변환
  current_value = map(adc_value, 0, 1023, 0, 255);

  // 값이 변경되면 시리얼 모니터 출력
  if (current_value != prev_value)
  {
    Printf("ADC = %d, VALUE = %d\n",
           adc_value, current_value);

    prev_value = current_value;
  }

  // 스위치 인터럽트 이벤트 확인
  byte event = sw_read_event();

  if (event & 0x01)
  {
    save_first();
  }

  if (event & 0x02)
  {
    save_second();
  }

  if (event & 0x04)
  {
    calculate();
  }

  delay(10);
}
