// TESTING OF AN ARRAY

main()
{
  int   ind = 0;
  array aa  = newarray(20);

  print("Calculating...\n");
  for(ind = 1; ind <= 20; ++ind)
  {
    aa[ind-1] = ind * ind;  
  }
  print("Printing...\n");
  for(ind = 1; ind <= 20; ++ind)
  {
    print("The square of: ",ind," = ",aa[ind-1],"\n");
  }
  print("That's all folks!\n");
}
