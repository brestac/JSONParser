# Usage

```c++
  struct Person : JSONObject {
    std::string_view name = "";
    uint8_t age = 0;
  
    JSON_SERIALIZE_IMPL(name, age);
  }
  
  Person person{"William", 32};
  person.fromJSON("{age:22}");
  
  person.toJSON(Serial); // prints full object: {name:"William", "age":22}
  person.toJSON(Serial, true); // prints updated values: {"age":22}
```

## Parse top level array
```c++
  Person persons[2];
  JSON::parse("[{\"name\":\"Jean\",age:65}, {\"name\":\"Amélie\", age:27}]", persons);
```

## Parse with a callback
```c++
  Person persons[2];
  JSON::parse("[{\"name\":\"Jean\",age:65}, {\"name\":\"Amélie\", age:27}, {\"name\":\"Bob\", age:36}]", [] (JSONKey& key, JSONValue& value, bool& stop) {
    switch (key) {
      case "name"_hash : std::printf("Parsed name:%.*s", (int)value.length(), value.data());
        break;
      case "age"_hash : std::printf("Parsed age:%hhu", value);
        break;
    }

    // We stop after parsing index 1
    if (key.getArrayIndex() >= 1) {
      stop = true;
    }
  }
```

## Features
- Handles array overflow. The parser just skips to the end of the json array
- Handle escaped sequences in strings (char[N] or string_view)
- Handles Arduino streams. Input can be a const char* buffer or an Arduino Stream (File, StreamString, WiFiClient)

| JSON type | C++ type |
|-----------|----------|
| string    | char[N], std::string_view|
| boolean   | bool     |
| integer   | int64_t, int32_t, int16_t, int8_t |
| float     | float, double |
| null      | nullptr |
| Array     | json_type[N], std::array<json_type, N>, std::vector<json_type>, JSONObjectDerived[N] |
| Object    | JSONObject derived |
| Hex string | uint32_t[N], uint16_t[N],uint8_t[N] |