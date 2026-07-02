#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <functional>

#include "./env.h"

static bool connectWifi() {
  if (WiFi.status() == WL_CONNECTED) return true;

  // WIFI
  WiFi.mode(WIFI_STA);
  uint8_t mac[6] = {170,0,0,0,0,11};
  wifi_set_macaddr(STATION_IF, const_cast<uint8 *>(mac));

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint8_t numberOftry = 0;
  while (WiFi.status() != WL_CONNECTED && numberOftry < 15) {
    Serial.printf("... WiFi connecting status:%d\n", WiFi.status());
    delay(500);
    ++numberOftry;
  }

  if (++numberOftry >= 15) {
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

static void get_stream(String url, const std::function<void(WiFiClient*)>& callback) {
  WiFiClient client;
  HTTPClient http;

  bool connected = connectWifi();
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

  callback(stream);
}
