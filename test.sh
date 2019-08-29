#!/bin/bash

test () {
  expect="$1"
  arg="$2"

  ./sicc "$arg" > tst.s
  if [ "$(uname)" == 'Darwin' ]; then
    as -o tst.o tst.s
    ld -lSystem -w -e _main -o tst tst.o
  else
    gcc -static -o tst tst.s
  fi

  ./tst
  ret="$?"
  if [ "$expect" == "$ret" ]; then
    echo "$arg -> $ret"
  else
    echo "$expect expected but got $ret: $arg"
    exit 1
  fi
}

test 0 'test/hello.c'
test 0 'test/fib.c'
test 0 'test/while.c'
test 10 'test/scope.c'
test 19 'test/macro.c'
test 0 'test/sizeof.c'
test 0 'test/strings.c'
test 0 'test/operator.c'
test 0 'test/array.c'
test 0 'test/global_val.c'
test 30 'test/var_def.c'
test 42 'test/for.c'
test 0 'test/cond.c'
test 0 'test/ptr.c'
test 0 'test/struct.c'
test 0 'test/typedef.c'
test 0 'test/goto.c'
test 0 'test/while2.c'
test 29 'test/switch.c'
test 0 'test/for2.c'
test 201 'test/enum.c'
test 0 'test/static.c'
# test 0 'test/test.c'
# test 0 'int main() { return 0; }'
# test 15 'int main() { int a = 10; int b = 5; return a + b; }'
# test 4 'int one() { return 1; } int two() { return 2; } int main() { return one() + two() + one(); }'
# test 5 'int plus(int x, int y) { return x + y; } int main() { return plus(2, 3); }'
# test 1 'int main() { int i = 1; if (i) return 1; return i; }'
# test 2 'int main() { int *a = malloc(4); *a = 2; return *a; }'
# test 0 'int main() { char *str = malloc(6); *str = 65; *(str + 1) = 66; *(str + 2) = 67; *(str + 3) = 68; *(str + 4) = 69; *(str + 5) = 0; printf(str); return 0; }'
# test 0 'char *hello() { char *s = malloc(6); *(s) = 72; *(s+1) = 101; *(s+2) = 108; *(s+3) = 108; *(s+4) = 111; *(s+5) = 0; return s; } int main() { char *str = hello(); printf(str); return 0; }'
# test 0 'int main() { printf("Hello, world!\n"); return 0; }'
# test 0 'int main() { printf("%d\n", 10); return 0; }'
# test 36 'plus(a, b, c, d, e, f, g, h) { return a + b + c + d + e + f + g + h; } main() { return plus(1, 2, 3, 4, 5, 6, 7, 8); }'
# test 1 'main(argc) { return argc; }'
# test 0 'main() { a = 2; b = 5; if (a < b) return 0; else return 1; }'
