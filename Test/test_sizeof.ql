// TEST sizeof function

main()
{
	array  var  = newarray(14);
    int    size = sizeof(var);
	string str  = "The quick brown fox jumped over the lazy dog!";
	
	print("Size of array  is: ",size,"\n");
	print("Size of string is: ",sizeof(str),"\n");
}