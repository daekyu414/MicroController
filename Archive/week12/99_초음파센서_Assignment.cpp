#include <Arduino.h>

const int TRIG_PIN = A4; // Trig : A4 (PC4)
const int ECHO_PIN = 12; // Echo : D12 (PB4)
const int BUZZER_PIN = A2;

const unsigned long TIMEOUT_US = 25000;

// 거리 (cm) 를 잰다. 범위를 벗어나면 -1
float read_distance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    unsigned long t = pulseIn(ECHO_PIN, HIGH, TIMEOUT_US);
    if (t == 0) {
        return -1;
    }
    return t / 58.0;
}

void setup() {
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
}

void loop()
{
  float d = read_distance();

  if (d < 0 || d > 30)
  {
    noTone(BUZZER_PIN);
  }
  else if(d < 10)
  {
    tone(BUZZER_PIN, 1000);
  }
  else
  {
    tone(BUZZER_PIN, 1000);
    delay(100);
    noTone(BUZZER_PIN);
    delay(d * 10);
  }
  delay(60);
}
