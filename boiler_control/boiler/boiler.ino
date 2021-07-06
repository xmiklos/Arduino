#include <OneWire.h>
#define REQUIRESALARMS false
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <TM1637Display.h>
#include <Adafruit_HMC5883_U.h>

/* pins */

#define MOTOR_PIN_A 8
#define MOTOR_PIN_B 9
#define LED_PIN 12
#define TEMP_PIN 11
#define AUTO_PIN 3
#define SAVE_PIN 4
#define TEMP_POT_PIN A0
#define POS_POT_PIN A1
#define CLK 6
#define DIO 7
#define REMOTE_PIN 5

/* linear actuator parameters */
#define LENGTH 100  // milimeters
#define BLINK_SPEED  100
#define RESET_TIME 40000 //ms

/* desired temperature/movement accuracy */
#define TEMP_RESOLUTION 2
#define TEMP_MIN 44
#define TEMP_MAX 60
#define GET_TEMP_SPEED 5000
#define AUTO_START_TEMP 50
#define RESET 0

/* magnet */
#define MAGNET_MIN 138
#define MAGNET_MAX 247

/* internal */
#define TEMP_COUNT (((TEMP_MAX-TEMP_MIN) / TEMP_RESOLUTION) + 1)
#define s2ms(a) ((a)*1000) // seconds to miliseconds

#define POSITION_POT_TRESHOLD 30
#define TEMP_POT_TRESHOLD 3

#define EEPROM_ENABLE 0

Adafruit_HMC5883_Unified magnet = Adafruit_HMC5883_Unified(12345);

int magnet_y() {
  sensors_event_t mag_data;
  magnet.getEvent(&mag_data);
  //Serial.println(mag_data.magnetic.y);
  int val = mag_data.magnetic.y + 200; // eliminate negative values
  return val;
}

int global_temp = 0;

