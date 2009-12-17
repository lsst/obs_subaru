#ifndef SHTCLHANDLE_H
#define SHTCLHANDLE_H

#if !defined(NOTCL)
#include "ftcl.h"
#endif
#include "shCSchema.h"
#include "shCList.h"
#include "shChain.h"

#define TCL_SUFFIX    "h"  /* Suffix for TCL layer addresses (h0, h1, ...) */
#define MAX_HANDLES   10
#define HANDLE_NAMELEN 100		/* maximum length of a handle's name
					   (e.g. "h123") */

/*
 * Structure describing Dervish handles. We are not limited to dealing with a
 * REGION now, rather we can have anything else that we desire (STARS, MASKS,
 * etc.) So, instead of having routines to translate to and fro each handle,
 * we have this structure that is generic enough to contain anything.
 */
typedef struct tagHandle  {
   void *ptr;   /* Pointer to the actual data type */
   TYPE type;   /* Type of the data type ptr points to */
} HANDLE;

#if !defined(NOTCL)
/*
 * Function prototypes
 */
#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

int  shTclHandleInit(Tcl_Interp *a_interp);
int  p_shTclHandleNew(Tcl_Interp *a_interp, char *a_handleName);
int  p_shTclHandleAddrBind(Tcl_Interp *a_interp, const HANDLE a_handle,
			   const char *a_handleName);
int  p_shTclHandleDel(Tcl_Interp *a_interp, const char *a_handleName);
int  p_shTclHandleAddrGet(Tcl_Interp *a_interp, const char *a_handleName,
			  HANDLE **a_handle);
int  p_shTclHandleNameGet(Tcl_Interp *a_interp, const void *addr);
TYPE p_shTclHandleTypeGet(Tcl_Interp *a_interp, const char *a_handleName);
/* HANDLE *shTclHandleExprEval(Tcl_Interp *interp, char *line); */
/* S. Kent: Redefine the inputs */
int shTclHandleExprEval(Tcl_Interp *interp, char *line, HANDLE *userHandle,
   void **userPtr);

RET_CODE shTclHandleExprEvalWithInfo(
		    Tcl_Interp *interp, /* IN: the TCL interpreter */
		    char *line,		 /* IN: expression to be evaluated */
		    char **address_out,	 /* OUT: return address */
		    SCHEMA_ELEM *sch_elm,/* OUT: contain type information */
		    int *out_type        /* final type */
		   );

/* Wei Peng, 04/14/94. bad place to declar this routine. I can't find
   a better place! */
RET_CODE shExprEval(SCHEMA* schema,  char *line, char* address_in, 
	 char** address_out, int* out_type, char** userPtr);

RET_CODE shExprTypeEval(SCHEMA *, char *, int *);

int shTclChainHandleNew (Tcl_Interp *interp, char *chainname, char *type,
                        CHAIN **chain);

int shTclHandleNew (Tcl_Interp *interp, char *name, char *strType,
                    void *thing);
int shTclAddrGetFromName (Tcl_Interp *interp, char *name, void **address, 
                          char *strType);
int shTclChainAddrGetFromName (Tcl_Interp *, char *, char *, CHAIN **);
int shTclHandleGetFromName (Tcl_Interp *interp, char *name, HANDLE *handle);

HANDLE *shHandleWalk(void);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif /* if !defined(NOTCL) */

#endif
