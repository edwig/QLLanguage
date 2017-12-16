// Testing the string.find function
// Also using the string.size function

main()
{
    //           "0---------1---------2---------3---------4---------5---------6---------7---------8---------9--"
	string all = "This is a longer string with everything in it: the quick brown fox jumped over the lazy dog!";
	int position = 0;
	int size = 0;
	
	// Finding the 'y' char
	position = all.find('y');
	print("The first 'y' sits at position: ",position,"\n");  // 33
	
	++position;
	position = all.find('y',position);
	print("The second 'y' sits at position: ",position,"\n");	// 86
	
	// Finding the word 'the'
	position = all.find("the");
	print("The first 'the' sits at position: ",position,"\n");  // 47
	
	// Finding the second word 'the'
	++position;
	position = all.find("the",position);
	print("The second 'the' sits at position: ",position,"\n");  // 79
	
	size = all.size();
	print("The size of the string is: ",size,"\n"); // 92
}