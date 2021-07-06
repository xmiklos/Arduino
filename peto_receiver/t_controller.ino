class Controller {

  Blink blinker;
  Disp disp;
  Encoder enc;
  Timer enc_ch;
  Timer show;
  uint8_t temp;

  long old_read   = 0;
  long old_pos    = 0;
  long alarm_val  = 0;
  
public:

  Controller() 
    :blinker(LED_PIN),
    disp(CLK, DIO),
    enc(ENC_A, ENC_B)
    {}

  void start() {
    Serial.println("Boiler remote start!");

    disp.show_none();
    show.start(3000);
  }

  bool stop_check() {
    
    blinker.stop_check();
    disp.stop_check();
    enc_ch.stop_check();

    byte s = get_state();
    if (s) {
      enc_ch.stop();
    }

   if ((s || (show.stop_check() && !enc_ch.is_running())) && got2) {
      got2 = false;
      disp.show_celsius(global_temp);
      Serial.println("start");
   }
   
   if (global_temp <= alarm_val && got) {
      Serial.println("alarm!!!");
      blinker.start(3000);
   }

   if (!show.is_running()) {
      show.start(3000);
   }

    //temperature pot
    long new_read = enc.read() / 4;
    long diff = old_read - new_read;
    old_read = new_read;

    long new_pos = constrain(old_pos + diff, 0, 100);
    if (new_pos != old_pos) {
      old_pos = new_pos;
      Serial.println(new_pos);
      alarm_val = new_pos;
      disp.show_celsius(alarm_val);
      enc_ch.stop();
      enc_ch.start(3000);
    }

    return true;
  }
}; // Controller end
