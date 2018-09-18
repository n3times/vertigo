MAX_SIZE_MANT = 2
MAX_SIZE_MANT_EXP = 3
MAX_VALUE_EXP = 3
MIN_VALUE_EXP = -3

FIX_FLOAT = -1

# Rounds m to exactly d digits, padding with 0's if necessary.
# Returns rounded_m, overflow.
def round_pad(m, d):
  assert len(m) > 0 and d >= 0
  if len(m) <= d:
    return m.ljust(d, '0'), False
  is_inc = m[d] >= '5'
  m = m[:d]
  if not is_inc:
    return m, False
  if d == 0:
    return '', True
  inc_m = str(int(m) + 1)
  if len(inc_m) > d:
    return m[1:], True
  return inc_m, False

# Trims up to d 0's from m.
def trim(m, d):
  n = 0
  for i in range(d):
    if m[len(m) - i - 1] != '0':
      break
    n += 1
  return m[:len(m)-n]

def is_fix(f):
  return not is_float(f)

def is_float(f):
  return f == FIX_FLOAT

def display_overflow(f):
  n = MAX_SIZE_MANT_EXP
  if is_fix(f):
    n = min(1 + f, n)
  m = ''.ljust(n, '9')
  e = str(MAX_VALUE_EXP)
  return m, e, 1, True

def display_underflow(f):
  m = '1'
  e = str(MIN_VALUE_EXP)
  return m, e, 1, True

# m: a string of > 0 digits
# e: an integer
# f: -1 or 0, 1, 2, ...
# sci and eng are boolean
def display(m, e, f, sci, eng):
  assert len(m) > 0 and m[0] != '0'
  assert not(eng and MAX_SIZE_MANT_EXP < 3)
  # d_m: number of digits of m to consider
  # d_int: digits before '.'
  # d_z: zeroes after '.'
  # size:
  # Compute d_m, d_int, d_z.
  exp = sci or e >= MAX_SIZE_MANT or (is_float(f) and -e > MAX_SIZE_MANT + 1)
  if eng or exp:
    d_int = 1
    if eng:
      d_int = d_int + e%3
      e = e - e%3
    d_z = 0
    size = MAX_SIZE_MANT_EXP if is_float(f) else min(MAX_SIZE_MANT_EXP, d_int + f)
  else:
    d_int = max(0, e + 1)
    size = MAX_SIZE_MANT if is_float(f) else min(MAX_SIZE_MANT, d_int + f)
    d_z = 0 if e >= 0 else min(-(e + 1), size)
  d_m = size - d_z
  # Round.
  old_m = m
  m, carry = round_pad(m, d_m)
  overflow = e > MAX_VALUE_EXP or (e == MAX_VALUE_EXP and carry)
  if overflow:
    return display_overflow(f)
  if carry:
    return display('1', e + 1, f, sci, eng)
  if e < MIN_VALUE_EXP:
    return display_underflow(f)
  if is_float(f):
    m = trim(m, d_m - d_int)
  if len(m) == 0 and is_float(f):
    return display(old_m, e, f, True, eng)
  e_str = str(abs(e)).rjust(2, '0') if (exp or eng) else ''
  if e_str != '' and e < 0:
    e_str = '-' + e_str
  if d_z > 0:
    m = ''.zfill(d_z) + m
  if d_int == 0 and len(m) < MAX_SIZE_MANT:
    m = '0' + m
    d_int += 1
  return m, e_str, d_int, False

def pretty_display(m, e, f = -1, sci = False, eng = False):
  m, e, d, o = display(m, e, f, sci, eng)
  #assert e != '' or len(m) + len(e) <= MAX_SIZE__M
  #assert e == '' or len(m) <= MAX_SIZE_MANT_EXP
  oo = ''
  if o:
    oo = '?'
  print(m[:d], '.', m[d:], ' ', e, oo, sep='')

def test(sci, eng):
  print("\nCase 1:")
  for e in range(-12, 13):
    pretty_display('1', e, -1, sci, eng)

  print("\nCase 9:")
  for e in range(-12, 13):
    pretty_display('9', e, -1, sci, eng)

  print("\nCase fix 1:")
  for e in range(-12, 13):
    pretty_display('115', e, 1, sci, eng)

  print("\nCase fix 3:")
  for e in range(-12, 13):
    pretty_display('115', e, 3, sci, eng)

print("\n\n\n\nSCI")
test(True, False)
print("\n\n\n\nENG")
test(True, True)
print("\n\n\n\nDEF")
test(False, False)

pretty_display('112', 2, 2)
pretty_display('99999999', -99)
pretty_display('4', -4, 3)
