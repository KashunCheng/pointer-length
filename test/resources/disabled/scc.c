#include <alloca.h>

int test_b(int* a, int b);
int test_a(int* a, int b)
{
  if (b == 0)
  {
    return 0;
  }
  int* c = (int*) alloca(sizeof(int) * b);
  for (int i = 0; i < b; i++)
  {
    c[i] = a[i];
  }
  return test_b(c, b);
}

int test_b(int* a, int b)
{
  int* c = (int*) alloca(sizeof(int) * b);
  for (int i = 0; i < b; i++)
  {
    c[i] = a[i];
  }
  return *c + test_a(c + 1, b - 1);
}

int main() {
  int a[5] = {1, 2, 3, 4, 5};
  int b = 5;
  int result = test_a(a, b);
  return result;
}