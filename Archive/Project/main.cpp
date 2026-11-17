#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_NeoPixel.h>

// =====================
// 온도센서 핀
// =====================
#define SENSOR1_PIN 2
#define SENSOR2_PIN 3
#define SENSOR3_PIN 4

// =====================
// 네오픽셀 핀
// =====================
#define PIXEL_ALWAYS_PIN 5     // 중앙 유도등 앞쪽
#define PIXEL1_PIN 6           // 1번 갈림길
#define PIXEL2_PIN 7           // 2번 갈림길
#define PIXEL3_PIN 8           // 3번 갈림길
#define PIXEL_CENTER2_PIN 10   // 중앙 유도등 뒤쪽

// =====================
// 피에조 부저 핀
// =====================
#define BUZZER_PIN 9

// =====================
// 네오픽셀 개수
// =====================
#define PIXEL_ALWAYS_COUNT 15
#define PIXEL1_COUNT 1
#define PIXEL2_COUNT 1
#define PIXEL3_COUNT 1
#define PIXEL_CENTER2_COUNT 15

// 기준 온도
#define LIMIT_TEMP 33.0

// =====================
// DS18B20 설정
// =====================
OneWire oneWire1(SENSOR1_PIN);
OneWire oneWire2(SENSOR2_PIN);
OneWire oneWire3(SENSOR3_PIN);

DallasTemperature sensor1(&oneWire1);
DallasTemperature sensor2(&oneWire2);
DallasTemperature sensor3(&oneWire3);

// =====================
// 네오픽셀 설정
// =====================
Adafruit_NeoPixel pixelAlways(
  PIXEL_ALWAYS_COUNT,
  PIXEL_ALWAYS_PIN,
  NEO_GRB + NEO_KHZ800
);

Adafruit_NeoPixel pixel1(
  PIXEL1_COUNT,
  PIXEL1_PIN,
  NEO_GRB + NEO_KHZ800
);

Adafruit_NeoPixel pixel2(
  PIXEL2_COUNT,
  PIXEL2_PIN,
  NEO_GRB + NEO_KHZ800
);

Adafruit_NeoPixel pixel3(
  PIXEL3_COUNT,
  PIXEL3_PIN,
  NEO_GRB + NEO_KHZ800
);

Adafruit_NeoPixel pixelCenter2(
  PIXEL_CENTER2_COUNT,
  PIXEL_CENTER2_PIN,
  NEO_GRB + NEO_KHZ800
);


// =====================
// 네오픽셀 전체 켜기 함수
// =====================
void turnOnStrip(Adafruit_NeoPixel &strip, int count) {

  for (int i = 0; i < count; i++) {
    strip.setPixelColor(i, strip.Color(0, 50, 0));
  }

  strip.show();
}


// =====================
// 네오픽셀 전체 끄기 함수
// =====================
void turnOffStrip(Adafruit_NeoPixel &strip, int count) {

  for (int i = 0; i < count; i++) {
    strip.setPixelColor(i, 0, 0, 0);
  }

  strip.show();
}


void setup() {

  Serial.begin(9600);

  // 온도센서 시작
  sensor1.begin();
  sensor2.begin();
  sensor3.begin();

  sensor1.setResolution(12);
  sensor2.setResolution(12);
  sensor3.setResolution(12);

  // 네오픽셀 시작
  pixelAlways.begin();
  pixel1.begin();
  pixel2.begin();
  pixel3.begin();
  pixelCenter2.begin();

  // 처음에는 모두 끄기
  pixelAlways.clear();
  pixel1.clear();
  pixel2.clear();
  pixel3.clear();
  pixelCenter2.clear();

  pixelAlways.show();
  pixel1.show();
  pixel2.show();
  pixel3.show();
  pixelCenter2.show();

  // 피에조 부저
  pinMode(BUZZER_PIN, OUTPUT);
}


void loop() {

  // =====================
  // 온도 측정
  // =====================
  sensor1.requestTemperatures();
  sensor2.requestTemperatures();
  sensor3.requestTemperatures();

  float temp1 = sensor1.getTempCByIndex(0);
  float temp2 = sensor2.getTempCByIndex(0);
  float temp3 = sensor3.getTempCByIndex(0);


  // =====================
  // 온도 출력
  // =====================
    Serial.print(temp1, 2);
    Serial.print(",");
    Serial.print(temp2, 2);
    Serial.print(",");
    Serial.println(temp3, 2);


  // ==================================================
  // 1. 센서 3개 모두 33도 이상
  // → 모든 네오픽셀 OFF
  // ==================================================
  if (temp1 >= LIMIT_TEMP &&
      temp2 >= LIMIT_TEMP &&
      temp3 >= LIMIT_TEMP) {

    turnOffStrip(pixelAlways, PIXEL_ALWAYS_COUNT);
    turnOffStrip(pixelCenter2, PIXEL_CENTER2_COUNT);

    turnOffStrip(pixel1, PIXEL1_COUNT);
    turnOffStrip(pixel2, PIXEL2_COUNT);
    turnOffStrip(pixel3, PIXEL3_COUNT);
  }


  // ==================================================
  // 2. 센서2와 센서3 둘 다 33도 이상
  //
  // D5 : 1~10번째 ON
  //       11~15번째 OFF
  //
  // D10 : 전체 OFF
  // ==================================================
  else if (temp2 >= LIMIT_TEMP &&
           temp3 >= LIMIT_TEMP) {

    // D5
    for (int i = 0; i < PIXEL_ALWAYS_COUNT; i++) {

      if (i < 10) {
        pixelAlways.setPixelColor(
          i,
          pixelAlways.Color(0, 50, 0)
        );
      }
      else {
        pixelAlways.setPixelColor(i, 0, 0, 0);
      }
    }

    pixelAlways.show();


    // D10 전체 OFF
    turnOffStrip(pixelCenter2, PIXEL_CENTER2_COUNT);


    // D6 : 센서1 상태에 따라 결정
    if (temp1 >= LIMIT_TEMP) {
      turnOffStrip(pixel1, PIXEL1_COUNT);
    }
    else {
      turnOnStrip(pixel1, PIXEL1_COUNT);
    }

    // 센서2, 센서3 위험
    turnOffStrip(pixel2, PIXEL2_COUNT);
    turnOffStrip(pixel3, PIXEL3_COUNT);
  }


  // ==================================================
  // 3. 센서3만 포함하여 33도 이상
  //
  // D10 : 1~5번째 ON
  //       6~15번째 OFF
  // ==================================================
  else if (temp3 >= LIMIT_TEMP) {

    // D5 전체 ON
    turnOnStrip(pixelAlways, PIXEL_ALWAYS_COUNT);


    // D10
    for (int i = 0; i < PIXEL_CENTER2_COUNT; i++) {

      if (i < 5) {

        // 실제 1~5번째
        pixelCenter2.setPixelColor(
          i,
          pixelCenter2.Color(0, 50, 0)
        );

      }
      else {

        // 실제 6~15번째
        pixelCenter2.setPixelColor(i, 0, 0, 0);
      }
    }

    pixelCenter2.show();


    // D6
    if (temp1 >= LIMIT_TEMP) {
      turnOffStrip(pixel1, PIXEL1_COUNT);
    }
    else {
      turnOnStrip(pixel1, PIXEL1_COUNT);
    }


    // D7
    if (temp2 >= LIMIT_TEMP) {
      turnOffStrip(pixel2, PIXEL2_COUNT);
    }
    else {
      turnOnStrip(pixel2, PIXEL2_COUNT);
    }


    // D8은 센서3가 위험하므로 OFF
    turnOffStrip(pixel3, PIXEL3_COUNT);
  }


  // ==================================================
  // 4. 센서3은 정상인 경우
  // 중앙 유도등은 전체 ON
  // ==================================================
  else {

    // 중앙 유도등 전체 ON
    turnOnStrip(pixelAlways, PIXEL_ALWAYS_COUNT);
    turnOnStrip(pixelCenter2, PIXEL_CENTER2_COUNT);


    // D6 - 센서1
    if (temp1 >= LIMIT_TEMP) {
      turnOffStrip(pixel1, PIXEL1_COUNT);
    }
    else {
      turnOnStrip(pixel1, PIXEL1_COUNT);
    }


    // D7 - 센서2
    if (temp2 >= LIMIT_TEMP) {
      turnOffStrip(pixel2, PIXEL2_COUNT);
    }
    else {
      turnOnStrip(pixel2, PIXEL2_COUNT);
    }


    // D8 - 센서3
    if (temp3 >= LIMIT_TEMP) {
      turnOffStrip(pixel3, PIXEL3_COUNT);
    }
    else {
      turnOnStrip(pixel3, PIXEL3_COUNT);
    }
  }


  // =====================
  // 피에조 부저
  // 하나라도 33도 이상이면 울림
  // =====================
  if (temp1 >= LIMIT_TEMP ||
      temp2 >= LIMIT_TEMP ||
      temp3 >= LIMIT_TEMP) {

    tone(BUZZER_PIN, 1000);

  }
  else {

    noTone(BUZZER_PIN);
  }


  delay(1000);
}
