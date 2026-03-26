all: main

CXX = clang++
override CXXFLAGS += -g -Wall -Werror

# Exclude the arduino/ and tests/ directories from the normal desktop build
SRCS = $(shell find . -name '.ccls-cache' -type d -prune \
              -o -name 'arduino' -type d -prune \
              -o -name 'tests' -type d -prune \
              -o -type f -name '*.cpp' -print \
       | sed -e 's/ /\\ /g')
HEADERS = $(shell find . -name '.ccls-cache' -type d -prune \
                -o -name 'arduino' -type d -prune \
                -o -type f -name '*.h' -print)

main: $(SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o "$@"

main-debug: $(SRCS) $(HEADERS)
	NIX_HARDENING_ENABLE= $(CXX) $(CXXFLAGS) -O0 $(SRCS) -o "$@"

# ----------------------------------------------------------------
# Arduino stub build — exercises the #ifdef ARDUINO code paths
# on desktop using minimal Stream stubs in arduino/Stream.h.
# Put -I./arduino FIRST so our stub wins over the root Stream.h.
# Usage: make arduino-test && ./arduino-test
# ----------------------------------------------------------------
arduino-test: tests/arduino.cpp $(HEADERS) arduino/Stream.h
	$(CXX) $(CXXFLAGS) -DARDUINO -I./arduino -I. \
		tests/arduino.cpp -o arduino-test

clean:
	rm -f main main-debug arduino-test
