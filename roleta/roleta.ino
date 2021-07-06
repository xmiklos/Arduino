#include <IRremote.h>

int RECV_PIN = PD6;
#define MA PD4
#define MB PD5
#define UPPER_STOP 3
#define LOWER_STOP 2

IRrecv irrecv(RECV_PIN);

decode_results results;
long running = 0;
volatile byte up;
volatile byte down;
bool upc = false;
bool downc = false;

void go_up() {
    digitalWrite(MA, HIGH);
    digitalWrite(MB , LOW);
    running = millis();
}

void go_down() {
    digitalWrite(MA , LOW);
    digitalWrite(MB, HIGH);
    running = millis();
}

void dont_go() {
    digitalWrite(MA, LOW);
    digitalWrite(MB , LOW);
    running = 0;
}

bool run_for(long t) {
    if (running && millis() - running >= t) {
      return true;
    }

    return false;
}

void up_stop() {
  up = digitalRead(UPPER_STOP);
  upc = true;
}

void down_stop() {
  down = digitalRead(LOWER_STOP);
  downc = true;
}

void setup()
{
  Serial.begin(9600);
  irrecv.enableIRIn(); // Start the receiver
  pinMode(MA, OUTPUT);
  pinMode(MB , OUTPUT);
  Serial.println("test");
  digitalWrite(MA, LOW);
  digitalWrite(MB , LOW);
  down = digitalRead(LOWER_STOP);
  up = digitalRead(UPPER_STOP);

  attachInterrupt(digitalPinToInterrupt(UPPER_STOP), up_stop, CHANGE);
  attachInterrupt(digitalPinToInterrupt(LOWER_STOP), down_stop, CHANGE);
}

void loop() {

  if (upc) {
     Serial.print("upc: ");
      Serial.println(digitalRead(UPPER_STOP));
      upc = false;
  }
if (downc) {
     Serial.print("downc: ");
      Serial.println(digitalRead(LOWER_STOP));
      downc = false;
  }
if (run_for(500) && up == HIGH) {
  Serial.println("STOP1");
  dont_go();
}

if (run_for(500) && down == LOW) {
  Serial.println("STOP2");
  dont_go();
}
  
  if (irrecv.decode(&results)) {

  if (results.value == 0xFFA25D && down == HIGH) {
    go_down();
  } else if (results.value == 0xFF629D) {
    dont_go();
  } else if (results.value == 0xFFE21D && up == LOW) {
    go_up();
  }
  
  Serial.println(results.value, HEX);
  irrecv.resume(); // Receive the next value
 
}


  delay(100);
}
