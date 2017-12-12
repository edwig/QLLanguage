//////////////////////////////////////////////////////////////////////////
//
// Q++ Language header
//
// 2014 (c) ir. W.E. Huisman
//
//////////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"
#include "bcd.h"
#include <map>
#include <vector>
#include <SQLDatabase.h>
#include <SQLQuery.h>
#include <SQLVariant.h>

// VERSION OF QL LANGUAGE
// USED IN *.qob FILES
#define QL_VERSION        200 // 2.00

#define QUANTUM_PROMPT    "Quantum Language (c) 2014-2017 ir. W.E. Huisman"
#define QUANTUM_VERSION   "2.0"

// Tracing macro
#define tracing(x)          if(p_trace) fprintf(stderr,x)
#define tracingx(x,...)     if(p_trace) fprintf(stderr,x,__VA_ARGS__)

// Maximum size of stack up to 0xFFFF
#define STACK_MAX         0x7FFF // 32767
#define STACK_DEFAULT     0x07D0 //  2000

// Datatypes for QL
#define DTYPE_ENDMARK     0x0001
#define DTYPE_NIL         0x0002
#define DTYPE_INTEGER     0x0003
#define DTYPE_STRING      0x0004
#define DTYPE_BCD         0x0005
#define DTYPE_FILE        0x0006
#define DTYPE_DATABASE    0x0007
#define DTYPE_QUERY       0x0008
#define DTYPE_VARIANT     0x0009
#define DTYPE_ARRAY       0x000A
#define DTYPE_OBJECT      0x000B
#define DTYPE_CLASS       0x000C
#define DTYPE_SCRIPT      0x000D
#define DTYPE_INTERNAL    0x000E
#define DTYPE_EXTERNAL    0x000F
#define DTYPE_STREAM      0x0010
// Added to type on storage of a FLAG_REFERENCE object 
// in a file stream (e.g. on disk) (String is the name)
#define DTYPE_REFERENCE   0x0080
#define DTYPE_MASK        0x007F

// Minimum and maximum datatype
#define _DTMIN   DTYPE_ENDMARK
#define _DTMAX   DTYPE_EXTERNAL

// Type flags
#define FLAG_DEALLOC      0x0001    // Object should be deallocated on free
#define FLAG_NULL         0x0002    // Object is logical NULL
#define FLAG_REFERENCE    0x0004    // Object is not garbage collected (REFERENCE!!)

// GC Generation marks
#define GC_ALIVE          0x0001
#define GC_MARKED         0x0002
#define GC_SWEEP          0x0004

// Storage class symbol types 
#define ST_CLASS	    1	  // Class definition       lives in m_classes
#define ST_DATA		    2	  // data member            lives in m_classes class.attributes
#define ST_SDATA	    3	  // static data member     lives in m_globals
#define ST_FUNCTION	  4	  // function member        lives in m_classes class.members
#define ST_SFUNCTION	5	  // static function member lives in m_symbols/m_globals

// Forward declarations for many objects
class Array;
class Class;
class Object;
class Function;
class QLVirtualMachine;
class QLInterpreter;

// Shortcut for many defines
#define QLvm  QLVirtualMachine

// Types for the MemObject
typedef int (*Internal)(QLInterpreter*,int);
typedef unsigned char shortint;

// Method for internal data types
typedef struct _method
{
  int       m_datatype;
  CString   m_methodname;
  Internal  m_internal;
}
Method;

// General memory object for use in all modules
// This class structure is publicly available

class MemObject
{
public:
  MemObject();
  MemObject(MemObject* p_orig);
 ~MemObject();
  
  // Assignment operator
  MemObject& operator=(const MemObject& p_other);
  
  // Put a type in the object or clear it again
  void  AllocateType(int p_type);
  void  DeAllocate();
  bool  IsMarked();

  // DATA STRUCTUUR

  shortint        m_type;           // Datatype of the object (DTYPE_XXX)
  shortint        m_generation;     // Garbage collector generation marks (GC_XXX)
  shortint        m_flags;          // Type and optimization flags (FLAG_XXX)
  shortint        m_storage;        // Class storage type (static/local data/function)
  union _value
  {
    UINT_PTR      v_all;            // Used if accessed as a memobject, instead of a type
    int			      v_integer;        // DTYPE_INTEGER  value
    CString*      v_string;         // DTYPE_STRING   value
    bcd*          v_floating;       // DTYPE_BCD      value
    FILE*         v_file;           // DTYPE_FILE     value
    Array*        v_array;          // DTYPE_ARRAY    value
    Object*       v_object;         // DTYPE_OBJECT   value
    Class*        v_class;          // DTYPE_CLASS    value
    Function*     v_script;	        // DTYPE_SCRIPT   Internal compiled script function
    Internal      v_internal;       // DTYPE_INTERNAL Internal C++ function
    CString*      v_sysname;        // DTYPE_EXTERNAL External PInvoke function
    SQLDatabase*  v_database;       // DTYPE_DATABASE 
    SQLQuery*     v_query;          // DTYPE_QUERY
    SQLVariant*   v_variant;        // DTYPE_VARIANT
  }
  m_value;

private:
  // For use with the GC Garbage Collector
  MemObject*      m_next;		        // Link to next object
  MemObject*      m_prev;           // Link to prev object
  friend          QLVirtualMachine;
};

inline bool
MemObject::IsMarked()
{
  return (m_generation & GC_MARKED) != 0;
}

// Name mapping for global objects in the virtual machine
typedef std::map<CString, MemObject*>     NameMap;
typedef std::map<CString, Class*>         ClassMap;
typedef std::multimap<CString, Method*>   MethodMap;
