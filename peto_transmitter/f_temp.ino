class Temp {

  int pin;
  Timer timer;
  
  OneWire one_wire;
  DallasTemperature sensors;

public:

  Temp(int pin):
    one_wire(pin),
    sensors(&one_wire) {
    sensors.begin();
    timer.start(GET_TEMP_SPEED);
  };
  
  int get_temp() {
    sensors.requestTemperatures();
    int temp = sensors.getTempCByIndex(0);
    //Serial.print("Temp: ");
    //Serial.println(temp);
    return temp;
  }

  unsigned stop_check() {
    bool ret = timer.stop_check();
    if (ret) {
      timer.start(GET_TEMP_SPEED);
    }
    return ret;
  }

}; // Motor end
