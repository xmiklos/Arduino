#include <TM1637Display.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Value.h>

#define PIN_DISP_CLK  0
#define PIN_DISP_DIO  2

#define AP_NAME       "Watch"
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

TM1637Display disp(PIN_DISP_CLK, PIN_DISP_DIO);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  disp.setBrightness(10);
  //Serial.println("connecting...");
  connect_to_wifi();
  timeClient.begin();
  timeClient.update();
  set_geo_time_offset();
}

Value<int> hour(-1);

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    //Serial.println("connected");
    timeClient.update();

    if (hour.change( timeClient.getHours() )) {
      set_geo_time_offset();
      Serial.println( timeClient.getFormattedTime() );
      // when offset changes hour, avoid update
      hour.change( timeClient.getHours() );
    }

    disp.showNumberDecEx(timeClient.getHours(), 0b01000000, true, 2, 0);
    disp.showNumberDecEx(timeClient.getMinutes(), 0b01000000, true, 2, 2);

  } else {
    connect_to_wifi();
  }
}

void set_geo_time_offset() {
  WiFiClient client;
  HTTPClient http;
  char url[] = "http://ip-api.com/line?fields=offset";
  Serial.print("[HTTP] get_geo_time_offset begin...\n");
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
        timeClient.setTimeOffset( payload.toInt() );
        Serial.println(payload);
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }
}

void connect_to_wifi() {
  WiFiManager wifiManager;
  wifiManager.autoConnect(AP_NAME);
}
