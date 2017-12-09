// Comment outside all scopes

global string name = "Edwig Huisman";
global string surname;
global int    num = 12;

// The main function
// Globals must be initialized before calling this

main()
{
  string t = "This is a text\n";
  int    i = 0;

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
