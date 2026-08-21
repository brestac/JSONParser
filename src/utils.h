#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// include for ntohs
#ifdef ARDUINO_ARCH_ESP8266 // TODO: check for other platforms
#include <ESP8266WiFi.h>
#else
#include <arpa/inet.h>
#endif

#include "constants.h"
#include "macros.h"

template <typename T, size_t N> constexpr bool copy_bytes_be_to_h(T (&dst)[N], uint8_t *src, size_t src_size);
template <typename T, size_t N> constexpr bool copy_bytes_be_to_h(T dst, uint8_t (&src)[N]);
template <typename T, size_t N> constexpr bool copy_hex_be_to_h(T (&dst)[N], const char* src, size_t src_size);
bool get_byte_fromHexString(uint8_t &value, const char* src, size_t src_size);
template <typename T> bool get_unsigned_integral_fromHexString(T &value, const char* src, size_t src_size);
template <typename T> constexpr T be_to_h(T value);
unsigned long long now();

template<uint8_t N>
constexpr bool is_in_ranges(char c, const char (&ranges)[N][2]) {
  for (uint8_t i = 0; i < N; i++) {
    if (c >= ranges[i][0] && c <= ranges[i][1]) {
      return true;
    }
  }

  return false;
}

constexpr uint8_t _hex_to_dec(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  } else if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  } else if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  } else {
    return 0;
  }
}

template <typename T> constexpr T be_to_h(T value) {
  if constexpr (sizeof(T) == 4) {
    return ntohl(value);
  } else if constexpr (sizeof(T) == 2) {
    return ntohs(value);
  } else {
    return value;
  }
}

constexpr bool _is_hex_char(char c) {
  return is_in_ranges(c, JSON_HEX_CHARACTERS_RANGES);
}

bool get_byte_fromHexString(uint8_t &value, const char* src, size_t src_size) {
  if (src_size < 2) return false;

  if (_is_hex_char(src[0])) {
    uint8_t high_mid_byte = _hex_to_dec(src[0]);
    if (_is_hex_char(src[1])) {
      uint8_t low_mid_byte = _hex_to_dec(src[1]);
      value = (high_mid_byte << 4) | low_mid_byte;
      return true;
    }
  }

  return false;
}

template <typename T> bool get_unsigned_integral_fromHexString(T &value, const char* src, size_t src_size) {
  constexpr size_t target_length = sizeof(T);

  if (src_size < target_length * 2) {
    return false;
  }

  uint8_t bytes[target_length];
  for (size_t i = 0; i < target_length; i++) {
    if (!get_byte_fromHexString(bytes[i], src + i * 2, src_size - i * 2)) {
      return false;
    }
  }

  value = be_to_h(*(T *)bytes);

  return true;
}

template <typename T, size_t N> constexpr bool copy_hex_be_to_h(T (&dst)[N], const char* src, size_t src_size) {

  size_t dst_element_size = sizeof(T);
  size_t dst_elements_count = N;
  size_t dst_size = dst_element_size * dst_elements_count;

  if (dst_size == 0 || src_size == 0) {
    return false;
  }

  bool modified = false;

  size_t max_elements_count = std::min(dst_elements_count, src_size / dst_element_size);

  for (size_t i = 0; i < max_elements_count; i++) {
    T new_element_value = 0;
    if (get_unsigned_integral_fromHexString(new_element_value, src + i * dst_element_size * 2,
                                            src_size - i * dst_element_size * 2)) {
      if (dst[i] != new_element_value) {
        dst[i] = new_element_value;
        modified = true;
      }
    }
  }

  size_t dst_final_size = max_elements_count * dst_element_size;

  if (dst_final_size > src_size) {
    memset((uint8_t *)dst + dst_final_size, 0, dst_final_size - src_size);
    modified = true;
  }

  return modified;
}

void print_bitwise_mask([[maybe_unused]] size_t mask, size_t count) {

  DEBUG_PRINTF("mask: ");

  for (size_t i = 0; i < count; i++) {
    DEBUG_PRINTF("%d ", (mask & (1 << i)) != 0);
  }

  DEBUG_PRINTF("\n");
}

// static bool is_char_in_range(unsigned char c, uint_64_t mask) {
//   return (mask & (1ULL << c)) != 0ULL;
// }

unsigned long long now() {
  auto time = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(time.time_since_epoch()).count();
}

#if JSON_DEBUG_LEVEL > 0
void replace_endl(char *str, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (str[i] == '\n' || str[i] == '\r' || str[i] == '\t') {
      str[i] = ' ';
    }
  }
}

template <size_t N, size_t M> void replace_str(char (&input)[N], char (&oldChars)[M], char newChar) {
  for (size_t i = 0; i < N; i++) {
    for (size_t j = 0; j < M; j++) {
      if (input[i] == oldChars[j]) {
        input[i] = newChar;
      }
    }
  }
}
#endif

constexpr uint32_t hash32(const char* str, size_t len) {
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < len; ++i) {
    hash ^= static_cast<uint32_t>(str[i]);
    hash *= 16777619u;
  }
  return hash;
}

constexpr uint32_t hash32(std::string_view key) {
  return hash32(key.data(), key.length());
}

inline double multiplyByPowerOfTen(double value, int exponent) {
  if (exponent == 0) return value;

  double factor = 1.0;
  int abs_exponent = exponent < 0 ? -exponent : exponent;

  // Exponentiation rapide par carré pour optimiser la vitesse de calcul de la puissance
  double base = 10.0;
  while (abs_exponent > 0) {
      if (abs_exponent & 1) factor *= base;
      base *= base;
      abs_exponent >>= 1;
  }

  if (exponent < 0) {
        value /= factor;
  } else {
        value *= factor;
  }

  return value;
}
/*
// Table de puissances de 10 précalculée pour éviter l'appel coûteux à std::pow
static const double POW10[] = {
    1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8,  1e9,
    1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e22
};

inline double fast_parse_floating_json(const char* ptr, const char** end) {  
    if (ptr == nullptr) return 0.0;
    if (*ptr == '\0') return 0.0;

    // 1. Gestion du signe
    bool negative = false;
    if (*ptr == '-') {
        negative = true;
        ptr++;
    } else if (*ptr == '+') {
      ptr++;
    }

    uint64_t value = 0;
    int decimal_digits = -1;
    bool has_digits = false;

    // 2. Parser la partie entière et décimale en un seul grand entier
    while (*ptr != '\0') {
        if (*ptr >= '0' && *ptr <= '9') {
            value = value * 10 + (*ptr - '0');
            has_digits = true;
            if (decimal_digits >= 0) {
                decimal_digits++;
            }
            ptr++;
        } else if (*ptr == '.') {
            if (decimal_digits >= 0) break; // Deuxième point trouvé (erreur de syntaxe)
            decimal_digits = 0;
            ptr++;
        } else {
            break; // Caractère non numérique ou début de l'exposant 'e'/'E'
        }
    }

    if (!has_digits) {
        return 0.0;
    }

    // Ajustement de l'exposant de base lié à la virgule
    int exponent = (decimal_digits > 0) ? -decimal_digits : 0;

    // 3. Gestion de la notation scientifique JSON (e ou E)
    if (*ptr != '\0') {
        if (*ptr == 'e' || *ptr == 'E') {
          ptr++;

            bool exp_negative = false;
            //char sign = *ptr;
            if (*ptr != '\0') {
                if (*ptr == '-') { exp_negative = true; ptr++; }
                else if (*ptr == '+') { ptr++; }
            }

            int exp_value = 0;
            while (*ptr != '\0') {
                //char digit = *ptr;
                if (*ptr >= '0' && *ptr <= '9') {
                    exp_value = exp_value * 10 + (*ptr - '0');
                    ptr++;
                } else {
                    break;
                }
            }
            exponent += exp_negative ? -exp_value : exp_value;
        }
    }

    // 4. Conversion finale (Calcul à la volée pour économiser la Flash ROM)
    double result = value;

    if (exponent != 0) {
        // Calcul de la puissance de 10 de manière itérative ou via les fonctions de base
        // Sur microcontrôleur avec FPU matérielle, l'utilisation d'une boucle ou d'un multiplicateur
        // évite de charger des grosses bibliothèques d'arrondis parfaits.
        double factor = 1.0;
        int abs_exponent = exponent < 0 ? -exponent : exponent;

        // Exponentiation rapide par carré pour optimiser la vitesse de calcul de la puissance
        double base = 10.0;
        while (abs_exponent > 0) {
            if (abs_exponent & 1) factor *= base;
            base *= base;
            abs_exponent >>= 1;
        }

        if (exponent < 0) {
            result /= factor;
        } else {
            result *= factor;
        }
    }

    *end = ptr;
    return negative ? -result : result;
}
*/