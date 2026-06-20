Usage

```c++
struct Person : JSONObject {
  std::string_view name = "";
  uint8_t age = 0;

  JSON_SERIALIZE_IMPL(name, age);
}

MyStruct sensor{"William", 32};
sensor.fromJSON("{age:22}");
sensor.toJSON(Serial); // prints full object: {name:"William", "age":22}
sensor.toJSON(Serial, true); // prints updated values: {"age":22}
```

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