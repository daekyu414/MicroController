#include <Arduino.h>

void setup()
{
    Serial.begin(9600, SERIAL_8N2);
}

void loop()
{
    if (Serial.available() > 0)
    {
        byte data = Serial.read();
        Serial.write(data);  // 받은 바이트 그대로 반환
    }
}
