//////////////////////////////////////////////////////////////////////////
//
// QL_main.cpp : Defines the entry point for the console driver
// ir. W.E. Huisman (c) 2015 - 2018
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "QL_Language.h"
#include "QL_vm.h"
#include "QL_Debugger.h"
#include "QL_Compiler.h"
#include "QL_Interpreter.h"
#include "QL_Exception.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace std;

// Static command line options
bool    g_verbose     = false;
bool    g_comptrace   = false;
bool    g_objecttrace = false;
bool    g_inttrace    = false;
bool    g_objectfile  = false;
bool    g_dumpmem     = false;
CString g_entrypoint(_T("main"));

// Provide standard drivers for output
void osputs_stdout(LPCTSTR p_string)
{
  _fputts(p_string,stdout);
}

void osputs_stderr(LPCTSTR p_string)
{
  _fputts(p_string,stderr);
}

void PrintVersion()
{
  if (g_verbose)
  {
    _tprintf(_T("%s\n"),          QUANTUM_PROMPT);
    _tprintf(_T("Version: %s\n"), QUANTUM_VERSION);
  }
}

void
Usage()
{
  _tprintf(_T("\n")
         _T("ql [options] sourcefile[.ql] [objectfile.qob]\n")
         _T("\n")
         _T("The following are valid options\n")
         _T("-v        Verbose + copyrights\n")
         _T("-c        Compile only to object file\n")
         _T("-b        Show compiling trace (object, method, bytecode)\n")
         _T("-t        Show tracing bytecode execution\n")
         _T("-e name   Use 'name' as entry point, other than 'main'\n")
         _T("-d name   Use 'name' as database name\n")
         _T("-u name   Use 'name' as database user's name\n")
         _T("-p word   Use 'word' as database password\n")
         _T("-h        Show this help page\n")
         _T("-o        show contents of object file\n")
         _T("-x        Dump object chain on exit\n"));
}

bool
ParseCommandLine(int& index)
{
  for (index = 1; index < __argc; ++index)
  {
    LPCTSTR lpszParam = __targv[index];

    if (lpszParam[0] == _T('-') || lpszParam[0] == '/')
    {
      if (_totlower(lpszParam[1]) == _T('h') || lpszParam[1] == '?')
      {
        g_verbose = true;
        PrintVersion();
        Usage();
        return false;
      }
      else if (_totlower(lpszParam[1]) == 'c')
      {
        g_objectfile = true;
      }
      else if (_totlower(lpszParam[1]) == 'v')
      {
        g_verbose = true;
      }
      else if (_totlower(lpszParam[1]) == 'b')
      {
        g_comptrace = true;
      }
      else if (_totlower(lpszParam[1]) == 't')
      {
        g_inttrace = true;
      }
      else if(_totlower(lpszParam[1]) == 'x')
      {
        g_dumpmem = true;
      }
      else if(_totlower(lpszParam[1]) == 'o')
      {
        g_objecttrace = true;
      }
      else if (_totlower(lpszParam[1]) == 'e')
      {
        g_entrypoint = __targv[++index];
        if(g_entrypoint.IsEmpty())
        {
          g_entrypoint = _T("main");
        }
      }
      else if (_totlower(lpszParam[1]) == 'd')
      {
        db_database = __targv[++index];
      }
      else if (_totlower(lpszParam[1]) == 'u')
      {
        db_user = __targv[++index];
      }
      else if (_totlower(lpszParam[1]) == 'p')
      {
        db_password = __targv[++index];
      }
      else
      {
        _tprintf(_T("Unknown option: %s"),lpszParam);
        Usage();
        return false;
      }
    }
    else
    {
      // Stops at the first non-option on the command line
      break;
    }
  }
  return true;
}

int InitMFC()
{
  HMODULE hModule = ::GetModuleHandle(NULL);
  if(hModule == NULL)
  {
    _tprintf(_T("QL: Fatal Error: module initialization failed\n"));
    return 1;
  }
  // initialize MFC and print and error on failure
  if(!AfxWinInit(hModule,NULL,::GetCommandLine(),0))
  {
    _tprintf(_T("QL: Fatal Error: MFC initialization failed\n"));
    return 1;
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// MAIN PROGRAM DRIVER
//
//////////////////////////////////////////////////////////////////////////

int _tmain(int argc,TCHAR* argv[],TCHAR* envp[])
{
  // Initialize MFC and String management
  int returnCode = InitMFC();
  if(returnCode)
  {
    return returnCode;
  }

  // Record argument list
  qlargc = argc;
  qlargv = argv;

  bool compiled = false;

  // Handle command line input and act on it
  if(argc > 1)
  {
    int ind = 0;
    if(ParseCommandLine(ind))
    {
      QLVirtualMachine vm;

      PrintVersion();
      while(ind < argc)
      {
        if(vm.IsObjectFile(argv[ind]) && !g_objectfile)
        {
          compiled = vm.LoadFile(argv[ind],g_objecttrace);
          if(compiled && g_verbose)
          {
            _tprintf(_T("Read object file: %s\n"),argv[ind]);
          }
        }
        else if(vm.IsSourceFile(argv[ind]))
        {
          compiled = vm.CompileFile(argv[ind],g_comptrace);
          if(compiled && g_verbose)
          {
            _tprintf(_T("Compiled source file: %s\n"),argv[ind]);
          }
        }
        else
        {
          _ftprintf(stderr,_T("File not found: %s\n"),argv[ind]);
          returnCode = 1;
        }
        // Next argument from command line
        ++ind;
      }

      if(g_objectfile)
      {
        // Last argument is the object file
        if(vm.WriteFile(argv[argc - 1],g_objecttrace) && g_verbose)
        {
          _tprintf(_T("Written object file: %s\n"),argv[argc - 1]);
        }
      }
      else if(compiled)
      {
        // Now execute main or the entrypoint
        try
        {
          vm.SetDumping(g_dumpmem);

          QLInterpreter inter(&vm, g_inttrace);
          returnCode = inter.Execute(g_entrypoint);
        }
        catch(int &error)
        {
          returnCode = error;
        }
        catch(QLException& exp)
        {
          returnCode = -1;
          _ftprintf(stderr,_T("%s\n"),exp.GetErrorMessage().GetString());
        }
      }
    }
  }
  else
  {
    // Started with no arguments: tell the world who we are
    g_verbose = true;
    PrintVersion();
    Usage();
  }
  return returnCode;
}
