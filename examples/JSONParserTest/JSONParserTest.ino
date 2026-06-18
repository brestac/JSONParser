#define JSON_DEBUG_LEVEL 0
#define WIFI_SSID "wifix"
#define WIFI_PASSWORD "rosebude"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <StreamString.h>
#include "src/JSONParser.h"
#include "src/JSONPrinter.h"
#include "./test.h"

bool connectWifi();

static uint32_t free_heap = 0U;
static uint32_t free_stack = 0U;

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

  free_heap = ESP.getFreeHeap();
  free_stack = ESP.getFreeContStack();

  Serial.println("");
//  run_tests();

  test_http_stream(WIFI_SSID, WIFI_PASSWORD, "http://192.168.1.2:10000/canada.json");
}

void loop() {
  // static bool printed = false;
  // if (printed == false) {
  //   Serial.printf("Free heap: %u => %u\n", free_heap, ESP.getFreeHeap());
  //   Serial.printf("Free stack: %u => %u\n", free_stack, ESP.getFreeContStack());
  //   Serial.printf("GLOBAL_STRING_POOL_SIZE=%zu\n", JSON::GLOBAL_STRING_POOL_SIZE);
  //   printed = true;
  // }

  delay(10);
}

bool connectWifi(const char*ssid, const char*pwd) {
  DEBUG_PRINTLN("Connecting as wifi client...");
  if (WiFi.status() == WL_CONNECTED) return true;

  // WIFI
  WiFi.mode(WIFI_STA);
  uint8_t mac[6] = {170,0,0,0,0,11};
  wifi_set_macaddr(STATION_IF, const_cast<uint8 *>(mac));

  WiFi.begin(ssid, pwd);

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
    Serial.println(WiFi.localIP());

    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
  }

  return true;
}

void test_http_stream(const char*ssid, const char*pwd, String url) {
  WiFiClient client;
  HTTPClient http;

  bool connected = connectWifi(ssid, pwd);
  if (!connected) {
    Serial.println("[WIFI] Cannot connect\n");
    return;
  }

  Serial.print("[HTTP] begin...\n");

  // configure server and url
  http.addHeader("Content-Type", "text/plain");

  bool begin = http.begin(client, url);
  if (!begin) {
    Serial.println("[HTTP] Cannot connect to host");
    return;
  }
  Serial.printf("[HTTP] GET %s\n", url.c_str());
  // start connection and send HTTP header
  int httpCode = http.GET();

  if (httpCode < 0) {
    Serial.printf("[HTTP] Connection refused %d\n", httpCode);
    return;
  }

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[HTTP] Bad response %d\n", httpCode);
    http.end();
    return;
  }

  WiFiClient *stream = http.getStreamPtr();

  unsigned long timer = millis();
  while (stream->available() <= 0) {
    if (millis() - timer > 5000) {
      Serial.println("[HTTP] stream timeout");
      return;
    }
    delay(10);
  }

  test_parse_geojson_big_with_limited_geometry_from_stream(stream);
}
