// Testing the square/round functions

main()
{
  int ind;
  int val;
  int sqr;

  for(ind = 1.0; ind <= 500.0; ++ind)
  {
    val = ind * ind;
    sqr = sqrt(val);
    sqr = round(sqr,4);

    print ("Val: ",ind," Square: ",val," Root: ", sqr, "\n");
  }
}