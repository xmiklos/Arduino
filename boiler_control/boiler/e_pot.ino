#define NUM_READINGS 100

class Pot {

  int pin;
  int from;
  int to;
  
  int readings[NUM_READINGS] = { 0 };
  int read_index = 0;
  long total = 0;
  int average = 0;
  int last = 0;

public:

  Pot(int pin, int from, int to) {
    this->pin = pin;
    this->from = from;
    this->to = to;
  };

  int read() {
    return map(analogRead(pin), 0, 1023, from, to);
  }

  bool change(int treshold = 0) {
    total -= readings[read_index];
    readings[read_index] = read();
    total += readings[read_index];
    read_index++;
    
    average = total / NUM_READINGS;
    
    if (read_index >= NUM_READINGS) {
      read_index = 0;
      
      int l = last;
      last = average;
      //printf("last: %d, avg: %d\n", last, average);
      return abs(last - l) >= 1;
    }

    return false;
  }
  
}; // Button end
