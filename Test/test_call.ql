print1number(a)
{
  print("The number is: ",a,"\n");
}

my_function(a,b,g)
{
  local c = 12 + a;
  local d = a  + b;
    
  print("Sum: ",d," Local: ",c,"\n");
  print1number(g);
}
    
main()
{
  my_function(2,4,8); // Prints: "Sum: 6, Local: 14"
  my_function(3,5,7); // Prints: "Sum: 8, Local: 15"
  
  print("All OK\n");
}