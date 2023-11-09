//////////////////////////////////////////////////////////////////////////
//
// QL Language interpreter
// ir. W.E. Huisman (c) 2018
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "QL_Language.h"
#include "QL_MemObject.h"
#include "QL_Interpreter.h"
#include "QL_Debugger.h"
#include "QL_Objects.h"
#include "QL_vm.h"
#include "QL_Opcodes.h"
#include <memory.h>
#include <string.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define iszero(x)	((x)->m_type == DTYPE_INTEGER && (x)->m_value.v_integer == 0)
#define istrue(x)	((x)->m_type != DTYPE_NIL && !iszero(x))

QLInterpreter::QLInterpreter(QLVirtualMachine* p_vm,bool p_trace)
              :m_vm(p_vm)
              ,m_debugger(NULL)
              ,m_trace(0)
              ,m_code(NULL)
              ,m_pc(NULL)
              ,m_frame_pointer(nullptr)
              ,m_stack_pointer(nullptr)
              ,m_stack_base(nullptr)
              ,m_stack_top(nullptr)
              ,m_stacksize(0)
{
  SetTracing(p_trace);
  p_vm->SetInterpreter(this);
}

QLInterpreter::~QLInterpreter()
{
  SetTracing(0);
  DestroyStack();
}

// Mark all objects on the stack as preparatory action
// before we can call the gc() and cleanup
// In this way we preserve the stack
void
QLInterpreter::Mark()
{
  if(m_stack_base)
  {
    MemObject** stack = m_stack_top;
    do 
    {
      if(*stack)
      {
        if((*stack)->IsMarked() == false)
        {
          m_vm->MarkObject(*stack);
        }
      }
    } 
    while (m_stack_pointer != stack--);
  }
}

// Set tracing status and prepare a debugger
void
QLInterpreter::SetTracing(bool p_trace)
{
  m_trace = p_trace;
  if(m_debugger)
  {
    delete m_debugger;
    m_debugger = nullptr;
  }
  if(m_trace)
  {
    m_debugger = new QLDebugger(m_vm);
  }
}

// Allocate the stack
void
QLInterpreter::AllocateStack()
{
  // Already allocated
  if(m_stack_base)
  {
    return;
  }

  // We need at minimum a default stack size
  if(m_stacksize == 0)
  {
    m_stacksize = STACK_DEFAULT;
  }

  // Create the stack
  m_stack_base    = (MemObject**) calloc(m_stacksize,sizeof(MemObject*));
  m_stack_top     = m_stack_base + m_stacksize - 1;
  m_stack_pointer = m_stack_top;
}

// Remove the stack again
void
QLInterpreter::DestroyStack()
{
  if(m_stack_base)
  {
    free(m_stack_base);
    m_stack_base    = nullptr;
    m_stack_top     = nullptr;
    m_stack_pointer = nullptr;
    m_frame_pointer = nullptr;
  }
}

// execute from an entry point function 
int 
QLInterpreter::Execute(CString p_name)
{
  // Check initialization of the VM
  m_vm->CheckInit();
  // Check the allocation of the stack
  AllocateStack();

  // EXECUTE the global init code, if any
  if(m_vm->HasInitCode())
  {
    Interpret(nullptr,nullptr);
  }

  // Various parameters
  shortint   type   = 0;
  MemObject* symbol = nullptr;
  Function*  func   = nullptr;
  
  // Find the main entry point
  NameMap& symbols = m_vm->GetSymbols();
  NameMap::iterator it = symbols.find(p_name);
  if(it == symbols.end())
  {
    func = m_vm->FindScript(p_name);
    if(func == nullptr)
    {
      m_vm->Error("Cannot find the entry point: %s\n",p_name);
      // Symbol not found
      return -1;
    }
    type = DTYPE_SCRIPT;
  }
  else
  {
    MemObject* symbol = it->second;
    type = symbol->m_type;
  }

  // EXECUTE the main entry point
  switch(type)
  {
    case DTYPE_INTERNAL:  if(symbol && symbol->m_value.v_internal)
                          {
                            (*symbol->m_value.v_internal)(this,0);
                          }
                          return -1;
    case DTYPE_EXTERNAL:  // CallExternalFunction()
                          return -1;
    case DTYPE_SCRIPT:    return Interpret(nullptr,func);
  }
  return -1;
}

// interpret - interpret bytecode instructions
int
QLInterpreter::Interpret(Object* p_object,Function* p_function)
{
  register int  pcoff,n;
  Object*       runObject   = p_object;
  Object*       calObject   = nullptr;
  Function*     runFunction = p_function;
  Function*     calFunction = nullptr;
  MemObject**   topframe;
  MemObject*    val;
  int           number  = 0;
  int           pop     = 0;
  bool          newline = true;
  bcd           floating;
  CString       selector;
  Class*        vClass;

  // initialize
  m_code = m_pc = runFunction ? runFunction->GetBytecode() : m_vm->GetBytecode();

  /* make a dummy call frame */
  CheckStack(STACKFRAME_SIZE);
  PushInteger(0);
  PushInteger(0);
  PushInteger(0);
  PushInteger(0);
  m_stack_pointer = PushInteger(0);
  m_frame_pointer = topframe = m_stack_pointer;

  // execute each instruction
  while(true)
  {
    if(m_trace) 
    {
      // Decode exactly one bytecode instruction
      m_debugger->DecodeInstruction(runFunction,m_code,(int) (m_pc - m_code));
      newline = true;
    }

    // Execute program counter and increment it in the same action
    switch(*m_pc++) 
    {
      case OP_CALL:		  // CALL A FUNCTION (SCRIPT, INTERNAL, EXTERNAL)
                        n = *m_pc++;  // Where we find our callee
                        if(m_trace)
                        {
                          // Finish tracing if active. Show what we will be calling
                          m_debugger->PrintObject(m_stack_pointer[n]);
                          newline = false;
                        }
                        switch (m_stack_pointer[n]->m_type) 
                        {
                          case DTYPE_INTERNAL:(*m_stack_pointer[n]->m_value.v_internal)(this,n);
                                              pop = n; // Number of arguments to pop again
                                              break;
                          case DTYPE_STRING:  calFunction = m_vm->FindScript(*m_stack_pointer[n]->m_value.v_string);
                                              if(calFunction == nullptr)
                                              {
                                                m_vm->Error("Call to non-existing function: %s",*m_stack_pointer[n]->m_value.v_string);
                                              }
                                              break;
                          case DTYPE_SCRIPT:  calFunction = m_stack_pointer[n]->m_value.v_script;
                                              break;
                          default:            m_vm->Error("Call to non-procedure, Type %s",GetTypename(m_stack_pointer[n]->m_type));
                                              return -1;
                        }
                        if(calFunction)
                        {
                           // Test number of arguments and data types
                           TestFunctionArguments(calFunction,n);

                           CheckStack(STACKFRAME_SIZE);
                           PushInteger(0);                                     // No object
                           PushFunction(runFunction);                          // Running function
                           PushInteger(n);                                     // NUMBER OF ARGUMENTS
                           PushInteger((int)(m_stack_top - m_frame_pointer));  // OFFSET SP FROM TOP (BEGINNING)
                           PushInteger((int)(m_pc - m_code));                  // OFFSET IN BYTECODE
                           m_code = m_pc   = calFunction->GetBytecode();       // New bytecode program counter
                           runFunction     = calFunction;                      // Now running this function
                           m_frame_pointer = m_stack_pointer;
                           runObject       = nullptr;
                           calFunction     = nullptr;
                        }
                        break;
      case OP_RETURN:		// RETURN FROM A SCRIPT FUNCTION
                        if (m_frame_pointer == topframe) 
                        {
                          if(m_trace)
                          {
                            osputs_stderr("\n");
                          }
                          // Last return on top level. Stops the script interpreting
                          // See if we did a integer return value
                          if(m_stack_pointer[0]->m_type == DTYPE_INTEGER)
                          {
                            return m_stack_pointer[0]->m_value.v_integer;
                          }
                          return 0;
                        }
                        val             = m_stack_pointer[0];
                        runObject       = nullptr;
                        m_stack_pointer = m_frame_pointer;
                        pcoff           = m_frame_pointer[SF_OFF_PRGCOUNTER]->m_value.v_integer;
                        n               = m_frame_pointer[SF_OFF_ARGUMENTS] ->m_value.v_integer;
                        runFunction     = m_frame_pointer[SF_OFF_FUNCTION]  ->m_value.v_script;
                        if(m_frame_pointer[SF_OFF_OBJECT]->m_type == DTYPE_OBJECT)
                        {
                          runObject = m_frame_pointer[SF_OFF_OBJECT]->m_value.v_object;
                        }
                        m_frame_pointer = m_stack_top - m_frame_pointer[SF_OFF_FRAMEPNTR]->m_value.v_integer;
                        m_code = runFunction->GetBytecode();
                        m_pc   = m_code + pcoff;
                        m_stack_pointer += STACKFRAME_SIZE; 
                        // Restore return value from function
                        m_stack_pointer[0] = val;
                        if(m_trace)
                        {
                          m_debugger->PrintReturn(runFunction);
                        }
                        // Pop this amount of variables from the stack
                        pop = n;
                        break;
      case OP_LOAD:  		// REFERENCE (LOAD A VARIABLE VALUE)
                        m_stack_pointer[0] = m_vm->GetGlobal(*m_pc++);
                        break;
      case OP_STORE:		// STORE a variable
                        m_vm->SetGlobal(*m_pc++,m_stack_pointer[0]);
                        break;
      case OP_VLOAD: 		// LOAD A VECTOR/ARRAY ELEMENT
                        CheckType(0,DTYPE_INTEGER);
                        switch (m_stack_pointer[1]->m_type) 
                        {
                          case DTYPE_ARRAY:  VectorRef(); break;
                          case DTYPE_STRING: StringRef(); break;
                          default:	         BadType(1,DTYPE_ARRAY); break;
                        }
                        // Implicit "POP 1"
                        pop = 1;
                        break;
      case OP_VSTORE:		// STORE AN ELEMENT BACK IN A VECTOR/ARRAY
                        CheckType(1,DTYPE_INTEGER);
                        switch (m_stack_pointer[2]->m_type) 
                        {
                          case DTYPE_ARRAY:  VectorSet(); break;
                          case DTYPE_STRING: StringSet(); break;
                          default:	         BadType(1,DTYPE_ARRAY); break;
                        }
                        pop = 2;
                        break;
      case OP_MLOAD:		// LOAD AN OBJECT MEMBER ON THE STACK
                        m_stack_pointer[0] = runObject->GetAttribute(*m_pc++);
                        break;
      case OP_MSTORE: 	// STORE TOS IN AN OBJECT MEMBER
                        n = *m_pc++;
                        if(runObject->SetAttribute(n,m_vm->AllocMemObject(m_stack_pointer[0])) == false)
                        {
                          BadMemberArgument(runObject,n);
                        }
                        break;
      case OP_ALOAD:		// LOAD AN ARGUMENT ON TOS
                        number = ArgumentReference(*m_pc++);
                        m_stack_pointer[0] = m_frame_pointer[number];
                        break;
      case OP_ASTORE: 	// STORE TOS IN AN ARGUMENT
                        number = ArgumentReference(*m_pc++);
                        m_frame_pointer[number] = m_stack_pointer[0];
                        break;
      case OP_TLOAD: 		// REFERENCE A LOCAL VARIABLE
                        n = *m_pc++;
                        m_stack_pointer[0] = m_frame_pointer[-n-1];
                        break;
      case OP_TSTORE: 	// SET a value in a Temporary (local variable)
                        n = *m_pc++;
                        m_frame_pointer[-n-1] = m_stack_pointer[0];
                        break;
      case OP_TSPACE:		// Create space on the stack for local variables (temporaries)
                        ReserveSpace(*m_pc++);
                        break;
      case OP_BRT:  		// BRANCH if TRUE
                        m_pc = (istrue(m_stack_pointer[0])) ? m_pc = m_code + GetWordOperand() : m_pc + 2;
                        break;
      case OP_BRF:  		// BRANCH IF FALSE
                        m_pc = (istrue(m_stack_pointer[0])) ? m_pc + 2 : m_pc = m_code + GetWordOperand();
                        break;
      case OP_BR:   		// UNCONDITIONAL BRANCH
                        m_pc = m_code + GetWordOperand();
                        break;
      case OP_NIL:  		// SET TOS TO NIL (ZERO)
                        SetNil(0);
                        break;
      case OP_PUSH: 		// PUSH INTEGER TO TOS
                        CheckStack(1);
                        PushInteger(0);
                        break;
      case OP_NOT:		  // NOT OPERATOR ON TOS
                        SetInteger(istrue(m_stack_pointer[0]) ? FALSE : TRUE);
                        break;
      case OP_NEG:		  // NEGATE TOS
                        CheckType(0,DTYPE_INTEGER);
                        m_stack_pointer[0]->m_value.v_integer = -(m_stack_pointer[0]->m_value.v_integer);
                        break;
      case OP_ADD:		  // PERFORM OPERATOR ADD on INTEGER, STRING, BCD or VARIANT
                        inter_operator(OP_ADD);
                        pop = 1;
                        break;
      case OP_SUB:		  // PERFORM OPERATOR SUBTRACT on INTEGER, STRING, BCD or VARIANT
                        inter_operator(OP_SUB);
                        pop = 1;
                        break;
      case OP_MUL:		  // PERFORM OPERATOR MULTIPLY on INTEGER, STRING, BCD or VARIANT
                        inter_operator(OP_MUL);
                        pop = 1;
                        break;
      case OP_DIV:  		// PERFORM OPERATOR DIVIDE on INTEGER, STRING, BCD or VARIANT
                        inter_operator(OP_DIV);
                        pop = 1;
                        break;
      case OP_REM: 		  // PERFORM OPERATOR REMAINDER on INTEGER, STRING, BCD or VARIANT
                        inter_operator(OP_REM);
                        pop = 1;
                        break;
      case OP_INC:  	  // INCREMENT TOS (INTEGER, BCD or VARIANT)
                        Inter_increment();
                        break;
      case OP_DEC: 		  // DECREMENT TOS (INTEGER, BCD or VARIANT)
                        Inter_decrement();
                        break;
      case OP_BAND: 		// OPERATOR BINARY-AND on an INTEGER
                        Inter_binary(OP_BAND);
                        pop = 1;
                        break;
      case OP_BOR:  	  // OPERATOR BINARY-OR on an INTEGER	
                        Inter_binary(OP_BOR);
                        pop = 1;
                        break;
      case OP_XOR: 		  // OPERATOR BINARY-EXCLUSIVE OR on an INTEGER
                        Inter_binary(OP_XOR);
                        pop = 1;
                        break;
      case OP_BNOT: 		// OPERATOR BINARY-NOT on an INTEGER
                        Inter_binary(OP_BNOT);
                        break;
      case OP_SHL:  		// OPERATOR BINARY SHIFTLEFT or PRINT TO STREAM
                        switch(m_stack_pointer[1]->m_type) 
                        {
                          case DTYPE_INTEGER: CheckType(0,DTYPE_INTEGER);
                                              m_stack_pointer[1]->m_value.v_integer <<= m_stack_pointer[0]->m_value.v_integer;
                                              break;
                          case DTYPE_FILE:    m_vm->Print(m_stack_pointer[1]->m_value.v_file,false,m_stack_pointer[0]);
                                              break;
                          default:            break;
                        }
                        pop = 1;
                        break;
      case OP_SHR:  		// OPERATOR BINARY SHIFT RIGHT
                        CheckType(0,DTYPE_INTEGER);
                        CheckType(1,DTYPE_INTEGER);
                        m_stack_pointer[1]->m_value.v_integer >>= m_stack_pointer[0]->m_value.v_integer;
                        pop = 1;
                        break;
      case OP_LT:       // OPERATOR LESS THAN (<)
                        inter_operator(OP_LT);
                        pop = 1;
                        break;
      case OP_LE:       // OPERATOR LESS THAN OR EQUAL TO (<=)
                        inter_operator(OP_LE);
                        pop = 1;
                        break;
      case OP_EQ: 		  // OPERATOR EQUAL (==)
                        inter_operator(OP_EQ);
                        pop = 1;
                        break;
      case OP_NE:       // OPERATOR NOT EQUAL (!=)
                        inter_operator(OP_NE);
                        pop = 1;
                        break;
      case OP_GE:       // OPERATOR GREATER THAN OR EQUAL TO (>=)
                        inter_operator(OP_GE);
                        pop = 1;
                        break;
      case OP_GT:       // OPERATOR GREATER THAN (<)
                        inter_operator(OP_GT);
                        pop = 1;
                        break;
      case OP_LIT:      // LOAD A LITERAL FOR THE CURRENT FUNCTION
                        val = runFunction ? runFunction->GetLiteral(*m_pc++) 
                                          : m_vm->GetLiteral(*m_pc++);
                        if(val->m_type == DTYPE_INTEGER ||
                           val->m_type == DTYPE_STRING)
                        {
                          // Get a duplicate (by-value) from a literal
                          // See the QL_Compiler::add_literal function!
                          m_stack_pointer[0] = m_vm->AllocMemObject(val);
                        }
                        else
                        {
                          // INTERNAL, ARRAY Etc are by-reference
                          m_stack_pointer[0] = val;
                        }
                        break;
      case OP_SEND:     // SEND REQUEST -> CALL A MEMBER OF AN OBJECT, INTERNAL METHOD
                        n = *m_pc++; // Get the stack offset

                        // Check that the member selector is a string!
                        CheckType(n - 1, DTYPE_STRING);

                        if(m_trace)
                        {
                          // Finish tracing if active. Show what we will be calling
                          m_debugger->PrintObject(m_stack_pointer[n]);
                          newline = false;
                        }

                        // See if it is an immediate scripted object
                        if(m_stack_pointer[n]->m_type == DTYPE_OBJECT)
                        {
                          // SEND REQUEST TO AN OBJECT
                          calObject = m_stack_pointer[n]->m_value.v_object;
                          vClass    = calObject->GetClass();
                          selector  = *m_stack_pointer[n-1]->m_value.v_string;
                          // Creating the "this" pointer on the stack on the place of the selector!
                          m_stack_pointer[n - 1] = m_stack_pointer[n];

                          if(selector.IsEmpty())
                          {
                            NoMethod(selector);
                            break;
                          }
                          val = vClass->RecursiveFindFuncMember(selector);

                          switch(val->m_type)
                          {
                            case DTYPE_INTERNAL:	(*val->m_value.v_internal)(this,n);
                                                  pop = n;
                                                  break;
                            case DTYPE_STRING:    val = calObject->GetClass()->FindFuncMember(selector);
                                                  if(val)
                                                  {
                                                    calFunction = val->m_value.v_script;
                                                  }
                                                  else
                                                  {
                                                    NoMethod(selector);
                                                  }
                                                  break;
                            case DTYPE_SCRIPT:    calFunction = val->m_value.v_script;
                                                  break;
                            default:		          m_vm->Error("Bad method, Selector '%s', Type %d",selector,val->m_type);
                                                  break;
                          }
                          if(calFunction)
                          {
                            // Test arguments. Allow for 'this' pointer as extra argument
                            TestFunctionArguments(calFunction,n - 1);

                            CheckStack(STACKFRAME_SIZE);
                            PushObject(runObject);
                            PushFunction(runFunction);
                            PushInteger(n);
                            PushInteger((int)(m_stack_top - m_frame_pointer));
                            PushInteger((int)(m_pc - m_code));
                            m_code = m_pc   = calFunction->GetBytecode();
                            runFunction     = calFunction;
                            runObject       = calObject;
                            m_frame_pointer = m_stack_pointer;
                            calFunction     = nullptr;
                            calObject       = nullptr;
                          }
                        }
                        else
                        {
                          // SEND REQUEST TO INTERNAL OBJECT
                          DoSendInternal(n);
                          // POP two of the stack
                          pop = n;
                        }
                        break;
      case OP_DUP2:     // Duplicate top two stack entries
                        CheckStack(2);
                        m_stack_pointer -= 2; // Grow the stack by 2
                        m_stack_pointer[0] = m_stack_pointer[2];
                        m_stack_pointer[1] = m_stack_pointer[3];
                        break;
      case OP_NEW:      // Create a new object from a class reference
                        if(m_stack_pointer[0]->m_type != DTYPE_CLASS)
                        {
                          BadType(0,DTYPE_CLASS);
                        }
                        m_stack_pointer[0] = m_vm->NewObject(m_stack_pointer[0]->m_value.v_class);
                        break;
      case OP_DESTROY:  // Delete the object on the top of the stack
                        if(m_stack_pointer[0]->m_type != DTYPE_OBJECT)
                        {
                          BadType(0,DTYPE_OBJECT);
                        }
                        calObject = m_stack_pointer[0]->m_value.v_object;

                        // Put object on the stack
                        PushInteger(0);
                        m_stack_pointer[0] = m_stack_pointer[1];

                        val = calObject->GetClass()->RecursiveFindFuncMember("destroy");
                        if(val && val->m_value.v_script)
                        {
                           // Use destroy function: no arguments allowed
                           calFunction = val->m_value.v_script;
                           TestFunctionArguments(calFunction,0);
                           // Same as a OP_SEND method
                           CheckStack(STACKFRAME_SIZE);
                           PushObject(runObject);
                           PushFunction(runFunction);
                           PushInteger(1); // No arguments
                           PushInteger((int)(m_stack_top - m_frame_pointer));
                           PushInteger((int)(m_pc - m_code));
                           m_code = m_pc   = calFunction->GetBytecode();
                           runFunction     = calFunction;
                           runObject       = calObject;
                           m_frame_pointer = m_stack_pointer;
                           calFunction     = nullptr;
                           calObject       = nullptr;
                        }
                        break;
      case OP_DELETE:   // At the end of the "Destroy" DTOR method
                        // The compiler emits an OP_DELETE after the OP_DESTROY
                        // so we always end up here after a "destroy <object>"
                        number = m_vm->DestroyObject(m_stack_pointer[0]);
                        SetInteger(number);
                        break;
      case OP_SWITCH:   // PERFORM A SWITCH STATEMENT
                        // Get number of cases
                        n   = GetWordOperand();
                        // Get value
                        val = m_stack_pointer[0];
                        // Walk the cases list
                        while (--n >= 0) 
                        {
                          pcoff = GetWordOperand();
                          if(Equal(val,runFunction->GetLiteral(pcoff)))
                          {
                            break;
                          }
                          m_pc += 2;
                        }
                        m_pc = m_code + GetWordOperand();
                        break;
      default:		      // UNKNOWN BYTECODE
                        m_vm->Error("INTERNAL Bad opcode: %02X",m_pc[-1]);
                        break;
    }
    if(m_trace)
    {
      // Complete the trace by printing the object (optionally)
      m_debugger->PrintObject(m_stack_pointer[0],newline);
    }
    // Popping the stack is not generated as bytecode
    // but is inserted by the interpreter as virtual code
    if(pop)
    {
      PopStack(pop);
      pop = 0;
    }
  }
  return 0;
}

// Test correct data types and number of arguments
void
QLInterpreter::TestFunctionArguments(Function* p_function,int p_num)
{
  // Check number of arguments
  if(p_function->GetNumberOfArguments() != p_num)
  {
    m_vm->Error("Wrong number of arguments [%d] in call to function: %s. Needed: %d\n"
               ,p_num
               ,p_function->GetFullName()
               ,p_function->GetNumberOfArguments());
  }

  // Check all arguments, in order of appearance
  for(int ind = p_num - 1;ind >= 0; --ind)
  {
    if(m_stack_pointer[ind]->m_type != p_function->GetArgument(ind))
    {
      m_vm->Error("Wrong datatype in parameter %d to function %s\n",p_num - ind,p_function->GetFullName());
    }
  }
}

// Unary operators
void
QLInterpreter::Inter_increment()
{
  int type = m_stack_pointer[0]->m_type;
  switch(type)
  {
    case DTYPE_INTEGER: ++(m_stack_pointer[0]->m_value.v_integer);
                        break;
    case DTYPE_BCD:     ++(*m_stack_pointer[0]->m_value.v_floating);
                        break;
    case DTYPE_VARIANT: { SQLVariant one((long)1);
                          *m_stack_pointer[0]->m_value.v_variant = 
                          *m_stack_pointer[0]->m_value.v_variant + one;
                        }
                        break;
    default:            BadType(0,DTYPE_INTEGER);
                        break;
  }
}

void
QLInterpreter::Inter_decrement()
{
  int type = m_stack_pointer[0]->m_type;
  switch(type)
  {
    case DTYPE_INTEGER: --(m_stack_pointer[0]->m_value.v_integer);
                        break;
    case DTYPE_BCD:     --(*m_stack_pointer[0]->m_value.v_floating);
                        break;
    case DTYPE_VARIANT: { SQLVariant one((long)1);
                          *m_stack_pointer[0]->m_value.v_variant = 
                          *m_stack_pointer[0]->m_value.v_variant - one;
                        }
                        break;
    default:            BadType(0,DTYPE_INTEGER);
                        break;
  }
}

// Do "operand operator operand"
void
QLInterpreter::inter_operator(BYTE p_operator)
{
  // Switch on the left hand side operand
  switch(m_stack_pointer[1]->m_type)
  {
    case DTYPE_INTEGER: switch(m_stack_pointer[0]->m_type)
                        {
                          case DTYPE_INTEGER:   return inter_intint_operator(p_operator);
                          case DTYPE_BCD:       return inter_intbcd_operator(p_operator);
                          case DTYPE_STRING:    return inter_intstr_operator(p_operator);
                          case DTYPE_VARIANT:   return inter_intvar_operator(p_operator);
                        }
                        break;
    case DTYPE_BCD:     switch(m_stack_pointer[0]->m_type)
                        {
                          case DTYPE_INTEGER:   return inter_bcdint_operator(p_operator);
                          case DTYPE_BCD:       return inter_bcdbcd_operator(p_operator);
                          case DTYPE_STRING:    return inter_bcdstr_operator(p_operator);
                          case DTYPE_VARIANT:   return inter_bcdvar_operator(p_operator);
                        }
                        break;
    case DTYPE_STRING:  switch(m_stack_pointer[0]->m_type)
                        {
                          case DTYPE_INTEGER:   return inter_strint_operator(p_operator);
                          case DTYPE_BCD:       return inter_strbcd_operator(p_operator);
                          case DTYPE_STRING:    return inter_strstr_operator(p_operator);
                          case DTYPE_VARIANT:   return inter_strvar_operator(p_operator);
                        }
                        break;
    case DTYPE_VARIANT: switch(m_stack_pointer[0]->m_type)
                        {
                          case DTYPE_INTEGER:   return inter_varint_operator(p_operator);
                          case DTYPE_BCD:       return inter_varbcd_operator(p_operator);
                          case DTYPE_STRING:    return inter_varstr_operator(p_operator);
                          case DTYPE_VARIANT:   return inter_varvar_operator(p_operator);
                        }
                        break;
    default:            BadOperator(p_operator);
                        break;
  }
}

// OPERATOR: INTEGER (result) = INTEGER oper INTEGER;
void
QLInterpreter::inter_intint_operator(BYTE p_operator)
{
  int number = 0;
  int left   = m_stack_pointer[1]->m_value.v_integer;
  int right  = m_stack_pointer[0]->m_value.v_integer;
  bool yesno;

  switch(p_operator)
  {
    case OP_ADD:  number = left + right;
                  SetInteger(number);
                  break;
    case OP_SUB:  number = left - right;
                  SetInteger(number);
                  break;
    case OP_MUL:  number = left * right;
                  SetInteger(number);
                  break;
    case OP_DIV:  if(right == 0)
                  {
                    m_vm->Info("Caught a 'division by zero'");
                  }
                  else
                  {
                    number = left / right;
                  }
                  SetInteger(number);
                  break;
    case OP_REM:  if(right == 0)
                  {
                    m_vm->Info("Caught a 'division by zero'");
                  }
                  else
                  {
                    number = left % right;
                  }
                  SetInteger( number);
                  break;
    case OP_LT:   yesno = left < right;
                  SetInteger(yesno);
                  break;
    case OP_GT:   yesno = left > right;
                  SetInteger( yesno);
                  break;
    case OP_LE:   yesno = left <= right;
                  SetInteger( yesno);
                  break;
    case OP_GE:   yesno = left >= right;
                  SetInteger( yesno);
                  break;
    case OP_EQ:   yesno = left == right;
                  SetInteger( yesno);
                  break;
    case OP_NE:   yesno = left != right;
                  SetInteger( yesno);
                  break;
  }
}

// OPERATOR: RESULT (BCD) = BCD oper BCD;
void
QLInterpreter::inter_bcdbcd_operator(BYTE p_operator)
{
  bcd  fl;
  bool yesno = false;
  bcd* left  = m_stack_pointer[1]->m_value.v_floating;
  bcd* right = m_stack_pointer[0]->m_value.v_floating;

  switch(p_operator)
  {
    case OP_ADD:  fl = *left + *right;
                  SetBcd(fl);
                  break;
    case OP_SUB:  fl = *left - *right;
                  SetBcd(fl);
                  break;
    case OP_MUL:  fl = *left * *right;
                  SetBcd( fl);
                  break;
    case OP_DIV:  if(right->IsNULL())
                  {
                    m_vm->Info("Caught a 'division by zero'");
                    fl.Zero();
                  }
                  else
                  {
                    fl = *left / *right;
                  }
                  SetBcd(fl);
                  break;
    case OP_REM:  if(right->IsNULL())
                  {
                    m_vm->Info("Caught a 'division by zero'");
                    fl.Zero();
                  }
                  else
                  {
                    fl = *left % *right;
                  }
                  SetBcd(fl);
                  break;
    case OP_LT:   yesno = *left < *right;
                  SetInteger(yesno);
                  break;
    case OP_GT:   yesno = *left > *right;
                  SetInteger(yesno);
                  break;
    case OP_LE:   yesno = *left <= *right;
                  SetInteger(yesno);
                  break;
    case OP_GE:   yesno = *left >= *right;
                  SetInteger(yesno);
                  break;
    case OP_EQ:   yesno = *left == *right;
                  SetInteger(yesno);
                  break;
    case OP_NE:   yesno = *left != *right;
                  SetInteger(yesno);
                  break;
  }
}

// OPERATOR: RESULT (bcd) = INTEGER oper BCD;
void
QLInterpreter::inter_intbcd_operator(BYTE p_operator)
{
  bcd  fl;
  bool yesno = false;
  bcd* left = m_stack_pointer[1]->m_value.v_floating;
  bcd  right( m_stack_pointer[0]->m_value.v_integer);

  switch(p_operator)
  {
    case OP_ADD:  fl = *left + right;
                  SetBcd(fl);
                  break;
    case OP_SUB:  fl = *left - right;
                  SetBcd(fl);
                  break;
    case OP_MUL:  fl = *left * right;
                  SetBcd( fl);
                  break;
    case OP_DIV:  if(m_stack_pointer[0]->m_value.v_integer == 0)
                  {
                    m_vm->Info("Caught a 'division by zero'");
                    fl.Zero();
                  }
                  else
                  {
                    fl = *left / right;
                  }
                  SetBcd( fl);
                  break;
    case OP_REM:  if(m_stack_pointer[0]->m_value.v_integer == 0)
                  {
                    m_vm->Info("Caught a 'division by zero'");
                    fl.Zero();
                  }
                  else
                  {
                    fl = *left % right;
                  }
                  SetBcd( fl);
                  break;
    case OP_LT:   yesno = *left < right;
                  SetInteger(yesno);
                  break;
    case OP_GT:   yesno = *left > right;
                  SetInteger(yesno);
                  break;
    case OP_LE:   yesno = *left <= right;
                  SetInteger(yesno);
                  break;
    case OP_GE:   yesno = *left >= right;
                  SetInteger(yesno);
                  break;
    case OP_EQ:   yesno = *left == right;
                  SetInteger(yesno);
                  break;
    case OP_NE:   yesno = *left != right;
                  SetInteger(yesno);
                  break;
  }
}

// OPERATOR: RESULT (integer) = INTEGER oper VARIANT;
void
QLInterpreter::inter_intvar_operator(BYTE p_operator)
{
  bool yesno = false;
  int result = 0;
  int left   = m_stack_pointer[1]->m_value.v_integer;
  int right  = m_stack_pointer[0]->m_value.v_variant->GetAsSLong();

  switch(p_operator)
  {
    case OP_ADD:  result = left + right;
                  SetInteger(result);
                  break;
    case OP_SUB:  result = left - right;
                  SetInteger(result);
                  break;
    case OP_MUL:  result = left * right;
                  SetInteger(result);
                  break;
    case OP_DIV:  if(right == 0)
                  {
                    m_vm->Info("Caught a 'division by zero'");
                  }
                  else
                  {
                    result = left / right;
                  }
                  SetInteger(result);
                  break;
    case OP_REM:  if(right == 0)
                  {
                    m_vm->Info("Caught a 'division by zero'");
                  }
                  else
                  {
                    result = left % right;
                  }
                  SetInteger(result);
                  break;
    case OP_LT:   yesno = left < right;
                  SetInteger(yesno);
                  break;
    case OP_GT:   yesno = left > right;
                  SetInteger(yesno);
                  break;
    case OP_LE:   yesno = left <= right;
                  SetInteger(yesno);
                  break;
    case OP_GE:   yesno = left >= right;
                  SetInteger(yesno);
                  break;
    case OP_EQ:   yesno = left == right;
                  SetInteger(yesno);
                  break;
    case OP_NE:   yesno = left != right;
                  SetInteger(yesno);
                  break;
  }
}

// OPERATOR: RESULT(bcd) = BCD oper INTEGER;
void
QLInterpreter::inter_bcdint_operator(BYTE p_operator)
{
  bcd  result;
  bool yesno = false;
  bcd* left = m_stack_pointer[1]->m_value.v_floating;
  bcd  right( m_stack_pointer[0]->m_value.v_integer);

  switch(p_operator)
  {
    case OP_ADD:  result = *left + right;
                  SetBcd(result);
                  break;
    case OP_SUB:  result = *left - right;
                  SetBcd(result);
                  break;
    case OP_MUL:  result = *left * right;
                  SetBcd(result);
                  break;
    case OP_DIV:  if(m_stack_pointer[0]->m_value.v_integer == 0)
                  {
                    m_vm->Info("Caught a 'division by zero'");
                  }
                  else
                  {
                    result = *left / right;
                  }
                  SetBcd(result);
                  break;
    case OP_REM:  if(m_stack_pointer[0]->m_value.v_integer == 0)
                  {
                    m_vm->Info("Caught a 'division by zero'");
                  }
                  else
                  {
                    result = *left % right;
                  }
                  SetBcd(result);
                  break;
    case OP_LT:   yesno = *left < right;
                  SetInteger(yesno);
                  break;
    case OP_GT:   yesno = *left > right;
                  SetInteger(yesno);
                  break;
    case OP_LE:   yesno = *left <= right;
                  SetInteger(yesno);
                  break;
    case OP_GE:   yesno = *left >= right;
                  SetInteger(yesno);
                  break;
    case OP_EQ:   yesno = *left == right;
                  SetInteger(yesno);
                  break;
    case OP_NE:   yesno = *left != right;
                  SetInteger(yesno);
                  break;
  }
}

// OPERATOR : RESULT (STRING) = STRING oper STRING;
// Subset of operators for string operator string
// SUB / MUL / DIV / REM cannot be implemented
void
QLInterpreter::inter_strstr_operator(BYTE p_operator)
{
  bool yesno;

  switch(p_operator)
  {
    case OP_ADD: {  CString left  = *m_stack_pointer[1]->m_value.v_string;
                    CString right = *m_stack_pointer[0]->m_value.v_string;
                    SetString(left + right);
                 }
                 break;
    case OP_LT:  yesno = *m_stack_pointer[1]->m_value.v_string < *m_stack_pointer[0]->m_value.v_string;
                 SetInteger(yesno);
                 break;
    case OP_GT:  yesno = *m_stack_pointer[1]->m_value.v_string > *m_stack_pointer[0]->m_value.v_string;
                 SetInteger( yesno);
                 break;
    case OP_LE:  yesno = *m_stack_pointer[1]->m_value.v_string <= *m_stack_pointer[0]->m_value.v_string;
                 SetInteger( yesno);
                 break;
    case OP_GE:  yesno = *m_stack_pointer[1]->m_value.v_string >= *m_stack_pointer[0]->m_value.v_string;
                 SetInteger( yesno);
                 break;
    case OP_EQ:  yesno = m_stack_pointer[1]->m_value.v_string->Compare(*m_stack_pointer[0]->m_value.v_string) == 0;
                 SetInteger( yesno);
                 break;
    case OP_NE:  yesno = m_stack_pointer[1]->m_value.v_string->Compare(*m_stack_pointer[0]->m_value.v_string) != 0;
                 SetInteger( yesno);
                 break;
    default:     BadOperator(p_operator);
                 break;
  }
}

// OPERATOR: RESULT(string) = INTEGER oper STRING
// ADD / DIV / REM / SUB cannot be implemented
void
QLInterpreter::inter_intstr_operator(BYTE p_operator)
{
  int   number = m_stack_pointer[1]->m_value.v_integer;
  CString* str = m_stack_pointer[0]->m_value.v_string;
  CString result;

  switch(p_operator)
  {
    case OP_MUL:  for(int ind = 0;ind < number; ++ind)
                  {
                    result += *str;
                  }
                  SetString(result);
                  break;
    case OP_LT:   number = atoi(*str) < number;
                  SetInteger(number);
                  break;
    case OP_GT:   number = atoi(*str) > number;
                  SetInteger( number);
                  break;
    case OP_LE:   number = atoi(*str) <= number;
                  SetInteger( number);
                  break;
    case OP_GE:   number = atoi(*str) >= number;
                  SetInteger( number);
                  break;
    case OP_EQ:   number = atoi(*str) == number;
                  SetInteger( number);
                  break;
    case OP_NE:   number = atoi(*str) != number;
                  SetInteger( number);
                  break;
    default:      BadOperator(p_operator);
                  break;
  }
}

// OPERATOR: RESULT (bcd) = BCD oper VARIANT
void
QLInterpreter::inter_bcdvar_operator(BYTE p_operator)
{
  bool yesno = false;
  bcd*  left = m_stack_pointer[1]->m_value.v_floating;
  bcd  right = m_stack_pointer[0]->m_value.v_variant->GetAsBCD();
  bcd result;

  switch(p_operator)
  {
    case OP_ADD:  result = *left + right;
                  SetBcd(result);
                  break;
    case OP_SUB:  result = *left - right;
                  SetBcd(result);
                  break;
    case OP_MUL:  result = *left * right;
                  SetBcd(result);
                  break;
    case OP_DIV:  if(right.IsNULL())
                  {
                    m_vm->Info("Caught a 'division by zero'");
                  }
                  else
                  {
                    result = *left / right;
                  }
                  SetBcd(result);
                  break;
    case OP_REM:  if(right.IsNULL() == 0)
                  {
                    m_vm->Info("Caught a 'division by zero'");
                  }
                  else
                  {
                    result = *left % right;
                  }
                  SetBcd(result);
                  break;
    case OP_LT:   yesno = *left < right;
                  SetInteger(yesno);
                  break;
    case OP_GT:   yesno = *left > right;
                  SetInteger(yesno);
                  break;
    case OP_LE:   yesno = *left <= right;
                  SetInteger(yesno);
                  break;
    case OP_GE:   yesno = *left >= right;
                  SetInteger(yesno);
                  break;
    case OP_EQ:   yesno = *left == right;
                  SetInteger(yesno);
                  break;
    case OP_NE:   yesno = *left != right;
                  SetInteger(yesno);
                  break;
  }
}

// OPERATOR: RESULT (string) = STRING oper INTEGER;
void
QLInterpreter::inter_strint_operator(BYTE p_operator)
{
  bool yesno = false;
  int left   = atoi(*m_stack_pointer[1]->m_value.v_string);
  int right  =       m_stack_pointer[0]->m_value.v_integer;
  CString str;

  switch(p_operator)
  {
    case OP_ADD:  str = *m_stack_pointer[1]->m_value.v_string;
                  str.AppendFormat("%d",right);
                  SetString(str);
                  break;
    case OP_MUL:  for(int ind = 0;ind < right; ++ind)
                  {
                    str += *m_stack_pointer[1]->m_value.v_string;
                  }
                  SetString(str);
                  break;
    case OP_LT:   yesno = left < right;
                  SetInteger(yesno);
                  break;
    case OP_GT:   yesno = left > right;
                  SetInteger(yesno);
                  break;
    case OP_LE:   yesno = left <= right;
                  SetInteger(yesno);
                  break;
    case OP_GE:   yesno = left >= right;
                  SetInteger(yesno);
                  break;
    case OP_EQ:   yesno = left == right;
                  SetInteger(yesno);
                  break;
    case OP_NE:   yesno = left != right;
                  SetInteger(yesno);
                  break;
    default:      BadOperator(p_operator);
  }
};

// OPERATOR: RESULT(bcd) = BCD oper STRING;
void
QLInterpreter::inter_bcdstr_operator(BYTE p_operator)
{
  CString str;
  int  number;
  bool  yesno;
  bcd* left = m_stack_pointer[1]->m_value.v_floating;
  bcd  right(*m_stack_pointer[0]->m_value.v_string);

  switch(p_operator)
  {
    case OP_MUL:  number = left->AsLong();
                  for (int ind = 0; ind < number; ++ind)
                  {
                    str += *m_stack_pointer[0]->m_value.v_string;
                  }
                  SetString(str);
                  break;
    case OP_LT:   yesno = *left < right;
                  SetInteger(yesno);
                  break;
    case OP_GT:   yesno = *left > right;
                  SetInteger( yesno);
                  break;
    case OP_LE:   yesno = *left <= right;
                  SetInteger( yesno);
                  break;
    case OP_GE:   yesno = *left >= right;
                  SetInteger( yesno);
                  break;
    case OP_EQ:   yesno = *left == right;
                  SetInteger( yesno);
                  break;
    case OP_NE:   yesno = *left != right;
                  SetInteger( yesno);
                  break;
    default:      BadOperator(p_operator);
  }
}

// Operator: STRING oper BCD
void
QLInterpreter::inter_strbcd_operator(BYTE p_operator)
{
  bool yesno;
  bcd  left(*  m_stack_pointer[1]->m_value.v_string);
  bcd* right = m_stack_pointer[0]->m_value.v_floating;

  switch(p_operator)
  {
    case OP_LT: yesno = left < *right;
                SetInteger(yesno);
                break;
    case OP_GT: yesno = left > *right;
                SetInteger(yesno);
                break;
    case OP_LE: yesno = left <= *right;
                SetInteger(yesno);
                break;
    case OP_GE: yesno = left >= *right;
                SetInteger(yesno);
                break;
    case OP_EQ: yesno = left == *right;
                SetInteger(yesno);
                break;
    case OP_NE: yesno = left != *right;
                SetInteger(yesno);
                break;
    default:    BadOperator(p_operator);
  }
}

// OPERATOR: RESULT (string) = STRING oper VARIANT;
void
QLInterpreter::inter_strvar_operator(BYTE p_operator)
{
  bool yesno = false;
  CString left = *m_stack_pointer[1]->m_value.v_string;
  CString  right( m_stack_pointer[0]->m_value.v_variant->GetAsChar());

  switch(p_operator)
  {
    case OP_ADD:  SetString(left + right);
                  break;
    case OP_LT:   yesno = left < right;
                  SetInteger(yesno);
                  break;
    case OP_GT:   yesno = left > right;
                  SetInteger( yesno);
                  break;
    case OP_LE:   yesno = left <= right;
                  SetInteger( yesno);
                  break;
    case OP_GE:   yesno = left >= right;
                  SetInteger( yesno);
                  break;
    case OP_EQ:   yesno = left == right;
                  SetInteger( yesno);
                  break;
    case OP_NE:   yesno = left != right;
                  SetInteger( yesno);
                  break;
    default:      BadOperator(p_operator);
                  break;
  }
}

// Operator: VARIANT oper INTEGER
void
QLInterpreter::inter_varint_operator(BYTE p_operator)
{
  int  number = 0;
  int  left   = m_stack_pointer[1]->m_value.v_variant->GetAsSLong();
  int  right  = m_stack_pointer[0]->m_value.v_integer;
  bool yesno  = false;

  switch(p_operator)
  {
    case OP_ADD:  number = left + right;
                  SetInteger(number);
                  break;
    case OP_SUB:  number = left - right;
                  SetInteger(number);
                  break;
    case OP_MUL:  number = left * right;
                  SetInteger(number);
                  break;
    case OP_DIV:  if(right == 0)
                  {
                    m_vm->Info("Caught a 'division by zero'");
                  }
                  else
                  {
                    number = left / right;
                  }
                  SetInteger(number);
                  break;
    case OP_REM:  if(right == 0)
                  {
                    m_vm->Info("Caught a 'division by zero'");
                  }
                  else
                  {
                    number = left % right;
                  }
                  SetInteger( number);
                  break;
    case OP_LT:   yesno = left < right;
                  SetInteger(yesno);
                  break;
    case OP_GT:   yesno = left > right;
                  SetInteger( yesno);
                  break;
    case OP_LE:   yesno = left <= right;
                  SetInteger( yesno);
                  break;
    case OP_GE:   yesno = left >= right;
                  SetInteger( yesno);
                  break;
    case OP_EQ:   yesno = left == right;
                  SetInteger( yesno);
                  break;
    case OP_NE:   yesno = left != right;
                  SetInteger( yesno);
                  break;
  }
}

// Operator: RESULT (bcd) = VARIANT oper BCD;
void
QLInterpreter::inter_varbcd_operator(BYTE p_operator)
{
  bcd  number;
  bcd  left  = m_stack_pointer[1]->m_value.v_variant->GetAsBCD();
  bcd* right = m_stack_pointer[0]->m_value.v_floating;
  bool yesno;

  switch(p_operator)
  {
    case OP_ADD:  number = left + *right;
                  SetBcd(number);
                  break;
    case OP_SUB:  number = left - *right;
                  SetBcd(number);
                  break;
    case OP_MUL:  number = left * *right;
                  SetBcd(number);
                  break;
    case OP_DIV:  if(right->IsNULL())
                  {
                    m_vm->Info("Caught a 'division by zero'");
                  }
                  else
                  {
                    number = left / *right;
                  }
                  SetBcd(number);
                  break;
    case OP_REM:  if(right->IsNULL())
                  {
                    m_vm->Info("Caught a 'division by zero'");
                  }
                  else
                  {
                    number = left % *right;
                  }
                  SetBcd(number);
                  break;
    case OP_LT:   yesno = left < *right;
                  SetInteger(yesno);
                  break;
    case OP_GT:   yesno = left > *right;
                  SetInteger( yesno);
                  break;
    case OP_LE:   yesno = left <= *right;
                  SetInteger( yesno);
                  break;
    case OP_GE:   yesno = left >= *right;
                  SetInteger( yesno);
                  break;
    case OP_EQ:   yesno = left == *right;
                  SetInteger( yesno);
                  break;
    case OP_NE:   yesno = left != *right;
                  SetInteger( yesno);
                  break;
  }
}

// OPERATOR: RESULT (variant) = VARIANT oper STRING;
void
QLInterpreter::inter_varstr_operator(BYTE p_operator)
{
  SQLVariant  result;
  SQLVariant* left = m_stack_pointer[1]->m_value.v_variant;
  SQLVariant  right( m_stack_pointer[0]->m_value.v_string->GetString());
  bool yesno;

  switch(p_operator)
  {
    case OP_ADD:  result = *left + right;
                  SetVariant(result);
                  break;
    case OP_SUB:  result = *left - right;
                  SetVariant(result);
                  break;
    case OP_MUL:  result = *left * right;
                  SetVariant(result);
                  break;
    case OP_DIV:  if(right.IsNULL() || right.IsEmpty())
                  {
                    m_vm->Info("Caught a 'division by zero'");
                  }
                  else
                  {
                    result = *left / right;
                  }
                  SetVariant(result);
                  break;
    case OP_REM:  if(right.IsNULL() || right.IsEmpty())
                  {
                    m_vm->Info("Caught a 'division by zero'");
                  }
                  else
                  {
                    result = *left % right;
                  }
                  SetVariant(result);
                  break;
    case OP_LT:   yesno = *left < right;
                  SetInteger(yesno);
                  break;
    case OP_GT:   yesno = *left > right;
                  SetInteger( yesno);
                  break;
    case OP_LE:   yesno = *left <= right;
                  SetInteger( yesno);
                  break;
    case OP_GE:   yesno = *left >= right;
                  SetInteger( yesno);
                  break;
    case OP_EQ:   yesno = *left == right;
                  SetInteger( yesno);
                  break;
    case OP_NE:   yesno = *left != right;
                  SetInteger( yesno);
                  break;
  }
}

// Operator: RESULT (variant) = VARIANT oper VARIANT;
void
QLInterpreter::inter_varvar_operator(BYTE p_operator)
{
  SQLVariant  number;
  SQLVariant* left  = m_stack_pointer[1]->m_value.v_variant;
  SQLVariant* right = m_stack_pointer[0]->m_value.v_variant;
  bool yesno;

  switch(p_operator)
  {
    case OP_ADD:  number = *left + *right;
                  SetVariant(number);
                  break;
    case OP_SUB:  number = *left - *right;
                  SetVariant(number);
                  break;
    case OP_MUL:  number = *left * *right;
                  SetVariant(number);
                  break;
    case OP_DIV:  if(right->IsNULL() || right->IsEmpty())
                  {
                    m_vm->Info("Caught a 'division by zero'");
                  }
                  else
                  {
                    number = *left / *right;
                  }
                  SetVariant(number);
                  break;
    case OP_REM:  if(right->IsNULL() || right->IsEmpty())
                  {
                    m_vm->Info("Caught a 'division by zero'");
                  }
                  else
                  {
                    number = *left % *right;
                  }
                  SetVariant(number);
                  break;
    case OP_LT:   yesno = *left < *right;
                  SetInteger(yesno);
                  break;
    case OP_GT:   yesno = *left > *right;
                  SetInteger(yesno);
                  break;
    case OP_LE:   yesno = *left <= *right;
                  SetInteger(yesno);
                  break;
    case OP_GE:   yesno = *left >= *right;
                  SetInteger(yesno);
                  break;
    case OP_EQ:   yesno = *left == *right;
                  SetInteger(yesno);
                  break;
    case OP_NE:   yesno = *left != *right;
                  SetInteger(yesno);
                  break;
  }
}

// Equal comparison for the SWITCH statement
// Objects do NOT come from the stack, but are referenced
// and can be literals, arrays or anything else
bool
QLInterpreter::Equal(MemObject* p_left,MemObject* p_right)
{
  switch(p_left->m_type)
  {
    case DTYPE_INTEGER: switch(p_right->m_type)
                        {
                          case DTYPE_INTEGER: return p_left->m_value.v_integer == p_right->m_value.v_integer;
                          case DTYPE_STRING:  return p_left->m_value.v_integer == atoi(*p_right->m_value.v_string);
                          case DTYPE_BCD:     return p_left->m_value.v_integer == p_right->m_value.v_floating->AsLong();
                          case DTYPE_VARIANT: return p_left->m_value.v_integer == p_right->m_value.v_variant->GetAsSLong();
                        }
                        break;
    case DTYPE_STRING:  switch(p_right->m_type)
                        {
                          case DTYPE_INTEGER: return atoi(*p_left->m_value.v_string) ==  p_right->m_value.v_integer;
                          case DTYPE_STRING:  return *p_left->m_value.v_string       == *p_right->m_value.v_string;
                          case DTYPE_BCD:     return bcd(*p_left->m_value.v_string)  == *p_right->m_value.v_floating;
                          case DTYPE_VARIANT: return *p_left->m_value.v_string       == CString(p_right->m_value.v_variant->GetAsChar());
                        }
                        break;
    case DTYPE_BCD:     switch(p_right->m_type)
                        {
                          case DTYPE_INTEGER: return p_left->m_value.v_floating->AsLong() == p_right->m_value.v_integer;
                          case DTYPE_STRING:  return *p_left->m_value.v_floating          == bcd(*p_right->m_value.v_string);
                          case DTYPE_BCD:     return *p_left->m_value.v_floating          == *p_right->m_value.v_floating;
                          case DTYPE_VARIANT: return *p_left->m_value.v_floating          ==  p_right->m_value.v_variant->GetAsBCD();
                        }
                        break;
    case DTYPE_VARIANT: switch(p_right->m_type)
                        {
                          case DTYPE_INTEGER: return         p_left->m_value.v_variant->GetAsSLong() ==  p_right->m_value.v_integer;
                          case DTYPE_STRING:  return CString(p_left->m_value.v_variant->GetAsChar()) == *p_right->m_value.v_string;
                          case DTYPE_BCD:     return p_left->m_value.v_variant->GetAsBCD()           == *p_right->m_value.v_floating;
                          case DTYPE_VARIANT: return *p_left->m_value.v_variant                      == *p_right->m_value.v_variant;
                        }
                        break;
    default:            m_vm->Error("Cannot be used as selector for a switch statement. Type: %d\n",p_left->m_type);
                        return false;
  }
  m_vm->Error("Cannot be used as selector for a case statement in a switch. Type: %d\n",p_right->m_type);
  return false;
}

// Binary operators '|' '&' and '~'
void
QLInterpreter::Inter_binary(BYTE p_operator)
{
  // TOS must be integer
  CheckType(0,DTYPE_INTEGER);

  switch(p_operator)
  {
    case OP_BAND: // OPERATOR BINARY-AND on an INTEGER
                  CheckType(1,DTYPE_INTEGER);
                  m_stack_pointer[0]->m_value.v_integer =
                  m_stack_pointer[1]->m_value.v_integer & m_stack_pointer[0]->m_value.v_integer;
                  break;
    case OP_BOR:  // OPERATOR BINARY-OR on an INTEGER	
                  CheckType(1,DTYPE_INTEGER);
                  m_stack_pointer[0]->m_value.v_integer =
                  m_stack_pointer[1]->m_value.v_integer | m_stack_pointer[0]->m_value.v_integer;
                  break;
    case OP_XOR: 	// OPERATOR BINARY-EXCLUSIVE OR on an INTEGER
                  CheckType(1,DTYPE_INTEGER);
                  m_stack_pointer[0]->m_value.v_integer = 
                  m_stack_pointer[1]->m_value.v_integer ^ m_stack_pointer[0]->m_value.v_integer;
                  break;
    case OP_BNOT: // OPERATOR BINARY-NOT on an INTEGER
                  m_stack_pointer[0]->m_value.v_integer = m_stack_pointer[0]->m_value.v_integer > 0 ? 0 : 1;
                  break;
  }
}

// Stack offset of an argument reference (this-pointer, member arguments)
int         
QLInterpreter::ArgumentReference(int n)
{
  int number = 0;
  if(n >= 0)
  {
    // Getting the number of arguments when calling this function/member
    number = m_frame_pointer[SF_OFF_ARGUMENTS]->m_value.v_integer;
    if(n && n >= number)
    {
      m_vm->Error("Too few arguments in calling function/member");
    }
    // Calculate stack offset for argument
    number = STACKFRAME_SIZE + number - n - 1;
  }
  else
  {
    m_vm->Error("Illegal argument reference: %d",n);
  }
  return number;
}

// Reserve stack space for local variables
void
QLInterpreter::ReserveSpace(int p_arguments)
{
  if(p_arguments > 0)
  {
    // member/function using local variables
    CheckStack(p_arguments);
    while(--p_arguments >= 0)
    {
      --m_stack_pointer;
      SetNil(0);
    }
  }
}

// Load an array element as in rvalue: "array[index]"
// TOS    = index
// TOS[1] = array
// After this function a "POP 1" must be done
void
QLInterpreter::VectorRef()
{
  Array* array = m_stack_pointer[1]->m_value.v_array;
  int    index = m_stack_pointer[0]->m_value.v_integer;

  if (index < 0 || index >= array->GetSize())
  {
    m_vm->Error("Array subscript out of bounds: %d",index);
  }
  m_stack_pointer[0] = array->GetEntry(index);
}

// Load a string element as in "string[index]"
// TOS    = index
// TOS[1] = string
// After this function a "POP 1" must be done
void
QLInterpreter::StringRef()
{
  CString* string = m_stack_pointer[1]->m_value.v_string;
  int      index  = m_stack_pointer[0]->m_value.v_integer;

  int cc = 0;
  if(index < 0 || index >= string->GetLength())
  {
    m_vm->Error("String subscript out of bounds: %d",index);
  }
  else
  {
    cc = string->GetAt(index);
  }
  SetInteger(cc);
}

// VectorSet as in "array[index] = value"
// TOS    = value
// TOS[1] = index
// TOS[2] = array
// After this function a "POP 2" must be done
void
QLInterpreter::VectorSet()
{
  if(m_trace && m_debugger)
  {
    m_debugger->PrintIndexedObject(m_stack_pointer[2],m_stack_pointer[1],m_stack_pointer[0]);
  }

  Array* array = m_stack_pointer[2]->m_value.v_array;
  int    index = m_stack_pointer[1]->m_value.v_integer;

  if (index < 0 || index >= array->GetSize())
  {
    m_vm->Error("Array subscript out of bounds: %d",index);
  }
  array->SetEntry(index,m_stack_pointer[0]);
}

// Set a string element as in "string[index] = value"
// TOS    = value
// TOS[1] = index
// TOS[2] = string
void
QLInterpreter::StringSet()
{
  CString* string = m_stack_pointer[2]->m_value.v_string;
  int      index  = m_stack_pointer[1]->m_value.v_integer;
  int      cc     = m_stack_pointer[0]->m_value.v_integer;

  CheckType(0,DTYPE_INTEGER);
  if (index < 0 || index >= string->GetLength())
  {
    m_vm->Error("String subscript out of bounds: %d",index);
  }
  string->SetAt(index,cc);
  SetInteger(cc);
}

// Get data word from program counter
int 
QLInterpreter::GetWordOperand()
{
  int b;
  b = *m_pc++;
  return ((*m_pc++ << 8) | b);
}

// Send request to a method of an internal data type
// e.g. DTYPE_DATABASE object gets an "Open" request
void
QLInterpreter::DoSendInternal(int p_offset)
{
  int     type =  m_stack_pointer[p_offset  ]->m_type;
  CString name = *m_stack_pointer[p_offset-1]->m_value.v_string;

  Method* method = m_vm->FindMethod(name,type);
  if(method)
  {
    // Call the internal method
    // First two arguments are not used by method itself
    // So the count should be biased for the real argument count
    (*method->m_internal)(this, p_offset - 1);
  }
  else
  {
    QLVirtualMachine::Error("Internal method [%s::%s] not found!",GetTypename(type),name);
  }
}

/* type names */
static char *tnames[] = 
{
   ""
  ,"ENDMARKER"
  ,"NIL"
  ,"INTEGER"
  ,"STRING"
  ,"BCD"
  ,"FILE"
  ,"SQLDATABASE"
  ,"SQLQUERY"
  ,"SQLVARIANT"
  ,"ARRAY"
  ,"OBJECT"
  ,"CLASS"
  ,"SCRIPT"
  ,"INTERNAL"
  ,"EXTERNAL"
};

// typename - get the name of a type 
CString
QLInterpreter::GetTypename(int type)
{
  CString buffer;
  if(type >= _DTMIN && type <= _DTMAX)
  {
    return (tnames[type]);
  }
  buffer.Format("TYPE (%d)",type);
  return buffer;
}

// Report a bad operand type 
int
QLInterpreter::BadType(int off,int expected_type)
{
  CString type1 = GetTypename(m_stack_pointer[off]->m_type);
  CString type2 = GetTypename(expected_type);
  m_vm->Info("PC: %04x, Offset %d, Type %s, Expected %s",(int)(m_pc - m_code),off,type1,type2);
  m_vm->Error("Bad argument type");
  return 0;
}

typedef struct _opnames 
{
  int   opcode;
  char* oper;
}
OpNames;

OpNames opnames[] =
{
  { OP_ADD,   "+ (add)"             }
 ,{ OP_SUB,   "- (subtract)"        }
 ,{ OP_MUL,   "* (multiply)"        }
 ,{ OP_DIV,   "/ (division)"        }
 ,{ OP_REM,   "% (remainder)"       }
 ,{ OP_EQ,    "== (equals)"         }
 ,{ OP_NE,    "!= (not equal)"      }
 ,{ OP_LT,    "< (less than)"       }
 ,{ OP_GT,    "> (greater than)"    }
 ,{ OP_LE,    "<= (lesser equal)"   }
 ,{ OP_GE,    ">= (greater equal)"  }
 ,{ 0,        NULL                  }
};

CString
QLInterpreter::OperatorName(int p_opcode)
{
  OpNames* oper = opnames;
  while(oper->opcode)
  {
    if(oper->opcode == p_opcode)
    {
      return CString(oper->oper);
    }
    ++oper;
  }
  return "";
}

void
QLInterpreter::BadOperator(int p_oper)
{
  CString operName  = OperatorName(p_oper);
  CString typeName1 = GetTypename(m_stack_pointer[1]->m_type);
  CString typeName2 = GetTypename(m_stack_pointer[0]->m_type);

  m_vm->Info("PC: %04x, Bad types/operator combination: %s %s %s",(int)(m_pc - m_code),typeName1,operName,typeName2);
  m_vm->Error("Bad operator type");
}

// Report a failure to find a method for a selector
void 
QLInterpreter::NoMethod(CString selector)
{
  m_vm->Error("No method for selector '%s'",selector);
}

// No such number of members on an object
void
QLInterpreter::BadMemberArgument(Object* p_object,int p_member)
{
  CString classname = p_object->GetClass()->GetName();
  m_vm->Error("Object of class [%s] does not have %d members",classname,p_member);
}

// Report a stack overflow error
void 
QLInterpreter::StackOverflow()
{
  m_vm->Error("Stack overflow");
}

//////////////////////////////////////////////////////////////////////////
//
// THE STACK
//
//////////////////////////////////////////////////////////////////////////

void
QLInterpreter::CheckStack(int p_size)
{
  if (m_stack_pointer - p_size < m_stack_base)
  {
    StackOverflow();
  }
}

MemObject**
QLInterpreter::PushInteger(int p_num)
{
  MemObject* object = m_vm->AllocMemObject(DTYPE_INTEGER);
  object->m_value.v_integer = p_num;
  *(--m_stack_pointer) = object;
  return m_stack_pointer;
}

MemObject**
QLInterpreter::PushFunction(Function* p_func)
{
  MemObject* object = m_vm->AllocMemObject(DTYPE_NIL);
  object->m_value.v_script = p_func;
  object->m_type   = DTYPE_SCRIPT;
  object->m_flags |= FLAG_REFERENCE;
  *(--m_stack_pointer) = object;
  return m_stack_pointer;
}

MemObject**
QLInterpreter::PushObject(Object* p_object)
{
  MemObject* object = m_vm->AllocMemObject(DTYPE_NIL);
  object->m_value.v_object = p_object;
  object->m_type = DTYPE_OBJECT;
  object->m_flags |= FLAG_REFERENCE;
  *(--m_stack_pointer) = object;
  return m_stack_pointer;
}

// Set stack[offset] to NIL
void
QLInterpreter::SetNil(int p_offset)
{
  m_stack_pointer[p_offset] = m_vm->AllocMemObject(DTYPE_NIL);
}

// Set TOS to an INTEGER
void
QLInterpreter::SetInteger(int p_value)
{
  MemObject* object = m_vm->AllocMemObject(DTYPE_INTEGER);
  object->m_value.v_integer = p_value;
  m_stack_pointer[0] = object;
}

// Set TOS to a STRING
void
QLInterpreter::SetString(CString p_string)
{
  MemObject* object = m_vm->AllocMemObject(DTYPE_STRING);
  *object->m_value.v_string = p_string;
  m_stack_pointer[0] = object;
}

// Set TOS to a FILE
void
QLInterpreter::SetFile(FILE* p_fp)
{
  MemObject* object = m_vm->AllocMemObject(DTYPE_FILE);
  object->m_value.v_file = p_fp;
  m_stack_pointer[0] = object;
}

// Set TOS to a BCD
void
QLInterpreter::SetBcd(bcd p_float)
{
  MemObject* object = m_vm->AllocMemObject(DTYPE_BCD);
  *object->m_value.v_floating = p_float;
  m_stack_pointer[0] = object;
}

// Set TOS to a VARIANT
void
QLInterpreter::SetVariant(SQLVariant p_variant)
{
  MemObject* object = m_vm->AllocMemObject(DTYPE_VARIANT);
  *object->m_value.v_variant = p_variant;
  m_stack_pointer[0] = object;
}

// Check that the TOS[offset] is of one of two types
void
QLInterpreter::CheckType(int p_offset,int p_type1,int p_type2 /*=0*/)
{
  if(m_stack_pointer[p_offset]->m_type != p_type1)
  {
    if(p_type2)
    {
      if(m_stack_pointer[p_offset]->m_type == p_type2)
      {
        return;
      }
    }
    BadType(p_offset,p_type1);
  }
}

// Pop from the stack will dragging the TOS after us
void
QLInterpreter::PopStack(int p_num)
{
  MemObject* val = m_stack_pointer[0];
  m_stack_pointer += p_num;
  m_stack_pointer[0] = val;

  if(m_trace && m_debugger)
  {
    m_debugger->PopStack(p_num);
  }
}

//////////////////////////////////////////////////////////////////////////
//
// Getting an argument for the QL_Functions module
//
//////////////////////////////////////////////////////////////////////////

// Getting an argument as an INTEGER
int
QLInterpreter::GetIntegerArgument(int p_num)
{
  int number = 0;

  switch(m_stack_pointer[p_num]->m_type)
  {
    case DTYPE_INTEGER:   number = m_stack_pointer[p_num]->m_value.v_integer;
                          break;
    case DTYPE_STRING:    number = atoi(*m_stack_pointer[p_num]->m_value.v_string);
                          break;
    case DTYPE_BCD:       number = m_stack_pointer[p_num]->m_value.v_floating->AsLong();
                          break;
    case DTYPE_VARIANT:   number = m_stack_pointer[p_num]->m_value.v_variant->GetAsSLong();
                          break;
    default:              BadType(p_num,DTYPE_INTEGER);
                          break;
  }
  return number;
}

// Getting an argument as a STRING
CString
QLInterpreter::GetStringArgument(int p_num)
{
  CString str;
  switch(m_stack_pointer[p_num]->m_type)
  {
    case DTYPE_STRING:  str = *m_stack_pointer[p_num]->m_value.v_string;
                        break;
    case DTYPE_INTEGER: str.Format("%d",m_stack_pointer[p_num]->m_value.v_integer);
                        break;
    case DTYPE_BCD:     str = m_stack_pointer[p_num]->m_value.v_floating->AsString();
                        break;
    case DTYPE_VARIANT: m_stack_pointer[p_num]->m_value.v_variant->GetAsString(str);
                        break;
    default:            BadType(p_num,DTYPE_STRING);
                        break;
  }
  return str;
}

// Getting an argument as a BCD
bcd
QLInterpreter::GetBcdArgument(int p_num)
{
  bcd number;
  switch(m_stack_pointer[p_num]->m_type)
  {
    case DTYPE_BCD:     number = *m_stack_pointer[p_num]->m_value.v_floating;
                        break;
    case DTYPE_INTEGER: number = bcd(m_stack_pointer[p_num]->m_value.v_integer);
                        break;
    case DTYPE_STRING:  number = bcd(m_stack_pointer[p_num]->m_value.v_string->GetString());
                        break;
    case DTYPE_VARIANT: number = m_stack_pointer[p_num]->m_value.v_variant->GetAsBCD();
                        break;
    default:            BadType(p_num,DTYPE_BCD);
                        break;
  }  
  return number;
}

SQLVariant*
QLInterpreter::GetSQLVariantArgument(int p_num)
{
  if(m_stack_pointer[p_num]->m_type == DTYPE_VARIANT)
  {
    return m_stack_pointer[p_num]->m_value.v_variant;
  }
  return nullptr;
}

//////////////////////////////////////////////////////////////////////////
//
//  External test system
//
//////////////////////////////////////////////////////////////////////////

int
QLInterpreter::GetTestInterations()
{
  return m_testIterations;
}

int
QLInterpreter::GetTestResult()
{
  return m_testResult;
}

int
QLInterpreter::GetTestRunning()
{
  return m_testRunning;
}

void
QLInterpreter::SetTestIterations(int p_iterations)
{
  m_testIterations = p_iterations;
}

void
QLInterpreter::SetTestResult(int p_result)
{
  m_testResult = p_result;
}

void
QLInterpreter::SetTestRunning(int p_running)
{
  m_testRunning = p_running;
}
