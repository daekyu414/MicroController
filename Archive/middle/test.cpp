
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#define STX 0x02
#define ETX 0x03

#define LED_ALL_ON_OFF_M 0x01
#define LED_ALTERNATING_M 0x02
#define LED_SHIFT_M 0x03

#define ADC_ON_M 0x01
#define ADC_OFF_M 0x02
#define ADC_LED_M 0x03

const int FIRST_PIN = 2;
const int LED_CNT = 8;
const int POT_PIN = A1;

// UART 수신 변수
byte decode = 0;
byte len = 0;
byte len_cnt = 0;

byte rx_func = 0;
byte rx_param = 0;

char rx_hw[11];

// ADC 관련 변수
int adc_flag = 0;
int adc_led_control = 0;

unsigned long last_adc = 0;


// ----------------------------------------
// LED 출력 (Active Low)
// ----------------------------------------
void led_write(byte value)
{
    value = ~value;

    for (int i = 0; i < LED_CNT; i++)
    {
        digitalWrite(FIRST_PIN + i,
                     (value >> i) & 1);
    }
}


// ----------------------------------------
// UART 송신
// ----------------------------------------
void tx_char(char c)
{
    Serial.write(c);
}

void tx_str(const char* str)
{
    Serial.print(str);
}


// ----------------------------------------
// LED 전체 점멸
// ----------------------------------------
void LED_ALL_ON_OFF(byte cnt)
{
    for (int i = 0; i < cnt; i++)
    {
        led_write(0xFF);
        delay(200);

        led_write(0x00);
        delay(200);
    }
}


// ----------------------------------------
// LED 교대 점멸
// ----------------------------------------
void LED_ALTERNATING(byte cnt)
{
    for (int i = 0; i < cnt; i++)
    {
        led_write(0xAA);
        delay(200);

        led_write(0x55);
        delay(200);
    }

    led_write(0x00);
}


// ----------------------------------------
// LED 왕복 시프트
// ----------------------------------------
void LED_SHIFT(byte cnt)
{
    for (int k = 0; k < cnt; k++)
    {
        for (int i = 0; i < LED_CNT; i++)
        {
            led_write(0x01 << i);
            delay(100);
        }

        for (int i = 0; i < LED_CNT; i++)
        {
            led_write(0x80 >> i);
            delay(100);
        }
    }

    led_write(0x00);
}


// ----------------------------------------
// ADC 값을 LED 8개로 표시
// ----------------------------------------
void disp_led(unsigned int adc)
{
    int count = (adc + 127) / 128;

    byte value = 0;

    for (int i = 0; i < count; i++)
    {
        value |= (1 << i);
    }

    led_write(value);
}


// ----------------------------------------
// 명령 해석
// ----------------------------------------
void hex_command(const char* hw,
                 byte func, byte param)
{
    // 일반 LED 제어
    if (strcmp(hw, "LED") == 0)
    {
        adc_flag = 0;
        adc_led_control = 0;

        led_write(0x00);

        if (func == LED_ALL_ON_OFF_M)
        {
            LED_ALL_ON_OFF(param);
        }
        else if (func == LED_ALTERNATING_M)
        {
            LED_ALTERNATING(param);
        }
        else if (func == LED_SHIFT_M)
        {
            LED_SHIFT(param);
        }
    }

    // ADC 제어
    else if (strcmp(hw, "ADC") == 0)
    {
        if (func == ADC_ON_M)
        {
            adc_flag = 1;
        }
        else if (func == ADC_OFF_M)
        {
            adc_flag = 0;
            adc_led_control = 0;
            led_write(0x00);
        }
        else if (func == ADC_LED_M)
        {
            if (adc_flag == 1)
            {
                adc_led_control = 1;
            }
        }
    }
}


// ----------------------------------------
// UART 수신 및 패킷 해석
// ----------------------------------------
void serial_receive()
{
    while (Serial.available() > 0)
    {
        byte data = Serial.read();

        // STX 확인
        if (decode == 0)
        {
            if (data == STX)
            {
                decode = 1;
                len_cnt = 0;
            }
        }

        // LEN 수신
        else if (decode == 1)
        {
            len = data;

            // H/W 문자열 최대 10바이트
            if (len >= 3 && len <= 12)
            {
                decode = 2;
            }
            else
            {
                decode = 0;
            }
        }

        // H/W 문자열 수신
        else if (decode == 2)
        {
            rx_hw[len_cnt] = (char)data;
            len_cnt++;

            if (len_cnt == len - 2)
            {
                rx_hw[len_cnt] = '\0';
                decode = 3;
            }
        }

        // FUNC 수신
        else if (decode == 3)
        {
            rx_func = data;
            decode = 4;
        }

        // PARAM 수신
        else if (decode == 4)
        {
            rx_param = data;
            decode = 5;
        }

        // ETX 확인
        else if (decode == 5)
        {
            decode = 0;

            if (data == ETX)
            {
                hex_command(rx_hw,
                            rx_func,
                            rx_param);
            }
        }
    }
}


// ----------------------------------------
// 초기 설정
// ----------------------------------------
void setup()
{
    for (int i = 0; i < LED_CNT; i++)
    {
        pinMode(FIRST_PIN + i, OUTPUT);
    }

    pinMode(POT_PIN, INPUT);

    led_write(0x00);

    // UART : 9600bps, 8N2
    Serial.begin(9600, SERIAL_8N2);
}


// ----------------------------------------
// 메인 루프
// ----------------------------------------
void loop()
{
    // UART 수신
    serial_receive();

    // ADC 동작
    if (adc_flag == 1)
    {
        if (millis() - last_adc >= 200)
        {
            last_adc = millis();

            unsigned int adc_value =
                analogRead(POT_PIN);

            // ADC 값 UART 출력
            char text[32];

            snprintf(text, sizeof(text),
                     "ADC: %u\r\n", adc_value);

            tx_str(text);

            // ADC 값을 LED에 표시
            if (adc_led_control == 1)
            {
                disp_led(adc_value);
            }
        }
    }
}
