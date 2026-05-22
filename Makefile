all: desktop-test

CXX = clang++
override CXXFLAGS += -g -Wall -Werror

SRC_HEADERS   = $(wildcard src/*.h)
TEST_HEADERS  = $(wildcard tests/*.h)
HEADERS       = $(SRC_HEADERS) $(TEST_HEADERS)

desktop-test: tests/desktop.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -I. tests/desktop.cpp -o build/desktop-test
	cp tests/canada.json build/canada.json

desktop-test-debug: tests/desktop.cpp $(HEADERS)
	NIX_HARDENING_ENABLE= $(CXX) $(CXXFLAGS) -O0 -I. tests/desktop.cpp -o build/desktop-test-debug

xcode:
	mkdir -p build/xcode
	cd build/xcode && cmake -G Xcode \
		-DCMAKE_C_COMPILER="$(shell xcrun -find cc 2>/dev/null || echo /usr/bin/clang)" \
		-DCMAKE_CXX_COMPILER="$(shell xcrun -find c++ 2>/dev/null || echo /usr/bin/clang++)" \
		../..

clean:
	rm -f build/desktop-test build/desktop-test-debug
