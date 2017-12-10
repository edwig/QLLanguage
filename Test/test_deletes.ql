// FIRST CLASS

class firstclass
{
  firstclass();
  int one;
  int two;
}

firstclass::firstclass()
{
  print("Created first\n");
}

// SECOND CLASS

class secondclass
{
  secondclass();
  identity();
  destroy();
  
  int number;
}

secondclass::secondclass(int num)
{
  number = num;
  print("Created second: ",number,"\n");
  print("OBJECT = ",this,"\n");
}
  
secondclass::identity()
{
  print("I am number: ",number,"\n");
  print("OBJECT = ",this,"\n");
}  
  
secondclass::destroy()
{
  print("Destroyed second class: ",number,"\n");
  print("OBJECT = ",this,"\n");
}

// MAIN PROGRAM

main()
{
  firstclass var1 = new firstclass();
  array vec  = newarray(5);
  int   ind  = 0;
  
  for(ind = 0;ind < 5;++ind)
  {
    vec[ind] = new secondclass(ind);
  }
  
  delete var1;
  
  for(ind = 0;ind < 5;++ind)
  {
    vec[ind].identity();
  }
  
  for(ind = 0;ind < 5;++ind)
  {
    delete vec[ind];
  }
  
  print("All done\n");
}
