#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef struct {
  int64_t mant;
  int exp;
} number_t;

typedef struct {
  char chars[12];
  int dp;
  bool overflow;
} display_t;

#endif  // TYPES_H
