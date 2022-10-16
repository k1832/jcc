#!/bin/bash
input="$1"
make clean
make
./jcc "$input" > tmp.s
cc -o tmp tmp.s
./tmp
echo $?
