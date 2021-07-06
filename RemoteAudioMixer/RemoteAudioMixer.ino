#include <LiquidCrystal_I2C.h>
#include <RotaryEncoder.h>
#include <Timer.h>
#include <Value.h>

#define BUFFER_SIZE         50

#define PIN_ENCODER_SW1      2
#define PIN_ENCODER_A1       4
#define PIN_ENCODER_B1       5

#define PIN_ENCODER_SW2      A2
#define PIN_ENCODER_A2       6
#define PIN_ENCODER_B2       7

#define PIN_ENCODER_SW3      12
#define PIN_ENCODER_A3       8
#define PIN_ENCODER_B3       9

#define PIN_ENCODER_SW4      3
#define PIN_ENCODER_A4       10
#define PIN_ENCODER_B4       11

#define BUTTON_UP             A1
#define BUTTON_DOWN           A0

LiquidCrystal_I2C lcd(0x27, 20, 4);
uint8_t arrow_down_char[8]  = {  0x04, 0x04, 0x04, 0x04, 0x15, 0x0E, 0x04, 0x00 };
uint8_t arrow_up_char[8]    = {  0x04, 0x0E, 0x15, 0x04, 0x04, 0x04, 0x04, 0x00 };

RotaryEncoder encoders[] = {
  RotaryEncoder(PIN_ENCODER_B4, PIN_ENCODER_A4),
  RotaryEncoder(PIN_ENCODER_B3, PIN_ENCODER_A3),
  RotaryEncoder(PIN_ENCODER_B2, PIN_ENCODER_A2),
  RotaryEncoder(PIN_ENCODER_B1, PIN_ENCODER_A1)
};

Timer keep_alive(5000, true, true); // keep serial alive
Timer fall_asleep(30000, true, true);    // while running, its alive
Timer debounce0(300, false, false);
Timer debounce1(300, false, false);
Timer debounce2(300, false, false);

Value<int> light_on(1);
Value<int> up(1);
Value<int> down(1);

Value<int> sw[] = {
  Value<int>(1),
  Value<int>(1),
  Value<int>(1),
  Value<int>(1),
};

ISR (PCINT0_vect) { // handle pin change interrupt for D8 to D13 here
  encoders[0].tick();
  encoders[1].tick();
  if (!debounce0.started()) {
    debounce0.start();
    sw[2].set( digitalRead(PIN_ENCODER_SW3) );
    sw[2].set( 1 );
    light_on.set(1);
  }
}

ISR (PCINT1_vect) { // handle pin change interrupt for A0 to A5 here
  if (!debounce1.started()) {
    debounce1.start();
    //Serial.println("vect1");
    down.set( digitalRead(BUTTON_DOWN) );
    down.set( 1 );
    up.set( digitalRead(BUTTON_UP) );
    up.set( 1 );
    sw[1].set( digitalRead(PIN_ENCODER_SW2) );
    sw[1].set( 1 );
    light_on.set(1);
  }
}

ISR (PCINT2_vect) // handle pin change interrupt for D0 to D7 here
{
  encoders[2].tick();
  encoders[3].tick();
  if (!debounce2.started()) {
    debounce2.start();
    //Serial.println("vect2");
    sw[0].set( digitalRead(PIN_ENCODER_SW1) );
    sw[0].set( 1 );
    sw[3].set( digitalRead(PIN_ENCODER_SW4) );
    sw[3].set( 1 );
    light_on.set(1);
  }
}

enum Commands
{
  AddSession = 0,
  ClearSessions = 1,
  Flash = 2,
  Thunder = 3,
  SetVolume = 4,
};

typedef struct {
  int8_t volume;
  char name[15];
} session;

const int session_max   = 10;
session sessions[ session_max ];
int mute[ session_max ] = {0};
int session_count       = 0;
int session_offset      = 0;
bool should_render      = false;

void setup() {
  pinMode(PIN_ENCODER_SW1, INPUT_PULLUP);
  pinMode(PIN_ENCODER_SW2, INPUT_PULLUP);
  pinMode(PIN_ENCODER_SW3, INPUT_PULLUP);
  pinMode(PIN_ENCODER_SW4, INPUT_PULLUP);
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  
  PCICR |= (1 << PCIE2) | (1 << PCIE0) | (1 << PCIE1);
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19) | (1 << PCINT20) | (1 << PCINT21) | (1 << PCINT22) | (1 << PCINT23);
  PCMSK0 |= (1 << PCINT0) | (1 << PCINT1) | (1 << PCINT2) | (1 << PCINT3) | (1 << PCINT4);
  PCMSK1 |= (1 << PCINT8) | (1 << PCINT9) | (1 << PCINT10);

  Serial.setTimeout(15000);
  Serial.begin(9600);

  lcd.begin();
  lcd.createChar(2, arrow_down_char);
  lcd.createChar(3, arrow_up_char);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting...");
}

char buffer[ BUFFER_SIZE ];
char printbuff[20];

size_t bytes_read = 0;

void loop() {
  debounce0.tick();
  debounce1.tick();
  debounce2.tick();

  for (int i = 0; i < 4; i++) {
    int session = i + session_offset;
    if (sw[i].change() && session < session_count) {
      if (mute[session]) { // is muted
        set_volume(session, mute[session]);
        mute[session] = 0;
      } else { // not muted
        mute[session] = sessions[ session ].volume;
        set_volume(session, 0);
      }
    }
  }

  if (up.change()) {
    if (session_offset > 0) {
      session_offset--;
      should_render = true;
    }
  }
  
  if (down.change()) {
    if (session_offset < session_count - 4) {
      session_offset++;
      should_render = true;
    }
  }
  
  for (int i = 0; i < 4; ++i) {
    int d = encoders[i].getDirection();
    if (d) {
      int session = i + session_offset;
      if (session < session_count) { // volume change
        set_volume(session, constrain( sessions[ session ].volume + (5 * d), 0, 100));
        mute[ session ] = 0;
      }
    }
  }
  
  if (Serial.available()) {
      int c = Serial.read();
      bytes_read++;
      if (c == Flash) {
        Serial.write( (char)Thunder );
        lcd.clear();
        lcd.setCursor ( 0, 0 );
        lcd.print("Connected!   ");
      } else if (c == AddSession) {
        size_t read = Serial.readBytes((byte*)(sessions + session_count), sizeof(session));
        if (read == sizeof(session)) {
          session_count++;
          should_render = true;
        }
        bytes_read += read;
      } else if (c == ClearSessions) {
        session_count   = 0;
        session_offset  = 0;
        should_render = true;
      }
      
      fall_asleep.restart();
      light_on.set(1);

      //Serial.flush();
  } else {

    if (keep_alive.tick()) {
      Serial.write( (char)Flash );
    }

    if (fall_asleep.tick()) {
      light_on.set(0);
      session_count   = 0;
      session_offset  = 0;
      should_render = true;
    }

    if (should_render) {
      render();
    }
  }

  if (light_on.change()) {
    if (light_on.get()) {
      lcd.backlight();
    } else {
      lcd.noBacklight();
    }
  }
  //delay(1);
}

void set_volume(int index, int volume) {
  sessions[ index ].volume = volume;
  lcd.setCursor(15, index - session_offset);
  sprintf(printbuff, "%3d%%", sessions[index].volume);
  lcd.print(printbuff);
  Serial.write((byte)SetVolume);
  Serial.write((byte)index);
  Serial.write((byte)sessions[ index ].volume);
}

void render() {
  
  for (int i = 0; i < 4; ++i) {
    if (i + session_offset < session_count) {
      lcd.setCursor(0, i);
      lcd.print("                    ");
      lcd.setCursor(0, i);
      lcd.print(sessions[i + session_offset ].name);
      lcd.setCursor(15, i);
      sprintf(printbuff, "%3d%%", sessions[i + session_offset ].volume);
      lcd.print(printbuff);

      if (i == 0) {
        if (session_offset > 0) {
          lcd.write(3);
        } else {
          lcd.print(" ");
        }
      }

      if (i == 3) {
        if (session_offset < session_count - 4) {
          lcd.write(2);
        } else {
          lcd.print(" ");
        }
      }
    } else {
      lcd.setCursor(0, i);
      lcd.print("                    ");
    }
  }

  should_render = false;
}


