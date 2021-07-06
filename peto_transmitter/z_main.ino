#include <avr/wdt.h>

Controller control;
Timer t;

void setup() {
  Serial.begin(9600);

      // nastavení typu bezdrátové komunikace
      vw_set_ptt_inverted(true);
      // nastavení rychlosti přenosu v bitech za sekundu
      
      // nastavení čísla datového pinu pro vysílač
      vw_setup(100);
  //while (!Serial);
  control.start();

  wdt_enable(WDTO_2S);
}

void loop() {
	wdt_reset();
  //unsigned long time = millis();

  control.stop_check();
  //unsigned long time2 = millis();
  //printf("%d\n", time2 - time);
}


