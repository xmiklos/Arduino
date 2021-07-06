
Controller * control;


void setup() {
  Serial.begin(9600);
  // while (!Serial);
  control = new Controller();
  control->start();
  vw_set_ptt_inverted(true); // Required for DR3100
  vw_set_rx_pin(REMOTE_PIN);
  vw_setup(100);   // Bits per sec
  vw_rx_start();

  pinMode(ENC_SW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_SW), sw, RISING );
}


void loop() {
  if (vw_get_message(buf, &buflen)) {
      global_temp = *((uint8_t*)buf);
      got = true;
      got2 = true;
      Serial.println(global_temp);
  }
  control->stop_check();
}

