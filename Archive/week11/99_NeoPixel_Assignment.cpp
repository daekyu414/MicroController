#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

const int NEO_PIN = A4; // 링 IN : A4 (PC4)
const int LED_COUNT = 16;
const int SW_PIN = 10;

Adafruit_NeoPixel ring(LED_COUNT, NEO_PIN, NEO_GRBW + NEO_KHZ800);

uint32_t redColor;
uint32_t greenColor;
uint32_t blueColor;
uint32_t whiteColor;

volatile byte sw_event = 0;
volatile unsigned long last_press = 0;

unsigned long last_frame = 0;

// PB2, PB3, PB4 = 0x00011100 = 0x1C
// 풀업이라 아무것도 안 눌렀을 때는 세 비트 모두 1이다.
volatile byte prev_pb = 0x04;

ISR(PCINT0_vect)
{
    byte now = PINB & 0x04;
    byte fell = prev_pb & ~now;
    prev_pb = now;

    if (fell & 0x04)
    {
        if (millis() - last_press >= 200)
        {
            sw_event = sw_event | 0x01;
            last_press = millis();
        }
    }
}

int interval = 300;
int interval_cnt = 0;
int interval_arr[3] = {300, 100, 500};

int led_index = 0;
int led_positions[5] = {0, 9, 3, 12, 6};

void led_one(int index, uint32_t color)
{
    ring.setPixelColor(index, color);
    ring.show();
}

int rotate_index = 0;

void led_rotate(uint32_t color, int offset)
{
    ring.clear();

    for (int i = 0; i < 5; i++)
    {
        int pos = (led_positions[i] + offset) % LED_COUNT;
        ring.setPixelColor(pos, color);
    }

    ring.show();
}

// 16개 전부 끈다
void led_all_off()
{
    ring.clear(); // 16개 색을 모두 0으로
    ring.show();
}

void setup()
{
  pinMode(SW_PIN, INPUT_PULLUP);

  PCICR = PCICR | (1 << PCIE0);     // PORTB 그룹 사용
  PCMSK0 = PCMSK0 | (1 << PCINT2);   // PB2 감시

  last_press = millis();

  ring.begin();
  ring.clear();
  ring.show();

    //                       R    G    B    W
    redColor = ring.Color(128, 0,   0,   0);
    greenColor = ring.Color(0, 128, 0,   0);
    blueColor = ring.Color(0,   0, 128, 0);
    whiteColor = ring.Color(0,  0,   0, 128); // 흰색은 W 칸을 쓴다
}

void loop()
{
    // 깃발을 읽고 비우는 동안에는 인터럽트를 잠시 막는다
    noInterrupts();
    byte event = sw_event;
    sw_event = 0;
    interrupts();

    if (event & 0x01)
    {
      interval_cnt = (interval_cnt + 1) % 3;
      interval = interval_arr[interval_cnt];
    }

    if (millis() - last_frame >= (unsigned long)interval)
    {
      if(led_index < 5)
      {
        led_one(led_positions[led_index], whiteColor);
        led_index++;
      }
      else if(rotate_index < LED_COUNT)
      {
        led_rotate(whiteColor, rotate_index + 1);
        rotate_index++;
      }
      else if(rotate_index >= LED_COUNT)
      {
        led_all_off();
        led_index = 0;
        rotate_index = 0;
      }

      last_frame = millis();
    }
}
