#ifndef COMMON_H
#define COMMON_H

#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

#include "types.h"

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define ABS(x) ((x) < 0 ? -(x) : (x))

typedef enum {FLEX, SCI, ENG} ee_mode_t;

#endif // COMMON_H
