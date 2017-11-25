// TESTING OF AN ARRAY

main()
{
  local ind = 0;
  local aa  = newvector(20);

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
