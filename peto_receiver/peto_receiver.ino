#define REQUIRESALARMS false
#include <TM1637Display.h>
#include <VirtualWire.h>
#include <Encoder.h>

/* pins */

#define LED_PIN 11
#define TEMP_POT_PIN A1
#define TEMP_POT_TRESHOLD 18
#define CLK 4
#define DIO 5
#define REMOTE_PIN 6
#define TEMP_MIN 0
#define TEMP_MAX 100

#define ENC_A 3
#define ENC_B 7
#define ENC_SW 2

#define BLINK_SPEED  500

uint8_t buf[VW_MAX_MESSAGE_LEN];
uint8_t buflen = VW_MAX_MESSAGE_LEN;
uint8_t global_temp;
bool got = false;
bool got2 = false;

volatile byte state_change = false;

void sw() {
  state_change = true;
}

byte get_state() {
  byte s = state_change;
  if (s) {
    state_change = false;
  }
  return s;
}
