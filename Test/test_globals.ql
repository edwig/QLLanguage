// Comment outside all scopes

global name = "Edwig Huisman";
global surname;
global num = 12;

// The main function
// Globals must be initialized before calling this

main()
{
  local t = "This is a text\n";
  local i = 0;

  print("The name is: ",name,"\n");
  print(t);
  print("The value of num = ",num,"\n");
  for(i = 1; i <= num; ++i)
  {
    print("Value of i: ",i,"\n");
  }

  surname = "Huisman";
  
  print("Surname is: ",surname,"\n");
}
