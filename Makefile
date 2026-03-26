all: desktop-test

CXX = clang++
override CXXFLAGS += -g -Wall -Werror

HEADERS = $(shell find . -name '.ccls-cache' -type d -prune \
                -o -name 'arduino' -type d -prune \
                -o -type f -name '*.h' -print)

# ----------------------------------------------------------------
# Desktop test build -- exercises the PointerCursor / desktop code paths.
# Usage: make desktop-test && ./desktop-test
# ----------------------------------------------------------------
desktop-test: tests/desktop.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -I. tests/desktop.cpp -o desktop-test

desktop-test-debug: tests/desktop.cpp $(HEADERS)
	NIX_HARDENING_ENABLE= $(CXX) $(CXXFLAGS) -O0 -I. tests/desktop.cpp -o desktop-test-debug

# ----------------------------------------------------------------
# Arduino stub build -- exercises the #ifdef ARDUINO code paths
# on desktop using minimal Stream stubs in arduino/Stream.h.
# Put -I./arduino FIRST so our stub wins over the root Stream.h.
# Usage: make arduino-test && ./arduino-test
# ----------------------------------------------------------------
arduino-test: tests/arduino.cpp $(HEADERS) arduino/Stream.h
	$(CXX) $(CXXFLAGS) -DARDUINO -I./arduino -I. \
		tests/arduino.cpp -o arduino-test

clean:
	rm -f desktop-test desktop-test-debug arduino-test
