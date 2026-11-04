
#include <Arduino.h>

// ================================
// 1. 핀 번호 및 LED 개수 설정
// ================================

// LED 8개를 D4 ~ D11에 연결 (Active-Low)
const int FIRST_LED = 4;
const int LED_CNT = 8;

// 외부 인터럽트를 사용할 스위치 핀
// Arduino UNO: D2 = INT0, D3 = INT1
const int SW1_PIN = 2;
const int SW2_PIN = 3;


// ================================
// 2. LED 출력 함수
// ================================

void led_write(byte value)
{
  // LED가 Active-Low 방식이므로 비트를 반전
  // 1 = LED ON, 0 = LED OFF로 사용하기 위함
  value = ~value;

  for(int i = 0; i < LED_CNT; i++)
  {
    // value의 i번째 비트를 추출하여 LED에 출력
    // bit0 = D4, bit1 = D5, ... bit7 = D11
    digitalWrite(FIRST_LED + i, (value >> i) & 0x01);
  }
}


// ================================
// 3. LED 왕복 시프트 함수
// ================================

void led_shift(int count, int speed)
{
  // count: 왕복 횟수
  // speed: LED 이동 간격(ms)

  for(int i = 0; i < count; i++)
  {
    int start;

    // 첫 번째 왕복에서는 첫 LED부터 시작
    if(i == 0)
    {
      start = 0;
    }
    else
    {
      // 두 번째 왕복부터는 첫 LED 중복 출력 방지
      start = 1;
    }

    // LED를 왼쪽에서 오른쪽으로 이동
    // D4 -> D5 -> ... -> D11
    for(int j = start; j < LED_CNT; j++)
    {
      led_write(1 << j);
      delay(speed);
    }

    // LED를 오른쪽에서 왼쪽으로 이동
    // D10 -> D9 -> ... -> D4
    for(int j = LED_CNT - 2; j >= 0; j--)
    {
      led_write(1 << j);
      delay(speed);
    }
  }
}


// ================================
// 4. LED 교대 점멸 함수
// ================================

void led_alternating(int count, int speed)
{
  for(int i = 0; i < count; i++)
  {
    // 10101010: 짝수 인덱스 LED OFF, 홀수 ON
    led_write(0xAA);
    delay(speed);

    // 01010101: 짝수 인덱스 LED ON, 홀수 OFF
    led_write(0x55);
    delay(speed);
  }
}


// ================================
// 5. 양 끝에서 중앙으로 이동하는 함수
// ================================

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
      // 다음 반복에서 양 끝 LED 중복 출력 방지
      start = 1;
    }

    // 양 끝에서 중앙으로 LED 2개씩 이동
    for(int j = start; j < LED_CNT / 2; j++)
    {
      // 대칭 위치의 LED 2개를 동시에 켬
      led_write((1 << j) | (1 << (LED_CNT - 1 - j)));
      delay(speed);
    }

    // 중앙에서 다시 양 끝으로 이동
    for(int j = (LED_CNT / 2) - 2; j >= 0; j--)
    {
      led_write((1 << j) | (1 << (LED_CNT - 1 - j)));
      delay(speed);
    }
  }
}


// ================================
// 6. 모든 LED 점멸 함수
// ================================

void led_all_onoff(int count, int speed)
{
  for(int i = 0; i < count; i++)
  {
    // LED 8개 모두 ON
    led_write(0xFF);
    delay(speed);

    // LED 8개 모두 OFF
    led_write(0x00);
    delay(speed);
  }
}


// ================================
// 7. 인터럽트 관련 전역 변수
// ================================

// ISR과 loop()에서 공유하는 변수이므로 volatile 사용
// bit0 = SW1 이벤트 발생
// bit1 = SW2 이벤트 발생
volatile byte sw_event = 0;

// 마지막으로 인터럽트를 처리한 시각
// 200ms 이내 중복 입력을 방지하기 위해 사용
volatile unsigned long last_press = 0;


// ================================
// 8. SW1 인터럽트 서비스 루틴
// ================================

void isr_sw1()
{
  // 마지막 입력으로부터 200ms가 지나지 않았다면 무시
  if(millis() - last_press < 200)
  {
    return;
  }

  // 마지막 입력 시각 갱신
  last_press = millis();

  // bit0을 1로 설정하여 SW1 이벤트 기록
  sw_event = sw_event | 0x01;
}


// ================================
// 9. SW2 인터럽트 서비스 루틴
// ================================

void isr_sw2()
{
  // 스위치 바운싱에 의한 중복 입력 방지
  if(millis() - last_press < 200)
  {
    return;
  }

  last_press = millis();

  // bit1을 1로 설정하여 SW2 이벤트 기록
  sw_event = sw_event | 0x02;
}


// ================================
// 10. 초기 설정
// ================================

void setup()
{
  // D4 ~ D11을 LED 출력 핀으로 설정
  for(int i = 0; i < LED_CNT; i++)
  {
    pinMode(FIRST_LED + i, OUTPUT);
  }

  // 내부 풀업 저항 활성화
  // 평상시 HIGH, 스위치를 누르면 LOW
  pinMode(SW1_PIN, INPUT_PULLUP);
  pinMode(SW2_PIN, INPUT_PULLUP);

  // HIGH -> LOW로 변경되는 순간 인터럽트 발생
  // D2에서 인터럽트가 발생하면 isr_sw1() 실행
  attachInterrupt(digitalPinToInterrupt(SW1_PIN),
                  isr_sw1, FALLING);

  // D3에서 인터럽트가 발생하면 isr_sw2() 실행
  attachInterrupt(digitalPinToInterrupt(SW2_PIN),
                  isr_sw2, FALLING);

  // 초기 LED 전체 OFF
  led_write(0x00);
}


// ================================
// 11. 메인 반복문
// ================================

void loop()
{
  // 인터럽트를 잠시 비활성화
  // 이벤트를 읽고 초기화하는 중 ISR이 실행되지 않도록 함
  noInterrupts();

  // 발생한 스위치 이벤트를 지역 변수로 복사
  byte event = sw_event;

  // 이벤트를 읽었으므로 초기화
  sw_event = 0;

  // 인터럽트 다시 활성화
  interrupts();


  // SW1 이벤트(bit0)가 발생했다면
  if(event & 0x01)
  {
    // LED 왕복 시프트 2회 실행
    led_shift(2, 80);

    // LED 전체 OFF
    led_write(0x00);
  }

  // SW2 이벤트(bit1)가 발생했다면
  if(event & 0x02)
  {
    // LED 교대 점멸 2회 실행
    led_alternating(2, 300);

    // LED 전체 OFF
    led_write(0x00);
  }

  // 기본 동작: LED 전체 점멸 1회
  // 0.5초 ON -> 0.5초 OFF
  led_all_onoff(1, 500);
}
