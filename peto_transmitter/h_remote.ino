#include <VirtualWire.h>

class Remote {

    uint8_t buf[70];

  public:

    Remote(int pin) {
    };

    void sendByte(byte b) {
      buf[0] = b;
      vw_send(buf, 1);
      vw_wait_tx();
      Serial.println("sent");
    }

}; // Remote end
