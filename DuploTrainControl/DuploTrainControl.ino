#include "Lpf2Hub.h"
#include "Timer.h"
#include <Value.h>
#include <movingAvg.h>
#define P_BACKWARD  36
#define P_STOP      39
#define P_FORWARD   34
#define P_BLUE      35
#define P_GREEN     32
#define P_LED       33
#define P_RED       25
#define P_HORN      26

#define BUTTON_PIN_BITMASK 0x9C00000000

#define AVG_COUNT 20

byte motor = (byte)DuploTrainHubPort::MOTOR;

Timer pressed(1000, false, false);

class MyHub
  : public Lpf2Hub
{
  public:
    void activateBaseSpeaker()
    {
      byte setSoundMode[8] = { 0x41, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01 };
      WriteValue(setSoundMode, 8);
    }

    void playSound(byte sound)
    {
      byte playSound[6] = { 0x81, 0x01, 0x11, 0x51, 0x01, sound };
      WriteValue(playSound, 6);
    }

    void activateRgbLight()
    {
      byte port = getPortForDeviceType((byte)DeviceType::HUB_LED);
      byte setColorMode[8] = { 0x41, port, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 };
      WriteValue(setColorMode, 8);
    }

    void setLedColor(Color color)
    {
      byte port = getPortForDeviceType((byte)DeviceType::HUB_LED);
      byte setColor[6] = { 0x81, port, 0x11, 0x51, 0x00, color };
      Lpf2Hub::WriteValue(setColor, 6);
    }

    void setLedRGBColor(char red, char green, char blue)
    {
      byte port = getPortForDeviceType((byte)DeviceType::HUB_LED);
      byte setRGBColor[8] = {0x81, port, 0x11, 0x51, 0x01, red, green, blue};
      WriteValue(setRGBColor, 8);
    }
};

void colorSensorCallback(void *hub, byte portNumber, DeviceType deviceType, uint8_t *pData)
{
  Lpf2Hub *myHub = (Lpf2Hub *)hub;

  if (deviceType == DeviceType::DUPLO_TRAIN_BASE_COLOR_SENSOR)
  {
    int color = myHub->parseColor(pData);
    Serial.print("Color: ");
    Serial.println(COLOR_STRING[color]);
    myHub->setLedColor((Color)color);

    if (color == (byte)RED)
    {
      myHub->playSound((byte)DuploTrainBaseSound::BRAKE);
    }
    else if (color == (byte)BLUE)
    {
      myHub->playSound((byte)DuploTrainBaseSound::WATER_REFILL);
    }
    else if (color == (byte)YELLOW)
    {
      myHub->playSound((byte)DuploTrainBaseSound::HORN);
    }
  }
}

void speedometerSensorCallback(void *hub, byte portNumber, DeviceType deviceType, uint8_t *pData)
{
  Lpf2Hub *myHub = (Lpf2Hub *)hub;

  if (deviceType == DeviceType::DUPLO_TRAIN_BASE_SPEEDOMETER)
  {
    if (pressed.started()) {
      return;
    }
    int speed = myHub->parseSpeedometer(pData);
    Serial.print("Speed: ");
    Serial.println(speed);
    if (speed > 10)
    {
      Serial.println("Forward");
      myHub->setBasicMotorSpeed(motor, 50);
    }
    else if (speed < -10)
    {
      Serial.println("Back");
      myHub->setBasicMotorSpeed(motor, -50);
    }
    else
    {
      Serial.println("Stop");
      myHub->stopBasicMotor(motor);
    }
  }
}

Timer debounce(200, false, false);
bool forward  = false;
bool backward = false;
bool stop     = false;
bool horn     = false;

void IRAM_ATTR isr_forward() {
  if (!debounce.started()) {
    debounce.start();
    forward = true;
    Serial.println("forward = true");
  }
}

void IRAM_ATTR isr_backward() {
  if (!debounce.started() && digitalRead(P_BACKWARD) == HIGH) {
    debounce.start();
    backward = true;
    Serial.println("backward = true");
  }
}

void IRAM_ATTR isr_stop() {
  if (!debounce.started() && digitalRead(P_STOP) == HIGH) {
    debounce.start();
    stop = true;
    Serial.println("stop = true");
  }
}

void IRAM_ATTR isr_horn() {
  if (!debounce.started()) {
    debounce.start();
    horn = true;
    Serial.println("horn = true");
  }
}

Lpf2Hub train;

Timer connecting(30000, false, false);
Timer blinking(500, false, true);
Timer bt_debounce(200, false, false);

movingAvg red_avg(AVG_COUNT);
movingAvg green_avg(AVG_COUNT);
movingAvg blue_avg(AVG_COUNT);

void setup() {
  Serial.begin(115200);
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
  Serial.println("Duplo Start!");
  pinMode(P_LED, OUTPUT);
  digitalWrite(P_LED, HIGH);
  pinMode(P_FORWARD, INPUT);
  pinMode(P_BACKWARD, INPUT);
  pinMode(P_STOP, INPUT);
  pinMode(P_HORN, INPUT);
  attachInterrupt(P_FORWARD, isr_forward, RISING);
  attachInterrupt(P_BACKWARD, isr_backward, RISING);
  attachInterrupt(P_STOP, isr_stop, RISING);
  attachInterrupt(P_HORN, isr_horn, RISING);
  red_avg.begin();
  blue_avg.begin();
  green_avg.begin();
}

Value<int> red(0, 0, 255);
Value<int> green(0, 0, 255);
Value<int> blue(0, 0, 255);


void loop() {


  if (!train.isConnected() && !train.isConnecting()) {
    Serial.println("train.init();");
    train.init();
  }

  if (train.isConnecting()) {
    connecting.start();
    train.connectHub();
    blinking.start();

    if (train.isConnected()) {
      delay(200);
      train.activatePortDevice((byte)DuploTrainHubPort::SPEEDOMETER, speedometerSensorCallback);
      delay(200);
      // connect speed sensor and activate it for updates
      //train.activatePortDevice((byte)DuploTrainHubPort::COLOR, colorSensorCallback);
      //delay(200);
      /*train.activateRgbLight();
      delay(200);
      train.activateBaseSpeaker();
      delay(200);*/
      train.setLedColor(GREEN);
      connecting.stop();
      blinking.stop();
      digitalWrite(P_LED, HIGH);
      Serial.println("Connected to HUB");

    }

  }

  // buttons

  if (train.isConnected() && !bt_debounce.started()) {
    if (forward) {
      bt_debounce.start();
      Serial.println("setBasicMotorSpeed(motor, 50)");
      pressed.restart();
      train.setBasicMotorSpeed(motor, 50);
    } else if (backward) {
      bt_debounce.start();
      Serial.println("setBasicMotorSpeed(motor, -50)");
      pressed.restart();
      train.setBasicMotorSpeed(motor, -50);
    } else if (stop) {
      bt_debounce.start();
      Serial.println("train.stopBasicMotor(motor);");
      pressed.restart();
      train.stopBasicMotor(motor);
    } else if (horn) {
      bt_debounce.start();
      Serial.println("train.playSound");
      train.playSound((byte)DuploTrainBaseSound::HORN);
    }
    forward = backward = stop = horn = false;
  }

  // RGB

  if (train.isConnected() && !bt_debounce.started()) {
    red.set_th(red_avg.reading(map(analogRead(P_RED), 0, 4095, 0, 255)), 2);
    green.set_th(green_avg.reading(map(analogRead(P_GREEN), 0, 4095, 0, 255)), 2);
    blue.set_th(blue_avg.reading(map(analogRead(P_BLUE), 0, 4095, 0, 255)), 2);

    bool rc = red.change();
    bool gc = green.change();
    bool bc = blue.change();
    if (rc || gc || bc) {
      bt_debounce.start();
      Serial.println("rgb change");
      Serial.println(red.get());
      Serial.println(green.get());
      Serial.println(blue.get());
      train.setLedRGBColor(red.get(), green.get(), blue.get());
    }
  }

  if (connecting.tick()) {
    blinking.stop();
    digitalWrite(P_LED, LOW);
    Serial.println("deep sleep");
    esp_deep_sleep_start();
  }

  if (blinking.tick()) {
    digitalWrite(P_LED, digitalRead(P_LED) == HIGH ? LOW : HIGH);
  }

  debounce.tick();
  bt_debounce.tick();
  pressed.tick();
}
