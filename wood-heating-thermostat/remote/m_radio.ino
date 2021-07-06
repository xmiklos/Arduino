typedef struct {
  int8_t sensor;
  int8_t ciel;
  int8_t vykon;
  int8_t rezim;
  int8_t request;
  int8_t initial;
  unsigned long time;
} data_packet;

Timer repeat_send(4000, false);
Timer keep_alive(10000);
Timer invalidate_sensor(60000);

volatile bool initial = true;

void send(int reason = 0) {
  repeat_send.stop();
  radio.stopListening();
  Serial.print("sending ");
  Serial.println(reason);
  int8_t request = teplota_dom.get() == UNDEF;
  data_packet packet = {
    teplota_dom.get(),
    teplota_ciel.get(),
    vykon.get(),
    rezim.get(),
    request,
    initial,
    millis()
  };
  radio.startWrite(&packet, sizeof(data_packet), 0);
}

int fail_count = 0;
void check_radio(void)
{
  bool tx, fail, rx;
  radio.whatHappened(tx, fail, rx);
  
  if ( tx ) {
      fail_count = 0;
      radio.flush_tx();
      radio.startListening();
      Serial.println("Send: OK");
      //keep_alive.restart();
      invalidate_sensor.restart();
  }
  
  if ( fail ) {
      fail_count++;
      radio.flush_tx();
      radio.startListening();
      repeat_send.start( fail_count * 2000 );
      Serial.println("Send: Failed");
  }

  if ( rx || radio.available()){
    
    while (radio.available()) {
      Serial.println("received ");
      data_packet packet;
      radio.read(&packet, sizeof(packet));

      teplota_kotol.set( packet.sensor, network );

      if (packet.initial) { // je to inicialny packet
        Serial.println("got initial");
        if (millis() > packet.time) { // sme starsi
          Serial.println("im older");
          initial = false;
          repeat_send.start(1000); // posleme odpoved
        }
        continue; // inicialny paket nikdy neukladam
      }
      initial = false; // dostal som neinicialny packet, mam spravne nastavenia
      Serial.println("normal");

      if (current_view.get() != 0 || edit_state.get() != 1) {
        teplota_ciel.set( packet.ciel, network );
      }
      if (current_view.get() != 1 || edit_state.get() != 1) {
        vykon.set( packet.vykon, network );
      }
      if (current_view.get() != 5 || edit_state.get() != 1) {
        rezim.set( packet.rezim, network );
      }
      if (packet.request) {
        repeat_send.start(2000);
      }
      //keep_alive.restart();
      invalidate_sensor.restart();
      Serial.println(packet.ciel);
      Serial.println(packet.sensor);
    }
  }
}
