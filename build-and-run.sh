#!/bin/bash
input="$1"
./jcc "$input" > tmp.s
cc -o tmp tmp.s
./tmp
echo $?
