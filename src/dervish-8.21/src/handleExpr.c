/*****************************************************************************
******************************************************************************
**<AUTO>
** FILE: handleExpr.c
**<HTML>
 * This is code to parse handle expressions.
 * Handles are pointers to complex objects like structures, etc.  Many
 * times one wants to access elements of those structures at the TCL
 * level.  This routine takes as input a C-style handle expression that
 * refers to some element in a structure and returns a handle to that element.
 * All structures involved in the parsing must have their schema
 * declared to dervish.
 * <P>
 * Any dervish routine that takes a handle as an input can also take
 * a handle expression.
 * <P>
 * Example: if h17 is a handle to a REGION, then h17.nrow is a handle
 * to an INT which the the number of rows in that region.
 * <P>
 * The function calls to access the parser are:
 *<P>
 * 	int shTclHandleExprEval(interp, line, &amp;handle, &amp;userPtr)
 *<P>
 * where line is the input character string, handle is user-supplied storage
 * to store the output handle structure, and userPtr is user-supplied storage
 * for certain exceptional cases.  The exceptional cases arise in the following
 * 2 instances.  Suppose h22 is a pointer to a structure as follows:
 * <PRE>
 *	typedef struct {
 *	   int array[10];
 *	   } EXAMPLE;
 * </PRE>
 * then the expressions   &amp;h22   returns a handle to a pointer to a
 * structure of type EXAMPLE, and  h22.array   return a handle to a
 * pointer to an array of INT's.  In both cases, extra storage is needed
 * to hold the actual pointer.  This storage is supplied by the user in
 * userPtr.  The calling routine is responsible for making sure that this
 * storage is persistent if the returned handle structure is to remain
 * valid outside the scope of the calling routine (e.g., if the
 * result of the handle expression is explictly bound to a new handle,
 * then userPtr should be shMalloc'ed by the calling routine.)
 *<P>
 * The grammar for expressions is:
 *<PRE>
 * expr : term
 *	| * expr
 *	| &amp; expr
 *	| (type) term
 *
 * term : primary
 *	| term . WORD
 *	| term -> WORD
 *	| term &lt; INTEGER &gt;
 *	| term [ INTEGER ]
 *
 * primary : WORD
 *	| ( expr )
 *	| expr
 *</PRE>
 * This results in the normal C precedence and associativity for these
 * operators.
 *<P>
 * There is a problem, in that a ( in expr can be a (cast) or an (expr); this
 * is handled by looking to see if the first token in the parentheses is
 * a known type.
 *<P>
 * The expressions are evaluated by means of a recursive descent parser.
 *</HTML>
 *</AUTO>
**
** ENTRY POINT		SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
** shHandleExpr		public  Do the work.
**
** ENVIRONMENT:
**	ANSI C.
**
** REQUIRED PRODUCTS:
**	tcl
**	dervish
**
** AUTHORS:
**	Creation date:  Sept. or Oct. 1993
**	Robert Lupton
**	Steve Kent (Small modifications only)
**      Wei Peng 04/15/94 added a routine to evaluate expresion from C level.
**      	and modified affected lexical analyzer routines.
**
** RETURN VALUES:
**	TCL_OK -   Glorious success
**	TCL_ERROR -   Failed miserably.
**
** CALLS TO:
**	routineName27
**	routineName36
**	routineName98
**
** MODIFIED: S. Kent Oct. 30, 1993 Return results in user-defined storage
** to avoid potential gotcha's.
**
**
**============================================================================
*/
/*
******************************************************************************
******************************************************************************
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifndef NOTCL
#include "tclExtend.h"
#include "ftcl.h"
#endif
#include "dervish_msg_c.h"
#include "region.h"
#include "shC.h"
#include "shCUtils.h"
#include "shCHdr.h"
#include "shCFitsIo.h"
#include "shCErrStack.h"
#include "shCRegUtils.h"
#include "shCRegPrint.h"
#include "shCAssert.h"
#include "shCEnvscan.h"
#include "shCGarbage.h"
#ifndef NOTCL
#include "shTclRegSupport.h"
#include "shCSao.h"
#include "shTclVerbs.h"
#include "shTk.h"
#endif
#include "shTclHandle.h"

#include "shCSchema.h"

/*============================================================================  
**============================================================================
**
** LOCAL MACROS, DEFINITIONS, ETC.
**
**============================================================================
**
*/
#define DBG_VAL 3			/* shVerbosity required to make this
					   file chatty */
/*
 * Token definitions
 */
#define ERROR 256
#define INT 257
#define WORD 258
#define ARROW 259			/* -> */
#define HANDLEADDR_START 260
#define HANDLEADDR       261

/*----------------------------------------------------------------------------
**
** Internal functions
**
 * Parser
 */
static int expr(void);			/* see grammar in header */
static int primary(void);		/* for definitions of these */
static int term(void);
static int error(char *msg);		/* report an error */

static int integer;			/* value of current token if INT */
static char word[100];			/* value of current token if WORD */
static int token;			/* current token */

/*
 * Lex analyser
 */
static void drop_saved_state(void);	/* forget a saved state */
static int get_subscript(void);		/* get a subscript from input */
static void int_or_word(void);		/* is a token an INT or a WORD? */
static void next_token(void);		/* get the next token */
static int restore_lex_state(void);	/* restore or save the */
static int save_lex_state(void);	/*         state of the lex analyser */
#if defined(DEBUG)
static void print_token(void);		/* print current token */
#endif

/*
 * I/O
 */
static int peek(void);			/* look at next character */
static int readc(void);			/* read a character */
static int unreadc(int c);		/* push a character */

static char *str,*sptr;
#ifndef NOTCL
static Tcl_Interp *interp = NULL;	/* the TCL interpreter in use */
#endif

/*****************************************************************************/
/*
 * These are the properties being manipulated by this parser
 */
static void *g_addr = NULL;		/* current value of expr */
static void *g_addr_ptr;		/* address of addr */
static char *g_nelem;			/* dimensions, c.f. SCHEMA_ELEM */
static int   g_nstar;			/* expr's degree of indirection */
static TYPE  g_type;			/* type of expr */

/*
 * This flag indicates if parsing is driven by shExprEval() or 
 * shExprTypeEval(). The latter routine was written by Steve Kent outside of
 * this file and later folded in this source file. As such, it does not need
 * two global variables: g_addr and g_addr_ptr.
 * This flag is set in shExprTypeEval(). Upon entry in this compilation unit,
 * or a call to shExprEval() it is reset to 0.
 */
static int g_Flag = 0;

#ifndef NOTCL
/*****************************************************************************/
/*
 * evaluate an expression involving a handle, *, &, ., ->, [] or <>, and ()
 *
 * Precedence is an in C
 *
 * The result is returned in a user supplied handle.
 * TCL_OK is return in case of success;
 * TCL_ERROR is otherwise returned.
 * 
 */
int
shTclHandleExprEval(
		    Tcl_Interp *iinterp, /* IN: the TCL interpreter */
		    char *line,		 /* IN: expression to be evaluated */
		    HANDLE *handle,	 /* OUT: Translated handle */
		    void **userPtr	 /* OUT: Indirect address storage,
					         if needed; else NULL */
		    )
{
   int len = strlen(line) - 1;
/* S. Kent.  Following declaration is no longer used.  Storage for the
** returned HANDLE is now supplied by the calling program */
/** static HANDLE handle; **/		/* the result of all this work */
   sptr = str = line;			/* copy line out to globals */
   interp = iinterp;			/* copy interp out too */

   g_addr = NULL;

   g_Flag = 0;				/* Evaluate type and address */

   while(isspace(line[len])) {		/* trim trailing whitespace */
      line[len--] = '\0';
   }

   next_token();
   if(expr() < 0) {
      return(TCL_ERROR);
   }
   
   if(token != EOF) {
      error("Trailing characters at end of expression");
      return(TCL_ERROR);
   }

   if(g_nelem != NULL) {		/* a non-dereferenced array */
      g_addr_ptr = g_addr;		/* convert to a pointer */
      g_addr = &g_addr_ptr;
   }
   
   if(g_addr == &g_addr_ptr) {
      handle->type = shTypeGetFromName("PTR");
/*
 * We need the _address_ of our pointer in the handle. We've been using
 * g_addr_ptr, but that isn't good enough once this function finishes.
 *
 * The memory'll be freed when the handle is deleted or rebound
 */
/* No more! Avoid potential memory-leak problems.
** Use user-supplied storage instead */
/*      handle.ptr = shMalloc(sizeof(void *));	*/
/*      *(void **)handle.ptr = g_addr_ptr;	*/
	*userPtr = g_addr_ptr;
	handle->ptr = userPtr;
   } else {
      if (userPtr != NULL) *userPtr = NULL;
      if(g_nstar > 0) {
	 if(g_nstar == 1 && g_type == shTypeGetFromName("CHAR")) {
	    g_type = shTypeGetFromName("STR");
	 } else {
	    g_type = shTypeGetFromName("PTR");
	 }
      }
      handle->type = g_type;
      handle->ptr = g_addr;
   }
   
   return(TCL_OK);
}


/******************************************************************************
<AUTO EXTRACT>
 * 
 *  ROUTINE: shTclHandleExprEvalWithInfo
 *
 *  DESCRIPTION: evaluate an expression involving a handle, *, &amp;, ., ->, [] 
 *		 or <>, and (). 
 *
 *  RETURN VALUES:
 *      TCL_OK: operation is a success.
 *	TCL_ERROR: could not evaluate correctly the expression
 *
 *      in address_out: the address pointed by the handle expression
 *      in sch_elm: a complete 'description' of what is the result of the 
 *                  handle expression. This is usefull since the handle types
 *                  are slightly poor in Dervish.
 *      in out_type: the underlined type. (this is equivalent to the type
 *                   described in sch_elm->type but expressed in integer value)
 *
</AUTO> */

RET_CODE
shTclHandleExprEvalWithInfo(
		    Tcl_Interp *iinterp, /* IN: the TCL interpreter */
		    char *line,		 /* IN: expression to be evaluated */
		    char **address_out,	 /* OUT: return address */
		    SCHEMA_ELEM *sch_elm,/* OUT: contain type information */
		    int *out_type        /* final type */
		    )
{
   char *typename,buff[100];
   const SCHEMA *schema;
   int len = strlen(line) - 1;
   sptr = str = line;			/* copy line out to globals */
   interp = iinterp;			/* copy interp out too */

   g_addr = NULL;

   g_Flag = 0;				/* Evaluate type and address */

   while(isspace(line[len])) {		/* trim trailing whitespace */
      line[len--] = '\0';
   }

   next_token();
   if(expr() < 0) {
      return(TCL_ERROR);
   }
   
   if(token != EOF) {
      error("Trailing characters at end of expression");
      return(TCL_ERROR);
   }

   if (address_out!=NULL) *address_out = (char *)g_addr;
   if (out_type!=NULL) *out_type = g_type;

   if (sch_elm != NULL) {
#if SCHEMA_ELEMS_HAS_TTYPE
     sch_elm->ttype = g_type;
#endif
     typename = shNameGetFromType(g_type);
     strcpy (sch_elm->type,typename);  /* because sch_elm is an actual array */
     sch_elm->nstar = g_nstar;
     sch_elm->nelem = g_nelem;
     sch_elm->offset = 0;		/* offset is zero here since we already
					   returning the address of the element */

     if (strcmp(typename,"UNKNOWN")!=0) {
       if((schema = shSchemaGet(typename)) == NULL) {
	 sprintf(buff,"Cannot retrieve schema for %s",typename);
	 return(error(buff));
       }
       sch_elm->size = schema->size ;     /* Will be needed if the object is an 
					     array ! */
     } else 
       sch_elm->size = 0;               /* We don't know !! */
   };

   return(TCL_OK);

}
#endif

/****************************************************************************/
/*
 * This C callable routine provides the same functionality as the TCL C
 * routine, shTclHandleExprEval.  It is meant to evaluate an expression
 * involving a handle, *, &, ., ->, [] or <>, and ().  Precedence is as
 * in C.
 *
 * G. Sergey 5/16/94
 * The expression to be evaluated (line) may contain a single "?", to
 * identify the location of the object (specified by address_in and 
 * schema) within the string.  If a "?" is not present, the expression
 * is assumed to refer to a member of the object.  Examples:
 *
 * "(REGION *)?.myfield"  =  "(REGION *)myobject.myfield" 
 *             "myfield"  =  "myobject.myfield"
 * 
 */
RET_CODE
shExprEval(
   SCHEMA* schema,  	/* schema that describes the object of address_in */
   char *line, 	   	/* a string to eval */
   char* address_in, 	/* address of the object */
   char** address_out,	/* return address */
   int *out_type,       /* final type */
   char** userPtr
)
  		  
{
  int len = strlen(line) - 1;
  sptr = str = line;			/* copy line out to globals */
  #ifndef NOTCL
  interp = NULL;			
  #endif

   /* Set globals */
   g_addr  = address_in;
   g_nstar = 0;		
   g_nelem = NULL;

   g_Flag  = 0;

   if(schema==NULL || line==NULL || address_in==NULL) {
      shErrStackPush("No schema\n");
      return SH_GENERIC_ERROR;
   }
  
   if(line==NULL || address_in==NULL) 
   {
       if(address_out!=NULL) *address_out=NULL; 
       return SH_SUCCESS;
   }

   g_type=shTypeGetFromName(schema->name);

   while(isspace(line[len])) {		/* trim trailing whitespace */
      line[len--] = '\0';
   }

   /* If the string contains a '?' then get the next token */
   if (strchr(line, '?') != NULL) {
     next_token();
   /* Otherwise assume the first token is the object that was passed 
      as a parameter to this routine.  This will affect the "primary"
      function when it eventually gets called.  Note that g_addr has
      already been set. */
   } else {
     token = HANDLEADDR_START;
   }

   if(expr() < 0) {
      return SH_GENERIC_ERROR;
   }
   
   if(token != EOF) {
      error("Trailing characters at end of expression");
      return SH_GENERIC_ERROR;
   }

   if(g_nelem != NULL) {		/* a non-dereferenced array */
      g_addr_ptr = g_addr;		/* convert to a pointer */
      g_addr = &g_addr_ptr;
   }
   
   if(g_addr == &g_addr_ptr) 
   {
      *userPtr = (char *)g_addr_ptr;
      *address_out = (char*) userPtr;
      
     
   } else 
  {
      *userPtr=NULL;
      *address_out = (char *)g_addr;
   }
   if(out_type!=NULL) *out_type = g_type;
   
   return SH_SUCCESS;
}

/*
 * This routine was written by Steve Kent to support his chain extensions.
 * According to his mail message:
 * 
 *  shExprTypeEval differs from shExprEval (the C callable routine) in one
 *  critical aspect: shExprEval returns both the TYPE and a pointer to the
 *  actual evalued location from a handle expression; shExprTypeEval returns
 *  just the TYPE.  The reason is that when it is called, it does not have
 *  available a top-level handle pointer that can be evaluated to a valid 
 *  location. There is a second difference that I don' know if Wei put into 
 *  shExprEval by design: when a handle expression evaluates to a pointer to 
 *  some structure or array, Robert Lupton (in shTclHandleExprEval) converts 
 *  the returned handle to be of type PTR; Wei made shExprEval return the 
 *  type of the pointed-to thing.  I desire the first behavior.  Rather than 
 *  hacking code (and possibly breaking it), I ended up making a separate 
 *  routine.  If you want, it should be possible to modify either 
 *  shTclHandleExprEval or shExprEval to take an extra argument that 
 *  determines whether pointer evaluation occurs or not. It would also 
 *  require modifying other routines that call it, and I didn't want to get
 *  into so much work.
 *
 */
RET_CODE
shExprTypeEval(
   SCHEMA* schema,  /* schema that describes the object of address_in */
   char *line, 	   	/* a string to eval */
   int *out_type        /* final type */
)
  		  
{
   int len = strlen(line) - 1;
/* Following line was left out by mistake when this routine 1st inserted
 * - S. K. */
   sptr = str = line;                 /* copy line out to globals */
   #ifndef NOTCL
   interp = NULL;			
   #endif

   /* Set globals */
   g_nstar = 0;		
   g_nelem = NULL;

   g_Flag = 1;

   if(schema==NULL || line==NULL) {
      shErrStackPush("No schema\n");
      return SH_GENERIC_ERROR;
   }
  
   if(line==NULL) 
   {
       return SH_SUCCESS;
   }

   g_type=shTypeGetFromName((char *)schema->name);

   while(isspace(line[len])) {		/* trim trailing whitespace */
      line[len--] = '\0';
   }

   /* If the string contains a '?' then get the next token */
   if (strchr(line, '?') != NULL) {
     next_token();
   /* Otherwise assume the first token is the object that was passed 
      as a parameter to this routine.  This will affect the "primary"
      function when it eventually gets called.  Note that g_addr has
      already been set. */
   } else {
     token = HANDLEADDR_START;
   }

   if(expr() < 0) {
      return SH_GENERIC_ERROR;
   }
   
   if(token != EOF) {
      error("Trailing characters at end of expression");
      return SH_GENERIC_ERROR;
   }

/* Adjust the output type for several special cases. */

   if(g_nelem != NULL) 			/* a non-dereferenced array */
      g_type = shTypeGetFromName("PTR");
   if(g_nstar > 0) {
	 if(g_nstar == 1 && g_type == shTypeGetFromName("CHAR")) {
	    g_type = shTypeGetFromName("STR");
	 } else {
	    g_type = shTypeGetFromName("PTR");
	 }
      }

   if(out_type!=NULL) *out_type = g_type;
   
   return SH_SUCCESS;
}


/*****************************************************************************/

static int
expr(void)
{
   TYPE cast_type;			/* type to cast to */
   int cast_nstar;			/* degree of indirection to cast to */
   static char *cast_nelem;		/* array type to cast to */
   static char s_cast_nelem[100];	/* storage for cast_nelem */
   const SCHEMA *schema;		/* base type to cast to */
   int subscript;			/* value of desired subscript in cast*/
   
   for(;;) {
      switch (token) {

       case '*':

	 next_token();
	 if(expr() < 0) {
	    return(-1);
	 }
         if (g_Flag == 0)  {
  	     if (g_nstar == 0) {
	         return(
                      error("expression to be dereferenced is not a pointer"));
	     } else if(g_addr == NULL) {
	         return(error("expression to be dereferenced is NULL"));
	     }

	     g_addr = *(void **)g_addr;
	     g_addr_ptr = NULL;

         } else if (g_Flag == 1)
             if (g_nstar == 0)
   	         return(error("expression to be dereferenced is NULL"));

	 g_nstar--;

	 shDebug(DBG_VAL,"\t*");
	 return(0);

       case '&':

	 next_token();
	 if(expr() < 0) {
	    return(-1);
	 }

         if (g_Flag == 0)  {
	     if(g_addr == &g_addr_ptr) {
	        return(error("Cannot take address of an address"));
	     }
             g_addr_ptr = g_addr;
             g_addr = &g_addr_ptr;
         }

	 g_nstar++;

	 shDebug(DBG_VAL,"\t&");
	 return(0);

       case '(':			/* maybe a cast */
	 if(save_lex_state() < 0) {
	    return(-1);
	 }
	 next_token();
	 if(token == WORD &&
	    (schema = shSchemaGet(word)) != NULL) { /* it's a cast */
	    drop_saved_state();
	    next_token();

	    cast_nelem = NULL;
	    cast_nstar = 0;
	    while(token == '*') {
	       cast_nstar++;
	       next_token();
	    }
	    if(token == '<' || token == '[') {
	       cast_nelem = s_cast_nelem;
	       *cast_nelem = '\0';
	       do {			/* build up nelem string */
		  if((subscript = get_subscript()) < 0) {
		     return(-1);
		  }
		  next_token();

		  sprintf(cast_nelem,"%s%d",
			  (cast_nelem == s_cast_nelem ? "" : " "),subscript);
		  cast_nelem += strlen(cast_nelem);
	       } while(token == '<' || token == '[');
	       cast_nelem = s_cast_nelem;
	    }

	    if(token == EOF) {
	       return(error("premature end of expression"));
	    } else if(token != ')') {
	       return(error("Expected ) at end of cast"));
	    } else {
	       cast_type = shTypeGetFromName((char *)schema->name);
	    }
	    next_token();		/* eat the ')' */

	    if(expr() < 0) {
	       return(-1);
	    }
	    g_type = cast_type;		/* do the cast here */
	    g_nstar = cast_nstar;
	    g_nelem = cast_nelem;
	    return(0);
	 } else {			/* not a cast */
	    if(restore_lex_state() < 0) {
	       return(-1);
	    }
	    return(term());		/* so let term deal with (...) */
	 }
	
/* NOTE! NOTE!
   This break was removed to satisfy the picky compiler ... 
   remember to put a break here if the above code is changed and
   a break is meant here!

	 break;
*/
       case ERROR:
	 return(-1);
       default:
	 return(term());
      }
    }
   /* this should never be reached */
   /* these lines prevents a warning dietz 6-26 */
   exit(2);
   return(-1);
}

/*****************************************************************************/
/*
 * Parse a term
 */
static int
term(void)
{
   char buff[200];
   int anOperator = token;		/* which operator was it? */
   const SCHEMA *schema;
   const SCHEMA_ELEM *c_schema_elem;	/* we need to modify ->nstar */
   SCHEMA_ELEM schema_elem;
   int size;				/* size of an array element */
   int subscript;			/* value of desired subscript */
   char *typename;			/* name of current type */

   if(primary() < 0) {	/*  address is already initialized . */
     return(-1);
   }

   for(;;) {
      switch (token) {
       case '.':			/* prim.WORD */
       case ARROW:			/* prim->WORD */
	 anOperator = token;
	 
         if (g_Flag == 0)
             if (g_addr == NULL)
	        return(error("Can't dereference a NULL address"));
	 
	 next_token();			/* eat the . or -> and get next */
	 if(token != WORD) {
	    return(error("expected WORD after ."));
	 }
	 if(anOperator == '.') {
	    if(g_nstar != 0) {
	       return(error("You cannot use . with a pointer"));
	    }
	    shDebug(DBG_VAL,"\t.%s",word);
	 } else {
	    if(g_nstar != 1) {
	       return(error("You can only use -> with a pointer to a struct"));
	    }

            if (g_Flag == 0)
                if((g_addr = *(void **)g_addr) == NULL) {
	            return(error("Can't dereference a NULL address"));
	        }

	    shDebug(DBG_VAL,"\t->%s",word);
	 }

	 typename = shNameGetFromType(g_type);
	 if((c_schema_elem = shSchemaElemGet(typename,word)) == NULL) {
	    sprintf(buff,"%s is not a member of %s",word,typename);
	    return(error(buff));
	 }
	 schema_elem = *c_schema_elem;
	 g_nelem = schema_elem.nelem;
	 g_nstar = schema_elem.nstar;
	 schema_elem.nstar = 0;		/* we want to know the base type */
#if SCHEMA_ELEMS_HAS_TTYPE
	 schema_elem.ttype = UNKNOWN_SCHEMA;
#endif

         if (g_Flag == 0)  {
	     g_addr = shElemGet(g_addr,&schema_elem,&g_type);
             g_addr_ptr = NULL;
         } else if (g_Flag == 1)
             g_type = shTypeGetFromSchema(&schema_elem);
	 
	 next_token();			/* eat the WORD */
	 break;
       case '<':			/* prim<INT> */
       case '[':			/* prim[INT] */
	 typename = shNameGetFromType(g_type);
	 if((schema = shSchemaGet(typename)) == NULL) {
	    sprintf(buff,"Cannot retrieve schema for %s",typename);
	    return(error(buff));
	 }
	 
	 if(g_nelem != NULL) {		/* a real n-dimensional array */
/*
 * We must deal with all of the dimensions now. For an array declared
 * as arr[Z][Y][X] the nelem string is "Z Y X", and the array element
 * arr[z][y][x] is x + X*(y + Y*z) elements from the start of the array
 */
	    char *dimen = g_nelem;	/* pointer to current dimen */
	    int offset = 0;		/* offset in units of size of data */

	    *buff = '\0';
	    for(;;) {
	       if((subscript = get_subscript()) < 0) {
		  return(-1);
	       }
	       if(subscript >= atoi(dimen)) {
		  return(error("Subscript is too large"));
	       }
	       next_token();		/* eat the > or ] */
	       sprintf(buff + strlen(buff),"[ %d ] ",subscript);
	       
	       offset = subscript + offset*atoi(dimen);

	       if((dimen = strchr(dimen,' ')) == NULL) { /* no more indices */
		  break;
	       }
	       dimen++;			/* skip ' ' */

	       if(token != '<' && token != '[') { /* no more subscripts */
		  return(error("Only fully indexed arrays are supported"));
	       }
	    }

	    size = (g_nstar == 0) ? schema->size : sizeof(void *);

            if (g_Flag == 0)
                g_addr = (void *)((char *)g_addr + size*offset);

	    g_nelem = NULL;
	    shDebug(DBG_VAL,"\t%s",buff);
	 } else {			/* a pointer to be dereferenced */
	    if(g_nstar == 0) {
	       return(error("expression to be subscripted is not a pointer"));
	    }

            if (g_Flag == 0) 
	        if(*(void **)g_addr == NULL)
	           return(error("expression to be subscripted is NULL"));

	    if((subscript = get_subscript()) < 0) {
	       return(-1);
	    }
	    next_token();
	    g_nstar--;
	    size = (g_nstar == 0) ? schema->size : sizeof(void *);

            if (g_Flag == 0)  {
	        g_addr = (void *)((char *)*(void **)g_addr + size*subscript);
                g_addr_ptr = NULL;
	    }

	    shDebug(DBG_VAL,"\t[ %d ]",subscript);
	 }
	 break;
       default:
	 return(0);
      }
   }
   /* this should never be reached */
   /* these lines prevents a warning dietz 6-26 */
   exit(2);
   return(-1);
}

/*****************************************************************************/
/*
 * Parse a primary expression
 */
static int
primary(void)
{
   HANDLE *handle;			/* handle being manipulated */
   
   for(;;) {
      switch (token) {

       /* For this special case, g_addr has already been set in shExprEval.  
          Assume the next token is '.' then continue */
       case HANDLEADDR_START:
          token = '.';
	  return(0);
       /* For this case too, g_addr has already been set in shExprEval.  
          This case occurs when the '?' character is detected by 
          next_token.  Get the subseqent token and continue */
       case HANDLEADDR:
          next_token();
	  return(0);

       case WORD:		/* this should be a handle's name */
         if (g_Flag == 1)  {
             printf("Unexpected WORD\n");
             return(error("unexpected handle name"));
         } else if (g_Flag == 0)  {
             if(g_addr == NULL) {	/* OK, we haven't seen a handle yet */
#ifndef NOTCL
	        if(p_shTclHandleAddrGet(interp,word,&handle) == TCL_ERROR) {
	           shError("Unknown handle: %s\n",word);
	           return(-1);
	        }
#endif
	        shDebug(DBG_VAL,"\thandle: %s",word);
	        g_addr = handle->ptr;
	        g_type = handle->type;
	        g_addr_ptr = NULL;
	        g_nstar = 0;
	        g_nelem = NULL;
	     } else
	        return(error("unexpected handle name"));
         }

          next_token();
	  return(0);
       case '(':			/* start ( expr ) */
	 next_token();
	 if(expr() < 0) {
	    return(-1);
	 } else if(token != ')') {
	    return(error("expected to see )"));
	 }
	 next_token();
	 
	 return(0);
       case '*':			/* could be an expr */
       case '&':
	 if(expr() < 0) {
	    return(-1);
	 }
	 next_token();
	 break;
       case EOF:
	 return(error("premature end of expression"));
       case ERROR:
	 return(-1);
       default:
	 return(error("unexpected token"));
      }
    }
   /* this should never be reached */
   /* these lines prevents a warning dietz 6-26 */
   exit(2);
   return(-1);
}

/*****************************************************************************/
/*
 * read a subscript from the input stream; the initial < or [ is still
 * the current token. You have to call next_token() to eat the closing
 * > or ] (this allows error() to indicate errors correctly).
 */
static int
get_subscript(void)
{
   char buff[200];
   int start = token;			/* starting character, < or [ */
   int subscript;
   
   next_token();
	 
   if(token != INT) {
      sprintf(buff,"expected int after %c",start);
      return(error(buff));
   }
   subscript = integer;
   next_token();
   
   if(start == '<' && token != '>') {
      return(error("expected > after < INT "));
   } else if(start == '[' && token != ']') {
      return(error("expected ] after [ INT "));
   }

   return(subscript);
}

/*****************************************************************************/
/*
 * Print an error message, and indicate where the problem occurred
 */
static int
error(char *msg)
{
   shError("Syntax error: %s",msg);
   shError("%s",str);
   shError("%*s^",sptr - str - 1,"");
   next_token();
   return(-1);
}

/*****************************************************************************/
/*
 * End of parser, now the Lex analyser
 */

static void
next_token(void)
{
   int c;
   char *ptr = word;

   while(ptr < word + 100) {
      if((c = readc()) == EOF) {
	 if(ptr == word) {
	    token = EOF;
	    return;
	 }
	 *ptr = '\0';
	 int_or_word();
	 return;
      }

      switch (c) {
       case '\n':
       case '\t':
       case ' ':
       case '.':
       case '(':
       case ')':
       case '[':
       case ']':
       case '<':
       case '>':
       case '*':
       case '&':
       case '-':
       case '?':
 	 if(ptr > word) {		/* terminate previous token... */
	    if(isspace(c)) {
	       int len = ptr - word;
	       if((len == 5  && strncmp(word,"const",len) == 0 )||
		  (len == 12 && strncmp(word,"const struct",len) == 0) ||
		  (len == 6  && strncmp(word,"signed",len) == 0 )||
		  (len == 8  && strncmp(word,"unsigned",len) == 0)) { /* but not
								       in these
								       cases */
		  *ptr++ = c;
		  break;
	       }
	    }
	    (void)unreadc(c);
	    *ptr = '\0';
	    int_or_word();
	 } else if(c == ' ' || c == '\t') {
	    break;
	 } else if(c == '-') {
	    if(peek() == '>') {
	       (void)readc();
	       token = ARROW;
	    } else {
	       token = ERROR;
	    }
	 } else if(c == '?') {
            /* Special case - see shExprEval routine for details */
            token = HANDLEADDR;		
	 } else {
	    token = c;
	 }
#if defined(DEBUG)
	 print_token();
#endif
	 return;
       default:
	 *ptr++ = c;
	 break;
      }
   }
}

/********************************************************************/

static void
int_or_word(void)
{
   char *ptr = word;

   while(isdigit(*ptr)) ptr++;
   if(*ptr == '\0') {
      integer = atoi(word);
      token = INT;
   } else {
      token = WORD;
   }
}

/********************************************************************/

#if defined(DEBUG)
static void
print_token(void)
{
   switch (token) {
    case ERROR:
      printf("ERROR\n");
      break;
    case INT:
      printf("INT: %d\n",integer);
      break;
    case HANDLEADDR_START:
      printf("HANDLEADDR_START\n");
      break;
    case HANDLEADDR:
      printf("HANDLEADDR\n");
      break;
    case WORD:
      printf("WORD: %s\n",word);
      break;
    case ARROW:
      printf("->\n");
      break;
    case '\n':
      printf("\\n\n");
      break;
    default:
      printf("%c\n",token);
      break;
   }
}
#endif

/*****************************************************************************/
/*
 * save or restore a lex state, i.e. the current token and the state
 * of the read pointer
 */
static int saved_integer;		/* saved value of integer */
static char *saved_sptr = NULL;		/* saved value of sptr */
static int saved_token;			/* saved token */

static int
save_lex_state(void)
{
   if(saved_sptr != NULL) {
      return(error("a lex state is already saved"));
   }
   saved_integer = integer;
   saved_sptr = sptr;
   saved_token = token;

   return(0);
}

static int
restore_lex_state(void)
{
   if(saved_sptr == NULL) {
      return(error("no lex state is currently saved"));
   }
   integer = saved_integer;
   sptr = saved_sptr;
   token = saved_token;

   drop_saved_state();

   return(0);
}

/*
 * forget the saved state
 */
static void
drop_saved_state(void)
{
   saved_sptr = NULL;
}

/********************************************************************/
/*
 * I/O (from a string not a file)
 */
/********************************************************************/

static int
readc(void)
{
   int c;

   if((c = *sptr++) == '\0') {
      sptr--;
      return(EOF);
   } else {
      return(c);
   }
}

/********************************************************************/

static int
unreadc(int c)
{
   if(sptr > str) {
      *--sptr = c;
   }
   return(c);
}

/********************************************************************/

static int
peek(void)
{
   return(*sptr == '\0' ? EOF : *sptr);
}
