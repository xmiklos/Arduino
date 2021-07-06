class Controller {

  Temp temp;
  Remote remote;
  
public:

  Controller()
    :temp(TEMP_PIN),
    remote(REMOTE_PIN)
    {}

  void start() {
    Serial.println("Transmitter controller start!");
  }

  bool stop_check() {

    // send to remote
    bool temp_stop_check = temp.stop_check();
    if (temp_stop_check) {
        int t = temp.get_temp(); // sensor read
  		//if (global_temp != t) {
  			Serial.print("Temp: ");
  			Serial.println(t);
  		//}
        remote.sendByte(t);
    }

    return true;
  }
}; // Controller end
