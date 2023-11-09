//////////////////////////////////////////////////////////////////////////
// 
// QL Language Virtual Machine executing the bytecode
// ir. W.E. Huisman
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "QL_Language.h"
#include "QL_MemObject.h"
#include "QL_vm.h"
#include "QL_Exception.h"
#include "QL_Objects.h"
#include "QL_Interpreter.h"
#include "QL_Functions.h"
#include "QL_Compiler.h"
#include "QL_Debugger.h"
#include "QL_Opcodes.h"
#include "bcd.h"
#include <stdarg.h>
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Globals for the QL language are defined here
int    qlargc = 0;
char** qlargv = NULL;

CString db_database;
CString db_user;
CString db_password;

QLVirtualMachine::QLVirtualMachine()
{
  m_root_object   = nullptr;
  m_last_object   = nullptr;
  m_interpreter   = nullptr;
  m_globals       = nullptr;
  m_literals      = nullptr;
  m_initcode      = nullptr;
  m_transaction   = nullptr;
  m_threshold     = THRESHOLD_DEFAULT;
  m_dumpchain     = false;
  m_allocs        = 0;
  m_position      = 0;
  m_initcode_size = 0;

  InitializeCriticalSection(&m_lock);
}

QLVirtualMachine::~QLVirtualMachine()
{
  DestroyObjectChain();
  CleanUpClasses();
  CleanUpGlobals();
  CleanUpLiterals();
  CleanUpMethods();
  CleanUpInitcode();

  DeleteCriticalSection(&m_lock);
}

void
QLVirtualMachine::CheckInit()
{
  // Initialize the SQLComponents to the English language
  InitSQLComponents();

  // See if the gc object chain has been initialized
  if(m_root_object == nullptr)
  {
    // Very special call to AllocMemObject. VM not initialized yet
    m_root_object = AllocMemObject(DTYPE_ENDMARK, false);
    m_last_object = AllocMemObject(DTYPE_ENDMARK,false);
    m_root_object->m_next = m_last_object;
    m_last_object->m_prev = m_root_object;

    init_functions(this);

    m_globals  = new Array();
    m_literals = new Array();
  }
}

// Setting the alloc threshold for the GC
void        
QLVirtualMachine::SetGCThreshold(int p_threshold)
{
  if(p_threshold < THRESHOLD_RELAXED)
  {
    p_threshold = THRESHOLD_RELAXED;
  }
  if(p_threshold > THRESHOLD_RELAXED)
  {
    p_threshold = THRESHOLD_RELAXED;
  }
  m_threshold = p_threshold;
}

/*static*/ void
QLVirtualMachine::Error(const char* p_format,...)
{
  CString text;

  va_list  argList;
  va_start(argList, p_format);
  text.FormatV(p_format,argList);
  va_end(argList);
  fputs(text,stderr);

  // throw as an error exception
  throw QLException(text,EXCEPTION_BY_ERROR);
}

void
QLVirtualMachine::Info(const char* p_format,...)
{
  CString text;

  va_list  argList;
  va_start(argList, p_format);
  text.FormatV(p_format, argList);
  va_end(argList);
  fprintf(stdout,"[%s]\n",text.GetString());
}

//////////////////////////////////////////////////////////////////////////
//
// USE THE COMPILER
//
//////////////////////////////////////////////////////////////////////////

// Compile a QL source code file into this VM
bool        
QLVirtualMachine::CompileFile(const char* p_filename,bool p_trace)
{
  // See to it that the VM is initialized
  CheckInit();

  bool result = false;
  // compile file
  QLCompiler comp(this);
  QLDebugger* dbg  = nullptr;
  FILE*       file = nullptr;

  // See if tracing the compiler is requested
  if(p_trace)
  {
    dbg = new QLDebugger(this);
    comp.SetDebugger(dbg,1);
  }

  // check that name ends in .ql
  CString filename(p_filename);
  if(filename.Right(3).CompareNoCase(".ql"))
  {
    filename += ".ql";
  }

  fopen_s(&file,filename,"r");
  if(file)
  {
    result = comp.CompileDefinitions((int(*)(void*))fgetc,(void*)file);
    fclose(file);
  }

  // Remove the debugger again
  if(dbg)
  {
    delete dbg;
    dbg = nullptr;
  }
  return result;
}

bool
QLVirtualMachine::IsObjectFile(const char* p_filename)
{
  bool result = false;
  CString filename(p_filename);

  if(_access(p_filename,04) != 0)
  {
    filename += ".qob";
  }
  if(_access(p_filename,04) != 0)
  {
    return false;
  }
  FILE* fp = nullptr;
  fopen_s(&fp,filename,"r");
  if(fp != nullptr)
  {
    try
    {
      if(ReadHeader(fp,false))
      {
        fclose(fp);
        result = true;
      }
    }
    catch(QLException&)
    {
      result = false;
    }
    fclose(fp);
  }
  return result;
}

bool
QLVirtualMachine::IsSourceFile(const char* p_filename)
{
  CString filename(p_filename);

  if(_access(p_filename,04) == 0)
  {
    return true;
  }

  // Test for a .QL file
  if(filename.Right(3).CompareNoCase(".ql"))
  {
    filename += ".ql";
    if(_access(filename,04) == 0)
    {
      return true;
    }
  }
  return false;
}

__declspec(thread) static char* compile_buffer = nullptr;

int readbuffer(void* p_buff)
{

  char* buff = reinterpret_cast<char*>(p_buff);
  if(!compile_buffer)
  {
    compile_buffer = buff;
  }
  if(!*compile_buffer)
  {
    return EOF;
  }
  return *compile_buffer++;
}

// Compile a QL source code buffer string into this VM
bool
QLVirtualMachine::CompileBuffer(const char* p_buffer,bool p_trace)
{
  // See to it that the VM is initialized
  CheckInit();

  bool result = false;
  // compile file
  QLCompiler comp(this);
  QLDebugger* dbg = nullptr;
  FILE* file = nullptr;

  // See if tracing the compiler is requested
  if(p_trace)
  {
    dbg = new QLDebugger(this);
    comp.SetDebugger(dbg,1);
  }
  // Reset our buffer
  compile_buffer = nullptr;
  // Compile the buffered string
  result = comp.CompileDefinitions(readbuffer,(void*)p_buffer);

  // Remove the debugger again
  if(dbg)
  {
    delete dbg;
    dbg = nullptr;
  }
  return result;
}

//////////////////////////////////////////////////////////////////////////
//
// MEMORY API
//
//////////////////////////////////////////////////////////////////////////

MemObject*
QLVirtualMachine::AllocMemObject(int p_type,bool p_running /*=true*/)
{
  // Call the garbage collector every now and then!
  if((m_allocs % m_threshold) == 0)
  {
    GC();
  }

  // Create a new MemObject
  MemObject* object = new MemObject;

  // Place object in the GC chain
  if(m_last_object)
  {
    object->m_prev = m_last_object->m_prev;
    m_last_object->m_prev = object;
    object->m_next = m_last_object;
    object->m_prev->m_next = object;
  }
  else if(p_running)
  {
    Error("INTERNAL: AllocMemObject called before VM is initialized!");
  }
  // Record the flags
  object->m_generation = GC_ALIVE;
  object->m_flags     |= FLAG_NULL;

  // Make the dependent object
  MemObjectSetType(object,p_type);

// #ifdef _DEBUG
//   TRACE("Number of allocs: %d\n",m_allocs);
// #endif

  // Increment the number of allocs
  ++m_allocs;

  return object;
}

MemObject*  
QLVirtualMachine::AllocMemObject(const MemObject* p_other)
{
  // Call the garbage collector every now and then!
  if ((m_allocs % 1000) == 0)
  {
    GC();
  }

  // Create a new MemObject
  MemObject* object = new MemObject();

  // Place object in the GC chain
  if(m_last_object)
  {
    object->m_prev = m_last_object->m_prev;
    m_last_object->m_prev = object;
    object->m_next = m_last_object;
    object->m_prev->m_next = object;
  }
  // Record the flags
  object->m_generation = GC_ALIVE;
  object->m_flags |= FLAG_NULL;

  switch(p_other->m_type)
  {
    case DTYPE_NIL:       object->m_value.v_integer   = 0;
                          break;
    case DTYPE_INTEGER:   object->m_value.v_integer   = p_other->m_value.v_integer;
                          break;
    case DTYPE_STRING:    object->m_value.v_string    = new CString(*p_other->m_value.v_string);
                          break;
    case DTYPE_BCD:       object->m_value.v_floating  = new bcd(*p_other->m_value.v_floating);
                          break;
    case DTYPE_FILE:      object->m_value.v_file      = p_other->m_value.v_file;
                          break;
    case DTYPE_DATABASE:  object->m_value.v_database  = p_other->m_value.v_database;
                          object->m_flags |= FLAG_REFERENCE;
                          break;
   case DTYPE_QUERY:      object->m_value.v_query     = p_other->m_value.v_query;
                          object->m_flags |= FLAG_REFERENCE;
                          break;
    case DTYPE_VARIANT:   object->m_value.v_variant   = p_other->m_value.v_variant;
                          object->m_flags |= FLAG_REFERENCE;
                          break;
    case DTYPE_ARRAY:     // Cannot copy this object
    case DTYPE_OBJECT:    // Must be copy object
    case DTYPE_CLASS:     // error
    case DTYPE_SCRIPT:    
    case DTYPE_INTERNAL:
    case DTYPE_EXTERNAL:  Error("Cannot copy this type of object");
  }
  object->m_type = p_other->m_type;
  
// #ifdef _DEBUG
//   TRACE("Number of allocs: %d\n",m_allocs);
// #endif

  // Increment the number of allocs
  ++m_allocs;

  return object;
}


void
QLVirtualMachine::FreeMemObject(MemObject* p_object,bool p_running /*=true*/)
{
  // Delete the type data
  MemObjectSetType(p_object,DTYPE_NIL);

  if(p_running)
  {
    // If the object is not one of the endmarkers in the chain
    // readjust the prev/next chain of the other objects
    // so this object becomes free
    if(p_object->m_next && p_object->m_prev)
    {
      p_object->m_prev->m_next = p_object->m_next;
      p_object->m_next->m_prev = p_object->m_prev;
    }
  }
// #ifdef _DEBUG
//   TRACE("Freeing an alloc: %d\n",--m_allocs);
// #endif
  // Now delete the memobject
  delete p_object;
}

// Can be called with DTYPE_NIL to deallocate storage and set to NIL
// OR can be called to allocate a specific datatype
/*static*/ void
QLVirtualMachine::MemObjectSetType(MemObject* p_object,int p_type)
{
  if(p_type == DTYPE_NIL && p_object->m_type)
  {
    if(p_object->m_flags & FLAG_REFERENCE)
    {
      p_object->m_value.v_integer = 0;
    }
    else
    {
      p_object->DeAllocate();
    }
    p_object->m_type  = DTYPE_NIL;
    p_object->m_flags = 0;
  }
  else
  {
    p_object->AllocateType(p_type);
  }
}

//////////////////////////////////////////////////////////////////////////
//
// GLOBALS CLASSES SYMBOLS SCRIPTS
//
//////////////////////////////////////////////////////////////////////////

Class*
QLVirtualMachine::FindClass(CString& p_name)
{
  ClassMap::iterator it = m_classes.find(p_name);
  if(it == m_classes.end())
  {
    return nullptr;
  }
  return it->second;
}

Function*
QLVirtualMachine::AddScript(CString p_name)
{
  MemObject* entry = AddEntry(m_scripts,p_name,ST_SFUNCTION);
  if(entry->m_type == DTYPE_STRING)
  {
    entry->DeAllocate();
    entry->AllocateType(DTYPE_SCRIPT);
    entry->m_value.v_script->SetName(p_name);
  }
  return entry->m_value.v_script;
}

MemObject*  
QLVirtualMachine::AddInternal(CString p_name)
{
  MemObject* intern = AllocMemObject(DTYPE_INTERNAL);
  m_symbols.insert(std::make_pair(p_name,intern));
  return intern;
}

MemObject*
QLVirtualMachine::FindSymbol(CString p_name)
{
  NameMap::iterator it = m_symbols.find(p_name);
  if(it != m_symbols.end())
  {
    return it->second;
  }
  return nullptr;
}

Function*
QLVirtualMachine::FindScript(CString p_name)
{
  NameMap::iterator it = m_scripts.find(p_name);
  if(it != m_scripts.end())
  {
    MemObject* object = it->second;
    if(object->m_type == DTYPE_SCRIPT)
    {
      return object->m_value.v_script;
    }
  }

  // Maybe it's an objects member
  int pos = p_name.Find("::");
  if(pos > 0)
  {
    CString classname  = p_name.Left(pos);
    CString membername = p_name.Mid(pos + 2);

    Class* theClass = FindClass(classname);
    if(theClass)
    {
      MemObject* member = theClass->FindFuncMember(membername);
      if(member)
      {
        return member->m_value.v_script;
      }
    }
  }
  return nullptr;
}

// Add an entry to a dictionary 
MemObject*
QLVirtualMachine::AddEntry(NameMap& dict,CString p_key,int p_storage)
{
  MemObject* entry;
  NameMap::iterator it = dict.find(p_key);
  if(it == dict.end())
  {
    entry = AllocMemObject(DTYPE_STRING);
    *(entry->m_value.v_string) = p_key;
    entry->m_storage = p_storage;
    dict.insert(std::make_pair(p_key,entry));
  }
  else
  {
    entry = it->second;
  }
  return entry;
}

// Add a new literal in the global space
void
QLVirtualMachine::AddLiteral(MemObject* p_object)
{
  if(m_literals == nullptr)
  {
    m_literals = new Array();
  }
  m_literals->AddEntry(p_object);
}

// addsymbol
MemObject*
QLVirtualMachine::AddSymbol(CString p_name)
{
  return AddEntry(m_symbols,p_name,ST_SDATA);
}

void
QLVirtualMachine::AddClass(Class* p_class)
{
  CString className = p_class->GetName();
  m_classes.insert(std::make_pair(className,p_class));
}

// Create new object and add all attribute members
MemObject*  
QLVirtualMachine::NewObject(Class* p_class)
{
  MemObject* val = AllocMemObject(DTYPE_OBJECT);
  val->m_value.v_object->Init(this,p_class);
  return val;
}

int
QLVirtualMachine::DestroyObject(MemObject* p_object)
{
  if(p_object->m_type == DTYPE_OBJECT)
  {
    FreeMemObject(p_object);

    // Call the garbage collector every now and then!
    if((m_allocs++ % m_threshold) == 0)
    {
      GC();
    }
    return 1;
  }
  return 0;
}

// Find existing global for the compiler
int
QLVirtualMachine::FindGlobal(CString p_name)
{
  for (int ind = 0; ind < m_globals->GetSize(); ++ind)
  {
    MemObject* object = m_globals->GetEntry(ind);
    if (object->m_type == DTYPE_STRING)
    {
      if (object->m_value.v_string->Compare(p_name) == 0)
      {
        return ind;
      }
    }
  }
  return -1;  
}

int
QLVirtualMachine::AddGlobal(MemObject* p_object,CString p_name)
{
  int n = FindGlobal(p_name);

  if(n < 0)
  {
    // Not found, add a new one
    n = m_globals->GetSize();
    if(p_object == nullptr)
    {
      p_object = AddSymbol(p_name);
    }
    m_globals->AddEntry(p_object);
  }
  return n;
}

MemObject*  
QLVirtualMachine::GetGlobal(unsigned p_index)
{
  if(p_index >= 0 && p_index < (unsigned)m_globals->GetSize())
  {
    return m_globals->GetEntry(p_index);
  }
  Error("Global entry out of range: %d",p_index);
  return nullptr;
}

void
QLVirtualMachine::SetGlobal(unsigned p_index,MemObject* p_object)
{
  if(p_index >= 0 && p_index < (unsigned) m_globals->GetSize())
  {
    m_globals->SetEntry(p_index,p_object);
    return;
  }
  Error("Global entry out of range: %d", p_index);
}

MemObject*
QLVirtualMachine::GetLiteral(unsigned p_index)
{
  if(m_literals)
  {
    return m_literals->GetEntry(p_index);
  }
  return nullptr;
}


CString
QLVirtualMachine::FindSymbolName(MemObject* p_object)
{
  for(auto& sym : m_symbols)
  {
    if((sym.second->m_type        == p_object->m_type) &&
       (sym.second->m_value.v_all == p_object->m_value.v_all))
    {
      return sym.first;
    }
  }
  return CString("<Symbol-not-found>");
}

Method*
QLVirtualMachine::FindMethod(CString p_name, int p_type)
{
  MethodMap::iterator iter1 = m_methods.lower_bound(p_name);
  MethodMap::iterator iter2 = m_methods.upper_bound(p_name);
  MethodMap::iterator it;

  for(it = iter1;it != iter2; ++it)
  {
    if(it->second->m_datatype == p_type)
    {
      return it->second;
    }
  }
  // Nothing found
  return nullptr;
}

Method*
QLVirtualMachine::AddMethod(CString p_name, int p_type)
{
  Method* found = FindMethod(p_name,p_type);
  if(found == nullptr)
  {
    found = new Method();
    found->m_datatype   = p_type;
    found->m_methodname = p_name;
    found->m_internal   = nullptr;
    // Remember new method
    m_methods.insert(std::make_pair(p_name,found));
  }
  return found;
}

void
QLVirtualMachine::AddBytecode(BYTE* p_bytecode,unsigned p_size)
{
  if(!m_initcode)
  {
    m_initcode_size = p_size;
    m_initcode = new BYTE[p_size + 1];
    memcpy(m_initcode,p_bytecode,p_size);
  }
  else
  {
    BYTE* code = new BYTE[m_initcode_size + p_size + 1];
    memcpy(code,m_initcode,m_initcode_size);
    memcpy(&m_initcode[m_initcode_size],p_bytecode,p_size);
    m_initcode_size += p_size;
    delete [] m_initcode;
    m_initcode = code;
  }
  // Mark as the end of the init group
  m_initcode[m_initcode_size] = (BYTE) OP_RETURN;
}


//////////////////////////////////////////////////////////////////////////
//
// PRINTING
//
//////////////////////////////////////////////////////////////////////////

// print1 - print one value 
int
QLVirtualMachine::Print(FILE* p_fp,int p_quoteFlag,MemObject* p_value)
{
  int len = 0;
  CString value;

  if(p_fp == nullptr)
  {
    p_fp = stdout;
  }

  if(p_value)
  switch (p_value->m_type)
  {
    case DTYPE_NIL:     value = "NIL";
                        break;
    case DTYPE_ENDMARK: value = "<ENDMARK>";
                        break;
    case DTYPE_INTEGER: value.Format("%ld",p_value->m_value.v_integer);
                        break;
    case DTYPE_STRING:  value = *p_value->m_value.v_string;
                        if (p_quoteFlag)
                        {
                          value = "\"" + value + "\"";
                        }
                        break;
    case DTYPE_BCD:     value = p_value->m_value.v_floating->AsString();
                        break;
    case DTYPE_FILE:   	value.Format("<File: %s>",FindSymbolName(p_value).GetString());
                        break;
    case DTYPE_DATABASE:value.Format("<Database: %s>",p_value->m_value.v_database->GetDatabaseName().GetString());
                        break;
    case DTYPE_QUERY:   value.Format("<Query: %p>",p_value->m_value.v_query);
                        break;
    case DTYPE_VARIANT: p_value->m_value.v_variant->GetAsString(value);
                        break;
    case DTYPE_ARRAY:   value.Format("<Array: %p>",p_value->m_value.v_array);
                        break;
    case DTYPE_OBJECT:  value.Format("<Object: %p Class: %s>"
                                     ,p_value->m_value.v_object
                                     ,p_value->m_value.v_object->GetClass()->GetName().GetString());
                        break;
    case DTYPE_CLASS:   value.Format("<Class: %s>",p_value->m_value.v_class->GetName().GetString());
                        break;
    case DTYPE_SCRIPT:  value.Format("<Function: %s>",p_value->m_value.v_script->GetName().GetString());
                        break;
    case DTYPE_INTERNAL:value.Format("<Internal: %s>",FindSymbolName(p_value).GetString());
                        break;
    case DTYPE_EXTERNAL:value.Format("<External: %s>",p_value->m_value.v_sysname->GetString());
                        break;
    default:            Error("Undefined type: %d", p_value->m_type);
                        break;
  }
  if(p_fp == stdout)
  {
    osputs_stdout(value);
    len = value.GetLength();
  }
  else if(p_fp == stderr)
  {
    osputs_stderr(value);
    len = value.GetLength();
  }
  else
  {
    len = fprintf(p_fp,value);
  }
  return len;
} 

//////////////////////////////////////////////////////////////////////////
// 
// TRANSACTIONS
//
//////////////////////////////////////////////////////////////////////////

int
QLVirtualMachine::SetSQLTransaction(SQLTransaction* p_trans)
{
  // Already a transaction, and trying to set a new one?
  if(m_transaction && p_trans)
  {
    return 0;
  }
  m_transaction = p_trans;
  return 1;
}

SQLTransaction* 
QLVirtualMachine::GetSQLTransaction()
{
  return m_transaction;
}

//////////////////////////////////////////////////////////////////////////
//
// GARBAGE COLLECTOR
//
//////////////////////////////////////////////////////////////////////////

void
QLVirtualMachine::GC()
{
  // STEP A: Mark all reachable memory objects
  MarkClasses();
  MarkMap(m_symbols);
  MarkMap(m_scripts);
  if(m_globals)
  {
    m_globals->Mark(this);
  }
  if(m_interpreter)
  {
    m_interpreter->Mark();
  }

  // STEP B: Delete unmarked memory objects
  RemoveUnmarked();
}

void
QLVirtualMachine::MarkClasses()
{
  for(auto& clas : m_classes)
  {
    clas.second->Mark(this);
  }
}

void 
QLVirtualMachine::MarkMap(NameMap& p_map)
{
  for(auto& obj : p_map)
  {
    if(obj.second->IsMarked() == false)
    {
      MarkObject(obj.second);
    }
  }
}

// Recursively mark objects
void
QLVirtualMachine::MarkObject(MemObject* p_object)
{
  // Mark the object
  p_object->m_generation |= GC_MARKED;

  switch(p_object->m_type)
  {
    case DTYPE_ARRAY:   p_object->m_value.v_array->Mark(this);
                        break;
    case DTYPE_OBJECT:  p_object->m_value.v_object->Mark(this);
                        break;
    case DTYPE_SCRIPT:  p_object->m_value.v_script->Mark(this);
                        break;
  }
}

void
QLVirtualMachine::RemoveUnmarked()
{
  // See if the chain is filled?
  if(m_root_object == nullptr || m_root_object->m_next == nullptr)
  {
    // No, nothing to do
    return;
  }
  // Start at the root object
  MemObject* object = m_root_object->m_next;

  // Walk until the end marker
  do 
  {
    if((object->m_generation & GC_MARKED) == 0)
    {
      // Object not marked
      if(!(object->m_flags & FLAG_REFERENCE))
      {
        MemObject* next = object->m_next;
        FreeMemObject(object);
        object = next;
      }
      else
      {
        // Object is a stack reference object, skip it
        object = object->m_next;
      }
    }
    else
    {
      // Object swiped, Next object
      object->m_generation = GC_ALIVE;
      object = object->m_next;
    }
  } 
  while (object && object->m_type != DTYPE_ENDMARK);
}

// Only to be called at destruction time
void
QLVirtualMachine::DestroyObjectChain()
{
  MemObject* object = m_root_object;
  while(object)
  {
    MemObject* next = object->m_next;

    if(m_dumpchain)
    {
      DumpObject(object);
    }
    FreeMemObject(object,false);

    if(object == m_last_object)
    {
      // Stop at the last object
      break;
    }
    // Next object
    object = next;
  }
}

void
QLVirtualMachine::DumpObject(MemObject* p_object)
{
  fputs("MEM: ",stderr);
  Print(stderr,true,p_object);
  fputs("\n",stderr);
}

void
QLVirtualMachine::CleanUpClasses()
{
  for(auto& cl : m_classes)
  {
    delete cl.second;
  }
  m_classes.clear();
}

void
QLVirtualMachine::CleanUpGlobals()
{
  if(m_globals)
  {
    delete m_globals;
    m_globals = nullptr;
  }
}

void
QLVirtualMachine::CleanUpLiterals()
{
  if(m_literals)
  {
    delete m_literals;
    m_literals = nullptr;
  }
}

void
QLVirtualMachine::CleanUpMethods()
{
  // Clean up all internal methods for data types
  for(auto& me : m_methods)
  {
    delete me.second;
  }
}

void
QLVirtualMachine::CleanUpInitcode()
{
  if(m_initcode)
  {
    delete [] m_initcode;
    m_initcode = nullptr;
  }
}