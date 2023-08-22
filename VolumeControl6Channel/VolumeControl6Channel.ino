#include <PT2258.h>
#include <Wire.h>
#include <IRremote.h>
#include <Value.h>
#include <Timer.h>
#include <EEPROM.h>
#include <TM1637Display.h>

#define DEBUG 1
#define RECV_PIN 7
#define HOLD 0xFFFFFFFF
#define DELAY 250
#define CLK 2
#define DIO 3

decode_results results;
unsigned long last_code;
unsigned long delay_t = DELAY;
IRrecv irrecv(RECV_PIN);
PT2258 pt2258(0x88);
Value<int> volume(0, -79, 0, true);
int volume_eeprom = 0;
Timer save_timer(10000, false, false);
TM1637Display display(CLK, DIO);

void setup() {
  Serial.begin(115200);  // start the Serial communication
  Serial.println("Starting...");
  Wire.begin();  // start the I2C bus
  //Wire.setClock(100000);  // the PT2258 is specified to work with a bus clock of 100kHz

  if (!pt2258.begin()) {  // initialise the PT2258
    Serial.println("PT2258: connection error!");
    while (1) delay(10);
  }

  irrecv.enableIRIn();
  pt2258.volumeAll(-79);  // at the beginning the volume is by default at 100%. Set the desired volume at startup before unmuting next
  pt2258.mute(false);   // the mute is active when the device powers up. Unmute it to ear the sound

  EEPROM.get(0, volume_eeprom);
  if (volume_eeprom != 0) {
    volume.set(volume_eeprom);
  }
  display.setBrightness(0x0f);
  display.clear();
  display.showNumberDec(volume.get(), false, 3, 1);
  Serial.println("Started!");
}

void loop() {

  if (irrecv.decode(&results)) {
    if (DEBUG) Serial.println(results.value, HEX);

    if (results.value == HOLD) {
      results.value = last_code;
      delay_t = 100;
    } else {
      last_code = results.value;
      delay_t = DELAY;
    }

    if (results.value == 0xC03F20DF) {  // +
      volume.inc();
      if (DEBUG) Serial.println("Stisknuto VOL+");
    } else if (results.value == 0xC03F10EF) {  // -
      volume.dec();
      if (DEBUG) Serial.println("Stisknuto VOL-");
    }  // else mute

    irrecv.resume();
  }

  if (volume.change()) {
    pt2258.attenuationAll( volume.get() );
    pt2258.mute(volume.get() > -79 ? false : true);
    display.showNumberDec(volume.get(), false, 3, 1);
    Serial.println(volume.get());
    save_timer.restart();
  }

  if (save_timer.tick()) {
    volume_eeprom = volume.get();
    EEPROM.put(0, volume_eeprom);
  }

  delay(delay_t);
}
