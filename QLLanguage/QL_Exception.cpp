//////////////////////////////////////////////////////////////////////////
//
// QL Language exception
// ir. W.E. Huisman (c) 2018
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "QL_Exception.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

QLException::QLException(TCHAR* p_message)
            :m_message(p_message)
            ,m_code(0)

{
}

QLException::QLException(TCHAR* p_message, int p_code)
            :m_message(p_message)
            ,m_code(p_code)

{
}

QLException::QLException(CString p_message, int p_code)
            :m_message(p_message)
            ,m_code(p_code)

{
}

QLException::~QLException()
{
}

CString
QLException::GetErrorMessage()
{
  CString error;
  error.Format(_T("ERROR [%d] : %s"), m_code, m_message);
  return error;
}
