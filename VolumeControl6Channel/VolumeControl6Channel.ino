#include <PT2258.h>
#include <Wire.h>
#include <IRremote.h>
#include <Value.h>
#include <Timer.h>
#include <EEPROM.h>
#include <TM1637Display.h>

#define DEBUG 0
#define RECV_PIN 7
#define HOLD 0x0
#define DELAY 200
#define CLK 2
#define DIO 3
#define RELAY 5

unsigned long last_code;
unsigned long delay_t = DELAY;

PT2258 pt2258(0x88);
Value<int> volume(0, 0, 79, true);
int volume_eeprom = 0;
Timer save_timer(10000, false, false);
Timer relay_timer(2000, false, false);
Timer mute_timer(2000, false, false);
TM1637Display display(CLK, DIO);
bool mute = false;
bool relay_on = false;
decode_results results;
IRrecv irrecv(RECV_PIN);

void setup() {
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, HIGH);

  Serial.begin(115200);  // start the Serial communication
  Serial.println("Starting...");
  Wire.begin();  // start the I2C bus

  Serial.println("Begin...");
  if (!pt2258.begin()) {  // initialise the PT2258
    Serial.println("PT2258: connection error!");
    while (1) delay(10);
  }

  irrecv.enableIRIn();
  pt2258.volumeAll(79);
  pt2258.mute(false);

  EEPROM.get(0, volume_eeprom);
  if (volume_eeprom != 0) {
    volume.set(volume_eeprom);
  }
  display.setBrightness(0);
  display.clear();
  display.showNumberDec(volume.get(), false, 2, 1);
  Serial.println("Started!");
}

void loop() {
  
  if (irrecv.decode(&results)) {
    if (DEBUG) Serial.println(results.value, HEX);

    /*
    A084B2A2 - BLUE
    9774845A - YELLOW
    2B77163A - GREEN
    4ED5545A - RED
    */

    if (results.value == 0xA084B2A2) {  // +
      volume.set(volume.get() + 2);
      if (mute) {
        mute = false;
        pt2258.mute(mute);
      }
      if (DEBUG) Serial.println("Stisknuto VOL+");
    } else if (results.value == 0x9774845A) {  // -
      volume.set(volume.get() - 2);
      if (mute) {
        mute = false;
        pt2258.mute(mute);
      }
      if (DEBUG) Serial.println("Stisknuto VOL-");

    } else if (results.value == 0x2B77163A && !mute_timer.started()) {  // mute
      mute_timer.start();
      mute = !mute;
      pt2258.mute(mute);
      if (DEBUG) Serial.println("Stisknuto Mute");

    } else if (results.value == 0x4ED5545A && !relay_timer.started()) {  // RELAY ON/OFF
      relay_timer.start();
      relay_on = !relay_on;
      if (relay_on) {
        digitalWrite(RELAY, LOW);
        display.showNumberDec(volume.get(), false, 2, 1);
      } else {
        digitalWrite(RELAY, HIGH);
        display.clear();
      }
      if (DEBUG) Serial.println("Stisknuto Rele ON/OFF");

    }

    irrecv.resume();
  }

  if (volume.change()) {
    pt2258.attenuationAll(79 - volume.get());
    display.showNumberDec(volume.get(), false, 2, 1);
    Serial.println(volume.get());
    save_timer.restart();
  }

  if (save_timer.tick()) {
    volume_eeprom = volume.get();
    EEPROM.put(0, volume_eeprom);
  }
  mute_timer.tick();
  relay_timer.tick();

  delay(delay_t);
}
