// arduino_test.cpp
// Tests the Arduino/Stream code paths of the JSON library on desktop.
// Compile with:  make arduino-test
// Run with:      ./arduino-test
#define JSON_DEBUG_LEVEL 1
#define __ENABLE_TESTS__ 1
#define ARDUINO 1

#if __ENABLE_TESTS__ == 1

#include "../JSON/JSONParser.h"
#include "../JSON/JSONPrinter.h"

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

static void check(bool condition, const char *label) {
    if (condition) {
        printf("  [PASS] %s\n", label);
        ++passed;
    } else {
        printf("  [FAIL] %s\n", label);
        ++failed;
    }
}

// ----------------------------------------------------------------
// Test 1 – fromJSON via StringStream
// ----------------------------------------------------------------

static void test_parse_from_stream() {
    printf("\n--- Test: fromJSON via StreamCursor ---\n");

    const char *json = "{\"id\":42,\"temperature\":23.5,\"active\":true}";

    StringStream stream(json);
    JSON::StreamCursor cursor(stream);

    Sensor s;
    JSON::ParseResult result = s.fromJSON(cursor);

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

static void test_partial_parse() {
    printf("\n--- Test: partial fromJSON ---\n");

    Sensor s;
    s.id          = 99;
    s.temperature = 10.0f;
    s.active      = false;

    // Only update "active"
    const char *json = "{\"active\":true}";
    StringStream stream(json);
    JSON::StreamCursor cursor(stream);

    JSON::ParseResult result = s.fromJSON(cursor);

    check(!result.error,         "parse succeeded");
    check(s.id == 99,            "id unchanged (99)");
    check(s.active == true,      "active updated to true");
}

// ----------------------------------------------------------------
// Test 3 – fromJSON with template Stream helper (fromJSON(T& stream))
// ----------------------------------------------------------------

static void test_parse_via_stream_template() {
    printf("\n--- Test: fromJSON(Stream&) template helper ---\n");

    const char *json = "{\"version\":3,\"interval\":0.5}";
    StringStream stream(json);

    Config c;
    // Call the template helper that wraps Stream in StreamCursor internally
    JSON::ParseResult result = c.fromJSON(stream);

    check(!result.error,                    "parse succeeded");
    check(c.version == 3,                   "version == 3");

    float diff = c.interval - 0.5f;
    check(diff > -0.01f && diff < 0.01f,   "interval ≈ 0.5");
}

// ----------------------------------------------------------------
// Test 4 – toJSON via StreamCursor (output goes to stdout)
// ----------------------------------------------------------------

static void test_serialize_to_stream() {
    printf("\n--- Test: toJSON via StreamCursor ---\n");

    Sensor s;
    s.id          = 7;
    s.temperature = 36.6f;
    s.active      = true;

    // Use a StringStream as the backing stream; writes go to stdout via print()
    StringStream stream("");   // read side is empty; write side uses stdout
    JSON::StreamCursor cursor(stream);

    printf("  Output: ");
    fflush(stdout);

    size_t written = s.toJSON(cursor);

    printf("\n");
    check(written > 0, "toJSON wrote bytes");
}

// ----------------------------------------------------------------
// Test 5 – roundtrip: parse then re-serialize
// ----------------------------------------------------------------

static void test_roundtrip() {
    printf("\n--- Test: roundtrip parse → serialize ---\n");

    Sensor original;
    original.id          = 55;
    original.temperature = 19.8f;
    original.active      = false;

    // Serialize original to a char buffer via PointerCursorWriter
    char buf[256] = {};
    original.toJSON(buf, sizeof(buf));

    printf("  Serialized: %s\n", buf);

    // Parse that buffer back via a stream
    StringStream stream(buf);
    JSON::StreamCursor cursor(stream);

    Sensor copy;
    JSON::ParseResult result = copy.fromJSON(cursor);

    check(!result.error,         "roundtrip parse succeeded");
    check(copy.id == 55,         "roundtrip id == 55");
    check(copy.active == false,  "roundtrip active == false");

    float diff = copy.temperature - 19.8f;
    check(diff > -0.1f && diff < 0.1f, "roundtrip temperature ≈ 19.8");
}

static void test_print_to_buffer() {
    printf("\n--- Test: print ---\n");
    Sensor s;
    s.id          = 7;
    s.temperature = 36.6f;
    s.active      = true;

    char buf[256] = {};
    s.toJSON(buf, sizeof(buf));
    printf("  Serialized: %s\n", buf);
}

static void test_print_to_serial() {
    printf("\n--- Test: print ---\n");
    Sensor s;
    s.id          = 7;
    s.temperature = 36.6f;
    s.active      = true;
    s.toJSON(Serial);
}
// ----------------------------------------------------------------
// main
// ----------------------------------------------------------------

int main() {
    printf("=== Arduino Stream path tests ===\n");

    test_parse_from_stream();
    test_partial_parse();
    test_parse_via_stream_template();
    test_serialize_to_stream();
    test_roundtrip();

    test_print_to_buffer();
    test_print_to_serial();

    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed == 0 ? 0 : 1;
}

#else
int main() { return 0;}
#endif
