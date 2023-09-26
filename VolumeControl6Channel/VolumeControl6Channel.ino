#include <PT2258.h>
#include <Wire.h>
#include <IRremote.h>
#include <Value.h>
#include <Timer.h>
#include <EEPROM.h>
#include <TM1637Display.h>

#define DEBUG 1
#define RECV_PIN 7
#define HOLD 0x0
#define DELAY 250
#define CLK 2
#define DIO 3
#define RELAY 5

unsigned long last_code;
unsigned long delay_t = DELAY;

PT2258 pt2258(0x88);
Value<int> volume(0, 0, 79, true);
int volume_eeprom = 0;
Timer save_timer(10000, false, false);
TM1637Display display(CLK, DIO);

void setup() {
  Serial.begin(115200);  // start the Serial communication
  Serial.println("Starting...");
  Wire.begin();  // start the I2C bus
  //Wire.setClock(100000);  // the PT2258 is specified to work with a bus clock of 100kHz
  Serial.println("Begin...");
  if (!pt2258.begin()) {  // initialise the PT2258
    Serial.println("PT2258: connection error!");
    while (1) delay(10);
  }

  IrReceiver.begin(RECV_PIN);
  pt2258.volumeAll(79);
  pt2258.mute(false);

  EEPROM.get(0, volume_eeprom);
  if (volume_eeprom != 0) {
    volume.set(volume_eeprom);
  }
  display.setBrightness(2);
  display.clear();
  display.showNumberDec(volume.get(), false, 2, 1);
  Serial.println("Started!");
}

void loop() {

  if (IrReceiver.decode()) {
    if (DEBUG) Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX);

    if (IrReceiver.decodedIRData.decodedRawData == HOLD) {
      IrReceiver.decodedIRData.decodedRawData = last_code;
      delay_t = 100;
    } else {
      last_code = IrReceiver.decodedIRData.decodedRawData;
      delay_t = DELAY;
    }

    if (IrReceiver.decodedIRData.decodedRawData == 0xE401BF) {  // +
      volume.set( volume.get() + 2 );
      if (DEBUG) Serial.println("Stisknuto VOL+");
    } else if (IrReceiver.decodedIRData.decodedRawData == 0xD8027F) {  // -
      volume.set( volume.get() - 2 );
      if (DEBUG) Serial.println("Stisknuto VOL-");
    }  // else mute

    // FF000F - RED
    // E8017F - green
    // D8027F - yellow
    // E401BF - blue
    

    IrReceiver.resume();
  }

  if (volume.change()) {
    pt2258.attenuationAll( 79 - volume.get() );
    pt2258.mute(volume.get() > 0 ? false : true);
    display.showNumberDec(volume.get(), false, 2, 1);
    Serial.println(volume.get());
    save_timer.restart();
  }

  if (save_timer.tick()) {
    volume_eeprom = volume.get();
    EEPROM.put(0, volume_eeprom);
  }

  delay(delay_t);
}
