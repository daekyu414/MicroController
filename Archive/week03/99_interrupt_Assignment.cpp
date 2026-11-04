#include <Arduino.h>
#include <avr/interrupt.h>

const int FIRST_PIN = 2; // LED : D2 ~ D9
const int LED_COUNT = 8;

const int SW1_PIN = 10; // PB2 - PCINT2
const int SW2_PIN = 11; // PB3 - PCINT3
const int SW3_PIN = 12; // PB4 - PCINT4

void led_write(byte value) {
  value = ~value;

  for (int i = 0; i < LED_COUNT; i++) {
    digitalWrite(FIRST_PIN + i, (value >> i) & 1);
  }
}

// ISR 과 loop 가 함께 쓰는 변수에는 volatile 을 붙인다.
// 안 붙이면 컴파일러가 "이 값은 안 변한다"고 단정해 버린다.
volatile byte sw_event = 0;
volatile unsigned long last_press = 0;

// PB2, PB3, PB4 = 0x00011100 = 0x1C
// 풀업이라 아무것도 안 눌렀을 때는 세 비트 모두 1이다.
volatile byte prev_pb = 0x1C;

ISR(PCINT0_vect) {
  byte now = PINB & 0x1C;     // 스위치 세 개만 본다
  byte fell = prev_pb & ~now; // 1 -> 0 으로 떨어진 비트 = 눌린 것
  prev_pb = now;

  if (millis() - last_press < 200)
  {
    return;
  } // 채터링 무시
  last_press = millis();

  if (fell & 0x04) {
    sw_event = sw_event | 0x01;
  } // PB2 = SW1
  if (fell & 0x08) {
    sw_event = sw_event | 0x02;
  } // PB3 = SW2
  if (fell & 0x10) {
    sw_event = sw_event | 0x04;
  } // PB4 = SW3
}

// -- 지금 무슨 동작을 하고 있는가 ────────────────────────
const byte TASK_DEFAULT = 0;
const byte TASK_ALTERNATIVE1 = 1;
const byte TASK_ALTERNATIVE2 = 2;
const byte TASK_SHIFT = 3;

int task = TASK_DEFAULT; // 지금 하는 동작
int step = 0;             // 지금 화면에 떠 있는 화면 번호
int tesk_step[4] = {0,};
int interval = 300;       // 화면 하나가 유지되는 시간(ms)

unsigned long last_frame = 0; // 지금 화면을 띄운 시각

byte frame_default(int step)
{
    if(step == 0)
    {
        return 0xAA;
    }
    return 0x55;
}

byte frame_alternative1(int step)
{
    if(step == 0)
    {
        return 0xF0;
    }
    return 0x0F;
}

byte frame_alternative2(int step)
{
    if(step == 0)
    {
        return 0xC3;
    }
    return 0x3C;
}

byte frame_shift(int step)
{
    int n = step % 14;

    if(n >= 8)
    {
        n = 14 - n;
    }

    return 1 << n;
}

void draw_frame()
{
    if(task == TASK_ALTERNATIVE1)
    {
        led_write(frame_alternative1(step));
    }
    else if(task == TASK_ALTERNATIVE2)
    {
        led_write(frame_alternative2(step));
    }
    else if(task == TASK_SHIFT)
    {
        led_write(frame_shift(step));
    }
    else
    {
        led_write(frame_default(step));
    }
}

void setup()
{
  for (int i = 0; i < LED_COUNT; i++) {
    pinMode(FIRST_PIN + i, OUTPUT);
  }

  pinMode(SW1_PIN, INPUT_PULLUP);
  pinMode(SW2_PIN, INPUT_PULLUP);
  pinMode(SW3_PIN, INPUT_PULLUP);

  // PCINT 는 Arduino 함수가 없어 레지스터를 직접 설정한다
  PCICR = PCICR | (1 << PCIE0);     // PORTB 그룹 사용
  PCMSK0 = PCMSK0 | (1 << PCINT2)   // PB2 감시
                  | (1 << PCINT3)  // PB3 감시
                  | (1 << PCINT4); // PB4 감시

  draw_frame();
  last_frame = millis();
}

void loop() {
  // 깃발을 읽고 비우는 동안에는 인터럽트를 잠시 막는다
  noInterrupts();
  byte event = sw_event;
  sw_event = 0;
  interrupts();

  if (event != 0)
  {
    if (event & 0x01)
    {
      tesk_step[task] = step;
      task = (task + 1) % 4;
      step = tesk_step[task];
    }
    if (event & 0x02) {
      interval = interval + 50;
      if(interval > 500)
      {
        interval = 100;
      }
    }
    if (event & 0x04) {
      interval = interval - 50;
      if(interval < 100)
      {
        interval = 500;
      }
    }

    draw_frame(); // 기다리지 않고 바로 첫 화면부터
    last_frame = millis();
    return;
  }

  // 시간이 됐을 때만 다음 화면으로 넘어간다
  if (millis() - last_frame >= (unsigned long)interval)
  {
    step = step + 1;

    if (task <= TASK_ALTERNATIVE2 && step >= 2)
    {
      step = 0;
    }
    if(task == TASK_SHIFT && step >= 14)
    {
        step = 0;
    }

    draw_frame();
    last_frame = millis();
  }
}
