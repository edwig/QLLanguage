// Testing the addition of strings and integers

main()
{
   local str1 = "This is a ";
   local str2 = "STRING";
   local str3;
   
   local num1 = 12;
   local num2 = 24;
   local num3;
   
   str3 = str1 + str2;
   num3 = num1 + num2;
   
   print(str3,"\n");
   print("Result: ",num3,"\n");
   
   str3 = str1 + num2;
   print("Mixed : ",str3,"\n");
}
