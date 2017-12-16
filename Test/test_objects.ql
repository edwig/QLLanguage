// TEST two objects super/sub classing

global int last2 = 99;

class foo // the base class
{
  int a;
  int b;
  
  static foo last;
  static get_last();
}

foo::foo(int aa,int bb)
{
  a = aa;
  b = bb;
  last = this;
}

foo::get_last()
{
  return last;
}

foo::set_a(int aa)
{
  a = aa;
  return this;
}

foo::set_b(int bb)
{
  b = bb;
}

foo::count()
{
  int i;
  for (i = a; i <= b; ++i)
  {
    print(i,"\n");
  }
}

foo::get_a()
{
  return a;
}

foo::get_b()
{
  return b;
}

class bar : foo // a derived class
{
  int c;
}

bar::bar(int aa,int bb,int cc)
{
  a = aa;
  b = bb;
  // foo(aa,bb);
  c = cc;
  return this;
}

bar::get_c()
{
  return c;
}
 
bar::set_c(int cc)
{
  c = cc;
}

main()
{
    string cr = "\n";
    foo foo1 = new foo(1,2);
    foo foo2 = new foo(11,22);
    
    print("foo1->a=",foo1->get_a(),cr);
    print("foo2->a=",foo2->get_a(),cr);
    print("foo1->b=",foo1->get_b(),cr);
    print("foo2->b=",foo2->get_b(),cr);
    {
      bar bar1 = new bar(111, 222, 333);
      bar bar2 = new bar(1111,2222,3333);
    
      print("bar1->a=",bar1->get_a(),cr);
      print("bar1->b=",bar1->get_b(),cr);
      print("bar1->c=",bar1->get_c(),cr);
      print("bar2->a=",bar2->get_a(),cr);
      print("bar2->b=",bar2->get_b(),cr);
      print("bar2->c=",bar2->get_c(),cr);
      print("Foo1 counting\n");
      foo1->count();
      print("Foo2 counting\n");
      foo2->count();
      print("global last = ",last2,cr);
      print("foo::last   = ",foo::get_last()->get_a(),cr);
      print("bar::last   = ",bar::get_last()->get_a(),cr);
  }
  gc();
}
