//////////////////////////////////////////////////////////////////////////
// 
// QL Language Virtual Machine executing the bytecode
// THIS IS THE READING OF THE OBJECT CODE FROM *.QOB FILES
//
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
#include "QL_Opcodes.h"
#include "bcd.h"
#include <stdarg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////
//
// FILE API : READING
//
//////////////////////////////////////////////////////////////////////////

bool
QLVirtualMachine::LoadFile(TCHAR* p_filename,bool p_trace)
{
  bool result = false;
  FILE* file = NULL;
  CString filename(p_filename);
  if(filename.Right(4).CompareNoCase(_T(".qob")))
  {
    filename += _T(".qob");
  }

  _tfopen_s(&file,filename,_T("rb"));
  if(file)
  {
    if(ReadFromFile(file,p_trace))
    {
      result = true;
    }
    else
    {
      _tprintf(_T("Object file NOT correctly loaded: %s\n"),filename.GetString());
    }
    fclose(file);
  }
  return result;
}

bool
QLVirtualMachine::ReadFromFile(FILE* p_fp,bool p_trace)
{
  // In case it is the first thing we do, check that we have
  // the root chain and the functions initialized
  CheckInit();

  // Clean up our stack/heap as much as possible
  // In case we do a second case read
  GC();

  try
  {
    tracing(_T("\nQL Program reading from file.\n\n"));
    tracing(_T("CODE             TRACING\n"));
    tracing(_T("---------------- ------------------------------------\n"));

    // Read the header
    ReadHeader(p_fp,p_trace);

    // Read classes stream
    ReadStream (p_fp,p_trace,_T("CLASSES stream header!"));
    ReadClasses(p_fp,p_trace,m_classes);

    ReadStream (p_fp,p_trace,_T("SYMBOLS stream header!"));
    ReadNameMap(p_fp,p_trace,m_symbols,_T("SYMBOLS"));

    // Read globals stream
    ReadStream   (p_fp,p_trace,_T("GLOBALS stream header!"));
    MustReadArray(p_fp,p_trace,m_globals,_T("GLOBALS"));

    // Read global literals
    ReadStream   (p_fp,p_trace,_T("GLOBAL LITERALS stream header!"));
    MustReadArray(p_fp,p_trace,m_literals,_T("GLOBAL LITERALS"));

    // Read global bytecode
    ReadStream  (p_fp,p_trace,_T("INIT BYTECODE stream header!"));
    ReadBytecode(p_fp,p_trace,&m_initcode,&m_initcode_size);

    // Read script stream
    ReadStream (p_fp,p_trace,_T("FUNCTIONS stream header!"));
    ReadNameMap(p_fp,p_trace,m_scripts,_T("FUNCTIONS"));

    // Read end of stream
    ReadStream(p_fp,p_trace,_T("END-OF-STREAM"));

    tracing(_T("\nQL Object file read-in OK!\n"));
  }
  catch(QLException& exception)
  {
    _ftprintf(stderr,_T("%s\n"),exception.GetMessage().GetString());
    return false;
  }
  // Thunking of the references
  Thunking();

  return true;
}

//////////////////////////////////////////////////////////////////////////
//
// READING THE HEAP FROM AN OBJECT FILE
//
//////////////////////////////////////////////////////////////////////////

int
QLVirtualMachine::Getc(FILE* p_fp,bool p_trace)
{
  // Getting the character
  int cc = ::_gettc(p_fp);
  // Decoding the character
  cc ^= 0xFF;

  if(p_trace)
  {
    // Reset to the left border?
    if(m_position >= 15)
    {
      _ftprintf(stderr,_T("\n"));
      m_position = 0;
    }
    // Print one byte
    _ftprintf(stderr,_T("%2.2X "),cc & 0xFF);
    m_position += 3;
  }
  return cc;
}

bool
QLVirtualMachine::ReadHeader(FILE* p_fp, bool p_trace)
{
  int   one = 0;
  int   two = 0;
  long  version = 0;
  TCHAR* error = _T("NOT a magic 'QL Object File' marker.");

  one = Getc(p_fp,p_trace);
  two = Getc(p_fp,p_trace);
  if (one != _T('Q') || two != 'L')
  {
    throw QLException(error);
  }
  TracingText(p_trace,_T("QL"));
  // Read version number
  MustReadInteger(p_fp,p_trace,&version,error);
  TracingText(p_trace,_T("QL Object file version: %2.2f"), (float)(version / 100));
  if (version != QL_VERSION)
  {
    error = _T("QL Object file WRONG VERSION!");
    throw QLException(error);
  }
  return true;
}

void
QLVirtualMachine::ReadStream(FILE* p_fp,bool p_trace,TCHAR* p_message)
{
  if (Getc(p_fp,p_trace) != DTYPE_FILE)
  {
    throw QLException(p_message);
  }
  TracingText(p_trace,p_message);
}

bool
QLVirtualMachine::ReadInteger(FILE* p_fp, bool p_trace, long* n)
{
  int r;

  // Clear result
  *n = 0;

  // Read 4 bytes
  r = Getc(p_fp,p_trace);  *n += r << 24;
  r = Getc(p_fp,p_trace);  *n += r << 16;
  r = Getc(p_fp,p_trace);  *n += r << 8;
  r = Getc(p_fp,p_trace);  *n += r;

  return (r != _TEOF);
}

bool
QLVirtualMachine::MustReadInteger(FILE* p_fp, bool p_trace, long* n, TCHAR* p_errorMessage)
{
  int type = Getc(p_fp,p_trace);
  if (type != DTYPE_INTEGER)
  {
    throw QLException(p_errorMessage);
  }
  bool res = ReadInteger(p_fp, p_trace, n);
  if (res == false)
  {
    throw QLException(p_errorMessage);
  }
  return res;
}

CString     
QLVirtualMachine::ReadString(FILE* p_fp, bool p_trace)
{
  long size = 0;

  if(MustReadInteger(p_fp,p_trace,&size,_T("MISREAD STRING LENGTH")))
  {
    TracingText(p_trace,_T("String size: %d"),size);

    CString theString;

    for(int ind = 0; ind < size; ++ind)
    {
      theString += (const TCHAR) Getc(p_fp,p_trace);
    }
    if(Getc(p_fp,p_trace) != 0)
    {
      throw QLException(_T("String with no ending!"));
    }
    TracingText(p_trace,_T("String : [%s]"), theString.GetString());
    return theString;
  }
  else
  {
    throw QLException(_T("Misread string!"));
  }
  return _T("");
}

CString 
QLVirtualMachine::MustReadString(FILE* p_fp,bool p_trace,TCHAR* p_errorMessage)
{
  int type = Getc(p_fp,p_trace);
  if(type != DTYPE_STRING)
  {
    throw QLException(p_errorMessage);
  }
  TracingText(p_trace,_T("STRING"));
  return ReadString(p_fp,p_trace);
}

bcd*
QLVirtualMachine::ReadFloat(FILE* p_fp, bool p_trace)
{
  int ch   = 0;
  SQL_NUMERIC_STRUCT numeric;

  // Read in the numeric structure
  numeric.precision = Getc(p_fp,p_trace);
  numeric.scale     = Getc(p_fp,p_trace);
  numeric.sign      = Getc(p_fp,p_trace);
  // Mantissa
  for(int i = 0;i < SQL_MAX_NUMERIC_LEN; ++i)
  {
    ch = Getc(p_fp,p_trace);
    numeric.val[i] = ch;
  }
  // See if we're still good on reading the file
  if(ch == _TEOF)
  {
    throw QLException(_T("Misread bcd float!"));
  }

  // Place in a BCD
  bcd* val = new bcd(&numeric);

  TracingText(p_trace,_T("BCD: %s"),val->AsString().GetString());
  return val;
}

bcd*
QLVirtualMachine::MustReadFloat(FILE* p_fp,bool p_trace,TCHAR* p_message)
{
  int type = Getc(p_fp,p_trace);
  if(type != DTYPE_BCD)
  {
    throw QLException(p_message);
  }
  return ReadFloat(p_fp,p_trace);
}

FILE*
QLVirtualMachine::ReadFileName(FILE* p_fp, bool p_trace)
{
  CString name = MustReadString(p_fp,p_trace,_T("Misread file stream name!"));

  MemObject* fp = FindSymbol(name);
  if(fp && fp->m_type == DTYPE_FILE)
  {
    return fp->m_value.v_file;
  }
  throw QLException(CString(_T("File pointer not found: ")) + name,DTYPE_FILE);
  return nullptr;
}

Array*
QLVirtualMachine::ReadArray(FILE* p_fp, bool p_trace,Array* p_array /*=nullptr*/,TCHAR* p_name /*=nullptr*/)
{
  CString name = p_name ? p_name : _T("ARRAY");
  TracingText(p_trace,_T("READING %s"),name);

  // Get the array size
  long arraySize = 0;
  MustReadInteger(p_fp,p_trace,&arraySize,_T("Missing array size"));
  TracingText(p_trace,_T("Arraysize: %d"),arraySize);

  // Create new array or use parameter
  Array* array = p_array ? p_array : new Array();

  // Read all members from stream
  for (int ind = 0; ind < arraySize; ++ind)
  {
    TRACE(_T("Reading item: %d\n"),ind + 1);
    array->AddEntry(ReadMemObject(p_fp,p_trace));
  }
  TracingText(p_trace,_T("END %s"),name);

  return array;
}

Array*
QLVirtualMachine::MustReadArray(FILE* p_fp, bool p_trace, Array* p_array /*=nullptr*/,TCHAR* p_name /*=nullptr*/)
{
  int type = Getc(p_fp,p_trace);
  if(type != DTYPE_ARRAY)
  {
    throw QLException(_T("Misread array!"));
  }
  return ReadArray(p_fp,p_trace,p_array,p_name);
}

Object*
QLVirtualMachine::ReadObject(FILE* p_fp, bool p_trace)
{
  // Read the class name
  CString className = MustReadString(p_fp,p_trace,_T("Misread object class name"));
  Class*  theClass  = FindClass(className);
  Object* object = new Object(theClass);

  // Read number of attributes
  long attributes = 0;
  MustReadInteger(p_fp,p_trace,&attributes,_T("Misread number of object attributes!"));

  // Read all attributes of the object
  for (int ind = 0; ind < attributes; ++ind)
  {
    object->SetAttribute(ind,ReadMemObject(p_fp,p_trace));
  }
  TracingText(p_trace,_T("END OBJECT"));

  return object;
}

Class*
QLVirtualMachine::ReadClass(FILE* p_fp,bool p_trace)
{
  int type = Getc(p_fp,p_trace);
  if(type != DTYPE_CLASS)
  {
    throw QLException(_T("Misread class in stream!"));
  }

  CString className = MustReadString(p_fp,p_trace,_T("Misread class name!"));
  CString  baseName = MustReadString(p_fp,p_trace,_T("Misread base class name!"));

  Class* baseClass = nullptr;
  if(!baseName.IsEmpty())
  {
    baseClass = FindClass(baseName);
    if(baseClass == nullptr)
    {
      // Precreate base class
      // Assume it comes later on in the stream
      baseClass = new Class(baseName);
      AddClass(baseClass);
    }
  }

  // Find class or make it. It could be made as a base class previously
  Class* theClass = FindClass(className);
  if(theClass == nullptr)
  {
    theClass = new Class(className, baseClass);
    AddClass(theClass);
  }
  else if(baseClass && !theClass->GetBaseClass())
  {
    // Base class found, but not yet set
    theClass->SetBaseClass(baseClass);
  }
  
  // Read the members array
  Array& members = theClass->GetMembers();
  MustReadArray(p_fp,p_trace,&members,_T("MEMBERS"));

  // Read attributes names
  Array& attribs = theClass->GetAttributes();
  MustReadArray(p_fp, p_trace, &attribs,_T("ATTRIBUTES"));

  TracingText(p_trace,_T("END CLASS"));

  return theClass;
}

void        
QLVirtualMachine::ReadBytecode(FILE* p_fp, bool p_trace, BYTE** p_bytecode,int* p_size)
{
  long length = 0;
  // Read the length up front
  MustReadInteger(p_fp,p_trace,&length,_T("Misread bytecode length!"));
  TracingText(p_trace,_T("BYTECODE (Size: %d)"),length);

  // Allocate the bytecode array
  BYTE* bytecode = new BYTE[length + 2];
  BYTE* pointer  = bytecode;

  // Read the bytecode array
  for (int ind = 0; ind < length; ++ind)
  {
    int cc = Getc(p_fp,p_trace);
    *pointer++ = (BYTE) cc;

    if (cc == _TEOF)
    {
      throw QLException(_T("Bytecode not written!"));
    }
  }
  // Write extra OP_RETURN
  *pointer++ = OP_RETURN;
  // End marker
  *pointer = 0;

  // Transfer the result
  *p_bytecode = bytecode;
  *p_size     = length;

  TracingText(p_trace,_T("END OF BYTECODE"));
}

Function*   
QLVirtualMachine::ReadScript(FILE* p_fp, bool p_trace)
{
  Class* theClass = nullptr;

  // Write the function name
  CString functionName = MustReadString(p_fp, p_trace,_T("Misread script function name!"));
  // Write class name (if any)
  CString className    = MustReadString(p_fp, p_trace,_T("Misread member class name!"));
  if(!className.IsEmpty())
  {
    theClass = FindClass(className);
    if(!theClass)
    {
      throw QLException(_T("Cannot find member class!"));
    }
  }

  // Create the script function
  Function* function = new Function(functionName);
  function->SetClass(theClass);

  // Read argument data types
  int type = Getc(p_fp,p_trace);
  if(type == DTYPE_ARRAY)
  {
    long num = 0;
    MustReadInteger(p_fp,p_trace,&num,_T("Number of arguments"));

    for(int ind = 0;ind < num; ++ind)
    {
      long arg = 0;
      MustReadInteger(p_fp,p_trace,&arg,_T("Argument"));
      function->AddArgument(arg);
    }
  }
  else
  {
    throw QLException(_T("Unknown argument marker for script!"));
  }

  // Read literals array
  type = Getc(p_fp,p_trace);
  if(type == DTYPE_ARRAY)
  {
    function->SetLiterals(ReadArray(p_fp,p_trace,nullptr,_T("LITERALS")));
  }
  else if(type != DTYPE_NIL)
  {
    throw QLException(_T("Unknown literal marker for script!"));
  }

  // Read the bytecode for the function
  BYTE* bytecode = nullptr;
  int   bytecode_size = 0;

  ReadBytecode(p_fp,p_trace,&bytecode,&bytecode_size);
  function->SetBytecode(bytecode,bytecode_size);

  delete[] bytecode;


  TracingText(p_trace,_T("END SCRIPT"));

  return function;
}

Internal    
QLVirtualMachine::ReadInternal(FILE* p_fp, bool p_trace)
{
  // Read the internal name
  CString funcName = MustReadString(p_fp,p_trace,_T("Misread internal function name!"));
  MemObject* object = FindSymbol(funcName);

  if(object->m_type == DTYPE_INTERNAL)
  {
    return object->m_value.v_internal;
  }
  throw QLException(_T("Unknown internal function found!"));
  return nullptr;
}

CString*
QLVirtualMachine::ReadExternal(FILE* p_fp, bool p_trace)
{
  CString name = MustReadString(p_fp,p_trace,_T("Misread external system function name!"));
  return new CString(name);
}

void
QLVirtualMachine::ReadReference(FILE* p_fp,bool p_trace,MemObject* p_object)
{
  CString type = datatype_names[p_object->m_type & DTYPE_MASK];
  TracingText(p_trace,_T("REFERENCE TO %s"),type);

  CString name = MustReadString(p_fp,p_trace,_T("Misreading reference name!"));
  p_object->m_value.v_string = new CString(name);
}

MemObject*
QLVirtualMachine::ReadMemObject(FILE* p_fp,bool p_trace)
{
  MemObject* object = AllocMemObject(DTYPE_NIL);
  object->m_type = Getc(p_fp,p_trace);

  if(object->m_type & DTYPE_REFERENCE)
  {
    // Reading a reference to an object
    ReadReference(p_fp,p_trace,object);
  }
  else switch(object->m_type)
  {
    // Reading of the actual object
    case DTYPE_NIL:       break;
    case DTYPE_INTEGER:   ReadInteger(p_fp,p_trace,(long*) &object->m_value.v_integer); 
                          TracingText(p_trace,_T("Integer: %d"),object->m_value.v_integer);
                          break;
    case DTYPE_STRING:    TracingText(p_trace,_T("STRING"));
                          object->m_value.v_string = new CString(ReadString(p_fp,p_trace));
                          object->m_flags |= FLAG_DEALLOC;
                          break;
    case DTYPE_BCD:       TracingText(p_trace,_T("BCD"));
                          object->m_value.v_floating = ReadFloat(p_fp,p_trace);
                          object->m_flags |= FLAG_DEALLOC;
                          break;
    case DTYPE_FILE:      TracingText(p_trace,_T("FILENAME"));
                          object->m_value.v_file = ReadFileName(p_fp,p_trace);
                          break;
    case DTYPE_ARRAY:     TracingText(p_trace,_T("ARRAY"));
                          object->m_value.v_array = ReadArray(p_fp,p_trace);
                          object->m_flags |= FLAG_DEALLOC;
                          break;
    case DTYPE_OBJECT:    TracingText(p_trace,_T("OBJECT"));
                          object->m_value.v_object = ReadObject(p_fp,p_trace);
                          object->m_flags |= FLAG_DEALLOC;
                          break;
    case DTYPE_CLASS:     // never reached
                          break;
    case DTYPE_SCRIPT:    TracingText(p_trace,_T("SCRIPT"));
                          object->m_value.v_script = ReadScript(p_fp,p_trace);
                          object->m_flags |= FLAG_DEALLOC;
                          break;
    case DTYPE_INTERNAL:  TracingText(p_trace,_T("INTERNAL"));
                          object->m_value.v_internal = ReadInternal(p_fp,p_trace);
                          break;
    case DTYPE_EXTERNAL:  TracingText(p_trace,_T("EXTERNAL"));
                          object->m_value.v_sysname = ReadExternal(p_fp,p_trace);
                          object->m_flags |= FLAG_DEALLOC;
                          break;
    default:              Error(_T("Unknown type in file stream!"));
                          break;
  }
  return object;
}

void
QLVirtualMachine::ReadClasses(FILE* p_fp, bool p_trace, ClassMap& p_map)
{
  // Read number of classes
  long classes = 0;
  MustReadInteger(p_fp,p_trace,&classes,_T("Misread number of classes!"));

  // Read and remember all classes
  for(int ind = 0;ind < classes; ++ind)
  {
    ReadClass(p_fp,p_trace);
  }
  TracingText(p_trace,_T("END OF CLASSES"));
}

void
QLVirtualMachine::ReadNameMap(FILE* p_fp, bool p_trace, NameMap&  p_map,TCHAR* p_name)
{
  long size = 0;
  MustReadInteger(p_fp,p_trace,&size,_T("Misread namemap size!"));
  TracingText(p_trace,_T("%s (Size: %d)"),p_name,size);

  for(int ind = 0;ind < size; ++ind)
  {
    CString name;
    MemObject* object = ReadMemObject(p_fp,p_trace);

    if(object->m_type == DTYPE_STRING)
    {
      name = *object->m_value.v_string;
    }
    if(object->m_type == DTYPE_SCRIPT)
    {
      name = object->m_value.v_script->GetName();
    }

    // Add to the map
    if(!name.IsEmpty())
    {
      NameMap::iterator it = p_map.find(name);
      if(it == p_map.end())
      {
        // Only insert the object if not found in the map already
        p_map.insert(std::make_pair(name,object));
      }
    }
  }
  TracingText(p_trace,_T("END OF %s"),p_name);
}

//////////////////////////////////////////////////////////////////////////
//
// THUNKING TO BE DONE AFTER A LOAD FROM FILE
//
//////////////////////////////////////////////////////////////////////////

void
QLVirtualMachine::Thunking()
{
  MemObject* object = m_root_object->m_next;

  while(object && object->m_next->m_type != DTYPE_ENDMARK)
  {
    if(object->m_type & DTYPE_REFERENCE)
    {
      // Getting the actual name
      CString name = *object->m_value.v_string;

      // Remember and clearing the read-in string
      CString* str = object->m_value.v_string;
      object->m_value.v_all = 0;

      // Find the reference
      switch(object->m_type & DTYPE_MASK)
      {
        case DTYPE_OBJECT:  object->m_value.v_object = new Object(FindClass(name));
                            break;
        case DTYPE_CLASS:   object->m_value.v_class  = FindClass(name);
                            break;
        case DTYPE_SCRIPT:  object->m_value.v_script = FindScript(name);
                            break;
        default:            Error(_T("Unknown reference data type!"));
                            break;
      }

      // See if thunking successfull
      if(object->m_value.v_all)
      {
        // Clear the reference marker
        object->m_type &= ~DTYPE_REFERENCE;

        // Tell the object it carries a reference
        // and should not deallocate the object
        if(object->m_type == DTYPE_CLASS || object->m_type == DTYPE_SCRIPT)
        {
          object->m_flags |= FLAG_REFERENCE;
        }

        // Delete the saved string
        delete str;
      }
      else
      {
        // Not thunked yet, save for a next try in a next loading operation
        object->m_value.v_string = str;
      }
    } 
    // Next object
    object = object->m_next;
  }
}