//////////////////////////////////////////////////////////////////////////
// 
// QL Language Virtual Machine executing the bytecode
// THIS IS THE WRITING OF THE OBJECT CODE TO *.QOB FILES
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
#include "QL_Objects.h"
#include "bcd.h"
#include <stdarg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////
//
// FILE API : WRITING
//
//////////////////////////////////////////////////////////////////////////

bool 
QLVirtualMachine::WriteFile(char* p_filename,bool p_trace)
{
  FILE* file = NULL;

  // check that name ends in .qob
  CString filename(p_filename);
  if(filename.Right(3).CompareNoCase(".ql") == 0)
  {
    filename = filename.Left(filename.GetLength() - 3);
  }
  if(filename.Right(4).CompareNoCase(".qob"))
  {
    filename += ".qob";
  }

  fopen_s(&file,filename,"wb");
  if(file)
  {
    tracingx("\nQL Program writing to file: %s\n\n",filename.GetString());
    if(WriteToFile(file,p_trace) == false)
    {
      printf("Object file NOT written correctly to: %s\n",filename.GetString());
    }
    fclose(file);
    return true;
  }
  return false;
}

bool
QLVirtualMachine::WriteToFile(FILE* p_fp,bool p_trace)
{
  try
  {
    tracing("CODE             TRACING\n");
    tracing("---------------- ------------------------------------\n");

    WriteHeader(p_fp,p_trace);

    WriteStream (p_fp,p_trace,"CLASSES stream header!");
    WriteClasses(p_fp,p_trace,m_classes);

    WriteStream (p_fp,p_trace,"SYMBOLS stream header!");
    WriteNameMap(p_fp,p_trace,m_symbols,"SYMBOLS",false);

    WriteStream(p_fp,p_trace,"GLOBALS stream header!");
    WriteArray (p_fp,p_trace,m_globals);

    WriteStream(p_fp,p_trace,"GLOBAL LITERALS stream header!");
    WriteArray (p_fp,p_trace,m_literals);

    WriteStream  (p_fp,p_trace,"INIT BYTECODE stream header!");
    WriteBytecode(p_fp,p_trace,m_initcode,m_initcode_size);

    WriteStream(p_fp,p_trace,"FUNCTIONS stream header!");
    WriteNameMap(p_fp,p_trace,m_scripts,"FUNCTIONS",true);

    WriteStream(p_fp,p_trace,"END-OF-STREAM");
    tracing("\nQL Object written OK!\n");
  }
  catch(QLException& exception)
  {
    fprintf(stderr,"%s\n",exception.GetMessage().GetString());
    return false;
  }
  return true;
}

//////////////////////////////////////////////////////////////////////////
//
// WRITING THE HEAP TO AN OBJECT FILE
//
//////////////////////////////////////////////////////////////////////////

// PUT 1 CHAR on the file stream
int
QLVirtualMachine::Putc(int cc,FILE* p_fp,bool p_trace)
{
  if(p_trace)
  {
    // Reset to the left border?
    if(m_position >= 15)
    {
      fprintf(stderr,"\n");
      m_position = 0;
    }
    // Print one byte
    fprintf(stderr,"%2.2X ",cc & 0xFF);
    m_position += 3;
  }
  // Put one byte on the file stream
  return putc(cc ^ 0xFF,p_fp);
}

// ADD tracing text to the output
void
QLVirtualMachine::TracingText(bool p_trace,const char* p_text,...)
{
  // See if we have something to do
  if(p_trace == false)
  {
    return;
  }
  CString str;
  va_list txt;

  // Format our comment
  va_start(txt,p_text);
  str.FormatV(p_text,txt);
  va_end(txt);

  // Print comment in second column
  while(m_position++ < 17)
  {
    fprintf(stderr," ");
  }
  // Replace non readable chars
  str.Replace("\n","\\n");
  str.Replace("\r","\\r");
  str.Replace("\t","\\t");
  str.Replace("\f","\\f");

  // Really printing
  fprintf(stderr,"%s\n",str.GetString());

  // Reset the position
  m_position = 0;
}


// Write object file header
void
QLVirtualMachine::WriteHeader(FILE* p_fp,bool p_trace)
{
  TracingText(p_trace,"FILE HEADER");

  Putc('Q',p_fp,p_trace);
  Putc('L',p_fp,p_trace);
  TracingText(p_trace,"QL bytecode stream");

  WriteInteger(p_fp,p_trace,QL_VERSION);
  TracingText(p_trace,"QL Version: %2.2f",(float)(QL_VERSION / 100));
}

void
QLVirtualMachine::WriteStream(FILE* p_fp,bool p_trace,char* p_message)
{
  if(Putc(DTYPE_FILE,p_fp,p_trace) == EOF)
  {
    throw QLException(p_message);
  }
  TracingText(p_trace,p_message);
}

void
QLVirtualMachine::WriteInteger(FILE* p_fp,bool p_trace,int n)
{
  Putc(DTYPE_INTEGER,p_fp,p_trace);
  if(Putc((int)(n >> 24) & 0xFF,p_fp,p_trace) == EOF ||
     Putc((int)(n >> 16) & 0xFF,p_fp,p_trace) == EOF ||
     Putc((int)(n >>  8) & 0xFF,p_fp,p_trace) == EOF ||
     Putc((int)(n)       & 0xFF,p_fp,p_trace) == EOF )
  {
    throw QLException("Integer not written!");
  }
}

void
QLVirtualMachine::WriteString(FILE* p_fp,bool p_trace,CString* p_string,char* p_extra /*=nullptr*/)
{
  // Actual string data
  const char* pnt = p_string->GetString();
  CString extra;

  // String type + the string itself
  Putc(DTYPE_STRING,p_fp,p_trace);
  if(p_extra)
  {
    extra = p_extra + CString(" : [") + *p_string + "]";
  }
  else
  {
    // Just the string itself
    extra = "String: [" + *p_string + "]";
  }
  TracingText(p_trace,extra);

  // Size of the string
  WriteInteger(p_fp,p_trace,p_string->GetLength());
  TracingText(p_trace,"Stringsize: %d",p_string->GetLength());

  // Write string onto file
  for(int ind = 0; ind <= p_string->GetLength(); ++ind)
  {
    if(Putc(*pnt++,p_fp,p_trace) == EOF)
    {
      throw QLException("String not written!");
    }
  }
  // Eventually trace a newline
  if(m_position > 0 && m_position <= 15)
  {
    TracingText(p_trace,"");
  }
}

void
QLVirtualMachine::WriteFloat(FILE* p_fp,bool p_trace,bcd* p_float)
{
  bool res = false;

  // BCD type + the value
  Putc(DTYPE_BCD,p_fp,p_trace);
  TracingText(p_trace,"BCD: %s",p_float->AsString().GetString());

  // Getting the BCD in a structure
  SQL_NUMERIC_STRUCT numeric;
  p_float->AsNumeric(&numeric);

  // Write the structure
  Putc(numeric.precision,p_fp,p_trace);
  Putc(numeric.scale,    p_fp,p_trace);
  Putc(numeric.sign,     p_fp,p_trace);

  for(int i = 0;i < SQL_MAX_NUMERIC_LEN; ++i)
  {
    res = Putc(numeric.val[i],p_fp,p_trace);
  }
  TracingText(p_trace,"");

  if(res == false)
  {
    throw QLException("BCD float not written!");
  }
}

void
QLVirtualMachine::WriteFileName(FILE* p_fp,bool p_trace,MemObject* p_object)
{
  CString name = FindSymbolName(p_object);

  Putc(DTYPE_FILE,p_fp,p_trace);
  TracingText(p_trace,"FILENAME");
  WriteString(p_fp,p_trace,&name);
}

// Write an array to file
void
QLVirtualMachine::WriteArray(FILE* p_fp,bool p_trace,Array* p_array,char* p_extra /*= nullptr*/,bool p_doScripts /*=false*/)
{
  // Arrays can be empty (literals, globals etc)
  if(p_array == nullptr)
  {
    Putc(DTYPE_NIL,p_fp,p_trace);
    CString extra("EMPTY ARRAY");
    if(p_extra) extra = CString("NO ") + p_extra;
    TracingText(p_trace,extra);
    return;
  }
  // Array marker and size
  Putc(DTYPE_ARRAY,p_fp,p_trace);
  TracingText(p_trace,p_extra ? p_extra : "ARRAY");
  WriteInteger(p_fp,p_trace,p_array->GetSize());
  TracingText(p_trace,"Arraysize: %d",p_array->GetSize());

  // Stream all members to file
  for(int ind = 0;ind < p_array->GetSize(); ++ind)
  {
    TRACE("Writing array: %d\n",ind + 1);
    WriteMemObject(p_fp,p_trace,p_array->GetEntry(ind),p_doScripts);
  }
  // End array marker
  CString end("END ");
  end += p_extra ? p_extra : "ARRAY";
  TracingText(p_trace,end);
}

void
QLVirtualMachine::WriteObject(FILE* p_fp,bool p_trace,Object* p_object)
{
  Class* theClass = p_object->GetClass();
  int    numAttrib = theClass->GetSize();

  // Write object marker
  Putc(DTYPE_OBJECT,p_fp,p_trace);
  TracingText(p_trace,"OBJECT");

  // Write class name
  WriteString(p_fp,p_trace,&theClass->GetName());

  // Write number of attributes
  WriteInteger(p_fp,p_trace,numAttrib);
  TracingText(p_trace,"Attributes: %d",numAttrib);

  // Write all attributes of the object
  for(int ind = 0;ind < numAttrib; ++ind)
  {
    WriteMemObject(p_fp,p_trace,p_object->GetAttribute(ind));
  }
  TracingText(p_trace,"END OBJECT");
}

void
QLVirtualMachine::WriteClass(FILE* p_fp,bool p_trace,Class* p_class)
{
  Class* baseClass = p_class->GetBaseClass();
  CString className = p_class->GetName();
  CString baseName;
  if(baseClass)
  {
    baseName = baseClass->GetName();
  }

  // Write class marker
  Putc(DTYPE_CLASS,p_fp,p_trace);
  TracingText(p_trace,"CLASS DEFINITION");
  // Write class Name
  WriteString(p_fp,p_trace,&className,"Classname");
  // Write base class name
  WriteString(p_fp,p_trace,&baseName,"Baseclass");
  // Write members array
  WriteArray(p_fp,p_trace,&p_class->GetMembers(),"MEMBERS",true);
  // Write attributes names
  WriteArray(p_fp,p_trace,&p_class->GetAttributes(),"ATTRIBUTES");

  TracingText(p_trace,"END CLASS");
}

void
QLVirtualMachine::WriteBytecode(FILE* p_fp,bool p_trace,BYTE* p_bytecode,int p_length)
{
  // Write the length up front
  WriteInteger(p_fp,p_trace,p_length);
  TracingText(p_trace,"BYTECODE (Length: %d)",p_length);

  // Write the bytecode array
  for(int ind = 0;ind < p_length; ++ind)
  {
    int cc = *p_bytecode++;
    if(Putc(cc,p_fp,p_trace) == EOF)
    {
      throw QLException("Bytecode not written!");
    }
  }
  TracingText(p_trace,"");
}

void
QLVirtualMachine::WriteScript(FILE* p_fp,bool p_trace,Function*  p_script)
{
  // Write script should not be called recursively for recursive functions
  if(p_script->GetWriting())
  {
    return;
  }
  p_script->SetWriting(true);

  CString className;
  CString functionName;
  Class*  theClass = p_script->GetClass();

  // Getting the names
  functionName = p_script->GetName();
  if(theClass)
  {
    className = theClass->GetName();
  }

  // Write script marker
  Putc(DTYPE_SCRIPT,p_fp,p_trace);
  TracingText(p_trace,"SCRIPT");
  // Write the function name
  WriteString(p_fp,p_trace,&functionName,"Functionname");
  // Write class name (if any)
  WriteString(p_fp,p_trace,&className,"Classname");
  // Write number of arguments and data types
  WriteTypes(p_fp,p_trace,p_script->GetArgumentTypes());
  // Write literals array
  WriteArray(p_fp,p_trace,p_script->GetLiterals(),"LITERALS");
  // Write bytecode 
  WriteBytecode(p_fp,p_trace,p_script->GetBytecode(),p_script->GetBytecodeSize());

  TracingText(p_trace,"END SCRIPT");
}

void 
QLVirtualMachine::WriteTypes(FILE* p_fp,bool p_trace,ArgTypes& p_types)
{
  int num = (int)p_types.size();

  Putc(DTYPE_ARRAY,p_fp,p_trace);
  TracingText(p_trace,"Number of arguments");
  WriteInteger(p_fp,p_trace,num);

  for(int ind = 0;ind < num; ++ind)
  {
    WriteInteger(p_fp,p_trace,p_types[ind]);
    TracingText(p_trace,"Datatype argument %d",ind);
  }
}

void
QLVirtualMachine::WriteInternal(FILE* p_fp,bool p_trace,MemObject* p_internal)
{
  CString name = FindSymbolName(p_internal);

  Putc(DTYPE_INTERNAL,p_fp,p_trace);
  TracingText(p_trace,"INTERNAL");
  WriteString(p_fp,p_trace,&name);
}

void
QLVirtualMachine::WriteExternal(FILE* p_fp,bool p_trace,CString* p_external)
{
  Putc(DTYPE_EXTERNAL,p_fp,p_trace);
  TracingText(p_trace,"EXTERNAL");
  WriteString(p_fp,p_trace,p_external);
}

void
QLVirtualMachine::WriteReference(FILE* p_fp,bool p_trace,MemObject* p_object)
{
  CString name;

  switch(p_object->m_type)
  {
    // ACTUAL STORAGE OF THE OBJECT
    case DTYPE_NIL:        Putc(DTYPE_NIL,p_fp,p_trace);
                           TracingText(p_trace,"NIL");
                           return;
    case DTYPE_INTEGER:    WriteInteger (p_fp,p_trace,p_object->m_value.v_integer);  return;
    case DTYPE_STRING:     WriteString  (p_fp,p_trace,p_object->m_value.v_string);   return;
    case DTYPE_BCD:        WriteFloat   (p_fp,p_trace,p_object->m_value.v_floating); return;
    case DTYPE_FILE:       WriteFileName(p_fp,p_trace,p_object);                     return;
    case DTYPE_INTERNAL:   WriteInternal(p_fp,p_trace,p_object);                     return;
    case DTYPE_EXTERNAL:   WriteExternal(p_fp,p_trace,p_object->m_value.v_sysname);  return;
    case DTYPE_ARRAY:      Error("Cannot write reference for an array");             return;
    case DTYPE_OBJECT:     name = p_object->m_value.v_object->GetClass()->GetName();
                           break;
    case DTYPE_CLASS:      name = p_object->m_value.v_class->GetName();
                           break;
    case DTYPE_SCRIPT:     name = p_object->m_value.v_script->GetFullName();
                           break;
    default:               Error("Unknown data type!\n");
                           break;
  }

  // Reference is stored in the stream with the type
  CString objecttype = datatype_names[p_object->m_type];
  int type = p_object->m_type | DTYPE_REFERENCE;
  Putc(type,p_fp,p_trace);
  TracingText(p_trace,"REFERENCE TO %s",objecttype);

  // Write the name of the reference
  WriteString(p_fp,p_trace,&name);
}

void
QLVirtualMachine::WriteMemObject(FILE* p_fp,bool p_trace,MemObject* p_object,bool p_scripts /*=false*/)
{
  if(p_object->m_flags & FLAG_REFERENCE)
  {
    // WRITE REFERENCE OF THE OBJECT
    WriteReference(p_fp,p_trace,p_object);
  }
  else switch(p_object->m_type)
  {
    // ACTUAL STORAGE OF THE OBJECT
    case DTYPE_NIL:        Putc(DTYPE_NIL,p_fp,p_trace);
                           TracingText(p_trace,"NULL");
                           break;
    case DTYPE_INTEGER:    WriteInteger(p_fp,p_trace,     p_object->m_value.v_integer);
                           TracingText (p_trace,"int: %d",p_object->m_value.v_integer);
                           break;
    case DTYPE_STRING:     WriteString  (p_fp,p_trace,p_object->m_value.v_string);     break;
    case DTYPE_BCD:        WriteFloat   (p_fp,p_trace,p_object->m_value.v_floating);   break;
    case DTYPE_FILE:       WriteFileName(p_fp,p_trace,p_object);                       break;
    case DTYPE_ARRAY:      WriteArray   (p_fp,p_trace,p_object->m_value.v_array);      break;
    case DTYPE_OBJECT:     WriteObject  (p_fp,p_trace,p_object->m_value.v_object);     break;
    case DTYPE_CLASS:      WriteClass   (p_fp,p_trace,p_object->m_value.v_class);      break;
    case DTYPE_SCRIPT:     if(p_scripts)
                           {
                             WriteScript(p_fp,p_trace,p_object->m_value.v_script);
                           }
                           else
                           {
                             WriteReference(p_fp,p_trace,p_object);
                             // WriteString(p_fp,p_trace,&p_object->m_value.v_script->GetFullName(),"Functionname");
                           }
                           break;
    case DTYPE_INTERNAL:   WriteInternal(p_fp,p_trace,p_object);                       break;
    case DTYPE_EXTERNAL:   WriteExternal(p_fp,p_trace,p_object->m_value.v_sysname);    break;
    default:               TracingText  (p_trace,"FIX ME: UNKNOWN DATA TYPE");         break;
  }
}

void
QLVirtualMachine::WriteClasses(FILE* p_fp,bool p_trace,ClassMap& p_map)
{
  // Write number of classes
  int num = (int)m_classes.size();
  WriteInteger(p_fp,p_trace,num);
  TracingText(p_trace,"CLASSES (Size: %d)",num);

  // Write out all classes
  for(auto& cl : m_classes)
  {
    WriteClass(p_fp,p_trace,cl.second);
  }
  TracingText(p_trace,"END OF CLASSES");
}

void
QLVirtualMachine::WriteNameMap(FILE* p_fp,bool p_trace,NameMap& p_map,const char* p_name,bool p_doScripts)
{
  int size = 0;

  // Count the number of objects to write
  for(auto& ob : p_map)
  {
    if(ob.second->m_type != DTYPE_INTERNAL)
    {
      ++size;
    }
  }

  // Getting the name
  CString mapname(p_name);
  mapname.AppendFormat(" (Size: %d)",size);

  // Write size of the map
  WriteInteger(p_fp,p_trace,size);
  TracingText(p_trace,mapname);

  // Write the map
  for(auto& ob : p_map)
  {
    if(ob.second->m_type != DTYPE_INTERNAL)
    {
      // Only write the object, if it is not an internal C++ object
      // Those will always be implicitly loaded by the QL_Functions module
      WriteMemObject(p_fp,p_trace,ob.second,p_doScripts);
    }
  }

  mapname = CString("END OF ") + p_name;
  TracingText(p_trace,mapname);
}
