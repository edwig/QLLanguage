//////////////////////////////////////////////////////////////////////////
//
// VARIOUS OBJECT FOR THE Q++ Scripting language
// ir W.E. Huisman (c) 2018
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "QL_Language.h"
#include "QL_MemObject.h"
#include "QL_Objects.h"
#include "QL_vm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Finding your datatype name with DTYPE_* macros
char* datatype_names[0x11]
{
   ""
  ,"ENDMARK"
  ,"NIL"
  ,"INTEGER"
  ,"STRING"
  ,"BCD"
  ,"FILE"
  ,"DATABASE"
  ,"QUERY"
  ,"VARIANT"
  ,"ARRAY"
  ,"OBJECT"
  ,"CLASS"
  ,"SCRIPT"
  ,"INTERNAL"
  ,"EXTERNAL"
  ,"STREAM"
};

//////////////////////////////////////////////////////////////////////////
//
// The MemoryObject
//
//////////////////////////////////////////////////////////////////////////

MemObject::MemObject()
{
  m_next = m_prev  = nullptr;
  m_type = m_flags = m_generation = m_storage = 0;
  m_value.v_all    = NULL;
}

MemObject::MemObject(MemObject* p_orig)
{
  *this = *p_orig;
}

MemObject::~MemObject()
{
  DeAllocate();
}

MemObject& 
MemObject::operator=(const MemObject& p_other)
{
  // Retain the rootchain
  m_next       = p_other.m_next;
  m_prev       = p_other.m_prev;
  // General flags are copied and the fact that's a REFERENCE
  // is append here, so the objects will not be deleted
  m_type       = p_other.m_type;
  m_flags      = p_other.m_flags | FLAG_REFERENCE;
  m_generation = p_other.m_generation;
  m_storage    = p_other.m_storage;

  // Copy the contents OR pointer to *some object*
  m_value.v_all = p_other.m_value.v_all;

  return *this;
}

void
MemObject::AllocateType(int p_type)
{
  switch(p_type)
  {
    case DTYPE_ENDMARK:     break;
    case DTYPE_NIL:         break;
    case DTYPE_INTEGER:     break;
    case DTYPE_STRING:      m_value.v_string = new CString();
                            m_flags |= FLAG_DEALLOC;
                            break;
    case DTYPE_BCD:         m_value.v_floating = new bcd();
                            m_flags |= FLAG_DEALLOC;
                            break;
    case DTYPE_FILE:        break;
    case DTYPE_ARRAY:       m_value.v_array = new Array();
                            m_flags |= FLAG_DEALLOC;
                            break;
    case DTYPE_OBJECT:      m_value.v_object = new Object();
                            m_flags |= FLAG_DEALLOC;
                            break;
    case DTYPE_SCRIPT:      m_value.v_script = new Function();
                            m_flags |= FLAG_DEALLOC;
                            break;
    case DTYPE_INTERNAL:    break;
    case DTYPE_EXTERNAL:    m_value.v_sysname = new CString();
                            m_flags |= FLAG_DEALLOC;
                            break;
    case DTYPE_DATABASE:    m_value.v_database = new SQLDatabase();
                            m_flags |= FLAG_DEALLOC;
                            break;
    case DTYPE_QUERY:       m_value.v_query = new SQLQuery();
                            m_flags |= FLAG_DEALLOC;
                            break;
    case DTYPE_VARIANT:     m_value.v_variant = new SQLVariant();
                            m_flags |= FLAG_DEALLOC;
                            break;
    default:                QLvm::Error("INTERNAL: Unknown datatype in AllocMemObject: %d",p_type);
  }
  m_type = p_type;
}

void
MemObject::DeAllocate()
{
  if((m_flags & FLAG_REFERENCE) == 0)
  {
    switch(m_type)
    {
      case DTYPE_ENDMARK: // fall through
      case DTYPE_NIL:     // fall through
      case DTYPE_INTEGER: break;
      case DTYPE_STRING:  delete m_value.v_string;      break;
      case DTYPE_BCD:     delete m_value.v_floating;    break;
      case DTYPE_FILE:    break;
      case DTYPE_DATABASE:delete m_value.v_database;    break;
      case DTYPE_QUERY:   delete m_value.v_query;       break;
      case DTYPE_VARIANT: delete m_value.v_variant;     break;
      case DTYPE_ARRAY:   delete m_value.v_array;       break;
      case DTYPE_OBJECT:  delete m_value.v_object;      break;
      case DTYPE_CLASS:   break; // Never reached
      case DTYPE_SCRIPT:  delete m_value.v_script;      break;
      case DTYPE_INTERNAL:break; // Never reached
      case DTYPE_EXTERNAL:delete m_value.v_sysname;     break;
      default:            QLvm::Error("INTERNAL: Unknown datatype in DeAllocate: %d",m_type);
    }
    // Reset the pointer value
    m_value.v_all = NULL;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// ARRAY
//
//////////////////////////////////////////////////////////////////////////

Array::Array()
{
}

Array::Array(QLvm* p_vm,int p_size)
{
  for(int ind = 0; ind < p_size; ++ind)
  {
    m_members.push_back(p_vm->AllocMemObject(DTYPE_NIL));
  }
}

Array::~Array()
{
}

MemObject*   
Array::AddEntry(QLvm* p_vm,CString p_string)
{
  MemObject* entry = p_vm->AllocMemObject(DTYPE_STRING);
  m_members.push_back(entry);
  *(entry->m_value.v_string) = p_string;
  return entry;
}

MemObject*   
Array::AddEntry(QLvm* p_vm,int p_integer)
{
  MemObject* entry = p_vm->AllocMemObject(DTYPE_INTEGER);
  m_members.push_back(entry);
  entry->m_value.v_integer = p_integer;
  return entry;
}

// General typed entry

MemObject*   
Array::AddEntry(MemObject* p_memob)
{
  m_members.push_back(p_memob);
  return p_memob;
}

MemObject*
Array::AddEntryOfType(QLvm* p_vm,int p_type)
{
  MemObject* entry = p_vm->AllocMemObject(p_type);
  m_members.push_back(entry);
  return entry;
}

int
Array::FindStringEntry(CString p_name)
{
  for(unsigned ind = 0;ind < m_members.size(); ++ind)
  {
    if(m_members[ind]->m_type == DTYPE_STRING)
    {
      if(m_members[ind]->m_value.v_string->Compare(p_name) == 0)
      {
        return ind;
      }
    }
  }
  return -1;
}

int
Array::FindIntegerEntry(int p_value)
{
  for(unsigned ind = 0;ind < m_members.size(); ++ind)
  {
    if(m_members[ind]->m_type == DTYPE_INTEGER)
    {
      if(m_members[ind]->m_value.v_integer == p_value)
      {
        return ind;
      }
    }
  }
  return -1;
}

// FInd by object reference (literals e.g.)
int
Array::FindEntry(MemObject* p_object)
{
  for(unsigned ind = 0;ind < m_members.size(); ++ind)
  {
    if(m_members[ind] == p_object)
    {
      return ind;
    }
  }
  return -1;
}

MemObject*
Array::FindEntry(CString p_name)
{
  for (unsigned ind = 0; ind < m_members.size(); ++ind)
  {
    switch (m_members[ind]->m_type)
    {
      case DTYPE_STRING:  if (m_members[ind]->m_value.v_string->Compare(p_name) == 0)
                          {
                            return m_members[ind];
                          }
                          break;
      case DTYPE_SCRIPT:  if (m_members[ind]->m_value.v_script->GetName().Compare(p_name) == 0)
                          {
                            return m_members[ind];
                          }
                          break;
    }
  }
  return nullptr;
}

MemObject*
Array::FindFuncEntry(CString p_name)
{
  for(unsigned ind = 0; ind < m_members.size(); ++ind)
  {
    if(m_members[ind]->m_type == DTYPE_SCRIPT)
    {
      if(m_members[ind]->m_value.v_script->GetName().Compare(p_name) == 0)
      {
        return m_members[ind];
      }
    }
  }
  return nullptr;
}


MemObject*
Array::FindEntry(CString p_name,int& p_entryNum)
{
  for(unsigned ind = 0;ind < m_members.size(); ++ind)
  {
    switch(m_members[ind]->m_type)
    {
      case DTYPE_STRING:  if(m_members[ind]->m_value.v_string->Compare(p_name) == 0)
                          {
                            p_entryNum = ind;
                            return m_members[ind];
                          }
                          break;
      case DTYPE_SCRIPT:  if(m_members[ind]->m_value.v_script->GetName().Compare(p_name) == 0)
                          {
                            p_entryNum = ind;
                            return m_members[ind];
                          }
                          break;
    }
  }
  return nullptr;
}

MemObject*   
Array::GetEntry(unsigned p_number)
{
  if(m_members.size() > p_number)
  {
    return m_members[p_number];
  }
  return nullptr;
}

int
Array::GetSize()
{
  return (int)m_members.size();
}

void
Array::SetEntry(unsigned p_number,MemObject* p_object)
{
  if(p_number >= 0 && p_number < m_members.size())
  {
    m_members[p_number] = p_object;
  }
}

void
Array::Mark(QLvm* p_vm)
{
  for(unsigned ind = 0;ind < m_members.size(); ++ind)
  {
    if(m_members[ind]->IsMarked() == false)
    {
      p_vm->MarkObject(m_members[ind]);
    }
  }
}


//////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//////////////////////////////////////////////////////////////////////////

Class::Class(CString p_name)
      :m_name(p_name)
      ,m_base(nullptr)
      ,m_size(0)
{
}

Class::Class(CString p_name, Class* p_base)
      :m_name(p_name)
      ,m_base(p_base)
      ,m_size(0)
{

}

Class::~Class()
{
}

MemObject*
Class::AddDataMember(QLvm* p_vm,CString p_name,int p_storage)
{
  if(p_storage == ST_SDATA)
  {
    // Check that global does not already exists
    if(p_vm->FindGlobal(p_name) >= 0)
    {
      return nullptr;
    }
  }

  MemObject* member = m_attributes.AddEntry(p_vm,p_name);
  member->m_storage = p_storage;
  // One more data member in this class
  ++m_size;

  return member;
}

MemObject*
Class::AddFunctionMember(QLvm* p_vm,CString p_name,int p_storage)
{
  // See if it was already added
  MemObject* member = FindFuncMember(p_name);
  if(member)
  {
    return member;
  }
  member = m_members.AddEntryOfType(p_vm,DTYPE_SCRIPT);
  Function*  functn = member->m_value.v_script;
  functn->SetName(p_name);
  functn->SetClass(this);
  member->m_storage = p_storage;

  return member;
}

// Setters
void    
Class::SetName(CString p_name)
{
  m_name = p_name;
}

void
Class::SetSize(unsigned p_size)
{
  m_size = p_size;
}

void
Class::SetBaseClass(Class* p_base)
{
  // but only if not yet set
  if(m_base == nullptr)
  {
    m_base = p_base;
  }
}

// Getters
CString 
Class::GetName()
{
  return m_name;
}

Class*  
Class::GetBaseClass()
{
  return m_base;
}

Array&  
Class::GetMembers()
{
  return m_members;
}

unsigned 
Class::GetSize()
{
  return m_size;
}

Array&  
Class::GetAttributes()
{
  return m_attributes;
}

MemObject*  
Class::FindMember(CString p_name)
{
  MemObject* member = m_members.FindEntry(p_name);
  if(member != nullptr)
  {
    return member;
  }
  return m_attributes.FindEntry(p_name);
}

MemObject*
Class::FindFuncMember(CString p_name)
{
  return m_members.FindFuncEntry(p_name);
}

MemObject*  
Class::FindDataMember(CString p_name)
{
  return m_members.FindEntry(p_name);
}

MemObject*
Class::RecursiveFindMember(CString p_name)
{
  MemObject* member = FindMember(p_name);
  if(member != nullptr)
  {
    return member;
  }
  if(m_base != nullptr)
  {
    member = m_base->RecursiveFindMember(p_name);
  }
  return member;
}

MemObject*
Class::RecursiveFindFuncMember(CString p_name)
{
  MemObject* entry = m_members.FindEntry(p_name);
  if(entry == nullptr && m_base)
  {
    return m_base->RecursiveFindFuncMember(p_name);
  }
  return entry;
}

MemObject*  
Class::RecursiveFindDataMember(CString p_name)
{
  MemObject* member = FindDataMember(p_name);
  if(member == nullptr && m_base)
  {
    return m_base->RecursiveFindDataMember(p_name);
  }
  return member;
}

MemObject*  
Class::RecursiveFindDataMember(CString p_name,int& p_entryNum)
{
  p_entryNum = -1;
  MemObject* entry = m_attributes.FindEntry(p_name,p_entryNum);
  if(entry)
  {
    int offset  = m_size - m_attributes.GetSize();
    p_entryNum += offset;
    return entry;
  }
  if(m_base)
  {
    return m_base->RecursiveFindDataMember(p_name,p_entryNum);
  }
  return nullptr;
}

void
Class::Mark(QLvm* p_vm)
{
  for(int ind = 0;ind < m_members.GetSize(); ++ind)
  {
    if(m_members.GetEntry(ind)->IsMarked() == false)
    {
      p_vm->MarkObject(m_members.GetEntry(ind));
    }
  }
  for(int ind = 0;ind < m_attributes.GetSize(); ++ind)
  {
    if(m_attributes.GetEntry(ind)->IsMarked() == false)
    {
      p_vm->MarkObject(m_attributes.GetEntry(ind));
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//////////////////////////////////////////////////////////////////////////

Function::Function()
         :m_literals(nullptr)
         ,m_class(nullptr)
         ,m_bytecode(nullptr)
         ,m_bytecode_size(0)
         ,m_writing(false)
{
}

Function::Function(CString p_name)
         :m_name(p_name)
         ,m_literals(nullptr)
         ,m_class(nullptr)
         ,m_bytecode(nullptr)
         ,m_bytecode_size(0)
         ,m_writing(false)
{
}

Function::~Function()
{
  if(m_bytecode)
  {
    free(m_bytecode);
    m_bytecode = 0;
    m_bytecode_size = 0;
  }
  if(m_literals)
  {
    delete m_literals;
    m_literals = nullptr;  
  }
}

// Operations
void
Function::AddArgument(int p_type)
{
  m_arguments.push_back(p_type);
}

void    
Function::AddLiteral(QLvm* p_vm,CString p_literal)
{
  if(m_literals == nullptr)
  {
    m_literals = new Array();
  }
  m_literals->AddEntry(p_vm,p_literal);
}

void
Function::AddLiteral(QLvm* p_vm, MemObject* p_object)
{
  if (m_literals == nullptr)
  {
    m_literals = new Array();
  }
  m_literals->AddEntry(p_object);
}

void    
Function::SetBytecode(BYTE* p_bytecode, unsigned p_size)
{
  m_bytecode = (BYTE*) malloc(p_size + 1);
  memcpy(m_bytecode,p_bytecode,p_size);
  m_bytecode[m_bytecode_size = p_size] = 0;
}

void    
Function::SetName(CString p_name)
{
  m_name = p_name;
}

void
Function::SetClass(Class* p_class)
{
  m_class = p_class;
}

// Getters
CString
Function::GetName()
{
  return m_name;
}

CString
Function::GetFullName()
{
  if(m_class)
  {
    return m_class->GetName() + "::" + m_name;
  }
  return m_name;
}


Class*
Function::GetClass()
{
  return m_class;
}

int
Function::GetArgument(int p_arg)
{
  if(p_arg >= 0 && p_arg < (int)m_arguments.size())
  {
    return m_arguments[p_arg];
  }
  return DTYPE_NIL;
}

int
Function::GetNumberOfArguments()
{
  return (int)m_arguments.size();
}

BYTE*   
Function::GetBytecode()
{
  return m_bytecode;
}

int
Function::GetBytecodeSize()
{
  return m_bytecode_size;
}

MemObject*
Function::GetLiteral(unsigned p_number)
{
  if (m_literals)
  {
    return m_literals->GetEntry(p_number);
  }
  return nullptr;
}

CString
Function::GetLiteralString(unsigned p_number)
{
  if(m_literals)
  {
    MemObject* object = m_literals->GetEntry(p_number);
    if(object && object->m_type == DTYPE_STRING)
    {
      return *(object->m_value.v_string);
    }
  }
  return CString("");
}

Array*  
Function::GetLiterals()
{
  return m_literals;
}

int     
Function::GetLiteralsSize()
{
  if(m_literals)
  {
    return (int)m_literals->GetSize();
  }
  return 0;
}

ArgTypes& 
Function::GetArgumentTypes()
{
  return m_arguments;
}

void
Function::SetLiteral(unsigned p_number, MemObject* p_object)
{
  if(m_literals)
  {
    m_literals->SetEntry(p_number,p_object);
  }
}

void
Function::SetLiterals(Array* p_literals)
{
  if(m_literals)
  {
    delete m_literals;
  }
  m_literals = p_literals;
}

void
Function::Mark(QLvm* p_vm)
{
  if(m_literals)
  {
    m_literals->Mark(p_vm);
  }
}

void
Function::SetWriting(bool p_writing)
{
  m_writing = p_writing;
}

bool
Function::GetWriting()
{
  return m_writing;
}

//////////////////////////////////////////////////////////////////////////
//
// OBJECT
//
//////////////////////////////////////////////////////////////////////////

Object::Object()
{
}

Object::Object(Class* p_class)
       :m_class(p_class)
{
}

Object::~Object()
{
}

void
Object::Init(QLvm* p_vm,Class* p_class)
{
  // Find a base class if any
  Class* base = p_class->GetBaseClass();

  // Try base classes first
  if(base)
  {
    Init(p_vm,base);
  }
  // Remember our class
  m_class = p_class;


  // Add all data members to the object
  Array& members = p_class->GetAttributes();
  for(int ind = 0;ind < members.GetSize(); ++ind)
  {
    MemObject* member = p_vm->AllocMemObject(members.GetEntry(ind)->m_type);
    m_attributes.AddEntry(member);
  }
}

Class*
Object::GetClass()
{
  return m_class;
}

MemObject*
Object::GetAttribute(int p_index)
{
  if(p_index >= 0 && p_index < m_attributes.GetSize())
  {
    return m_attributes.GetEntry(p_index);
  }
  return nullptr;
}

bool
Object::SetAttribute(int p_index,MemObject* p_attrib)
{
  if(p_index >= 0 && p_index < m_attributes.GetSize())
  {
    m_attributes.SetEntry(p_index,p_attrib);
    return true;
  }
  return false;
}

void
Object::Mark(QLvm* p_vm)
{
  m_attributes.Mark(p_vm);
}

