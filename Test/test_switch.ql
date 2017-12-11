// TEST CASES SWITCH

show_choice(int n)
{
   switch(n)
   {
       case 1: print("one"); break;
       case 2: print("two"); break;
       case 3: print("three"); break;
       default:print("unknown"); break;
   }
   print("\n");
}

main()
{
  show_choice(3);
  show_choice(1);
  show_choice(2);
  show_choice(0);
}