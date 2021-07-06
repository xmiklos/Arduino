#include <Value.h>
#include <Timer.h>
#include <VirtualWire.h>

Value<int> light_on(0, true);
Timer button_pressed_debounce(800, false, false);

void button_pressed() {
  if (!button_pressed_debounce.started()) {
    button_pressed_debounce.start();
    light_on.set( !light_on.get() );
  }
  Serial.println("button");
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(3, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(2,  INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(2), button_pressed, CHANGE);

  vw_set_ptt_inverted(true);
  vw_setup(100);
  vw_set_rx_pin(4);
  vw_rx_start();
}

uint8_t buffer[ VW_MAX_MESSAGE_LEN ];
uint8_t buflen = VW_MAX_MESSAGE_LEN;

typedef struct {
  int target;
  int value;
} packet;

void loop() {
  button_pressed_debounce.tick();

  if (light_on.change()) {
    if (light_on.get()) {
      digitalWrite(3, HIGH);
      digitalWrite(5, LOW);
    } else {
      digitalWrite(3, LOW);
      digitalWrite(5, HIGH);
    }
  }

  if (vw_get_message(buffer, &buflen)) {
    button_pressed_debounce.start();
    packet p = * ((packet*)buffer);
    Serial.println("got");
    if (p.target == 1) {
      light_on.set( !light_on.get() );
    }
  }
}
