%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
 
#include "libfits.h"
#include "dervish_msg_c.h"
#include "shCErrStack.h"
#include "shCUtils.h"

#include "prvt/shCSchemaTransLexToken.h"
#include "shCSchemaTrans.h"


extern void get_name(int);
extern int yyprevious;

#undef yylook
#undef yywrap

extern int yylook(void);
extern int yywrap(void);

%}

comment   #.*\n
proc      -proc  
dimen     -dimen
ratio     -ratio
heaptype  -heaptype
heaplength -heaplength
qstring     \"[^\"\n]*\"
string      [a-zA-Z0-9_{}]+[^ ;\t\n]*
space       [ \t]
nl          \n


%%

{comment}      ;

^[ \t]*name    {get_name(0); return SCHTRS_NAME;}
^[ \t]*cont    {get_name(0); return SCHTRS_CONT;}
^[ \t]*ignore  {get_name(0); return SCHTRS_IGNORE;}
{proc}         {get_name(0); return SCHTRS_PROC;}
{dimen}        {get_name(0); return SCHTRS_DIMEN;}
{ratio}        {get_name(0); return SCHTRS_RATIO;}
{heaptype}     {get_name(0); return SCHTRS_HEAPTYPE;}
{heaplength}   {get_name(0); return SCHTRS_HEAPLENGTH;}
{string}       {get_name(0); return SCHTRS_STRING;} 
{qstring}      {get_name(1); return SCHTRS_STRING;}
{space}        ;
{nl}           ;
.              {return yytext[0];}

%%


yywrap()
{
        return(1);
}


