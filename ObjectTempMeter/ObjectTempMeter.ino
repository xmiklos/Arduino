#include <Adafruit_MLX90614.h>
#include <TM1637Display.h>

#define CLK 2
#define DIO 3
#define SWITCH 5
#define PROBE_POWER 7

Adafruit_MLX90614   mlx = Adafruit_MLX90614();
TM1637Display       display(CLK, DIO);

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Start");
  mlx.begin();

  display.setBrightness(0x0f);
  uint8_t data[] = { 0xff, 0xff, 0xff, 0xff };
  display.setSegments(data);

  pinMode(SWITCH, INPUT_PULLUP);
  pinMode(PROBE_POWER, OUTPUT);
  digitalWrite(PROBE_POWER, HIGH);
}

void loop() {
  int t = 0;
  if (digitalRead(SWITCH) == HIGH) {
    t = round( mlx.readObjectTempC() * 10 );
  } else {
    t = round( mlx.readAmbientTempC() * 10 );
  }
  
  display.showNumberDec(t);

  delay(200);
}
