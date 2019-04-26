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

test 100 '100'
test 20 '20'
test 6 '1 + 2 + 3'
test 10 '10 - 5 + 5 - 2 + 5 - 3'
test 2 '10 - 3 - (2 + 3)'
test 8 '2 * 2 * 2'
test 5 '10 / 2'
