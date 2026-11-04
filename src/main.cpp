
#include <Arduino.h>
#include <avr/interrupt.h>

// ========================================
// 1. 핀 설정 및 프로토콜
// ========================================
#define FIRST_LED 2
#define LED_CNT 8

#define FIRST_SW 10
#define SW_CNT 4

#define POT_PIN A1
#define CDS_PIN A2

#define STX 0x02
#define ETX 0x03
#define MAX_DATA 16

#define MODE_IDLE 0
#define MODE_ALL 1
#define MODE_ALT 2
#define MODE_SHIFT 3

// PC -> 보드
#define CMD_PING          0x01
#define CMD_GET_STATUS    0x02
#define CMD_LED_SET       0x10
#define CMD_LED_RUN       0x11
#define CMD_LED_STOP      0x12
#define CMD_SET_PERIOD    0x13
#define CMD_SET_SPEED_SRC 0x14
#define CMD_ADC_READ      0x20
#define CMD_ADC_STREAM    0x21

// 보드 -> PC
#define CMD_ACK        0x80
#define CMD_ADC_DATA   0xA0
#define CMD_STATUS     0xB0
#define CMD_EVT_SWITCH 0x90
#define CMD_EVT_LED    0x91
#define CMD_EVT_MODE   0x92
#define CMD_EVT_SPEED  0x93

// ========================================
// 2. 전역 변수
// ========================================

// LED
byte ledPattern = 0;
byte ledMode = MODE_IDLE;
byte ledRepeat = 0;
byte ledStart = 0xAA;
byte ledStep = 0;
unsigned int ledCycles = 0;

unsigned long lastFrame = 0;
unsigned int framePeriod = 500;

// 속도
byte speedSource = 1;       // 부팅 기본: 가변저항
unsigned int pcPeriod = 500;
unsigned int period = 500;
unsigned int potValue = 0;

unsigned long lastPotRead = 0;
unsigned long lastSpeedEvent = 0;
unsigned int reportedPeriod = 500;
bool speedEventSent = false;

// ADC 스트림 (채널 0=가변저항, 1=조도)
bool streamOn[2] = {false, false};
unsigned int streamInterval[2] = {1000, 1000};
unsigned long lastStream[2] = {0, 0};

// 스위치 인터럽트 및 디바운스
volatile byte swInterrupt = 0;
byte stableSwitchPins = 0;

bool swDebouncing = false;
unsigned long lastSwitchChange = 0;

const unsigned long DEBOUNCE_MS = 30;

// UART 수신 파서
byte rxState = 0;
byte rxCmd = 0;
byte rxLen = 0;
byte rxIndex = 0;
byte rxData[MAX_DATA];

unsigned long lastRxTime = 0;

// ========================================
// 3. 패킷 송신
// ========================================
void sendPacket(byte cmd, const byte* data, byte len)
{
    Serial.write(STX);
    Serial.write(cmd);
    Serial.write(len);

    for (int i = 0; i < len; i++)
    {
        Serial.write(data[i]);
    }

    Serial.write(ETX);
}

void sendAck(byte processedCmd)
{
    byte data[1] = {processedCmd};
    sendPacket(CMD_ACK, data, 1);
}

void sendStatus()
{
    byte flags = 0;

    if (streamOn[0]) flags |= 0x01;
    if (streamOn[1]) flags |= 0x02;

    byte data[6] = {
        ledMode,
        ledPattern,
        (byte)(period >> 8),
        (byte)(period & 0xFF),
        speedSource,
        flags
    };

    sendPacket(CMD_STATUS, data, 6);
}

void sendADC(byte channel)
{
    int value;

    if (channel == 0)
        value = analogRead(POT_PIN);
    else
        value = analogRead(CDS_PIN);

    byte data[3] = {
        channel,
        (byte)(value >> 8),
        (byte)(value & 0xFF)
    };

    sendPacket(CMD_ADC_DATA, data, 3);
}

void sendEventSwitch(byte sw)
{
    byte data[1] = {sw};
    sendPacket(CMD_EVT_SWITCH, data, 1);
}

void sendEventLED(byte pattern)
{
    byte data[1] = {pattern};
    sendPacket(CMD_EVT_LED, data, 1);
}

void sendEventMode(byte mode, byte reason)
{
    byte data[2] = {mode, reason};
    sendPacket(CMD_EVT_MODE, data, 2);
}

void sendEventSpeed()
{

    byte data[4] = {
        (byte)(period >> 8),
        (byte)(period & 0xFF),
        (byte)(potValue >> 8),
        (byte)(potValue & 0xFF)
    };

    sendPacket(CMD_EVT_SPEED, data, 4);

    reportedPeriod = period;
    lastSpeedEvent = millis();
    speedEventSent = true;
}

// ========================================
// 4. LED 출력
// ========================================
void led_write(byte pattern)
{
    // 패턴이 같다면 이벤트를 보내지 않는다.
    if (ledPattern == pattern)
        return;

    ledPattern = pattern;

    // Active-Low : 1=LED ON, 실제 출력 LOW
    for (int i = 0; i < LED_CNT; i++)
    {
        if (pattern & (1 << i))
            digitalWrite(FIRST_LED + i, LOW);
        else
            digitalWrite(FIRST_LED + i, HIGH);
    }

    sendEventLED(ledPattern);
}

// 왕복 시프트: 총 14단계
byte shiftPattern(byte step)
{
    if (step < 8)
        return (byte)(1 << step);

    return (byte)(1 << (14 - step));
}

byte getStepPattern()
{
    if (ledMode == MODE_ALL)
    {
        return (ledStep % 2 == 0) ? 0xFF : 0x00;
    }

    if (ledMode == MODE_ALT)
    {
        return (ledStep % 2 == 0) ?
               ledStart : (byte)(~ledStart);
    }

    if (ledMode == MODE_SHIFT)
    {
        return shiftPattern(ledStep);
    }

    return 0;
}

byte getStepCount()
{
    if (ledMode == MODE_SHIFT)
        return 14;

    return 2;
}

// ========================================
// 5. LED 모드 실행 및 정지
// ========================================
void startLedMode(byte mode, byte repeat,
                  byte start, byte reason)
{
    ledMode = mode;
    ledRepeat = repeat;
    ledStep = 0;
    ledCycles = 0;

    // ALTERNATING 시작 패턴
    ledStart = (start == 0) ? 0xAA : start;

    lastFrame = millis();
    framePeriod = period;

    sendEventMode(ledMode, reason);
    led_write(getStepPattern());
}

void stopLedMode(byte reason, bool forceModeEvent = false)
{
    bool wasRunning = (ledMode != MODE_IDLE);

    ledMode = MODE_IDLE;

    led_write(0x00);

    if (wasRunning || forceModeEvent)
    {
        sendEventMode(MODE_IDLE, reason);
    }
}

void led_control()
{
    if (ledMode == MODE_IDLE)
        return;

    unsigned long now = millis();

    if (now - lastFrame < framePeriod)
        return;

    // 현재 단계가 끝났으므로 다음 단계로
    ledStep++;

    if (ledStep >= getStepCount())
    {
        ledStep = 0;
        ledCycles++;

        // 반복 0 = 무한
        if (ledRepeat != 0 && ledCycles >= ledRepeat)
        {
            stopLedMode(2);  // 원인 2 = 반복 완료
            return;
        }
    }

    // 변경된 주기는 다음 단계부터 적용
    lastFrame = now;
    framePeriod = period;

    led_write(getStepPattern());
}

// ========================================
// 6. 스위치 PCINT 인터럽트
// ========================================
// D10=PB2, D11=PB3, D12=PB4, D13=PB5
ISR(PCINT0_vect)
{
    swInterrupt = 1;
}

void handleSwitch(byte sw)
{
    // 반드시 EVT_SWITCH 먼저 송신
    sendEventSwitch(sw);

    if (sw == 1)
    {
        startLedMode(MODE_ALL, 3, 0, 1);
    }
    else if (sw == 2)
    {
        startLedMode(MODE_ALT, 3, 0xAA, 1);
    }
    else if (sw == 3)
    {
        startLedMode(MODE_SHIFT, 3, 0, 1);
    }
    else if (sw == 4)
    {
        stopLedMode(1, true);
    }
}

void switch_control()
{
    if (swInterrupt)
    {
        noInterrupts();
        swInterrupt = 0;
        interrupts();

        lastSwitchChange = millis();
        swDebouncing = true;
    }

    if (!swDebouncing)
        return;

    if (millis() - lastSwitchChange < DEBOUNCE_MS)
        return;

    byte nowPins = PINB & 0x3C;

    // HIGH -> LOW로 바뀐 스위치
    byte pressed = stableSwitchPins & (~nowPins);

    stableSwitchPins = nowPins;
    swDebouncing = false;

    for (int i = 0; i < SW_CNT; i++)
    {
        if (pressed & (1 << (i + 2)))
        {
            handleSwitch(i + 1);
        }
    }
}

// ========================================
// 7. 가변저항 속도 제어
// ========================================
unsigned int calculatePotPeriod(unsigned int pot)
{
    return 50 + (unsigned long)pot * 950 / 1023;
}

void pot_control()
{
    unsigned long now = millis();

    if (now - lastPotRead < 20)
        return;

    lastPotRead = now;

    // PC 속도 기준이면 가변저항 변경 무시
    if (speedSource == 0)
        return;

    potValue = analogRead(POT_PIN);
    period = calculatePotPeriod(potValue);

    int difference = (int)period - (int)reportedPeriod;

    if (difference < 0)
        difference = -difference;

    if (difference < 20)
        return;

    // EVT_SPEED 최소 100ms 간격
    if (speedEventSent &&
        now - lastSpeedEvent < 100)
        return;

    sendEventSpeed();
}

// ========================================
// 8. ADC 스트림
// ========================================
void adc_control()
{
    unsigned long now = millis();

    for (int ch = 0; ch < 2; ch++)
    {
        if (!streamOn[ch])
            continue;

        if (now - lastStream[ch] >= streamInterval[ch])
        {
            lastStream[ch] = now;
            sendADC(ch);
        }
    }
}

// ========================================
// 9. 명령 유효성 검사 및 실행
// ========================================
unsigned int readWord(byte highByte, byte lowByte)
{
    return ((unsigned int)highByte << 8) | lowByte;
}

void executeCommand()
{
    // 잘못된 명령은 응답 없이 버린다.
    switch (rxCmd)
    {
        case CMD_PING:
        {
            if (rxLen != 0) return;

            sendAck(rxCmd);
            break;
        }

        case CMD_GET_STATUS:
        {
            if (rxLen != 0) return;

            sendStatus();
            break;
        }

        case CMD_LED_SET:
        {
            if (rxLen != 1) return;

            // 응답 먼저
            sendAck(rxCmd);

            // 실행 중인 모드는 종료
            if (ledMode != MODE_IDLE)
            {
                ledMode = MODE_IDLE;
                sendEventMode(MODE_IDLE, 0);
            }

            led_write(rxData[0]);
            break;
        }

        case CMD_LED_RUN:
        {
            if (rxLen != 3) return;

            byte mode = rxData[0];
            byte repeat = rxData[1];
            byte start = rxData[2];

            if (mode < MODE_ALL || mode > MODE_SHIFT)
                return;

            sendAck(rxCmd);

            startLedMode(mode, repeat, start, 0);
            break;
        }

        case CMD_LED_STOP:
        {
            if (rxLen != 0) return;

            sendAck(rxCmd);
            stopLedMode(0);
            break;
        }

        case CMD_SET_PERIOD:
        {
            if (rxLen != 2) return;

            unsigned int newPeriod =
                readWord(rxData[0], rxData[1]);

            if (newPeriod < 50 || newPeriod > 1000)
                return;

            sendAck(rxCmd);

            pcPeriod = newPeriod;
            speedSource = 0;
            period = pcPeriod;

            break;
        }

        case CMD_SET_SPEED_SRC:
        {
            if (rxLen != 1) return;

            if (rxData[0] > 1)
                return;

            sendAck(rxCmd);

            speedSource = rxData[0];

            if (speedSource == 0)
            {
                period = pcPeriod;
            }
            else
            {
                potValue = analogRead(POT_PIN);
                period = calculatePotPeriod(potValue);
            }

            break;
        }

        case CMD_ADC_READ:
        {
            if (rxLen != 1) return;

            if (rxData[0] > 1)
                return;

            // ADC_DATA가 명령에 대한 응답
            sendADC(rxData[0]);
            break;
        }



case CMD_ADC_STREAM:
{
    if (rxLen != 2 && rxLen != 4)
        return;

    byte ch = rxData[0];
    byte enable = rxData[1];

    if (ch > 1 || enable > 1)
        return;

    if (enable == 0)
    {
        // OFF: LEN=2 또는 LEN=4(간격 0) 허용
        if (rxLen == 4)
        {
            if (rxData[2] != 0 || rxData[3] != 0)
                return;
        }
        else if (rxLen != 2)
        {
            return;
        }

        streamOn[ch] = false;
    }
    else
    {
        // ON: LEN=4, 최소 간격 100ms
        if (rxLen != 4)
            return;

        unsigned int interval =
            readWord(rxData[2], rxData[3]);

        if (interval < 100)
            return;

        streamInterval[ch] = interval;
        streamOn[ch] = true;
    }

    lastStream[ch] = millis();

    sendAck(rxCmd);
    break;
}


    }
}

// ========================================
// 10. UART 패킷 수신
// ========================================
// 상태 0: STX 대기
// 상태 1: CMD
// 상태 2: LEN
// 상태 3: DATA
// 상태 4: ETX

void serial_receive()
{
    // 중간에 패킷이 끊긴 경우 초기화
    if (rxState != 0 &&
        millis() - lastRxTime > 100)
    {
        rxState = 0;
    }

    while (Serial.available() > 0)
    {
        byte value = (byte)Serial.read();
        lastRxTime = millis();

        switch (rxState)
        {
            case 0:
            {
                if (value == STX)
                    rxState = 1;

                break;
            }

            case 1:
            {
                rxCmd = value;
                rxState = 2;
                break;
            }

            case 2:
            {
                rxLen = value;
                rxIndex = 0;

                // LEN >= 17이면 폐기
                if (rxLen >= 17)
                {
                    rxState = 0;
                }
                else if (rxLen == 0)
                {
                    rxState = 4;
                }
                else
                {
                    rxState = 3;
                }

                break;
            }

            case 3:
            {
                // DATA 내부의 0x03도 정상 데이터
                rxData[rxIndex] = value;
                rxIndex++;

                if (rxIndex >= rxLen)
                {
                    rxState = 4;
                }

                break;
            }

            case 4:
            {
                // 정확한 위치의 ETX만 인정
                rxState = 0;

                if (value == ETX)
                {
                    executeCommand();
                }

                break;
            }

            default:
            {
                rxState = 0;
                break;
            }
        }
    }
}

// ========================================
// 11. SETUP
// ========================================
void setup()
{
    // LED D2 ~ D9 (Active-Low)
    for (int i = 0; i < LED_CNT; i++)
    {
        digitalWrite(FIRST_LED + i, HIGH);
        pinMode(FIRST_LED + i, OUTPUT);
    }

    // 스위치 D10 ~ D13 (INPUT_PULLUP)
    for (int i = 0; i < SW_CNT; i++)
    {
        pinMode(FIRST_SW + i, INPUT_PULLUP);
    }

    pinMode(POT_PIN, INPUT);
    pinMode(CDS_PIN, INPUT);

    // PCINT0: PORTB의 PB2 ~ PB5
    stableSwitchPins = PINB & 0x3C;

    PCMSK0 |= (1 << PCINT2);
    PCMSK0 |= (1 << PCINT3);
    PCMSK0 |= (1 << PCINT4);
    PCMSK0 |= (1 << PCINT5);

    PCIFR |= (1 << PCIF0);
    PCICR |= (1 << PCIE0);

    // 기본 속도: 가변저항
    potValue = analogRead(POT_PIN);
    period = calculatePotPeriod(potValue);

    framePeriod = period;
    reportedPeriod = period;
    pcPeriod = 500;

    speedSource = 1;
    ledMode = MODE_IDLE;
    ledPattern = 0;

    // UART : 38400bps, 8O2
    Serial.begin(38400, SERIAL_8O2);

    // 전원 ON 직후 STATUS 1회
    sendStatus();
}

// ========================================
// 12. LOOP
// ========================================
void loop()
{
    serial_receive();  // PC 명령 우선 처리
    switch_control();  // 스위치 인터럽트 처리
    pot_control();     // 주기 및 EVT_SPEED
    led_control();     // 비차단 LED 단계 변경
    adc_control();     // ADC 스트림
}
