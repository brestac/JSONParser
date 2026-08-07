all: desktop-test

CXX = clang++
override CXXFLAGS += -g -Wall -Werror -Werror=unused-variable -Wno-format-security -DJSON_DEBUG_LEVEL_NONE=0 -DJSON_DEBUG_LEVEL_INFO=1 -DJSON_DEBUG_LEVEL_WARNING=2 -DJSON_DEBUG_LEVEL_ERROR=3

SRC_HEADERS   = $(wildcard src/*.h)
TEST_HEADERS  = $(wildcard tests/*.h)
HEADERS       = $(SRC_HEADERS) $(TEST_HEADERS)
RESOURCES 		= $(wildcard examples/JSONParserTest/*.*json)

desktop-test: tests/desktop.cpp $(HEADERS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -O2 -std=gnu++17 -I. tests/desktop.cpp -o build/desktop-test
	cp -n $(RESOURCES) build/

desktop-test-debug: tests/desktop.cpp $(HEADERS)
	@mkdir -p build
	NIX_HARDENING_ENABLE= $(CXX) $(CXXFLAGS) -O0 -std=gnu++23 -Wextra -Wpedantic -Wno-gnu-zero-variadic-macro-arguments -Wno-variadic-macros -Wno-vla-extension -ferror-limit=50 -I. tests/desktop.cpp -o build/desktop-test-debug
	cp -n $(RESOURCES) build/

desktop-test-arduino: tests/desktop.cpp $(HEADERS)
	@mkdir -p build
	g++ $(CXXFLAGS) -Wno-unknown-pragmas -Os -std=gnu++17 -g -fno-rtti -falign-functions=4 -ffunction-sections -fdata-sections -fno-exceptions -I. tests/desktop.cpp -o build/desktop-test-arduino
	cp -n $(RESOURCES) build/

desktop-test-faster: tests/desktop.cpp $(HEADERS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -O3 -flto -std=gnu++23 -I. tests/desktop.cpp -o build/desktop-test-faster
	cp -n $(RESOURCES) build/

xcode:
	mkdir -p build/xcode
	cd build/xcode && cmake -G Xcode \
		-DCMAKE_CXX_COMPILER="$(shell xcrun -find c++ 2>/dev/null || echo /usr/bin/clang++)" \
		../..

clean:
	rm -f desktop-test desktop-test-debug
	rm -rf build/
