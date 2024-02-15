//////////////////////////////////////////////////////////////////////////
//
// QL Language debugger
// ir. W.E. Huisman (c) 2018
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "QL_Language.h"
#include "QL_MemObject.h"
#include "QL_Opcodes.h"
#include "QL_Debugger.h"
#include "QL_Objects.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// BEWARE: The order of this array follows the order of opcodes
//         as is defined in the 'QL_Opcodes.h' header file

OTDEF opcode_table[] = 
{
  { OP_BRT,     _T("BRT"),    FMT_WORD, -1 },  // Branch on true
  { OP_BRF,     _T("BRF"),    FMT_WORD, -1 },  // Branch on false
  { OP_BR,      _T("BR"),     FMT_WORD, -1 },  // Branch
  { OP_NIL,     _T("NIL"),    FMT_NONE, -1 },  // Load TOS with NILL
  { OP_PUSH,    _T("PUSH"),   FMT_NONE, -1 },  // Push NILL onto stack
  { OP_NOT,     _T("NOT"),    FMT_NONE,  0 },  // logical negate TOS
  { OP_NEG,     _T("NEG"),    FMT_NONE,  0 },  // Negate TOS
  { OP_ADD,     _T("ADD"),    FMT_NONE,  0 },  // Add top two stack entries
  { OP_SUB,     _T("SUB"),    FMT_NONE,  0 },  // Subtract    top two stack entries
  { OP_MUL,     _T("MUL"),    FMT_NONE,  0 },  // Multiply    top two stack entries
  { OP_DIV,     _T("DIV"),    FMT_NONE,  0 },  // Divide      top two stack entries
  { OP_REM,     _T("REM"),    FMT_NONE,  0 },  // Remainder   top two stack entries
  { OP_BAND,    _T("BAND"),   FMT_NONE,  0 },  // BITWISE AND top two stack entries
  { OP_BOR,     _T("BOR"),    FMT_NONE,  0 },  // BITWISE OR  top two stack entries
  { OP_XOR,     _T("XOR"),    FMT_NONE,  0 },  // BITWISE XOR top tow stack entries
  { OP_BNOT,    _T("BNOT"),   FMT_NONE,  0 },  // BITWISE NOT top two stack entries
  { OP_SHL,     _T("SHL"),    FMT_NONE, -1 },  // Shift left  top two stack entries
  { OP_SHR,     _T("SHR"),    FMT_NONE, -1 },  // shift right top two stack entries
  { OP_LT,      _T("LT"),     FMT_NONE, -1 },  // <           top two stack entries
  { OP_LE,      _T("LE"),     FMT_NONE, -1 },  // <=          top two stack entries
  { OP_EQ,      _T("EQ"),     FMT_NONE, -1 },  // ==          top two stack entries
  { OP_NE,      _T("NE"),     FMT_NONE, -1 },  // !=          top two stack entries
  { OP_GE,      _T("GE"),     FMT_NONE, -1 },  // >=          top two stack entries
  { OP_GT,      _T("GT"),     FMT_NONE, -1 },  // >           top two stack entries
  { OP_INC,     _T("INC"),    FMT_NONE,  0 },  // Increment TOS
  { OP_DEC,     _T("DEC"),    FMT_NONE,  0 },  // Decrement TOS
  { OP_LIT,     _T("LIT"),    FMT_LIT,   0 },  // Load literal value
  { OP_RETURN,  _T("RETURN"), FMT_NONE, -1 },  // Return 
  { OP_CALL,    _T("CALL"),   FMT_BYTE,  0 },  // Call a function
  { OP_LOAD,    _T("LOAD"),   FMT_LIT,   0 },  // Load a variable
  { OP_STORE,   _T("STORE"),  FMT_LIT,   0 },  // Set value of a variable
  { OP_VLOAD,   _T("VLOAD"),  FMT_NONE,  0 },  // Load a vector element
  { OP_VSTORE,  _T("VSTORE"), FMT_NONE, -1 },  // Set a vector element
  { OP_MLOAD,   _T("MLOAD"),  FMT_BYTE,  0 },  // Load a member variable
  { OP_MSTORE,  _T("MSTORE"), FMT_BYTE,  0 },  // Set a member variable
  { OP_ALOAD,   _T("ALOAD"),  FMT_BYTE,  0 },  // Load argument value
  { OP_ASTORE,  _T("ASTORE"), FMT_BYTE,  0 },  // Set an argument value
  { OP_TLOAD,   _T("TLOAD"),  FMT_BYTE,  0 },  // Load temporary value
  { OP_TSTORE,  _T("TSTORE"), FMT_BYTE,  0 },  // Set temporary value
  { OP_TSPACE,  _T("TSPACE"), FMT_BYTE, -1 },  // Allocate temp space
  { OP_SEND,    _T("SEND"),   FMT_BYTE,  0 },  // Send message to an object
  { OP_DUP2,    _T("DUP2"),   FMT_NONE, -1 },  // Duplicate top two stack entries
  { OP_NEW,     _T("NEW"),    FMT_NONE,  0 },  // Create a new class object
  { OP_DELETE,  _T("DELETE"), FMT_NONE,  0 },  // Delete an object variable by calling Destroy
  { OP_DESTROY, _T("DESTROY"),FMT_NONE,  0 },  // Really destroy the object
  { OP_SWITCH,  _T("SWITCH"), FMT_TABLE,-1 },  // Switch table entry
  { 0,          NULL,     0,        -1 }   // End of opcode table
};

QLDebugger::QLDebugger(QLVirtualMachine* p_vm)
           :m_vm(p_vm)
           ,m_printObject(false)
{
}

QLDebugger::~QLDebugger()
{
}

// decode_procedure - decode the instructions in a code object
void 
QLDebugger::DecodeProcedure(Function* p_function)
{
  int   done = 0;
  int   len  = p_function->GetBytecodeSize();
  BYTE* code = p_function->GetBytecode();

  CString funcName = p_function->GetName();
  if(funcName != m_lastFunc)
  {
    // Display what we are debugging
    CString name(_T("Function: "));
    if(p_function->GetClass())
    {
      CString className = p_function->GetClass()->GetName();
      name += className + _T("::");
    }
    name += funcName;
    name += _T("\n");
    osputs_stderr(name);

    // Remember last function
    m_lastFunc = funcName;
  }

  for(int ind = 0;ind < len; ind += done)
  {
    done = DecodeInstruction(p_function,code,ind,true);
    PrintObject(nullptr);
  }
}

void
QLDebugger::DecodeGlobals(BYTE* cbuff,int oldptr,int cptr)
{
  int done = 0;

  for(int ind = oldptr;ind < cptr; ind += done)
  {
    done = DecodeInstruction(nullptr,cbuff,ind,true);
  }
}

// decode_instruction - decode a single bytecode instruction
int 
QLDebugger::DecodeInstruction(Function* p_function,BYTE* code,int lc,bool p_generator /*=false*/)
{
  CString name = p_function ? p_function->GetFullName() : _T("GLOBALS");
  CString buffer;
  BYTE*   cp;
  OTDEF*  opcode;
  int n   = 1;
  int cnt = 0;
  int i   = 0;

  // Safest way to calculate the bytecode address
  cp = &code[lc];

  // Print the ORG of the function, at the first offset
  if(lc == 0)
  {
    buffer.Format(_T("              ORG    %s\n"),name.GetString());
    osputs_stderr(buffer);
  }

  // Print relative bytecode address
  buffer.Format(_T("%04X %02X "),lc,*cp);
  osputs_stderr(buffer);

  // display the operands


  if(*cp >= 1 && *cp <= OP_LAST)
  {
    // Getting the opcode from the opcode array
    opcode = &opcode_table[(*cp - 1)];

    switch (opcode->ot_fmt) 
    {
      case FMT_NONE:		buffer.Format(_T("      %-9s"),opcode->ot_name);
                        osputs_stderr(buffer);
                        break;
      case FMT_BYTE:	  buffer.Format(_T("%02X    %-6s %02X"),cp[1],opcode->ot_name,cp[1]);
                        osputs_stderr(buffer);
                        n += 1; // skip 1 extra byte
                        break;
      case FMT_WORD:		buffer.Format(_T("%02X %02X %-6s %02X%02X"),cp[1],cp[2],opcode->ot_name,cp[2],cp[1]);
                        osputs_stderr(buffer);
                        n += 2; // skip 1 extra word
                        break;
      case FMT_LIT:   	buffer.Format(_T("%02X    %-6s %02X"),cp[1],opcode->ot_name,cp[1]);
                        osputs_stderr(buffer);
                        if(p_generator)
                        {
                          osputs_stderr(_T("   ; "));
                          if(p_function)
                          {
                            m_vm->Print(stderr,TRUE,p_function->GetLiteral(cp[1]));
                          }
                          else
                          {
                            m_vm->Print(stderr,TRUE,m_vm->GetLiteral(cp[1]));
                            osputs_stderr(_T("\n"));
                          }
                        }
                        n += 1; // skip 1 byte / literal = byte size
                        break;
      case FMT_TABLE:   buffer.Format(_T("%02x %02x %s %02x%02x\n"),cp[1],cp[2],opcode->ot_name,cp[2],cp[1]);
                        osputs_stderr(buffer);
                        cnt = cp[2] << 8 | cp[1];
                        n  += 2 + cnt * 4 + 2;
                        i   = 3;

                        // Print the jump table of the switch statement  
                        while (--cnt >= 0) 
                        {
                          buffer.Format(_T("     %02X%02X  %02X%02X   ; "),cp[i+1],cp[i],cp[i+3],cp[i+2]);
                          osputs_stderr(buffer);
                          m_vm->Print(stderr,TRUE,p_function->GetLiteral((cp[i+1] << 8) | cp[i]));
                          osputs_stderr(_T("\n"));
                          i += 4;
                        }
                        buffer.Format(_T("     %02X%02X         ; DEFAULT"),cp[i+1],cp[i]);
                        osputs_stderr(buffer);
                        break;
    }
    // Recall the print offset
    m_printObject = opcode->ot_poff;
  }
  else
  {
    // unknown opcode
    osputs_stderr(_T("      <UNKNOWN>\n"));
    m_printObject = -1;
  }
  // Number of bytes scanned/skipped
  return n;
}

// Print return into function
void
QLDebugger::PrintReturn(Function* p_function)
{
  CString buffer;
  buffer.Format(_T("\n              RET    %s"),p_function->GetFullName().GetString());
  osputs_stderr(buffer);
}

// Print an object as an explanation of the printed bytecode
void
QLDebugger::PrintObject(MemObject* p_stack,bool p_newline /*=true*/)
{
  if(m_printObject >= 0 && p_stack)
  {
    osputs_stderr(_T("   ; "));
    m_vm->Print(stderr, TRUE, &p_stack[m_printObject]);
  }
  if(p_newline)
  {
    osputs_stderr(_T("\n"));
  }
  // Reset the print
  m_printObject = -1;
}

void
QLDebugger::PrintIndexedObject(MemObject* p_vector,MemObject* p_index,MemObject* p_value)
{
  osputs_stderr(_T("   ; "));
  m_vm->Print(stderr,TRUE,p_vector);
  if(p_index)
  {
    osputs_stderr(_T("["));
    m_vm->Print(stderr,TRUE,p_index);
    osputs_stderr(_T("]"));
  }
  if(p_value)
  {
    osputs_stderr(_T(" = "));
    m_vm->Print(stderr,TRUE,p_value);
  }
}

void
QLDebugger::PopStack(int p_num)
{
  CString buffer;
  buffer.Format(_T("              POP    %2.2X\n"),p_num);
  osputs_stderr(buffer);
}
