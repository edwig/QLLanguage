// Testing the string.substring method

main()
{
    //           "0---------1---------2---------3---------4---------5---------6---------7---------8---------9--"
	string all = "This is a longer string with everything in it: the quick brown fox jumped over the lazy dog!";
	string sub;
	int position = 0;

	position = all.find("every");
	sub = all.substring(position,10);
	print("It is in: ",sub,"\n");
	
	position = all.find("lazy");
	sub = all.substring(position);
	
	print("It is a:  ",sub,"\n");
}