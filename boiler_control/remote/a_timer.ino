typedef enum run_st {
  BEFORE, RUN
} run_st;

class Timer {
  unsigned long start_time;
  unsigned long wait_time;
  run_st state;

  public:

  Timer() {
    state = BEFORE;
  }
  
  bool start(unsigned long wait_time) {
    if (state == BEFORE) {
      this->wait_time = wait_time;
      state = RUN;
      start_time = millis();
      return true;
    }
  
    return false;
  }

  void stop() {
    state = BEFORE;
  }
  
  bool stop_check() {
    if (state == RUN) {
      unsigned long current_time = millis();
      if (current_time - start_time >= wait_time) {
        state = BEFORE; // stop
        return true;
      }
    }
  
    return false;
  }

  bool is_running() {
    return state == RUN;
  }
};

