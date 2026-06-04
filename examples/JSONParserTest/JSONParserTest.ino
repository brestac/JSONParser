#define JSON_DEBUG_LEVEL 1
#define WIFI_SSID "wifix"
#define WIFI_PASSWORD "rosebude"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <StreamString.h>
#include "src/JSONParser.h"
#include "src/JSONPrinter.h"
#include "./test.h"

void setup() {
  Serial.begin(115200);
  delay(500);

  LittleFS.begin();

  Serial.println("");
  //   Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
  //   Serial.printf("Free stack: %u\n", ESP.getFreeContStack());
  //run_tests();
  test_parse_geojon_from_tcp_stream();
}

void loop() {
  delay(10);
}

void connectWifi() {
  DEBUG_PRINTLN("Connecting as wifi client...");

  // WIFI
  WiFi.mode(WIFI_STA);
  uint8_t macAddress[6] = {170, 0, 0, 0, 0, 1};
  wifi_set_macaddr(STATION_IF, const_cast<uint8 *>(macAddress));

  WiFi.onStationModeConnected([](const WiFiEventStationModeConnected &event) {
    Serial.println("Station connected");
  });

  WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected &event) {
    Serial.println("Station disconnected");
    connectWifi();
  });

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint8_t numberOftry = 0;
  while (WiFi.status() != WL_CONNECTED && numberOftry < 10) {
    Serial.printf("... WiFi connecting status:%d\n", WiFi.status());
    delay(1000);
  }

  if (++numberOftry >= 10) {
    Serial.printf("... WiFi timeout status:%d\n", WiFi.status());
  } else {
    Serial.printf("WiFi connected status:%d\n", WiFi.status());
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
  }
}

void test_parse_geojon_from_tcp_stream() {
    connectWifi();

    WiFiClient client;
    HTTPClient http;  // must be declared after WiFiClient for correct destruction order, because used by http.begin(client,...)

    Serial.print("[HTTP] begin...\n");

    // configure server and url
    bool begin = http.begin(client, "http://192.168.1.2:9000/data.geojson");
    if (!begin) {
      return;
    }

    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      WiFiClient* clientPtr = http.getStreamPtr();
      Serial.printf("Available %d\n", clientPtr->available());
      test_parse_geojson_from_stream(clientPtr);
    }
}
