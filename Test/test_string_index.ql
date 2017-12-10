// Test the STRING.index method

main()
{
  string str = "Edwig Huisman";
  int    num = 1;
 
  num = str.index(0);
  
  print("First char = ",str.index(0),"\n");    // 69
  print("Last  char = ",str.index(-1),"\n");   // 110
  print("Fifth char = ",str.index(4),"\n");    // 103
}
