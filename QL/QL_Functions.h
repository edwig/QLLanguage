//////////////////////////////////////////////////////////////////////////
//
// QL Language functions
// ir. W.E. Huisman (c) 2018
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "QL_vm.h"

/* init_functions - initialize the internal functions */
void init_functions(QLVirtualMachine* p_vm);

// MUST BE DEFINED IN THE MAIN PROGRAM DRIVER!!
// Used for the xgetarg function
extern int    qlargc;
extern char** qlargv;

extern CString db_database;
extern CString db_user;
extern CString db_password;
