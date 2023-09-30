#include <Keyboard.h>
#include <IRremote.h>

#define RECV_PIN 7
#define DEBUG 1
#define HOLD 0xFFFFFFFF
#define DELAY 250

IRrecv irrecv(RECV_PIN);
decode_results results;
unsigned long last_code;
unsigned long delay_t = DELAY;

void setup() {
  if (DEBUG) {
    Serial.begin(9600);
    while(!Serial);
    Serial.println("Keyboard Setup!");
  }
  irrecv.enableIRIn();
  Keyboard.begin();
}

void loop() {
  if (irrecv.decode(&results)) {
    if (DEBUG) Serial.println(results.value, HEX);
    
    if (results.value == HOLD) {
      results.value = last_code;
      delay_t = 150;
    } else {
      last_code = results.value;
      delay_t = DELAY;
    }
    
    uint8_t c = map( results.value );
    if (c) {
      Keyboard.write( c );
      delay(10);
    }

    irrecv.resume();
  }
  delay(delay_t);
}

uint8_t map(unsigned long code) {

  switch(code) {
    case 0xFFA25D:
    case 0xFB00B24D:
      if (DEBUG) Serial.println("Stisknuto CH-");
      return 'M';
    case 0xFF629D:
    case 0xFB009A65:
    case 0xC03F00FF:
      if (DEBUG) Serial.println("Stisknuto CH");
      return 'O'; // System Power Down
    case 0xFFE21D:
    case 0xFB008A75:
      if (DEBUG) Serial.println("Stisknuto CH+");
      return 'I';
    case 0xFF22DD:
    case 0xFB00B04F:
    case 0xC03F42BD:
      if (DEBUG) Serial.println("Stisknuto PREV");
      return 'R';
    case 0xFF02FD:
    case 0xFB008877:
    case 0xC03F22DD:
      if (DEBUG) Serial.println("Stisknuto NEXT");
      return 'F';
    case 0xFFC23D:
    case 0xFB00708F:
    case 0xFB0048B7:
    case 0xC03FC23D:
      if (DEBUG) Serial.println("Stisknuto PLAY/PAUSE");
      return ' ';
    case 0xFFE01F:
    case 0xFB00C837:
    case 0xC03FD827:
      if (DEBUG) Serial.println("Stisknuto VOL-");
      return '-'; // Keyboard Volume Down
    case 0xFFA857:
    case 0xFB00A857:
    case 0xC03F7887:
      if (DEBUG) Serial.println("Stisknuto VOL+");
      return '+'; // Keyboard Volume Up
    case 0xFF906F:
    case 0xFB009867:
    case 0xcC03F28D7:
      if (DEBUG) Serial.println("Stisknuto EQ");
      return 'U'; // Keyboard Mute
    case 0xFF6897:
    case 0xFB00AA55:
    case 0xC03FC03F:
      if (DEBUG) Serial.println("Stisknuto 0");
      return 'c';
    case 0xFF9867:
    case 0xFB0042BD:
    case 0xC03F8A75:
      if (DEBUG) Serial.println("Stisknuto 100+");
      return KEY_HOME;
    case 0xFFB04F:
    case 0xFB0002FD:
    case 0xC03F02FD:
      if (DEBUG) Serial.println("Stisknuto 200+");
      return KEY_END;
    case 0xFF30CF:
    case 0xFB00A25D:
    case 0xC03F0AF5:
      if (DEBUG) Serial.println("Stisknuto 1");
      return KEY_BACKSPACE;
    case 0xFF18E7:
    case 0xFB00609F:
    case 0xC03F30CF:
      if (DEBUG) Serial.println("Stisknuto 2");
      return KEY_UP_ARROW;
    case 0xFF7A85:
    case 0xFB0000FF:
      if (DEBUG) Serial.println("Stisknuto 3");
      return KEY_PAGE_UP;
    case 0xFF10EF:
    case 0xFB005AA5:
    case 0xC03F10EF:
      if (DEBUG) Serial.println("Stisknuto 4");
      return KEY_LEFT_ARROW;
    case 0xFF38C7:
    case 0xFB0058A7:
    case 0xC03FC837:
      if (DEBUG) Serial.println("Stisknuto 5");
      return KEY_RETURN;
    case 0xFF5AA5:
    case 0xFB00D827:
    case 0xC03F20DF:
      if (DEBUG) Serial.println("Stisknuto 6");
      return KEY_RIGHT_ARROW;
    case 0xFF42BD:
    case 0xFB00A05F:
    case 0xC03FF20D:
      if (DEBUG) Serial.println("Stisknuto 7");
      return KEY_ESC;
    case 0xFF4AB5:
    case 0xFB006897:
    case 0xC03F08F7:
      if (DEBUG) Serial.println("Stisknuto 8");
      return KEY_DOWN_ARROW;
    case 0xFF52AD:
    case 0xFB00C03F:
      if (DEBUG) Serial.println("Stisknuto 9");
      return KEY_PAGE_DOWN;
    default:
      return 0;
  }
}
