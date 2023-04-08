#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Timer.h>
#include <Value.h>
#include "EncoderStepCounter.h"
#include <espnow.h>
#define ENCODER1_PIN1 13
#define ENCODER1_INT1 digitalPinToInterrupt(ENCODER1_PIN1)
#define ENCODER1_PIN2 15 
#define ENCODER1_INT2 digitalPinToInterrupt(ENCODER1_PIN2)

#define ENCODER2_PIN1 12
#define ENCODER2_INT1 digitalPinToInterrupt(ENCODER2_PIN1)
#define ENCODER2_PIN2 14
#define ENCODER2_INT2 digitalPinToInterrupt(ENCODER2_PIN2)

EncoderStepCounter encoder1(ENCODER1_PIN1, ENCODER1_PIN2, HALF_STEP);
EncoderStepCounter encoder2(ENCODER2_PIN1, ENCODER2_PIN2, HALF_STEP);

ICACHE_RAM_ATTR void interrupt1() {
  encoder1.tick();
}

ICACHE_RAM_ATTR void interrupt2() {
  encoder2.tick();
}

WiFiUDP udp;
void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Start!");

  WiFi.mode(WIFI_STA);
  WiFi.begin("4Wheel", "hesloveslo");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Connected! Gateway IP address: ");
  Serial.println(WiFi.gatewayIP());

  //Serial.printf("UDP server on port %d\n", 8888);

  //udp.begin(8888);

  pinMode(D2, OUTPUT);
  pinMode(D1, OUTPUT);
  digitalWrite(D2, HIGH);
  digitalWrite(D1, LOW);
  encoder1.begin();
  encoder2.begin();
  attachInterrupt(ENCODER1_INT1, interrupt1, CHANGE);
  attachInterrupt(ENCODER1_INT2, interrupt1, CHANGE);
  attachInterrupt(ENCODER2_INT1, interrupt2, CHANGE);
  attachInterrupt(ENCODER2_INT2, interrupt2, CHANGE);
}

int direction1() {
  int pos = encoder1.getPosition();
  if (pos != 0) {
    encoder1.reset();
  }
  return pos;
}

int direction2() {
  int pos = encoder2.getPosition();
  if (pos != 0) {
    encoder2.reset();
  }
  return pos;
}

Value<int> throttle(10, 0, 20);
Value<int> steering(8, 1, 15);
unsigned long bits = 0x80800000;
Timer second(1000, true, true);
unsigned long t = 0;
void loop() {
  int change = 0;
  if (throttle.change( throttle.get() + direction1() )) {
    Serial.println(throttle.get());
    change = 1;
  }

  if (steering.change( steering.get() + direction2() )) {
    Serial.println(steering.get());
    change = 1;
  }

  if (change) {
    bits = to_bits(throttle.get(), steering.get());


  }
    
  udp.beginPacket(WiFi.gatewayIP(), 4444);
  udp.write((char*)&bits, 4);
  udp.endPacket();

  if (second.tick() && WiFi.status() != WL_CONNECTED) {
    ESP.reset();
  }

  //delay(10);
}

unsigned long to_bits(unsigned int throttle_in, unsigned int steering_in) {
  unsigned long result = 0;

  // off, 1 = 0ff, 0 == on
  if (throttle_in == 10) {
    result = 1;
  }
  result <<= 7;

  int th = throttle_in < 10 ? 10 + (10 - throttle_in) : throttle_in;
  unsigned long throttle = map(th, 10, 20, 0, 127);
  // throttle, 7bits
  result |= (throttle & 0x7F);

  // throttle, 7bits
  result <<= 4;
  result |= (steering_in & 0xF);

  // shift completelly to left
  result <<= 20;

  if (throttle_in < 10) {
    result |= (1 << 12);
  }

  if (0) { // dump bits
    for (int i = 31; i >= 0; i--) {
      Serial.print( (result >> i) & 1, BIN );
      if (i % 8 == 0) {
        Serial.print(" ");
      }
    }
    Serial.println();
  }

  return result;
}
