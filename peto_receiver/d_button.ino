class Button {

  int pin;
  int last;

  int _read() {
    last = digitalRead(pin);
    return last;
  }

public:

  Button(int pin) {
    this->pin = pin;
    pinMode(pin, INPUT_PULLUP);
    _read();
    
    Serial.println(last);
  };

  bool change() {
    int l = last;
    int r = _read();
    return l != r;
  }

  bool high() {
    return last == HIGH;
  }

  bool low() {
    return last == LOW;
  }
  
}; // Button end
