
Controller * control;


void setup() {
  Serial.begin(9600);
  // while (!Serial);
  control = new Controller();
  control->start();
}

void loop() {
  
  control->stop_check();
  //unsigned long time2 = millis();
  //Serial.println(time2 - time);
}

