//////////////////////////////////////////////////////////////////////////
//
// QL Language debugger
// ir. W.E. Huisman (c) 2018
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "QL_Language.h"
#include "QL_vm.h"

// instruction output formats
#define FMT_NONE	0
#define FMT_BYTE	1
#define FMT_WORD	2
#define FMT_LIT		3
#define FMT_TABLE 4 // Switch table

// Opcode type definition table
typedef struct 
{ 
  int   ot_code; // OPCODE in internal numeric value 
  char* ot_name; // OPCODE logical name as a string
  int   ot_fmt;  // Type of formatting to be done
  int   ot_poff; // Print stack offset (-1 = do nothing)
} 
OTDEF;

class QLDebugger : public CObject
{
public:
  QLDebugger(QLVirtualMachine* p_vm);
 ~QLDebugger();

  // decode global parts outside functions/classes
  void   DecodeGlobals(BYTE* cbuff,int oldptr,int cptr);
  // decode the instructions in a code object 
  void   DecodeProcedure(Function* p_function);
  // decode a single bytecode instruction
  int    DecodeInstruction(Function* p_function,BYTE* code,int lc,bool p_generator = false);
  // Print return into function
  void   PrintReturn(Function* p_function);
  // Print an object as an explanation of the printed bytecode
  void   PrintObject(MemObject* p_object,bool p_newline = true);
  // Print an indexed object "array/string [index] " with option value
  void   PrintIndexedObject(MemObject* p_vector,MemObject* p_index,MemObject* p_value);

  // Print the popping of the stack
  void   PopStack(int p_num);

private:
  QLVirtualMachine* m_vm;
  CString           m_lastFunc;
  int               m_printObject;
};
