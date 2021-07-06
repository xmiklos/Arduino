
const double max_voltage = 5000;

double get_actuator_current() {

  double voltage  = 0;
  double current  = 0;
  double sum      = 0;
  
  digitalWrite(PIN_CURRENT_ENABLE, HIGH);
  for(int i = 0; i < 100; i++) {
    voltage   = (max_voltage * analogRead(PIN_CURRENT_SENSOR)) / 1023.0;
    current   = (voltage - max_voltage/2) / 185;
    sum       += current;
  }
  digitalWrite(PIN_CURRENT_ENABLE, LOW);
  
  //Serial.println((sum / 100) * 1000 + 740);
  return (sum / 100) * 1000 + 740;
}

bool actuator_closing = false;    // we are closing or opening
bool actuator_closed = false;
Timer actuator(0, false, false);
Timer actuator_alternator(300000, false, false);

void actuator_stop() {
  digitalWrite(PIN_ACTUATOR_A, LOW);
  digitalWrite(PIN_ACTUATOR_B, LOW);
  actuator.stop();
  if (actuator_closing) {
    Serial.println("stopped closing");
    actuator_closed = true;
  } else {
    actuator_closed = false;
    Serial.println("stopped opening");
  }
}

void actuator_open(int time_ms) {
  if (!actuator.started() && actuator_closed) {
    Serial.println("opening");
    digitalWrite(PIN_ACTUATOR_A, HIGH);
    digitalWrite(PIN_ACTUATOR_B, LOW);
    actuator.restart(time_ms);
    actuator_closing = false;
  }
}

void actuator_close() {
  if (!actuator.started() && !actuator_closed) {
    Serial.println("closing");
    digitalWrite(PIN_ACTUATOR_A, LOW);
    digitalWrite(PIN_ACTUATOR_B, HIGH);
    actuator_closing = true;
    actuator.restart(50000);
  }
}

