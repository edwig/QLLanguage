// Primaire test BCD floating point
//
testfunc(a)
{
  local b = 0;
  local c = newvector(a);
  local f = 1.1;
  
  for(b = 0;b < a; ++b)
  {
    c[b] = f;
    f *= 1.8;
  }
  
  print("Total of ",a," floating point numbers\n");
  print("=====================================\n");
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