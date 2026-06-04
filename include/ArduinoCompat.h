#pragma once

#include <chrono>
#include <stdlib.h>

void randomSeed(uint32_t seed) { srand(seed); }
// get a ramdom number between 0 and max
int random(int max) {
  return rand() % max;
}

unsigned long long micros() {
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}

void yield() {}