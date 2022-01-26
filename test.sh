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
assert 0 "main() {0;}"
assert 42 "main() {42;}"

# return calculation result of add and sub
assert 21 "main() {5+20-4;}"
assert 0 "main() {3+15-18;}"
assert 255 "main() {2+189-2+66;}"

# return calculation result of add and sub (with spaces)
assert 41 "main() {12 + 34 - 5;}"
assert 81 "main() {  52 + 34 - 5 ;}"

# return calculation result of expressions including "+-*/()"
assert 47 "main() {5+6*7;}"
assert 15 "main() {5*(9-6);}"
assert 4 "main() {(3+5)/2;}"

# include unary
assert 1 "main() {-5+6;}"
assert 4 "main() {(3+5)/+2;}"

# relationals
assert 1 "main() {0==0;}"
assert 0 "main() {1==0;}"
assert 1 "main() {-12<12;}"
assert 1 "main() {5+12-12<6+1;}"
assert 0 "main() {12<12;}"
assert 1 "main() {-12<=12;}"
assert 1 "main() {0<=0;}"
assert 0 "main() {9<=8;}"
assert 1 "main() {0>=0;}"
assert 1 "main() {1>=0;}"
assert 0 "main() {0>=1;}"

# handling one letter varialbes
assert 3 "main() {a=3;}"
assert 14 "main() {a=3; b=5*6-8; a+b/2;}"
assert 224 "main() {a=112; a+a;}"

assert 15 "main() {a=1; b=2; a_b=a+b; ab00=12; a_b+ab00;}"
assert 1 "main() {zz=1; z=2; zz;}"

# return statement
assert 3 "main() {return 3; a=10; a;}"
assert 10 "main() {abc=2; return abc*5;}"

# if statement
assert 0 "main() {if(0) return 1; else return 0;}"
assert 1 "main() {if(1) return 1; else return 0;}"
assert 1 "main() {if(-1) return 1; else return 0;}"

assert 0 "main() {a=0; if(0) a=1; return a;}"
assert 1 "main() {a=0; if(1) a=1; return a;}"

assert 1 "main() {a=-1; if(0) a=0; else if (1) a=1; else a=2; return a;}"
assert 2 "main() {a=-1; if(0) a=0; else if (0) a=1; else a=2; return a;}"

# while statement
assert 0 "main() {a=0; while(0) a=a+1; return a;}"
assert 11 "main() {a=0; while(a<=10) a=a+1; return a;}"

# for statement
assert 128 "main() {a=1; for(i=0; i<7; i=i+1) a=a*2; return a;}"  # 2^7
assert 5 "main() {a=0; for(i=0; i<10; i=i+2) a=a+1; return a;}"
assert 1 "main() {a=1; for(i=0; i<0; i=i+1) a=a+1; return a;}"

# block
assert 10 "main() {ret=0; a=10; while(a) {ret=ret+1; a=a-1;} return ret;}"
assert 2 "main() {a=0; b=0; if(1) {a=1; b=1;} else {a=1;} return a+b;}"
assert 3 "main() {a=1; b=2; return a+b;}"
assert 1 "main() { if(1) { if(1){return 1;} } } return 0;"

# function
assert 5 "main() {return my_sum(2, 3);} my_sum(a, b) {return a+b;}"
assert 1 "main() {return fib(1);} fib(n) {if(n<=1) return n; else return fib(n-1) + fib(n-2);}"
assert 1 "main() {return fib(2);} fib(n) {if(n<=1) return n; else return fib(n-1) + fib(n-2);}"
assert 2 "main() {return fib(3);} fib(n) {if(n<=1) return n; else return fib(n-1) + fib(n-2);}"
assert 3 "main() {return fib(4);} fib(n) {if(n<=1) return n; else return fib(n-1) + fib(n-2);}"
assert 5 "main() {return fib(5);} fib(n) {if(n<=1) return n; else return fib(n-1) + fib(n-2);}"
assert 8 "main() {return fib(6);} fib(n) {if(n<=1) return n; else return fib(n-1) + fib(n-2);}"
assert 13 "main() {return fib(7);} fib(n) {if(n<=1) return n; else return fib(n-1) + fib(n-2);}"
assert 120 "main() {return combination(10, 3);} combination(n, r) {if(n==r) return 1; if(r==0) return 1; return combination(n-1, r-1) + combination(n-1, r);}"

# mod (%)
assert 0 "main() {10%5;}"
assert 2 "main() {10%4;}"
assert 4 "main() {3+5%2;}"
assert 0 "main() {return is_prime(10);} is_prime(n) {if(n==2) return 1; for(i=2; i*i<=n; i=i+1) {if(n%i==0) return 0;} return 1;}"
assert 1 "main() {return is_prime(7);} is_prime(n) {if(n==2) return 1; for(i=2; i*i<=n; i=i+1) {if(n%i==0) return 0;} return 1;}"

echo OK
