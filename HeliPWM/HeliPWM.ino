#include <IRremote.h>
#include <Timer.h>
#define MAIN_A_PWM 9
#define MAIN_A_DIR 8

#define MAIN_B_PWM 10
#define MAIN_B_DIR 11

#define BACK_A 5
#define BACK_B 6


int RECV_PIN = 2;

IRrecv irrecv(RECV_PIN);

decode_results results;

typedef unsigned long uint;

void setup() {
  Serial.begin(115200);
  while(!Serial);
  Serial.println("start");
  irrecv.enableIRIn();
/*  pinMode( MAIN_A_PWM, OUTPUT );
  pinMode( MAIN_A_DIR, OUTPUT );
  pinMode( MAIN_B_PWM, OUTPUT );
  pinMode( MAIN_B_DIR, OUTPUT );
  pinMode( BACK_A, OUTPUT );
  pinMode( BACK_B, OUTPUT );

  analogWrite( MAIN_A_PWM, LOW );
  digitalWrite( MAIN_A_DIR, HIGH );
  analogWrite( MAIN_B_PWM, LOW );
  digitalWrite( MAIN_B_DIR, HIGH );
  digitalWrite( BACK_A, HIGH );
  digitalWrite( BACK_B, HIGH );*/
}

Timer running(200, false, false);
void loop() {
    running.tick();

  if (irrecv.decode(&results)) {

    irrecv.resume();
    uint bits     = results.value;
    uint off      = bits >> 31;
    uint thrust   = (bits >> 24) & 0x7F;
    uint yaw      = (bits >> 20) & 0xF;
    uint pitch    = (bits >> 16) & 0xF;

/*
    Serial.print(thrust);
    Serial.print(" ");
    Serial.print(yaw);
    Serial.print(" ");
    Serial.println(pitch);
*/
    
    for (int i = 31; i >= 0; i--) {
      Serial.print( (bits >> i) & 1, BIN );
      if (i % 8 == 0) {
         Serial.print(" ");
      }
    }
    Serial.println();

    
  }
  
    delay(10);
}
