my_function(a,b)
{
  local c = 12 + a;
  local d = a  + b;
    
  print("Sum: ",d," Local: ",c,"\n");
  {
    local e = 20;
    local f = 15;
    local g = e + f + d + c;
    print("Som: ",g,"\n");

    return g;
  }
}

do_strings()
{
	local str1 = "aap";
	local str2 = "noot";
	local str3 = "mies";
	
	local str4 = "aap" + " : " + "noot" + " : " + mies;
	print(str4,"\n");
}
    
main()
{
  my_function(2,8);
  my_function(3,7);
  do_strings();
  
  print("All OK\n");
}
