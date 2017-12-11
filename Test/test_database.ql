// Database test

main()
{
  database dbs = newdbs("owoc09","k2b","k2b");
  query    qry = newquery(dbs);
  int  counter = 0;

  if(dbs.IsOpen())
  {
    if(qry.DoSQLStatement("SELECT usr_name,usr_id FROM k2b_aut_gebruiker"))
    {
      while(qry.GetRecord())
      {
        string name = qry.GetColumn(1);
        int    id   = qry.GetColumn(2);

        print("User name: ",name,"\n");
        print("User ID  : ",id,  "\n\n");

        ++counter;
      }
      qry.Close();
      dbs.Close();

      print("Total users: ", counter, "\n");
    }
    else
    {
      print("Incorrect query!\n");
    }
  }
  else
  {
    print("Database not opened!\n");
  }
}