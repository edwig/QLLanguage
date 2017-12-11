
class vehicle
{
  int maxspeed;
  int cost;
  
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



main()
{
  vehicle car = new vehicle(150,2000);

  print("The maximum speed is: ",car->GetSpeed(),"\n");
  gc();
}