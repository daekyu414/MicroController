/*
  마이크로컨트롤러응용 - 3차시 예제 (PCINT)

  LED : D2 ~ D9                     (2차시와 동일, Active Low)
  SW1 : D10 = PB2 = PCINT2           왕복 shift 2회
  SW2 : D11 = PB3 = PCINT3           교대 점멸 2회
  SW3 : D12 = PB4 = PCINT4           교차 2회
  기본 동작 : 전체 점멸

  동작 중에 다른 스위치를 누르면 그 동작을 먼저 끝낸 뒤,
  중단될 때 떠 있던 화면과 남은 시간까지 그대로 되살려 이어간다.
*/

#include <Arduino.h>
#include <avr/interrupt.h>

const int FIRST_PIN = 2; // LED : D2 ~ D9
const int LED_COUNT = 8;

const int SW1_PIN = 10; // PB2 - PCINT2
const int SW2_PIN = 11; // PB3 - PCINT3
const int SW3_PIN = 12; // PB4 - PCINT4

const byte ALL_ON = 0xFF;
const byte ALL_OFF = 0x00;

// 비트는 LED 켜짐을 뜻한다. bit0 = D2
// 회로가 Active Low 라서 뒤집어 내보낸다
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

// PCINT0 그룹(PORTB)은 어느 핀이 바뀌어도 이 ISR 하나가 불린다.
// 게다가 누를 때와 뗄 때 모두 발생하므로 직접 구분해야 한다.
ISR(PCINT0_vect) {
  byte now = PINB & 0x1C;     // 스위치 세 개만 본다
  byte fell = prev_pb & ~now; // 1 -> 0 으로 떨어진 비트 = 눌린 것 <- 이것만 감지
  prev_pb = now;

  if (millis() - last_press < 200) {
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

// -- 화면 계산
// delay 를 쓰지 않으므로, "몇 번째 화면인가"만 받아서 모양을 돌려준다.

// 전체 점멸 : 켜짐 / 꺼짐 두 화면이 번갈아
byte frame_allonoff(int s) {
  if (s % 2 == 0) {
    return ALL_ON;
  }
  return ALL_OFF;
}

// 왕복 shift : 0~7 로 올라갔다 6~1 로 내려온다. 한 왕복 = 14 화면
byte frame_shift(int s) {
  int n = s % 14;
  int i = n;

  if (n >= 8) {
    i = 14 - n;
  }

  return 1 << i;
}

// 교대 점멸 : 0xAA 와 그 반대 모양 0x55
byte frame_alt(int s) {
  if (s % 2 == 0) {
    return 0xAA;
  }            // 1010 1010
  return 0x55; // 0101 0101
}

// 교차 : 0~3 으로 모였다 2~1 로 벌어진다. 한 번 = 6 화면
byte frame_cross(int s) {
  int n = s % 6;
  int i = n;

  if (n >= 4) {
    i = 6 - n;
  }

  return (1 << i) | (1 << (LED_COUNT - 1 - i));
}

// -- 지금 무슨 동작을 하고 있는가 ────────────────────────
const byte TASK_DEFAULT = 0;
const byte TASK_SHIFT = 1;
const byte TASK_ALT = 2;
const byte TASK_CROSS = 3;

// 각 동작의 화면 수 (마지막 +1 은 첫 LED 로 돌아오는 화면)
const int SHIFT_STEPS = 14 * 2 + 1;
const int ALT_STEPS = 2 * 2;
const int CROSS_STEPS = 6 * 2 + 1;
const int DEFAULT_STEPS = 2; // 켜짐 / 꺼짐

byte task = TASK_DEFAULT; // 지금 하는 동작
int step = 0;             // 지금 화면에 떠 있는 화면 번호
int interval = 500;       // 화면 하나가 유지되는 시간(ms)

unsigned long last_frame = 0; // 지금 화면을 띄운 시각

// 지금 화면에 떠 있어야 할 모양을 그린다
void draw_frame() {
  if (task == TASK_SHIFT) {
    led_write(frame_shift(step));
  } else if (task == TASK_ALT) {
    led_write(frame_alt(step));
  } else if (task == TASK_CROSS) {
    led_write(frame_cross(step));
  } else {
    led_write(frame_allonoff(step));
  }
}

// 동작이 다 끝났는가 (기본 동작은 끝나지 않는다)
bool task_finished() {
  if (task == TASK_SHIFT) {
    return step >= SHIFT_STEPS;
  }
  if (task == TASK_ALT) {
    return step >= ALT_STEPS;
  }
  if (task == TASK_CROSS) {
    return step >= CROSS_STEPS;
  }

  return false;
}

// -- 중단된 동작을 쌓아 두는 스택 ────────────────────────
// CPU 가 ISR 에 들어갈 때 복귀 주소와 레지스터를 스택에 쌓는 것과 같다.
// 화면 번호뿐 아니라 "그 화면이 더 떠 있어야 할 시간"까지 함께 보관한다.
const int STACK_MAX = 4;

byte stack_task[STACK_MAX];
int stack_step[STACK_MAX];
int stack_interval[STACK_MAX];
int stack_left[STACK_MAX]; // 남은 시간(ms)
int stack_top = 0;

void push_task() {
  if (stack_top >= STACK_MAX) {
    return;
  } // 넘치면 더 쌓지 않는다

  // 지금 화면이 떠 있던 시간을 빼서, 앞으로 더 떠 있어야 할 시간을 구한다
  unsigned long used = millis() - last_frame;
  int left = interval - (int)used;
  if (left < 0) {
    left = 0;
  }

  stack_task[stack_top] = task;
  stack_step[stack_top] = step;
  stack_interval[stack_top] = interval;
  stack_left[stack_top] = left;
  stack_top = stack_top + 1;
}

// 쌓아 둔 것이 있으면 가장 최근 것부터 꺼내 그 화면을 되살린다
void restore_task()
{
  int left;

  if (stack_top > 0) {
    stack_top = stack_top - 1;
    task = stack_task[stack_top];
    step = stack_step[stack_top];
    interval = stack_interval[stack_top];
    left = stack_left[stack_top];
  }
  else {
    task = TASK_DEFAULT;
    step = 0;
    interval = 500;
    left = interval;
  }

  draw_frame(); // 중단될 때 떠 있던 화면을 그대로 되살린다

  // 남은 시간만큼만 더 유지되도록 시작 시각을 과거로 당겨 둔다
  last_frame = millis() - (unsigned long)(interval - left);
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

// delay 가 하나도 없다. loop 는 멈추지 않고 계속 돌면서
//   1) 스위치 이벤트가 왔는지 보고
//   2) 화면 바꿀 시간이 됐는지 확인한다
void loop() {
  // 깃발을 읽고 비우는 동안에는 인터럽트를 잠시 막는다
  noInterrupts();
  byte event = sw_event;
  sw_event = 0;
  interrupts();

  if (event != 0) {
    push_task(); // 떠 있던 화면과 남은 시간을 쌓아 둔다

    if (event & 0x01)
    {
      task = TASK_SHIFT;
      interval = 80;
    }
    if (event & 0x02) {
      task = TASK_ALT;
      interval = 300;
    }
    if (event & 0x04) {
      task = TASK_CROSS;
      interval = 150;
    }

    step = 0;
    draw_frame(); // 기다리지 않고 바로 첫 화면부터
    last_frame = millis();
    return;
  }

  // 시간이 됐을 때만 다음 화면으로 넘어간다
  if (millis() - last_frame >= (unsigned long)interval)
  {
    step = step + 1;

    if (task == TASK_DEFAULT && step >= DEFAULT_STEPS)
    {
      step = 0;
    }

    if (task_finished())
    {
      restore_task(); // 끝났으면 쉬는 화면 없이 바로 복귀
      return;
    }

    draw_frame();
    last_frame = millis();
  }
}
