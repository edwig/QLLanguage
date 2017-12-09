// CLASS with a constructor with two variables

class firstclass
{
  firstclass(a,b);
  identity();
  plus();
  x;
  y;
}

firstclass::firstclass(a,b)
{
  x = a;
  y = b;
  print("Created object first",this,"\n");
}

firstclass::identity()
{
  print("X = ",x," Y = ",y,"\n");
  print("OBJECT = ",this,"\n");
}  
  
firstclass::plus()
{
  return x + y;
}  
  
// MAIN PROGRAM

main()
{
  local var1 = new firstclass(13,7);
  local var2 = new firstclass(12,88);

  var1.identity();
  var2.identity();
  
  print("Total 1 : ",var1.plus(),"\n");
  print("Total 2 : ",var2.plus(),"\n");
  
  print("All done\n");
}
