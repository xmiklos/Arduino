
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

  pinMode(PIN_ACTUATOR_A, OUTPUT);
  pinMode(PIN_ACTUATOR_B, OUTPUT);
  pinMode(PIN_ENCODER_SW, INPUT_PULLUP);
  pinMode(PIN_CURRENT_ENABLE, OUTPUT);

  // encoder ISR
  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT20) | (1 << PCINT21);

  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_SW), view_edit_isr, RISING);  
  attachInterrupt(digitalPinToInterrupt(PIN_RF24_INT), check_radio, LOW);

  Serial.begin(9600);
  printf_begin();

  dallas_temp.begin();
  dallas_temp.requestTemperatures();
  teplota_kotol.set( dallas_temp.getTempCByIndex(0) );

  radio.begin();
  //radio.enableDynamicPayloads();
  radio.setPayloadSize( sizeof( data_packet ) );
  radio.openWritingPipe(address[0]);
  radio.openReadingPipe(1, address[1]);
  radio.startListening();
  radio.setDataRate(RF24_250KBPS);

  restore_data();
  rezim.set(0);

  actuator_close();
  Serial.println("Start (base)");
  radio.printDetails();

  edit_state.set( 0 );
}

bool rerender = true;
bool vykon_change = false;

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
    teplota_dom.set(UNDEF);
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
      views[ current_view.get() ].data->inc();
    } else if (direction < 0) {
      views[ current_view.get() ].data->dec();
    }
  }

  if (current_view.change()) {
    rerender = true;
  }

  if (!enc_used.started() && should_get_temp.tick()) {
    //Serial.println("Getting temp");
    dallas_temp.requestTemperatures();
    teplota_kotol.set( dallas_temp.getTempCByIndex(0) );
    //Serial.println("got");
  }

  if (teplota_kotol.change()) {
    send(1);
    rerender = true;
  }

  if (teplota_ciel.change()) {
    rerender = true;
    if (teplota_ciel.origin() == network) {
      store_data();
    }
  }

  if (teplota_dom.change()) {
    rerender = true;
  }

  if (vykon.change()) {
    rerender = true;
    vykon_change = true;
    if (!enc_used.started()) { // remote changed power
      actuator_close();
    }
    if (vykon.origin() == network) {
      store_data();
    }
  }

  if (rezim.get() == 0 && teplota_kotol.get() >= 48 && !enc_used.started()) { // start
    rezim.set( rezim.get_prev() != 0 ? rezim.get_prev() : 1 );
    send(2);
  }

  if (rezim.change()) {
    rerender = true;

    if (rezim.get() != 1) {
      actuator_alternator.stop();
    }
  }

  if (zatvor_vzduch.change()) {
    rerender = true;
  }

  if (edit_state.change()) {
    rerender = true;
    if (!edit_state.get()) {
      send(2);
      store_data();
    }
    if (vykon_change) {
      vykon_change = false;
      actuator_close();
    }
  }

  if (
      actuator.tick()
      || (actuator.elapsed() > 500 && abs(get_actuator_current()) < 200) // no current
  ) {
    actuator_stop();
  }

  if (teplota_kotol.get() <= zatvor_vzduch.get() && rezim.get() != 0 /* start */) {
    actuator_close();
  } else if (rezim.get() == 1) { // auto
    if (teplota_dom.get() != UNDEF) {
      if (teplota_dom.get() < teplota_ciel.get()) {
        actuator_open(500 * (vykon.get()));
      } else {

        if (!actuator_alternator.started()) {
          actuator_close();
          actuator_alternator.start();
        } else if (actuator_alternator.tick()) {
          if (actuator_closed) {
            actuator_open(500 * (vykon.get()));
          } else {
            actuator_close();
          }
          actuator_alternator.start();
        }
      }
    }
  } else if (rezim.get() == 2) { // manual
    actuator_open(500 * (vykon.get()));
  } else if (rezim.get() == 0) { // start
    actuator_open(max(500 * (vykon.get()), 3000));
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

}
