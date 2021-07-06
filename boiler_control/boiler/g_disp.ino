class Disp {

  TM1637Display displej;

public:

  Disp(int clk, int dio, int brightness = 10) :displej(clk, dio) {
    displej.setBrightness(brightness);
  };

  void from_to(int a, int b) {
    if (a > 99) {
      a = 99;
    }
    if (b > 99) {
      b = 99;
    }
    displej.showNumberDec(a*100+b, false, 4, 0);
  }

  void from_to(int a) {
    displej.showNumberDec(a, false, 4, 0);
  }

  const uint8_t deg_celsius[2] = {
    SEG_A | SEG_B | SEG_F | SEG_G,
    SEG_A | SEG_F | SEG_E | SEG_D
  };

  void show_celsius(int a) {
    displej.showNumberDec(a, false, 2, 0);
    displej.setSegments(deg_celsius, 2, 2);
  }

}; // Motor end
