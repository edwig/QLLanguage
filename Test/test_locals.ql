my_function(int a,int b)
{
  int c = 12 + a;
  int d = a  + b;
    
  print("Sum: ",d," Local: ",c,"\n");
  {
    int  e = 20;
    int  f = 15;
    int  g = e + f + d + c;
    print("Som: ",g,"\n");

    return g;
  }
}

do_strings()
{
	string str1 = "aap";
	string str2 = "noot";
	string str3 = "mies";
	
	string str4 = "aap" + " : " + "noot" + " : " + mies;
  
	print(str4,"\n");
}
    
main()
{
  my_function(2,8);
  my_function(3,7);
  do_strings();
  
  print("All OK\n");
}
