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
test 200 '100 120 200'
