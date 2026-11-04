#include <Arduino.h>

void setup()
{
    Serial.begin(9600, SERIAL_8N2);
}

void loop()
{
    while (Serial.available() > 0)
    {
        byte data = Serial.read();

        Serial.print("0x");

        if (data < 0x10)
        {
            Serial.print("0");
        }

        Serial.println(data, HEX);
    }
}