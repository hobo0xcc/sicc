// compute fibonacci numbers
int fib(int n)
{
  if (n < 2)
    return n;
  else
    return fib(n - 1) + fib(n - 2);
}
/*
 * main function
 */
int main()
{
  int n = 10;
  printf("%d\n", fib(n));
  return 0;
}
