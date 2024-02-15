#include "stdafx.h"
#include "RunRedirect.h"
#include "CppUnitTest.h"
#include <direct.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTests
{		
	TEST_CLASS(UnitTest)
	{
	public:
		
		TEST_METHOD(test_add)
		{
      DoTheTest(_T("test_add"));
		}

    TEST_METHOD(test_array)
    {
      DoTheTest(_T("test_array"));
    }

    TEST_METHOD(test_basic)
    {
      DoTheTest(_T("test_basic"));
    }

    TEST_METHOD(test_call)
    {
      DoTheTest(_T("test_call"));
    }

    TEST_METHOD(test_constructor_2)
    {
      DoTheTest(_T("test_constructor_2"));
    }

    TEST_METHOD(test_deletes)
    {
      DoTheTest(_T("test_deletes"));
    }

    TEST_METHOD(test_falsetrue)
    {
      DoTheTest(_T("test_falsetrue"));
    }

    TEST_METHOD(test_float)
    {
      DoTheTest(_T("test_float"));
    }

    TEST_METHOD(test_for_loop)
    {
      DoTheTest(_T("test_for_loop"));
    }

    TEST_METHOD(test_globals)
    {
      DoTheTest(_T("test_globals"));
    }

    TEST_METHOD(test_locals)
    {
      DoTheTest(_T("test_locals"));
    }

    TEST_METHOD(test_object)
    {
      DoTheTest(_T("test_object"));
    }

    TEST_METHOD(test_objects)
    {
      DoTheTest(_T("test_objects"));
    }

    TEST_METHOD(test_recurse)
    {
      DoTheTest(_T("test_recurse"));
    }

    TEST_METHOD(test_reference)
    {
      DoTheTest(_T("test_reference"));
    }

    TEST_METHOD(test_sizeof)
    {
      DoTheTest(_T("test_sizeof"));
    }

    TEST_METHOD(test_square)
    {
      DoTheTest(_T("test_square"));
    }

    TEST_METHOD(test_string_find)
    {
      DoTheTest(_T("test_string_find"));
    }

    TEST_METHOD(test_string_index)
    {
      DoTheTest(_T("test_string_index"));
    }

    TEST_METHOD(test_string_leftright)
    {
      DoTheTest(_T("test_string_leftright"));
    }

    TEST_METHOD(test_string_substring)
    {
      DoTheTest(_T("test_string_substring"));
    }

    TEST_METHOD(test_string_upperlower)
    {
      DoTheTest(_T("test_string_upperlower"));
    }

    TEST_METHOD(test_stringarray)
    {
      DoTheTest(_T("test_stringarray"));
    }

    TEST_METHOD(test_switch)
    {
      DoTheTest(_T("test_switch"));
    }

    TEST_METHOD(test_database)
    {
      DoTheTest(_T("test_database"));
    }

    TEST_METHOD(test_database_detail)
    {
      DoTheTest(_T("test_database_detail"));
    }

    void DoTheTest(CString p_filename)
    {
      CString message(_T("Running: "));
      message += p_filename;
      Logger::WriteMessage(message);

      _tchdir(m_basedir);

      CString result;
      CString compileOption = _T("-c ");

      CString sourceFile = m_basedir + p_filename + _T(".ql");
      CString objectFile = m_basedir + p_filename + _T(".qob");
      CString outputFile = m_basedir + p_filename + _T(".ok");
      CString qlRuntime  = m_exedir  + _T("ql.exe");

      // Compile the test: result MUST be zero (0)
      int res = CallProgram_For_String(qlRuntime,compileOption + sourceFile,result);
      Assert::AreEqual(res,0);

      // Run the program, resulting in output
      res = CallProgram_For_String(qlRuntime,objectFile,result);

      CString correct = ReadOutputFile(outputFile);

      correct.TrimRight(_T("\r\n"));
      result.TrimRight(_T("\r\n"));
      result.Replace(_T("\r"),_T(""));
      correct.Replace(_T("\r"),_T(""));

      // Program must deliver exactly the same as the output file
      Assert::AreEqual(correct.GetString(),result.GetString());
    }

    CString ReadOutputFile(CString p_filename)
    {
      CFile   file;
      CString output;
      CFileException exp;

      try
      {
        if(file.Open(p_filename,CFile::modeRead | CFile::shareDenyNone,&exp))
        {
          TCHAR buffer[1024 + 1];
          int size = 0;
        
          while((size = file.Read(buffer,1024)) > 0)
          {
            buffer[size] = 0;
            output += buffer;
          }
          file.Close();
        }
        else
        {
          PrintFileError(exp,p_filename);
        }
      }
      catch(CFileException& exp)
      {
        PrintFileError(exp,p_filename);
      }
      return output;
    }

    void PrintFileError(CFileException& exp,CString& p_filename)
    {
      TCHAR buffer[1024];
      CString message(_T("Failed reading file: "));
      message += p_filename + _T(" : ");
      exp.GetErrorMessage(buffer, 1024);
      message += buffer;

      Logger::WriteMessage(message);
      Assert::Fail();
    }

  private:
    CString m_basedir { _T("C:\\Develop\\QLLanguage\\Test\\") };
    CString m_exedir  { _T("C:\\Develop\\QLLanguage\\bin\\")  };

	};
}