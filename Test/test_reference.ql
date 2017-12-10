// Testing calling by-ref or by-value
//
// Shows that calling is done by-value for int/string types
// Shows that calling is done by-reference for array types

Other(int a)
{
	print("Argument a = ",a,"\n");
	a *= 100;
	print("Argument a = ",a,"\n");
}

OtherString(string b)
{
	print("Argument b = ",b,"\n");
	b = "something other";
	print("Argument b = ",b,"\n");
}

OtherArray(array c)
{
	print("Array index 0: ",c[0],"\n");
	print("Array index 1: ",c[1],"\n");
	print("Array index 2: ",c[2],"\n");
	c[0] = 666;
	c[1] = 777;
	c[2] = 888;
}

main()
{
	int a = 10;
	string b = "The test";
	array c;
	
	Other(a);
	print("After call a = ",a,"\n");
	
	a = 20;
	Other(a);
	print("After call a = ",a,"\n");
	
	OtherString(b);
	print("After call b = ",b,"\n");
	
	c = newarray(3);
	c[0] = 10;
	c[1] = 11;
	c[2] = 12;
	
	OtherArray(c);
	print("After call c[0] : ",c[0],"\n");
	print("After call c[1] : ",c[1],"\n");
	print("After call c[2] : ",c[2],"\n");
}

