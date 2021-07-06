/*
 * IRremote: IRrecvDemo - demonstrates receiving IR codes with IRrecv
 * An IR detector/demodulator must be connected to the input RECV_PIN.
 * Version 0.1 July, 2009
 * Copyright 2009 Ken Shirriff
 * http://arcfn.com
 */

#include <IRremote.h>

int RECV_PIN = 7;

IRrecv irrecv(RECV_PIN);

decode_results results;

void setup()
{
  Serial.begin(9600);
  // In case the interrupt driver crashes on setup, give a clue
  // to the user what's going on.
  Serial.println("Enabling IRin");
  irrecv.enableIRIn(); // Start the receiver
  Serial.println("Enabled IRin");
}

void loop() {
  if (irrecv.decode(&results)) {
    Serial.println(results.value, BIN);
    /*for (unsigned int i = 0; i < 4; ++i) {
      unsigned long mybyte = (results.value >> (8*i)) & 0xff;
      Serial.print(mybyte);
      Serial.print(" ");
    }*/
    for (unsigned int i = 0; i < results.rawlen; ++i) {
      Serial.print(results.rawbuf[i]);
      Serial.print(" ");
    }
    Serial.println();
    irrecv.resume(); // Receive the next value
  }
  delay(100);
}
