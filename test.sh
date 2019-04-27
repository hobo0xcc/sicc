#!/bin/bash

test () {
  expect="$1"
  arg="$2"

  ./sicc "$arg" > tst.s
  if [ "$(uname)" == 'Darwin' ]; then
    as -o tst.o tst.s
    ld -lSystem -w -e main -o tst tst.o
  else
    gcc -static -o tst tst.s
  fi

  ./tst
  ret="$?"
  if [ "$expect" == "$ret" ]; then
    echo "$arg -> $ret"
  else
    echo "$expect expected but got $ret"
    exit 1
  fi
}

test 100 'return 100;'
test 20 'return 20;'
test 6 'return 1 + 2 + 3;'
test 10 'return 10 - 5 + 5 - 2 + 5 - 3;'
test 2 'return 10 - 3 - (2 + 3);'
test 8 'return 2 * 2 * 2;'
test 5 'return 10 / 2;'
test 5 'return (2 * 3 + 5 * 2) / 3;'
test 10 'a = 10; return a;'
test 8 'a = 12; b = 5; c = 9; return a + b - c;'
test 2 'a = 2; b = 4; return a;'
test 21 'foo = 2 * 3 + 5; bar = 9 + 2 + 4; return foo + bar - 5;'
test 0 'a = 1; if (a) return 0; return 1;'
test 42 'foo = 42; if (foo) return foo; return 0;'
