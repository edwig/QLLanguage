//////////////////////////////////////////////////////////////////////////
//
// MEMORY OBJECT FOR THE Q++ Scripting language
// Implements garbage collectable memory objects
//
// ir W.E. Huisman (c) 2018
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "QL_Language.h"

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

  // DATA STRUCTURE

  shortint        m_type;           // Datatype of the object (DTYPE_XXX)
  shortint        m_generation;     // Garbage collector generation marks (GC_XXX)
  shortint        m_flags;          // Type and optimization flags (FLAG_XXX)
  shortint        m_storage;        // Class storage type (static/local data/function)
  union _value
  {
    UINT_PTR      v_all;            // Used if accessed as a memory object, instead of a type
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

