#!/bin/bash

time_gcc() {
    gcc -o tst "$1"
    echo "gcc: $1"
    time ./tst
}

time_sicc() {
    ./sicc "$1" > tst.s
    as -o tst.o tst.s
    ld -lSystem -w -e _main -o tst tst.o
    echo "sicc: $1"
    time ./tst
}

time_gcc "$1"
time_sicc "$1"
