
class Blink {
  Timer whole;
  Timer between;
  
  unsigned long between_speed = 0;
  unsigned on;
  int blink_pin;

  void _led_on() {
    digitalWrite(blink_pin, HIGH);
    on = 1;
  }

  void _led_off() {
    digitalWrite(blink_pin, LOW);
    on = 0;
  }

  void _toggle() {
    if (!on) {
      _led_on();
    } else {
      _led_off();
    }
  }

  void _init(int pin, unsigned long between_speed) {
    this->blink_pin = pin;
    this->between_speed = between_speed;
    this->on    = 0;
    pinMode(pin, OUTPUT);
    _led_off();
  }

public:

  Blink(int pin) {
    _init(pin, BLINK_SPEED);
  }

  Blink(int pin, unsigned long between_speed) {
    _init(pin, between_speed);
  }

  bool start(unsigned long ms) {
    bool ret = false;
    if (!whole.is_running() && !between.is_running()) {
      between.start(between_speed);
      ret = whole.start(ms);
      _toggle();
    }
    
    return ret;
  }

  bool start() {
    bool ret = false;
    if (!whole.is_running() && !between.is_running()) {
      ret = between.start(between_speed);
      _toggle();
    }

    return ret;
  }

  void stop() {
    if (!whole.is_running() && between.is_running()) {
      between.stop();
      _led_off();
    }
  }
  
  bool stop_check() {
    unsigned ret = false;
    if (whole.stop_check()) {
      between.stop();
      _led_off();
      ret = true;
    }
    
    if (between.stop_check()) {
      _toggle();
      between.start(BLINK_SPEED);
    }

    return ret;
  }
};

