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
  destroy();
}

secondclass::secondclass()
{
  print("Created second\n");
}
  
secondclass::destroy()
{
  print("Destroyed second class\n");
}

// MAIN PROGRAM

main()
{
  local var1 = new firstclass();
  local var2 = new secondclass();

  delete var1;
  delete var2;
  
  print("All done\n");
}
