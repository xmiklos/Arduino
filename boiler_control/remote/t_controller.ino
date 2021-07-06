class Controller {

  Blink blinker;
  Pot temp_pot;
  Disp disp;
  byte temp;
  Remote remote;
  Timer pot_change;
  int temp_pot_val = 0;
  
public:

  Controller() 
    :blinker(LED_PIN),
    temp_pot(TEMP_POT_PIN, TEMP_MIN, TEMP_MAX),
    disp(CLK, DIO),
    remote(REMOTE_PIN)
    {}

  void start() {
    Serial.println("Boiler remote start!");
    blinker.start();
    blinker.stop();
    disp.show_none();
  }

  bool stop_check() {
    
    blinker.stop_check();
    disp.stop_check();
    pot_change.stop_check();

    // receive from master controller
   // Serial.println(remote.getByte());
    if (!pot_change.is_running() && remote.ready()) {
      disp.show_celsius(remote.get_temp());
      Serial.println(remote.get_temp());
      if (remote.get_temp() <= temp_pot_val) {
          Serial.println("alarm!!!");
          blinker.start(3000);
      }
    }

    //temperature pot
    if (temp_pot.change(TEMP_POT_TRESHOLD)) {
      temp_pot_val = temp_pot.read();
      disp.show_celsius(temp_pot_val);
      pot_change.stop();
      pot_change.start(3000);
    }

    return true;
  }
}; // Controller end
