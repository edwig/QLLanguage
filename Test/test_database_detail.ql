// Testing the database functions

main()
{
	dbase  db     = newdbase("testing","sysdba","altijd");
	query  qry    = newquery(db);
	string select = "SELECT SUM(amount) FROM detail";
	
	qry.DoSQLStatement(select);
	if(qry.GetRecord())
	{
		variant var = qry.GetColumn(1);
		print("Total amount: ",var,"\n");  // 1350
	}
	qry.Close();
}