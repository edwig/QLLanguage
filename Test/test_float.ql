// Primaire test BCD floating point
//
testfunc(int a)
{
  int   b = 0;
  array c = newarray(a);
  bcd   f = 1.1;
  
  for(b = 0;b < a; ++b)
  {
    c[b] = f;
    f *= 1.8;
  }
  
  print("Total of ",a," floating point numbers\n");
  print("=====================================\n");
  b = 1;
  
  for(b = 0;b < a; ++b)
  {
    print("Getal = ", c[b], "\n");
  }
  print("-----------------\n\n");
}

main()
{
  print("Testing of floating point numbers\n");
  print("---------------------------------\n");
  print("\n");
  testfunc(10);
  testfunc(20);
}
