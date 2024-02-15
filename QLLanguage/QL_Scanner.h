//////////////////////////////////////////////////////////////////////////
//
// QL Language scanner
// ir W.E. Huisman (c) 2018
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "QL_Language.h"

/* token definitions */
#define T_NOTOKEN	 -1
#define T_EOF		    0

/* non-character tokens */
#define _TMIN		      256
#define T_STRING	    256
#define T_IDENTIFIER	257
#define T_NUMBER	    258
#define T_FLOAT       259
#define T_CLASS		    260
#define T_STATIC	    261
#define T_GLOBAL      262
#define T_IF		      263
#define T_ELSE		    264
#define T_WHILE		    265
#define T_RETURN	    266
#define T_FOR		      267
#define T_BREAK		    268
#define T_CONTINUE	  269
#define T_DO		      270
#define T_SWITCH      271
#define T_CASE        272
#define T_DEFAULT     273
#define T_NEW		      274
#define T_DELETE      275
#define T_NIL		      276
#define T_FALSE       277
#define T_TRUE        278
#define T_LE		      279	/* '<=' */
#define T_EQ		      280	/* '==' */
#define T_NE		      281	/* '!=' */
#define T_GE		      282	/* '>=' */
#define T_SHL		      283	/* '<<' */
#define T_SHR		      284	/* '>>' */
#define T_AND		      285	/* '&&' */
#define T_OR		      286	/* '||' */
#define T_INC		      287	/* '++' */
#define T_DEC		      288	/* '--' */
#define T_ADDEQ		    289	/* '+=' */
#define T_SUBEQ		    290	/* '-=' */
#define T_MULEQ		    291	/* '*=' */
#define T_DIVEQ		    292	/* '/=' */
#define T_REMEQ		    293	/* '%=' */
#define T_ANDEQ		    294	/* '&=' */
#define T_OREQ		    295	/* '|=' */
#define T_XOREQ		    296	/* '^=' */
#define T_SHLEQ		    297	/* '<<=' */
#define T_SHREQ		    298	/* '>>=' */
#define T_CC		      299	/* '::' */
#define T_MEMREF	    300	/* '->' */
#define _TMAX		      300

typedef struct _keyword_table
{
  TCHAR *kt_keyword;
  int   kt_token;
}
KeywordTable;

class QLScanner 
{
public:
  QLScanner(int(*gf)(void*), void* data);
 ~QLScanner();

  int     GetToken();                 // get token
  CString GetTokenAsString();
  int     GetTokenAsInteger();
  bcd     GetTokenAsFloat();
    
  int     SaveToken(int p_token);     // Saved token
  CString TokenName(int tkn);         // get token name
  void    ParseError(const TCHAR* msg);


private:
  int     ReadToken();
  int     GetString();
  int     GetCharacter();
  int     LiteralCharacter();
  int     GetID(int ch);
  int     GetNumber(int ch);
  int     SkipSpaces();
  int     IsIDcharacter(int ch);
  int     getch();

  // DATA

  int   (*m_getc_function)(void*);  // getc function
  void*   m_getc_data;	            // getc data-pointer
  int     m_save_token;	            // look ahead token
  int     m_save_char;	            // look ahead character
  int     m_last_char;	            // last input character 
  int     m_line_number;	          // line number, lines read
  CString m_line;                   // Next input line
  int     m_line_pos;               // Working on this position

  // Current scan status
  int     m_tokenAsInteger;	        // Next token as numeric value
  bcd     m_tokenAsFloat;           // Next token as floating point
  CString m_tokenAsString;          // Next token as a string
};

// save token
inline int
QLScanner::SaveToken(int p_token)
{
  return (m_save_token = p_token);
}

inline CString
QLScanner::GetTokenAsString()
{
  return m_tokenAsString;
}

inline int
QLScanner::GetTokenAsInteger()
{
  return m_tokenAsInteger;
}

inline bcd
QLScanner::GetTokenAsFloat()
{
  return m_tokenAsFloat;
}
