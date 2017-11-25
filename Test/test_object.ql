
class vehicle
{
  maxspeed;
  cost;
  GetSpeed();
  GetAge();
}

vehicle::vehicle(sp,co)
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
  local car = new vehicle(150,2000);

  print("The maximum speed is: ",car->GetSpeed(),"\n");
  gc();
}