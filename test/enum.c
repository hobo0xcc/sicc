#include <stdio.h>

enum _hello {
  HELLO,
  HALLO = 200,
  KONNICHIWA,
};

int main(void) {
  int hello = HELLO;
  int hallo = HALLO;
  int konnichiwa = KONNICHIWA;
  printf("%d\n%d\n%d\n", hello, hallo, konnichiwa);
  return KONNICHIWA;
}
