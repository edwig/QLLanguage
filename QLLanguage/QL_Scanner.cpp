//////////////////////////////////////////////////////////////////////////
//
// QL Language scanner
// ir W.E. Huisman (c) 2018
//
//////////////////////////////////////////////////////////////////////////

#include "Stdafx.h"
#include "QL_Scanner.h"
#include "bcd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// keyword table 
static KeywordTable keywordTable[] =
{
  { _T("class"),    T_CLASS     },
  { _T("static"),   T_STATIC    },
  { _T("global"),   T_GLOBAL    },
  { _T("if"),       T_IF        },
  { _T("else"),     T_ELSE      },
  { _T("while"),    T_WHILE     },
  { _T("return"),   T_RETURN    },
  { _T("for"),      T_FOR       },
  { _T("break"),    T_BREAK     },
  { _T("continue"), T_CONTINUE  },
  { _T("do"),       T_DO        },
  { _T("switch"),   T_SWITCH    },
  { _T("case"),     T_CASE      },
  { _T("default"),  T_DEFAULT   },
  { _T("new"),      T_NEW       },
  { _T("delete"),   T_DELETE    },
  { _T("nil"),      T_NIL       },
  { _T("false"),    T_FALSE     },
  { _T("true"),     T_TRUE      },
  { NULL,       0           }
};

// token name table 
// Beware: must be in the order of the T_* macro names
static TCHAR *tokenNames[] =
{
  _T("<string>"),
  _T("<identifier>"),
  _T("<number>"),
  _T("class"),
  _T("static"),
  _T("global"),
  _T("local"),
  _T("if"),
  _T("else"),
  _T("while"),
  _T("return"),
  _T("for"),
  _T("break"),
  _T("continue"),
  _T("do"),
  _T("switch"),
  _T("case"),
  _T("default"),
  _T("new"),
  _T("delete"),
  _T("nil"),
  _T("false"),
  _T("true"),
  _T("<="),
  _T("=="),
  _T("!="),
  _T(">="),
  _T("<<"),
  _T(">>"),
  _T("&&"),
  _T("||"),
  _T("++"),
  _T("--"),
  _T("+="),
  _T("-="),
  _T("*="),
  _T("/="),
  _T("%="),
  _T("&="),
  _T("|="),
  _T("^="),
  _T("<<="),
  _T(">>="),
  _T("::"),
  _T("->")
};

QLScanner::QLScanner(int(*gf)(void *), void* data)
{
  m_getc_function = gf;
  m_getc_data     = data;

  /* setup the line buffer */
  m_line.Empty();
  m_line_pos    = 0;
  m_line_number = 0;

  /* no lookahead yet */
  m_save_token = T_NOTOKEN;
  m_save_char  = _T('\0');

  /* no last character */
  m_last_char = _T('\0');
}

QLScanner::~QLScanner()
{

}

// get the next token
int QLScanner::GetToken()
{
  int tkn;

  if ((tkn = m_save_token) != T_NOTOKEN)
  {
    m_save_token = T_NOTOKEN;
  }
  else
  {
    tkn = ReadToken();
  }
  return (tkn);
}

// get the name of a token
CString QLScanner::TokenName(int tkn)
{
  if (tkn == T_EOF)
  {
    return (_T("<eof>"));
  }
  else if (tkn >= _TMIN && tkn <= _TMAX)
  {
    return (tokenNames[tkn - _TMIN]);
  }
  // Give back as a string
  CString tokenAsChar(_T(" "));
  tokenAsChar.SetAt(0,(_TUCHAR) tkn);

  return tokenAsChar;
}

// read the next token
int QLScanner::ReadToken()
{
  int ch, ch2;

  // check the next character 
  for (;;)
  {
    switch (ch = SkipSpaces())
    {
    case EOF:	      return (T_EOF);
    case _T('"'):	  return (GetString());
    case _T('\''):	return (GetCharacter());
    case _T('<'):	  switch (ch = getch())
                    {
                      case _T('='): return (T_LE);
                      case _T('<'): if ((ch = getch()) == '=')
                                {
                                  return (T_SHLEQ);
                                }
                                m_save_char = ch;
                                return (T_SHL);
                      default:	m_save_char = ch;
                                return ('<');
                    }
                    break;
    case _T('='):	  if ((ch = getch()) == '=')
                    {
                      return (T_EQ);
                    }
                    m_save_char = ch;
                    return ('=');
    case _T('!'):	  if ((ch = getch()) == '=')
                    {
                      return (T_NE);
                    }
                    m_save_char = ch;
                    return ('!');
    case _T('>'):	  switch (ch = getch())
                    {
                      case _T('='): return (T_GE);
                      case _T('>'): if ((ch = getch()) == '=')
                                {
                                  return (T_SHREQ);
                                }
                                m_save_char = ch;
                                return (T_SHR);
                      default:	m_save_char = ch;
                                return ('>');
                    }
                    break;
    case _T('&'):	  switch (ch = getch())
                    {
                      case _T('&'): return (T_AND);
                      case _T('='): return (T_ANDEQ);
                      default:  m_save_char = ch;
                                return ('&');
                    }
                    break;
    case _T('|'):	  switch (ch = getch())
                    {
                      case _T('|'): return (T_AND);
                      case _T('='): return (T_OREQ);
                      default:  m_save_char = ch;
                                return ('|');
                    }
                    break;
    case _T('^'):	  if ((ch = getch()) == '=')
                    {
                      return (T_XOREQ);
                    }
                    m_save_char = ch;
                    return ('^');
    case _T('+'):	  switch (ch = getch())
                    {
                      case _T('+'): return (T_INC);
                      case _T('='): return (T_ADDEQ);
                      default:  m_save_char = ch;
                                return ('+');
                    }
                    break;
    case _T('-'):	  switch (ch = getch())
                    {
                      case _T('-'): return (T_DEC);
                      case _T('='): return (T_SUBEQ);
                      case _T('>'): return (T_MEMREF);
                      default:  m_save_char = ch;
                                return ('-');
                    }
                    break;
    case _T('.'):   return T_MEMREF;
    case _T('*'):	  if ((ch = getch()) == '=')
                    {
                      return (T_MULEQ);
                    }
                    m_save_char = ch;
                    return ('*');
    case _T('/'):	  switch (ch = getch())
                    {
                      case _T('='): return (T_DIVEQ);
                      case _T('/'): while ((ch = getch()) != _TEOF)
                                {
                                  if (ch == '\n')
                                  {
                                    break;
                                  }
                                }
                                break;
                      case _T('*'): ch = ch2 = _TEOF;
                                for (; (ch2 = getch()) != _TEOF; ch = ch2)
                                {
                                  if (ch == _T('*') && ch2 == '/')
                                  {
                                    break;
                                  }
                                }
                                break;
                      default: 	m_save_char = ch;
                                return ('/');
                    }
                    break;
    case _T(':'):	  if ((ch = getch()) == ':')
                    {
                      return (T_CC);
                    }
                    m_save_char = ch;
                    return (':');
    default:	      if(_istdigit(ch))
                    {
                      return (GetNumber(ch));
                    }
                    else if (IsIDcharacter(ch))
                    {
                      return (GetID(ch));
                    }
                    else
                    {
                      m_tokenAsString.Empty();
                      m_tokenAsString = (const TCHAR) ch;
                      return (ch);
                    }
                    break;
    }
  }
}

// get a string
int QLScanner::GetString()
{
  int ch;
  int size = 0;

  m_tokenAsString.Empty();
  while ((ch = LiteralCharacter()) != EOF && ch != '"')
  {
    m_tokenAsString += (const TCHAR) ch;
  }
  if (ch == EOF)
  {
    m_save_char = EOF;
  }
  return (T_STRING);
}

// get a character constant
int QLScanner::GetCharacter()
{
  m_tokenAsString.Empty();
  m_tokenAsInteger = LiteralCharacter();
  m_tokenAsString = (const TCHAR) m_tokenAsInteger;
  if (getch() != '\'')
  {
    ParseError(_T("Expecting a closing single quote"));
  }
  return (T_NUMBER);
}

// get a character from a literal string
int QLScanner::LiteralCharacter()
{
  int ch;

  if ((ch = getch()) == '\\')
  {
    switch (ch = getch())
    {
      case _T('n'):  ch = _T('\n'); break;
      case _T('t'):  ch = _T('\t'); break;
      case _T('r'):  ch = _T('\r'); break;
      case _T('f'):  ch = _T('\f'); break;
      case _T('\\'): ch = _T('\\'); break;
      case _TEOF:  ch = _T('\\'); m_save_char = _TEOF; break;
    }
  }
  return (ch);
}

// get an identifier
int QLScanner::GetID(int ch)
{
  int i;

  /* get the identifier */
  m_tokenAsString.Empty();
  m_tokenAsString = (const TCHAR) ch;
  while ((ch = getch()) != _TEOF && IsIDcharacter(ch))
  {
    m_tokenAsString += (const TCHAR) ch;
  }
  m_save_char = ch;

  /* check to see if it is a keyword */
  for (i = 0; keywordTable[i].kt_keyword != NULL; ++i)
  {
    if(m_tokenAsString.Compare(keywordTable[i].kt_keyword) == 0)
    {
      return (keywordTable[i].kt_token);
    }
  }
  return (T_IDENTIFIER);
}

// get a number 
int QLScanner::GetNumber(int ch)
{
  int type = T_NUMBER;

  // get the number 
  m_tokenAsString.Empty();
  m_tokenAsString = (const TCHAR) ch;
  m_tokenAsInteger = ch - _T('0');
  while ((ch = getch()) != _TEOF && _istdigit(ch))
  {
    m_tokenAsInteger = m_tokenAsInteger * 10 + ch - _T('0');
    m_tokenAsString += (const TCHAR) ch;
  }
  if (ch == '.')
  {
    type = T_FLOAT;

    m_tokenAsFloat = (const long)m_tokenAsInteger;
    bcd temp((long)1);

    // Decimal part
    while ((ch = getch()) != _TEOF && _istdigit(ch))
    {
      temp *= (ch - '0');
      m_tokenAsString += (const TCHAR) ch;
      temp /= 10;
    }
    m_tokenAsFloat += temp;

    // Exponent part
    if (_totlower(ch) == 'e')
    {
      int exp = 0;
      while ((ch = getch()) != _TEOF && _istdigit(ch))
      {
        exp = exp * 10 + ch - _T('0');
      }
      m_tokenAsFloat.TenPower(exp);
    }
  }

  m_save_char = ch;
  return type;
}

// skip leading spaces
int QLScanner::SkipSpaces()
{
  int ch;
  while ((ch = getch()) != _T('\0') && _istspace(ch));
  return (ch);
}

// is this an identifier character
int QLScanner::IsIDcharacter(int ch)
{
  return (_istupper(ch)
       || _istlower(ch)
       || _istdigit(ch)
       || ch == '_');
}

// get the next character 
int QLScanner::getch()
{
  int ch;

  // check for a lookahead character 
  if ((ch = m_save_char) != '\0')
  {
    m_save_char = _T('\0');
  }
  // check for a buffered character
  else
  {
    while ((ch = m_line.GetAt(m_line_pos++)) == '\0')
    {
      /* check for being at the end of file */
      if (m_last_char == EOF)
      {
        return (EOF);
      }

      /* read the next line */
      m_line.Empty();
      m_line_pos = 0;
      while ((m_last_char = (*m_getc_function)(m_getc_data)) != EOF)
      {
        m_line += (const TCHAR) m_last_char;
        if(m_last_char == '\n')
        {
          break;
        }
      }
      ++m_line_number;
    }
  }
  // return the current character 
  return (ch);
}

// report an error in the current line
void QLScanner::ParseError(const TCHAR* msg)
{
  int ch;
  CString buffer;
  CString pointer;

  // redisplay the line with the error
  buffer.Format(_T(">>> %s <<<\n>>> in line %d <<<\n%s"), msg, m_line_number, m_line);
  osputs_stderr(buffer);

  // point to the position immediately following the error 
  for(int ind = 0;ind < m_line.GetLength(); ++ind)
  {
    ch = m_line.GetAt(ind);
    pointer += (ind == _T('\t') ? _T('\t') : _T(' '));
  }
  // Add the pointer
  pointer += _T("^\n");
  osputs_stderr(pointer);

  // invoke the error trap
  throw 1;
}
