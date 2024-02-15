//////////////////////////////////////////////////////////////////////////
//
// QL Language exception
// ir. W.E. Huisman (c) 2018
//
//////////////////////////////////////////////////////////////////////////

#pragma once

const int EXCEPTION_BY_ERROR = 2;

class QLException
{
public:
  QLException(TCHAR* p_message);
  QLException(TCHAR* p_message, int p_code);
  QLException(CString p_message,int p_code);
 ~QLException();

  // Get the message
  CString GetMessage()  { return m_message; };
  // Get the code
  int     GetCode()     { return m_code; };
  // Get the error
  CString GetErrorMessage();
private:
  int     m_code;
  CString m_message;
};