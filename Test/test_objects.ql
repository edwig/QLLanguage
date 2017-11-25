class foo // the base class
{
  a,b;
  static last;
  static get_last();
}

foo::foo(aa,bb)
{
  a = aa; b = bb;
  return last = this;
}

foo::get_last()
{
  return last;
}

foo::set_a(aa)
{
  a = aa;
  return this;
}

foo::set_b(bb)
{
  b = bb;
  return this;
}

foo::count()
{
  local i;
  for (i = a; i <= b; ++i)
  {
    print(i,"\n");
  }
  return this;
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
  c;
}

bar::bar(aa,bb,cc)
{
  foo(aa,bb);
  c = cc;
  return this;
}

bar::get_c()
{
  return c;
}
 
bar::set_c(cc)
{
    c = cc;
    return this;
}

main()
{
    local cr = "\n";
    local foo1 = new foo(1,2);
    local foo2 = new foo(11,22);
    // static last value
    last = 99;
    
    print("foo1=",foo1,cr);
    print("foo2=",foo2,cr);
    print("foo1->a=",foo1->get_a(),cr);
    print("foo2->a=",foo2->get_a(),cr);
    print("foo1->b=",foo1->get_b(),cr);
    print("foo2->b=",foo2->get_b(),cr);
    {
      local bar1 = new bar(111,222,333);
      local bar2 = new bar(1111,2222,3333);
    
      print("bar1=",bar1,cr);
      print("bar2=",bar2,cr);
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
      print("last=",last,cr);
      print("foo::last=",foo::get_last(),cr);
      print("bar::last=",bar::get_last(),cr);
  }
  gc();
}
