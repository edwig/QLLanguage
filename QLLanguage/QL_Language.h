//////////////////////////////////////////////////////////////////////////
//
// Q++ Language header
//
// 2014-2018 (c) ir. W.E. Huisman
//
//////////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"
#include "bcd.h"
#include <map>
#include <vector>
#include <SQLDatabase.h>
#include <SQLQuery.h>
#include <SQLVariant.h>

// VERSION OF QL LANGUAGE
// USED IN *.qob FILES
#define QL_VERSION        200 // 2.00

#define QUANTUM_PROMPT    _T("Quantum Language (c) 2014-2018 ir. W.E. Huisman")
#define QUANTUM_VERSION   _T("2.0")

// Tracing macro
#define tracing(x)          if(p_trace) _ftprintf(stderr,x)
#define tracingx(x,...)     if(p_trace) _ftprintf(stderr,x,__VA_ARGS__)

// Maximum size of stack up to 0xFFFF
#define STACK_MAX         0x7FFF // 32767
#define STACK_DEFAULT     0x07D0 //  2000

// Datatypes for QL
#define DTYPE_ENDMARK     0x0001
#define DTYPE_NIL         0x0002
#define DTYPE_INTEGER     0x0003
#define DTYPE_STRING      0x0004
#define DTYPE_BCD         0x0005
#define DTYPE_FILE        0x0006
#define DTYPE_DATABASE    0x0007
#define DTYPE_QUERY       0x0008
#define DTYPE_VARIANT     0x0009
#define DTYPE_ARRAY       0x000A
#define DTYPE_OBJECT      0x000B
#define DTYPE_CLASS       0x000C
#define DTYPE_SCRIPT      0x000D
#define DTYPE_INTERNAL    0x000E
#define DTYPE_EXTERNAL    0x000F
#define DTYPE_STREAM      0x0010
// Added to type on storage of a FLAG_REFERENCE object 
// in a file stream (e.g. on disk) (String is the name)
#define DTYPE_REFERENCE   0x0080
#define DTYPE_MASK        0x007F

// Minimum and maximum datatype
#define _DTMIN   DTYPE_ENDMARK
#define _DTMAX   DTYPE_EXTERNAL

// Type flags
#define FLAG_DEALLOC      0x0001    // Object should be deallocated on free
#define FLAG_NULL         0x0002    // Object is logical NULL
#define FLAG_REFERENCE    0x0004    // Object is not garbage collected (REFERENCE!!)

// GC Generation marks
#define GC_ALIVE          0x0001
#define GC_MARKED         0x0002
#define GC_SWEEP          0x0004

// Storage class symbol types 
#define ST_CLASS	    1	  // Class definition       lives in m_classes
#define ST_DATA		    2	  // data member            lives in m_classes class.attributes
#define ST_SDATA	    3	  // static data member     lives in m_globals
#define ST_FUNCTION	  4	  // function member        lives in m_classes class.members
#define ST_SFUNCTION	5	  // static function member lives in m_symbols/m_globals

// Forward declarations for many objects
class Array;
class Class;
class Object;
class Function;
class MemObject;
class QLVirtualMachine;
class QLInterpreter;

// Shortcut for many defines
#define QLvm  QLVirtualMachine

// Types for the MemObject
typedef int (*Internal)(QLInterpreter*,int);
typedef _TUCHAR shortint;

// Method for internal data types
typedef struct _method
{
  int       m_datatype;
  CString   m_methodname;
  Internal  m_internal;
}
Method;

// Name mapping for global objects in the virtual machine
typedef std::map<CString, MemObject*>     NameMap;
typedef std::map<CString, Class*>         ClassMap;
typedef std::multimap<CString, Method*>   MethodMap;

// Globals for the QL library
extern int    qlargc;       // Rest of the startup parameters
extern TCHAR** qlargv;       // All startup parameters

extern CString db_database; // Default database name
extern CString db_user;     // Default user name for the database
extern CString db_password; // Default password for the database user

// Forward declarations
// Implementations must provide these!
void osputs_stdout(LPCTSTR str);
void osputs_stderr(LPCTSTR str);

// Selecting the right library to link with automatically
// So we do not need to worry about which library to use in the linker settings
#ifdef UNICODE

#if defined _M_IX86
#define QLL_PLATFORM "Ux86"
#else
#define QLL_PLATFORM "Ux64"
#endif

#else // UNICODE

#if defined _M_IX86
#define QLL_PLATFORM "x86"
#else
#define QLL_PLATFORM "x64"
#endif

#endif // UNICODE


#if defined _DEBUG
#define QLL_CONFIGURATION "D"
#else
#define QLL_CONFIGURATION "R"
#endif 

#ifndef QLL_COMPONENTS_NOAUTOLINK
#pragma comment(lib,"QL_Language_"                       QLL_PLATFORM QLL_CONFIGURATION ".lib")
#pragma message("Automatically linking with QLLanguage_" QLL_PLATFORM QLL_CONFIGURATION ".lib")
#else
#pragma message("Now building library QLLanguage_" QLL_PLATFORM QLL_CONFIGURATION ".lib")
#endif 
