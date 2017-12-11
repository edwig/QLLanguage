factorial(int n)
{
    return n == 0 ? 1 : n * factorial(n-1);
}

do_factorial(int n)
{
    print(n," factorial is ",factorial(n),"\n");
}

main()
{
  int i = 0;

  for (i = 1; i < 14; ++i)
  {
    do_factorial(i);
  }
}
