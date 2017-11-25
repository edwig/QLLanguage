// Testing of a string array

main()
{
  local chr = "A";
  local cc  = 0;
  local str = newstring(20);
  local len = 0;
  local ind = 0;

  str = "Dit is een test";
  len = sizeof(str);

  for(ind = 0;ind < len; ++ ind)
  {
    cc = str[ind];
    chr[0] = cc;
    print("Char ",ind," = ",cc , " : ", chr, "\n");
  }
}