#include <stdlib.h>

typedef struct {
  int a;
  int* b;
  char unused[20];
} MyStruct;

typedef struct {
  size_t size;
  int* data;
} MyStruct2;

int test(MyStruct* a, int b)
{
  int r = 0;
  for(int i = 0; i < b; i++)
  {
    r += a[i].a;
  }
  return r;
}

int test2(MyStruct2* s)
{
  int r = 0;
  for(int i = 0; i < s->size; i++)
  {
    r += s->data[i];
  }
  return r;
}

int main(int argc, char** argv) {
  int b = 5;
  MyStruct* ptr = (MyStruct*) malloc(b * sizeof(MyStruct));
  for (int i = 0; i < b; i++) {
    ptr[i].a = i + 1;
    ptr[i].b = NULL;
  }
  int result = test(ptr, b);

  MyStruct2 s2;
  int size = argv[1] ? atoi(argv[1]) : 10;
  s2.data = (int*) malloc(size * sizeof(int));
  s2.size = size;
  test2(&s2);
  return result;
}