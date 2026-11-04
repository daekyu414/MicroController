
#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// ==================================================
// 8-bit Binary ALU - 1~5차시 종합 응용
//
// LED : D2 ~ D9  (Active Low)
// SW1 : D10      (MODE 변경)
// SW2 : D11      (A 저장)
// SW3 : D12      (B 저장 + 계산)
// POT : A1       (숫자 입력)
// CDS : A3       (조도센서)
// UART: 9600
// ==================================================

// ==================================================
// 1. MODE 정의
// ==================================================

#define MODE_ADD        0
#define MODE_SUB        1
#define MODE_MUL        2
#define MODE_DIV        3
#define MODE_MOD        4
#define MODE_AND        5
#define MODE_OR         6
#define MODE_XOR        7
#define MODE_NOT        8
#define MODE_SHL        9
#define MODE_SHR        10
#define MODE_ROL        11
#define MODE_ROR        12
#define MODE_POPCOUNT   13
#define MODE_GRAY       14
#define MODE_ADC_BAR    15
#define MODE_DARK       16
#define MODE_ALT        17
#define MODE_SHIFT      18
#define MODE_BLINK      19

#define MODE_COUNT      20

const char* modeNames[MODE_COUNT] = {
    "ADD", "SUB", "MUL", "DIV", "MOD",
    "AND", "OR", "XOR", "NOT",
    "SHL", "SHR", "ROL", "ROR",
    "POPCOUNT", "GRAY",
    "ADC_BAR", "DARK_COUNTER",
    "LED_ALT", "LED_SHIFT", "LED_BLINK"
};

byte mode = MODE_ADD;

// ==================================================
// 2. 연산기 변수
// ==================================================

byte A = 15;
byte B = 3;
byte result = 0;

long exactResult = 0;

bool carryFlag = false;
bool borrowFlag = false;
bool overflowFlag = false;
bool divZeroFlag = false;

// ==================================================
// 3. ADC / 조도센서 변수
// ==================================================

unsigned int potADC = 0;
unsigned int cdsADC = 0;

unsigned int darkThreshold = 400;

bool darkState = false;
byte darkCount = 0;

unsigned long lastADC = 0;

// ==================================================
// 4. 인터럽트 변수
// ==================================================

volatile byte sw_event = 0;
volatile byte prev_sw = 0x07;

unsigned long last_press[3] = {0, 0, 0};

// ==================================================
// 5. LED 제어 변수
// ==================================================

unsigned int periodMs = 200;

unsigned long lastMove = 0;
unsigned int frame = 0;

bool previewActive = false;
byte previewValue = 0;
unsigned long previewStart = 0;

// UART로 실행하는 임시 LED 동작
bool effectRunning = false;

byte effectKind = 0;
byte effectSeed = 0;

unsigned int effectFrame = 0;
unsigned int effectTotal = 0;

unsigned long effectLastTime = 0;

// ==================================================
// 6. UART 수신 변수
// ==================================================

char rxBuffer[20];

byte rxIndex = 0;
bool receiving = false;

// ==================================================
// 7. LED 출력 (PORT 레지스터)
// ==================================================

void led_write(byte value)
{
    // Active Low
    // 1을 출력하고 싶으면 실제 핀에는 0 출력

    value = ~value;

    // D2 ~ D7 = PD2 ~ PD7
    // PD0, PD1은 UART이므로 보존

    PORTD = (PORTD & 0x03)
          | ((value & 0x3F) << 2);

    // D8 ~ D9 = PB0 ~ PB1
    // PB2 ~ PB4 스위치 설정 보존

    PORTB = (PORTB & 0xFC)
          | ((value >> 6) & 0x03);
}

// ==================================================
// 8. ADC 초기화 및 읽기
// ==================================================

void adc_init()
{
    // AVcc 기준전압
    ADMUX = (1 << REFS0);

    // ADC 활성화, 분주비 128
    ADCSRA = (1 << ADEN)
           | (1 << ADPS2)
           | (1 << ADPS1)
           | (1 << ADPS0);
}

unsigned int adc_read(byte channel)
{
    // ADC 채널 선택
    ADMUX = (ADMUX & 0xF0) | channel;

    // 변환 시작
    ADCSRA |= (1 << ADSC);

    // 변환 완료까지 대기
    while (ADCSRA & (1 << ADSC)) {
    }

    return ADC;
}

// ==================================================
// 9. UART 송신
// ==================================================

void tx_char(char c)
{
    Serial.write(c);
}

void tx_str(char* str, int len)
{
    for (int i = 0; i < len; i++) {
        tx_char(str[i]);
    }
}

void Printf(const char* fmt, ...)
{
    char text[160];

    va_list args;

    va_start(args, fmt);

    vsnprintf(text, sizeof(text), fmt, args);

    va_end(args);

    tx_str(text, strlen(text));
}

// ==================================================
// 10. 16진수 문자열 변환
//
// 예시:
// "0F" -> 15
// "FF" -> 255
// 잘못된 입력 -> -1
// ==================================================

long hex_to_number(const char* str)
{
    long value = 0;
    int count = 0;

    if (*str == '\0') {
        return -1;
    }

    while (*str != '\0') {

        char c = *str;
        int digit = 0;

        if (c >= '0' && c <= '9') {
            digit = c - '0';
        }
        else if (c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        }
        else if (c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
        }
        else {
            return -1;
        }

        count++;

        if (count > 4) {
            return -1;
        }

        value = (value << 4) | digit;

        str++;
    }

    return value;
}

// ==================================================
// 11. 8비트 ALU 연산
// ==================================================

void calculate()
{
    carryFlag = false;
    borrowFlag = false;
    overflowFlag = false;
    divZeroFlag = false;

    exactResult = 0;
    result = 0;

    // 시프트 횟수는 0 ~ 7
    byte n = B & 0x07;

    switch (mode) {

        // ------------------------------
        // 덧셈
        // ------------------------------

        case MODE_ADD:

            exactResult = (long)A + B;

            result = (byte)exactResult;

            if (exactResult > 255) {
                carryFlag = true;
            }

            // signed 8-bit overflow
            if ((~(A ^ B) & (A ^ result) & 0x80) != 0) {
                overflowFlag = true;
            }

            break;

        // ------------------------------
        // 뺄셈
        // ------------------------------

        case MODE_SUB:

            exactResult = (long)A - B;

            result = (byte)exactResult;

            if (exactResult < 0) {
                borrowFlag = true;
            }

            if (((A ^ B) & (A ^ result) & 0x80) != 0) {
                overflowFlag = true;
            }

            break;

        // ------------------------------
        // 곱셈
        // ------------------------------

        case MODE_MUL:

            exactResult = (long)A * B;

            result = (byte)exactResult;

            if (exactResult > 255) {
                carryFlag = true;
            }

            break;

        // ------------------------------
        // 나눗셈
        // ------------------------------

        case MODE_DIV:

            if (B == 0) {
                divZeroFlag = true;
            }
            else {
                result = A / B;
                exactResult = result;
            }

            break;

        // ------------------------------
        // 나머지
        // ------------------------------

        case MODE_MOD:

            if (B == 0) {
                divZeroFlag = true;
            }
            else {
                result = A % B;
                exactResult = result;
            }

            break;

        // ------------------------------
        // AND
        // ------------------------------

        case MODE_AND:

            result = A & B;
            exactResult = result;

            break;

        // ------------------------------
        // OR
        // ------------------------------

        case MODE_OR:

            result = A | B;
            exactResult = result;

            break;

        // ------------------------------
        // XOR
        // ------------------------------

        case MODE_XOR:

            result = A ^ B;
            exactResult = result;

            break;

        // ------------------------------
        // NOT
        // ------------------------------

        case MODE_NOT:

            result = ~A;
            exactResult = result;

            break;

        // ------------------------------
        // 왼쪽 시프트
        // ------------------------------

        case MODE_SHL:

            exactResult = (long)A << n;

            result = (byte)exactResult;

            if (exactResult > 255) {
                carryFlag = true;
            }

            break;

        // ------------------------------
        // 오른쪽 시프트
        // ------------------------------

        case MODE_SHR:

            result = A >> n;
            exactResult = result;

            break;

        // ------------------------------
        // 왼쪽 회전
        // ------------------------------

        case MODE_ROL:

            if (n == 0) {
                result = A;
            }
            else {
                result = (byte)(
                    (A << n) | (A >> (8 - n))
                );
            }

            exactResult = result;

            break;

        // ------------------------------
        // 오른쪽 회전
        // ------------------------------

        case MODE_ROR:

            if (n == 0) {
                result = A;
            }
            else {
                result = (byte)(
                    (A >> n) | (A << (8 - n))
                );
            }

            exactResult = result;

            break;

        // ------------------------------
        // 1비트 개수 계산
        // ------------------------------

        case MODE_POPCOUNT:

            result = 0;

            for (int i = 0; i < 8; i++) {

                if (A & (1 << i)) {
                    result++;
                }
            }

            exactResult = result;

            break;

        // ------------------------------
        // Gray Code
        // ------------------------------

        case MODE_GRAY:

            result = A ^ (A >> 1);
            exactResult = result;

            break;

        // ------------------------------
        // 나머지 모드
        // ------------------------------

        default:

            result = A;
            exactResult = A;

            break;
    }
}

// ==================================================
// 12. 현재 상태 출력
// ==================================================

void printStatus()
{
    Printf(
        "\r\nMODE=%d %s\r\n",
        (int)mode,
        modeNames[mode]
    );

    Printf(
        "A=%u (0x%02X), B=%u (0x%02X)\r\n",
        (unsigned int)A,
        (unsigned int)A,
        (unsigned int)B,
        (unsigned int)B
    );

    Printf(
        "RESULT=%u (0x%02X), EXACT=%ld\r\n",
        (unsigned int)result,
        (unsigned int)result,
        exactResult
    );

    Printf(
        "C=%d BOR=%d V=%d Z=%d DIV0=%d\r\n",
        (int)carryFlag,
        (int)borrowFlag,
        (int)overflowFlag,
        (int)(result == 0),
        (int)divZeroFlag
    );

    Printf(
        "ADC=%u CDS=%u DARK=%u\r\n",
        potADC,
        cdsADC,
        (unsigned int)darkCount
    );
}

// ==================================================
// 13. MODE 변경
// ==================================================

void setMode(byte nextMode)
{
    if (nextMode >= MODE_COUNT) {
        return;
    }

    mode = nextMode;

    frame = 0;
    lastMove = millis();

    effectRunning = false;
    previewActive = false;

    calculate();

    printStatus();
}

// ==================================================
// 14. UART 임시 LED 동작
// ==================================================

void startEffect(byte kind, byte count, byte seed)
{
    if (count == 0 || count > 20) {
        Printf("ERROR: COUNT 1~20\r\n");
        return;
    }

    effectKind = kind;
    effectSeed = seed;

    effectFrame = 0;

    if (kind == 4) {
        effectTotal = (unsigned int)count * 15;
    }
    else {
        effectTotal = (unsigned int)count * 2;
    }

    effectLastTime = millis() - periodMs;

    effectRunning = true;
}

// ==================================================
// 15. UART 명령 처리
//
// <M02> -> MODE 2 (MUL)
// <A0F> -> A = 15
// <B03> -> B = 3
// <C>   -> 계산
// <S>   -> 상태 확인
// <R>   -> 초기화
//
// PPT 응용:
// <13>  -> 전체 점멸 3회
// <2AA> -> AA/55 교대
// <45>  -> 왕복 시프트 5회
// ==================================================

void processPacket(char* packet)
{
    char cmd = packet[0];

    long data = hex_to_number(packet + 1);

    if (cmd == 'S' && packet[1] == '\0') {
        printStatus();
        return;
    }

    if (cmd == 'C' && packet[1] == '\0') {
        calculate();
        printStatus();
        return;
    }

    if (cmd == 'R' && packet[1] == '\0') {
        A = 0;
        B = 0;
        darkCount = 0;

        setMode(MODE_ADD);
        return;
    }

    // 모드 변경
    if (cmd == 'M' && data >= 0 && data < MODE_COUNT) {
        setMode((byte)data);
        return;
    }

    // A 저장
    if (cmd == 'A' && data >= 0 && data <= 255) {

        A = (byte)data;

        calculate();
        printStatus();

        return;
    }

    // B 저장
    if (cmd == 'B' && data >= 0 && data <= 255) {

        B = (byte)data;

        calculate();
        printStatus();

        return;
    }

    // LED 동작 주기
    if (cmd == 'P' && data >= 50 && data <= 1000) {

        periodMs = (unsigned int)data;

        Printf("PERIOD=%u ms\r\n", periodMs);

        return;
    }

    // 조도센서 임계값
    if (cmd == 'T' && data >= 30 && data <= 990) {

        darkThreshold = (unsigned int)data;

        darkState = (cdsADC < darkThreshold);

        Printf("THRESHOLD=%u\r\n", darkThreshold);

        return;
    }

    // LED 전체 점멸
    if (cmd == '1' && data >= 1 && data <= 20) {

        startEffect(1, (byte)data, 0);
        return;
    }

    // LED 교대
    if (cmd == '2' && strlen(packet) == 3 &&
        data >= 0 && data <= 255) {

        startEffect(2, 3, (byte)data);
        return;
    }

    // LED 왕복 시프트
    if (cmd == '4' && data >= 1 && data <= 20) {

        startEffect(4, (byte)data, 0);
        return;
    }

    Printf("UNKNOWN COMMAND: <%s>\r\n", packet);
}

// ==================================================
// 16. UART 수신 FSM
// ==================================================

void uartTask()
{
    while (Serial.available() > 0) {

        char c = Serial.read();

        // STX
        if (c == '<') {

            receiving = true;
            rxIndex = 0;
        }

        // ETX
        else if (c == '>') {

            if (receiving) {

                rxBuffer[rxIndex] = '\0';

                if (rxIndex > 0) {
                    processPacket(rxBuffer);
                }
            }

            receiving = false;
        }

        // DATA
        else if (receiving) {

            if (rxIndex < sizeof(rxBuffer) - 1) {

                rxBuffer[rxIndex] = c;
                rxIndex++;
            }
            else {

                receiving = false;
                Printf("PACKET ERROR\r\n");
            }
        }
    }
}

// ==================================================
// 17. PCINT 인터럽트
//
// D10 = PB2
// D11 = PB3
// D12 = PB4
// ==================================================

ISR(PCINT0_vect)
{
    // 현재 스위치 상태 읽기
    byte now = (PINB >> 2) & 0x07;

    // 이전 HIGH -> 현재 LOW
    byte pressed = prev_sw & (~now);

    // 3비트만 사용
    pressed &= 0x07;

    // 이벤트 저장
    sw_event |= pressed;

    prev_sw = now;
}

// ==================================================
// 18. 스위치 이벤트 처리
// ==================================================

void buttonTask(unsigned long now)
{
    byte event;

    noInterrupts();

    event = sw_event;
    sw_event = 0;

    interrupts();

    for (int i = 0; i < 3; i++) {

        if ((event & (1 << i)) == 0) {
            continue;
        }

        // 아직 눌려 있는지 확인
        if (PINB & (1 << (PB2 + i))) {
            continue;
        }

        // 디바운싱
        if (now - last_press[i] < 70) {
            continue;
        }

        last_press[i] = now;

        // SW1 : 모드 변경
        if (i == 0) {

            byte next = mode + 1;

            if (next >= MODE_COUNT) {
                next = 0;
            }

            setMode(next);
        }

        // SW2 : A 저장
        else if (i == 1) {

            if (mode == MODE_DARK) {

                darkCount = 0;

                Printf("DARK COUNTER RESET\r\n");
            }

            else {

                A = (byte)(potADC >> 2);

                calculate();

                previewValue = A;
                previewActive = true;
                previewStart = now;

                Printf("A SAVED\r\n");

                printStatus();
            }
        }

        // SW3 : B 저장 후 계산
        else if (i == 2) {

            B = (byte)(potADC >> 2);

            calculate();

            previewActive = false;

            Printf("B SAVED / CALCULATED\r\n");

            printStatus();
        }
    }
}

// ==================================================
// 19. ADC 및 조도센서 처리
// ==================================================

void sensorTask(unsigned long now)
{
    // 50ms 간격으로 ADC 읽기
    if (now - lastADC < 50) {
        return;
    }

    lastADC = now;

    // 가변저항
    potADC = adc_read(1);

    // 조도센서
    cdsADC = adc_read(3);

    // 밝은 상태 -> 어두운 상태
    if (darkState == false &&
        cdsADC < darkThreshold - 20) {

        darkState = true;

        darkCount++;

        Printf(
            "DARK COUNT=%u\r\n",
            (unsigned int)darkCount
        );
    }

    // 다시 밝아질 때
    else if (darkState == true &&
             cdsADC > darkThreshold + 20) {

        darkState = false;
    }
}

// ==================================================
// 20. LED 패턴 계산
// ==================================================

byte getLedPattern(unsigned long now)
{
    // ADC 막대 그래프
    if (mode == MODE_ADC_BAR) {

        int level = 1 + (potADC * 8L / 1024);

        byte value = 0;

        for (int i = 0; i < level; i++) {
            value |= (1 << i);
        }

        return value;
    }

    // 어두워진 횟수를 이진수로 출력
    if (mode == MODE_DARK) {
        return darkCount;
    }

    // 교대 점멸
    if (mode == MODE_ALT) {

        if (frame % 2 == 0) {
            return A;
        }
        else {
            return ~A;
        }
    }

    // 왕복 시프트
    if (mode == MODE_SHIFT) {

        int pos = frame % 15;

        if (pos > 7) {
            pos = 14 - pos;
        }

        return (1 << pos);
    }

    // 전체 점멸
    if (mode == MODE_BLINK) {

        if (frame % 2 == 0) {
            return 0xFF;
        }
        else {
            return 0x00;
        }
    }

    // 0으로 나눌 때 오류 표시
    if (divZeroFlag == true) {

        if ((now / 250) % 2 == 0) {
            return 0xFF;
        }
        else {
            return 0x00;
        }
    }

    // 기본 : 계산 결과
    return result;
}

// ==================================================
// 21. 임시 LED 동작
// ==================================================

byte getEffectPattern()
{
    // 전체 점멸
    if (effectKind == 1) {

        if (effectFrame % 2 == 0) {
            return 0xFF;
        }
        else {
            return 0x00;
        }
    }

    // 교대 점멸
    if (effectKind == 2) {

        if (effectFrame % 2 == 0) {
            return effectSeed;
        }
        else {
            return ~effectSeed;
        }
    }

    // 왕복 시프트
    if (effectKind == 4) {

        int pos = effectFrame % 15;

        if (pos > 7) {
            pos = 14 - pos;
        }

        return (1 << pos);
    }

    return 0;
}

// ==================================================
// 22. LED 출력 Task
// ==================================================

void displayTask(unsigned long now)
{
    // UART 명령으로 실행한 LED 동작
    if (effectRunning == true) {

        if (now - effectLastTime >= periodMs) {

            effectLastTime = now;

            if (effectFrame < effectTotal) {

                led_write(getEffectPattern());

                effectFrame++;
            }
            else {

                effectRunning = false;

                led_write(0x00);

                Printf("EFFECT FINISHED\r\n");
            }
        }

        return;
    }

    // SW2를 누르면 A를 0.5초 표시
    if (previewActive == true) {

        if (now - previewStart < 500) {

            led_write(previewValue);
            return;
        }

        previewActive = false;
    }

    // 애니메이션 모드
    if (mode == MODE_ALT ||
        mode == MODE_SHIFT ||
        mode == MODE_BLINK) {

        if (now - lastMove >= periodMs) {

            lastMove = now;

            frame++;
        }
    }

    // LED에 결과 출력
    led_write(getLedPattern(now));
}

// ==================================================
// 23. SETUP
// ==================================================

void setup()
{
    // D2 ~ D7 : OUTPUT
    DDRD |= 0xFC;

    // D8 ~ D9 : OUTPUT
    DDRB |= 0x03;

    // D10 ~ D12 : INPUT
    DDRB &= ~0x1C;

    // 스위치 내부 풀업
    PORTB |= 0x1C;

    led_write(0x00);

    // UART 시작
    Serial.begin(9600);

    // ADC 초기화
    adc_init();

    potADC = adc_read(1);
    cdsADC = adc_read(3);

    darkState = (cdsADC < darkThreshold);

    // 스위치 현재 상태
    prev_sw = (PINB >> 2) & 0x07;

    // PCINT2, 3, 4 활성화
    PCMSK0 |= (1 << PCINT2)
            | (1 << PCINT3)
            | (1 << PCINT4);

    // 남아 있는 인터럽트 플래그 제거
    PCIFR = (1 << PCIF0);

    // PCINT0 그룹 활성화
    PCICR |= (1 << PCIE0);

    for (int i = 0; i < 3; i++) {
        last_press[i] = millis() - 70;
    }

    // 전역 인터럽트 허용
    sei();

    calculate();

    Printf("\r\n============================\r\n");
    Printf("  8-BIT BINARY ALU SYSTEM\r\n");
    Printf("  Arduino UNO / ATmega328P\r\n");
    Printf("  UART 9600\r\n");
    Printf("============================\r\n");

    Printf("SW1=MODE\r\n");
    Printf("SW2=SAVE A\r\n");
    Printf("SW3=SAVE B + CALCULATE\r\n");

    printStatus();
}

// ==================================================
// 24. MAIN LOOP
// ==================================================

void loop()
{
    unsigned long now = millis();

    uartTask();

    buttonTask(now);

    sensorTask(now);

    displayTask(now);
}
