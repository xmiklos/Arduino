class Motor {

  int pin_a;
  int pin_b;
  int moving;
  int target = -1;
  int magnet_min = MAGNET_MIN;
  int magnet_max = MAGNET_MAX;
  Blink * blinker;

  int num_readings = 10;
  int readings[10] = { 0 };
  int read_index = 0;
  long total = 0;
  int average = 0;

  void go_up() {
    digitalWrite(pin_a, HIGH);
    digitalWrite(pin_b, LOW);
    moving = 1;
    blinker->start();
  }
  
  void go_down() {
    digitalWrite(pin_a, LOW);
    digitalWrite(pin_b, HIGH);
    moving = -1;
    blinker->start();
  }
  
  void stop() {
    digitalWrite(pin_a, LOW);
    digitalWrite(pin_b, LOW);
    moving = 0;
    blinker->stop();
  }

public:

  Motor(int a, int b, Blink * blinker){
    pin_a = a;
    pin_b = b;
    moving = 0;
    this->blinker = blinker;

    pinMode(pin_a, OUTPUT);
    pinMode(pin_b, OUTPUT);
  }

  int position() {
    int magnet_val = magnet_y();

    total -= readings[read_index];
    readings[read_index] = magnet_val;
    total += readings[read_index];
    read_index++;
    
    average = total / num_readings;
    
    if (read_index >= num_readings) {
      read_index = 0;
    }
    
    return map(constrain(average, magnet_min, magnet_max), magnet_min, magnet_max, 0, LENGTH);
  }

  void status(int current_position, int target_position) {
     Serial.print("Going");
      if (target_position > position()) {
        Serial.print(" up");
      } else {
        Serial.print(" down");
      }
      Serial.print(" from ");
      Serial.print(current_position);
      Serial.print(" to ");
      Serial.println(target_position);
  }

  void go_to(int target_position) {
  
    target_position = constrain(target_position, 0, 100);
    int current_position = position();

    int diff = abs(target_position - current_position);
    if (moving && diff < 1) {
      stop();
    }

    if (
          (moving > 0 && current_position >= target)
          || (moving < 0 && current_position <= target)
    ) {
      stop();
    }

    if (target != target_position) {
      if (target_position > current_position) {
        if (moving != 1) {
          go_up();
        }
      } else {
        if (moving != -1) {
          go_down();
        }
      }
      
      status(current_position, target_position);
  
      target = target_position;
    }
  }
  
  void reset() {
    Serial.println("Reseting...");
    go_up();
    delay(RESET_TIME);
    magnet_max = magnet_y();
    Serial.print("Magnet max: ");
    Serial.println(magnet_max);
    go_down();
    delay(RESET_TIME); 
    magnet_min = magnet_y();
    Serial.print("Magnet min: ");
    Serial.println(magnet_min);
    stop();
  }


}; // Motor end
