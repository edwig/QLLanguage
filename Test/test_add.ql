// Testing the addition of strings and integers

main()
{
   string str1 = "This is a ";
   string str2 = "STRING";
   string str3,str4;
   
   int num1 = 12;
   int num2 = 24;
   int num3 = 3, num4 = 4;
   
   str3 = str1 + str2;
   num3 = num1 + num2;
   
   print(str3,"\n");
   print("Result: ",num3,"\n");
   
   str3 = str2 + num2;
   print("Mixed : ",str3,"\n");
   
   return 0;
}
