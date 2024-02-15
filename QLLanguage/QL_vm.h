//////////////////////////////////////////////////////////////////////////
//
// QL Language virtual machine
// ir W.E. Huisman (c) 2018
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "QL_Language.h"
#include "QL_Objects.h"

// Values for the GC alloc counter
#define THRESHOLD_DEFAULT      1000
#define THRESHOLD_AGGRESIVE     100
#define THRESHOLD_RELAXED  10000000

// Forward declarations
class QLCompiler;
class QLInterpreter;

using SQLComponents::SQLTransaction;

class QLVirtualMachine
{
public:
  QLVirtualMachine();
 ~QLVirtualMachine();

  // Check initialization
  void        CheckInit();
  // Compile a QL source code file into this VM
  bool        CompileFile(LPCTSTR p_filename,bool p_trace);
  bool        CompileBuffer(LPCTSTR p_buffer,bool p_trace);

  // SetInterpreter
  void        SetInterpreter(QLInterpreter* p_inter);
  // Setting the alloc threshold for the GC
  void        SetGCThreshold(int p_threshold);
  // Setting the dumping of the object chain
  void        SetDumping(bool p_dump);
  // Garbage Collection
  void        GC();
  // Error handling for all objects
  static void Error(LPCTSTR p_format, ...);
  // Printing of information
  void        Info(LPCTSTR p_format, ...);
  // Print - print one value 
  int         Print(FILE* p_fp,int p_quoteFlag,MemObject* p_value);

  // Getters
  NameMap&    GetSymbols();
  NameMap&    GetScripts();

  // MEMORY API AND GC
  MemObject*  AllocMemObject(int type,bool p_running = true);
  MemObject*  AllocMemObject(const MemObject* p_other);
  void        FreeMemObject(MemObject* p_object,bool p_running = true);
  void        MemObjectSetType(MemObject* p_object, int p_type);
  void        MarkObject(MemObject* p_object);

  // CLASSES SYMBOLS GLOBALS AND SCRIPTS
  Class*      FindClass  (CString& p_name);
  Function*   AddScript  (CString  p_name);
  MemObject*  AddInternal(CString p_name);
  MemObject*  FindSymbol (CString p_name);
  Function*   FindScript (CString p_name);
  Method*     AddMethod  (CString p_name,int p_type);
  Method*     FindMethod (CString p_name,int p_type);

  // Add an entry to a dictionary 
  MemObject*  AddEntry(NameMap& dict,CString p_key,int p_storage);
  // Adding various objects
  MemObject*  AddSymbol (CString p_name);
  void        AddClass  (Class* p_class);
  int         AddGlobal (MemObject* p_object,CString p_name);
  void        AddLiteral(MemObject* p_object);
  void        AddBytecode(BYTE* p_bytecode,unsigned p_size);
  // XTOR and DTOR an object
  MemObject*  NewObject(Class* p_class);
  int         DestroyObject(MemObject* p_object);

  MemObject*  GetGlobal(unsigned p_index);
  void        SetGlobal(unsigned p_index,MemObject* p_object);
  MemObject*  GetLiteral(unsigned p_index);
  BYTE*       GetBytecode();
  int         FindGlobal(CString p_name);
  CString     FindSymbolName(MemObject* p_object);
  bool        HasInitCode();

  // FILE STREAMING OPERATIONS
  bool        WriteFile(TCHAR* p_filename,bool p_trace);
  bool        LoadFile (TCHAR* p_filename,bool p_trace);

  // Test for types of files
  bool        IsObjectFile(const TCHAR* p_filename);
  bool        IsSourceFile(const TCHAR* p_filename);

  // SQLTransaction
  int             SetSQLTransaction(SQLTransaction* p_trans);
  SQLTransaction* GetSQLTransaction();

private:
  // Garbage collector sub-functions
  void        MarkClasses();
  void        MarkMap(NameMap& p_map);
  void        RemoveUnmarked();
  void        DestroyObjectChain();
  void        CleanUpClasses();
  void        CleanUpGlobals();
  void        CleanUpLiterals();
  void        CleanUpMethods();
  void        CleanUpInitcode();
  void        DumpObject(MemObject* p_object);

  void        TracingText(bool p_trace,const TCHAR* p_text,...);

  // Writing the heap to file
  bool        WriteToFile   (FILE* p_fp, bool p_trace);
  int         Putc  (int cc, FILE* p_fp, bool p_trace);
  void        WriteHeader   (FILE* p_fp, bool p_trace);
  void        WriteStream   (FILE* p_fp, bool p_trace, TCHAR* p_message);
  void        WriteInteger  (FILE* p_fp, bool p_trace, int n);
  void        WriteString   (FILE* p_fp, bool p_trace, CString*   p_string,   TCHAR* p_extra = nullptr);
  void        WriteFloat    (FILE* p_fp, bool p_trace, bcd*       p_float);
  void        WriteFileName (FILE* p_fp, bool p_trace, MemObject* p_object);
  void        WriteArray    (FILE* p_fp, bool p_trace, Array*     p_array,    TCHAR* p_extra = nullptr,bool p_doScripts = false);
  void        WriteObject   (FILE* p_fp, bool p_trace, Object*    p_object);
  void        WriteClass    (FILE* p_fp, bool p_trace, Class*     p_class);
  void        WriteBytecode (FILE* p_fp, bool p_trace, BYTE*      p_bytecode, int p_length);
  void        WriteScript   (FILE* p_fp, bool p_trace, Function*  p_script);
  void        WriteTypes    (FILE* p_fp, bool p_trace, ArgTypes&  p_types);
  void        WriteInternal (FILE* p_fp, bool p_trace, MemObject* p_internal);
  void        WriteExternal (FILE* p_fp, bool p_trace, CString*   p_external);
  void        WriteReference(FILE* p_fp, bool p_trace, MemObject* p_object);
  void        WriteMemObject(FILE* p_fp, bool p_trace, MemObject* p_object,bool p_ref = false);
  void        WriteClasses  (FILE* p_fp, bool p_trace, ClassMap&  p_map);
  void        WriteNameMap  (FILE* p_fp, bool p_trace, NameMap&   p_map,const TCHAR* p_name,bool p_doScripts);

  // Reading into the heap from a file
  bool        ReadFromFile    (FILE* p_fp, bool p_trace);
  int         Getc            (FILE* p_fp, bool p_trace);
  bool        ReadHeader      (FILE* p_fp, bool p_trace);
  void        ReadStream      (FILE* p_fp, bool p_trce,  TCHAR* p_errorMessage);
  bool        ReadInteger     (FILE* p_fp, bool p_trace, long* n);
  bool        MustReadInteger (FILE* p_fp, bool p_trace, long* n, TCHAR* p_errorMessage);
  CString     ReadString      (FILE* p_fp, bool p_trace);
  CString     MustReadString  (FILE* p_fp, bool p_trace, TCHAR* p_errorMessage);
  bcd*        ReadFloat       (FILE* p_fp, bool p_trace);
  bcd*        MustReadFloat   (FILE* p_fp, bool p_trace, TCHAR* p_message);
  FILE*       ReadFileName    (FILE* p_fp, bool p_trace);
  Array*      ReadArray       (FILE* p_fp, bool p_trace, Array* p_array = nullptr,TCHAR* p_name = nullptr);
  Array*      MustReadArray   (FILE* p_fp, bool p_trace, Array* p_array = nullptr,TCHAR* p_name = nullptr);
  Object*     ReadObject      (FILE* p_fp, bool p_trace);
  Class*      ReadClass       (FILE* p_fp, bool p_trace);
  void        ReadBytecode    (FILE* p_fp, bool p_trace, BYTE** p_bytecode,int* p_size);
  Function*   ReadScript      (FILE* p_fp, bool p_trace);
  Internal    ReadInternal    (FILE* p_fp, bool p_trace);
  CString*    ReadExternal    (FILE* p_fp, bool p_trace);
  void        ReadReference   (FILE* p_fp, bool p_trace, MemObject* p_object);
  MemObject*  ReadMemObject   (FILE* p_fp, bool p_trace);
  void        ReadClasses     (FILE* p_fp, bool p_trace, ClassMap& p_map);
  void        ReadNameMap     (FILE* p_fp, bool p_trace, NameMap&  p_map,TCHAR* p_name);

  // Thunking to be done after a file load
  void        Thunking();

  // Globals
  ClassMap    m_classes;   // All defined script classes
  NameMap     m_symbols;   // Static symbols defined
  Array*      m_globals;   // Global variables for the scripts
  Array*      m_literals;  // Literals for globals
  NameMap     m_scripts;   // All defined script functions, including "main()"
  MethodMap   m_methods;   // All internal defined methods for internal datatypes
  BYTE*       m_initcode;  // Code to run before the entrypoint
  int         m_initcode_size;

  // The object chain for gc
  MemObject*  m_root_object;
  MemObject*  m_last_object;
  // Number of memory allocations for GC
  int         m_allocs;
  // After this number of allocations, a GC is forced
  int         m_threshold;
  // Debug printing
  bool        m_dumpchain;
  int         m_position;

  // The current interpreter
  QLInterpreter* m_interpreter;

  // The one-and-only SQL Transaction
  SQLTransaction* m_transaction;

  CRITICAL_SECTION m_lock;
};

inline NameMap&
QLVirtualMachine::GetSymbols()
{
  return m_symbols;
}

inline NameMap&
QLVirtualMachine::GetScripts()
{
  return m_scripts;
}

inline void
QLVirtualMachine::SetInterpreter(QLInterpreter* p_inter)
{
  m_interpreter = p_inter;
}

inline void
QLVirtualMachine::SetDumping(bool p_dump)
{
  m_dumpchain = p_dump;
}

inline BYTE*
QLVirtualMachine::GetBytecode()
{
  return m_initcode;
}

inline bool
QLVirtualMachine::HasInitCode()
{
  return m_initcode != nullptr;
}
