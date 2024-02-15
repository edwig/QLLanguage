//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
// QL Language functions
// ir. W.E. Huisman (c) 2018
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "QL_Language.h"
#include "QL_MemObject.h"
#include "QL_Functions.h"
#include "QL_vm.h"
#include "QL_Interpreter.h"
#include "QL_Objects.h"
#include "SQLDatabase.h"
#include "SQLQuery.h"
#include "SQLVariant.h"
#include <stdio.h>
#include <process.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAX_PATH     260
#define BUFFER_SIZE (32 * 1024)

// Add a built-in function
static void
add_function(TCHAR* p_name,int (*p_fcn)(QLInterpreter*,int),QLVirtualMachine* p_vm)
{
  MemObject* sym = p_vm->AddInternal(p_name);
  sym->m_value.v_internal = p_fcn;
}

// Add a built-in file
static void 
add_file(TCHAR* p_name,FILE* p_fp,QLVirtualMachine* p_vm)
{
  MemObject* sym = p_vm->AddSymbol(p_name);
  p_vm->MemObjectSetType(sym,DTYPE_NIL);
  p_vm->MemObjectSetType(sym,DTYPE_FILE);
  sym->m_value.v_file = p_fp;
}

// Add a method for a built-in-datatype
static void 
add_method(int p_type,TCHAR* p_name,int (*p_fcn)(QLInterpreter*,int),QLVirtualMachine* p_vm)
{
  Method* method = p_vm->AddMethod(p_name,p_type);
  method->m_internal = p_fcn;
}

// Check the number of arguments on the stack
static int 
argcount(QLInterpreter* p_inter,int n,int cnt)	
{ 
  QLVirtualMachine* vm = p_inter->GetVirtualMachine();

  if(n < 0)
  {
    //  Happens on internal methods of data types
    return TRUE;
  }
  if ((n) < (cnt)) 
  {
    vm->Error(_T("Too many arguments"));
    return (FALSE);
  }
  else if ((n) > (cnt))
  {
    vm->Error(_T("Too few arguments"));
    return (FALSE);
  }
  // n == cnt
  return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//
// ALL THE FUNCTIONS FOLLOW HERE
//
//////////////////////////////////////////////////////////////////////////

// Get the data type of a value
static int xtypeof(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  int type = p_inter->GetStackPointer()[0]->m_type;
  p_inter->SetInteger(type);
  return 0;
}

// Allocate a new array vector
static int xnewarray(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  int size = p_inter->GetIntegerArgument(0);
  QLVirtualMachine* vm = p_inter->GetVirtualMachine();

  Array* array = new Array(vm,size);
  MemObject* object = vm->AllocMemObject(DTYPE_NIL);
  object->m_type = DTYPE_ARRAY;
  object->m_value.v_array = array;
  object->m_flags |= FLAG_DEALLOC;

  p_inter->GetStackPointer()[0] = object;
  return 0;
}

// Allocate a new string
static int xnewstring(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,0);
  p_inter->SetString(_T(""));
  return 0;
}

// Get the size of a vector or string
static int xsizeof(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  MemObject* object = *p_inter->GetStackPointer();
  switch (object->m_type) 
  {
    case DTYPE_ARRAY:   p_inter->SetInteger(object->m_value.v_array->GetSize());
                        break;
    case DTYPE_STRING:  p_inter->SetInteger(object->m_value.v_string->GetLength());
                        break;
    case DTYPE_BCD:     p_inter->SetInteger(sizeof(bcd));
                        break;
    case DTYPE_INTEGER: p_inter->SetInteger(sizeof(int));
                        break;
    case DTYPE_DATABASE:p_inter->SetInteger(sizeof(SQLDatabase));
                        break;
    case DTYPE_QUERY:   p_inter->SetInteger(sizeof(SQLQuery));
                        break;
    case DTYPE_VARIANT: p_inter->SetInteger(sizeof(SQLVariant));
                        break;
    default:            // Cannot take the size of this object. Fail silently as 'zero'
                        p_inter->SetInteger(0);
                        break;
  }
  return 0;
}

// Turn trace on or off
static int xtrace(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  p_inter->CheckType(0,DTYPE_INTEGER);
  int onoff = p_inter->GetIntegerArgument(0);
  p_inter->SetTracing(onoff != 0 ? true : false);
  return 0;
}

// open a file
static int xfopen(QLInterpreter* p_inter,int argc)
{
  FILE*  fp = NULL;

  argcount(p_inter,argc,2);
  CString fileName = p_inter->GetStringArgument(1);
  CString mode     = p_inter->GetStringArgument(0);

  _tfopen_s(&fp,fileName,mode);
  if (fp == NULL)
  {
    p_inter->SetNil(0);
  }
  else
  {
    p_inter->SetFile(fp);
  }
  return 0;
}

// Close a file
static int xfclose(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  FILE* fp = p_inter->GetStackPointer()[0]->m_value.v_file;
  p_inter->SetInteger(fclose(fp));
  return 0;
}

// Get a character from a file
static int  xgetc(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  p_inter->CheckType(0,DTYPE_FILE);
  FILE* fp = p_inter->GetStackPointer()[0]->m_value.v_file;
  p_inter->SetInteger(_gettc(fp));
  return 0;
}

// Output a character to a file
static int xputc(QLInterpreter* p_inter,int argc)
{
  MemObject** sp = p_inter->GetStackPointer();

  argcount(p_inter,argc,2);
  FILE* fp = sp[0]->m_value.v_file;
  int   cc = p_inter->GetIntegerArgument(1);
  p_inter->SetInteger(_puttc(cc,fp));
  return 0;
}

// Get a string from standard input
static int xgets(QLInterpreter* p_inter,int argc)
{
  int cc = 0;
  MemObject** sp = p_inter->GetStackPointer();

  argcount(p_inter,argc,1);
  FILE* fp = sp[0]->m_value.v_file;
  CString s;
  while((cc = _gettc(fp)) != _TEOF && cc != '\n')
  {
    s.Append((const TCHAR*) &cc);
  }
  p_inter->SetString(s);
  return 0;
}

static int xputs(QLInterpreter* p_inter,int argc)
{
  MemObject** sp = p_inter->GetStackPointer();

  argcount(p_inter,argc,2);
  FILE* fp = sp[0]->m_value.v_file;
  CString* str = sp[1]->m_value.v_string;
  p_inter->SetInteger(_fputts(*str,fp));
  return 0;
}

// Generic print function to standard output
static int xprint(QLInterpreter* p_inter,int argc)
{
  int len = 0;
  MemObject** sp = p_inter->GetStackPointer();
  QLVirtualMachine* vm = p_inter->GetVirtualMachine();

  for (int n = argc; --n >= 0; )
  {
    len += vm->Print(stdout,FALSE,sp[n]);
  }
  // total chars printed
  p_inter->SetInteger(len);

  return 0;
}

// Generic print function to a file descriptor
static int xfprint(QLInterpreter* p_inter,int argc)
{
  int len = 0;
  MemObject** sp = p_inter->GetStackPointer();
  QLVirtualMachine* vm = p_inter->GetVirtualMachine();

  // First argument is the file descriptor
  p_inter->CheckType(argc,DTYPE_FILE);
  FILE* file = sp[argc]->m_value.v_file;

  for (int n = argc - 1; n >= 0; --n)
  {
    len += vm->Print(file,FALSE,sp[n]);
  }
  p_inter->SetInteger(len);
  return 0;
}

// Get an argument from the argument list
static int xgetarg(QLInterpreter* p_inter,int argc)
{
  MemObject** sp = p_inter->GetStackPointer();
  int      index = p_inter->GetIntegerArgument(0);

  argcount(p_inter,argc,1);

  if (index >= 0 && index < qlargc)
  {
    p_inter->SetString(qlargv[index]);
  }
  else
  {
    p_inter->SetNil(0);
  }
  return 0;
}

// Execute a system command
static int xsystem(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  CString command = p_inter->GetStringArgument(0);
  p_inter->SetInteger(_tsystem(command));
  return 0;
}

// Exit the system
static int xexit(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  int ex = p_inter->GetIntegerArgument(0);
  // DIRECT EXIT THIS QL INTERPRETER AND SURROUNDING PROGRAM
  exit(ex);
}

// Do the garbage collection
static int xgc(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,0);
  p_inter->GetVirtualMachine()->GC();
  p_inter->SetNil(0);
  return 0;
}

// Trigonometric Sine
static int xsin(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd(val.Sine());
  return 0;
}

// Trigonometric Cosine
static int xcos(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd( val.Cosine());
  return 0;
}

// Trigonometric Tangent
static int xtan(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd( val.Tangent());
  return 0;
}

// Trigonometric Arcsine
static int xasin(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd( val.ArcSine());
  return 0;
}

// Trigonometric Arccosine
static int xacos(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd( val.ArcCosine());
  return 0;
}

// Trigonometric Arctangent
static int xatan(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd( val.ArcTangent());
  return 0;
}

// Mathematical square root
static int xsqrt(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd( val.SquareRoot());
  return 0;
}

// Mathematical ceiling
static int xceil(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd( val.Ceiling());
  return 0;
}

// Mathematical floor
static int xfloor(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd( val.Floor());
  return 0;
}

// Mathematical exponent
static int xexp(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd( val.Exp());
  return 0;
}

// Mathematical 10 base logarithm
static int xlog(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd( val.Log10());
  return 0;
}

// Mathematical natural logarithm
static int xlogn(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd( val.Log());
  return 0;
}

// Mathematical 10 base logarithm
static int xlog10(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd( val.Log10());
  return 0;
}

// Mathematical power
static int xpow(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,2);
  bcd val = p_inter->GetBcdArgument(0);
  bcd pow = p_inter->GetBcdArgument(1);
  p_inter->SetBcd( val.Power(pow));
  return 0;
}

// Mathematical random number
static int xrand(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,0);
  int rr = rand();
  bcd val = bcd(rr) / bcd((long)RAND_MAX);
  QLVirtualMachine* vm = p_inter->GetVirtualMachine();
  p_inter->SetBcd(val);
  return 0;
}

// Conversion to BCD
static int xtobcd(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  MemObject* object = *p_inter->GetStackPointer();

  if(object->m_type == DTYPE_INTEGER)
  {
    int n = p_inter->GetIntegerArgument(0);
    bcd val(n);
    p_inter->SetBcd(val);
  }
  else if(object->m_type == DTYPE_STRING)
  {
    CString str = p_inter->GetStringArgument(0);
    bcd val(str);
    p_inter->SetBcd(val);
  }
  else if(object->m_type == DTYPE_VARIANT)
  {
    bcd val = p_inter->GetSQLVariantArgument(0)->GetAsBCD();
    p_inter->SetBcd(val);
  }
  else if(object->m_type == DTYPE_BCD)
  {
    // Nothing to be done
  }
  else
  {
    p_inter->CheckType(0,DTYPE_BCD);
  }
  return 0;
}

// Conversion to INTEGER
static int xtoint(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  MemObject* object = *p_inter->GetStackPointer();

  if(object->m_type == DTYPE_BCD)
  {
    bcd val = p_inter->GetBcdArgument(0);
    p_inter->SetInteger(val.AsLong());
  }
  else if(object->m_type == DTYPE_STRING)
  {
    CString str = p_inter->GetStringArgument(0);
    p_inter->SetInteger(_ttoi(str));
  }
  else if(object->m_type == DTYPE_VARIANT)
  {
    int val = p_inter->GetSQLVariantArgument(0)->GetAsSLong();
    p_inter->SetInteger(val);
  }
  else if(object->m_type == DTYPE_INTEGER)
  {
    // NOTHING TO BE DONE
  }
  else
  {
    p_inter->CheckType(0,DTYPE_INTEGER);
  }
  return 0;
}

// Conversion to STRING
static int xtostr(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  MemObject* object = *p_inter->GetStackPointer();

  if(object->m_type == DTYPE_INTEGER)
  {
    int n = p_inter->GetIntegerArgument(0);
    CString str;
    str.Format(_T("%d"),n);
    p_inter->SetString(str);
  }
  else if(object->m_type == DTYPE_BCD)
  {
    bcd n = p_inter->GetBcdArgument(0);
    CString val = n.AsString();
    p_inter->SetString(val);
  }
  else if(object->m_type == DTYPE_VARIANT)
  {
    CString str(p_inter->GetSQLVariantArgument(0)->GetAsChar());
    p_inter->SetString(str);
  }
  else if(object->m_type == DTYPE_STRING)
  {
    // NOTHING TO BE DONE
  }
  else
  {
    p_inter->CheckType(0,DTYPE_STRING);
  }
  return 0;
}

// Conversion to VARIANT
static int xtovariant(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  MemObject* object = *p_inter->GetStackPointer();

  if(object->m_type == DTYPE_INTEGER)
  {
    p_inter->SetVariant(SQLVariant((long)object->m_value.v_integer));
  }
  else if(object->m_type == DTYPE_STRING)
  {
    p_inter->SetVariant(SQLVariant(object->m_value.v_string));
  }
  else if(object->m_type == DTYPE_BCD)
  {
    p_inter->SetVariant(SQLVariant(object->m_value.v_floating));
  }
  else
  {
    p_inter->BadType(0,object->m_type);
  }
  return 0;
}

// Mathematical absolute value
static int xabs(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  MemObject* object = *p_inter->GetStackPointer();

  if(object->m_type == DTYPE_INTEGER)
  {
    int n = p_inter->GetIntegerArgument(0);
    p_inter->SetInteger(n < 0 ? -n : n);
  }
  if(object->m_type == DTYPE_BCD)
  {
    bcd n = p_inter->GetBcdArgument(0);
    p_inter->SetBcd(n.AbsoluteValue());
  }
  else
  {
    p_inter->CheckType(0,DTYPE_BCD,DTYPE_INTEGER);
  }
  return 0;
}

static int xround(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,2);
  MemObject** sp = p_inter->GetStackPointer();

  p_inter->CheckType(1,DTYPE_BCD);
  p_inter->CheckType(0,DTYPE_INTEGER);

  bcd number = *sp[1]->m_value.v_floating;
  int rnd    =  sp[0]->m_value.v_integer;

  number.Round(rnd);
  p_inter->SetBcd(number);

  return 0;
}

// newdbs([database[,user[,password]])
//
static int xnewdbase(QLInterpreter* p_inter,int argc)
{
  MemObject** sp = p_inter->GetStackPointer();
  QLVirtualMachine* vm = p_inter->GetVirtualMachine();

  if(argc > 3)
  {
    vm->Error(_T("Too many arguments"));
    return (FALSE);
  }
  CString database(db_database);
  CString user    (db_user);
  CString password(db_password);

  // All arguments must be strings
  if(argc >= 1 && database.IsEmpty()) 
  {
    p_inter->CheckType(0,DTYPE_STRING);
    CString name = *sp[argc - 1]->m_value.v_string;
    if(!name.IsEmpty())
    {
      database = name;
    }
  }
  if(argc >= 2 && user.IsEmpty()) 
  {
    p_inter->CheckType(1,DTYPE_STRING);
    CString name = *sp[argc - 2]->m_value.v_string;
    if (!name.IsEmpty())
    {
      user = name;
    }
  }
  if(argc == 3 && password.IsEmpty()) 
  {
    p_inter->CheckType(2,DTYPE_STRING);
    CString name = *sp[0]->m_value.v_string;
    if (!name.IsEmpty())
    {
      password = name;
    }
  }
  MemObject* object = vm->AllocMemObject(DTYPE_DATABASE);

  if(!database.IsEmpty())
  {
    // Build the connect string
    CString connect;
    connect.Format(_T("DSN=%s;UID=%s;PWD=%s"),database,user,password);

    try
    {
      object->m_value.v_database->Open(connect);
    }
    catch(CString& s)
    {
      vm->Info(_T("Open SQL database: %s"),s);
    }
  }
  else
  {
    vm->Info(_T("To open a ODBC database, you must at least supply a database name!"));
  }
  // Return SQLDatabase on the stack
  sp[0] = object;

  // ready
  return 0;
}

static int xnewquery(QLInterpreter* p_inter,INT argc)
{
  argcount(p_inter,argc,1);
  MemObject** sp = p_inter->GetStackPointer();
  QLVirtualMachine* vm = p_inter->GetVirtualMachine();

  p_inter->CheckType(0,DTYPE_DATABASE);

  MemObject* object = vm->AllocMemObject(DTYPE_QUERY);
  object->m_value.v_query->Init(sp[0]->m_value.v_database);

  // Put query on stack
  sp[0] = object;

  return 0;
}

static int xdbsIsOpen(QLInterpreter* p_inter, int argc) 
{
  argcount(p_inter,argc,0);
  p_inter->CheckType(1,DTYPE_DATABASE);
  MemObject**  sp  = p_inter->GetStackPointer();
  SQLDatabase* dbs = sp[1]->m_value.v_database;
  int isopen = dbs->IsOpen();
  p_inter->SetInteger(isopen);

  return 0;
}

static int xdbsClose(QLInterpreter* p_inter, int argc) 
{
  argcount(p_inter,argc,0);
  p_inter->CheckType(1,DTYPE_DATABASE);

  MemObject**  sp  = p_inter->GetStackPointer();
  SQLDatabase* dbs = sp[1]->m_value.v_database;
  dbs->Close();
  // Always successful
  p_inter->SetInteger(1);
  return 0;
}

static int xdbsTrans(QLInterpreter* p_inter,int argc)
{
  int result = 0;
  argcount(p_inter,argc,0);
  p_inter->CheckType(1,DTYPE_DATABASE);

  MemObject**  sp  = p_inter->GetStackPointer();
  SQLDatabase* dbs = sp[1]->m_value.v_database;

  SQLTransaction* trans = new SQLTransaction(dbs,_T("QL"));
  if(trans)
  {
    QLVirtualMachine* vm = p_inter->GetVirtualMachine();
    result = vm->SetSQLTransaction(trans);
  }
  p_inter->SetInteger(result);
  return 0;
}

static int xdbsCommit(QLInterpreter* p_inter,int argc)
{
  int result = 0;
  argcount(p_inter,argc,0);
  p_inter->CheckType(1,DTYPE_DATABASE);

  MemObject**  sp  = p_inter->GetStackPointer();
  SQLDatabase* dbs = sp[1]->m_value.v_database;

  QLVirtualMachine* vm = p_inter->GetVirtualMachine();
  SQLTransaction* trans = vm->GetSQLTransaction();
  if(trans)
  {
    try
    {
      dbs->CommitTransaction(trans);
      vm->SetSQLTransaction(nullptr);
      result = 1;
    }
    catch(CString& error)
    {
      vm->Info(_T("Transaction error: %s"),error);
    }
  }
  // Result of the commit
  p_inter->SetInteger(result);
  return 0;
}

static int xqryClose(QLInterpreter* p_inter, int argc) 
{
  argcount(p_inter,argc,0);
  p_inter->CheckType(1,DTYPE_QUERY);
  MemObject** sp = p_inter->GetStackPointer();
  QLVirtualMachine* vm = p_inter->GetVirtualMachine();
  
  SQLQuery* qry = sp[1]->m_value.v_query;
  qry->Close();
  // Always successful
  p_inter->SetInteger(1);
  return 0;
}

static int xqryDoSQL(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  MemObject** sp = p_inter->GetStackPointer();
  QLVirtualMachine* vm = p_inter->GetVirtualMachine();

  int    result = 0;
  SQLQuery* qry =  sp[argc + 1]->m_value.v_query;
  CString text  = *sp[argc - 1]->m_value.v_string;
  try
  {
    qry->DoSQLStatement(text);
    result = 1;
  }
  catch(CString& s)
  {
    vm->Info(_T("SQL error: %s"),s);
    return 0;
  }
  p_inter->SetInteger(result);
  return 1;
}

static int xqryScalar(QLInterpreter* p_inter,int argc)
{
  if(xqryDoSQL(p_inter,argc))
  {
    MemObject** sp = p_inter->GetStackPointer();
    SQLQuery* qry = sp[argc + 1]->m_value.v_query;
    if(qry->GetRecord())
    {
      SQLVariant* var = qry->GetColumn(1);
      p_inter->SetVariant(var);
      return 1;
    }
  }
  return 0;
}

int xqryRecord(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,0);
  p_inter->CheckType(1,DTYPE_QUERY);
  MemObject** sp = p_inter->GetStackPointer();
  QLVirtualMachine* vm = p_inter->GetVirtualMachine();

  SQLQuery* qry = sp[1]->m_value.v_query;
  int result = 0;

  try
  {
    result = qry->GetRecord();
  }
  catch(CString& s)
  {
    vm->Info(_T("SQL error: %s"),s);
  }
  p_inter->SetInteger(result);
  return 0;
}

static int xqryColumn(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  p_inter->CheckType(0,DTYPE_INTEGER);
  p_inter->CheckType(2,DTYPE_QUERY);
  MemObject** sp = p_inter->GetStackPointer();
  QLVirtualMachine* vm = p_inter->GetVirtualMachine();

  SQLQuery* qry = sp[2]->m_value.v_query;
  int    column = sp[0]->m_value.v_integer;

  MemObject* result = vm->AllocMemObject(DTYPE_NIL);
  SQLVariant* variant = qry->GetColumn(column);
  if(variant)
  {
    result->m_value.v_variant = new SQLVariant(variant);
    result->m_type  = DTYPE_VARIANT;
    result->m_flags = FLAG_DEALLOC;
  }
  sp[0] = result;
  return 0;
}

static int xqryColType(QLInterpreter* p_inter,int argc)
{
  int type = 0;
  argcount(p_inter,argc,1);
  p_inter->CheckType(0,DTYPE_INTEGER);
  p_inter->CheckType(2,DTYPE_QUERY);
  MemObject** sp = p_inter->GetStackPointer();

  SQLQuery* qry = sp[2]->m_value.v_query;
  int    column = sp[0]->m_value.v_integer;

  if(column >= 0 && column < qry->GetNumberOfColumns())
  {
    type = qry->GetColumnType(column);
  }
  else
  {
    p_inter->GetVirtualMachine()->Error(_T("GetColumnType: Wrong column number: %d"),column);
  }
  p_inter->SetInteger(type);
  return 0;
}

static int xqryColNumber(QLInterpreter* p_inter,int argc)
{
  int type = 0;
  argcount(p_inter,argc,1);
  p_inter->CheckType(0,DTYPE_STRING);
  p_inter->CheckType(2,DTYPE_QUERY);
  MemObject** sp = p_inter->GetStackPointer();

  SQLQuery* qry = sp[2]->m_value.v_query;
  CString name = *sp[0]->m_value.v_string;

  int number = qry->GetColumnNumber(name);
  p_inter->SetInteger(number);
  return 0;
}

static int xqryColName(QLInterpreter* p_inter,int argc)
{
  CString name;
  argcount(p_inter,argc,1);
  p_inter->CheckType(0,DTYPE_INTEGER);
  p_inter->CheckType(2,DTYPE_QUERY);
  MemObject** sp = p_inter->GetStackPointer();

  SQLQuery* qry = sp[2]->m_value.v_query;
  int    column = sp[0]->m_value.v_integer;

  if(column >= 0 && column < qry->GetNumberOfColumns())
  {
    qry->GetColumnName(column,name);
  }
  else
  {
    p_inter->GetVirtualMachine()->Error(_T("GetColumnName: Wrong column number: %d"),column);
  }
  p_inter->SetString(name);
  return 0;
}

static int xqryColLength(QLInterpreter* p_inter,int argc)
{
  int length = 0;
  argcount(p_inter,argc,1);
  p_inter->CheckType(0,DTYPE_INTEGER);
  p_inter->CheckType(2,DTYPE_QUERY);
  MemObject** sp = p_inter->GetStackPointer();

  SQLQuery* qry = sp[2]->m_value.v_query;
  int    column = sp[0]->m_value.v_integer;

  if(column >= 0 && column < qry->GetNumberOfColumns())
  {
    length = qry->GetColumnLength(column);
  }
  else
  {
    p_inter->GetVirtualMachine()->Error(_T("GetColumnLength: Wrong column number: %d"),column);
  }
  p_inter->SetInteger(length);
  return 0;
}

static int xqryColDLen(QLInterpreter* p_inter,int argc)
{
  int length = 0;
  argcount(p_inter,argc,1);
  p_inter->CheckType(0,DTYPE_INTEGER);
  p_inter->CheckType(2,DTYPE_QUERY);
  MemObject** sp = p_inter->GetStackPointer();

  SQLQuery* qry = sp[2]->m_value.v_query;
  int    column = sp[0]->m_value.v_integer;

  if(column >= 0 && column < qry->GetNumberOfColumns())
  {
    length = qry->GetColumnDisplaySize(column);
  }
  else
  {
    p_inter->GetVirtualMachine()->Error(_T("GetColumnDisplaySize: Wrong column number: %d"),column);
  }
  p_inter->SetInteger(length);
  return 0;
}

static int xqryError(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,0);
  p_inter->CheckType(1,DTYPE_QUERY);

  MemObject** sp = p_inter->GetStackPointer();
  SQLQuery*  qry = sp[1]->m_value.v_query;
  CString  error = qry->GetError();

  p_inter->SetString(error);
  return 0;
}

static int xqryNumCols(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,0);
  p_inter->CheckType(1,DTYPE_QUERY);

  MemObject** sp = p_inter->GetStackPointer();
  SQLQuery*  qry = sp[1]->m_value.v_query;
  int columns = qry->GetNumberOfColumns();

  p_inter->SetInteger(columns);
  return 0;
}

// SQLQuery.SetMaxRows
static int xqryMaxRows(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  p_inter->CheckType(0,DTYPE_INTEGER);
  p_inter->CheckType(2,DTYPE_QUERY);

  MemObject** sp = p_inter->GetStackPointer();
  SQLQuery*  qry = sp[2]->m_value.v_query;
  int    maxrows = sp[0]->m_value.v_integer;

  qry->SetMaxRows(maxrows);
  p_inter->SetInteger(1);
  return 0;
}

// SQLQuery.SetParameter(param,value)
static int xqrySetParam(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,2);
  p_inter->CheckType(1,DTYPE_INTEGER);
  p_inter->CheckType(3,DTYPE_QUERY);

  MemObject** sp = p_inter->GetStackPointer();
  SQLQuery*  qry = sp[3]->m_value.v_query;
  int     number = sp[1]->m_value.v_integer;

  switch(sp[0]->m_type)
  {
    case DTYPE_INTEGER: qry->SetParameter(number, sp[0]->m_value.v_integer);
                        break;
    case DTYPE_STRING:  qry->SetParameter(number,*sp[0]->m_value.v_string);
                        break;
    case DTYPE_BCD:     qry->SetParameter(number,*sp[0]->m_value.v_floating);
                        break;
    case DTYPE_VARIANT: qry->SetParameter(number, sp[0]->m_value.v_variant);
                        break;
    default:            p_inter->BadType(0,sp[0]->m_type);
                        break;
  }
  p_inter->SetInteger(1);
  return 0;
}

static int xqryIsNull(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  p_inter->CheckType(0,DTYPE_INTEGER);
  p_inter->CheckType(2,DTYPE_QUERY);

  MemObject** sp = p_inter->GetStackPointer();
  SQLQuery*  qry = sp[2]->m_value.v_query;
  int     number = sp[0]->m_value.v_integer;

  p_inter->SetInteger(qry->IsNull(number));
  return 0;
}

static int xqryIsEmpty(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  p_inter->CheckType(1,DTYPE_INTEGER);
  p_inter->CheckType(2,DTYPE_QUERY);

  MemObject** sp = p_inter->GetStackPointer();
  SQLQuery*  qry = sp[2]->m_value.v_query;
  int     number = sp[0]->m_value.v_integer;

  p_inter->SetInteger(qry->IsEmpty(number));
  return 0;
}

static int xstrIndex(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  p_inter->CheckType(0,DTYPE_INTEGER);
  p_inter->CheckType(2,DTYPE_STRING);
  MemObject** sp = p_inter->GetStackPointer();

  CString* string = sp[2]->m_value.v_string;
  int      index  = sp[0]->m_value.v_integer;

  int result = 0;
  if(index >= 0 && index < string->GetLength())
  {
    result = string->GetAt(index);
  }
  else if(index > -string->GetLength())
  {
    result = string->GetAt(string->GetLength() + index);
  }
  p_inter->SetInteger(result);
  return 0;
}

// Perform a string.find(string) OR
// Perform a string.find(ch)
static int xstrFind(QLInterpreter* p_inter,int argc)
{
  // Expected position
  int position = -1;
  int starting = 0;
  int argument = 0;
  CString* string = nullptr;
  CString* find   = nullptr;

  MemObject** sp = p_inter->GetStackPointer();
  if(argc == 2)
  {
    p_inter->CheckType(0,DTYPE_INTEGER);
    p_inter->CheckType(1,DTYPE_STRING,DTYPE_INTEGER);
    p_inter->CheckType(3,DTYPE_STRING);

    // Where we search in
    starting = sp[0]->m_value.v_integer;
    string   = sp[3]->m_value.v_string;
    argument = 1;
  }
  else if(argc == 1)
  {
    p_inter->CheckType(0,DTYPE_STRING,DTYPE_INTEGER);
    p_inter->CheckType(2,DTYPE_STRING);

    // Where we search in
    starting = 0;
    string   = sp[2]->m_value.v_string;
    argument = 0;
  }
  else
  {
    p_inter->GetVirtualMachine()->Error(_T("Wrong number of arguments"));
  }

  // Finding either string or char
  if(sp[argument]->m_type == DTYPE_STRING)
  {
    // We search for a string
    CString* find = sp[argument]->m_value.v_string;
    position = string->Find(*find,starting);
  }
  else
  {
    // it's a character we search
    int ch = sp[argument]->m_value.v_integer;
    position = string->Find(ch,starting);
  }
  p_inter->SetInteger(position);
  return 0;
}

// Do the string.size() method 
static int xstrSize(QLInterpreter* p_inter,int p_argc)
{
  argcount(p_inter,p_argc,0);
  p_inter->CheckType(1,DTYPE_STRING);
  int size = p_inter->GetStringArgument(1).GetLength();
  p_inter->SetInteger(size);
  return 0;
}

static int xstrLeft(QLInterpreter* p_inter,int p_argc)
{
  argcount(p_inter,p_argc,1);
  p_inter->CheckType(0,DTYPE_INTEGER);
  p_inter->CheckType(2,DTYPE_STRING);

  MemObject** sp = p_inter->GetStackPointer();
  int     length = sp[0]->m_value.v_integer;
  CString*   str = sp[2]->m_value.v_string;
  CString result = str->Left(length);

  p_inter->SetString(result);
  return 0;
}

static int xstrRight(QLInterpreter* p_inter,int p_argc)
{
  argcount(p_inter,p_argc,1);
  p_inter->CheckType(0,DTYPE_INTEGER);
  p_inter->CheckType(2,DTYPE_STRING);

  MemObject** sp = p_inter->GetStackPointer();
  int     length = sp[0]->m_value.v_integer;
  CString*   str = sp[2]->m_value.v_string;
  CString result = str->Right(length);

  p_inter->SetString(result);
  return 0;
}

// Return a substring from a string
static int xstrSubstring(QLInterpreter* p_inter,int p_argc)
{
  MemObject** sp = p_inter->GetStackPointer();
  CString* string;
  int start  = 0;
  int length = 0;

  if(p_argc == 1)
  {
    p_inter->CheckType(0,DTYPE_STRING,DTYPE_INTEGER);
    p_inter->CheckType(2,DTYPE_STRING);

    string = sp[2]->m_value.v_string;
    start  = sp[0]->m_value.v_integer;
    length = string->GetLength();

  }
  else if(p_argc == 2)
  {
    p_inter->CheckType(0,DTYPE_INTEGER);
    p_inter->CheckType(1,DTYPE_INTEGER);
    p_inter->CheckType(3,DTYPE_STRING);

    string = sp[3]->m_value.v_string;
    start  = sp[1]->m_value.v_integer;
    length = sp[0]->m_value.v_integer;
  }
  else
  {
    p_inter->GetVirtualMachine()->Error(_T("Wrong number of arguments"));
  }
  CString result = string->Mid(start,length);

  // Setting the result
  p_inter->SetString(result);
  return 0;
}

// Do the string.makeupper() method 
static int xstrUpper(QLInterpreter* p_inter,int p_argc)
{
  argcount(p_inter,p_argc,0);
  p_inter->CheckType(1,DTYPE_STRING);
  CString string = p_inter->GetStringArgument(1);
  string.MakeUpper();
  p_inter->SetString(string);
  return 0;
}

// Do the string.makelower() method 
static int xstrLower(QLInterpreter* p_inter,int p_argc)
{
  argcount(p_inter,p_argc,0);
  p_inter->CheckType(1,DTYPE_STRING);
  CString string = p_inter->GetStringArgument(1);
  string.MakeLower();
  p_inter->SetString(string);
  return 0;
}

static int xsleep(QLInterpreter* p_inter,int p_argc)
{
  argcount(p_inter,p_argc,1);
  p_inter->CheckType(0,DTYPE_INTEGER);
  int wait = p_inter->GetIntegerArgument(0);
  if(wait)
  {
    Sleep(wait);
  }
  return 0;
}

// Function for outside test framework

// Do the "TestIterations" function
static int xtestit(QLInterpreter* p_inter,int p_argc)
{
  argcount(p_inter,p_argc,0);
  p_inter->SetInteger(p_inter->GetTestInterations());
  return 0;
}

// Do the "TestResult" function
static int xtestres(QLInterpreter* p_inter,int p_argc)
{
  argcount(p_inter,p_argc,0);
  p_inter->SetInteger(p_inter->GetTestResult());
  return 0;
}

// Do the "TestRunning(1)" function
static int xtestrun(QLInterpreter* p_inter,int p_argc)
{
  argcount(p_inter,p_argc,1);
  int running = p_inter->GetIntegerArgument(0);
  p_inter->SetInteger(running);
  p_inter->SetTestRunning(running);
  return 0;
}

/////////////////////////////////////////////////////////////////////
//
// Now we can do the INIT of all functions
//
/////////////////////////////////////////////////////////////////////

//  Initialize the internal functions
void init_functions(QLVirtualMachine* p_vm)
{
  // Adding default streams
  add_file(_T("stdin"), stdin,  p_vm);
  add_file(_T("stdout"),stdout, p_vm);
  add_file(_T("stderr"),stderr, p_vm);

  // Adding default functions
  add_function(_T("typeof"),    xtypeof,      p_vm);
  add_function(_T("newarray"),  xnewarray,    p_vm);
  add_function(_T("newstring"), xnewstring,   p_vm);
  add_function(_T("newdbase"),  xnewdbase,    p_vm);
  add_function(_T("newquery"),  xnewquery,    p_vm);
  add_function(_T("sizeof"),    xsizeof,      p_vm);
  add_function(_T("trace"),     xtrace,       p_vm);
  add_function(_T("fopen"),     xfopen,       p_vm);
  add_function(_T("fclose"),    xfclose,      p_vm);
  add_function(_T("getc"),      xgetc,        p_vm);
  add_function(_T("putc"),      xputc,        p_vm);
  add_function(_T("gets"),      xgets,        p_vm);
  add_function(_T("puts"),      xputs,        p_vm);
  add_function(_T("print"),     xprint,       p_vm);
  add_function(_T("fprint"),    xfprint,      p_vm);
  add_function(_T("getarg"),    xgetarg,      p_vm);
  add_function(_T("system"),    xsystem,      p_vm);
  add_function(_T("exit"),      xexit,        p_vm);
  add_function(_T("gc"),        xgc,          p_vm);
  add_function(_T("sin"),       xsin,         p_vm);
  add_function(_T("cos"),       xcos,         p_vm);
  add_function(_T("tan"),       xtan,         p_vm);
  add_function(_T("asin"),      xasin,        p_vm);
  add_function(_T("acos"),      xacos,        p_vm);
  add_function(_T("atan"),      xatan,        p_vm);
  add_function(_T("sqrt"),      xsqrt,        p_vm);
  add_function(_T("ceil"),      xceil,        p_vm);
  add_function(_T("floor"),     xfloor,       p_vm);
  add_function(_T("exp"),       xexp,         p_vm);
  add_function(_T("log"),       xlog,         p_vm);
  add_function(_T("logn"),      xlogn,        p_vm);
  add_function(_T("log10"),     xlog10,       p_vm);
  add_function(_T("pow"),       xpow,         p_vm);
  add_function(_T("rand"),      xrand,        p_vm);
  add_function(_T("tobcd"),     xtobcd,       p_vm);
  add_function(_T("toint"),     xtoint,       p_vm);
  add_function(_T("tostring"),  xtostr,       p_vm);
  add_function(_T("tovariant"), xtovariant,   p_vm);
  add_function(_T("abs"),       xabs,         p_vm);
  add_function(_T("round"),     xround,       p_vm);
  add_function(_T("Sleep"),     xsleep,       p_vm);

  // Function for outside test framework
  add_function(_T("TestIterations"),xtestit,  p_vm);
  add_function(_T("TestResult"),    xtestres, p_vm);
  add_function(_T("TestRunning"),   xtestrun, p_vm);

  // Add all object methods
  add_method(DTYPE_DATABASE, _T("IsOpen"),                xdbsIsOpen,   p_vm);
  add_method(DTYPE_DATABASE, _T("Close"),                 xdbsClose,    p_vm);
  add_method(DTYPE_DATABASE, _T("StartTransaction"),      xdbsTrans,    p_vm);
  add_method(DTYPE_DATABASE, _T("Commit"),                xdbsCommit,   p_vm);
  // QUERY methods
  add_method(DTYPE_QUERY,    _T("Close"),                 xqryClose,    p_vm);
  add_method(DTYPE_QUERY,    _T("DoSQLStatement"),        xqryDoSQL,    p_vm);
  add_method(DTYPE_QUERY,    _T("DoSQLScalar"),           xqryScalar,   p_vm);
  add_method(DTYPE_QUERY,    _T("GetRecord"),             xqryRecord,   p_vm);
  add_method(DTYPE_QUERY,    _T("GetColumn"),             xqryColumn,   p_vm);
  add_method(DTYPE_QUERY,    _T("GetColumnType"),         xqryColType,  p_vm);
  add_method(DTYPE_QUERY,    _T("GetColumnNumber"),       xqryColNumber,p_vm);
  add_method(DTYPE_QUERY,    _T("GetColumnName"),         xqryColName,  p_vm);
  add_method(DTYPE_QUERY,    _T("GetColumnLength"),       xqryColLength,p_vm);
  add_method(DTYPE_QUERY,    _T("GetColumnDisplayLength"),xqryColDLen,  p_vm);
  add_method(DTYPE_QUERY,    _T("GetError"),              xqryError,    p_vm);
  add_method(DTYPE_QUERY,    _T("GetNumberOfColumns"),    xqryNumCols,  p_vm);
  add_method(DTYPE_QUERY,    _T("SetMaxRows"),            xqryMaxRows,  p_vm);
  add_method(DTYPE_QUERY,    _T("SetParameter"),          xqrySetParam, p_vm);
  add_method(DTYPE_QUERY,    _T("IsNull"),                xqryIsNull,   p_vm);
  add_method(DTYPE_QUERY,    _T("IsEmpty"),               xqryIsEmpty,  p_vm);
  // STRING METHODS
  add_method(DTYPE_STRING,   _T("index"),                 xstrIndex,    p_vm);
  add_method(DTYPE_STRING,   _T("find"),                  xstrFind,     p_vm);
  add_method(DTYPE_STRING,   _T("size"),                  xstrSize,     p_vm);
  add_method(DTYPE_STRING,   _T("substring"),             xstrSubstring,p_vm);
  add_method(DTYPE_STRING,   _T("left"),                  xstrLeft,     p_vm);
  add_method(DTYPE_STRING,   _T("right"),                 xstrRight,    p_vm);
  add_method(DTYPE_STRING,   _T("makeupper"),             xstrUpper,    p_vm);
  add_method(DTYPE_STRING,   _T("makelower"),             xstrLower,    p_vm);

  // SQLQuery methods to implement:
  // 

  // Seed the random-number generator with the current time so that
  // the numbers will be different every time we run.
  srand((unsigned)time(NULL));
}

