//////////////////////////////////////////////////////////////////////////
//
// QL Language interpreter
// ir. W.E. Huisman (c) 2018
//
//////////////////////////////////////////////////////////////////////////

#pragma once

// Foreward declaration
class QLVirtualMachine;
class QLDebugger;
class Function;

using SQLComponents::SQLVariant;

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
  int               Interpret(Object* p_object,Function* p_function);

  // REGISTER OPERATIONS FOR GC
  void              Mark();

  // Stack handling for QL_Functions
  MemObject**       GetStackPointer();
  QLVirtualMachine* GetVirtualMachine();
  int               GetIntegerArgument(int p_num);
  CString           GetStringArgument(int p_num);
  bcd               GetBcdArgument(int p_num);
  SQLVariant*       GetSQLVariantArgument(int p_num);

  // Setting TOS with a value/object
  void              SetNil    (int p_offset);
  void              SetInteger(int p_value);
  void              SetString (CString p_string);
  void              SetFile   (FILE* p_fp);
  void              SetBcd    (bcd p_float);
  void              SetVariant(SQLVariant p_variant);

  // External testing system
  int               GetTestInterations();
  int               GetTestResult();
  int               GetTestRunning();
  void              SetTestIterations(int p_iterations);
  void              SetTestResult(int p_result);
  void              SetTestRunning(int p_running);

  // Datatypes check and reporting
  void              CheckType (int p_offset,int p_type1,int p_type2 = 0);
  int               BadType   (int p_offset,int p_type);

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
  // Stack offset of an argument reference (this-pointer, member arguments)
  int         ArgumentReference(int n);
  // Reserve stack space for local variables
  void        ReserveSpace(int p_arguments);

  // Stack handling
  void        AllocateStack();
  void        DestroyStack();
  void        CheckStack(int p_size);
  void        PopStack(int p_num);
  void        StackOverflow();
  MemObject** PushInteger(int p_num);
  MemObject** PushFunction(Function* p_func);
  MemObject** PushObject (Object* p_object);

  // Unary operators
  void        Inter_increment();
  void        Inter_decrement();

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
  // Binary operators '|' '&' and '~'
  void        Inter_binary(BYTE p_operator);

  // typename - get the name of a type
  CString     GetTypename(int type);
  // Report a failure to find a method for a selector
  void        NoMethod(CString selector);
  // Get the name of an operator
  CString     OperatorName(int p_opcode);
  // Report bad operator for top 2 types
  void        BadOperator(int p_oper);
  // No such number of members on an object
  void        BadMemberArgument(Object* p_object,int p_member);
  // Test correct data types and number of arguments
  void        TestFunctionArguments(Function* p_function,int p_num);
  
  QLVirtualMachine* m_vm;             // Connected Virtual Machine
  QLDebugger*       m_debugger;       // Connected debugger
  bool              m_trace;          // variable to control tracing
  BYTE*             m_code;           // currently executing code vector
  BYTE*             m_pc;             // the program counter

  // The system stack
  unsigned int      m_stacksize;      // System wide stacksize
  MemObject**       m_stack_base;     // array of MemObject[_stacksize];
  MemObject**       m_stack_top;      // _stack_base + _stacksize * sizeof(MemObject)
  MemObject**       m_stack_pointer;  // current stack pointer
  MemObject**       m_frame_pointer;  // the frame pointer

  // External testing system
  int               m_testIterations { 0 };   // Number of iterations of latest test
  int               m_testResult     { 0 };   // Result of latest test (0 = OK, >0 = ERROR)
  int               m_testRunning    { 0 };   // Do one more iteration? (0 = NO, 1 = YES)
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
QLInterpreter::SetStacksize(int p_size)
{
  m_stacksize = p_size;
}