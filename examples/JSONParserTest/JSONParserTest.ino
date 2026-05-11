#define JSON_DEBUG_LEVEL 0

#include <StreamString.h>
#include "src/JSONParser.h"
#include "src/JSONPrinter.h"

// ----------------------------------------------------------------
// Test structs
// ----------------------------------------------------------------

struct Sensor : public JSONObject {
    int   id          = 0;
    float temperature = 0.0f;
    bool  active      = false;
    char name[64];
    uint8_t num[1] = {1};
    TO_JSON_FROM_JSON(id, active, name /*,temperature, num*/);
};

struct Config : public JSONObject {
    int   version  = 0;
    float interval = 0.0f;
    TO_JSON_FROM_JSON(version, interval);
};

// ----------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------

static int passed = 0;
static int failed = 0;

template <typename... Args>
void check(bool condition, const char *format, Args&&... args) {
  if (condition) {
    Serial.printf("  [PASS] ");
    ++passed;
  } else {
    Serial.printf("  [FAIL] ");
    ++failed;
  }
  if constexpr (sizeof...(Args) > 0) {
    Serial.printf(format, std::forward<Args>(args)...);
    Serial.println("");
  } else {
    Serial.printf("%s\n", format);
  }
}
// ----------------------------------------------------------------
// Test 1 – fromJSON via char buffer
// ----------------------------------------------------------------

void test_parse_from_char_buffer() {
    DEBUG_PRINTF("\n--- Test: fromJSON via StreamCursor ---\n");

    const char *json = "{\"id\":42,\"name\":\"abc\",\"temperature\":23.5,\"active\":true, \"num\":[1,2,3]}";

    Sensor s;
    JSON::ParseResult result = s.fromJSON(json);

    check(!result.error,         "parse succeeded");
    check(s.id == 42,            "id == 42");
    check(strcmp(s.name, "abc") == 0,            "name == '%s'", s.name);
    check(s.active == true,      "active == true");
    check(s.num[0] == 1 && s.num[1] == 2 && s.num[2] == 3, "num == [1,2,3]");

    // temperature comparison with a tolerance
    float diff = s.temperature - 23.5f;
    check(diff > -0.01f && diff < 0.01f, "temperature ≈ 23.5");
}
// ----------------------------------------------------------------
// Test 1 – fromJSON via StreamString
// ----------------------------------------------------------------

void test_parse_from_stream() {
    DEBUG_PRINTF("\n--- Test: fromJSON via StreamCursor ---\n");

    const char *json = "{\"id\":42,\"temperature\":23.5,\"active\":true}";

    StreamString stream(json);

    Sensor s;
    JSON::ParseResult result = s.fromJSON(stream);

    check(!result.error,         "parse succeeded");
    check(s.id == 42,            "id == 42");
    check(s.active == true,      "active == true");

    // temperature comparison with a tolerance
    float diff = s.temperature - 23.5f;
    check(diff > -0.01f && diff < 0.01f, "temperature ≈ 23.5");
}

// ----------------------------------------------------------------
// Test 2 – fromJSON partial update
// ----------------------------------------------------------------

void test_partial_parse() {
    DEBUG_PRINTF("\n--- Test: partial fromJSON ---\n");

    Sensor s;
    s.id          = 99;
    s.temperature = 10.0f;
    s.active      = false;

    // Only update "active"
    const char *json = "{\"active\":true}";
    StreamString stream(json);

    JSON::ParseResult result = s.fromJSON(stream);

    check(!result.error,         "parse succeeded");
    check(s.id == 99,            "id unchanged (99)");
    check(s.active == true,      "active updated to true");
}

// ----------------------------------------------------------------
// Test 3 – fromJSON with StreamString
// ----------------------------------------------------------------

void test_parse_via_stream_template() {
    DEBUG_PRINTF("\n--- Test: fromJSON(Stream&) template helper ---\n");

    const char *json = "{\"version\":3,\"interval\":0.5}";
    StreamString stream(json);

    Config c;
    JSON::ParseResult result = c.fromJSON(stream);

    check(!result.error,                    "parse succeeded");
    check(c.version == 3,                   "version == 3");

    float diff = c.interval - 0.5f;
    check(diff > -0.01f && diff < 0.01f,   "interval ≈ 0.5");
}

// ----------------------------------------------------------------
// Test 4 – toJSON char buffer
// ----------------------------------------------------------------

void test_print_to_buffer() {
    DEBUG_PRINTF("\n--- Test: print ---\n");
    Sensor s;
    s.id          = 7;
    s.temperature = 36;
    s.active      = true;

    char buf[256] = {};
    size_t written = s.toJSON(buf, sizeof(buf));
    check(written > 0, "Wrote %zu bytes to char buffer: '%s'\n", written, buf);
}

// ----------------------------------------------------------------
// Test 5 – toJSON via StreamString
// ----------------------------------------------------------------

void test_serialize_to_stream() {
    DEBUG_PRINTF("\n--- Test: toJSON via StreamString ---\n");

    Sensor s;
    s.id          = 7;
    s.temperature = 36.55f;
    s.active      = true;
    strncpy(s.name, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", sizeof(s.name));

    // Use a StreamString as the backing stream;
    StreamString stream;

    size_t written = s.toJSON(stream);
    check(written > 0, "toJSON wrote %zu bytes to StreamString: '%s'\n", written, stream.c_str());
}

// ----------------------------------------------------------------
// Test 6 – toJSON Serial
// ----------------------------------------------------------------

void test_print_to_serial() {
    DEBUG_PRINTF("\n--- Test: print ---\n");
    Sensor s;
    s.id          = 7;
    s.temperature = 36.6f;
    s.active      = true;

    s.toJSON(Serial);
    check(true, "Wrote to Serial");
}

// ----------------------------------------------------------------
// Test 7 – roundtrip: parse then re-serialize
// ----------------------------------------------------------------

void test_roundtrip() {
    DEBUG_PRINTF("\n--- Test: roundtrip parse → serialize ---\n");

    Sensor original;
    original.id          = 55;
    original.temperature = 19.8f;
    original.active      = false;

    // Serialize original to a char buffer via PointerCursorWriter
    char buf[256] = {};
    original.toJSON(buf, sizeof(buf));

    // Parse that buffer back via a stream
    StreamString stream(buf);

    Sensor copy;
    JSON::ParseResult result = copy.fromJSON(stream);

    check(!result.error,         "roundtrip parse succeeded");
    check(copy.id == 55,         "roundtrip id == 55");
    check(copy.active == false,  "roundtrip active == false");

    float diff = copy.temperature - 19.8f;
    check(diff > -0.1f && diff < 0.1f, "roundtrip temperature ≈ 19.8");
}
// ----------------------------------------------------------------
// setup
// ----------------------------------------------------------------

void setup() {
    delay(500);
    Serial.begin(115200);
    delay(1000);
    Serial.flush();

    Serial.println("=== Arduino JSON parser tests ===");

    test_parse_from_char_buffer();
    test_parse_from_stream();
    test_partial_parse();
    test_parse_via_stream_template();

    test_print_to_buffer();
    test_serialize_to_stream();
    test_print_to_serial();

    test_roundtrip();

    Serial.printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
}

void loop() {
    delay(10);
}
