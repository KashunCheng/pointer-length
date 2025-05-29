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
  int a[5] = {1, 2, 3, 4, 5};
  int b = 5;
  int result;
  if (b==5) {
    result = test(a, b);
  } else {
    result = test(a+1, b-1);
  }
  return result;
}