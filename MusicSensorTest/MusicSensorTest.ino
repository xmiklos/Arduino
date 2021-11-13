
#include <Timer.h>

Timer _5min(60000, false, false);
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(2, INPUT_PULLUP);
  Serial.begin(9600);
  Serial.println("Start");
 
  attachInterrupt(digitalPinToInterrupt(2), falling, FALLING);
}

void rising() {
  digitalWrite(LED_BUILTIN, LOW);
}

void falling() {
  _5min.restart();
  digitalWrite(LED_BUILTIN, HIGH);
  //Serial.println("HIGH");
}

void loop() {
  if (_5min.tick()) {
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("LOW");
  }
}
