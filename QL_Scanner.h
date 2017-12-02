//////////////////////////////////////////////////////////////////////////
//
// QL Language scanner
// ir W.E. Huisman (c) 2017
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
#define T_LOCAL       263
#define T_IF		      264
#define T_ELSE		    265
#define T_WHILE		    266
#define T_RETURN	    267
#define T_FOR		      268
#define T_BREAK		    269
#define T_CONTINUE	  270
#define T_DO		      271
#define T_SWITCH      272
#define T_CASE        273
#define T_DEFAULT     274
#define T_NEW		      275
#define T_NIL		      276
#define T_LE		      277	/* '<=' */
#define T_EQ		      278	/* '==' */
#define T_NE		      279	/* '!=' */
#define T_GE		      280	/* '>=' */
#define T_SHL		      281	/* '<<' */
#define T_SHR		      282	/* '>>' */
#define T_AND		      283	/* '&&' */
#define T_OR		      284	/* '||' */
#define T_INC		      285	/* '++' */
#define T_DEC		      286	/* '--' */
#define T_ADDEQ		    287	/* '+=' */
#define T_SUBEQ		    288	/* '-=' */
#define T_MULEQ		    289	/* '*=' */
#define T_DIVEQ		    290	/* '/=' */
#define T_REMEQ		    291	/* '%=' */
#define T_ANDEQ		    292	/* '&=' */
#define T_OREQ		    293	/* '|=' */
#define T_XOREQ		    294	/* '^=' */
#define T_SHLEQ		    295	/* '<<=' */
#define T_SHREQ		    296	/* '>>=' */
#define T_CC		      297	/* '::' */
#define T_MEMREF	    298	/* '->' */
#define _TMAX		      298

typedef struct _keyword_table
{
  char *kt_keyword;
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
  void    ParseError(const char* msg);


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
