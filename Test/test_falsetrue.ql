// Testing the constants 'false' and 'true'

print_if_true(int value)
{
	if(value)
	{
		print("Value is true\n");
	}
	else
	{
		print("Value is false\n");
	}
}

main()
{
	int one = false;
	int two = true;
	
	if(one)
	{
		print("You should never see this!\n");
	}
	if(!one)
	{
		print("One is false\n");
	}
	
	if(two)
	{
		print("Two is true\n");
	}
	if(!two)
	{
		print("You should never see this!\n");
	}
	
	print_if_true(true);
	print_if_true(false);
}
