#include <OneWire.h>
#define REQUIRESALARMS false
#include <DallasTemperature.h>
#include <EEPROM.h>

/* pins */

#define TEMP_PIN 7  
#define REMOTE_PIN 12

#define GET_TEMP_SPEED 1000

int global_temp = 0;

