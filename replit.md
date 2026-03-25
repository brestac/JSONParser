# Custom C++ JSON Library

A header-only JSON serialization/deserialization library targeting both
**desktop C++17** (via `PointerCursor`) and **Arduino/ESP8266** (via `StreamCursor`),
with macro-driven struct-to-JSON field mapping.

---

## Build Targets

| Target | Command | Description |
|---|---|---|
| Desktop | `make && ./main` | Normal desktop build + run |
| Arduino path | `make arduino-test && ./arduino-test` | Compiles with `-DARDUINO -I./arduino -I.` using stub `arduino/Stream.h`; runs all 15 Stream-path tests |
| Debug build | `make main-debug` | Desktop build with `-O0`, no hardening |
| Clean | `make clean` | Removes `main`, `main-debug`, `arduino-test` |

---

## Architecture

### Cursor types

| Type | Header | Mode | Direction |
|---|---|---|---|
| `JSON::PointerCursor<T>` | `JSON/PointerCursor.h` | both | template base |
| `JSON::PointerCursorReader` = `PointerCursor<const char>` | | both | read |
| `JSON::PointerCursorWriter` = `PointerCursor<char>` | `PointerCursorWriter.h` (root) | both | write (buffer) |
| `JSON::PointerCursorPrinter` | `JSON/PointerCursorPrinter.h` | desktop only | write (stdout) |
| `JSON::StreamCursor` | `JSON/StreamCursor.h` | Arduino only | read + write |

### Key files

- `JSON/macros.h` — `STREAM_CURSOR_OVERRIDE`, `TO_JSON_FROM_JSON`, `PRINT_FUNC`
- `JSON/JSON.h` — public `parse`/`_parse`/`print` entry points
- `JSON/JSONData.h` — `JSONData` base class with pure-virtual `fromJSON`/`toJSON`
- `JSON/JSONStreamParser.h` — `JSONParserBase<Cursor>` template parser
- `JSON/JSONPrinter.h` — `print_json`, `print_key_value_pair`, `JSON::print`
- `JSON/types.h` — `is_cursor_reader_v`, `is_cursor_writer_v`, `key_value_checker_v`
- `JSON/UnknownValueType.h` — catch-all array element type
- `arduino/Stream.h` — minimal Arduino `Stream`/`StringStream`/`HardwareSerial` stubs
- `arduino/arduino_test.cpp` — 15-test suite for the Arduino code path

### Macro usage

```cpp
struct Sensor : public JSONData {
    int   id          = 0;
    float temperature = 0.0f;
    bool  active      = false;
    TO_JSON_FROM_JSON(id, temperature, active);
};
```

`TO_JSON_FROM_JSON(fields...)` expands to:
- `using JSONData::fromJSON; using JSONData::toJSON;`
- `STREAM_CURSOR_OVERRIDE(fields...)` which generates:
  - `fromJSON(PointerCursorReader)` + `toJSON(PointerCursorWriter)` (both modes)
  - `fromJSON(StreamCursor&)` + `toJSON(StreamCursor&)` + `fromJSON(T& stream)` template (Arduino only)
  - `toJSON(PointerCursorPrinter)` (desktop only)

---

## Known design notes

### `PointerCursorWriter` pass-by-value issue
`print_json`, `print_key_value_pair`, and `print_array_to` receive their `Cursor`
argument **by value**. For stream/printer cursors, copies write to the same
underlying destination so position is implicit. For `PointerCursorWriter`, each
copy tracks its own `_pos`; after nested calls that return a byte count, the
caller must manually `output.advance(n)`. This is done with `if constexpr` checks
guarded on `std::is_same_v<remove_cvref_t<Cursor>, JSON::PointerCursorWriter>`.

### `JSONParserBase<StreamCursor>` constructor (Arduino)
A plain non-template constructor `JSONParserBase(StreamCursor&)` exists only
under `#ifdef ARDUINO`. SFINAE via default template parameters caused clang to
reject instantiation of `JSONParserBase<PointerCursorReader>` when the
`enable_if` failed, so the template was removed and the constructor is now plain.
It is only ever called with `Cursor = StreamCursor`.

### `set_position` (Arduino)
`JSONStreamParser.h::parse_object` calls `_cursor.set_position(r.length)` after
a recursive parse. `StreamCursor` advances automatically; calling `set_position`
there would corrupt the read position. Guarded with `#ifndef ARDUINO` / `if
constexpr (!is_same_v<Cursor, StreamCursor>)`.

### `PointerCursorPrinter` specialisation (Arduino)
`types.h` specialises `is_cursor_writer<JSON::PointerCursorPrinter>` which is
only defined in desktop mode. Wrapped with `#ifndef ARDUINO`.
