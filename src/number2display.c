#include "common.h"

#define MAX_DISP_MANT 10
#define MAX_DISP_MANT_EXP 8
#define MAX_VALUE_EXP 99
#define MANT_SIZE 13

/** 10^n */
static mant_t pow10(n) {
  mant_t ret = 1LL;
  for (int i = 0; i < n; i++) {
    ret *= 10;
  }
  return ret;
}

/** fix mode */
static bool is_fix(int fix) {
  return fix >= 0 && fix <= 8;
}

/** float mode */
static bool is_float(int fix) {
  return fix == -1 || fix == 9;
}

/** mantissa/number is 0 **/
static bool is_zero(mant_t mant) {
  return mant == 0;
}

static int max_mant_size_for_mode(mode_t mode) {
  return mode == FLEX ? MAX_DISP_MANT : MAX_DISP_MANT_EXP;
}

static void display_number(bool is_neg, mant_t mant, int mant_size, 
                           bool is_exp, int exp, int dp, bool overflow,
                           display_t *display) {
  int end_mant = is_exp ? 8 : 10;
  int start_mant = end_mant - mant_size + 1;
  int start_exp = -1;
  int end_exp = -1;
  if (is_exp) {
    start_exp = 9;
    end_exp = 11;
  }
  int pos_sign = start_mant - 1;
 
  for (int i = 0; i < 12; i++) {
    char c = ' ';
    if (i == pos_sign && is_neg) {
      c = '-';
    } else if (i >= start_mant && i <= end_mant) {
      int d = (mant / pow10(mant_size - i + start_mant - 1)) % 10;
      c = '0' + d;
    } else if (i >= start_exp && i <= end_exp) {
      if (i == 9 && exp < 0) {
        c = '-';
      } else if (i == 10) {
        c = '0' + (ABS(exp) / 10);
      } else if (i == 11) {
        c = '0' + (ABS(exp) % 10);
      }
    }
    display->chars[i] = c;
  } 
  display->dp = dp;
  display->overflow = overflow;
}

static void display_zero(int fix, mode_t mode, display_t *display) {
  int mant_size = 1;
  if (is_fix(fix)) {
    mant_size = MIN(1 + fix, max_mant_size_for_mode(mode));
  }
  int dp = mode == FLEX ? 10 : 8;
  dp -= mant_size - 1;
  return display_number(false, 0, mant_size, mode != FLEX, 0, dp, false,
                        display);
}

static void display_overflow(bool neg, int fix, display_t *display) {
  int mant_size = MAX_DISP_MANT_EXP;
  if (is_fix(fix)) {
    mant_size = MIN(1 + fix, mant_size);
  }
  mant_t mant = pow10(mant_size) - 1;
  int dp = 8;
  dp -= mant_size - 1;
  return display_number(neg, mant, mant_size, true, MAX_VALUE_EXP, dp, true,
                        display);
}

static bool round_mant(mant_t *mant, int d) {
  mant_t rounded = *mant;
  rounded = (rounded + 5*pow10(MANT_SIZE - d - 1)) / pow10(MANT_SIZE - d);
  *mant = rounded;

  bool is_mant_overflow = rounded >= pow10(d);
  return is_mant_overflow;
}

static mant_t trim_zeroes(mant_t mant, int *d) {
  int trimmed = 0;
  for (int i = 0; i < *d; i++) {
    if (mant % 10 == 0) {
      mant /= 10;
      trimmed += 1;
    } else {
      break;
    }
  }
  *d = trimmed;
  return mant;
}

void number2display(number_t n, int fix, mode_t mode, display_t *display) {
  mant_t mant = n.mant;
  mant_t abs_mant = ABS(n.mant);
  int exp = n.exp;

  if (exp > MAX_VALUE_EXP) {
    return display_overflow(mant < 0, fix, display);
  }
  if (is_zero(n.mant)) {
    return display_zero(fix, mode, display);
  }
  if (exp >= MAX_DISP_MANT && mode == FLEX) {
    mode = SCI;
  }

  // Compute digits_before_point.
  int digits_before_point;
  switch(mode) {
  case FLEX:
    digits_before_point = MAX(0, exp + 1);
    break;
  case SCI:
    digits_before_point = 1;
    break;
  case ENG:
    digits_before_point = 1 +  (3 + exp%3) % 3;
    exp -= (3 + exp%3) % 3;
    break;
  }

  // Compute mant_size.
  int mant_size = max_mant_size_for_mode(mode);
  if (is_fix(fix)) {
    mant_size = MIN(mant_size, digits_before_point + fix);
  }

  // Compute zeroes_after_point.
  int zeroes_after_point = 0;
  if (mode == FLEX && exp < 0) {
    zeroes_after_point = MIN(-(exp + 1), mant_size);
  }

  // Compute digits_from_mant.        
  int digits_from_mant = mant_size - zeroes_after_point;

  // Round.
  bool mant_overflow = round_mant(&abs_mant, digits_from_mant);
  if (mant_overflow) {
    n.mant = mant < 0 ? -1000000000000LL
                      :  1000000000000LL;
    n.exp += 1;
    return number2display(n, fix, mode, display);
  }
  if (mode == FLEX && is_zero(abs_mant) && is_float(fix)) {
    n.exp = exp;
    return number2display(n, fix, SCI, display);
  }

  // Format.
  bool do_trim_zeroes = is_float(fix);
  mant_t disp = abs_mant;
  int disp_size = digits_from_mant;
  if (do_trim_zeroes) {
    int trim = digits_from_mant - digits_before_point;
    disp = trim_zeroes(disp, &trim);
    disp_size -= trim;
  }
  disp_size += zeroes_after_point;
  bool prepend_zero = digits_before_point == 0 && disp_size < MAX_DISP_MANT;
  if (prepend_zero) {
    disp_size += 1;
    digits_before_point += 1;
  }
  bool is_neg = mant < 0 && !is_zero(disp);
  int dp = mode == FLEX ? 10 : 8;
  dp -= disp_size - digits_before_point;
  return display_number(is_neg, disp, disp_size, 
                        mode != FLEX, exp, dp, false,
                        display);
}
