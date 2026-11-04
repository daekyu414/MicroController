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
| 주요 주제 | GPIO, 스위치, 인터럽트, UART, ADC |

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

### 기타 실습

- [`Archive/middle`](Archive/middle/): UART 프로토콜 및 통신 테스트
- [`Archive/test`](Archive/test/): 이진 ALU 등 추가 실험 코드

## 폴더 구조

```text
MicroController/
├── Archive/          # 개인 학습 및 실습 코드
│   ├── week01/      # LED
│   ├── week02/      # 스위치
│   ├── week03/      # 인터럽트
│   ├── week04/      # UART
│   ├── week05/      # ADC
│   ├── middle/      # 통신 테스트
│   └── test/        # 추가 실험
├── src/main.cpp      # PlatformIO 빌드 대상 코드
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
