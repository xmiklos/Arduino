#include <IRremote.h>
#include <Timer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define IR_PIN A0                        
#define SDA A4
#define SCL A5

#define FRONT_LEFT_DIR    0
#define FRONT_LEFT_SPEED  1
#define FRONT_RIGHT_DIR   2
#define FRONT_RIGHT_SPEED 3
#define REAR_LEFT_DIR     4
#define REAR_LEFT_SPEED   5
#define REAR_RIGHT_DIR    6
#define REAR_RIGHT_SPEED  7

#define FRONT_LEFT_SERVO  8
#define FRONT_RIGHT_SERVO 9
#define REAR_LEFT_SERVO   10
#define REAR_RIGHT_SERVO  11

#define SERVO_MIDDLE            329
#define FRONT_LEFT_CORRECTION   2
#define FRONT_RIGHT_CORRECTION  -1
#define REAR_LEFT_CORRECTION    0
#define REAR_RIGHT_CORRECTION   -23


#define C180_DIV_PI          M_PI/180.0
#define deg2rad(DVAL)        DVAL*C180_DIV_PI
#define rad2deg(RVAL)        RVAL*180/M_PI

#define analog2deg(AVAL)     0.45*AVAL // pretoze analog 200 je 90 stupnov
#define deg2analog(DVAL)     DVAL/0.45

#define analog2rad(AVAL)     deg2rad(0.45*AVAL) // pretoze analog 200 je 90 stupnov
#define rad2analog(RVAL)     rad2deg(RVAL)/0.45
#define cotg(x)              cos(x)/sin(x)
#define VLEN                 11.33
#define VWIDTH               13.5

#define ANALOG_MIN        512
#define ANALOG_MAX        4095


typedef unsigned long uint;
class ir_results {
  public:
    char off;
    uint thrust;  // 0 - 126
    uint yaw;     // 1 - 7 - (8) - 9 - 15
    uint pitch;   // 1 - 7 - (8) - 9 - 15
    char photo;
    char light;
    char l;
    char r;
    char L1;
    char L2;
    char R2;
};

class Wheel {
  public:
    int dir;
    int speed;
};

Wheel front_left  = { FRONT_LEFT_DIR, FRONT_LEFT_SPEED };
Wheel front_right = { FRONT_RIGHT_DIR, FRONT_RIGHT_SPEED };
Wheel rear_left   = { REAR_LEFT_DIR, REAR_LEFT_SPEED };
Wheel rear_right  = { REAR_RIGHT_DIR, REAR_RIGHT_SPEED };

IRrecv ir(IR_PIN);
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("start");
  ir.enableIRIn();
  pwm.begin();
  pwm.setPWMFreq(50);
}

void loop() {

  ir_results r;

  if (ir_get(&r)) {

    int speed = map(constrain(r.thrust, 0, 126), 0, 126, ANALOG_MIN, ANALOG_MAX);

    if (r.L2) {              // turn left in place
      backward(front_left, speed);
      backward(rear_left, speed);
      forward(front_right, speed);
      forward(rear_right, speed);
    } else if (r.R2) {              // turn right in place
      forward(front_left, speed);
      forward(rear_left, speed);
      backward(front_right, speed);
      backward(rear_right, speed);
    } else if (r.off || r.pitch == 8) {                    // stop
      stop(front_left);
      stop(front_right);
      stop(rear_left);
      stop(rear_right);
    } else if (r.pitch != 8) {                        // forward/reverse with turning

      bool reverse = r.pitch < 8;
      go(reverse, front_left,  speed);
      go(reverse, front_right, speed);
      go(reverse, rear_left,   speed);
      go(reverse, rear_right,  speed);

    }
    int turn_max    = 80;
    int turn_step   = turn_max / 7;
    int turn_angle  = r.yaw == 8   ? SERVO_MIDDLE
                    :  r.yaw < 8    ? map(r.yaw, 1, 7, SERVO_MIDDLE - turn_max, SERVO_MIDDLE - turn_step)
                    :                 map(r.yaw, 9, 15, SERVO_MIDDLE + turn_step, SERVO_MIDDLE + turn_max)
                    ;

    int middle_deviation = abs(SERVO_MIDDLE - turn_angle);
    double r1           = VLEN*cotg(analog2rad(middle_deviation))+VWIDTH/2;
    double outer_angle  = rad2analog( atan(VLEN / ( r1 + VWIDTH/2)) );
    Serial.println(analog2rad(middle_deviation));
    Serial.println(outer_angle);
    pwm.setPWM(FRONT_LEFT_SERVO, 0, (r.yaw <= 8 || r.L1 ? turn_angle : SERVO_MIDDLE + outer_angle) + FRONT_LEFT_CORRECTION); // +- corection
    pwm.setPWM(FRONT_RIGHT_SERVO, 0, (r.yaw >= 8 || r.L1 ? turn_angle : SERVO_MIDDLE - outer_angle) + FRONT_RIGHT_CORRECTION);
    if (r.L1) {
      pwm.setPWM(REAR_LEFT_SERVO, 0, turn_angle + REAR_LEFT_CORRECTION);
      pwm.setPWM(REAR_RIGHT_SERVO, 0, turn_angle + REAR_RIGHT_CORRECTION);
    } else if (r.photo) {
      int oposite_angle = ((turn_angle - SERVO_MIDDLE) * -1) + SERVO_MIDDLE;
      pwm.setPWM(REAR_LEFT_SERVO, 0, (r.yaw <= 8 ? oposite_angle : SERVO_MIDDLE - outer_angle) + REAR_LEFT_CORRECTION);
      pwm.setPWM(REAR_RIGHT_SERVO, 0, (r.yaw >= 8 ? oposite_angle : SERVO_MIDDLE + outer_angle) + REAR_RIGHT_CORRECTION);
    } else {
      pwm.setPWM(REAR_LEFT_SERVO, 0, SERVO_MIDDLE + REAR_LEFT_CORRECTION);
      pwm.setPWM(REAR_RIGHT_SERVO, 0, SERVO_MIDDLE + REAR_RIGHT_CORRECTION);
    }
  }

  //delay(10);
}

void go(bool reverse, Wheel w, int speed) {
  if (reverse) {
    backward(w, speed);
  } else {
    forward(w, speed);
  }
}

void forward(Wheel w, int speed) {
  speed = constrain(speed, 0, ANALOG_MAX);

  pwm.setPWM(w.dir, 0, 4096); // off
  pwm.setPWM(w.speed, 0, speed);
}

void backward(Wheel w, int speed) {
  speed = ANALOG_MAX - constrain(speed, 0, ANALOG_MAX);

  pwm.setPWM(w.dir, 4096, 0); // on
  pwm.setPWM(w.speed, 0, speed);
}

void stop(Wheel w) {
  backward(w, 0);
}

bool ir_get(ir_results * results) {
  decode_results  bit_results;

  if (ir.decode(&bit_results)) {
    uint bits = bit_results.value;
    ir.resume();

    if (0) { // dump bits
      for (int i = 31; i >= 0; i--) {
        Serial.print( (bits >> i) & 1, BIN );
        if (i % 8 == 0) {
          Serial.print(" ");
        }
      }
      Serial.println();
    }

    results->off     = bits >> 31;
    results->thrust  = (bits >> 24)  & 0x7F;
    results->yaw     = (bits >> 20)  & 0xF;
    results->pitch   = (bits >> 16)  & 0xF;
    results->photo   = (bits >> 5)   & 1;
    results->light   = (bits >> 7)   & 1;
    results->l       = (bits >> 9)   & 1;
    results->r       = (bits >> 8)   & 1;
    results->L1      = (bits >> 12)  & 1;
    results->L2      = (bits >> 4)   & 1;
    results->R2      = (bits >> 6)   & 1;

    if (1) {
      Serial.print(results->thrust);
      Serial.print(" ");
      Serial.print(results->yaw);
      Serial.print(" ");
      Serial.print(results->pitch);
      Serial.print(" ");
      if (results->off) Serial.print("off ");
      if (results->photo) Serial.print("photo ");
      if (results->light) Serial.print("light ");
      if (results->l) Serial.print("l ");
      if (results->r) Serial.print("r ");
      if (results->L1) Serial.print("L1 ");
      if (results->L2) Serial.print("L2 ");
      if (results->R2) Serial.print("R2 ");
      Serial.println();
    }

    return true;
  }

  return false;
}
