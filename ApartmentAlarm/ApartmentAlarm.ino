#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <RH_ASK.h>
#include <Timer.h>
#include <ESP8266WebServer.h>

#define AP_NAME       "ApartmentAlarm"
#define RADIO_PIN             4
#define ALARM_PIN             14
#define MAGNETIC_SWITCH_PIN   5

#define OBJ_OPEN  "{"
#define OBJ_CLOSE "}"
#define OBJ_COMMA ","
#define COLON ":"
#define QUOTE     "\""

const uint8_t pushover_fingerprint[20] = {
  0x8c, 0x6d, 0x45, 0x12, 0xec, 0x9a, 0x1c, 0x50, 0x8c, 0x23, 0x58, 0xa8, 0x0f, 0xaf, 0x97, 0xa1, 0xc0, 0x36, 0x8b, 0xee
};

typedef enum command_t
{
  KEEP_ALIVE,
  ALARM,
} command_t;

typedef struct packet_t {
  unsigned long id;
  command_t command;
} packet_t;

RH_ASK driver(2000, RADIO_PIN, 13);
ESP8266WebServer server(80);

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Apartment alarm start!");

  pinMode(MAGNETIC_SWITCH_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ALARM_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(ALARM_PIN, HIGH);
  delay(100);
  digitalWrite(ALARM_PIN, LOW);

  WiFiManager wifiManager;
  wifiManager.setTimeout(360);
  wifiManager.autoConnect(AP_NAME);

  driver.init();
  server.on("/switch", handle_switch);
  server.on("/state",  handle_state);
  server.begin();

  attachInterrupt(MAGNETIC_SWITCH_PIN, door_open, RISING);
}

Timer door_isr_debounce(200, false, false);
volatile bool door_opened = false;
ICACHE_RAM_ATTR void door_open()
{
  if (!door_isr_debounce.started()) {
    door_isr_debounce.start();
    door_opened = true;
    Serial.println("ISR");
  }
}

Timer alive(20 * 60 * 1000, false, false);
Timer alarm(200, false, true);
packet_t packet;
bool alarm_enabled = false; // apartment alarm
void loop() {
  door_isr_debounce.tick();

  if (!WiFi.isConnected()) {
    Serial.println("reconnecting!");
    WiFi.begin(); // try to reconnect
  } else {
    server.handleClient();
  }

  // basement alarm
  uint8_t buflen = sizeof(packet);
  if (driver.recv((uint8_t*)&packet, &buflen)) {
    Serial.println("recv!");
    if (packet.id == 0xABCD1234) {

      if (packet.command == KEEP_ALIVE) {
        digitalWrite(LED_BUILTIN, LOW);
        alive.restart();
        Serial.println("KEEP_ALIVE!");
      } else if (packet.command == ALARM) {
        digitalWrite(ALARM_PIN, HIGH);
        if (WiFi.isConnected()) {
          pushover_alarm("Basement alarm!");
        }
        Serial.println("ALARM!");
        alarm.start();
      }
    }
  }

  if (door_opened) {
    door_opened = false;
    if (alarm_enabled) {
      delay(500);
      if (digitalRead(MAGNETIC_SWITCH_PIN) == HIGH) {
        Serial.println("Apartment alarm!");
        if (WiFi.isConnected()) {
          pushover_alarm("Apartment alarm!");
          turn_on_light();
          turn_on_camera();
        }
        digitalWrite(ALARM_PIN, HIGH);
        alarm.start();
      }
    }
  }

  if (alive.tick()) {
    digitalWrite(LED_BUILTIN, HIGH);
  }

  if (alarm.tick()) {
    digitalWrite(ALARM_PIN, digitalRead(ALARM_PIN) == HIGH ? LOW : HIGH);
  }
}

void handle_switch() {

  alarm_enabled = !alarm_enabled;

  if (!alarm_enabled && alarm.started()) {
    alarm.stop();
    digitalWrite(ALARM_PIN, LOW);
  }

  String json = OBJ_OPEN;

  json += QUOTE;
  json += "alarm_enabled";
  json += QUOTE;
  json += COLON;
  json += alarm_enabled ? "true" : "false";
  json += OBJ_COMMA;

  json += QUOTE;
  json += "alarm_started";
  json += QUOTE;
  json += COLON;
  json += alarm.started() ? "true" : "false";
  //json += OBJ_COMMA;

  json += OBJ_CLOSE;

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/json", json);

}

void handle_state() {
  String json = OBJ_OPEN;

  json += QUOTE;
  json += "door_opened";
  json += QUOTE;
  json += COLON;
  json += digitalRead(MAGNETIC_SWITCH_PIN) == HIGH ? "true" : "false";
  json += OBJ_COMMA;

  json += QUOTE;
  json += "alarm_enabled";
  json += QUOTE;
  json += COLON;
  json += alarm_enabled ? "true" : "false";
  json += OBJ_COMMA;

  json += QUOTE;
  json += "alarm_started";
  json += QUOTE;
  json += COLON;
  json += alarm.started() ? "true" : "false";
  //json += OBJ_COMMA;

  json += OBJ_CLOSE;

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/json", json);
}

void add_param(char * result, const char * key, const char * value) {

  if (strlen(result)) {
    strcat(result, "&");
  }
  strcat(result, key);
  strcat(result, "=");
  strcat(result, value);
}

void pushover_alarm(const char * msg) {
  char message[200] = "";
  add_param(message, "token", "aooai2imwwsxjd64ng795corp3kate");
  add_param(message, "user", "umb6ykb1b4i7mnn1xaccvq87isdpug");
  //add_param(message, "url", "");
  //add_param(message, "url_title", "Basement alarm!");
  add_param(message, "priority", "2");
  add_param(message, "retry", "30");
  add_param(message, "expire", "10800");
  add_param(message, "message", msg);

  send_pushover( message );
}

void send_pushover(char * body) {

  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setFingerprint(pushover_fingerprint);

  HTTPClient https;
  char url[] = "https://api.pushover.net/1/messages.json";

  Serial.println("[HTTPS] send_pushover begin...");
  Serial.println(url);

  if (https.begin(*client, url)) {  // https
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpsCode = https.POST(body);

    if (httpsCode > 0) {
      // https header has been send and Server response header has been handled
      Serial.printf("[HTTPS] POST... code: %d\n", httpsCode);

      if (httpsCode == HTTP_CODE_OK || httpsCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = https.getString();
        Serial.println(payload);
      }
    } else {
      Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpsCode).c_str());
    }

    https.end();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
}

void turn_on_light() {

  WiFiClient client;
  HTTPClient http;
  if (http.begin(client, "http://192.168.0.180/switch")) {
    int httpCode = http.GET();
    http.end();
  }
}

void turn_on_camera() {
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setInsecure();

  HTTPClient https;
  if (https.begin(*client, "https://192.168.0.166/motion.php?o=1")) {
    https.GET();
    https.end();
  }
}
