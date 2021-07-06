#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include "DHT.h"
#include <Adafruit_MLX90614.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

const uint8_t thingspeak_fingerprint[20] = {
  0x27, 0x18, 0x92, 0xdd, 0xa4, 0x26, 0xc3, 0x07, 0x09, 0xb9, 0x7a, 0xe6, 0xc5, 0x21, 0xb9, 0x5b, 0x48, 0xf7, 0x16, 0xe1
};
const uint8_t pushover_fingerprint[20] = {
  0x8c, 0x6d, 0x45, 0x12, 0xec, 0x9a, 0x1c, 0x50, 0x8c, 0x23, 0x58, 0xa8, 0x0f, 0xaf, 0x97, 0xa1, 0xc0, 0x36, 0x8b, 0xee
};

#include <Value.h>
#include <Timer.h>

#define PIN_TEMP      0

#define AP_NAME       "DewPointWarning"

DHT                 dht(PIN_TEMP, DHT11);
Adafruit_MLX90614   mlx = Adafruit_MLX90614();

Value<double> t_objekt( 0, 0, 99);
Value<double> t_mistnost( 0, 0, 99);
Value<double> vlhkost( 0, 0, 99 );
Value<double> rosny_bod( 0, 0, 99 );
Value<double> result( 0, 0, 99 );

void setup() {
  Serial.begin(9600);
  while (!Serial);

  connect_to_wifi();
  dht.begin();
  mlx.begin();

  measure_and_send();
}

Timer should_measure(60000, true, true);
Timer pushover_limiter(1800000, false, false);

void loop() {
  pushover_limiter.tick();

  if (WiFi.status() == WL_CONNECTED) {

    if (should_measure.tick()) {
      measure_and_send();
    }

  } else {
    connect_to_wifi();
  }
}

void measure_and_send() {
  read_sensors();

  char buffer[60] = "";
  if (t_objekt.change()) {
    Serial.print("Zmena: t_objekt=");
    Serial.println(t_objekt.get());
    add_field(buffer, 1, t_objekt.get());
  }
  if (t_mistnost.change()) {
    Serial.print("Zmena: t_mistnost=");
    Serial.println(t_mistnost.get());
    add_field(buffer, 2, t_mistnost.get());
  }
  if (vlhkost.change()) {
    Serial.print("Zmena: vlhkost=");
    Serial.println(vlhkost.get());
    add_field(buffer, 3, vlhkost.get());
  }
  if (rosny_bod.change()) {
    Serial.print("Zmena: rosny_bod=");
    Serial.println(rosny_bod.get());
    add_field(buffer, 4, rosny_bod.get());
  }

  if (result.change()) {
    add_field(buffer, 5, result.get());
  }

  if ( strlen( buffer ) ) {
    send_thingspeak( buffer );
  }

  if (t_objekt.get() <= rosny_bod.get() || t_mistnost.get() <= rosny_bod.get()) {
    if (!pushover_limiter.started()) {
      send_dew_point_warning();
      pushover_limiter.start();
    }
  }
}

void send_thingspeak(char * url_params) {

  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setFingerprint(thingspeak_fingerprint);

  HTTPClient https;
  char base[] = "https://api.thingspeak.com/update?api_key=N9WZ5H2BU44NVEHE&";
  char url[ strlen(base) + strlen(url_params) + 1 ];
  strcpy(url, base);
  strcat(url, url_params);

  Serial.println("[HTTPS] send_thingspeak begin...");
  Serial.println(url);

  if (https.begin(*client, url)) {  // https
    int httpsCode = https.GET();

    if (httpsCode > 0) {
      // https header has been send and Server response header has been handled
      Serial.printf("[HTTPS] GET... code: %d\n", httpsCode);

      if (httpsCode == HTTP_CODE_OK || httpsCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = https.getString();
        Serial.println(payload);
      }
    } else {
      Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpsCode).c_str());
    }

    https.end();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
}

void send_dew_point_warning() {
  char message[1000] = "";
  add_param(message, "token", "azfzyayruc26ny786dvvymon8kmbbb");
  add_param(message, "user", "umb6ykb1b4i7mnn1xaccvq87isdpug");
  add_param(message, "url", "https://thingspeak.com/channels/1193124");
  add_param(message, "url_title", "See temperatures");
  add_param(message, "priority", "1");
  add_param(message, "message", "Objekt dosiahol teplotu rosneho bodu!");

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

void read_sensors() {

  for (int i = 0; i < 3; ++i) {
    double t = dht.readTemperature();
    double h = dht.readHumidity();
      Serial.println(t);
      Serial.println(h);
    if (isnan(t) || isnan(h)) {
      Serial.println("sensors read failed!");
      Serial.println(t);
      Serial.println(h);
      delay(2000);
      continue;
    }

    t_mistnost.set( one_decimal(t) );
    vlhkost.set( one_decimal(h) );
    rosny_bod.set( one_decimal( dewPointFast(t, h) ) );
    break;
  }

  double t_obj = mlx.readObjectTempC();
  Serial.println(t_obj);
  if (t_obj < 100) {
    t_objekt.set( one_decimal( t_obj ) );
  }

  result.set( t_objekt.get() - rosny_bod.get() );
}

void add_field(char * result, int field, double value) {
  char buffer[10];

  if (strlen(result)) {
    strcat(result, "&");
  }
  strcat(result, "field");
  sprintf(buffer, "%d", field);
  strcat(result, buffer);
  strcat(result, "=");
  sprintf(buffer, "%.1f", value);
  strcat(result, buffer);
}

void add_param(char * result, char * key, char * value) {

  if (strlen(result)) {
    strcat(result, "&");
  }
  strcat(result, key);
  strcat(result, "=");
  strcat(result, value);
}

double one_decimal(double in) {
  return round( in * 10 ) / 10;
}

double dewPointFast(double celsius, double humidity)
{
  double a = 17.271;
  double b = 237.7;
  double temp = (a * celsius) / (b + celsius) + log(humidity * 0.01);
  double Td = (b * temp) / (a - temp);
  return Td;
}

void connect_to_wifi() {
  WiFiManager wifiManager;
  wifiManager.autoConnect(AP_NAME);
}
