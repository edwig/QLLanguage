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
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// The one and only application object

using namespace std;

int    qlargc = 0;
char** qlargv = NULL;

// Static command line options
bool    verbose     = false;
bool    comptrace   = false;
bool    objecttrace = false;
bool    inttrace    = false;
bool    objectfile  = false;
bool    dumpmem     = false;
CString entrypoint("main");
CString db_database;
CString db_user;
CString db_password;

void PrintVersion()
{
  if (verbose)
  {
    printf("%s\n",          QUANTUM_PROMPT);
    printf("Version: %s\n", QUANTUM_VERSION);
  }
}

void
Usage()
{
  printf("\n"
         "ql [options] sourcefile[.ql] [objectfile.qob]\n"
         "\n"
         "The following are valid options\n"
         "-v        Verbose + copyrights\n"
         "-c        Compile only to object file\n"
         "-b        Show compiling trace (object, method, bytecode)\n"
         "-t        Show tracing bytecode execution\n"
         "-e name   Use 'name' as entry point, other than 'main'\n"
         "-d name   Use 'name' as database name\n"
         "-u name   Use 'name' as database user's name\n"
         "-p word   Use 'word' as database password\n"
         "-h        Show this help page\n"
         "-o        show contents of object file\n"
         "-x        Dump object chain on exit\n");
}

bool
ParseCommandLine(int& index)
{
  for (index = 1; index < __argc; ++index)
  {
    LPCSTR lpszParam = __argv[index];

    if (lpszParam[0] == '-' || lpszParam[0] == '/')
    {
      if (tolower(lpszParam[1]) == 'h' || lpszParam[1] == '?')
      {
        verbose = true;
        PrintVersion();
        Usage();
        return false;
      }
      else if (tolower(lpszParam[1]) == 'c')
      {
        objectfile = true;
      }
      else if (tolower(lpszParam[1]) == 'v')
      {
        verbose = true;
      }
      else if (tolower(lpszParam[1]) == 'b')
      {
        comptrace = true;
      }
      else if (tolower(lpszParam[1]) == 't')
      {
        inttrace = true;
      }
      else if(tolower(lpszParam[1]) == 'x')
      {
        dumpmem = true;
      }
      else if(tolower(lpszParam[1]) == 'o')
      {
        objecttrace = true;
      }
      else if (tolower(lpszParam[1]) == 'e')
      {
        entrypoint = __argv[++index];
        if (entrypoint.IsEmpty())
        {
          entrypoint = "main";
        }
      }
      else if (tolower(lpszParam[1]) == 'd')
      {
        db_database = __argv[++index];
      }
      else if (tolower(lpszParam[1]) == 'u')
      {
        db_user = __argv[++index];
      }
      else if (tolower(lpszParam[1]) == 'p')
      {
        db_password = __argv[++index];
      }
      else
      {
        printf("Unknown option: %s",lpszParam);
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

void osputs(const char* p_string)
{
  fputs(p_string, stderr);
}

//////////////////////////////////////////////////////////////////////////
//
// MAIN PROGRAM DRIVER
//
//////////////////////////////////////////////////////////////////////////

int main(int argc,char* argv[],char* envp[])
{
  int nRetCode = 0;

  HMODULE hModule = ::GetModuleHandle(NULL);
  if (hModule == NULL)
  {
    printf("QL: Fatal Error: module initialization failed\n");
    nRetCode = 1;
  }
  else
  {
    // initialize MFC and print and error on failure
    if(!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
    {
      printf("QL: Fatal Error: MFC initialization failed\n");
      nRetCode = 1;
    }
    else
    {
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
            if(vm.IsObjectFile(argv[ind]) && !objectfile)
            {
              compiled = vm.LoadFile(argv[ind],objecttrace);
              if(compiled && verbose)
              {
                printf("Read object file: %s\n",argv[ind]);
              }
            }
            else if (vm.IsSourceFile(argv[ind]))
            {
              compiled = vm.CompileFile(argv[ind],comptrace);
              if(compiled && verbose)
              {
                printf("Compiled source file: %s\n",argv[ind]);
              }
            }
            else
            {
              fprintf(stderr,"File not found: %s\n",argv[ind]);
              nRetCode = 1;
            }
            // Next argument from command line
            ++ind;
          }

          if(objectfile)
          {
            // Last argument is the object file
            if(vm.WriteFile(argv[argc - 1],objecttrace) && verbose)
            {
              printf("Written object file: %s\n",argv[argc - 1]);
            }
          }
          else if(compiled)
          {
            // Now execute main or the entrypoint
            try
            {
              vm.SetDumping(dumpmem);

              QLInterpreter inter(&vm, inttrace);
              nRetCode = inter.Execute(entrypoint);
            }
            catch(int &error)
            {
              nRetCode = error;
            }
            catch(QLException& exp)
            {
              nRetCode = -1;
              fprintf(stderr,"%s\n",exp.GetErrorMessage().GetString());
            }
          }
        }
      }
      else
      {
        // Started with no arguments: tell the world who we are
        verbose = true;
        PrintVersion();
        Usage();
      }
    }
  }
  return nRetCode;
}
