#include <VirtualWire.h>
#include <Timer.h>
#include "LowPower.h"

void wake_up()
{
    // Just a handler for the pin interrupt.
}

void setup() {
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  vw_set_ptt_inverted(true); // Required for DR3100
  vw_setup(100);
  vw_set_tx_pin(5);

  pinMode(2,  INPUT_PULLUP);
  digitalWrite(13, LOW);
}

typedef struct {
  int target;
  int value;
} packet;

void loop() {
  
  // sleep
  attachInterrupt(0, wake_up, LOW);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  detachInterrupt(0);

  // wake up

  digitalWrite(13, HIGH);
  packet p = { 1, 1 };
  vw_send((uint8_t *) &p, sizeof(p));
  vw_wait_tx();
  digitalWrite(13, LOW);
}
