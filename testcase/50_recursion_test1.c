int putint(int i);
int fact(int n) {
  if (n == 0) {
    return 1;
  }
  int nn;
  nn = n-1;
  return (n * fact(nn));
}

int main() {
  int n;
  n = 5;
    int tmp;
    int res;
    res = fact(n);
    tmp = putint(res);
  return fact(n);
}
