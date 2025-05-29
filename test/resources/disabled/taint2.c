#include <stdlib.h>
int test(int* a, int b)
{
  int r = 0;
  for(int i = 0; i < b; i++)
  {
    r += a[i];
  }
  return r;
}

int* mymalloc(int size) {
  return (int*)malloc(size * sizeof(int));
}

int* myfakemalloc(int* ptr) {
  return ptr;
}

int mid(int* a) {
  a = myfakemalloc(a);
  for(int i = 0; i < 5; i++) {
    a[i] = i + 1;
  }
  int b = 5;
  int result = test(a, b);
  return result;
}

int main(int argc, char** argv) {
  int* a = mymalloc(5);
  return mid(a);
}