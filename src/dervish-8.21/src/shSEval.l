%{
#include <stdio.h>
#include <stdlib.h>
#include "prvt/seCode.h"
#include <dervish.h>
#include "prvt/shSEvalGrammar.h"
/*inline int token(int x) {printf("token: %s: %d\n", yytext, x); return x;}*/
/* inline int token(int x) {return x;} */
#define token(x) x

#ifdef FLEX_SCANNER
/* define YY_INPUT for flex */
#define YY_INPUT(buf, result, max_size)\
	{\
		int c;\
		c = fs_getc();\
		result = (c == EOF) ? YY_NULL : (buf[0] = c, 1);\
	}
#else
#undef input
# define input() (((yytchar=yysptr>yysbuf?U(*--yysptr):fs_getc())==10?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)
#endif

#define sprint(x) fputs(x, yyout)
#define allprint(x) fputc(x, yyout)
#define yywrap() 1

/* for the input buffer */

extern char *lex_input;

/* To silence picky compiler */
int yylook (void);
static int fs_getc(void);
%}

arrow                   \-\>
handle                  h[0-9]+
alpha			[a-zA-Z_]
alpha_numerical		[a-zA-Z_0-9]
exponent		[eE][-+]?[0-9]+
unsigned_int		[0-9]+
string			\"([^"\n]|\\["\n])*\"
symbol			[\(\),\.\+\-\<\>\{\}\*\/\@\:\=\;\?\&\|\^\!]
white_space		[ \t\n\r]

%%

"int"				return token(TT_INT);
"unsigned"			return token(TT_UNSIGNED);
"float"				return token(TT_FLOAT);
"double"			return token(TT_DOUBLE);
"long"				return token(TT_LONG);
"short"				return token(TT_SHORT);
"char"				return token(TT_CHAR);
"string"			return token(TT_STRING);
"new"			        return token(NEW);
"sin"				return token(F_SIN);
"cos"				return token(F_COS);
"tan"				return token(F_TAN);
"asin"				return token(F_ASIN);
"acos"				return token(F_ACOS);
"atan"				return token(F_ATAN);
"exp"				return token(F_EXP);
"log"				return token(F_LOG);
"pow"				return token(F_POW);
"abs"				return token(F_ABS);
"atan2"				return token(F_ATAN2);
"log10"				return token(F_LOG10);
"sqrt"				return token(F_SQRT);
"min"				return token(F_MIN);
"max"				return token(F_MAX);
"$$"				return token(F_LC);
"strcpy"			return token(F_STRCPY);
"strncpy"			return token(F_STRNCPY);
"strlen"			return token(F_STRLEN);
"strcat"			return token(F_STRCAT);
"strncat"			return token(F_STRNCAT);
"strcmp"			return token(F_STRCMP);
"strncmp"			return token(F_STRNCMP);
"strcasecmp"			return token(F_STRCASECMP);
"strncasecmp"			return token(F_STRNCASECMP);
"stroffset"			return token(F_STROFFSET);
"and"				return token(L_AND);
"&&"				return token(L_AND);
"or"				return token(L_OR);
"||"				return token(L_OR);
"not"				return token(L_NOT);
"if"				return token(IF);
"then"				return token(THEN);
"else"				return token(ELSE);
"endif"				return token(ENDIF);
".EQ."				return token(C_EQ);
"=="				return token(C_EQ);
".GT."				return token(C_GT);
" > "				return token(C_GT);
"\t>"				return token(C_GT);
".GE."				return token(C_GE);
">="				return token(C_GE);
"=>"				return token(C_GE);
".LT."				return token(C_LT);
" < "				return token(C_LT);
"\t<"				return token(C_LT);
".LE."				return token(C_LE);
"<="				return token(C_LE);
"=<"				return token(C_LE);
".NE."				return token(C_NE);
"<>"				return token(C_NE);
"!="				return token(C_NE);
" << "				return token(B_SL);
" >> "				return token(B_SR);
"break"				return token(P_BREAK);
"continue"			return token(P_CONTINUE);
{arrow}  			return token(ARROW);
{handle}   {
             HANDLE han;
             if (shTclHandleGetFromName(g_seInterp, (char*)yytext, &han) == TCL_OK)
               {
                  if (han.type == shTypeGetFromName("CHAIN"))
                    {
                      p_shSeSymCopyHandle(T_CHAIN, han);
                      return token(CHAIN_HANDLE);
                    }
                  else if (han.type == shTypeGetFromName("VECTOR"))
                    {
                      p_shSeSymCopyHandle(T_VECTOR, han);
                      return token(VECTOR_HANDLE);
                    }
                  else if (han.type == shTypeGetFromName("TBLCOL"))
                    {
                      p_shSeSymCopyHandle(T_TBLCOL, han);
                      return token(TBLCOL_HANDLE);
                    }
                  else if ((*seFindClassFromName)(shNameGetFromType(han.type)) !=
                          NULL)
                    {
                      p_shSeSymCopyHandle(T_OBJECT, han);
                      return token(OBJECT_HANDLE);
                    }
                  else
                    {
                      p_shSeSymCopyHandle(T_SCHEMA, han);
                      return token(SCHEMA_HANDLE);
                    }
               }
             else
               {
                  p_shSeSymCopyIdentifier(yytext);
                  return token(IDENTIFIER);
               }
           }
{symbol}			return token(yytext[0]);
{alpha}{alpha_numerical}*	{
                                   p_shSeSymCopyIdentifier(yytext);
                                   return token(IDENTIFIER);
                                }
{unsigned_int}\.{unsigned_int}?({exponent})? {
                                               p_shSeSymCopyDouble(atof((char*)yytext));
                                               return token(FLOAT);
                                             }
\.{unsigned_int}?({exponent})?	{
                                   p_shSeSymCopyDouble(atof((char*)yytext));
                                   return token(FLOAT);
                                }
{unsigned_int}			{
                                   p_shSeSymCopyLong(atol((char*)yytext));
                                   return token(INTEGER);
                                }
{string}			{
                                   p_shSeSymCopyString(yytext);
                                   return token(STRING);
                                }
{white_space}			;
.				return token(yytext[0]);
%%

/* fs_getc() -- get a character from buffer pointed by *lex_input
 *
 * To get away from predefined input() in lex
 */

static int fs_getc(void)
{
	int c;
/*
	c = (int) *lex_input++;
*/
	c = (int) *lex_input;
	if (c) lex_input++;

	return((c?c:EOF));
}

/* reset_yy_lex() -- reset input buffer */

void reset_yy_lex(void)
{
#ifdef FLEX_SCANNER
	yy_flush_buffer(yy_current_buffer);
	yy_init = 1;
#else
	yysptr = yysbuf;
#endif
}

/*
yywrap()
{
	return(1);
}
*/
/* redefinition of part of lex debugging routines to remove the
 * dependency on lex library, so that the yy/YY renaming may work!
 */

/*
void sprint(char *s)
{
	fputs(s, yyout);
}

void allprint(char c)
{
	fputc(c, yyout);
}
*/
