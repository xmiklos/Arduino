#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <Ticker.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Value.h>
#include <Timer.h>
#include <EEPROM.h>

#define ONE_WIRE_BUS 4
#define MOTOR_OPEN  13
#define MOTOR_CLOSE 12
#define CLOSE_TEMP 43
#define CLOSE_AFTER 7200000
#define CLOSING_TIME 30000

WiFiUDP Udp;
Value<int> temp( -127 );
int close_temp = CLOSE_TEMP;
Timer temp_checker(15 * 1000);
Timer last_button_push(0, true, false);
Timer closing(CLOSING_TIME, false, false);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature temp_sensor(&oneWire);

ESP8266WebServer server(80);

void setup() {
  EEPROM.begin(64);
  EEPROM.get(0, close_temp);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MOTOR_OPEN, INPUT_PULLUP);
  pinMode(MOTOR_CLOSE, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(MOTOR_OPEN), up_down_isr, FALLING);
  attachInterrupt(digitalPinToInterrupt(MOTOR_CLOSE), up_down_isr, FALLING);
  
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);

  connect_to_wifi();

  server.on("/", handle_root);
  server.on("/temp", handle_temp);
  server.begin();

  Udp.begin(4444);
  temp_sensor.begin();
  get_temp();
}

ICACHE_RAM_ATTR void up_down_isr() {
  last_button_push.restart();
}

void loop() {

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);
    flash_thunder();
    server.handleClient();

    if (temp_checker.tick()) {
      get_temp();
    }

    if (
        temp.get() <= close_temp
        && last_button_push.elapsed() > CLOSE_AFTER
        && !closing.started()
    ) {
      Serial.print("closing start: ");
      Serial.println( last_button_push.elapsed() );
      pinMode(MOTOR_CLOSE, OUTPUT);
      digitalWrite(MOTOR_CLOSE, LOW);
      closing.start();
      start_blink();
    } else if (closing.tick()) {
      Serial.println("closing end");
      pinMode(MOTOR_CLOSE, INPUT_PULLUP);
      last_button_push.restart();
      stop_blink();
    }

  } else {
    start_blink();
    WiFi.begin();
    stop_blink();
  }
}

const char * html_start = "<html>"
  "<head>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
  "</head>"
  "<body>";

const char * html_end = "</body></html>";

void handle_root() {
  String message(html_start);
  message += "<h1>Kotol</h1><p>Teplota: ";
  message += temp.get();
  message += "&deg;C (<a href=\"javascript:location=location.pathname;\">Obnovit stranku</a>)</p>";

  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "set_close_temp") {
      String arg_temp = server.arg(i);
      long t = arg_temp.toInt();
      if (t) {
        close_temp = t;
        EEPROM.put(0, close_temp);
        EEPROM.commit();
        message += "<h5 style=\"color: red;\">Teplota uzatvorenia bola nastavena!</h5>";
      }
    }
  }

  message += "<form>Teplota uzatvorenia: <input type=\"number\" name=\"set_close_temp\" value=\"";
  message += close_temp;
  message += "\" size=\"4\" maxlength=\"2\">";
  message += "<input type=\"submit\" name=\"uloz\" value=\"Ulozit\"></form>";

  message += html_end;

  server.send(200, "text/html", message);
}

void handle_temp() {
  String message;
  message += temp.get();
  server.send(200, "text/text", message);
}

void get_temp() {
  temp_sensor.requestTemperatures();
  int t = temp_sensor.getTempCByIndex(0);
  Serial.printf("Temperature is: %d\n", t);
  temp.set( t );

  if (temp.change()) {
    send_temp(t);
  }
}

Ticker blinker;
void start_blink() {
  blinker.attach(0.3, blink);
}

void stop_blink() {
  blinker.detach();
}

void blink() {
  int state = digitalRead(LED_BUILTIN);
  digitalWrite(LED_BUILTIN, !state);
}

char udp_buffer[UDP_TX_PACKET_MAX_SIZE + 1];
char thunder[] = "thunder";

void flash_thunder() {
  int packet_size = Udp.parsePacket();
  if (packet_size) {
    int n = Udp.read(udp_buffer, UDP_TX_PACKET_MAX_SIZE);
    udp_buffer[ n ] = 0;
    Serial.println("Udp packet contents:");
    Serial.println(udp_buffer);

    if (strcmp(udp_buffer, "flash") == 0) {
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      Udp.write(thunder);
      Udp.endPacket();
    } else if (strcmp(udp_buffer, "temp") == 0) {
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      sprintf(udp_buffer, "%d", temp.get());
      Udp.write(udp_buffer);
      Udp.endPacket();
    }
  }
}

void connect_to_wifi() {
  WiFiManager wifiManager;
  wifiManager.autoConnect("Kotol");
}

void send_temp(int temp) {
  WiFiClient client;

  HTTPClient http;
  char format[] = "http://api.thingspeak.com/update?api_key=8P0I91KOA45B1EIA&field1=%d";
  char url[ sizeof(format) + 5 ];
  sprintf(url, format, temp);

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
