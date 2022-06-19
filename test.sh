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

# test to exit with error
# # Deplicated param
# assert 0 "int my_sum(int a, int a) {return a + a;} int main() {return my_sum(10, 10);}"
# # Redeclaration variable
# assert 0 "int my_sum(int a, int b) {int a; return a + b;} int main() {return my_sum(10, 10);}"
# assert 0 "int my_sum(int a, int b) {int c; int c; return a + b;} int main() {return my_sum(10, 10);}"
# # No parameter after ","
# assert 5 "int my_sum(int a, int b, ) {return a+b;} int main() {return my_sum(2, 3);}"
# # Left-hand side is not variable
# assert 5 "int my_sum(int a, int b) {5=a; return a + b;} int main() {return my_sum(2, 3);}"

# return the inputted number
assert 0 "int main() {0;}"
assert 42 "int main() {42;}"

# return calculation result of add and sub
assert 21 "int main() {5+20-4;}"
assert 0 "int main() {3+15-18;}"
assert 255 "int main() {2+189-2+66;}"

# return calculation result of add and sub (with spaces)
assert 41 "int main() {12 + 34 - 5;}"
assert 81 "int main() {  52 + 34 - 5 ;}"

# return calculation result of expressions including "+-*/()"
assert 47 "int main() {5+6*7;}"
assert 15 "int main() {5*(9-6);}"
assert 4 "int main() {(3+5)/2;}"

# include unary
assert 1 "int main() {-5+6;}"
assert 4 "int main() {(3+5)/+2;}"

# relationals
assert 1 "int main() {0==0;}"
assert 0 "int main() {1==0;}"
assert 1 "int main() {-12<12;}"
assert 1 "int main() {5+12-12<6+1;}"
assert 0 "int main() {12<12;}"
assert 1 "int main() {-12<=12;}"
assert 1 "int main() {0<=0;}"
assert 0 "int main() {9<=8;}"
assert 1 "int main() {0>=0;}"
assert 1 "int main() {1>=0;}"
assert 0 "int main() {0>=1;}"

# handling one letter varialbes
assert 3 "int main() {int a; a=3;}"
assert 14 "int main() {int a; int b; a=3; b=5*6-8; a+b/2;}"
assert 224 "int main() {int a; a=112; a+a;}"

assert 15 "int main() {int a; int b; int a_b; int ab00; a=1; b=2; a_b=a+b; ab00=12; a_b+ab00;}"
assert 1 "int main() {int zz; int z; zz=1; z=2; zz;}"

# return statement
assert 3 "int main() {return 3; int a; a=10; a;}"
assert 10 "int main() {int abc; abc=2; return abc*5;}"

# if statement
assert 0 "int main() {if(0) return 1; else return 0;}"
assert 1 "int main() {if(1) return 1; else return 0;}"
assert 1 "int main() {if(-1) return 1; else return 0;}"

assert 0 "int main() {int a; a=0; if(0) a=1; return a;}"
assert 1 "int main() {int a; a=0; if(1) a=1; return a;}"

assert 1 "int main() {int a; a=-1; if(0) a=0; else if (1) a=1; else a=2; return a;}"
assert 2 "int main() {int a; a=-1; if(0) a=0; else if (0) a=1; else a=2; return a;}"

# while statement
assert 0 "int main() {int a; a=0; while(0) a=a+1; return a;}"
assert 11 "int main() {int a; a=0; while(a<=10) a=a+1; return a;}"

# for statement
assert 128 "int main() {int i; int a; a=1; for(i=0; i<7; i=i+1) a=a*2; return a;}"  # 2^7
assert 5 "int main() {int i; int a; a=0; for(i=0; i<10; i=i+2) a=a+1; return a;}"
assert 1 "int main() {int i; int a; a=1; for(i=0; i<0; i=i+1) a=a+1; return a;}"
assert 10 "int main() {for(;;) return 10; return 100;}"

# block
assert 10 "int main() {int ret; int a; ret=0; a=10; while(a) {ret=ret+1; a=a-1;} return ret;}"
assert 2 "int main() {int a; int b; a=0; b=0; if(1) {a=1; b=1;} else {a=1;} return a+b;}"
assert 3 "int main() {int a; int b; a=1; b=2; return a+b;}"
assert 1 "int main() { if(1) { if(1){return 1;} } return 0;}"

# function
assert 5 "int my_sum(int a, int b) {return a+b;} int main() {return my_sum(2, 3);}"
assert 1 "int fib(int n) {if(n<=1) return n; else return fib(n-1) + fib(n-2);} int main() {return fib(1);}"
assert 1 "int fib(int n) {if(n<=1) return n; else return fib(n-1) + fib(n-2);} int main() {return fib(2);}"
assert 2 "int fib(int n) {if(n<=1) return n; else return fib(n-1) + fib(n-2);} int main() {return fib(3);}"
assert 3 "int fib(int n) {if(n<=1) return n; else return fib(n-1) + fib(n-2);} int main() {return fib(4);}"
assert 5 "int fib(int n) {if(n<=1) return n; else return fib(n-1) + fib(n-2);} int main() {return fib(5);}"
assert 8 "int fib(int n) {if(n<=1) return n; else return fib(n-1) + fib(n-2);} int main() {return fib(6);}"
assert 13 "int fib(int n) {if(n<=1) return n; else return fib(n-1) + fib(n-2);} int main() {return fib(7);}"
assert 120 "int combination(int n, int r) {if(n==r) return 1; if(r==0) return 1; return combination(n-1, r-1) + combination(n-1, r);} int main() {return combination(10, 3);} "

assert 21 "int ten_sum(int a, int b, int c, int d, int e, int f) {return a+b+c+d+e+f;} int main() {return ten_sum(1,2,3,4,5,6);}"
assert 15 "int ten_sum(int a, int b, int c, int d, int e, int f, int g, int h, int i) {return a+b+c+d+e+f-g-h+i;} int main() {return ten_sum(1,2,3,4,5,6,7,8,9);}"
assert 25 "int ten_sum(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j) {return a+b+c+d+e+f-g-h+i+j;} int main() {return ten_sum(1,2,3,4,5,6,7,8,9,10);}"

# mod (%)
assert 0 "int main() {10%5;}"
assert 2 "int main() {10%4;}"
assert 4 "int main() {3+5%2;}"
assert 0 "int is_prime(int n) {if(n==2) return 1; int i; for(i=2; i*i<=n; i=i+1) {if(n%i==0) return 0;} return 1;} int main() {return is_prime(10);}"
assert 1 "int is_prime(int n) {if(n==2) return 1; int i; for(i=2; i*i<=n; i=i+1) {if(n%i==0) return 0;} return 1;} int main() {return is_prime(7);}"

# accessing address and dereferecing using adress (*, &)
assert 3 "int main() {int x; int *y; x=3; y=&x; return *y;}"
assert 10 "int main() {int x; int *y; x=10; y=&x; return *y;}"
assert 3 "int main() {int x; int y; int *z; x=3; y=5; z=&y+8; return *z;}"
assert 10 "int main() {int x; int y; int *z; x=10; y=5; z=&y+8; return *z;}"

echo OK
