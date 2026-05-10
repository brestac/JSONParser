Usage
```c++
struct MyStruct : JSONObject {
  float temp;
  uint64_t timestamp;

  size_t fromJSON(const char* json) {
    updated = JSON.parse(json, "temp", temp, "timestamp", timestamp);
  }
  void toJSON(Stream& stream, bool showUpdates = false) {
    JSON.print(showUpdates ? updated : 0, stream, "temp", temp, "timestamp", timestamp);
  }
}

MyStruct sensor{25, 321654987};
sensor.fromJSON("{temp:22}");
sensor.toJSON(Serial, true); // prints {"temp":22}
sensor.toJSON(Serial, false); // prints {"temp":22, "timestamp":321654987}
```