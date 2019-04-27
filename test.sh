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
    echo "$expect expected but got $ret"
    exit 1
  fi
}

test 0 'main() { return 0; }'
test 15 'main() { a = 10; b = 5; return a + b; }'
test 4 'one() { return 1; } two() { return 2; } main() { return one() + two() + one(); }'
