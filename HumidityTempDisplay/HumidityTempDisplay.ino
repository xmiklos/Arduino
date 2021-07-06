#include <LiquidCrystal_I2C.h>
#include "DHT.h"

#include <Timer.h>
#include <Value.h>

#define PIN_TEMP             6

#define LCD_ADDRESS         0x3F
#define UNDEF -127

LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);
DHT dht_sensor(PIN_TEMP, DHT11);

void setup() {
  Serial.begin(9600);
  while(!Serial);
  lcd.begin();
  lcd.backlight();
  lcd.print("Inicializujem");
  lcd.setCursor(0, 1);
  dht_sensor.begin();
  
}

void loop() {

  lcd.setCursor(0, 0);
  lcd.print("Teplota: ");

  align(lcd, dht_sensor.readTemperature(), 6);
  lcd.print( (char) 223 );

  lcd.setCursor(0, 1); // new line

  lcd.print("Vlhkost: ");
  
  align(lcd, dht_sensor.readHumidity(), 6);
  lcd.print( "%" );

  delay(1000);
}

void align(LiquidCrystal_I2C& lcd, int value, int width) {
  if (value == UNDEF) {
    for (int i = 0; i < width - 2; ++i) {
      lcd.print(' ');
    }
    lcd.print('-');
    if (width > 1) {
      lcd.print('-');
    }
    return;
  }
  if (value < 0) {
    width--;
  }

  if (value < 10 && value > -10) {
    width--;
  } else if (value > 99 || value < -99) {
    width -= 3;
  } else {
    width -= 2;
  }
  for (int i = 0; i < width; ++i) {
    lcd.print(' ');
  }

  lcd.print(value);
}
