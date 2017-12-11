//////////////////////////////////////////////////////////////////////////
//
// QL Language Opcodes for bytecode interpreter
//
//////////////////////////////////////////////////////////////////////////

#pragma once

// BYTECODE OPCODES
// BEWARE: The order of opcodes is the same as the otab
//         array in the debugger. Do not omit values!!!
//
#define OP_BRT		 0x01	 // branch on true
#define OP_BRF		 0x02	 // branch on false
#define OP_BR		   0x03	 // branch unconditionally
#define OP_NIL		 0x04	 // load top of stack with nil
#define OP_PUSH		 0x05	 // push nil onto stack
#define OP_NOT		 0x06	 // logical negate top of stack
#define OP_NEG		 0x07	 // negate top of stack
#define OP_ADD		 0x08	 // add top two stack entries
#define OP_SUB		 0x09	 // subtract top two stack entries
#define OP_MUL		 0x0A	 // multiply top two stack entries
#define OP_DIV		 0x0B	 // divide top two stack entries
#define OP_REM		 0x0C	 // remainder of top two stack entries
#define OP_BAND		 0x0D	 // bitwise and of top two stack entries
#define OP_BOR		 0x0E	 // bitwise or of top two stack entries
#define OP_XOR		 0x0F	 // bitwise xor of top two stack entries
#define OP_BNOT		 0x10	 // bitwise not of top two stack entries
#define OP_SHL	 	 0x11	 // shift left top two stack entries
#define OP_SHR		 0x12	 // shift right top two stack entries
#define OP_LT		   0x13	 // less than
#define OP_LE		   0x14	 // less than or equal to
#define OP_EQ		   0x15	 // equal to
#define OP_NE		   0x16	 // not equal to
#define OP_GE		   0x17	 // greater than or equal to
#define OP_GT		   0x18	 // greater than
#define OP_INC		 0x19	 // increment
#define OP_DEC		 0x1A	 // decrement
#define OP_LIT		 0x1B	 // load literal
#define OP_RETURN	 0x1C	 // return from interpreter
#define OP_CALL		 0x1D	 // call a function
#define OP_LOAD		 0x1E	 // load a variable value
#define OP_STORE	 0x1F	 // set the value of a variable
#define OP_VLOAD	 0x20	 // load a vector element
#define OP_VSTORE	 0x21	 // set a vector element
#define OP_MLOAD	 0x22	 // load a member variable value
#define OP_MSTORE	 0x23	 // set a member variable
#define OP_ALOAD	 0x24	 // load an argument value
#define OP_ASTORE	 0x25	 // set an argument value
#define OP_TLOAD	 0x26	 // load a temporary variable value
#define OP_TSTORE	 0x27	 // set a temporary variable
#define OP_TSPACE	 0x28	 // allocate temporary variable space
#define OP_SEND		 0x29  // send a message to an object
#define OP_DUP2		 0x2A  // duplicate top two elements on the stack
#define OP_NEW		 0x2B  // create a new class object
#define OP_DELETE  0x2C  // Delete a class object
#define OP_DESTROY 0x2D  // Destroy deleted object
#define OP_SWITCH  0x2E  // Switch jump table
#define OP_LAST    0x2E  // LAST CODE IN ARRAY
