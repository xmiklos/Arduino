
Timer should_get_temp(5000, true);
Timer rerender_debounce(100, false, false);

void setup(){ 
  lcd.begin();
  lcd.backlight();
  lcd.print("Inicializujem");
  lcd.setCursor(0, 1);
  lcd.print("M. Miklos 2018");
  lcd.createChar(1, full_char);
  lcd.createChar(2, arrow_down_char);
  lcd.createChar(3, arrow_up_char);

  pinMode(PIN_ENCODER_SW, INPUT_PULLUP);
  pinMode(PIN_BEEP, OUTPUT);

  // encoder ISR
  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT20) | (1 << PCINT21);

  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_SW), view_edit_isr, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_RF24_INT), check_radio, LOW);

  Serial.begin(9600);
  printf_begin();

  dht_sensor.begin();
  teplota_dom.set( dht_sensor.readTemperature() );

  radio.begin();
  //radio.enableDynamicPayloads();
  radio.setPayloadSize( sizeof( data_packet ) );
  radio.openWritingPipe(address[1]);
  radio.openReadingPipe(1, address[0]);
  radio.startListening();
  radio.setDataRate(RF24_250KBPS);
  attachInterrupt(0, check_radio, LOW);

  restore_data();

  Serial.println("Start (remote)");
  radio.printDetails();

  edit_state.set( 0 );
}

bool rerender = true;

void loop() {

  button_debounce.tick();
  if(enc_used.tick()) {
    current_view.set( 0 ); // home screen
    edit_state.set( 0 );
  }

  if (keep_alive.tick() && !enc_used.started()) {
    Serial.print("keep_alive");
    send(4);
  }

  if (invalidate_sensor.tick()) {
    teplota_kotol.set(UNDEF);
  }

  int direction = enc.getDirection();
  if (edit_state.get() == 0) { // pohyb v menu
    if (direction > 0) {
      if (current_view.get() == view_count - 1) { // if last
        current_view.set(0); // start from begining
      } else {
        current_view.inc();
      }
    } else if (direction < 0) {
      if (current_view.get() == 0) { // if on first
        current_view.set( view_count - 1 ); // go to last
      } else {
        current_view.dec();
      }
    }
  } else if (views[ current_view.get() ].data != NULL) { // uprava polozky menu
    if (direction > 0) {
      views[ current_view.get() ].data->inc( user );
    } else if (direction < 0) {
      views[ current_view.get() ].data->dec( user );
    }
  }

  if (current_view.change()) {
    rerender = true;
  }

  if (!enc_used.started() && should_get_temp.tick()) {
    //Serial.println("Getting temp");
    teplota_dom.set( dht_sensor.readTemperature() );
    //Serial.println("got");
  }

  if (teplota_dom.change()) {
    send(1);
    rerender = true;
  }

  if (teplota_ciel.change()) {
    rerender = true;
    if (teplota_ciel.origin() == network) {
      store_data();
    }
  }

  if (teplota_kotol.change()) {
    rerender = true;
  }

  if (teplota_alarm.change()) {
    rerender = true;
  }

  if (vykon.change()) {
    rerender = true;
    if (vykon.origin() == network) {
      store_data();
    }
  }

  if (rezim.change()) {
    rerender = true;
  }

  if (alarm.change()) {
    rerender = true;
    if(alarm.get() == 0) {
      alarm_snooze.stop();
    }
    store_data();
  }

  if (edit_state.change()) {
    rerender = true;
    if (!edit_state.get()) {
      send(2);
      store_data();
    }
  }

  rerender_debounce.tick();
  if (rerender && !rerender_debounce.started()) {
    render();
    rerender = false;
    rerender_debounce.start();
  }

  if(repeat_send.tick()) { /* retry send on fail */
    send(3);
  }

  alarm_snooze.tick();

  if (
        alarm.get()
        && !alarm_snooze.started()
        && teplota_kotol.get() != UNDEF
        && teplota_kotol.get() <= teplota_alarm.get()
        && rezim.get() != 0
  ) {
    beeping.start();
  } else if (beeping.started()) {
    beep_off();
    beeping.stop();
    beeping2.stop();
  }

  if (beeping.tick()) {
    if (beeping2.started()) {
      beeping2.stop();
      beep_off();
    } else {
      beeping2.start();
      beep_on();
    }
  }

  if (beeping2.tick()) {
    beep_toggle();
  }

}
