#include <stdio.h>
#include <stdlib.h>

struct ptr {
  int a;
};

int main() {
  struct {
    char *a;
    char *b;
    int c;
  } str;
  str.a = "Hello";
  str.b = "world";
  str.c = 42;
  printf("%s\n", str.a);
  printf("%s\n", str.b);
  printf("%d\n", str.c);
  printf("sizeof 'str': %ld\n", sizeof str);

  struct ptr *ptr_struct = malloc(sizeof(struct ptr));
  printf("sizeof 'ptr_struct': %ld\n", sizeof ptr_struct);
  ptr_struct->a = 10;
  printf("ptr_struct->a == %d\n", ptr_struct->a);
  ptr_struct->a++;
  printf("added: ptr_struct->a == %d\n", ptr_struct->a);
  return 0;
}
