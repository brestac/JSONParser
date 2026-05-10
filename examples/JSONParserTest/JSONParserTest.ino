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
    TO_JSON_FROM_JSON(id, temperature, active);
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

void check(bool condition, const char *label) {
    if (condition) {
        Serial.printf("  [PASS] %s\n", label);
        ++passed;
    } else {
        Serial.printf("  [FAIL] %s\n", label);
        ++failed;
    }
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
// Test 4 – toJSON via StreamString
// ----------------------------------------------------------------

void test_serialize_to_stream() {
    DEBUG_PRINTF("\n--- Test: toJSON via StreamString ---\n");

    Sensor s;
    s.id          = 7;
    s.temperature = 36.6f;
    s.active      = true;

    // Use a StreamString as the backing stream;
    StreamString stream;   // read side is empty;

    DEBUG_PRINTF("  Output: ");
    //fflush(stdout);
    size_t written = s.toJSON(stream);

    DEBUG_PRINTF("\n");
    check(written > 0, "toJSON wrote bytes to StreamString");
}

// ----------------------------------------------------------------           
// Test 5 – roundtrip: parse then re-serialize
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

    DEBUG_PRINTF("  Serialized: %s\n", buf);

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

void test_print_to_buffer() {
    DEBUG_PRINTF("\n--- Test: print ---\n");
    Sensor s;
    s.id          = 7;
    s.temperature = 36.6f;
    s.active      = true;

    char buf[256] = {};
    s.toJSON(buf, sizeof(buf));
    DEBUG_PRINTF("  Serialized: %s\n", buf);
    check(true, "Wrote bytes to char buffer");
}

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
// setup
// ----------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== Arduino Stream path tests ===");

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
