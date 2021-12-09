#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./jcc "$input" > tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

# return the inputted number
assert 0 0
assert 42 42

# return calculation result of add and sub
assert 21 "5+20-4"
assert 0 "3+15-18"
assert 255 "2+189-2+66"

# return calculation result of add and sub (with spaces)
assert 41 "12 + 34 - 5"
assert 28 "12 - 34 + 50"
assert 81 "  52 + 34 - 5 "

# return calculation result of expressions including "+-*/()"
assert 47 '5+6*7'
assert 15 '5*(9-6)'
assert 4 '(3+5)/2'

# include unary
assert 1 '-5+6'
assert 15 '+5*(9-6)'
assert 4 '(3+5)/+2'

# relationals
assert 1 "0==0"
assert 0 "1==0"
assert 1 "-12<12"
assert 0 "12<12"
assert 1 "-12<=12"
assert 1 "0<=0"
assert 0 "9<=8"
assert 1 "0>=0"
assert 1 "1>=0"
assert 0 "0>=1"

echo OK
