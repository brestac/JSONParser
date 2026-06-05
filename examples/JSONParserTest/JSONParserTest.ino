#define JSON_DEBUG_LEVEL 0
#define WIFI_SSID "wifix"
#define WIFI_PASSWORD "rosebude"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <StreamString.h>
#include "src/JSONParser.h"
#include "src/JSONPrinter.h"
#include "./test.h"

void test_parse_from_tcp_stream(const char* url);
bool connectWifi();

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  LittleFS.begin();

  Serial.println("");
  //   Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
  //   Serial.printf("Free stack: %u\n", ESP.getFreeContStack());
  //run_tests();

  test_parse_from_tcp_stream("http://192.168.1.2:9000/sensor.json");
}

void loop() {
  delay(10);
}

bool connectWifi() {
  DEBUG_PRINTLN("Connecting as wifi client...");
  if (WiFi.status() == WL_CONNECTED) return true;

  // WIFI
  WiFi.mode(WIFI_STA);
  uint8_t macAddress[6] = { 170, 0, 0, 0, 0, 1 };
  wifi_set_macaddr(STATION_IF, const_cast<uint8*>(macAddress));

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint8_t numberOftry = 0;
  while (WiFi.status() != WL_CONNECTED && numberOftry < 10) {
    Serial.printf("... WiFi connecting status:%d\n", WiFi.status());
    delay(1000);
    ++numberOftry;
  }

  if (++numberOftry >= 10) {
    Serial.printf("... WiFi timeout status:%d\n", WiFi.status());
    return false;
  } else {
    Serial.printf("WiFi connected status:%d\n", WiFi.status());
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
  }

  return true;
}

void test_parse_from_tcp_stream(const char* url) {
  bool connected = connectWifi();
  if (!connected) return;

  WiFiClient client;
  HTTPClient http;  // must be declared after WiFiClient for correct destruction order, because used by http.begin(client,...)

  Serial.print("[HTTP] begin...\n");

  // configure server and url
  bool begin = http.begin(client, url);
  if (!begin) {
    // check(false, "Cannot connect to 192.168.1.2. Is the server running ? On a Mac, you can do 'ruby -run -e httpd . -p 9000'");
    return;
  }

  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();

  if (httpCode < 0) {
    return;
  }

  if (httpCode != HTTP_CODE_OK) {
    http.end();
    return;
  }

  WiFiClient* stream = http.getStreamPtr();

  unsigned long timer = millis();
  while (stream->available() <= 0) {
    if (millis() - timer > 5000) {
      return;
    }
    delay(10);
  }

  Sensor s;
  JSON::ParseResult r = s.fromJSON(stream);
  http.end();
  check(r.error == 0, "parse error = %hhu length=%zu", r.error, r.length);
  s.toJSON(Serial);
}
