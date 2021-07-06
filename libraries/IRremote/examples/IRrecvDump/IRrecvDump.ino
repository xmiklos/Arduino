/*
 * IRremote: IRrecvDump - dump details of IR codes with IRrecv
 * An IR detector/demodulator must be connected to the input RECV_PIN.
 * Version 0.1 July, 2009
 * Copyright 2009 Ken Shirriff
 * http://arcfn.com
 * JVC and Panasonic protocol added by Kristian Lauszus (Thanks to zenwheel and other people at the original blog post)
 * LG added by Darryl Smith (based on the JVC protocol)
 */

#include <IRremote.h>

/* 
*  Default is Arduino pin D11. 
*  You can change this to another available Arduino Pin.
*  Your IR receiver should be connected to the pin defined here
*/
int RECV_PIN = 7;

IRrecv irrecv(RECV_PIN);

decode_results results;

void setup()
{
  Serial.begin(9600);
  while(!Serial);
  Serial.println("start");
  irrecv.enableIRIn(); // Start the receiver
}

void dump(decode_results *results) {
  // Dumps out the decode_results structure.
  // Call this after IRrecv::decode()
  int count = results->rawlen;
  if (results->decode_type == UNKNOWN) {
    Serial.print("Unknown encoding: ");
  }
  else if (results->decode_type == NEC) {
    Serial.print("Decoded NEC: ");

  }
  else if (results->decode_type == SONY) {
    Serial.print("Decoded SONY: ");
  }
  else if (results->decode_type == RC5) {
    Serial.print("Decoded RC5: ");
  }
  else if (results->decode_type == RC6) {
    Serial.print("Decoded RC6: ");
  }
  else if (results->decode_type == PANASONIC) {
    Serial.print("Decoded PANASONIC - Address: ");
    Serial.print(results->address, HEX);
    Serial.print(" Value: ");
  }
  else if (results->decode_type == LG) {
    Serial.print("Decoded LG: ");
  }
  else if (results->decode_type == JVC) {
    Serial.print("Decoded JVC: ");
  }
  else if (results->decode_type == AIWA_RC_T501) {
    Serial.print("Decoded AIWA RC T501: ");
  }
  else if (results->decode_type == WHYNTER) {
    Serial.print("Decoded Whynter: ");
  }
  else if (results->decode_type == S929) {
    Serial.print("Decoded S929: ");
  }
  Serial.print(results->value, HEX);
  Serial.print(" (");
  Serial.print(results->bits, DEC);
  Serial.println(" bits)");
  Serial.print("Raw (");
  Serial.print(count, DEC);
  Serial.print("): ");

  for (int i = 1; i < count; i++) {
    if (i & 1) {
      Serial.print(results->rawbuf[i]*USECPERTICK, DEC);
    }
    else {
      Serial.write('-');
      Serial.print((unsigned long) results->rawbuf[i]*USECPERTICK, DEC);
    }
    Serial.print(" ");
  }
  Serial.println();
}

void loop() {
  if (irrecv.decode(&results)) {
    //Serial.println(results.value, BIN);
    //dump(&results);
    for (int i = 3; i >= 0; --i) {
      unsigned long x = (results.value >> (i*8)) & 0xFF;
      for (int j = 7; j >=0; --j) {
        if (x & (1 << j)) {
          break;
        } else {
          Serial.print("0");
        }
      }
      Serial.print(x, BIN);
      Serial.print(" ");
    }
    Serial.println();
    for (int i = 3; i >= 0; --i) {
      unsigned long x = (results.value >> (i*8)) & 0xFF;

      Serial.print(x, DEC);
      Serial.print(" ");
    }
    
    unsigned int on = results.value >> 31;
    unsigned int val = (results.value >> 24) & 0x7F;
    Serial.print(on);
    Serial.print(" ");
    Serial.println(val);
    irrecv.resume(); //Receive the next value
  }
  delay(10);
}
