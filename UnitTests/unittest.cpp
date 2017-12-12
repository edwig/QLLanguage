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
        char buffer[1024];
        CString message("Failed reading file: ");
        message += p_filename + " : ";
        exp.GetErrorMessage(buffer,1024);
        message += buffer;

        Logger::WriteMessage(message);
        Assert::Fail();
      }
      return output;
    }


  private:
    CString m_basedir { "C:\\Develop\\QLLanguage\\Test\\" };
    CString m_exedir  { "C:\\Develop\\QLLanguage\\bin\\"  };

	};
}