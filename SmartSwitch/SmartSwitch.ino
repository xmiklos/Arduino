#include <LiquidCrystal_I2C.h>
#include "DHT.h"

#include <NTPClient.h>
#include <WiFiUdp.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include <WiFiManager.h>
#include <TM1637Display.h>
#include <Value.h>
#include <Timer.h>

#define PIN_TEMP      2
#define PIN_DISP_CLK  13
#define PIN_DISP_DIO  12
#define PIN_SWITCH    4
#define PIN_RELAY     5
#define AP_NAME       "SmartSwitch1"

#define OBJ_OPEN	"{"
#define OBJ_CLOSE	"}"
#define OBJ_COMMA	","
#define COLON	":"
#define QUOTE		  "\""

DHT           dht_sensor(PIN_TEMP, DHT11);
TM1637Display disp(PIN_DISP_CLK, PIN_DISP_DIO);
ESP8266WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

Value<int> teplota( 0, 0, 99);
Value<int> vlhkost( 0, 0, 99 );
Value<int> relay( 1 );

Value<int> hour(0);

void setup() {
  Serial.begin(9600);
  while(!Serial);
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, HIGH);
  pinMode(PIN_SWITCH, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_SWITCH), switch_isr, CHANGE);

  dht_sensor.begin();

  disp.setBrightness(10);

  uint8_t data[] = { 0xff, 0xff, 0xff, 0xff };
  disp.setSegments(data);

  connect_to_wifi();

  server.on("/", handle_root);
  server.on("/switch", handle_switch);
  server.on("/state", handle_state);
  server.begin();
  timeClient.begin();
  timeClient.update();
  timeClient.setTimeOffset( get_geo_time_offset() );
  hour.change( timeClient.getHours() );
}

void read_sensors() {

  for (int i = 0; i < 3; ++i) {
    float t = dht_sensor.readTemperature();
    float h = dht_sensor.readHumidity();
    if (isnan(t) || isnan(h)) {
      Serial.println("sensors read failed!");
      Serial.println(t);
      Serial.println(h);
      delay(2000);
      continue;
    }
    teplota.set( t );
    vlhkost.set( h );
    Serial.println("sensors read ok");
    break;
  }
}

Timer switch_isr_debounce(200, false, false);
void handle_switch() {
  if (!switch_isr_debounce.started()) {
    switch_isr_debounce.start();
    relay.set( !relay.get() );

	String json = OBJ_OPEN;
	json += QUOTE;
	json += "switch";
  json += QUOTE;
  json += COLON;
	json += relay.get() ? 0 : 1;
	json += OBJ_CLOSE;

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/json", json);
  }
}

void handle_root() {
  String message = "<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head><body>";
  message += "<h1>Teplota: ";
  message += teplota.get();
  message += "&deg;C</h1>";
  message += "<h1>Vlhkost: ";
  message += vlhkost.get();
  message += "%</h1>";
  message += "</body></html>";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/html", message);
}

void handle_state() {
	String json = OBJ_OPEN;

	json += QUOTE;
	json += "teplota";
  json += QUOTE;
  json += COLON;
	json += teplota.get();
	json += OBJ_COMMA;
  
	json += QUOTE;
	json += "vlhkost";
	json += QUOTE;
  json += COLON;
	json += vlhkost.get();
	json += OBJ_COMMA;

	json += QUOTE;
	json += "switch";
  json += QUOTE;
  json += COLON;
	json += relay.get() ? 0 : 1;
	
	json += OBJ_CLOSE;

  server.sendHeader("Access-Control-Allow-Origin", "*");
	server.send(200, "text/json", json);
}

ICACHE_RAM_ATTR void switch_isr() {
  if (!switch_isr_debounce.started()) {
    switch_isr_debounce.start();
    relay.set( !relay.get() );
  }
}

Timer disp_change(2500, true, true);
void (*views[])() = {
  &render_temperature,
  &render_humidity,
  &render_time,
};
int view_count = sizeof(views) / sizeof(*views);
unsigned long current_view = 0;

const uint8_t deg_celsius[2] = {
  SEG_A | SEG_B | SEG_F | SEG_G,
  SEG_A | SEG_F | SEG_E | SEG_D
};
const uint8_t percent[2] = {
  SEG_A | SEG_B | SEG_F | SEG_G,
  SEG_G | SEG_E | SEG_C | SEG_D
};

Timer time_to_send(0, true, true);
void loop() {
  switch_isr_debounce.tick();

  int is_time_to_send = time_to_send.tick();
  time_to_send.start(30000);
  
  if (disp_change.tick()) {
    views[ ++current_view % view_count ]();
  }

  if (relay.change()) {
    Serial.print("relay change: ");
    Serial.println(relay.get());
    digitalWrite(PIN_RELAY, digitalRead(PIN_RELAY) == LOW ? HIGH : LOW);
  }

  if (is_time_to_send) {
    read_sensors();
  }

  if (WiFi.status() == WL_CONNECTED) {

    if ( is_time_to_send && teplota.change() ) {
      send(1, teplota.get());
    }

    if ( is_time_to_send && vlhkost.change() ) {
      send(2, vlhkost.get());
    }

    server.handleClient();
    timeClient.update();

    if (hour.change( timeClient.getHours() )) {
      timeClient.setTimeOffset( get_geo_time_offset() );

      // when offset changes hour, avoid update
      hour.change( timeClient.getHours() );
    }

  } else {
    connect_to_wifi();
  }
}
void send(int field, int value) {
  WiFiClient client;

  HTTPClient http;
  char format[] = "http://api.thingspeak.com/update?api_key=OKHVBY1KG4K61D7U&field%d=%d";
  char url[ sizeof(format) + 5 ];
  sprintf(url, format, field, value);

  Serial.print("[HTTP] send_temp begin...\n");
  Serial.println(url);
  if (http.begin(client, url)) {  // HTTP

    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();
        Serial.println(payload);
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }
}

int get_geo_time_offset() {
  WiFiClient client;
  HTTPClient http;
  char url[] = "http://ip-api.com/line?fields=offset";
  Serial.print("[HTTP] get_geo_time_offset begin...\n");
  Serial.println(url);
  int offset = 0;
  if (http.begin(client, url)) {  // HTTP

    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();
        offset = payload.toInt();
        Serial.println(offset);
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }

  return offset;
}

void render_temperature() {
  Serial.print("render_temperature ");
  Serial.println(teplota.get());
  disp.showNumberDec(teplota.get(), false, 2, 0);
  disp.setSegments(deg_celsius, 2, 2);
}

void render_humidity() {
  Serial.print("render_humidity ");
  Serial.println(vlhkost.get());
  disp.showNumberDec(vlhkost.get(), false, 2, 0);
  disp.setSegments(percent, 2, 2);
}

void render_time() {
  Serial.print("render_teim ");
  Serial.println(timeClient.getFormattedTime());
  disp.showNumberDecEx(timeClient.getHours(), 0b01000000, true, 2, 0);
  disp.showNumberDecEx(timeClient.getMinutes(), 0b01000000, true, 2, 2);
}

void connect_to_wifi() {
  WiFiManager wifiManager;
  wifiManager.autoConnect(AP_NAME);
}
