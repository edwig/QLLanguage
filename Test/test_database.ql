// Database basic test

main()
{
  dbase dbs = newdbase("testing","sysdba","altijd");
  query qry = newquery(dbs);
  string cr = "\n";
  int  counter = 0;
  int  columns = 0;
  int  collen  = 0;
  string name;

  if(dbs.IsOpen())
  {
    if(qry.DoSQLStatement("SELECT * FROM detail"))
    {
      while(qry.GetRecord())
      {
        variant id          = qry->GetColumn(1);
        variant master_id   = qry.GetColumn(2);
		    variant line        = qry.GetColumn(3);
		    variant description = qry.GetColumn(4);
		    variant amount      = qry.GetColumn(5);

		    print("RECORD\n");
		    print("ID         : ",id,cr);
		    print("Master ID  : ",master_id,cr);
		    print("Line number: ",line,cr);
		    print("Description: ",description,cr);
		    print("Amount     : ",amount,cr);
		    print(cr);

		    ++counter;
	    }
	    columns = qry.GetNumberOfColumns();
	    name    = qry.GetColumnName(4);
	    collen  = qry.GetColumnLength(4);
	  
      qry.Close();
      dbs.Close();

      print("Total records: ", counter,cr);
	    print("Columns total: ", columns,cr);
	    print("Name col 4   : ", name,cr);
	    print("Max name len : ", collen,cr);
	    print(cr);
	    print("Ready\n");
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
