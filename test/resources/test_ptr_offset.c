//{"func": "test"}
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
  char a[5] = {1, 2, 3, 4, 5};
  char *aa = a + 3;
  int b = 5 - (aa - a);
  int result = test(b, aa);
  return result;
}