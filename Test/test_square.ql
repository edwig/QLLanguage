// Testing the square/round functions

main()
{
  local ind;
  local val;
  local sqr;

  for(ind = 1.0; ind <= 500.0; ++ind)
  {
    val = ind * ind;
    sqr = sqrt(val);
    sqr = round(sqr,6);

    print ("Val: ",ind," Square: ",val," Root: ", sqr, "\n");
  }
}