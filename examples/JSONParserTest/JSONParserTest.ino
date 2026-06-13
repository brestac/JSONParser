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
WiFiClient* get_http_stream(HTTPClient& http, WiFiClient& client, const char*ssid, const char*pwd, const char* url);

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
  //run_tests();

  WiFiClient client;
  HTTPClient http;

  WiFiClient *stream = get_http_stream(http, client, WIFI_SSID, WIFI_PASSWORD, "http://192.168.1.2:9000/canada.json");
  test_parse_geojson_big_with_limited_geometry_from_stream(stream);
  http.end();
}

void loop() {
  static bool printed = false;
  if (printed == false) {
    Serial.printf("Free heap: %u => %u\n", free_heap, ESP.getFreeHeap());
    Serial.printf("Free stack: %u => %u\n", free_stack, ESP.getFreeContStack());
    Serial.printf("GLOBAL_STRING_POOL_SIZE=%zu\n", JSON::GLOBAL_STRING_POOL_SIZE);
    printed = true;
  }

  delay(10);
}

bool connectWifi(const char*ssid, const char*pwd) {
  DEBUG_PRINTLN("Connecting as wifi client...");
  if (WiFi.status() == WL_CONNECTED) return true;

  // WIFI
  WiFi.mode(WIFI_STA);
  uint8_t macAddress[6] = { 170, 0, 0, 0, 0, 1 };
  wifi_set_macaddr(STATION_IF, const_cast<uint8*>(macAddress));

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
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
  }

  return true;
}

WiFiClient* get_http_stream(HTTPClient& http, WiFiClient& client, const char*ssid, const char*pwd, const char* url) {

  bool connected = connectWifi(ssid, pwd);
  if (!connected) return nullptr;

  Serial.print("[HTTP] begin...\n");

  // configure server and url
  bool begin = http.begin(client, url);
  if (!begin) {
    // check(false, "Cannot connect to 192.168.1.2. Is the server running ? On a Mac, you can do 'ruby -run -e httpd . -p 9000'");
    return nullptr;
  }

  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();

  if (httpCode < 0) {
    return nullptr;
  }

  if (httpCode != HTTP_CODE_OK) {
    http.end();
    return nullptr;
  }

  WiFiClient *stream = http.getStreamPtr();

  unsigned long timer = millis();
  while (stream->available() <= 0) {
    if (millis() - timer > 5000) {
      return nullptr;
    }
    delay(10);
  }
  
  return stream;
}
