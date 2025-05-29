//{"func": "test"}
int test(int b, int* a)
{
  int r = 0;
  for(int i = 0; i < b; i++)
  {
    r += a[i];
  }
  return r;
}

int main(int argc, char** argv) {
  int a[5] = {1, 2, 3, 4, 5};
  int b = 5;
  int result = test(b, a);
  return result;
}