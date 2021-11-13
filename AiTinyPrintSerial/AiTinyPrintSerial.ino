/*
  Print a table of ASCII characters to the Serial Monitor

  By J.Sleeman (sparks@gogo.co.nz)
  MicroCore support by MCUdude
  ------------------------------------------------------------------------------

  [ See diagram: https://github.com/MCUdude/MicroCore#minimal-setup ]

  Prints out byte values in all possible formats:
    As raw binary values
    As ASCII-encoded decimal, hex, octal, and binary values

  For more on ASCII, see http://www.asciitable.com and http://en.wikipedia.org/wiki/ASCII

  RECOMMENDED SETTINGS FOR THIS SKETCH
  ------------------------------------------------------------------------------

  Tools > Board          : ATtiny13
  Tools > BOD            : [Use any BOD level you like]
  Tools > Clock          : [Use any clock except 128 kHz]
  Tools > Timing         : Micros disabled

  SERIAL REMINDER
  ------------------------------------------------------------------------------
  The baud rate is IGNORED on the ATtiny13 due to using a simplified serial.
  The actual Baud Rate used is dependant on the processor speed.
  Note that you can specify a custom baud rate if the following ones does
  not fit your application.

  THESE CLOCKS USES 115200 BAUD:   THIS CLOCK USES 57600 BAUD:
  (External)  20 MHz               (Internal) 4.8 MHz
  (External)  16 MHz
  (External)  12 MHz
  (External)   8 MHz
  (Internal) 9.6 MHz

  THESE CLOCKS USES 19200 BAUD:    THIS CLOCK USES 9600 BAUD:
  (Internal) 1.2 MHz               (Internal) 600 KHz
  (External)   1 MHz

  If you get garbage output:
   1. Check baud rate as above
   2. Check if you have anything else connected to TX/RX like an LED
   3. Check OSCCAL (see OSCCAL tuner example)
*/

#include <EEPROM.h>

void setup()
{


}

void loop()
{
    Serial.println(millis());
delay(1000);
}
