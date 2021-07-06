#include <avr/wdt.h>

Controller control;
uint8_t zprava[VW_MAX_MESSAGE_LEN];
Timer t;

void setup() {
  Serial.begin(9600);
  //while (!Serial);
  magnet.begin();
  control.start();

  vw_set_ptt_inverted(true);
  // nastavení rychlosti přenosu v bitech za sekundu
  vw_setup(1000);
  // nastavení čísla datového pinu pro vysílač
  vw_set_tx_pin(REMOTE_PIN);
  t.start(GET_TEMP_SPEED);
  wdt_enable(WDTO_2S);
}

void loop() {
	wdt_reset();
  //unsigned long time = millis();

  control.stop_check();
  //unsigned long time2 = millis();
  //printf("%d\n", time2 - time);

  if (t.stop_check()) {
    //Serial.println("sending...");
    vw_send((uint8_t*)&global_temp, sizeof(global_temp));
    vw_wait_tx();
    //Serial.println("sent...");
    t.start(GET_TEMP_SPEED);
  }
}


