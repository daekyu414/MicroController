
#include <Arduino.h>
#include <avr/io.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

const int POT_PIN = A1; // PC1 - ADC1

// --- 4차시에서 만든 Printf -----------------------

void tx_char(char txChar) {
    Serial.write(txChar);
}

void tx_str(char* txStr, int len) {
    for (int i = 0; i < len; i++) {
        tx_char(txStr[i]);
    }
}

void Printf(const char* fmt, ...) {
    char text[96];
    va_list args;

    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);

    tx_str(text, strlen(text));
}

// --- ADC 초기화 (레지스터 직접 설정) ----------------

void adc_init() {

    // A1(PC1)을 입력으로 설정
    DDRC &= ~(1 << DDC1);

    // PC1의 내부 풀업 저항 비활성화
    PORTC &= ~(1 << PORTC1);

    // 기준 전압 AVcc(5V), ADC1 채널 선택
    // REFS0 = 1, MUX0 = 1
    ADMUX = (1 << REFS0) | (1 << MUX0);

    // ADC 활성화
    // 분주비 128 설정
    // 16MHz / 128 = 125kHz
    ADCSRA = (1 << ADEN)
           | (1 << ADPS2)
           | (1 << ADPS1)
           | (1 << ADPS0);
}

// --- ADC 값 읽기 ----------------------------------

int adc_read() {

    // ADSC = 1로 설정하여 ADC 변환 시작
    ADCSRA |= (1 << ADSC);

    // 변환이 완료될 때까지 대기
    // 변환 완료 시 ADSC가 자동으로 0이 됨
    while (ADCSRA & (1 << ADSC)) {
    }

    // ADC 결과 레지스터 읽기 (0~1023)
    return ADC;
}

void setup() {
    Serial.begin(9600);

    // ADC 초기화
    adc_init();
}

void loop() {

    // ADC1(A1) 값 읽기
    int value = adc_read();

    Printf("ADC: %d\n", value);

    delay(500);
}
