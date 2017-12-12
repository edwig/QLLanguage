//////////////////////////////////////////////////////////////////////////
//
// QL Language functions
// ir. W.E. Huisman (c) 2017
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "QL_Language.h"
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
add_function(char* p_name,int (*p_fcn)(QLInterpreter*,int),QLVirtualMachine* p_vm)
{
  MemObject* sym = p_vm->AddInternal(p_name);
  sym->m_value.v_internal = p_fcn;
}

// Add a built-in file
static void 
add_file(char* p_name,FILE* p_fp,QLVirtualMachine* p_vm)
{
  MemObject* sym = p_vm->AddSymbol(p_name);
  p_vm->MemObjectSetType(sym,DTYPE_NIL);
  p_vm->MemObjectSetType(sym,DTYPE_FILE);
  sym->m_value.v_file = p_fp;
}

// Add a method for a built-in-datatype
static void 
add_method(int p_type,char* p_name,int (*p_fcn)(QLInterpreter*,int),QLVirtualMachine* p_vm)
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
    vm->Error("Too many arguments");
    return (FALSE);
  }
  else if ((n) > (cnt))
  {
    vm->Error("Too few arguments");
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
  p_inter->PushInteger(type);
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
  argcount(p_inter,argc,1);
  int size = p_inter->GetIntegerArgument(0);
  p_inter->SetString(0,size);
  return 0;
}

// Get the size of a vector or string
static int xsizeof(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  MemObject* object = *p_inter->GetStackPointer();
  switch (object->m_type) 
  {
    case DTYPE_ARRAY:   p_inter->SetInteger(1,object->m_value.v_array->GetSize());
                        break;
    case DTYPE_STRING:  p_inter->SetInteger(1,object->m_value.v_string->GetLength());
                        break;
    case DTYPE_BCD:     p_inter->SetInteger(1,sizeof(bcd));
                        break;
    case DTYPE_INTEGER: p_inter->SetInteger(1,sizeof(int));
                        break;
    case DTYPE_DATABASE:p_inter->SetInteger(1,sizeof(SQLDatabase));
                        break;
    case DTYPE_QUERY:   p_inter->SetInteger(1,sizeof(SQLQuery));
                        break;
    case DTYPE_VARIANT: p_inter->SetInteger(1,sizeof(SQLVariant));
                        break;
    default:            p_inter->SetInteger(1,0);
                        break;
  }
  p_inter->IncrementStackPointer();
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

  fopen_s(&fp,fileName,mode);
  if (fp == NULL)
  {
    p_inter->SetNil(0);
  }
  else
  {
    p_inter->SetFile(0,fp);
  }
  return 0;
}

// Close a file
static int xfclose(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  FILE* fp = p_inter->GetStackPointer()[0]->m_value.v_file;
  p_inter->SetInteger(0,fclose(fp));
  return 0;
}

// Get a character from a file
static int  xgetc(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  p_inter->CheckType(0,DTYPE_FILE);
  FILE* fp = p_inter->GetStackPointer()[0]->m_value.v_file;
  p_inter->SetInteger(0,getc(fp));
  return 0;
}

// Output a character to a file
static int xputc(QLInterpreter* p_inter,int argc)
{
  MemObject** sp = p_inter->GetStackPointer();

  argcount(p_inter,argc,2);
  FILE* fp = sp[0]->m_value.v_file;
  int   cc = p_inter->GetIntegerArgument(1);
  p_inter->SetInteger(0,putc(cc,fp));
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
  while((cc = getc(fp)) != EOF && cc != '\n')
  {
    s.Append((const char*) &cc);
  }
  p_inter->SetString(0,0);
  *(sp[0]->m_value.v_string) = s;
  return 0;
}

static int xputs(QLInterpreter* p_inter,int argc)
{
  MemObject** sp = p_inter->GetStackPointer();

  argcount(p_inter,argc,2);
  FILE* fp = sp[0]->m_value.v_file;
  CString* str = sp[1]->m_value.v_string;
  p_inter->SetInteger(0,fputs(*str,fp));
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
  // sp += argc;
  p_inter->IncrementStackPointer(argc);
  // total chars printed
  p_inter->SetInteger(0,len);

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

  for (int n = argc; --n >= 1; )
  {
    len += vm->Print(file,FALSE,sp[n]);
  }
  p_inter->IncrementStackPointer(argc);
  p_inter->SetInteger(0,len);

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
    p_inter->SetString(0,0);
    *(sp[0]->m_value.v_string) = CString(qlargv[index]);
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
  p_inter->SetInteger(0,system(command));
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
#ifdef _DEBUG
  TRACE("GARBAGE COLLECTION\n");
#endif
  p_inter->GetVirtualMachine()->GC();
#ifdef _DEBUG
  TRACE("GARBAGE COLLECTION DONE\n");
#endif
  p_inter->SetNil(0);
  return 0;
}

// Trigonometric Sine
static int xsin(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd(0,val.Sine());
  return 0;
}

// Trigonometric Cosine
static int xcos(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd(0, val.Cosine());
  return 0;
}

// Trigonometric Tangent
static int xtan(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd(0, val.Tangent());
  return 0;
}

// Trigonometric Arcsine
static int xasin(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd(0, val.ArcSine());
  return 0;
}

// Trigonometric Arccosine
static int xacos(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd(0, val.ArcCosine());
  return 0;
}

// Trigonometric Arctangent
static int xatan(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd(0, val.ArcTangent());
  return 0;
}

// Mathematical square root
static int xsqrt(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd(0, val.SquareRoot());
  return 0;
}

// Mathematical ceiling
static int xceil(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd(0, val.Ceiling());
  return 0;
}

// Mathematical floor
static int xfloor(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd(0, val.Floor());
  return 0;
}

// Mathematical exponent
static int xexp(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd(0, val.Exp());
  return 0;
}

// Mathematical 10 base logarithm
static int xlog(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd(0, val.Log10());
  return 0;
}

// Mathematical natural logarithm
static int xlogn(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd(0, val.Log());
  return 0;
}

// Mathematical 10 base logarithm
static int xlog10(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter, argc, 1);
  bcd val = p_inter->GetBcdArgument(0);
  p_inter->SetBcd(0, val.Log10());
  return 0;
}

// Mathematical power
static int xpow(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,2);
  bcd val = p_inter->GetBcdArgument(0);
  bcd pow = p_inter->GetBcdArgument(1);
  p_inter->IncrementStackPointer();
  p_inter->SetBcd(0, val.Power(pow));
  return 0;
}

// Mathematical random number
static int xrand(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,0);
  int rr = rand();
  bcd val = bcd(rr) / bcd((long)RAND_MAX);
  QLVirtualMachine* vm = p_inter->GetVirtualMachine();
  MemObject** sp = p_inter->PushInteger(0);
  sp[0]->m_type = DTYPE_INTEGER;
  sp[0]->m_value.v_floating = new bcd(val);
  sp[0]->m_flags |= FLAG_DEALLOC;
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
    p_inter->SetBcd(0,val);
  }
  else if(object->m_type == DTYPE_STRING)
  {
    CString str = p_inter->GetStringArgument(0);
    bcd val(str);
    p_inter->SetBcd(0,val);
  }
  else if(object->m_type == DTYPE_VARIANT)
  {
    bcd val = p_inter->GetSQLVariantArgument(0)->GetAsBCD();
    p_inter->SetBcd(0,val);
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
    p_inter->SetInteger(0,val.AsLong());
  }
  else if(object->m_type == DTYPE_STRING)
  {
    CString str = p_inter->GetStringArgument(0);
    p_inter->SetInteger(0,atoi(str));
  }
  else if(object->m_type == DTYPE_VARIANT)
  {
    int val = p_inter->GetSQLVariantArgument(0)->GetAsSLong();
    p_inter->SetInteger(0,val);
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
    str.Format("%d",n);
    p_inter->SetString(0,0);
    *(object->m_value.v_string) = str;
  }
  else if(object->m_type == DTYPE_BCD)
  {
    bcd n = p_inter->GetBcdArgument(0);
    CString val = n.AsString();
    p_inter->SetString(0,0);
    *(object->m_value.v_string) = val;
  }
  else if(object->m_type == DTYPE_VARIANT)
  {
    CString str(p_inter->GetSQLVariantArgument(0)->GetAsChar());
    p_inter->SetString(0,0);
    *(object->m_value.v_string) = str;
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

// Mathematical absolute value
static int xabs(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  MemObject* object = *p_inter->GetStackPointer();

  if(object->m_type == DTYPE_INTEGER)
  {
    int n = p_inter->GetIntegerArgument(0);
    p_inter->SetInteger(0,n < 0 ? -n : n);
  }
  if(object->m_type == DTYPE_BCD)
  {
    bcd n = p_inter->GetBcdArgument(0);
    p_inter->SetBcd(0,n.AbsoluteValue());
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
  p_inter->IncrementStackPointer(1);
  p_inter->SetBcd(0,number);

  return 0;
}

// newdbs([database[,user[,password]])
//
static int xnewdbs(QLInterpreter* p_inter,int argc)
{
  MemObject** sp = p_inter->GetStackPointer();
  QLVirtualMachine* vm = p_inter->GetVirtualMachine();

  if(argc > 3)
  {
    vm->Error("Too many arguments");
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
    connect.Format("DSN=%s;UID=%s;PWD=%s",database,user,password);

    try
    {
      object->m_value.v_database->Open(connect);
    }
    catch(CString& s)
    {
      vm->Info("Open SQL database: %s",s);
    }
  }
  else
  {
    vm->Info("To open a ODBC database, you must at least supply a database name!");
  }
  // Increment the stack pointer
  if(argc > 0)
  {
    p_inter->IncrementStackPointer(argc - 1);
    sp = p_inter->GetStackPointer();
  }
  // Return SQLDatabase on stack
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
  sp[1] = object;
  p_inter->IncrementStackPointer();

  return 0;
}

static int xdbsIsOpen(QLInterpreter* p_inter, int argc) 
{
  argcount(p_inter,argc,0);
  p_inter->CheckType(1,DTYPE_DATABASE);
  MemObject** sp = p_inter->GetStackPointer();
  SQLDatabase* dbs = sp[1]->m_value.v_database;
  int isopen = dbs->IsOpen();
  p_inter->SetInteger(0,isopen);

  return 0;
}

static int xdbsClose(QLInterpreter* p_inter, int argc) 
{
  argcount(p_inter,argc,0);
  p_inter->CheckType(1,DTYPE_DATABASE);
  MemObject** sp = p_inter->GetStackPointer();
  QLVirtualMachine* vm = p_inter->GetVirtualMachine();

  SQLDatabase* dbs = sp[1]->m_value.v_database;
  dbs->Close();
  // Always successful
  p_inter->SetInteger(0,1);
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
  p_inter->SetInteger(0,1);
  return 0;
}

static int xqryDoSQL(QLInterpreter* p_inter,int argc)
{
  argcount(p_inter,argc,1);
  MemObject** sp = p_inter->GetStackPointer();
  QLVirtualMachine* vm = p_inter->GetVirtualMachine();

  int    result = 0;
  SQLQuery* qry = sp[argc + 1]->m_value.v_query;
  CString text  = *sp[0]->m_value.v_string;
  try
  {
    qry->DoSQLStatement(text);
    result = 1;
  }
  catch(CString& s)
  {
    vm->Info("SQL error: %s",s);
  }
  p_inter->IncrementStackPointer();
  p_inter->SetInteger(0,result);
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
    vm->Info("SQL error: %s",s);
  }
  p_inter->SetInteger(0,result);
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
  p_inter->IncrementStackPointer();
  sp[1] = result;
  return 0;
}


int xstrIndex(QLInterpreter* p_inter,int argc)
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
  p_inter->SetInteger(0,result);
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
    p_inter->GetVirtualMachine()->Error("Wrong number of arguments");
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
  p_inter->SetInteger(0,position);
  return 0;
}

// Do the string.size() method 
static int xstrSize(QLInterpreter* p_inter,int p_argc)
{
  argcount(p_inter,p_argc,0);
  p_inter->CheckType(1,DTYPE_STRING);
  int size = p_inter->GetStringArgument(1).GetLength();
  p_inter->SetInteger(0,size);
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

  p_inter->SetString(0,0);
  (*sp[0]->m_value.v_string) = result;

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

  p_inter->SetString(0,0);
  (*sp[0]->m_value.v_string) = result;
  
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
    p_inter->GetVirtualMachine()->Error("Wrong number of arguments");
  }
  CString result = string->Mid(start,length);

  // Setting the result
  p_inter->SetString(0,0);
  *(sp[0]->m_value.v_string) = result;
  return 0;
}

// Do the string.size() method 
static int xstrUpper(QLInterpreter* p_inter,int p_argc)
{
  argcount(p_inter,p_argc,0);
  p_inter->CheckType(1,DTYPE_STRING);
  CString string = p_inter->GetStringArgument(1);
  string.MakeUpper();
  p_inter->SetString(0,0);

  MemObject** sp = p_inter->GetStackPointer();
  (*sp[0]->m_value.v_string) = string;
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
  add_file("stdin", stdin,  p_vm);
  add_file("stdout",stdout, p_vm);
  add_file("stderr",stderr, p_vm);

  // Adding default functions
  add_function("typeof",    xtypeof,      p_vm);
  add_function("newarray",  xnewarray,    p_vm);
  add_function("newstring", xnewstring,   p_vm);
  add_function("newdbs",    xnewdbs,      p_vm);
  add_function("newquery",  xnewquery,    p_vm);
  add_function("sizeof",    xsizeof,      p_vm);
  add_function("trace",     xtrace,       p_vm);
  add_function("fopen",     xfopen,       p_vm);
  add_function("fclose",    xfclose,      p_vm);
  add_function("getc",      xgetc,        p_vm);
  add_function("putc",      xputc,        p_vm);
  add_function("gets",      xgets,        p_vm);
  add_function("puts",      xputs,        p_vm);
  add_function("print",     xprint,       p_vm);
  add_function("fprint",    xfprint,      p_vm);
  add_function("getarg",    xgetarg,      p_vm);
  add_function("system",    xsystem,      p_vm);
  add_function("exit",      xexit,        p_vm);
  add_function("gc",        xgc,          p_vm);
  add_function("sin",       xsin,         p_vm);
  add_function("cos",       xcos,         p_vm);
  add_function("tan",       xtan,         p_vm);
  add_function("asin",      xasin,        p_vm);
  add_function("acos",      xacos,        p_vm);
  add_function("atan",      xatan,        p_vm);
  add_function("sqrt",      xsqrt,        p_vm);
  add_function("ceil",      xceil,        p_vm);
  add_function("floor",     xfloor,       p_vm);
  add_function("exp",       xexp,         p_vm);
  add_function("log",       xlog,         p_vm);
  add_function("logn",      xlogn,        p_vm);
  add_function("log10",     xlog10,       p_vm);
  add_function("pow",       xpow,         p_vm);
  add_function("rand",      xrand,        p_vm);
  add_function("tobcd",     xtobcd,       p_vm);
  add_function("toint",     xtoint,       p_vm);
  add_function("tostring",  xtostr,       p_vm);
  add_function("abs",       xabs,         p_vm);
  add_function("round",     xround,       p_vm);

  // Add all datatype methods
  add_method(DTYPE_DATABASE, "IsOpen",        xdbsIsOpen,   p_vm);
  add_method(DTYPE_DATABASE, "Close",         xdbsClose,    p_vm);
  add_method(DTYPE_QUERY,    "Close",         xqryClose,    p_vm);
  add_method(DTYPE_QUERY,    "DoSQLStatement",xqryDoSQL,    p_vm);
  add_method(DTYPE_QUERY,    "GetRecord",     xqryRecord,   p_vm);
  add_method(DTYPE_QUERY,    "GetColumn",     xqryColumn,   p_vm);
  add_method(DTYPE_STRING,   "index",         xstrIndex,    p_vm);
  add_method(DTYPE_STRING,   "find",          xstrFind,     p_vm);
  add_method(DTYPE_STRING,   "size",          xstrSize,     p_vm);
  add_method(DTYPE_STRING,   "substring",     xstrSubstring,p_vm);
  add_method(DTYPE_STRING,   "left",          xstrLeft,     p_vm);
  add_method(DTYPE_STRING,   "right",         xstrRight,    p_vm);
  add_method(DTYPE_STRING,   "makeupper",     xstrUpper,    p_vm);

  // Seed the random-number generator with the current time so that
  // the numbers will be different every time we run.
  srand((unsigned)time(NULL));
}

