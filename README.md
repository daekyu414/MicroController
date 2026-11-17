# MicroController Study

Arduino Uno와 PlatformIO를 사용해 마이크로컨트롤러를 공부하며 작성한 코드를 정리한 저장소입니다.

> 이 저장소의 기본 개발 환경과 템플릿은 수업에서 제공된 자료를 바탕으로 합니다.
> 제가 직접 실습하고 공부한 코드는 **[`Archive`](Archive/)** 폴더에 정리되어 있습니다.

## 학습 환경

| 항목 | 내용 |
|---|---|
| 보드 | Arduino Uno R3 (ATmega328P) |
| 언어 | C/C++ |
| 개발 환경 | Visual Studio Code + PlatformIO |
| 주요 주제 | GPIO, 스위치, 인터럽트, UART, ADC, 타이머, NeoPixel, 온습도·초음파 센서 |

## 학습 내용

### [Week 01 - LED 제어](Archive/week01/)

- LED 전체 점등 및 점멸
- 반복 동작을 함수로 구현
- 교차 패턴과 시프트 패턴 구현
- LED 제어 과제

### [Week 02 - 스위치 입력](Archive/week02/)

- 스위치 입력 기초
- 스위치로 LED 이동 방향 제어
- 마지막 LED와 첫 번째 LED 연결 처리
- 다중 스위치 제어 과제

### [Week 03 - 인터럽트](Archive/week03/)

- 외부 인터럽트
- Pin Change Interrupt(PCINT)
- 인터럽트를 이용한 스위치 및 LED 제어

### [Week 04 - UART 통신](Archive/week04/)

- 레지스터를 이용한 문자 및 문자열 송신
- `printf`를 이용한 시리얼 출력
- UART와 PCINT 연동
- STX/ETX 기반 패킷 프로토콜

### [Week 05 - ADC](Archive/week05/)

- 가변저항 값 읽기
- 가변저항을 이용한 LED 제어
- 조도 센서 값 읽기
- ADC 센서 기반 LED 제어

### [Week 10 - 타이머와 부저](Archive/week10/)

- 타이머 오버플로 인터럽트
- CTC 모드를 이용한 LED 제어
- 타이머를 이용한 부저 및 음계 출력

### [Week 11 - NeoPixel](Archive/week11/)

- NeoPixel LED 제어 실습
- NeoPixel 응용 과제

### [Week 12 - 온습도·초음파 센서](Archive/week12/)

- DHT22로 온도와 습도를 읽어 시리얼 모니터에 출력
- 습도에 따라 LED 막대 표시 (D2~D9, 최대 8개)
- HC-SR04의 Trig 신호와 `pulseIn()`으로 Echo 시간을 측정하고 거리 계산
- 거리가 가까울수록 LED가 많이 켜지는 표시 구현 (5cm 간격, 40cm 이상이면 모두 꺼짐)
- 초음파 측정 시간 초과 처리

| 예제 | 내용 | 센서 핀 |
|---|---|---|
| `1_온습도센서.cpp` | 온도·습도 시리얼 출력 | DHT22: A5 |
| `2_ 온습도센서_LED.cpp` | 습도에 따른 LED 막대 표시 | DHT22: A5 |
| `3_초음파센서.cpp` | Echo 시간과 거리(cm) 출력 | Trig: A4, Echo: D12 |
| `4_초음파센서_LED.cpp` | 거리에 따른 LED 막대 표시 | Trig: A4, Echo: D12 |

### 기타 실습

- [`Archive/middle`](Archive/middle/): UART 프로토콜 및 통신 테스트
- [`Archive/test`](Archive/test/): 이진 ALU 등 추가 실험 코드
- [`Archive/Project`](Archive/Project/): DS18B20 온도센서 3개와 NeoPixel, 부저를 연동한 프로젝트

## 폴더 구조

```text
MicroController/
├── Archive/          # 개인 학습 및 실습 코드
│   ├── week01/      # LED
│   ├── week02/      # 스위치
│   ├── week03/      # 인터럽트
│   ├── week04/      # UART
│   ├── week05/      # ADC
│   ├── week10/      # 타이머와 부저
│   ├── week11/      # NeoPixel
│   ├── week12/      # 온습도·초음파 센서
│   ├── Project/     # 온도센서·NeoPixel·부저 연동
│   ├── middle/      # 통신 테스트
│   └── test/        # 추가 실험
├── src/              # 실행할 예제를 main.cpp로 복사하는 폴더
├── platformio.ini    # PlatformIO 프로젝트 설정
└── README.md
```

## 코드 실행 방법

`Archive`의 파일은 학습 기록이므로 자동으로 빌드되지 않습니다. 실행할 코드를 `src/main.cpp`에 복사한 다음 PlatformIO에서 Build 또는 Upload를 실행합니다.

```bash
pio run
pio run --target upload
```

각 예제는 회로 구성과 핀 번호가 다를 수 있으므로 코드 상단의 핀 설정을 먼저 확인해야 합니다.

Week 12의 LED 예제는 D2~D9를 사용하며, LOW를 출력하면 LED가 켜지는 Active Low 방식입니다. DHT22는 2초 간격으로 읽고, 시리얼 모니터 속도는 `9600`으로 설정합니다. 필요한 DHT 라이브러리는 `platformio.ini`의 `lib_deps`에 등록되어 있습니다.
