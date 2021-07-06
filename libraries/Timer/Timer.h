class Timer {
  volatile unsigned long start_time;
  volatile unsigned long wait_time;
  volatile bool _started;
  volatile bool repeat;

  public:

  Timer(unsigned long wait_time, bool start_now = true, bool repeat = true): _started(false), repeat(repeat), wait_time(wait_time) {
    if (start_now) {
      start();
    }
  }
  
  bool start() {
    if (!_started) {
      start_time = millis();
      _started = true;
      return true;
    }

    return false;
  }

  bool start(unsigned long wait_time) {
    this->wait_time = wait_time;
    return start();
  }

  void restart() {
    stop();
    start();
  }

  void restart(unsigned long wait_time) {
    stop();
    start(wait_time);
  }

  void stop() {
    _started = false;
  }
  
  bool tick() {
    if (_started) {
      unsigned long current_time = millis();
      if (current_time - start_time >= wait_time) {
        if (repeat) {
          restart();
        } else {
          stop();
        }
        return true; // signal wait time is over
      }
    }
    return false;
  }
  
  bool started() {
	return _started;
  }
  
  unsigned long elapsed() {
	  if (!_started) {
		  return 0;
	  }
	  return millis() - start_time;
  }
};