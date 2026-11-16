#include <Arduino.h>

#define LED_PIN 2
#define SWITCH_PIN 10

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
}
void loop()
{
  if (digitalRead(SWITCH_PIN) == LOW)
    digitalWrite(LED_PIN, HIGH);
  else
    digitalWrite(LED_PIN, LOW);
}
