//////////////////////////////////////////////////////////////////////////
//
// VARIOUS OBJECT FOR THE Q++ Scripting language
// ir W.E. Huisman (c) 2018
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "QL_Language.h"
#include <vector>

class QLVirtualMachine;

typedef std::vector<MemObject*> Members;
typedef std::vector<int>        ArgTypes;

// Finding your datatype name with DTYPE_* macros
extern char* datatype_names[];

class Array
{
public:
  Array();
  Array(QLvm* p_vm,int p_size);
 ~Array();

  MemObject*   FindEntry(CString p_name);
  MemObject*   FindEntry(CString p_name,int& p_entryNum);
  int          FindEntry(MemObject* p_object);
  MemObject*   FindFuncEntry(CString p_name);
  int          FindStringEntry(CString p_name);
  int          FindIntegerEntry(int p_value);

  // Add an array member (all datamembers)
  MemObject*   AddEntry(QLvm* p_vm,CString p_string);
  MemObject*   AddEntry(QLvm* p_vm,int     p_integer);
  MemObject*   AddEntry(MemObject* p_memob);
  MemObject*   AddEntryOfType(QLvm* p_vm,int p_type);

  // Getters
  MemObject*   GetEntry(unsigned p_number);
  int          GetSize();
  // Setters
  void         SetEntry(unsigned p_number,MemObject* p_object);
  // Garbage collection
  void         Mark(QLvm* p_vm);
private:
  Members      m_members;
};

class Function
{
public:
  Function();
  Function(CString p_name);
 ~Function();

  // Operations
  void        AddArgument(int p_type);
  void        AddLiteral(QLvm* p_vm,CString p_literal);
  void        AddLiteral(QLvm* p_vm,MemObject* p_object);
  void        SetBytecode(BYTE* p_bytecode,unsigned p_size);
  void        SetName(CString p_name);
  void        SetClass(Class* p_class);
  void        SetWriting(bool p_writing);

  // Getters
  CString     GetName();
  CString     GetFullName();
  Class*      GetClass();
  int         GetArgument(int p_arg);
  int         GetNumberOfArguments();
  BYTE*       GetBytecode();
  int         GetBytecodeSize();
  bool        GetWriting();
  MemObject*  GetLiteral      (unsigned p_number);
  CString     GetLiteralString(unsigned p_number);
  Array*      GetLiterals();
  int         GetLiteralsSize();
  ArgTypes&   GetArgumentTypes();

  // Setters
  void        SetLiteral(unsigned p_number,MemObject* p_object);
  void        SetLiterals(Array* p_literals);

  // Garbage collection
  void        Mark(QLvm* p_vm);
private:
  Class*      m_class;
  CString     m_name;
  ArgTypes    m_arguments;
  int         m_bytecode_size;
  BYTE*       m_bytecode;
  Array*      m_literals;
  // Non-recursive writing of the object file
  bool        m_writing;
};

class Class
{
public:
  Class(CString p_name);
  Class(CString p_name,Class* p_base);
 ~Class();

  // Add members
  MemObject*  AddDataMember    (QLvm* p_vm,CString p_name,int p_storage);
  MemObject*  AddFunctionMember(QLvm* p_vm,CString p_name,int p_storage);

  // Find Operations
  MemObject*  FindMember(CString p_name);
  MemObject*  FindFuncMember(CString p_name);
  MemObject*  FindDataMember(CString p_name);
  MemObject*  RecursiveFindMember(CString p_name);
  MemObject*  RecursiveFindFuncMember(CString p_name);
  MemObject*  RecursiveFindDataMember(CString p_name);
  MemObject*  RecursiveFindDataMember(CString p_name,int& p_entryNum);

  // Setters
  void        SetName(CString p_name);
  void        SetSize(unsigned p_size);
  void        SetBaseClass(Class* p_base);
  // Getters
  CString     GetName();
  Class*      GetBaseClass();
  Array&      GetMembers();
  Array&      GetAttributes();
  unsigned    GetSize();
  // Garbage collection
  void        Mark(QLvm* p_vm);

private:
  CString     m_name;         // Our class name
  Class*      m_base;         // Pointer to the base class
  Array       m_members;      // Member functions
  unsigned    m_size;         // Size of attributes, including baseclass attributes
  Array       m_attributes;   // Attributes of this derived class only
};

class Object
{
public:
  Object();
  Object(Class* p_class);
 ~Object();
 // Init the object
 void         Init(QLvm* p_vm,Class* p_class);

  // Operational use of the object
  Class*      GetClass();
  MemObject*  GetAttribute(int p_index);
  bool        SetAttribute(int p_index,MemObject* p_attrib);
  // Garbage collector
  void        Mark(QLvm* p_vm);
private:
  Class*      m_class;
  Array       m_attributes;   // m_class->m_size of attributes
};

