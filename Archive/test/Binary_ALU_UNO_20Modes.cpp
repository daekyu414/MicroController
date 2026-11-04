
#include <Arduino.h>

// ==========================================
// 8-bit Binary ALU - Arduino UNO
// ==========================================

// LED : D2 ~ D9 (Active Low)
// SW1 : D10 (모드 변경)
// SW2 : D11 (A 저장)
// SW3 : D12 (B 저장 및 계산)
// POT : A1  (가변저항)
// CDS : A3  (조도센서)

// ==========================================
// 1. 핀 설정
// ==========================================

const int FIRST_LED = 2;
const int LED_COUNT = 8;

const int SW1 = 10;
const int SW2 = 11;
const int SW3 = 12;

const int POT_PIN = A1;
const int CDS_PIN = A3;

// ==========================================
// 2. 모드 설정
// ==========================================

const int MODE_COUNT = 20;

const char* modeNames[MODE_COUNT] = {
    "ADD", "SUB", "MUL", "DIV", "MOD",
    "AND", "OR", "XOR", "NOT",
    "SHL", "SHR", "ROL", "ROR",
    "POPCOUNT", "GRAY",
    "ADC_BAR", "DARK_COUNTER",
    "LED_ALT", "LED_SHIFT", "LED_BLINK"
};

int mode = 0;

// ==========================================
// 3. 연산 변수
// ==========================================

byte A = 15;
byte B = 3;
byte result = 0;

long exactResult = 0;

bool carryFlag = false;
bool borrowFlag = false;
bool overflowFlag = false;
bool divZeroFlag = false;

// ==========================================
// 4. ADC / 조도센서 변수
// ==========================================

int potADC = 0;
int cdsADC = 0;

int darkThreshold = 400;
int darkCount = 0;

bool darkState = false;

unsigned long lastADC = 0;

// ==========================================
// 5. 스위치 디바운싱
// ==========================================

const int swPins[3] = {SW1, SW2, SW3};

int lastRaw[3] = {HIGH, HIGH, HIGH};
int stableSW[3] = {HIGH, HIGH, HIGH};

unsigned long lastChange[3] = {0, 0, 0};

const int DEBOUNCE_MS = 40;

// ==========================================
// 6. LED 변수
// ==========================================

unsigned long lastMove = 0;
unsigned int periodMs = 200;

unsigned int frame = 0;

// LED 출력값이 같으면 재출력하지 않음
int previousLed = -1;

// A값 미리보기
bool previewActive = false;
byte previewValue = 0;
unsigned long previewStart = 0;

// ==========================================
// 7. UART 수신 변수
// ==========================================

char rxBuffer[20];
int rxIndex = 0;

bool receiving = false;

// UART LED 동작
bool effectRunning = false;

byte effectKind = 0;
byte effectSeed = 0;

unsigned int effectFrame = 0;
unsigned int effectTotal = 0;

unsigned long effectLastTime = 0;

// ==========================================
// 8. LED 출력 함수
// ==========================================

void led_write(byte value)
{
    // LED 상태가 같으면 출력 생략
    if (previousLed == value) {
        return;
    }

    previousLed = value;

    // LED는 Active Low
    // bit0 = D2, bit7 = D9

    for (int i = 0; i < LED_COUNT; i++) {

        if (value & (1 << i)) {
            digitalWrite(FIRST_LED + i, LOW);
        }
        else {
            digitalWrite(FIRST_LED + i, HIGH);
        }
    }
}

// ==========================================
// 9. 8비트 이진수 시리얼 출력
// ==========================================

void printBinary(byte value)
{
    for (int i = 7; i >= 0; i--) {

        if (value & (1 << i)) {
            Serial.print('1');
        }
        else {
            Serial.print('0');
        }
    }
}

// ==========================================
// 10. 16진수 문자열 변환
// 예: "0F" -> 15, "FF" -> 255
// 오류는 -1 반환
// ==========================================

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

// ==========================================
// 11. ALU 계산 함수
// ==========================================

void calculate()
{
    carryFlag = false;
    borrowFlag = false;
    overflowFlag = false;
    divZeroFlag = false;

    exactResult = 0;
    result = 0;

    // 시프트 횟수 : 0 ~ 7
    byte n = B & 0x07;

    switch (mode) {

        // 덧셈
        case 0:

            exactResult = (long)A + B;
            result = (byte)exactResult;

            if (exactResult > 255) {
                carryFlag = true;
            }

            if ((~(A ^ B) & (A ^ result) & 0x80) != 0) {
                overflowFlag = true;
            }

            break;

        // 뺄셈
        case 1:

            exactResult = (long)A - B;
            result = (byte)exactResult;

            if (exactResult < 0) {
                borrowFlag = true;
            }

            if (((A ^ B) & (A ^ result) & 0x80) != 0) {
                overflowFlag = true;
            }

            break;

        // 곱셈
        case 2:

            exactResult = (long)A * B;
            result = (byte)exactResult;

            if (exactResult > 255) {
                carryFlag = true;
            }

            break;

        // 나눗셈
        case 3:

            if (B == 0) {
                divZeroFlag = true;
            }
            else {
                result = A / B;
                exactResult = result;
            }

            break;

        // 나머지
        case 4:

            if (B == 0) {
                divZeroFlag = true;
            }
            else {
                result = A % B;
                exactResult = result;
            }

            break;

        // AND
        case 5:

            result = A & B;
            exactResult = result;
            break;

        // OR
        case 6:

            result = A | B;
            exactResult = result;
            break;

        // XOR
        case 7:

            result = A ^ B;
            exactResult = result;
            break;

        // NOT
        case 8:

            result = ~A;
            exactResult = result;
            break;

        // 왼쪽 시프트
        case 9:

            exactResult = (long)A << n;
            result = (byte)exactResult;

            if (exactResult > 255) {
                carryFlag = true;
            }

            break;

        // 오른쪽 시프트
        case 10:

            result = A >> n;
            exactResult = result;
            break;

        // 왼쪽 회전
        case 11:

            if (n == 0) {
                result = A;
            }
            else {
                result = (A << n) | (A >> (8 - n));
            }

            exactResult = result;
            break;

        // 오른쪽 회전
        case 12:

            if (n == 0) {
                result = A;
            }
            else {
                result = (A >> n) | (A << (8 - n));
            }

            exactResult = result;
            break;

        // 켜져 있는 비트 개수
        case 13:

            result = 0;

            for (int i = 0; i < 8; i++) {

                if (A & (1 << i)) {
                    result++;
                }
            }

            exactResult = result;
            break;

        // Gray Code 변환
        case 14:

            result = A ^ (A >> 1);
            exactResult = result;
            break;

        // 나머지 모드
        default:

            result = A;
            exactResult = A;
            break;
    }
}

// ==========================================
// 12. 결과 출력
// ==========================================

void printStatus()
{
    Serial.println();

    Serial.print(F("MODE : "));
    Serial.print(mode);
    Serial.print(F(" - "));
    Serial.println(modeNames[mode]);

    Serial.print(F("A = "));
    Serial.print(A);

    Serial.print(F(" (0x"));
    Serial.print(A, HEX);
    Serial.println(F(")"));

    Serial.print(F("B = "));
    Serial.print(B);

    Serial.print(F(" (0x"));
    Serial.print(B, HEX);
    Serial.println(F(")"));

    Serial.print(F("RESULT = "));
    Serial.println(result);

    Serial.print(F("BINARY = "));
    printBinary(result);
    Serial.println();

    Serial.print(F("EXACT = "));
    Serial.println(exactResult);

    Serial.print(F("FLAGS : C="));
    Serial.print(carryFlag);

    Serial.print(F(" BOR="));
    Serial.print(borrowFlag);

    Serial.print(F(" V="));
    Serial.print(overflowFlag);

    Serial.print(F(" Z="));
    Serial.print(result == 0);

    Serial.print(F(" DIV0="));
    Serial.println(divZeroFlag);
}

// ==========================================
// 13. 모드 변경
// ==========================================

void setMode(int nextMode)
{
    if (nextMode < 0 || nextMode >= MODE_COUNT) {
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

// ==========================================
// 14. 시리얼 LED 동작
// ==========================================

void startEffect(byte kind, byte count, byte seed)
{
    if (count == 0 || count > 20) {
        Serial.println(F("COUNT ERROR"));
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

// ==========================================
// 15. 시리얼 명령 처리
//
// <M00> : 덧셈 모드
// <M02> : 곱셈 모드
// <A0F> : A = 15
// <B03> : B = 3
// <C>   : 계산
// <S>   : 상태 출력
// <R>   : 초기화
//
// <13>  : 전체 점멸 3회
// <2AA> : 교대 점멸
// <45>  : 왕복 시프트 5회
// ==========================================

void processPacket(char* packet)
{
    char cmd = packet[0];
    long data = hex_to_number(packet + 1);

    // 상태 확인
    if (cmd == 'S' && packet[1] == '\0') {
        printStatus();
        return;
    }

    // 다시 계산
    if (cmd == 'C' && packet[1] == '\0') {
        calculate();
        printStatus();
        return;
    }

    // 초기화
    if (cmd == 'R' && packet[1] == '\0') {

        A = 0;
        B = 0;
        darkCount = 0;

        setMode(0);
        return;
    }

    // 모드 설정
    if (cmd == 'M' && data >= 0 && data < MODE_COUNT) {

        setMode((int)data);
        return;
    }

    // A 설정
    if (cmd == 'A' && data >= 0 && data <= 255) {

        A = (byte)data;

        calculate();
        printStatus();
        return;
    }

    // B 설정
    if (cmd == 'B' && data >= 0 && data <= 255) {

        B = (byte)data;

        calculate();
        printStatus();
        return;
    }

    // LED 주기 설정
    if (cmd == 'P' && data >= 50 && data <= 1000) {

        periodMs = (unsigned int)data;

        Serial.print(F("PERIOD = "));
        Serial.println(periodMs);
        return;
    }

    // 조도센서 임계값 설정
    if (cmd == 'T' && data >= 30 && data <= 990) {

        darkThreshold = (int)data;

        darkState = (cdsADC < darkThreshold);

        Serial.print(F("THRESHOLD = "));
        Serial.println(darkThreshold);
        return;
    }

    // 전체 점멸
    if (cmd == '1' && data >= 1 && data <= 20) {

        startEffect(1, (byte)data, 0);
        return;
    }

    // 교대 점멸
    if (cmd == '2' && strlen(packet) == 3 &&
        data >= 0 && data <= 255) {

        startEffect(2, 3, (byte)data);
        return;
    }

    // 왕복 시프트
    if (cmd == '4' && data >= 1 && data <= 20) {

        startEffect(4, (byte)data, 0);
        return;
    }

    Serial.println(F("UNKNOWN COMMAND"));
}

// ==========================================
// 16. UART 수신
// ==========================================

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

            if (rxIndex < (int)sizeof(rxBuffer) - 1) {

                rxBuffer[rxIndex] = c;
                rxIndex++;
            }
            else {

                receiving = false;

                Serial.println(F("PACKET ERROR"));
            }
        }
    }
}

// ==========================================
// 17. 스위치 입력 처리
// 디바운싱 + 눌림 순간 감지
// ==========================================

void handleButton(int sw, unsigned long now)
{
    // SW1 : 모드 변경
    if (sw == 0) {

        int nextMode = mode + 1;

        if (nextMode >= MODE_COUNT) {
            nextMode = 0;
        }

        setMode(nextMode);
    }

    // SW2 : A 저장
    else if (sw == 1) {

        // 조도 카운터 모드에서는 초기화
        if (mode == 16) {

            darkCount = 0;

            Serial.println(F("DARK COUNTER RESET"));
        }
        else {

            A = (byte)(potADC / 4);

            calculate();

            previewValue = A;
            previewActive = true;
            previewStart = now;

            Serial.println(F("A SAVED"));

            printStatus();
        }
    }

    // SW3 : B 저장 후 계산
    else if (sw == 2) {

        B = (byte)(potADC / 4);

        calculate();

        previewActive = false;

        Serial.println(F("B SAVED / CALCULATED"));

        printStatus();
    }
}

void buttonTask(unsigned long now)
{
    for (int i = 0; i < 3; i++) {

        int raw = digitalRead(swPins[i]);

        // 입력값이 바뀌면 시간 기록
        if (raw != lastRaw[i]) {
            lastChange[i] = now;
        }

        // 40ms 동안 같은 상태이면 확정
        if (now - lastChange[i] >= DEBOUNCE_MS) {

            if (raw != stableSW[i]) {

                stableSW[i] = raw;

                // HIGH -> LOW : 누르는 순간
                if (raw == LOW) {
                    handleButton(i, now);
                }
            }
        }

        lastRaw[i] = raw;
    }
}

// ==========================================
// 18. ADC / 조도센서
// ==========================================

void sensorTask(unsigned long now)
{
    // 50ms 주기로 측정
    if (now - lastADC < 50) {
        return;
    }

    lastADC = now;

    potADC = analogRead(POT_PIN);

    cdsADC = analogRead(CDS_PIN);

    // 밝음 -> 어두움
    if (darkState == false &&
        cdsADC < darkThreshold - 20) {

        darkState = true;

        // 8비트 카운터 : 255 -> 0
        darkCount++;

        if (darkCount > 255) {
            darkCount = 0;
        }

        Serial.print(F("DARK COUNT = "));
        Serial.println(darkCount);
    }

    // 어두움 -> 밝음
    else if (darkState == true &&
             cdsADC > darkThreshold + 20) {

        darkState = false;
    }
}

// ==========================================
// 19. LED 모드별 패턴
// ==========================================

byte getLedPattern(unsigned long now)
{
    // ADC LED 막대 그래프
    if (mode == 15) {

        int level = 1 + ((long)potADC * 8 / 1024);

        byte value = 0;

        for (int i = 0; i < level; i++) {
            value |= (1 << i);
        }

        return value;
    }

    // 조도센서 카운터
    if (mode == 16) {
        return (byte)darkCount;
    }

    // LED 교대 점멸
    if (mode == 17) {

        if (frame % 2 == 0) {
            return A;
        }
        else {
            return ~A;
        }
    }

    // LED 왕복 시프트
    if (mode == 18) {

        int pos = frame % 15;

        if (pos > 7) {
            pos = 14 - pos;
        }

        return (1 << pos);
    }

    // LED 전체 점멸
    if (mode == 19) {

        if (frame % 2 == 0) {
            return 0xFF;
        }
        else {
            return 0x00;
        }
    }

    // 0으로 나누기 오류
    if (divZeroFlag == true) {

        if ((now / 250) % 2 == 0) {
            return 0xFF;
        }
        else {
            return 0x00;
        }
    }

    // 연산 결과
    return result;
}

// ==========================================
// 20. UART LED 효과
// ==========================================

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

// ==========================================
// 21. LED 출력 관리
// ==========================================

void displayTask(unsigned long now)
{
    // UART LED 명령 우선 실행
    if (effectRunning == true) {

        if (now - effectLastTime >= periodMs) {

            effectLastTime = now;

            if (effectFrame < effectTotal) {

                led_write(getEffectPattern());

                effectFrame++;
            }
            else {

                effectRunning = false;

                Serial.println(F("EFFECT FINISHED"));
            }
        }

        return;
    }

    // SW2로 A 저장 시 0.5초 표시
    if (previewActive == true) {

        if (now - previewStart < 500) {

            led_write(previewValue);
            return;
        }

        previewActive = false;
    }

    // 애니메이션 시간 관리
    if (mode == 17 || mode == 18 || mode == 19) {

        if (now - lastMove >= periodMs) {

            lastMove = now;
            frame++;
        }
    }

    led_write(getLedPattern(now));
}

// ==========================================
// 22. SETUP
// ==========================================

void setup()
{
    // LED 출력 설정
    for (int i = 0; i < LED_COUNT; i++) {

        pinMode(FIRST_LED + i, OUTPUT);
    }

    led_write(0x00);

    // 스위치 입력 설정
    for (int i = 0; i < 3; i++) {

        pinMode(swPins[i], INPUT_PULLUP);

        lastRaw[i] = digitalRead(swPins[i]);
        stableSW[i] = lastRaw[i];
    }

    // UART 초기화
    Serial.begin(9600);

    // ADC 초기값 읽기
    potADC = analogRead(POT_PIN);
    cdsADC = analogRead(CDS_PIN);

    darkState = (cdsADC < darkThreshold);

    calculate();

    Serial.println();
    Serial.println(F("============================"));
    Serial.println(F("  8-BIT BINARY ALU SYSTEM"));
    Serial.println(F("  ARDUINO UNO - 20 MODES"));
    Serial.println(F("============================"));

    Serial.println(F("SW1 : MODE CHANGE"));
    Serial.println(F("SW2 : SAVE A"));
    Serial.println(F("SW3 : SAVE B + CALCULATE"));

    Serial.println(F("UART : <Mxx><Axx><Bxx><S>"));

    printStatus();
}

// ==========================================
// 23. MAIN LOOP
// ==========================================

void loop()
{
    unsigned long now = millis();

    uartTask();

    buttonTask(now);

    sensorTask(now);

    displayTask(now);
}
