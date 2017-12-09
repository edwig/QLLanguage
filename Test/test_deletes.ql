// FIRST CLASS

class firstclass
{
  firstclass();
  one;
  two;
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
  number;
}

secondclass::secondclass(num)
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
  local var1 = new firstclass();
  local vec  = newvector(5);
  local ind  = 0;
  
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
