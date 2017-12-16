// TEST string.makeupper / string.makelower

main()
{
  string str = "The Quick Brown Fox Jumped Over The Lazy Dog!";
  string upper = str.makeupper();
  string lower = upper.makelower();
  
  print("Original : ",str,  "\n");
  print("Uppercase: ",upper,"\n");
  print("Lowercase: ",lower,  "\n");
}