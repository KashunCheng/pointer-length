int test(int* a, int b)
{
  if (b == 0)
  {
    return 0;
  }
  return *a + test(a + 1, b - 1);
}

int main() {
  int a[5] = {1, 2, 3, 4, 5};
  int b = 5;
  int result = test(a, b);
  return result;
}
