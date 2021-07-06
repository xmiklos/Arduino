class Controller {

  Blink blinker;
  Motor motor;
  Button automatic;
  Button save;
  Pot position_pot;
  Pot temp_pot;
  Temp temp;
  Disp disp;
  bool auto_start = false;
  //Remote remote;

  int position = 0;
								  //44,	46,	48,	50,	52,	54,	56,	58,	60
  byte temp_pos_map[TEMP_COUNT] = {  0,	20,	10,	 1,  0,  0,  0,  0,  0 }; // index => position percentage

  /* private helper methods */
  /*int percentage_to_position(int percentage) {
    return map(constrain(percentage, 0, 100), 0, 100, 0, LENGTH);
  }*/

  int temp_to_mapindex(int temp) {
    return map(constrain(temp, TEMP_MIN, TEMP_MAX), TEMP_MIN, TEMP_MAX, 0, TEMP_COUNT - 1);
  }

  int temp_to_lb(int temp) {
    int index = temp_to_mapindex(temp);
    return index_to_lb(index);
  }

  int temp_to_ub(int temp) {
    int index = temp_to_mapindex(temp);
    return index_to_ub(index);
  }

  int index_to_lb(int index) {
    return TEMP_MIN + (TEMP_RESOLUTION * index);
  }

  int index_to_ub(int index) {
    return TEMP_MIN + (TEMP_RESOLUTION * (index + 1)) - 1;
  }

  int temp_to_position(int temp) {
    return temp_pos_map[temp_to_mapindex(temp)] ;
  }

  void update_temp_pos_map() {
    int position = position_pot.read(); // %
    int temp = temp_pot.read(); // °C
    int index = temp_to_mapindex(temp);

    temp_pos_map[ index ] = position;
    if (EEPROM_ENABLE) {
      EEPROM.update(index, position);
    }

    Serial.println("Update");
  }

  void load_temp_pos_map() {
    for (int i = 0; i < TEMP_COUNT; ++i) {
      temp_pos_map [ i ] = EEPROM.read(i);
    }
    print_map();
  }

  void print_map() {
    for (int i = 0; i < TEMP_COUNT; ++i) {
      Serial.print(index_to_lb(i));
      Serial.print(" - ");
      Serial.print(index_to_ub(i));
      Serial.print(" => ");
      Serial.print(temp_pos_map [ i ]);
      Serial.println("%");
    }
  }
  
public:

  Controller() 
    :blinker(LED_PIN),
    motor(MOTOR_PIN_A, MOTOR_PIN_B, &blinker),
    automatic(AUTO_PIN),
    save(SAVE_PIN),
    position_pot(POS_POT_PIN, 0, 100),
    temp_pot(TEMP_POT_PIN, TEMP_MIN, TEMP_MAX),
    temp(TEMP_PIN),
    disp(CLK, DIO)
    //remote(REMOTE_PIN)
    {}

  void start() {
    Serial.println("Boiler controller start!");
    if (RESET) {
      motor.reset();
    }

    if (EEPROM_ENABLE) {
      load_temp_pos_map();
    }
  }

  bool stop_check() {

    // stop blinker
    blinker.stop_check();
    bool auto_change = automatic.change();
    if (auto_change) {
      if (automatic.high()) {
        Serial.println("AUTO");
        if (!auto_start) {
            Serial.println("AUTOMATIC MODE");
        }
        auto_start = true;
      } else {
        Serial.println("MANUAL");
      }
    }

    // temperature pot
    if (temp_pot.change(TEMP_POT_TRESHOLD)) {
      int t = temp_pot.read();
      disp.from_to(temp_to_lb(t), temp_to_ub(t));
    }

    // position pot
    bool position_pot_change = position_pot.change(POSITION_POT_TRESHOLD);
    if (position_pot_change) {
        disp.from_to(position_pot.read());
    }

    // send to remote
    bool temp_stop_check = temp.stop_check();
    int t;
    if (temp_stop_check) {
        t = temp.get_temp(); // sensor read
		if (global_temp != t) {
			Serial.print("Temp: ");
			Serial.println(t);
		}
        global_temp = t;
        //Serial.println(t);
        disp.show_celsius(t);

        if (millis() > 10000 && t >= AUTO_START_TEMP) {
          if (!auto_start) {
            Serial.println("AUTOMATIC MODE");
          }
          auto_start = true;
        }
    }

    // auto/manual
    if (automatic.high()) {

      if (save.change() && save.low()) {
        update_temp_pos_map();
        blinker.start(300);
      }

      if (auto_start && temp_stop_check) {
        position = temp_to_position(t);
      } else if (!auto_start && position_pot_change) {
        Serial.println("ppr");
        position = position_pot.read();
      }

    } else {
      
      if (auto_change || position_pot_change) {
        position = position_pot.read();
        disp.from_to(position);
      }
      
    }
    
    // update motor position if needed
    motor.go_to(position);

    return true;
  }
}; // Controller end
