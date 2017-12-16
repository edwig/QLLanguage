// Testing an object

class vehicle
{
  int maxspeed;
  int cost;

  GetCost();  
  GetSpeed();
  GetAge();
}

vehicle::vehicle(int sp,int co)
{
  maxspeed = sp;
  cost = co;

  return this;
}

vehicle::GetSpeed()
{
  return maxspeed;
}

vehicle::GetAge()
{
  return 25;
}

vehicle::GetCost()
{
  return cost;
}

main()
{
  vehicle car = new vehicle(150,2000);

  print("The vehicle costs   : ",car->GetCost(), "\n");
  print("The maximum speed is: ",car->GetSpeed(),"\n");
  print("The age in years is : ",car->GetAge(),  "\n");
  gc();
}
