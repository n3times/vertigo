#ifndef NUMBER_H
#define NUMBER_H

typedef long long mant_t;

typedef struct {
  mant_t mant;
  int exp;
} number_t;

#endif // NUMBER_H
