//////////////////////////////////////////////////////////////////////////
//
// QL Language compiler
// ir. W.E. Huisman (c) 2018
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "QL_Language.h"
#include "QL_Scanner.h"
#include "QL_vm.h"

/* variable access function codes */
#define LOAD	1
#define STORE	2
#define PUSH	3
#define DUP	  4

// Break/continue stack size
#define SSIZE	10

// RVALUE type for PVAL
#define PV_NOVALUE   0
#define PV_CODE      1
#define PV_ARGUMENT  2
#define PV_TEMPORARY 3
#define PV_LITERAL   4
#define PV_VARIABLE  5
#define PV_MEMBER    6
#define PV_GLOBAL    7

// Max amount of bytecode per function
#define CMAX     32000

// partial value structure for the compiler
typedef struct _pval
{
  // int (*fcn)(int,int);
  int m_pval_type;
  int m_value;
  // int m_datatype;
} 
PVAL;

/* case entry structure */
typedef struct centry CENTRY;
struct centry 
{
  int     value;
  int     label;
  CENTRY* next;
};

/* switch entry structure */
typedef struct swentry SWENTRY;
struct swentry 
{
  int       nCases;
  CENTRY*   cases;
  int       defaultLabel;
  int       label;
};

typedef std::vector<CString> ARGUMENT;

// forward declarations
class QLDebugger;

class QLCompiler
{
public:
  QLCompiler(QLVirtualMachine* p_vm);
 ~QLCompiler();

 bool  CompileDefinitions(int (*getcf)(void *),void *getcd);
 void  SetDebugger(QLDebugger* p_debugger,int p_decode);

private:
  // Language parsing methods
  void    do_global_declaration();
  void    do_function(CString p_name);
  void    do_regular_function(CString p_name);
  void    do_member_function(Class* p_class);
  void    do_code(Function* p_function);
  void    do_class();
  void    do_statement();
  void    do_if();
  void    do_while();
  void    do_dowhile();
  void    do_for();
  void    do_switch();
  void    do_case();
  void    do_default();
  void    do_break();
  void    do_continue();
  void    do_block();
  void    do_return();
  void    do_init_expr();
  void    do_expr();
  void    do_test();
  void    do_expr1(PVAL* pv);
  void    do_expr2(PVAL* pv);
  void    do_expr3(PVAL* pv);
  void    do_expr4(PVAL* pv);
  void    do_expr5(PVAL* pv);
  void    do_expr6(PVAL* pv);
  void    do_expr7(PVAL* pv);
  void    do_expr8(PVAL* pv);
  void    do_expr9(PVAL* pv);
  void    do_expr10(PVAL* pv);
  void    do_expr11(PVAL* pv);
  void    do_expr12(PVAL* pv);
  void    do_expr13(PVAL* pv);
  void    do_expr14(PVAL* pv);
  void    do_expr15(PVAL* pv);
  void    do_assignment(PVAL* pv,int op);
  void    do_preincrement(PVAL* pv,int op);
  void    do_postincrement(PVAL* pv,int op);
  void    do_new(PVAL* pv);
  void    do_delete(PVAL* pv);
  void    do_send(CString selector,PVAL* pv);
  void    do_primary(PVAL* pv);
  void    do_call(PVAL* pv);
  void    do_index(PVAL* pv);
  void    do_lit_integer(long n);
  void    do_lit_string(CString str);
  void    do_lit_float(bcd fl);

  int       FindDataType(CString p_name);
  void      FindVariable(CString p_name,PVAL* pv);
  int       FindClassVariable(Class* p_class,CString p_name,PVAL* pv);
  MemObject* FindDataMember(CString p_name);
  Class*    get_class(CString p_name);
  int*      addbreak(int lbl);
  int       rembreak(int* old,int lbl);
  int*      addcontinue(int lbl);
  void      remcontinue(int* old);
  void      rvalue(PVAL* pv);
  void      Check_LValue(PVAL* pv);
  int       GetArgumentList(Function* p_function);
  void      AddArgument (CString p_name);
  void      AddTemporary(CString p_name);
  void      freelist(ARGUMENT* p_list);
  int       FindArgument(CString p_name);
  int       FindTemporary(CString p_name);
  int       AddLiteral(int p_type,MemObject** p_result,CString p_name = "",int p_value = 0);
  void      FreeLiterals();
  void      FetchRequireToken(int rtkn);
  void      RequireToken(int tkn,int rtkn);
  int       make_lit_integer(long n);
  int       make_lit_string(CString p_string);
  int       make_lit_variable(MemObject* p_sym);
  SWENTRY*  AddSwitch();
  void      RemoveSwitch(SWENTRY *old);
  int       CountOfTemporaries();
  void      emit_code(int p_type,int p_code,int p_value);
  void      code_argument(int fcn,int n);
  void      code_temporary(int fcn,int n);
  void      code_member(int fcn,int n);
  void      code_variable(int fcn,int n);
  int       code_index(int fcn);
  void      code_literal(int n);
  int       putcbyte(int b);
  int       putcword(int w);
  void      Fixup(int chn,int val);
  void      fixup_ref(int chn,int val);
  char*     GetMemory(int size);

  // DATA
  QLVirtualMachine* m_vm;           // QL Virtual Machine
  QLScanner*        m_scanner;      // QL Scanner object
  QLDebugger*       m_debugger;     // QL Debugger object
  ARGUMENT          m_arguments;	  // argument list */
  ARGUMENT          m_temporaries;	// temporary variable list */
  Array*            m_literals;	    // literal list 
  Class*            m_methodclass;	// bob_class of the current method */
  BYTE*             cbuff;	        // code buffer
  int               cptr;		        // code pointer
  /* break/continue stacks */
  int               bstack[SSIZE];
  int*              bsp;
  int               cstack[SSIZE];
  int*              csp;
  SWENTRY           sstack[SSIZE],*ssp,*ssbase;   // switch stack
  int               m_decode; 		  // flag for decoding functions
};

inline void 
QLCompiler::SetDebugger(QLDebugger* p_debugger,int p_decode)
{
  m_debugger = p_debugger;
  m_decode   = p_decode;
}
