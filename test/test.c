#define A(b, c, d) b + c + d

int ascii(char c)
{
  return c - '0';
}

int main()
{
  return ascii('9') + A(5, 5, 5);
}
