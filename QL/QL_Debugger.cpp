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

// Forward declarations
void osputs(const char* str);

// BEWARE: The order of this array follows the order of opcodes
//         as is defined in the 'QL_Opcodes.h' header file

OTDEF opcode_table[] = 
{
  {	OP_BRT,		  "BRT",		FMT_WORD, -1 },  // Branch on true
  {	OP_BRF,		  "BRF",		FMT_WORD, -1 },  // Branch on false
  {	OP_BR,		  "BR",		  FMT_WORD, -1 },  // Branch
  {	OP_NIL,		  "NIL",		FMT_NONE, -1 },  // Load TOS with nill
  {	OP_PUSH,	  "PUSH",		FMT_NONE, -1 },  // Push nill onto stack
  {	OP_NOT,		  "NOT",		FMT_NONE,  0 },  // logical negate TOS
  {	OP_NEG,		  "NEG",		FMT_NONE,  0 },  // Negate TOS
  {	OP_ADD,		  "ADD",		FMT_NONE,  0 },  // Add top two stack entries
  {	OP_SUB,		  "SUB",		FMT_NONE,  0 },  // Subtract    top two stack entries
  {	OP_MUL,		  "MUL",		FMT_NONE,  0 },  // Multiply    top two stack entries
  {	OP_DIV,		  "DIV",		FMT_NONE,  0 },  // Divide      top two stack entries
  {	OP_REM,		  "REM",		FMT_NONE,  0 },  // Remainder   top two stack entries
  {	OP_BAND,	  "BAND",		FMT_NONE,  0 },  // BITWISE AND top two stack entries
  {	OP_BOR,		  "BOR",		FMT_NONE,  0 },  // BITWISE OR  top two stack entries
  { OP_XOR,     "XOR",    FMT_NONE,  0 },  // BITWISE XOR top tow stack entries
  {	OP_BNOT,	  "BNOT",		FMT_NONE,  0 },  // BITWISE NOT top two stack entries
  {	OP_SHL,		  "SHL",		FMT_NONE, -1 },  // Shift left  top two stack entries
  {	OP_SHR,		  "SHR",		FMT_NONE, -1 },  // shift right top two stack entries
  {	OP_LT,		  "LT",		  FMT_NONE, -1 },  // <           top two stack entries
  {	OP_LE,		  "LE",		  FMT_NONE, -1 },  // <=          top two stack entries
  {	OP_EQ,		  "EQ",		  FMT_NONE, -1 },  // ==          top two stack entries
  {	OP_NE,		  "NE",		  FMT_NONE, -1 },  // !=          top two stack entries
  {	OP_GE,		  "GE",		  FMT_NONE, -1 },  // >=          top two stack entries
  {	OP_GT,		  "GT",		  FMT_NONE, -1 },  // >           top two stack entries
  {	OP_INC,		  "INC",		FMT_NONE,  0 },  // Increment TOS
  {	OP_DEC,		  "DEC",		FMT_NONE,  0 },  // Decrement TOS
  {	OP_LIT,		  "LIT",		FMT_LIT,   0 },  // Load literal value
  {	OP_RETURN,	"RETURN",	FMT_NONE, -1 },  // Return 
  {	OP_CALL,	  "CALL",		FMT_BYTE,  0 },  // Call a function
  {	OP_LOAD,	  "LOAD",		FMT_LIT,   0 },  // Load a variable
  {	OP_STORE,	  "STORE",	FMT_LIT,   0 },  // Set value of a variable
  {	OP_VLOAD,	  "VLOAD",  FMT_NONE,  0 },  // Load a vector element
  {	OP_VSTORE,	"VSTORE", FMT_NONE, -1 },  // Set a vector element
  {	OP_MLOAD,	  "MLOAD",	FMT_BYTE,  0 },  // Load a member variable
  {	OP_MSTORE,	"MSTORE", FMT_BYTE,  0 },  // Set a member variable
  {	OP_ALOAD,	  "ALOAD",	FMT_BYTE,  0 },  // Load argument value
  {	OP_ASTORE,	"ASTORE", FMT_BYTE,  0 },  // Set an argument value
  {	OP_TLOAD,   "TLOAD",	FMT_BYTE,  0 },  // Load temporary value
  {	OP_TSTORE,	"TSTORE", FMT_BYTE,  0 },  // Set temporary value
  {	OP_TSPACE,	"TSPACE",	FMT_BYTE, -1 },  // Allocate temp space
  {	OP_SEND,	  "SEND",		FMT_BYTE,  0 },  // Send message to an object
  {	OP_DUP2,	  "DUP2",		FMT_NONE, -1 },  // Duplicate top two stack entries
  {	OP_NEW,		  "NEW",		FMT_NONE,  0 },  // Create a new class object
  { OP_DELETE,  "DELETE", FMT_NONE,  0 },  // Delete an object variable by calling Destroy
  { OP_DESTROY, "DESTROY",FMT_NONE,  0 },  // Realy destroy the object
  { OP_SWITCH,  "SWITCH", FMT_TABLE,-1 },  // Switch table entry
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
    CString name("Function: ");
    if(p_function->GetClass())
    {
      CString className = p_function->GetClass()->GetName();
      name += className + "::";
    }
    name += funcName;
    name += "\n";
    osputs(name);

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
  CString name = p_function ? p_function->GetFullName() : "GLOBALS";
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
    buffer.Format("              ORG    %s\n",name.GetString());
    osputs(buffer);
  }

  // Print relative bytecode address
  buffer.Format("%04X %02X ",lc,*cp);
  osputs(buffer);

  // display the operands


  if(*cp >= 1 && *cp <= OP_LAST)
  {
    // Getting the opcode from the opcode array
    opcode = &opcode_table[(*cp - 1)];

    switch (opcode->ot_fmt) 
    {
      case FMT_NONE:		buffer.Format("      %-9s",opcode->ot_name);
                        osputs(buffer);
                        break;
      case FMT_BYTE:	  buffer.Format("%02X    %-6s %02X",cp[1],opcode->ot_name,cp[1]);
                        osputs(buffer);
                        n += 1; // skip 1 extra byte
                        break;
      case FMT_WORD:		buffer.Format("%02X %02X %-6s %02X%02X",cp[1],cp[2],opcode->ot_name,cp[2],cp[1]);
                        osputs(buffer);
                        n += 2; // skip 1 extra word
                        break;
      case FMT_LIT:   	buffer.Format("%02X    %-6s %02X",cp[1],opcode->ot_name,cp[1]);
                        osputs(buffer);
                        if(p_generator)
                        {
                          osputs("   ; ");
                          if(p_function)
                          {
                            m_vm->Print(stderr,TRUE,p_function->GetLiteral(cp[1]));
                          }
                          else
                          {
                            m_vm->Print(stderr,TRUE,m_vm->GetLiteral(cp[1]));
                            osputs("\n");
                          }
                        }
                        n += 1; // skip 1 byte / literal = byte size
                        break;
      case FMT_TABLE:   buffer.Format("%02x %02x %s %02x%02x\n",cp[1],cp[2],opcode->ot_name,cp[2],cp[1]);
                        osputs(buffer);
                        cnt = cp[2] << 8 | cp[1];
                        n  += 2 + cnt * 4 + 2;
                        i   = 3;

                        // Print the jump table of the switch statement  
                        while (--cnt >= 0) 
                        {
                          buffer.Format("     %02X%02X  %02X%02X   ; ",cp[i+1],cp[i],cp[i+3],cp[i+2]);
                          osputs(buffer);
                          m_vm->Print(stderr,TRUE,p_function->GetLiteral((cp[i+1] << 8) | cp[i]));
                          osputs("\n");
                          i += 4;
                        }
                        buffer.Format("     %02X%02X         ; DEFAULT",cp[i+1],cp[i]);
                        osputs(buffer);
                        break;
    }
    // Recall the print offset
    m_printObject = opcode->ot_poff;
  }
  else
  {
    // unknown opcode
    osputs("      <UNKNOWN>\n");
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
  buffer.Format("\n              RET    %s",p_function->GetFullName().GetString());
  osputs(buffer);
}

// Print an object as an explanation of the printed bytecode
void
QLDebugger::PrintObject(MemObject* p_stack,bool p_newline /*=true*/)
{
  if(m_printObject >= 0 && p_stack)
  {
    osputs("   ; ");
    m_vm->Print(stderr, TRUE, &p_stack[m_printObject]);
  }
  if(p_newline)
  {
    osputs("\n");
  }
  // Reset the print
  m_printObject = -1;
}

void
QLDebugger::PrintIndexedObject(MemObject* p_vector,MemObject* p_index,MemObject* p_value)
{
  osputs("   ; ");
  m_vm->Print(stderr,TRUE,p_vector);
  if(p_index)
  {
    osputs("[");
    m_vm->Print(stderr,TRUE,p_index);
    osputs("]");
  }
  if(p_value)
  {
    osputs(" = ");
    m_vm->Print(stderr,TRUE,p_value);
  }
}

void
QLDebugger::PopStack(int p_num)
{
  CString buffer;
  buffer.Format("              POP    %2.2X\n",p_num);
  osputs(buffer);
}
