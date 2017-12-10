// testing string.left(n) and string.right(n)

main()
{
    //           "0---------1---------2---------3---------4---------5---------6---------7---------8---------9--"
	string all = "This is a longer string with everything in it: the quick brown fox jumped over the lazy dog!";
	string sub;
	int position = 0;

	sub = all.left(7);
	print("Left  side: ",sub,"\n");
	
	sub = all.right(9);
	print("Right side: ",sub,"\n");
}