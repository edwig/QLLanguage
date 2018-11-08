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
      DoTheTest("test_add");
		}

    TEST_METHOD(test_array)
    {
      DoTheTest("test_array");
    }

    TEST_METHOD(test_basic)
    {
      DoTheTest("test_basic");
    }

    TEST_METHOD(test_call)
    {
      DoTheTest("test_call");
    }

    TEST_METHOD(test_constructor_2)
    {
      DoTheTest("test_constructor_2");
    }

    TEST_METHOD(test_deletes)
    {
      DoTheTest("test_deletes");
    }

    TEST_METHOD(test_falsetrue)
    {
      DoTheTest("test_falsetrue");
    }

    TEST_METHOD(test_float)
    {
      DoTheTest("test_float");
    }

    TEST_METHOD(test_for_loop)
    {
      DoTheTest("test_for_loop");
    }

    TEST_METHOD(test_globals)
    {
      DoTheTest("test_globals");
    }

    TEST_METHOD(test_locals)
    {
      DoTheTest("test_locals");
    }

    TEST_METHOD(test_object)
    {
      DoTheTest("test_object");
    }

    TEST_METHOD(test_objects)
    {
      DoTheTest("test_objects");
    }

    TEST_METHOD(test_recurse)
    {
      DoTheTest("test_recurse");
    }

    TEST_METHOD(test_reference)
    {
      DoTheTest("test_reference");
    }

    TEST_METHOD(test_sizeof)
    {
      DoTheTest("test_sizeof");
    }

    TEST_METHOD(test_square)
    {
      DoTheTest("test_square");
    }

    TEST_METHOD(test_string_find)
    {
      DoTheTest("test_string_find");
    }

    TEST_METHOD(test_string_index)
    {
      DoTheTest("test_string_index");
    }

    TEST_METHOD(test_string_leftright)
    {
      DoTheTest("test_string_leftright");
    }

    TEST_METHOD(test_string_substring)
    {
      DoTheTest("test_string_substring");
    }

    TEST_METHOD(test_string_upperlower)
    {
      DoTheTest("test_string_upperlower");
    }

    TEST_METHOD(test_stringarray)
    {
      DoTheTest("test_stringarray");
    }

    TEST_METHOD(test_switch)
    {
      DoTheTest("test_switch");
    }

    TEST_METHOD(test_database)
    {
      DoTheTest("test_database");
    }

    TEST_METHOD(test_database_detail)
    {
      DoTheTest("test_database_detail");
    }

    void DoTheTest(CString p_filename)
    {
      CString message("Running: ");
      message += p_filename;
      Logger::WriteMessage(message);

      _chdir(m_basedir);

      CString result;
      CString compileOption = "-c ";

      CString sourceFile = m_basedir + p_filename + ".ql";
      CString objectFile = m_basedir + p_filename + ".qob";
      CString outputFile = m_basedir + p_filename + ".ok";
      CString qlRuntime  = m_exedir  + "ql.exe";

      // Compile the test: result MUST be zero (0)
      int res = CallProgram_For_String(qlRuntime,compileOption + sourceFile,result);
      Assert::AreEqual(res,0);

      // Run the program, resulting in output
      res = CallProgram_For_String(qlRuntime,objectFile,result);

      CString correct = ReadOutputFile(outputFile);

      correct.TrimRight("\r\n");
      result.TrimRight("\r\n");
      result.Replace("\r","");
      correct.Replace("\r","");

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
          char buffer[1024 + 1];
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
      char buffer[1024];
      CString message("Failed reading file: ");
      message += p_filename + " : ";
      exp.GetErrorMessage(buffer, 1024);
      message += buffer;

      Logger::WriteMessage(message);
      Assert::Fail();
    }

  private:
    CString m_basedir { "C:\\Develop\\QLLanguage\\Test\\" };
    CString m_exedir  { "C:\\Develop\\QLLanguage\\bin\\"  };

	};
}