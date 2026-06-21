all: desktop-test

CXX = clang++
override CXXFLAGS += -g -Wall -Werror -DJSON_DEBUG_LEVEL_NONE=0 -DJSON_DEBUG_LEVEL_INFO=1 -DJSON_DEBUG_LEVEL_WARNING=2 -DJSON_DEBUG_LEVEL_ERROR=3

SRC_HEADERS   = $(wildcard src/*.h)
TEST_HEADERS  = $(wildcard tests/*.h)
HEADERS       = $(SRC_HEADERS) $(TEST_HEADERS)

desktop-test: tests/desktop.cpp $(HEADERS)
	@mkdir -p build
	cp -n tests/canada.json build/canada.json && $(CXX) $(CXXFLAGS) -O2 -std=gnu++17 -I. tests/desktop.cpp -o build/desktop-test

desktop-test-arduino: tests/desktop.cpp $(HEADERS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -Os -std=gnu++17 -g -fno-rtti -falign-functions=4 -ffunction-sections -fdata-sections -fno-exceptions -I. tests/desktop.cpp -o build/desktop-test-arduino
	cp -n tests/canada.json build/canada.json

desktop-test-debug: tests/desktop.cpp $(HEADERS)
	@mkdir -p build
	NIX_HARDENING_ENABLE= $(CXX) $(CXXFLAGS) -O0 -std=gnu++17 -Wextra -Wpedantic -Wno-gnu-zero-variadic-macro-arguments -Wno-variadic-macros -Wno-vla-extension -ferror-limit=50 -I. tests/desktop.cpp -o build/desktop-test-debug
	cp -n tests/canada.json build/canada.json

xcode:
	mkdir -p build/xcode
	cd build/xcode && cmake -G Xcode \\
		-DCMAKE_C_COMPILER="$(shell xcrun -find cc 2>/dev/null || echo /usr/bin/clang)" \\
		-DCMAKE_CXX_COMPILER="$(shell xcrun -find c++ 2>/dev/null || echo /usr/bin/clang++)" \\
		../..

clean:
	rm -f desktop-test desktop-test-debug
	rm -rf build/
