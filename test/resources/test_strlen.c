//{"func": "test"}
#include "string.h"
int test(int b, char* a)
{
  int r = 0;
  for(int i = 0; i < b; i++)
  {
    r += a[i];
  }
  return r;
}

int main(int argc, char** argv) {
  char a[5] = {1, 2, 3, 4, 0};
  int b = strlen(a);
  int result = test(b, a);
  return result;
}