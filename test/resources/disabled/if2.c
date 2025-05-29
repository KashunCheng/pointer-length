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
  int b = 0;
  int result = 0;
  if (b == 0) {
    int c[5] = {1, 2, 3, 4, 5};
    b = 5;
    result = test(c, b);
  } else {
    int c[6] = {1, 2, 3, 4, 5, 6};
    b = 6;
    result = test(c, b);
  }
  return result;
}
