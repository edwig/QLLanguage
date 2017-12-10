print1number(int a)
{
  print("The number is: ",a,"\n");
}

my_function(int a,int b,int g)
{
  int c = 12 + a;
  int d = a  + b;
    
  print("Sum: ",d," Local: ",c,"\n");
  print1number(g);
}
    
AllOK()
{
  print("All OK\n");
}	

	
main()
{
  int num = 0;

  // Cannot call with other than integers!
  // my_function(3,4,"test");    // ERROR!
  
  my_function(2,4,8); // Prints: "Sum: 6, Local: 14"
  my_function(3,5,7); // Prints: "Sum: 8, Local: 15"
  
  for(num = 0;num < 20; ++num)
  {
     print1number(num);
  }

  AllOK();
}
