//////////////////////////////////////////////////////////////////////////
//
// QL Language interpreter
// ir. W.E. Huisman (c) 2017
//
//////////////////////////////////////////////////////////////////////////

#pragma once

// Foreward declaration
class QLVirtualMachine;
class QLDebugger;
class Function;

// EACH STACKFRAME CONTAINS
// 1) Script function to be run
// 2) Number of arguments on the stack
// 3) Stack Frame pointer (stack offset)
// 4) Program counter (bytecode offset)
#define STACKFRAME_SIZE 5

// STACKFRAME CONTENTS
#define SF_OFF_OBJECT     4
#define SF_OFF_FUNCTION   3
#define SF_OFF_ARGUMENTS  2
#define SF_OFF_FRAMEPNTR  1
#define SF_OFF_PRGCOUNTER 0

class QLInterpreter 
{
public:
  QLInterpreter(QLVirtualMachine* p_vm, bool p_trace);
 ~QLInterpreter();

  // Setting tracing off code execution on-off
  void              SetTracing(bool p_trace);
  void              SetStacksize(int p_size);

  // Execute a bytecode function
  int               Execute(CString p_name);
  // interpret - interpret bytecode instructions
  void              Interpret(Object* p_object,Function* p_function);

  // REGISTER OPERATIONS FOR GC
  void              Mark();

  // Stack handling for QL_Functions
  MemObject**       GetStackPointer();
  QLVirtualMachine* GetVirtualMachine();
  void              IncrementStackPointer();
  void              IncrementStackPointer(int p_len);
  MemObject**       PushInteger(int p_num);
  int               GetIntegerArgument(int p_num);
  CString           GetStringArgument(int p_num);
  bcd               GetBcdArgument(int p_num);
  void              SetNil    (int p_offset);
  void              SetInteger(int p_offset,int p_value);
  void              SetString (int p_offset,int p_len);
  void              SetFile   (int p_offset,FILE* p_fp);
  void              SetBcd    (int p_offset,bcd p_float);
  void              SetVariant(int p_offset,SQLVariant p_variant);
  void              CheckType (int p_offset,int p_type1,int p_type2 = 0);

private:
  // x = vector[a], getting x from element [a]
  void        VectorRef();
  // vector[a] = x, setting element [a] to x
  void        VectorSet();
  // Getting char x from "x = string[i]"
  void        StringRef();
  // Setting char x in string "string[i] = x"
  void        StringSet();
  // Get data word operand
  int         GetWordOperand();
  // Send request to internal object
  void        DoSendInternal(int p_offset);

  // Stack handling
  void        AllocateStack();
  void        DestroyStack();
  void        CheckStack(int p_size);
  void        StackOverflow();
  MemObject** PushClass(Class* p_class);
  MemObject** PushFunction(Function* p_func);
  MemObject** PushObject (Object* p_object);

  // Do "operand operator operand"
  void        inter_operator       (BYTE p_operator);
  void        inter_intint_operator(BYTE p_operator);
  void        inter_intbcd_operator(BYTE p_operator);
  void        inter_intstr_operator(BYTE p_operator);
  void        inter_intvar_operator(BYTE p_operator);
  void        inter_bcdint_operator(BYTE p_operator);
  void        inter_bcdbcd_operator(BYTE p_operator);
  void        inter_bcdstr_operator(BYTE p_operator);
  void        inter_bcdvar_operator(BYTE p_operator);
  void        inter_strint_operator(BYTE p_operator);
  void        inter_strbcd_operator(BYTE p_operator);
  void        inter_strstr_operator(BYTE p_operator);
  void        inter_strvar_operator(BYTE p_operator);
  void        inter_varint_operator(BYTE p_operator);
  void        inter_varbcd_operator(BYTE p_operator);
  void        inter_varstr_operator(BYTE p_operator);
  void        inter_varvar_operator(BYTE p_operator);
  // Equal comparison for the SWITCH statement
  bool        Equal(MemObject* p_left,MemObject* p_right);

  // typename - get the name of a type
  CString     GetTypename(int type);
  // Report a bad operand type
  int         BadType(int p_offset,int p_type);
  // Report a failure to find a method for a selector
  void        NoMethod(CString selector);
  // Get the name of an operator
  CString     OperatorName(int p_opcode);
  // Report bad operator for top 2 types
  void        BadOperator(int p_oper);
  
  QLVirtualMachine* m_vm;             // Connected Virtual Machine
  QLDebugger*       m_debugger;       // Connected debugger
  bool              m_trace;          // variable to control tracing
  BYTE*             m_code;		        // currently executing code vector
  BYTE*             m_pc;	            // the program counter

  // The system stack
  unsigned int      m_stacksize;      // System wide stacksize
  MemObject**       m_stack_base;     // array of MemObject[_stacksize];
  MemObject**       m_stack_top;      // _stack_base + _stacksize * sizeof(MemObject)
  MemObject**       m_stack_pointer;  // current stackpointer
  MemObject**       m_frame_pointer;  // the frame pointer
};

inline MemObject**
QLInterpreter::GetStackPointer()
{
  return m_stack_pointer;
}

inline QLVirtualMachine*
QLInterpreter::GetVirtualMachine()
{
  return m_vm;
}

inline void
QLInterpreter::IncrementStackPointer()
{
  ++m_stack_pointer;
}

inline void
QLInterpreter::IncrementStackPointer(int p_len)
{
  m_stack_pointer += p_len;
}

inline void
QLInterpreter::SetStacksize(int p_size)
{
  m_stacksize = p_size;
}