#include "common.h"

#include "number2display.h"

static void test_number2display(void) {
  number_t n;
  while (1) {
    scanf(" %lld%d", &n.mant, &n.exp);
    if (getchar() == EOF) /* skip new line */ {
      break;
    }
    display_t display;
    number2display(n, -1, FLEX, &display);
    for (int i = 0; i < 12; i++) {
      printf("%c", display.chars[i]);
    }
    printf(".%02d", display.dp);
    if (display.overflow) {
      printf("?");
    }
    printf("\n");
  }
}

int main(void)
{
  test_number2display();
  return 0;
}
