#include "MCP41HVX1.h"
#include "Value.h"
#include <Timer.h>
#include <EEPROM.h>
#include "EncoderStepCounter.h"
#include <AverageValue.h>
#include "ACS712.h"
#include <LiquidCrystal_I2C.h>

#define VOLTMETER     A1
#define AMPMETER      A2
#define REF_VOLTAGE   4.96
#define OFFSET        1.938
#define HIGH_VOLTAGE  18.72
#define HIGH_LOW      (HIGH_VOLTAGE / REF_VOLTAGE)
#define REF_1_1024    (REF_VOLTAGE / 1024)
#define REF_1_256     (REF_VOLTAGE / 256) // 4.86
#define CONV_RATIO    REF_1_1024 * HIGH_LOW

#define BUTTON        6
#define ENCODER_PIN1  2
#define ENCODER_INT1  digitalPinToInterrupt(ENCODER_PIN1)
#define ENCODER_PIN2  3
#define ENCODER_INT2  digitalPinToInterrupt(ENCODER_PIN2)
#define POT_CHANGE_DELAY  5 // microseconds
#define LINE_MIN      0
#define LINE_MAX      2

EncoderStepCounter    encoder(ENCODER_PIN1, ENCODER_PIN2);
MCP41HVX1             pot( 10 );
AverageValue<float>   current(10);
Value<int>            position( 0, 0, 255 );
Value<int>            voltage_limit( 50, 19, 187 );
Value<int>            current_limit( 10, 0, 30 );
ACS712                current_sensor(ACS712_05B, AMPMETER);
LiquidCrystal_I2C     lcd(0x27, 16, 2);
Value<int>            line_selected( 2, LINE_MIN, LINE_MAX );
Value<int>            line_active( 0, 0, 1 );

byte arrow[8] = {
  0b10000,
  0b11000,
  0b11100,
  0b11110,
  0b11110,
  0b11100,
  0b11000,
  0b10000
};

byte switch_open[8] = {
  0b00000,
  0b00010,
  0b00100,
  0b01000,
  0b10001,
  0b10001,
  0b10001,
  0b10001
};

byte switch_close[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b11111,
  0b10001,
  0b10001,
  0b10001
};

Timer                 button_debounce(200, false, false);
Timer                 save(3000, false, false);

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  Serial.println("start");
  lcd.begin();
  lcd.createChar(1, arrow);
  lcd.createChar(2, switch_open);
  lcd.createChar(3, switch_close);
  lcd.backlight();
  lcd.print("Initializing");
  lcd.setCursor ( 0, 1 );
  lcd.print("warp core...");
  delay(500);

  //save_eeprom();
  load_eeprom();

  encoder.begin();
  pinMode(A0, INPUT_PULLUP);
  PCICR |= (1 << PCIE1);
  PCMSK1 |= (1 << PCINT8);
  attachInterrupt(ENCODER_INT1, interrupt, CHANGE);
  attachInterrupt(ENCODER_INT2, interrupt, CHANGE);

  pinMode(BUTTON, OUTPUT);
  digitalWrite(BUTTON, LOW);

  current_sensor.calibrate();
}

ISR(PCINT1_vect) {
  if (digitalRead(A0) == LOW && !button_debounce.started()) {
    button_debounce.start();

    if (line_selected.get() == 2) {
      digitalWrite(BUTTON, digitalRead(BUTTON) == LOW ? HIGH : LOW);
    } else {
      line_active.set( line_active.get() ? 0 : 1 );
    }
  }
}

void interrupt() {
  encoder.tick();
}

char direction() {
  signed char pos = encoder.getPosition();
  if (pos != 0) {
    encoder.reset();
  }

  return pos;
}

Timer                 report(200);
Timer                 cc(10000, false, false);
Timer                 cv(10000, false, false);
void loop() {

  cc.tick();
  cv.tick();
  current.push( abs( current_sensor.getCurrentDC() ) );

  if (line_active.get()) {
    if (line_selected.get() == 0) {
      voltage_limit.set( voltage_limit.get() + direction() );
    } else {
      current_limit.set( current_limit.get() + direction() );
    }
  } else {
    int d = line_selected.get() + direction();

    line_selected.set( d > LINE_MAX ? LINE_MIN : d < LINE_MIN ? LINE_MAX : d );
  }

  int vlc = voltage_limit.change();
  int clc = current_limit.change();
  if (vlc || clc) {
    save.restart();
  }

  double u_measured = get_voltage();
  double u_limit    = voltage_limit.get() / 10.0;
  double i_measured = current.average();
  double i_limit    = current_limit.get() / 10.0;
  double r_measured = u_measured / i_measured;

  if (i_measured > i_limit) {
    pot.WiperDecrement();

    Serial.println("CC DEC");

    Serial.print(i_measured);
    Serial.print(" ");
    Serial.println(i_limit);

    cc.start();
    /*
        double u_wanted = (i_limit * r_measured) / HIGH_LOW;
        pot.WiperSetPosition( u_wanted / REF_1_256 );
        Serial.print("CURRENT LIMIT ");
        Serial.println(u_wanted / REF_1_256);
    */
  } else if (abs(i_measured - i_limit) >= 0.14) {

    /*Serial.print("CCd ");
      Serial.println(abs(i_measured - i_limit), 3);
    */
    if (abs(u_measured - u_limit) >= 0.05) {


      //Serial.print("CVd ");
      //Serial.println(abs(u_measured - u_limit), 3);
      cv.start();
      if (u_measured < u_limit) {
        pot.WiperIncrement();
        Serial.println("CV INC");
      } else {
        pot.WiperDecrement();
        Serial.println("CV DEC");
      }
      /*
           Serial.print("VOLTAGE LIMIT ");
           Serial.println(round((max(u_limit - OFFSET, 0) / HIGH_LOW) / REF_1_256));
           Serial.println((u_limit - OFFSET) / HIGH_LOW, 6);
           Serial.println((u_limit - OFFSET));
           Serial.println(HIGH_LOW, 6);
           Serial.println(REF_1_256, 6);

           pot.WiperSetPosition( round( (max(u_limit - OFFSET, 0) / HIGH_LOW) / REF_1_256 ) );
      */
    } else {
      if (cv.started()) {
        Serial.print("CV END ");
        Serial.println(cv.elapsed());
        cv.stop();
      }
    }
  } else {
    if (cc.started()) {
      Serial.print("CC END ");
      Serial.println(cc.elapsed());
      Serial.print("CCd ");
      Serial.println(abs(i_measured - i_limit), 3);
      cc.stop();
    }
    cv.stop();
  }

  button_debounce.tick();


  if (save.tick()) {
    save_eeprom();
  }


  if (!cc.started() && !cv.started() && report.tick()) {
    /*if (counter < 256) {
      Serial.print(counter);
      Serial.print(",");
      Serial.println(u_measured);
      counter++;
      pot.WiperSetPosition( counter );
      }*/
    /*Serial.print( current.average() );
      Serial.print(", ");
      Serial.println( get_voltage() );*/
    render();
  }
}

double get_voltage() {
  //#define CONV_RATIO    (REF_VOLTAGE / 1024) * (HIGH_VOLTAGE / REF_VOLTAGE)
  //Serial.println(analogRead(VOLTMETER));
  return analogRead(VOLTMETER) * CONV_RATIO;
}

void render() {
  double v  = get_voltage();
  double vl = voltage_limit.get();

  lcd.noBlink();
  lcd.setCursor ( 0, 0 );

  //lcd.print("CV");

  // round in the same way as print
  if ((v + 0.05) * 10 < 100) {
    lcd.print(" ");
  }
  lcd.print(v, 1);
  lcd.print(" /");

  if (line_selected.get() == 0) {
    lcd.write((uint8_t)1);
  } else {
    lcd.print(" ");
  }

  if (vl < 100) {
    lcd.print(" ");
  }
  lcd.print(vl / 10.0, 1);
  lcd.print("V OUT");

  lcd.setCursor ( 0, 1 );

  //lcd.print("CC ");
  lcd.print(" ");
  lcd.print(abs(current.average()), 1);
  lcd.print(" /");
  if (line_selected.get() == 1) {
    lcd.write((uint8_t)1);
  } else {
    lcd.print(" ");
  }
  lcd.print(" ");
  lcd.print(current_limit.get() / 10.0, 1);
  lcd.print("A");

  if (line_selected.get() == 2) {
    lcd.write((uint8_t)1);
  } else {
    lcd.print(" ");
  }

  if (digitalRead(BUTTON) == HIGH) {
    lcd.print(" ON");
  } else {
    lcd.print("OFF");
  }

  if (line_active.get() && line_selected.get() != 2) {
    lcd.setCursor ( 5, line_selected.get() );
    lcd.blink();
  }

}

void load_eeprom() {
  unsigned int address = 0;

  // voltage_limit
  int vl;
  EEPROM.get( address, vl );
  voltage_limit.change( vl );
  address += sizeof( vl );

  // current_limit
  int cl;
  EEPROM.get( address, cl );
  current_limit.change( cl );
  address += sizeof( cl );
}

void save_eeprom() {
  unsigned int address = 0;

  // voltage_limit
  EEPROM.put( address, voltage_limit.get() );
  address += sizeof(voltage_limit.get());

  // current_limit
  EEPROM.put( address, current_limit.get() );
  address += sizeof(current_limit.get());
}
