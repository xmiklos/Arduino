#include "EncoderStepCounter.h"
#include <Value.h>
#define ENCODER_PIN1 2
#define ENCODER_INT1 digitalPinToInterrupt(ENCODER_PIN1)
#define ENCODER_PIN2 3
#define ENCODER_INT2 digitalPinToInterrupt(ENCODER_PIN2)

// Create instance for one full step encoder
EncoderStepCounter encoder(ENCODER_PIN1, ENCODER_PIN2);
// Use the following for half step encoders
//EncoderStepCounter encoder(ENCODER_PIN1, ENCODER_PIN2, HALF_STEP);

void setup() {
  Serial.begin(9600);
  Serial.println("start");
  // Initialize encoder
  encoder.begin();
  // Initialize interrupts
  pinMode(A0, INPUT_PULLUP);
  PCICR |= (1 << PCIE1);    // This enables Pin Change Interrupt 1 that covers the Analog input pins or Port C.
  PCMSK1 |= (1 << PCINT8);  // This enables the interrupt for pin 2 and 3 of Port C.
  attachInterrupt(ENCODER_INT1, interrupt, CHANGE);
  attachInterrupt(ENCODER_INT2, interrupt, CHANGE);
}

// Call tick on every change interrupt
ISR(PCINT1_vect) {
  //encoder.tick(); // just call tick() to check the state.
  if (digitalRead(A0) == LOW) {
    Serial.println("isr");
  }
}
void interrupt() {
  encoder.tick();
}
char direction() {
  signed char pos = encoder.getPosition();
  if (pos != 0) {
    encoder.reset();
    //Serial.println( pos );
  }
  
  return pos;
}

Value<int> position(0, 0, 256);

void loop() {
  if (position.change( position.get() + direction() )) {
    Serial.println(position.get());
  }
  
  
}
