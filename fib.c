#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define MEASURE 1

typedef unsigned long long lint;

lint _fibr(lint left, lint right, lint n) {
  if(n==0)
    return left;
  return _fibr(right, left+right, n-1);
}

lint fibr(lint n) {
  return _fibr(0, 1, n);
}

lint fibf(lint n) {
  lint l=0, r=1;
  while(n--) {
    lint tmp = l;
    l = r;
    r += tmp;
  }
  return l;
}

int measure(lint (*fib)(lint), lint n, int times) {
  clock_t sum=0;
  int i;
  for(i=0; i!=times; i++) {
    clock_t start;
    start = clock();
    fib(n);
    sum += clock() - start;
  }
  return sum / times * (1000000 / CLOCKS_PER_SEC); 
}

int main(int argc, const char *argv[])
{
#if MEASURE==0
  lint n, fr, ff;
  int times, m;;
  n = atoi(argv[1]);
  times = atoi(argv[2]);
  fr = fibr(n);
  ff = fibf(n);
  printf("fib(%lld) = %lld, %lld\n", n, fr, ff);
  m = measure(fibr, n, times);
  printf("elapsed: %d usec\n", m);
#else
  int i;
  for(i=0; i<=100000; i+=1000) {
    int n;
    n = (i<=10000 ? 1000 : 100);
    printf("%6d %6d %6d\n", i, measure(fibr, i, n), measure(fibf, i, n));
  }
#endif

  return 0;
}
