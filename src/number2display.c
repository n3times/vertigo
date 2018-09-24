#include "common.h"

#define MAX_DISP_MANT 10
#define MAX_DISP_MANT_EXP 8
#define MAX_VALUE_EXP 99
#define MANT_SIZE 13

#define LOC_MANT_END_MANT 10
#define LOC_MANT_END_EXP 8

/** 10^n */
static int64_t pow10(int n) {
  int64_t ret = 1;
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

static int max_mant_size(ee_mode_t ee_mode) {
  return ee_mode == FLEX ? MAX_DISP_MANT : MAX_DISP_MANT_EXP;
}

static int loc_mant_end_for_exp(bool is_exp) {
  return is_exp ? MAX_DISP_MANT_EXP : MAX_DISP_MANT;
}

static int loc_dp(bool is_exp, int size_mant, int digits_before_point) {
  int loc_mant_end = loc_mant_end_for_exp(is_exp);
  return loc_mant_end - size_mant + digits_before_point;
}

static void display_number(bool is_neg, int64_t disp_mant, int size_mant, 
                           bool is_exp, int exp, int dp, bool overflow,
                           display_t *display) {
  memcpy(display->chars, "            ", 12);

  int loc_mant_end = loc_mant_end_for_exp(is_exp);
  int loc_mant_start = loc_mant_end - size_mant + 1;
  int loc_mant_sign = loc_mant_start - 1;

  display->chars[loc_mant_sign] = is_neg ? '-' : ' ';
  for (int i = loc_mant_end; i >= loc_mant_start; i--) {
    display->chars[i] = '0' + disp_mant%10;
    disp_mant /= 10;
  }
  if (is_exp) {
    int loc_exp = loc_mant_end + 1;
    display->chars[loc_exp++] = exp < 0 ? '-' : ' ';
    display->chars[loc_exp++] = '0' + ABS(exp)/10;
    display->chars[loc_exp] = '0' + ABS(exp)%10;
  }
  display->dp = dp;
  display->overflow = overflow;
}

static void display_zero(int fix, ee_mode_t ee_mode, display_t *display) {
  int size_mant = is_float(fix) ? 1 
                                : MIN(1 + fix, max_mant_size(ee_mode));
  bool is_neg = 0 < 0;
  int64_t disp_mant = 0;
  bool is_exp = ee_mode != FLEX;
  int exp = 0;
  int dp = loc_dp(is_exp, size_mant, 1);
  bool overflow = false;
  return display_number(is_neg, disp_mant, size_mant,
                        is_exp, exp, dp, overflow, display);
}

static void display_overflow(bool is_neg, int fix, display_t *display) {
  int size_mant = is_float(fix) ? MAX_DISP_MANT_EXP
                                : MIN(1 + fix, MAX_DISP_MANT_EXP);
  int64_t disp_mant = pow10(size_mant) - 1;
  bool is_exp = true;
  int exp = MAX_VALUE_EXP;
  int dp = loc_dp(true, size_mant, 1);
  bool overflow = true;
  return display_number(is_neg, disp_mant, size_mant,
                        is_exp, exp, dp, overflow, display);
}

static bool mant_extract_digits(int64_t *mant, int d) {
  int64_t rounded = *mant;
  rounded = (rounded + 5*pow10(MANT_SIZE - d - 1)) / pow10(MANT_SIZE - d);
  *mant = rounded;

  bool is_mant_overflow = rounded >= pow10(d);
  return is_mant_overflow;
}

static int64_t trim_zeroes(int64_t mant, int *d) {
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

void number2display(number_t n, int fix, ee_mode_t ee_mode, display_t *display) {
  bool is_neg = n.mant < 0;

  if (n.exp > MAX_VALUE_EXP) {
    return display_overflow(is_neg, fix, display);
  }
  if (n.mant == 0) {
    return display_zero(fix, ee_mode, display);
  }
  if (n.exp >= MAX_DISP_MANT && ee_mode == FLEX) {
    ee_mode = SCI;
  }

  int exp = n.exp;

  // Compute digits_before_point.
  int digits_before_point;
  switch(ee_mode) {
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

  // Compute max_disp_mant_size.
  int max_disp_mant_size = max_mant_size(ee_mode);
  if (is_fix(fix)) {
    max_disp_mant_size = MIN(max_disp_mant_size, digits_before_point + fix);
  }

  // Compute zeroes_after_point.
  int zeroes_after_point = 0;
  if (ee_mode == FLEX && exp < 0) {
    zeroes_after_point = MIN(-(exp + 1), max_disp_mant_size);
  }

  // Extract significant digits from mantissa.
  int digits_from_mant = max_disp_mant_size - zeroes_after_point;
  int64_t mant = ABS(n.mant);
  bool mant_overflow = mant_extract_digits(&mant, digits_from_mant);
  if (mant_overflow) {
    n.mant = is_neg ? -pow10(MANT_SIZE - 1) : pow10(MANT_SIZE - 1);
    n.exp += 1;
    return number2display(n, fix, ee_mode, display);
  }
  if (ee_mode == FLEX && mant == 0 && is_float(fix)) {
    return number2display(n, fix, SCI, display);
  }

  // Format.
  bool do_trim_zeroes = is_float(fix);
  int size_mant = digits_from_mant;
  if (do_trim_zeroes) {
    int trim = digits_from_mant - digits_before_point;
    mant = trim_zeroes(mant, &trim);
    size_mant -= trim;
  }
  size_mant += zeroes_after_point;
  bool prepend_zero = digits_before_point == 0 && size_mant < MAX_DISP_MANT;
  if (prepend_zero) {
    size_mant += 1;
    digits_before_point += 1;
  }
  bool is_disp_neg = is_neg && mant != 0;
  bool is_exp = ee_mode != FLEX;
  int dp = loc_dp(is_exp, size_mant, digits_before_point);
  bool overflow = false;
  return display_number(is_disp_neg, mant, size_mant,
                        ee_mode != FLEX, n.exp, dp, overflow,
                        display);
}
