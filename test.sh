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
assert 0 "0;"
assert 42 "42;"

# return calculation result of add and sub
assert 21 "5+20-4;"
assert 0 "3+15-18;"
assert 255 "2+189-2+66;"

# return calculation result of add and sub (with spaces)
assert 41 "12 + 34 - 5;"
assert 81 "  52 + 34 - 5 ;"

# return calculation result of expressions including "+-*/()"
assert 47 "5+6*7;"
assert 15 "5*(9-6);"
assert 4 "(3+5)/2;"

# include unary
assert 1 "-5+6;"
assert 4 "(3+5)/+2;"

# relationals
assert 1 "0==0;"
assert 0 "1==0;"
assert 1 "-12<12;"
assert 1 "5+12-12<6+1;"
assert 0 "12<12;"
assert 1 "-12<=12;"
assert 1 "0<=0;"
assert 0 "9<=8;"
assert 1 "0>=0;"
assert 1 "1>=0;"
assert 0 "0>=1;"

# handling one letter varialbes
assert 3 "a=3;"
assert 14 "a=3; b=5*6-8; a+b/2;"
assert 224 "a=112; a+a;"

assert 15 "a=1; b=2; a_b=a+b; ab00=12; a_b+ab00;"
assert 1 "zz=1; z=2; zz;"

# return statement
assert 3 "return 3; a=10; a;"
assert 10 "abc=2; return abc*5;"

# if statement
assert 0 "if(0) return 1; else return 0;"
assert 1 "if(1) return 1; else return 0;"
assert 1 "if(-1) return 1; else return 0;"

assert 0 "a=0; if(0) a=1; return a;"
assert 1 "a=0; if(1) a=1; return a;"

assert 1 "a=-1; if(0) a=0; else if (1) a=1; else a=2; return a;"
assert 2 "a=-1; if(0) a=0; else if (0) a=1; else a=2; return a;"

# while statement
assert 0 "a=0; while(0) a=a+1; return a;"
assert 11 "a=0; while(a<=10) a=a+1; return a;"

echo OK
