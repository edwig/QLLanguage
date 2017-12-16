// Testing of a string array

main()
{
  string chr = "A";
  int    cc  = 0;
  string str = "Dit is een test";
  int    len = 0;
  int    ind = 0;

  len = sizeof(str);

  for(ind = 0;ind < len; ++ ind)
  {
    cc = str[ind];
    chr[0] = cc;
    print("Char ",ind," = ",cc , " : ", chr, "\n");
  }
}
