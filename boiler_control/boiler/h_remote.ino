#include <VirtualWire.h>

class Remote {

  uint8_t buf;

public:

  Remote(int pin) {
    // nastavení typu bezdrátové komunikace
    vw_set_ptt_inverted(true);
    // nastavení rychlosti přenosu v bitech za sekundu
    vw_setup(2000);
    // nastavení čísla datového pinu pro vysílač
    vw_set_tx_pin(pin);
  };

  void sendByte(byte b) {
    buf = b;
    vw_send(&buf, sizeof(buf));
    Serial.print("sent\n");
  }

}; // Remote end
