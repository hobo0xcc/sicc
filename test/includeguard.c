#ifndef INC_GUARD_C
#define INC_GUARD_C
#include <stdio.h>

#ifndef INC_GUARD_C
int failed1() {
  return 1;
}

#endif

int main(void) {
  printf("guarded!\n");
  return 0;
}

#endif

#ifndef INC_GUARD_C
int failed2() {
  return 2;
}

#endif
