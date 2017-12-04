// Test the STRING.index method

main()
{
  local str = "Edwig Huisman";
  local num = 1;
 
  num = str.index(0);
  
  print("First char = ",str.index(0),"\n");
  print("Last  char = ",str.index(-1),"\n");
  print("Fifth char = ",str.index(4),"\n");
}