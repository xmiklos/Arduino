
class Remote {

    uint8_t buf[VW_MAX_MESSAGE_LEN];
    uint8_t len = VW_MAX_MESSAGE_LEN;

public:

  Remote(int pin) {
    // nastavení typu bezdrátové komunikace
    vw_set_ptt_inverted(true);
    // nastavení rychlosti přenosu v bitech za sekundu
    vw_setup(1000);
    // nastavení čísla datového pinu pro vysílač
    vw_set_rx_pin(pin);
    
    vw_rx_start();
  };

  bool ready() {
    bool ret = vw_get_message(buf, &len);
    return ret;
  }

  int get_temp() {
    return *((int*)buf);
  }


}; // Remote end
