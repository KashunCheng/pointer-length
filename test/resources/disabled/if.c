int test(int* a, int b)
{
  int r = 0;
  for(int i = 0; i < b; i++)
  {
    r += a[i];
  }
  return r;
}

int main() {
  int* a = nullptr;
  int b = 0;
  if (b == 0) {
    int c[5] = {1, 2, 3, 4, 5};
    a = &c[0];
    b = 5;
  } else {
    int c[6] = {1, 2, 3, 4, 5, 6};
    a = &c[0];
    b = 6;
  }
  int result = test(a, b);
  return result;
}
