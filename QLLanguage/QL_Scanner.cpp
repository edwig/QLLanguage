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
  { "class",    T_CLASS     },
  { "static",   T_STATIC    },
  { "global",   T_GLOBAL    },
  { "if",       T_IF        },
  { "else",     T_ELSE      },
  { "while",    T_WHILE     },
  { "return",   T_RETURN    },
  { "for",      T_FOR       },
  { "break",    T_BREAK     },
  { "continue", T_CONTINUE  },
  { "do",       T_DO        },
  { "switch",   T_SWITCH    },
  { "case",     T_CASE      },
  { "default",  T_DEFAULT   },
  { "new",      T_NEW       },
  { "delete",   T_DELETE    },
  { "nil",      T_NIL       },
  { "false",    T_FALSE     },
  { "true",     T_TRUE      },
  { NULL,       0           }
};

// token name table 
// Beware: must be in the order of the T_* macro names
static char *tokenNames[] =
{
  "<string>",
  "<identifier>",
  "<number>",
  "class",
  "static",
  "global",
  "local",
  "if",
  "else",
  "while",
  "return",
  "for",
  "break",
  "continue",
  "do",
  "switch",
  "case",
  "default",
  "new",
  "delete",
  "nil",
  "false",
  "true",
  "<=",
  "==",
  "!=",
  ">=",
  "<<",
  ">>",
  "&&",
  "||",
  "++",
  "--",
  "+=",
  "-=",
  "*=",
  "/=",
  "%=",
  "&=",
  "|=",
  "^=",
  "<<=",
  ">>=",
  "::",
  "->"
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
  m_save_char  = '\0';

  /* no last character */
  m_last_char = '\0';
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
    return ("<eof>");
  }
  else if (tkn >= _TMIN && tkn <= _TMAX)
  {
    return (tokenNames[tkn - _TMIN]);
  }
  // Give back as a string
  CString tokenAsChar(" ");
  tokenAsChar.SetAt(0,(unsigned char) tkn);

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
    case EOF:	  return (T_EOF);
    case '"':	  return (GetString());
    case '\'':	return (GetCharacter());
    case '<':	  switch (ch = getch())
                {
                  case '=': return (T_LE);
                  case '<': if ((ch = getch()) == '=')
                            {
                              return (T_SHLEQ);
                            }
                            m_save_char = ch;
                            return (T_SHL);
                  default:	m_save_char = ch;
                            return ('<');
                }
                break;
    case '=':	  if ((ch = getch()) == '=')
                {
                  return (T_EQ);
                }
                m_save_char = ch;
                return ('=');
    case '!':	  if ((ch = getch()) == '=')
                {
                  return (T_NE);
                }
                m_save_char = ch;
                return ('!');
    case '>':	  switch (ch = getch())
                {
                  case '=': return (T_GE);
                  case '>': if ((ch = getch()) == '=')
                            {
                              return (T_SHREQ);
                            }
                            m_save_char = ch;
                            return (T_SHR);
                  default:	m_save_char = ch;
                            return ('>');
                }
                break;
    case '&':	  switch (ch = getch())
                {
                  case '&': return (T_AND);
                  case '=': return (T_ANDEQ);
                  default:  m_save_char = ch;
                            return ('&');
                }
                break;
    case '|':	  switch (ch = getch())
                {
                  case '|': return (T_AND);
                  case '=': return (T_OREQ);
                  default:  m_save_char = ch;
                            return ('|');
                }
                break;
    case '^':	  if ((ch = getch()) == '=')
                {
                  return (T_XOREQ);
                }
                m_save_char = ch;
                return ('^');
    case '+':	  switch (ch = getch())
                {
                  case '+': return (T_INC);
                  case '=': return (T_ADDEQ);
                  default:  m_save_char = ch;
                            return ('+');
                }
                break;
    case '-':	  switch (ch = getch())
                {
                  case '-': return (T_DEC);
                  case '=': return (T_SUBEQ);
                  case '>': return (T_MEMREF);
                  default:  m_save_char = ch;
                            return ('-');
                }
                break;
    case '.':   return T_MEMREF;
    case '*':	  if ((ch = getch()) == '=')
                {
                  return (T_MULEQ);
                }
                m_save_char = ch;
                return ('*');
    case '/':	  switch (ch = getch())
                {
                  case '=': return (T_DIVEQ);
                  case '/': while ((ch = getch()) != EOF)
                            {
                              if (ch == '\n')
                              {
                                break;
                              }
                            }
                            break;
                  case '*': ch = ch2 = EOF;
                            for (; (ch2 = getch()) != EOF; ch = ch2)
                            {
                              if (ch == '*' && ch2 == '/')
                              {
                                break;
                              }
                            }
                            break;
                  default: 	m_save_char = ch;
                            return ('/');
                }
                break;
    case ':':	  if ((ch = getch()) == ':')
                {
                  return (T_CC);
                }
                m_save_char = ch;
                return (':');
    default:	  if (isdigit(ch))
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
                  m_tokenAsString = (const char) ch;
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
    m_tokenAsString += (const char) ch;
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
  m_tokenAsString = (const char) m_tokenAsInteger;
  if (getch() != '\'')
  {
    ParseError("Expecting a closing single quote");
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
      case 'n':  ch = '\n'; break;
      case 't':  ch = '\t'; break;
      case 'r':  ch = '\r'; break;
      case 'f':  ch = '\f'; break;
      case '\\': ch = '\\'; break;
      case EOF:  ch = '\\'; m_save_char = EOF; break;
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
  m_tokenAsString = (const char) ch;
  while ((ch = getch()) != EOF && IsIDcharacter(ch))
  {
    m_tokenAsString += (const char) ch;
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
  m_tokenAsString = (const char) ch;
  m_tokenAsInteger = ch - '0';
  while ((ch = getch()) != EOF && isdigit(ch))
  {
    m_tokenAsInteger = m_tokenAsInteger * 10 + ch - '0';
    m_tokenAsString += (const char) ch;
  }
  if (ch == '.')
  {
    type = T_FLOAT;

    m_tokenAsFloat = (const long)m_tokenAsInteger;
    bcd temp((long)1);

    // Decimal part
    while ((ch = getch()) != EOF && isdigit(ch))
    {
      temp *= (ch - '0');
      m_tokenAsString += (const char) ch;
      temp /= 10;
    }
    m_tokenAsFloat += temp;

    // Exponent part
    if (tolower(ch) == 'e')
    {
      int exp = 0;
      while ((ch = getch()) != EOF && isdigit(ch))
      {
        exp = exp * 10 + ch - '0';
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
  while ((ch = getch()) != '\0' && isspace(ch));
  return (ch);
}

// is this an identifier character
int QLScanner::IsIDcharacter(int ch)
{
  return (isupper(ch)
       || islower(ch)
       || isdigit(ch)
       || ch == '_');
}

// get the next character 
int QLScanner::getch()
{
  int ch;

  // check for a lookahead character 
  if ((ch = m_save_char) != '\0')
  {
    m_save_char = '\0';
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
        m_line += (const char) m_last_char;
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
void QLScanner::ParseError(const char* msg)
{
  int ch;
  CString buffer;
  CString pointer;

  // redisplay the line with the error
  buffer.Format(">>> %s <<<\n>>> in line %d <<<\n%s", msg, m_line_number, m_line);
  osputs_stderr(buffer);

  // point to the position immediately following the error 
  for(int ind = 0;ind < m_line.GetLength(); ++ind)
  {
    ch = m_line.GetAt(ind);
    pointer += (ind == '\t' ? '\t' : ' ');
  }
  // Add the pointer
  pointer += "^\n";
  osputs_stderr(pointer);

  // invoke the error trap
  throw 1;
}
