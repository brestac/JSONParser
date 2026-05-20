all: desktop-test

CXX = clang++
override CXXFLAGS += -g -Wall -Werror

HEADERS = $(shell find . -name '.ccls-cache' -type d -prune \
                -o -type f -name '*.h' -print)

desktop-test: tests/desktop.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -I. tests/desktop.cpp -o desktop-test

desktop-test-debug: tests/desktop.cpp $(HEADERS)
	NIX_HARDENING_ENABLE= $(CXX) $(CXXFLAGS) -O0 -I. tests/desktop.cpp -o desktop-test-debug

clean:
	rm -f desktop-test desktop-test-debug
