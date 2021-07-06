class Pot {

  int pin;
  int last;
  int from;
  int to;

public:

  Pot(int pin, int from, int to) {
    this->pin = pin;
    this->from = from;
    this->to = to;
    read();
  };

  int read() {
    last = analogRead(pin);
    return map(last, 0, 1023, from, to);
  }

  bool change(int treshold = 0) {
    int l = last;
    int r = analogRead(pin);
    //if (abs(l - r) > treshold)
   // printf("last: %d, read: %d, diff: %d, ool: %d\n", l, r, l - r, abs(l - r) > treshold);
    return abs(l - r) > treshold;
  }
  
}; // Button end
