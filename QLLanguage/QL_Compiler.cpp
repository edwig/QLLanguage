//////////////////////////////////////////////////////////////////////////
//
// QL Language compiler
// ir. W.E. Huisman (c) 2018
//
//////////////////////////////////////////////////////////////////////////

#include "Stdafx.h"
#include "QL_Language.h"
#include "QL_MemObject.h"
#include "QL_Compiler.h"
#include "QL_Debugger.h"
#include "QL_Objects.h"
#include "QL_Opcodes.h"
#include <stdio.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

QLCompiler::QLCompiler(QLVirtualMachine* p_vm)
           :m_vm(p_vm)
           ,m_scanner(nullptr)
           ,m_debugger(nullptr)
           ,m_literals(nullptr)
           ,m_methodclass(nullptr)
           ,cbuff(nullptr)
           ,cptr(0)
           ,m_decode(0)
{
  cbuff = (BYTE*) GetMemory(CMAX);

  // Init break/continue stacks
  bsp = &bstack[0];
  csp = &cstack[0];
  ssp = &sstack[0];
  ssbase = ssp;
}

QLCompiler::~QLCompiler()
{
  if(cbuff)
  {
    free(cbuff);
    cbuff = nullptr;
  }
  FreeLiterals();
}

// compile_definitions - compile class or function definitions
bool
QLCompiler::CompileDefinitions(int (*getcf)(void*),void *getcd)
{
  bool result = true;
  CString name;
  int tkn;

  // initialize the scanner
  m_scanner = new QLScanner(getcf,getcd);
  bsp       = &bstack[-1];
  csp       = &cstack[-1];
  ssbase    = &sstack[-1];
  ssp       = ssbase;

  try
  {
    // process statements until end of file
    while ((tkn = m_scanner->GetToken()) != T_EOF) 
    {
      switch (tkn) 
      {
        case T_GLOBAL:      do_global_declaration();
                            break;
        case T_IDENTIFIER:  name = m_scanner->GetTokenAsString();
                            do_function(name);
                            break;
        case T_CLASS:       do_class();
                            break;
        default:	          m_scanner->ParseError(_T("Expecting a declaration"));
                            break;
      }
    }
  }
  catch(int code)
  {
    _ftprintf(stderr,_T("ERROR: Compiler stopped with error: %d\n"),code);
    result = false;
  }
  // Done with the scanner
  delete m_scanner;
  m_scanner = nullptr;

  return result;
}

// do global declaration
void
QLCompiler::do_global_declaration()
{
  int tkn = 0;
  int global_count = 0;
  int oldptr = cptr;

  // Reset code pointer
  cptr = 0;

  // parse each global declaration
  do
  {
    // parse each variable and initializer
    do
    {
      // Fetch datatype
      FetchRequireToken(T_IDENTIFIER);
      CString datatype = m_scanner->GetTokenAsString();
      int type = FindDataType(datatype);
      if(type == DTYPE_NIL)
      {
        m_scanner->ParseError(_T("Unknown global data type: ") + datatype);
      }
      // Get local variable name
      FetchRequireToken(T_IDENTIFIER);
      CString variable = m_scanner->GetTokenAsString();
      int global = m_vm->AddGlobal(nullptr,variable);

      if((tkn = m_scanner->GetToken()) == '=')
      {
        do_init_expr();
        putcbyte(OP_STORE);
        putcbyte(global);
      }
      else
      {
        m_scanner->SaveToken(tkn);
      }
    } 
    while((tkn = m_scanner->GetToken()) == ',');
    RequireToken(tkn,';');
  } 
  while((tkn = m_scanner->GetToken()) == T_GLOBAL);
  m_scanner->SaveToken(tkn);

  // Copy the literals to the global literals
  if(m_literals)
  {
    for(int ind = 0;ind < m_literals->GetSize(); ++ind)
    {
      m_vm->AddLiteral(m_literals->GetEntry(ind));
    }
    FreeLiterals();
  }

  // Save the bytecode for the global init
  m_vm->AddBytecode(cbuff,cptr);

  // Show what we just did
  if(m_decode && m_debugger)
  {
    m_debugger->DecodeGlobals(cbuff,oldptr,cptr);
  }
}

// Handle class declarations 
void 
QLCompiler::do_class()
{
  CString     className;
  CString     member;
  int         type;
  int         tkn;
  Class*      theClass  = nullptr;
  Class*      baseClass = nullptr;

  // get the class name
  FetchRequireToken(T_IDENTIFIER);
  className = m_scanner->GetTokenAsString();
    
  // get the optional base class
  if ((tkn = m_scanner->GetToken()) == ':') 
  {
    FetchRequireToken(T_IDENTIFIER);
    CString baseClassName = m_scanner->GetTokenAsString();
    baseClass = get_class(baseClassName);
    if(m_decode)
    {
      m_vm->Info(_T("Class '%s', Base class '%s'"),className,baseClassName);
    }
  }
  else 
  {
    m_scanner->SaveToken(tkn);
    if(m_decode)
    {
      m_vm->Info(_T("Class '%s'"),className);
    }
  }
  FetchRequireToken('{');

  // create the new class object
  theClass = new Class(className,baseClass);

  if(baseClass)
  {
    // Initial amount of members for this class
    // Each object has the members of their parent classes
    theClass->SetSize(baseClass->GetSize());
  }

  // Save the class information
  m_vm->AddClass(theClass);

  // handle each variable declaration 
  while ((tkn = m_scanner->GetToken()) != '}') 
  {
    int storage = 0;

    // check for static members 
    if ((type = tkn) == T_STATIC)
    {
      storage = type;
      tkn = m_scanner->GetToken();
    }
    // get the first identifier
    if (tkn != T_IDENTIFIER)
    {
      m_scanner->ParseError(_T("Expecting a member declaration"));
    }
    member = m_scanner->GetTokenAsString();

    // check for a member function declaration 
    // Ignoring the parameter list for now
    if ((tkn = m_scanner->GetToken()) == '(') 
    {
      GetArgumentList(nullptr);
      FetchRequireToken(')');
      theClass->AddFunctionMember(m_vm,member,storage == T_STATIC ? ST_SFUNCTION : ST_FUNCTION);
    }
    else 
    {
      // handle data members
      while(true)
      {
        int type = FindDataType(member);
        if(type == DTYPE_NIL)
        {
          m_scanner->ParseError(_T("Class data members must have a data type"));
        }
        if(tkn != T_IDENTIFIER)
        {
          m_scanner->ParseError(_T("Class data member must be an identifier"));
        }
        member = m_scanner->GetTokenAsString();
        if(!theClass->AddDataMember(m_vm,member,storage == T_STATIC ? ST_SDATA : ST_DATA))
        {
          m_scanner->ParseError(_T("Global variable of this name already exists!"));
        }

        // Getting next token
        tkn = m_scanner->GetToken();
        if (tkn != ',')
        {
          m_scanner->SaveToken(tkn);
          break;
        }
        FetchRequireToken(T_IDENTIFIER);
        member = m_scanner->GetTokenAsString();
        tkn = m_scanner->GetToken();
      }
    }	    
    FetchRequireToken(';');
  }
}

// handle function declarations
void 
QLCompiler::do_function(CString p_name)
{
  switch (m_scanner->GetToken()) 
  {
    case _T('('): 	  do_regular_function(p_name);
                  break;
    case T_CC:	  do_member_function(get_class(p_name));
                  break;
    default:	    m_scanner->ParseError(_T("Expecting a function declaration"));
                  break;
  }
}

// parse a regular function definition 
void 
QLCompiler::do_regular_function(CString p_name)
{
  // enter the function name 
//   if(decode)
//   {
//     m_vm->Info("Function '%s'",p_name);
//   }

  // Create the function
  Function* regular = m_vm->AddScript(p_name);

  // compile the body of the function
  do_code(regular);

  // free the argument and temporary symbol lists
  // Cannot be done in do_code, as it can be called recursively
  m_arguments.clear();
  m_temporaries.clear();
}

// Parse a member function definition
void 
QLCompiler::do_member_function(Class* p_class)
{
  CString name;
  CString selector;
  MemObject* entry;
    
  // get the selector
  FetchRequireToken(T_IDENTIFIER);
  selector = m_scanner->GetTokenAsString();
  FetchRequireToken('(');
  name = p_class->GetName();
  if(m_decode)
  {
    m_vm->Info(_T("Member function '%s::%s'"),name,selector);
  }

  // make sure the type matches the declaration 
  if((entry = p_class->FindMember(selector)) != nullptr &&
      entry->m_type != DTYPE_SCRIPT) // &&
//       entry->m_type != DTYPE_INTERNAL &&
//       entry->m_type != DTYPE_EXTERNAL)
  {
    m_scanner->ParseError(_T("Illegal redefinition"));
  }

  // Add the Member function and get the function
  entry = p_class->AddFunctionMember(m_vm,selector,ST_FUNCTION);

  // compile the body of the function
  do_code(entry->m_value.v_script);

  // free the argument and temporary symbol lists
  // Cannot be done in do_code, as it can be called recursively
  m_arguments.clear();
  m_temporaries.clear();
}

// Compile the code part of a function or method 
void
QLCompiler::do_code(Function* p_function)
{
  int tkn   = 0;
  int tcnt  = 0;
  int tcode = 0;

  // initialize
  m_arguments.clear();
  m_temporaries.clear();
  // reset code pointer
  cptr = 0;

  // add the implicit 'this' argument for member functions
  if(p_function->GetClass() != nullptr)
  {
    AddArgument(_T("this"));
  }
  m_methodclass = p_function->GetClass();
    
  // get the argument list
  GetArgumentList(p_function);

  tkn = m_scanner->GetToken();
  RequireToken(tkn,')');
    
  // reserve space for the temporaries
  putcbyte(OP_TSPACE);
  tcode = putcbyte(tcnt);

  // compile the code
  FetchRequireToken('{');
  do_block();

  // Are we doing a constructor or destructor?
  if(p_function->GetClass())
  {
    if((p_function->GetName().Compare(p_function->GetClass()->GetName()) == 0) ||
       (p_function->GetName().Compare(_T("destroy")) == 0))
    {
      // End XTOR/DTOR with loading the 'this' pointer
      putcbyte(OP_ALOAD);
      putcbyte(0);
    }
  }

  // Do the return proper
  putcbyte(OP_RETURN);

  // Count the temporaries
  tcnt = (int)m_temporaries.size();

  // Fix the number of temporaries in TSPACE
  // We can only do this AFTER the function is compiled
  fixup_ref(tcode,tcnt + 1);

  // Copy the literals to the function
  if(m_literals)
  {
    for(int ind = 0;ind < m_literals->GetSize(); ++ind)
    {
      p_function->AddLiteral(m_vm,m_literals->GetEntry(ind));
    }
    FreeLiterals();
  }

  // copy the bytecode tot the function
  p_function->SetBytecode(cbuff,cptr);

  // show the generated code
  if(m_decode && m_debugger)
  {
    m_debugger->DecodeProcedure(p_function);
  }
}

// Get the class associated with a symbol
Class* 
QLCompiler::get_class(CString p_name)
{
  Class* sym = m_vm->FindClass(p_name);
  if (sym == nullptr)
  {
    m_scanner->ParseError(_T("Expecting a class name"));
  }
  return sym;
}

// Compile a single statement
void 
QLCompiler::do_statement()
{
  int tkn;
  switch (tkn = m_scanner->GetToken()) 
  {
    case T_IF:		    do_if();	    break;
    case T_WHILE:	    do_while();	  break;
    case T_DO:		    do_dowhile();	break;
    case T_FOR:		    do_for();	    break;
    case T_BREAK:	    do_break();	  break;
    case T_CONTINUE:	do_continue();break;
    case T_RETURN:	  do_return();	break;
    case T_SWITCH:    do_switch();  break;
    case T_CASE:      do_case();    break;
    case T_DEFAULT:   do_default(); break;
    case _T('{'):		      do_block();	  break;
    case _T(';'):		      ;		          break;
    default:		      m_scanner->SaveToken(tkn);
                      do_expr();
                      FetchRequireToken(';');  
                      break;
  }
}

// do_if - compile the IF/ELSE expression 
void 
QLCompiler::do_if()
{
  int tkn,nxt,end;

  // compile the test expression
  do_test();

  // skip around the 'then' clause if the expression is false
  putcbyte(OP_BRF);
  nxt = putcword(0);

  // compile the 'then' clause
  do_statement();

  // compile the 'else' clause
  if ((tkn = m_scanner->GetToken()) == T_ELSE) 
  {
    putcbyte(OP_BR);
    end = putcword(0);
    Fixup(nxt,cptr);
    do_statement();
    nxt = end;
  }
  else
  {
    m_scanner->SaveToken(tkn);
  }
  // handle the end of the statement
  Fixup(nxt,cptr);
}

// Add a break level to the stack
int* 
QLCompiler::addbreak(int lbl)
{
  int *old = bsp;
  if (++bsp < &bstack[SSIZE])
  {
    *bsp = lbl;
  }
  else
  {
    m_scanner->ParseError(_T("Too many nested loops"));
  }
  return (old);
}

// Remove a break level from the stack
int
QLCompiler::rembreak(int* old,int lbl)
{
   return (bsp > old ? *bsp-- : lbl);
}

// Add a continue level to the stack
int* 
QLCompiler::addcontinue(int lbl)
{
  int *old = csp;
  if (++csp < &cstack[SSIZE])
  {
    *csp = lbl;
  }
  else
  {
    m_scanner->ParseError(_T("Too many nested loops"));
  }
  return (old);
}

// remcontinue - remove a continue level from the stack
void
QLCompiler::remcontinue(int* old)
{
  csp = old;
}

// Compile the WHILE expression
void 
QLCompiler::do_while()
{
  int nxt,end,*ob,*oc;

  // compile the test expression
  nxt = cptr;
  do_test();

  // skip around the loop body if the expression is false
  putcbyte(OP_BRF);
  end = putcword(0);

  // compile the loop body
  ob = addbreak(end);
  oc = addcontinue(nxt);
  do_statement();
  end = rembreak(ob,end);
  remcontinue(oc);

  // branch back to the start of the loop
  putcbyte(OP_BR);
  putcword(nxt);

  // handle the end of the statement 
  Fixup(end,cptr);
}

// Compile the DO/WHILE expression
void 
QLCompiler::do_dowhile()
{
  int nxt,end=0,*ob,*oc;

  // remember the start of the loop
  nxt = cptr;

  // compile the loop body
  ob = addbreak(0);
  oc = addcontinue(nxt);
  do_statement();
  end = rembreak(ob,end);
  remcontinue(oc);

  // compile the test expression
  FetchRequireToken(T_WHILE);
  do_test();
  FetchRequireToken(';');

  // branch to the top if the expression is true
  putcbyte(OP_BRT);
  putcword(nxt);

  // handle the end of the statement
  Fixup(end,cptr);
}

// Compile the FOR statement 
void 
QLCompiler::do_for()
{
  int tkn,nxt,end,body,update,*ob,*oc;

  // compile the initialization expression 
  FetchRequireToken('(');
  if ((tkn = m_scanner->GetToken()) != ';') 
  {
    m_scanner->SaveToken(tkn);
    do_expr();
    FetchRequireToken(';');
  }

  // compile the test expression 
  nxt = cptr;
  if ((tkn = m_scanner->GetToken()) != ';') 
  {
    m_scanner->SaveToken(tkn);
    do_expr();
    FetchRequireToken(';');
  }

  // branch to the loop body if the expression is true
  putcbyte(OP_BRT);
  body = putcword(0);

  // branch to the end if the expression is false
  putcbyte(OP_BR);
  end = putcword(0);

  // compile the update expression
  update = cptr;
  if ((tkn = m_scanner->GetToken()) != ')') 
  {
    m_scanner->SaveToken(tkn);
    do_expr();
    FetchRequireToken(')');
  }

  // branch back to the test code
  putcbyte(OP_BR);
  putcword(nxt);

  // compile the loop body
  Fixup(body,cptr);
  ob = addbreak(end);
  oc = addcontinue(update);
  do_statement();
  end = rembreak(ob,end);
  remcontinue(oc);

  // branch back to the update code
  putcbyte(OP_BR);
  putcword(update);

  // handle the end of the statement
  Fixup(end,cptr);
}

// Compile the BREAK statement 
void 
QLCompiler::do_break()
{
  if (bsp >= bstack) 
  {
    putcbyte(OP_BR);
    *bsp = putcword(*bsp);
  }
  else
  {
    m_scanner->ParseError(_T("Break outside of loop"));
  }
}

// Compile the CONTINUE statement
void 
QLCompiler::do_continue()
{
  if (csp >= cstack) 
  {
    putcbyte(OP_BR);
    putcword(*csp);
  }
  else
  {
    m_scanner->ParseError(_T("Continue outside of loop"));
  }
}

// Add a switch level to the stack
SWENTRY*
QLCompiler::AddSwitch()
{
  SWENTRY *old = ssp;
  if (++ssp < &sstack[SSIZE]) 
  {
    ssp->nCases = 0;
    ssp->cases = nullptr;
    ssp->defaultLabel = 0;
  }
  else
  {
    m_scanner->ParseError(_T("Too many nested switch statements"));
  }
  return old;
}

// remove a switch level from the stack
void 
QLCompiler::RemoveSwitch(SWENTRY *old)
{
  CENTRY *entry,*next;
  for (entry = ssp->cases; entry != nullptr; entry = next) 
  {
    next = entry->next;
    free(entry);
  }
  ssp = old;
}

// Compile the SWITCH statement
void
QLCompiler::do_switch()
{
  int dispatch,end,cnt;
  int*     ob;
  SWENTRY* os;
  CENTRY*  e;

  // compile the test expression
  do_test();

  // branch to the dispatch code
  putcbyte(OP_BR);
  dispatch = putcword(0);

  // compile the body of the switch statement
  os = AddSwitch();
  ob = addbreak(0);
  do_statement();
  end = rembreak(ob,0);

  // branch to the end of the statement
  putcbyte(OP_BR);
  end = putcword(end);

  // compile the dispatch code
  Fixup(dispatch,cptr);
  putcbyte(OP_SWITCH);
  putcword(ssp->nCases);

  // output the case table
  cnt = ssp->nCases;
  e   = ssp->cases;
  while (--cnt >= 0) 
  {
    putcword(e->value);
    putcword(e->label);
    e = e->next;
  }
  if (ssp->defaultLabel)
  {
    putcword(ssp->defaultLabel);
  }
  else
  {
    end = putcword(end);
  }
  //resolve break targets
  Fixup(end,cptr);

  // remove the switch context
  RemoveSwitch(os);
}

// Compile the CASE statement
void
QLCompiler::do_case()
{
  if (ssp > ssbase) 
  {
    CENTRY **pNext,*entry;
    int value;

    // get the case value
    switch (m_scanner->GetToken()) 
    {
      case _T('\\'):        switch (m_scanner->GetToken()) 
                        {
                          case T_IDENTIFIER:  m_scanner->GetToken();
                                              value = make_lit_string(m_scanner->GetTokenAsString());
                                              break;
                          default:            m_scanner->ParseError(_T("Expecting a literal symbol"));
                                              value = 0; // never reached
                        }
                        break;
      case T_NUMBER:    value = make_lit_integer(m_scanner->GetTokenAsInteger());
                        break;
      case T_STRING:    value = make_lit_string(m_scanner->GetTokenAsString());
                        break;
      case T_NIL:       value = 0;
                        do_lit_integer(0);
                        break;
      default:          m_scanner->ParseError(_T("Expecting a literal value"));
                        value = 0; // never reached
    }
    FetchRequireToken(':');

    // find the place to add the new case
    for (pNext = &ssp->cases; (entry = *pNext) != nullptr; pNext = &entry->next) 
    {
      if (value < entry->value)
      {
        break;
      }
      else if (value == entry->value)
      {
        m_scanner->ParseError(_T("Duplicate case"));
      }
    }

    // add the case to the list of cases
    if ((entry = (CENTRY *)calloc(1,sizeof(CENTRY))) == nullptr)
    {
      m_scanner->ParseError(_T("Out of memory"));
      return;
    }
    entry->value = value;
    entry->label = cptr;
    entry->next  = *pNext;
    *pNext = entry;

    // increment the number of cases
    ++ssp->nCases;
  }
  else
  {
    m_scanner->ParseError(_T("Case outside of switch"));
  }
}

// Compile the 'default' statement
void 
QLCompiler::do_default()
{
  if (ssp > ssbase) 
  {
    FetchRequireToken(':');
    ssp->defaultLabel = cptr;
  }
  else
  {
    m_scanner->ParseError(_T("Default outside of switch"));
  }
}

// Count the number of temporaries so far
int
QLCompiler::CountOfTemporaries()
{
  return (int)m_temporaries.size();
}

// compile the {} block expression
void 
QLCompiler::do_block()
{
  int tkn = m_scanner->GetToken();

  if(FindDataType(m_scanner->GetTokenAsString()) != DTYPE_NIL)
  {
    int tcnt = CountOfTemporaries();
    // parse each local declaration 
    do 
    {
      // parse each variable and initializer
      do 
      {
        // Get local variable name
        FetchRequireToken(T_IDENTIFIER);
        AddTemporary(m_scanner->GetTokenAsString());
        ++tcnt;

        if ((tkn = m_scanner->GetToken()) == '=') 
        {
          do_init_expr();
          putcbyte(OP_TSTORE);
          putcbyte(tcnt - 1);
        }
        else
        {
          m_scanner->SaveToken(tkn);
        }
      } 
      while ((tkn = m_scanner->GetToken()) == ',');
      // Now end in semi-colon
      RequireToken(tkn,';');
      // Next also a declaration?
      tkn = m_scanner->GetToken();
    } 
    while(FindDataType(m_scanner->GetTokenAsString()) != DTYPE_NIL);
  }

  // Now do the rest of the block
  if(tkn != '}')
  {
    do 
    {
      m_scanner->SaveToken(tkn);
      do_statement();
    } 
    while ((tkn = m_scanner->GetToken()) != '}');
  }
  else
  {
    putcbyte(OP_NIL);
  }
}

// Handle the RETURN expression 
void 
QLCompiler::do_return()
{
  do_expr();
  FetchRequireToken(';');
  putcbyte(OP_RETURN);
}

// compile a test expression for an if/while
void 
QLCompiler::do_test()
{
  FetchRequireToken('(');
  do_expr();
  FetchRequireToken(')');
}

// Do the initializing expression of a temporary
void
QLCompiler::do_init_expr()
{
  PVAL pv;
  do_expr2(&pv);
  rvalue(&pv);
}

// Parse an expression
void 
QLCompiler::do_expr()
{
  PVAL pv;
  do_expr1(&pv);
  rvalue(&pv);
}

// get the rvalue of a partial expression
void 
QLCompiler::rvalue(PVAL* pv)
{
  if(pv->m_pval_type)
  {
    emit_code(pv->m_pval_type,LOAD,pv->m_value);
    pv->m_pval_type = PV_NOVALUE;
  }
}

// Make sure we've got an lvalue  (Left-value)
void 
QLCompiler::Check_LValue(PVAL* pv)
{
  if (pv->m_pval_type == PV_NOVALUE)
  {
    m_scanner->ParseError(_T("Expecting an lvalue"));
  }
}

// handle the ',' operator
void 
QLCompiler::do_expr1(PVAL* pv)
{
  int tkn;
  do_expr2(pv);
  while ((tkn = m_scanner->GetToken()) == ',') 
  {
    rvalue(pv);
    do_expr1(pv); 
    rvalue(pv);
  }
  m_scanner->SaveToken(tkn);
}

// do_expr2 - handle the assignment operators
void 
QLCompiler::do_expr2(PVAL* pv)
{
  int tkn; // ,nxt,end;
  PVAL rhs;
  do_expr3(pv);
  while (( tkn = m_scanner->GetToken()) == _T('=')
        || tkn == T_ADDEQ || tkn == T_SUBEQ
        || tkn == T_MULEQ || tkn == T_DIVEQ || tkn == T_REMEQ
        || tkn == T_ANDEQ || tkn == T_OREQ  || tkn == T_XOREQ
        || tkn == T_SHLEQ) 
  {
    Check_LValue(pv);
    switch (tkn) 
    {
      case _T('='): 	    emit_code(pv->m_pval_type,PUSH,0);
                      do_expr1(&rhs); 
                      rvalue(&rhs);
                      emit_code(pv->m_pval_type,STORE,pv->m_value);
                      break;
      case T_ADDEQ:	  do_assignment(pv,OP_ADD);	    break;
      case T_SUBEQ:	  do_assignment(pv,OP_SUB);	    break;
      case T_MULEQ:	  do_assignment(pv,OP_MUL);	    break;
      case T_DIVEQ:	  do_assignment(pv,OP_DIV);	    break;
      case T_REMEQ:	  do_assignment(pv,OP_REM);	    break;
      case T_ANDEQ:	  do_assignment(pv,OP_BAND);	  break;
      case T_OREQ:	  do_assignment(pv,OP_BOR);	    break;
      case T_XOREQ:	  do_assignment(pv,OP_XOR);	    break;
      case T_SHLEQ:	  do_assignment(pv,OP_SHL);	    break;
      case T_SHREQ:	  do_assignment(pv,OP_SHR);	    break;
    }
    pv->m_pval_type = PV_NOVALUE;
  }
  m_scanner->SaveToken(tkn);
}

// do_assignment - handle assignment operations
void 
QLCompiler::do_assignment(PVAL* pv,int op)
{
  PVAL rhs;

  emit_code(pv->m_pval_type,DUP,0);
  emit_code(pv->m_pval_type,LOAD,pv->m_value);
  putcbyte(OP_PUSH);
  do_expr1(&rhs); 
  rvalue(&rhs);
  putcbyte(op);
  emit_code(pv->m_pval_type,STORE,pv->m_value);
}

// do_expr3 - handle the '?:' operator
void 
QLCompiler::do_expr3(PVAL* pv)
{
  int tkn,nxt,end;
  do_expr4(pv);
  while ((tkn = m_scanner->GetToken()) == '?') 
  {
    rvalue(pv);
    putcbyte(OP_BRF);
    nxt = putcword(0);
    do_expr1(pv); 
    rvalue(pv);
    FetchRequireToken(':');
    putcbyte(OP_BR);
    end = putcword(0);
    Fixup(nxt,cptr);
    do_expr1(pv); 
    rvalue(pv);
    Fixup(end,cptr);
  }
  m_scanner->SaveToken(tkn);
}

// do_expr4 - handle the '||' operator
void 
QLCompiler::do_expr4(PVAL* pv)
{
  int tkn,end=0;
  do_expr5(pv);
  while ((tkn = m_scanner->GetToken()) == T_OR) 
  {
    rvalue(pv);
    putcbyte(OP_BRT);
    end = putcword(end);
    do_expr5(pv); 
    rvalue(pv);
  }
  Fixup(end,cptr);
  m_scanner->SaveToken(tkn);
}

// do_expr5 - handle the '&&' operator
void 
QLCompiler::do_expr5(PVAL* pv)
{
  int tkn,end=0;
  do_expr6(pv);
  while ((tkn = m_scanner->GetToken()) == T_AND) 
  {
    rvalue(pv);
    putcbyte(OP_BRF);
    end = putcword(end);
    do_expr6(pv); 
    rvalue(pv);
  }
  Fixup(end,cptr);
  m_scanner->SaveToken(tkn);
}

// do_expr6 - handle the '|' operator
void 
QLCompiler::do_expr6(PVAL* pv)
{
  int tkn;
  do_expr7(pv);
  while ((tkn = m_scanner->GetToken()) == '|') 
  {
    rvalue(pv);
    putcbyte(OP_PUSH);
    do_expr7(pv); 
    rvalue(pv);
    putcbyte(OP_BOR);
  }
  m_scanner->SaveToken(tkn);
}

// do_expr7 - handle the '^' operator
void 
QLCompiler::do_expr7(PVAL * pv)
{
  int tkn;
  do_expr8(pv);
  while ((tkn = m_scanner->GetToken()) == '^') 
  {
    rvalue(pv);
    putcbyte(OP_PUSH);
    do_expr8(pv); 
    rvalue(pv);
    putcbyte(OP_XOR);
  }
  m_scanner->SaveToken(tkn);
}

// do_expr8 - handle the '&' operator
void 
QLCompiler::do_expr8(PVAL* pv)
{
  int tkn;
  do_expr9(pv);
  while ((tkn = m_scanner->GetToken()) == '&') 
  {
    rvalue(pv);
    putcbyte(OP_PUSH);
    do_expr9(pv); 
    rvalue(pv);
    putcbyte(OP_BAND);
  }
  m_scanner->SaveToken(tkn);
}

// do_expr9 - handle the '==' and '!=' operators
void 
QLCompiler::do_expr9(PVAL* pv)
{
  int tkn,op;
  do_expr10(pv);
  while ((tkn = m_scanner->GetToken()) == T_EQ || tkn == T_NE) 
  {
    switch (tkn) 
    {
      case T_EQ: op = OP_EQ; break;
      case T_NE: op = OP_NE; break;
    }
    rvalue(pv);
    putcbyte(OP_PUSH);
    do_expr10(pv); 
    rvalue(pv);
    putcbyte(op);
  }
  m_scanner->SaveToken(tkn);
}

// do_expr10 - handle the '<', '<=', '>=' and '>' operators
void 
QLCompiler::do_expr10(PVAL* pv)
{
  int tkn,op;
  do_expr11(pv);
  while ((tkn = m_scanner->GetToken()) == _T('<') || tkn == T_LE || tkn == T_GE || tkn == '>') 
  {
    switch (tkn) 
    {
      case _T('<'):  op = OP_LT; break;
      case T_LE: op = OP_LE; break;
      case T_GE: op = OP_GE; break;
      case _T('>'):  op = OP_GT; break;
    }
    rvalue(pv);
    putcbyte(OP_PUSH);
    do_expr11(pv); 
    rvalue(pv);
    putcbyte(op);
  }
  m_scanner->SaveToken(tkn);
}

// do_expr11 - handle the '<<' and '>>' operators
void 
QLCompiler::do_expr11(PVAL* pv)
{
  int tkn,op;
  do_expr12(pv);
  while ((tkn = m_scanner->GetToken()) == T_SHL || tkn == T_SHR) 
  {
    switch (tkn) 
    {
      case T_SHL: op = OP_SHL; break;
      case T_SHR: op = OP_SHR; break;
    }
    rvalue(pv);
    putcbyte(OP_PUSH);
    do_expr12(pv); 
    rvalue(pv);
    putcbyte(op);
  }
  m_scanner->SaveToken(tkn);
}

// do_expr12 - handle the '+' and '-' operators
void 
QLCompiler::do_expr12(PVAL* pv)
{
  int tkn,op;
  do_expr13(pv);
  while ((tkn = m_scanner->GetToken()) == _T('+') || tkn == '-') 
  {
    switch (tkn) 
    {
      case _T('+'): op = OP_ADD; break;
      case _T('-'): op = OP_SUB; break;
    }
    rvalue(pv);
    putcbyte(OP_PUSH);
    do_expr13(pv); 
    rvalue(pv);
    putcbyte(op);
  }
  m_scanner->SaveToken(tkn);
}

// do_expr13 - handle the '*' and '/' operators
void 
QLCompiler::do_expr13(PVAL* pv)
{
  int tkn,op;
  do_expr14(pv);
  while ((tkn = m_scanner->GetToken()) == _T('*') || tkn == _T('/') || tkn == '%') 
  {
    switch (tkn) 
    {
      case _T('*'): op = OP_MUL; break;
      case _T('/'): op = OP_DIV; break;
      case _T('%'): op = OP_REM; break;
    }
    rvalue(pv);
    putcbyte(OP_PUSH);
    do_expr14(pv);
    rvalue(pv);
    putcbyte(op);
  }
  m_scanner->SaveToken(tkn);
}

// do_expr14 - handle unary operators
void 
QLCompiler::do_expr14(PVAL* pv)
{
  int tkn;
  switch (tkn = m_scanner->GetToken()) 
  {
    case _T('-'): 	  do_expr15(pv); 
                  rvalue(pv);
                  putcbyte(OP_NEG);
                  break;
    case _T('!'):	    do_expr15(pv); 
                  rvalue(pv);
                  putcbyte(OP_NOT);
                  break;
    case _T('~'):	    do_expr15(pv); 
                  rvalue(pv);
                  putcbyte(OP_BNOT);
                  break;
    case T_INC:   do_preincrement(pv,OP_INC);
                  break;
    case T_DEC:   do_preincrement(pv,OP_DEC);
                  break;
    case T_NEW:   do_new(pv);
                  break;
    case T_DELETE:do_delete(pv);
                  break;
    default:	    m_scanner->SaveToken(tkn);
                  do_expr15(pv);
                  return;
  }
}

// do_preincrement - handle prefix '++' and '--'
void 
QLCompiler::do_preincrement(PVAL* pv,int op)
{
  do_expr15(pv);
  Check_LValue(pv);
  emit_code(pv->m_pval_type,DUP,0);
  emit_code(pv->m_pval_type,LOAD,pv->m_value);
  putcbyte(op);
  emit_code(pv->m_pval_type,STORE,pv->m_value);
  pv->m_pval_type = PV_NOVALUE;
}

// do_postincrement - handle postfix '++' and '--'
void 
QLCompiler::do_postincrement(PVAL* pv,int op)
{
  Check_LValue(pv);
  emit_code(pv->m_pval_type,DUP,0);
  emit_code(pv->m_pval_type,LOAD,pv->m_value);
  putcbyte(op);
  emit_code(pv->m_pval_type,STORE,pv->m_value);
  putcbyte(op == OP_INC ? OP_DEC : OP_INC);
  pv->m_pval_type = PV_NOVALUE;
}

// Handle the 'new' operator
void 
QLCompiler::do_new(PVAL* pv)
{
  CString selector;
  MemObject* lit = nullptr;
  Class* v_class;

  FetchRequireToken(T_IDENTIFIER);
  selector = m_scanner->GetTokenAsString();

  v_class = get_class(selector);

  code_literal(AddLiteral(DTYPE_NIL,&lit));
  lit->m_type = DTYPE_CLASS;
  lit->m_value.v_class = v_class;
  lit->m_flags |=  FLAG_REFERENCE;
  lit->m_flags &= ~FLAG_DEALLOC;

  putcbyte(OP_NEW);
  pv->m_pval_type = PV_NOVALUE;
    
  do_send(selector,pv);
}

void
QLCompiler::do_delete(PVAL* pv)
{
  CString object;

  do_expr15(pv);
  // Getting it. Cross our fingers it's an object
  rvalue(pv);
  // Send optionally to the 'destroy' method
  putcbyte(OP_DESTROY);
  // Really delete the object
  putcbyte(OP_DELETE);
}

// Handle function calls
void 
QLCompiler::do_expr15(PVAL* pv)
{
  CString selector;
  int tkn;
  do_primary(pv);
  while ((tkn = m_scanner->GetToken()) == _T('(')
                        ||     tkn == _T('[')
                        ||     tkn == T_MEMREF
                        ||     tkn == T_INC
                        ||     tkn == T_DEC)
  {
    switch (tkn) 
    {
      case _T('('):       do_call(pv);
                      break;
      case _T('['):       do_index(pv);
                      break;
      case T_MEMREF:  FetchRequireToken(T_IDENTIFIER);
                      selector = m_scanner->GetTokenAsString();
                      do_send(selector,pv);
                      break;
      case T_INC:     do_postincrement(pv,OP_INC);
                      break;
      case T_DEC:     do_postincrement(pv,OP_DEC);
                      break;
    }
  }
  m_scanner->SaveToken(tkn);
}

// parse a primary expression and unary operators
void 
QLCompiler::do_primary(PVAL* pv)
{
  CString id;
  Class*  v_class = nullptr;
  int     tkn = 0;

  switch (m_scanner->GetToken()) 
  {
    case _T('('):         	do_expr1(pv);
                        FetchRequireToken(')');
                        break;
    case T_NUMBER:    	do_lit_integer((long)m_scanner->GetTokenAsInteger());
                        pv->m_pval_type = PV_NOVALUE;
                        break;
    case T_FLOAT:       do_lit_float(m_scanner->GetTokenAsFloat());
                        pv->m_pval_type = PV_NOVALUE;
                        break;
    case T_STRING:    	do_lit_string(m_scanner->GetTokenAsString());
                        pv->m_pval_type = PV_NOVALUE;
                        break;
    case T_NIL:       	putcbyte(OP_NIL);
                        break;
    case T_TRUE:        do_lit_integer(1);
                        pv->m_pval_type = PV_NOVALUE;
                        break;
    case T_FALSE:       do_lit_integer(0);
                        pv->m_pval_type = PV_NOVALUE;
                        break;
    case T_IDENTIFIER:	id = m_scanner->GetTokenAsString();
                        if ((tkn = m_scanner->GetToken()) == T_CC) 
                        {
                          v_class = get_class(id);
                          FetchRequireToken(T_IDENTIFIER);
                          if (!FindClassVariable(v_class,m_scanner->GetTokenAsString(),pv))
                          {
                            m_scanner->ParseError(_T("Not a class member"));
                          }
                        }
                        else 
                        {
                          m_scanner->SaveToken(tkn);
                          FindVariable(id,pv);
                        }
                        break;
    default:            m_scanner->ParseError(_T("Expecting a primary expression"));
                        break;
  }
}

// compile a function call
void 
QLCompiler::do_call(PVAL* pv)
{
  int tkn,n=0;
    
  // get the value of the function
  rvalue(pv);

  // compile each argument expression
  if ((tkn = m_scanner->GetToken()) != ')') 
  {
    m_scanner->SaveToken(tkn);
    do 
    {
      putcbyte(OP_PUSH);
      do_expr2(pv); 
      rvalue(pv);
      ++n;
    } 
    while ((tkn = m_scanner->GetToken()) == ',');
  }
  RequireToken(tkn,')');
  putcbyte(OP_CALL);
  putcbyte(n);

  // we've got an rvalue now
  pv->m_pval_type = PV_NOVALUE;
}

// compile a message sending expression
void 
QLCompiler::do_send(CString selector,PVAL* pv)
{
  MemObject *lit = nullptr;
  int tkn = 0;
  int n   = 1;    // Arguments is at minimum 1 for the this pointer
    
  // get the receiver value
  rvalue(pv);

  // generate code to push the selector
  // Will be replaced by the 'this' pointer before the call!
  putcbyte(OP_PUSH);
  code_literal(AddLiteral(DTYPE_STRING,&lit,selector));
  if(lit)
  {
    *(lit->m_value.v_string) = selector;
  }

  // compile the argument list
  FetchRequireToken('(');
  if ((tkn = m_scanner->GetToken()) != ')') 
  {
    m_scanner->SaveToken(tkn);
    do 
    {
      putcbyte(OP_PUSH);
      do_expr2(pv); 
      rvalue(pv);
      ++n;
    } 
    while ((tkn = m_scanner->GetToken()) == ',');
  }
  RequireToken(tkn,')');

  // send the method message to the object
  putcbyte(OP_SEND);
  putcbyte(n);

  // we've got an rvalue now
  pv->m_pval_type = PV_NOVALUE;
}

// Compile an indexing operation
void 
QLCompiler::do_index(PVAL* pv)
{
  rvalue(pv);
  putcbyte(OP_PUSH);
  do_expr();
  FetchRequireToken(']');
  pv->m_pval_type = PV_CODE;
}

// get a comma separated list of identifiers
int 
QLCompiler::GetArgumentList(Function* p_function)
{
  int tkn,cnt=0;
  tkn = m_scanner->GetToken();
  if(tkn != ')')
  {
    m_scanner->SaveToken(tkn);
    do 
    {
      FetchRequireToken(T_IDENTIFIER);
      CString datatype = m_scanner->GetTokenAsString();
      int type = FindDataType(datatype);
      if(type == DTYPE_NIL)
      {
        m_scanner->ParseError(_T("Expected a valid argument data type"));
      }
      FetchRequireToken(T_IDENTIFIER);
      CString argument = m_scanner->GetTokenAsString();
      if(p_function)
      {
        p_function->AddArgument(type);
        AddArgument(argument);
      }
      ++cnt;
    } 
    while ((tkn = m_scanner->GetToken()) == ',');
  }
  m_scanner->SaveToken(tkn);
  return (cnt);
}

// DTYPE MACRO + 3!!
const TCHAR* internal_datatypes[]
{
   _T("int")
  ,_T("string")
  ,_T("bcd")
  ,_T("file")
  ,_T("dbase")
  ,_T("query")
  ,_T("variant")
  ,_T("array")
};

int
QLCompiler::FindDataType(CString p_name)
{
  int index = DTYPE_INTEGER;
  for(auto& name : internal_datatypes)
  {
    if(p_name.Compare(name) == 0)
    {
      return index;
    }
    ++index;
  }
  if(m_vm->FindClass(p_name))
  {
    return DTYPE_OBJECT;
  }
  return DTYPE_NIL;
}

// add a formal argument
void 
QLCompiler::AddArgument(CString p_name)
{
  m_arguments.push_back(p_name);
}

void
QLCompiler::AddTemporary(CString p_name)
{
  m_temporaries.push_back(p_name);
}

// free a list of arguments or temporaries
void 
QLCompiler::freelist(ARGUMENT* p_list)
{
  p_list->clear();
}

// find an argument offset 
int 
QLCompiler::FindArgument(CString p_name)
{
  int ind = 0;
  for(auto& arg : m_arguments)
  {
    if(arg.Compare(p_name) == 0)
    {
      return ind;
    }
    ++ind;
  }
  return (-1);
}

// Find a temporary variable offset
int 
QLCompiler::FindTemporary(CString p_name)
{
  int ind = 0;
  for(auto& tmp : m_temporaries)
  {
    if(tmp.Compare(p_name) == 0)
    {
      return ind;
    }
    ++ind;
  }
  return -1;
}

// Find a class data member
MemObject*
QLCompiler::FindDataMember(CString p_name)
{
  if(m_methodclass != nullptr)
  {
    return m_methodclass->RecursiveFindDataMember(p_name);
  }
  return nullptr;
}

// add a literal
int 
QLCompiler::AddLiteral(int p_type,MemObject** p_result,CString p_name/*=""*/,int p_value /*= 0*/)
{
  if(m_literals == nullptr)
  {
    m_literals = new Array();
  }
  // Optimize string literals
  if(p_type == DTYPE_STRING)
  {
    int n = m_literals->FindStringEntry(p_name);
    if(n >= 0)
    {
      if(*p_result == nullptr)
      {
        *p_result = m_literals->GetEntry(n);
      }
      return n;
    }
  } 
  // Optimize integer literals
  if(p_type == DTYPE_INTEGER)
  {
    int n = m_literals->FindIntegerEntry(p_value);
    if(n >= 0)
    {
      if(*p_result == nullptr)
      {
        *p_result = m_literals->GetEntry(n);
      }
      return n;
    }
  }
  int n = m_literals->GetSize();
  MemObject* lit = m_literals->AddEntryOfType(m_vm,p_type);
  *p_result = lit;
  return n;
}

// Free a list of literals
void 
QLCompiler::FreeLiterals()
{
  if(m_literals)
  {
    delete m_literals;
    m_literals = nullptr;
  }
}

// Fetch a token and check it
void 
QLCompiler::FetchRequireToken(int rtkn)
{
  RequireToken(m_scanner->GetToken(),rtkn);
}

// check for a required token
void 
QLCompiler::RequireToken(int tkn,int rtkn)
{
  CString message;

  if (tkn != rtkn) 
  {
    message.Format(_T("Expected token '%s', found '%s'")
                  ,m_scanner->TokenName(rtkn).GetString()
                  ,m_scanner->TokenName(tkn) .GetString());
    m_scanner->ParseError(message);
  }
}

// compile a literal integer
void 
QLCompiler::do_lit_integer(long n)
{
  MemObject* lit = nullptr;
  code_literal(AddLiteral(DTYPE_INTEGER,&lit,_T(""),n));
  lit->m_value.v_integer = n;
}

void
QLCompiler::do_lit_float(bcd fl)
{
  MemObject* lit = nullptr;
  code_literal(AddLiteral(DTYPE_BCD,&lit));
  *(lit->m_value.v_floating) = fl;
}

// compile a literal string
void 
QLCompiler::do_lit_string(CString str)
{
  code_literal(make_lit_string(str));
}

// make a literal string
int 
QLCompiler::make_lit_string(CString p_string)
{
  MemObject* lit = nullptr;
  int n = AddLiteral(DTYPE_STRING,&lit,p_string);
  if(lit)
  {
    // Only fill in if we made a new string
    *(lit->m_value.v_string) = p_string;
  }
  return n;
}

int
QLCompiler::make_lit_integer(long p_value)
{
  MemObject* lit;
  int n = AddLiteral(DTYPE_INTEGER,&lit);
  lit->m_value.v_integer = p_value;
  return n;
}

// make a literal reference to a variable
int 
QLCompiler::make_lit_variable(MemObject* p_sym)
{
  if(m_literals == nullptr)
  {
    m_literals = new Array();
  }
  // Find object by pointer reference
  int n = m_literals->FindEntry(p_sym);
  if(n >= 0)
  {
    return n;
  }
  // Make new entry
  n = m_literals->GetSize();
  m_literals->AddEntry(p_sym);
  return n;
}

// Find a variable
void 
QLCompiler::FindVariable(CString p_name,PVAL* pv)
{    
  int n = 0;

  if ((n = FindArgument(p_name)) >= 0) 
  {
    pv->m_pval_type = PV_ARGUMENT;
    pv->m_value = n;
  }
  else if ((n = FindTemporary(p_name)) >= 0) 
  {
    pv->m_pval_type = PV_TEMPORARY;
    pv->m_value = n;
  }
  else if (m_methodclass == nullptr || 
          !FindClassVariable(m_methodclass,p_name,pv)) 
  {
    if((n = m_vm->FindGlobal(p_name)) >= 0)
    {
      pv->m_pval_type = PV_VARIABLE;
      pv->m_value = n;
    }
    else
    {
      pv->m_pval_type = PV_LITERAL;
      pv->m_value = make_lit_variable(m_vm->AddSymbol(p_name));
    }
  }
}

// find a class member variable 
int 
QLCompiler::FindClassVariable(Class* p_class,CString p_name,PVAL* pv)
{
  int offset = 0;
  MemObject* entry = p_class->RecursiveFindMember(p_name);
  if(entry == nullptr)
  {
    return FALSE;
  }
  switch (entry->m_storage) 
  {
    case ST_DATA:     	pv->m_pval_type = PV_MEMBER;
                        p_class->RecursiveFindDataMember(p_name,offset);
                        pv->m_value = offset;
                        break;
    case ST_SDATA:	    pv->m_pval_type = PV_VARIABLE;
                        pv->m_value = m_vm->AddGlobal(entry,p_name);
                        break;
    case ST_FUNCTION: 	FindVariable(_T("this"),pv);
                        do_send(p_name,pv);
                        break;
    case ST_SFUNCTION:	code_literal(make_lit_variable(entry));
                        pv->m_pval_type = PV_NOVALUE;
                        break;
  }
  return (TRUE);
}

// Emit bytecode
void
QLCompiler::emit_code(int p_type,int p_code,int p_value)
{
  switch(p_type)
  {
    case PV_CODE:       code_index    (p_code);         break;
    case PV_ARGUMENT:   code_argument (p_code,p_value); break;
    case PV_TEMPORARY:  code_temporary(p_code,p_value); break;
    case PV_VARIABLE:   code_variable (p_code,p_value); break;
    case PV_LITERAL:    code_literal  (p_value);        break;
    case PV_MEMBER:     code_member   (p_code,p_value); break;
  }
}

// compile an argument reference
void 
QLCompiler::code_argument(int fcn,int n)
{
  switch (fcn) 
  {
    case LOAD:	putcbyte(OP_ALOAD);  putcbyte(n); break;
    case STORE:	putcbyte(OP_ASTORE); putcbyte(n); break;
  }
}

// compile a temporary variable reference
void 
QLCompiler::code_temporary(int fcn,int n)
{
  switch (fcn) 
  {
    case LOAD:	putcbyte(OP_TLOAD);  putcbyte(n); break;
    case STORE:	putcbyte(OP_TSTORE); putcbyte(n); break;
  }
}

// compile a data member reference
void 
QLCompiler::code_member(int fcn,int n)
{
  switch (fcn) 
  {
    case LOAD:	putcbyte(OP_MLOAD);  putcbyte(n); break;
    case STORE:	putcbyte(OP_MSTORE); putcbyte(n); break;
  }
}

// Compile a variable reference
void 
QLCompiler::code_variable(int fcn,int n)
{
  switch (fcn) 
  {
    case LOAD:	putcbyte(OP_LOAD);  putcbyte(n); break;
    case STORE:	putcbyte(OP_STORE); putcbyte(n); break;
  }
}

// Compile an indexed reference
int
QLCompiler::code_index(int fcn)
{
  switch (fcn) 
  {
    case LOAD:	putcbyte(OP_VLOAD);  break;
    case STORE:	putcbyte(OP_VSTORE); break;
    case PUSH:  putcbyte(OP_PUSH);   break;
    case DUP:	  putcbyte(OP_DUP2);   break;
  }
  return 0;
}

// code_literal - compile a literal reference
void 
QLCompiler::code_literal(int n)
{
  putcbyte(OP_LIT);
  putcbyte(n);
}

// put a code byte into data space
int 
QLCompiler::putcbyte(int b)
{
  if (cptr >= CMAX)
  {
    m_scanner->ParseError(_T("Insufficient code space"));
  }
  cbuff[cptr] = b;
  return (cptr++);
}

// put a code word into data space
int 
QLCompiler::putcword(int w)
{
  putcbyte(w);
  putcbyte(w >> 8);
  return (cptr-2);
}

// Fixup a single bytecode reference
void
QLCompiler::fixup_ref(int chn,int val)
{
  cbuff[chn] = val & 0xFF;
}

// Fixup a reference chain
void 
QLCompiler::Fixup(int chn,int val)
{
  int hval,nxt;
  for (hval = val >> 8; chn != 0; chn = nxt) 
  {
    nxt = (cbuff[chn] & 0xFF) | (cbuff[chn+1] << 8);
    cbuff[chn] = val;
    cbuff[chn+1] = hval;
  }
}

#pragma warning(disable: 4996)

// allocate memory and complain if there isn't enough
TCHAR*
QLCompiler::GetMemory(int size)
{
  TCHAR *val;
  if ((val = (TCHAR*)calloc(1,size)) == nullptr)
  {
    m_vm->Error(_T("Insufficient memory"));
  }
  return (val);
}
