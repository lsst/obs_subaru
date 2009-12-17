
%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
 
#include "libfits.h"
#include "dervish_msg_c.h"
#include "shCErrStack.h"
#include "shCUtils.h"

#include "shCSchemaTrans.h"

#if defined(_POSIX_SOURCE)
  /*#  define gettxt(FILE,MSG) (MSG)*/
#endif
   
typedef struct _junk
{
     char name[256];
}SCHTRS_SYMB;

struct _tmp_entry
{
   CONVTYPE type;
   char fits[256];
   char obj[256];
   char dtype[256];
   char ratio[256];
   char dimen[256];
   char proc[256];
   char htype[256];
   char hlen[256];
} g_entryNames;

SCHEMATRANS* g_xtbl;
unsigned ratioGood = FALSE;
unsigned dimenGood = FALSE;
unsigned procGood = FALSE;
unsigned htypeGood = FALSE;
unsigned hlenGood = FALSE;

#undef YYSTYPE
#define YYSTYPE SCHTRS_SYMB

extern void switchInit(void);
extern int yyparse(void);
extern int yylex(void);

%}


%token SCHTRS_STRING
%token SCHTRS_IGNORE
%token SCHTRS_NAME
%token SCHTRS_CONT
%token SCHTRS_PROC
%token SCHTRS_DIMEN
%token SCHTRS_RATIO
%token SCHTRS_HEAPTYPE
%token SCHTRS_HEAPLENGTH

%start entries


%%

entries:   /* empty */
       | entries entry            
       ;

entry :  type  others  ';'          
          {
              shSchemaTransEntryAdd(g_xtbl,
                g_entryNames.type,
                g_entryNames.fits,
                g_entryNames.obj,
                g_entryNames.dtype,
               (htypeGood)?g_entryNames.htype:NULL,
               (hlenGood)?g_entryNames.hlen:NULL,
               (procGood)?g_entryNames.proc:NULL,
               (dimenGood)?g_entryNames.dimen:NULL,
               (ratioGood)?atof(g_entryNames.ratio):1.0,
                -1);
              switchInit();


          }
      |  SCHTRS_IGNORE obj_name ';' 
         {
             g_entryNames.type = CONVERSION_IGNORE;
             shSchemaTransEntryAdd(g_xtbl,
                g_entryNames.type,
                "",                     /* g_entryNames.fits */
                g_entryNames.obj,
                "int",                     /* g_entryNames.dtype */
               NULL,
               NULL,
               NULL,
               NULL,
               1.0,
               -1);
              
             switchInit();
         }
      ;

type  :   SCHTRS_NAME    { g_entryNames.type = CONVERSION_BY_TYPE;}
      |   SCHTRS_CONT    { g_entryNames.type = CONVERSION_CONTINUE;}
      ;

others: name_declare | name_declare options

name_declare : fits_name  obj_name  obj_type
              ;

options:  option | options option ;


option:    SCHTRS_PROC    SCHTRS_STRING 
               {
                   strcpy(g_entryNames.proc, $2.name); 
                   procGood = TRUE;
               }
      |    SCHTRS_DIMEN    SCHTRS_STRING 
               {
                   strcpy(g_entryNames.dimen, $2.name);
                   dimenGood = TRUE;
               }
      |    SCHTRS_RATIO   SCHTRS_STRING 
               {
                   strcpy(g_entryNames.ratio, $2.name);
                   ratioGood = TRUE ;
                }
      |    SCHTRS_HEAPTYPE  SCHTRS_STRING  
               {
                   strcpy(g_entryNames.htype, $2.name);
                   htypeGood = TRUE;
               }
      |    SCHTRS_HEAPLENGTH  SCHTRS_STRING  
               {
                    strcpy(g_entryNames.hlen, $2.name);
                    hlenGood = TRUE;
               }
      ;
      
fits_name : SCHTRS_STRING  {strcpy(g_entryNames.fits, $1.name);}
          ;

obj_name : SCHTRS_STRING   {strcpy(g_entryNames.obj, $1.name);}
          ;

obj_type : SCHTRS_STRING   {strcpy(g_entryNames.dtype, $1.name);}
          ;

%%

void switchInit(void)
{
    	ratioGood = FALSE;
 	dimenGood = FALSE;
	procGood = FALSE;
	htypeGood = FALSE;
	hlenGood = FALSE;
}

#if !defined(DARWIN)
   extern int yylineno;
#endif
extern char yytext[];
extern int yyleng;
extern FILE* yyin;

void yyerror(char* s)
{
#if defined(DARWIN)
   shErrStackPush("%s at line ???\n", s);
#else
   shErrStackPush("%s at line %d\n", s, yylineno);
#endif
}


void get_name(int dequote)
{
    
    

    if(!dequote)strcpy(yylval.name, yytext);
    else 
    {
       strcpy(yylval.name, yytext+1);
       yylval.name[yyleng-2] = '\0';
    }
       

}


RET_CODE
shSchemaTransCreateFromFile(SCHEMATRANS *xtbl_ptr,
                   char* fileName)
{
    
    FILE* fp;

    fp = fopen(fileName, "r");
    if(fp == NULL)
    {
        shErrStackPush("Can't open file %s", fileName);
        return SH_GENERIC_ERROR;
    }
    yyin = fp;
    g_xtbl = xtbl_ptr;

    if(yyparse()!=0) 
    {
       shErrStackPush("Parser failed");
       return SH_GENERIC_ERROR;
    }
    return SH_SUCCESS;
}
