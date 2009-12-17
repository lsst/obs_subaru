/*****************************************************************************
******************************************************************************
**
** ABSTRACT:
**   This file contains utility routines for handles in Dervish. As we move
**   on in the project, returning and assigning a handle to a REGION quickly
**   becomes obsolete. Instead of having one REGION, we can now have a
**   myriad of objects such as STARS, MASKS, etc. So instead of having a
**   type of handle for each, we have one handle that includes the type
**   of the structure being manipulated, as well as the pointer
**
** ENTRY POINT		SCOPE	 DESCRIPTION
** -------------------------------------------------------------------------
** shTclHandleInit        public   Initialize the TCL table data structures
** p_shTclHandleNew       private  Create a new handle text string and allocate
**                                 space for a HANDLE in the table data
**                                 structure
** p_shTclHandleAddrBind  private  (Re)bind a handle text string to an address
** p_shTclHandleDel       private  Delete a handle text string and everything
**                                 associated with it in the TCL table
** p_shTclHandleAddrGet   private  Given a handle text string, get the address
** p_shTclHandleNameGet   private  Given an address, get the handle text string
** p_shTclHandleTypeGet   private  Given a handle text string, get the TYPE
**                                 field of its HANDLE structure
** shTclListHandleNew	  public   API to get a handle to a new list
** shTclChainHandleNew	  public   API to get a handle to a new list
** shTclHandleNew	  public   API to bind a new struct to a handle
** shTclListAddrGetFromName public API to get list address, given handle
**				   name - assures list is of correct type
** shTclChainAddrGetFromName public API to get chain address, given handle
**                                 name - assures chain is of correct type
** shTclAddrGetFromName	  public   API to get struct address given
**				   handle name
** shTclHandleGetFromName public   API to get handle address, given
**				   handle name
** shHandleWalk           public   API to walk the handle list
**
** ENVIRONMENT:
**	ANSI C.
**
** REQUIRED PRODUCTS:
**
******************************************************************************
******************************************************************************
*/
#include <memory.h>         /* For memset() and friends */
#include <ctype.h>
#include <string.h>
#include "ftcl.h"
#include "shCErrStack.h"
#include "shTclErrStack.h"
#include "tclExtend.h"
#include "shTclParseArgv.h"
#include "shCGarbage.h"
#include "shCUtils.h"
#include "shChain.h"
#include "prvt/utils_p.h"		/* For p_shMemRefCntrDecr/Incr() */
#include "region.h"			/* Needed for PIXDATATYPE in shCSao.h */
#include "shCSao.h"			/* Needed for CMD_HANDLE in shTclVerbs */
#include "shTclVerbs.h"			/* for prototypes for C++ */
#include "shTclHandle.h"
#include "shTclUtils.h" 		/* needed for shTclDeclare prototype */
#include "shC.h"

/*
 * Global variables
 */
static void *g_tableHeader = NULL;	/* Pointer to the table header */
#define BUFFER_SIZE 1000

/*****************************************************************************/

int shTclHandleInit(Tcl_Interp *a_interp)
{
   /*
    * Initialize the ftclHelp routines
    */
   if (ftclCreateHelp(a_interp) != TCL_OK)
       Tcl_AppendResult(a_interp,
          "p_shTclHandleInit: error initializing TCL Help", (char *) NULL);

   /*
    * Initialize and create a dynamic TCL handle table. Initially the use
    * count is set to 1. Note: what we are storing in the table really is
    * the HANDLE structure, not just a pointer to it.
    */
   if ((g_tableHeader = Tcl_HandleTblInit(TCL_SUFFIX, sizeof(HANDLE),
                                          MAX_HANDLES)) == NULL) {
       Tcl_AppendResult(a_interp,
          "p_shTclHandleInit: error initializing Handle table", (char *) NULL);
       return TCL_ERROR;
   }

   /*
    * Alter the handle table use count by MAX_HANDLES
    */
   if (Tcl_HandleTblUseCount(g_tableHeader, MAX_HANDLES) == 0)  {
       Tcl_AppendResult(a_interp,
          "p_shTclHandleInit: error increasing use count", (char *) NULL);
       return TCL_ERROR;
   }

   return TCL_OK;
}

/*
 * p_shTclHandleNew - Allocate memory for new HANDLE and associate a
 *                  textual handle name for it
 */
int p_shTclHandleNew(Tcl_Interp *a_interp, char *a_handleName)
{
   HANDLE *hand;
   static TYPE handle_type = UNKNOWN;

   if(handle_type == UNKNOWN) {
      handle_type = shTypeGetFromName("HANDLE");
   }

   /*
    * Store memory aside for a HANDLE and get the (TCL level) address
    * in the form "hn" where n >= 0. This string is saved in a_handleName.
    * It is the caller's responsibility to ensure that a_handleName is
    * large enough. There is a #define HANDLE_NAMELEN that they should use
    */
   if ((hand = (HANDLE *)Tcl_HandleAlloc(g_tableHeader, a_handleName))
                                         == NULL)  {
       Tcl_AppendResult(a_interp,
          "p_shTclHandleNew: error in allocating memory for new handle",
		(char *) NULL);
       return TCL_ERROR;
   }

   hand->ptr = NULL;
   hand->type = handle_type;

   return TCL_OK;
}

/*
 * p_shTclHandleAddrBind - (Re)bind an address to a handle; i.e. given a handle
 *                       text string 'h0', and a HANDLE hndl,
 *                       bind the address of hndl to 'h0'.
 *
 */
int p_shTclHandleAddrBind(Tcl_Interp *a_interp, const HANDLE a_handle,
                      const char *a_handleName)
{
   HANDLE *hndl;

   hndl = (HANDLE *)Tcl_HandleXlate(a_interp, g_tableHeader, a_handleName);
   if(hndl == NULL) {
       Tcl_AppendResult(a_interp,
          "\np_shTclHandleAddrBind: cannot translate handle text string ",
          a_handleName, " to a valid address.", (char *) NULL);
       return TCL_ERROR;
   }

   if(hndl->ptr != NULL)  {		/* rebinding an old address */
      if(shIsShMallocPtr(hndl->ptr)) {
	 p_shMemRefCntrDecr(hndl->ptr);
      }
   }

   /*
    * re-bind the handle name to the new address
    */
   *hndl = a_handle;
   if(hndl->ptr != NULL && shIsShMallocPtr(hndl->ptr)) {
      p_shMemRefCntrIncr(hndl->ptr);
   }

   return TCL_OK;
}

/*
 * p_shTclHandleDel - Delete a handle
 */
int p_shTclHandleDel(Tcl_Interp *a_interp, const char *a_handleName)
{
   HANDLE *hndl;

   /*
    * Has to exist before deletion can take place
    */
   hndl = (HANDLE *)Tcl_HandleXlate(a_interp, g_tableHeader, a_handleName);
   if(hndl == NULL) {
       Tcl_AppendResult(a_interp, "\np_shTclHandleDel: unknown handle name ",
                        a_handleName, ". Deletion failed.", (char *) NULL);
       return TCL_ERROR;
   }

   if(hndl->ptr != NULL && shIsShMallocPtr(hndl->ptr)) {
      p_shMemRefCntrDecr(hndl->ptr);
   }
   Tcl_HandleFree(g_tableHeader, hndl);

   return TCL_OK;
}

/*
 * p_shTclHandleAddrGet - Given a handle name, get the address; i.e.
 *                    given 'h0' return address
 */
int p_shTclHandleAddrGet(Tcl_Interp *a_interp, const char *a_handleName,
                     HANDLE **a_handle)
{
   HANDLE *hndl;

   /*
    * Make sure that the handle exists.
    */
   hndl = (HANDLE *)Tcl_HandleXlate(a_interp, g_tableHeader, a_handleName);
   if(hndl == NULL)  {
       Tcl_AppendResult(a_interp,
          "\np_shTclHandleAddrGet: unknown handle name ", a_handleName,
          ".", (char *) NULL);
       return TCL_ERROR;
   }
   /*
    * Save the address of the handle in the parameter so that the calling
    * function can access it.
    */
   *a_handle = hndl;

   return TCL_OK;
}

/*
 * p_shTclHandleNameGet - Given an address, get the handle name; i.e.
 *                   given hndl.ptr, return 'h0' (where hndl is of type HANDLE)
 */
int p_shTclHandleNameGet(Tcl_Interp *a_interp, const void *addr)
{
   HANDLE *hndl;
   char name[HANDLE_NAMELEN];
   int    walkKeyPtr = -1;		/* must be -1 for initial call to
					   Tcl_HandleWalk */
   /*
    * In order to get the handle name, we have to traverse the entire table,
    * looking for the address specified in the parameter a_handle. Once found,
    * we translate the address to a handle name using Tcl_WalkKeyToHandle().
    * If not found, then tough luck.
    */
   while((hndl = (HANDLE *)Tcl_HandleWalk(g_tableHeader,&walkKeyPtr)) != NULL){
      if(hndl->ptr == addr) {
	 Tcl_WalkKeyToHandle(g_tableHeader, walkKeyPtr,name);
	 Tcl_SetResult(a_interp,name,TCL_VOLATILE);
	 return(TCL_OK);
      }
   }

   Tcl_AppendResult(a_interp,
	   "\np_shTclHandleNameGet: cannot find handle name for given address", (char *) NULL);

   return(TCL_ERROR);
}

/*
 * p_shTclHandleTypeGet - Given a textual handle name, get the HANDLE TYPE
 *                      field
 */
TYPE
p_shTclHandleTypeGet(Tcl_Interp *a_interp, const char *a_handleName)
{
   HANDLE *hndl;

   /*
    * Make sure that the handle exists.
    */
   hndl = (HANDLE *)Tcl_HandleXlate(a_interp, g_tableHeader, a_handleName);
   if(hndl == NULL)  {
       Tcl_AppendResult(a_interp,
          "\np_shTclHandleTypeGet: unknown handle name ", a_handleName,
          ".", (char *) NULL);
       return( shTypeGetFromName("UNKNOWN"));
   }

   return(hndl->type);
}

/*****************************************************************************/
/*
 * Now for the TCL interface
 */
static char *module = "shHandle";		/* name of this set of code */

/*****************************************************************************/

static char *tclHandleNew_use =
  "USAGE: handleNew";
static char *tclHandleNew_hlp =
  "Return a new handle (it'll contain the NULL pointer)";

static int
tclHandleNew(
	     ClientData clientData,
	     Tcl_Interp *interp,
	     int argc,
	     char **argv
	     )
{
   HANDLE handle;
   char name[HANDLE_NAMELEN];

   if (argc != 1) {
      Tcl_SetResult(interp,tclHandleNew_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   handle.ptr = NULL; handle.type = shTypeGetFromName("UNKNOWN");
   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      p_shTclHandleDel(interp,name);
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclHandleBind_use =
  "USAGE: handleBind handle address type";
static char *tclHandleBind_hlp =
  "Bind an address and type to a pre-existing handle";

static int
tclHandleBind(
	     ClientData clientData,
	     Tcl_Interp *interp,
	     int argc,
	     char **argv
	     )
{
   long long addr;
   HANDLE handle;

   if (argc != 4) {
      Tcl_SetResult(interp,tclHandleBind_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   addr = strtoll(argv[2],NULL,0);
   handle.ptr = (addr == 0) ? NULL : (void *)addr;
   handle.type = (TYPE )(isdigit(*argv[3]) ? strtoll(argv[3], NULL, 0)
	:shTypeGetFromName(argv[3]));

/* S. Kent: With the advent of handle expressions, it is now possible
** to generate at the TCL level handle expressions of type PTR
** that point to volatile storage.  There is no way yet to gracefully
** determine if one should shMalloc storage for such pointers to make
** them more permanent.  I will do nothing here, since so much of RHL's
** TCL code uses this routine, but it should be purged eventually.
*/
   if (handle.type == shTypeGetFromName("PTR")) {
	Tcl_SetResult(interp,
   "HandleAddrBind is not guaranteed to work with pointer types at present",
   TCL_VOLATILE);
	}
   if(p_shTclHandleAddrBind(interp,handle,argv[1]) != TCL_OK) {
       return(TCL_ERROR);
   }

   Tcl_SetResult(interp,argv[1],TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclHandleBindFromHandle_use =
  "USAGE: handleBindFromHandle handle <handleExpr>";
static char *tclHandleBindFromHandle_hlp =
  "Bind a pre-existing handle to the thing pointed to by a handle expression";

static int
tclHandleBindFromHandle(
	     ClientData clientData,
	     Tcl_Interp *interp,
	     int argc,
	     char **argv
	     )
{
   HANDLE handle;
   void *userPtr;

   if (argc != 3) {
      Tcl_SetResult(interp,tclHandleBindFromHandle_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if (shTclHandleExprEval (interp, argv[2], &handle, &userPtr) !=
	TCL_OK) {
	Tcl_SetResult(interp, "Error translating handle expression",
	   TCL_VOLATILE);
	return (TCL_ERROR);
	}

/* S. Kent: With the advent of handle expressions, it is now possible
** to generate at the TCL level handle expressions of type PTR
** that point to volatile storage.  We can shMalloc space here for more
** permanent storage.  The space is deallocated when the handle bound to
** this location is deleted.
*/
   if (handle.type == shTypeGetFromName("PTR")) {
      if (userPtr != NULL) {
/* Allocated permanent storage for the returned indirection pointer */
         if ((handle.ptr = shMalloc(sizeof (void *))) == NULL) {
		Tcl_SetResult(interp,
  "Cannot allocate storage for indirection pointer",TCL_VOLATILE);
		return (TCL_ERROR);
		}
         *(void **)handle.ptr = userPtr;
         }
      }
   if(p_shTclHandleAddrBind(interp,handle,argv[1]) != TCL_OK) {
       return(TCL_ERROR);
   }

   Tcl_SetResult(interp,argv[1],TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclHandleDel_use =
  "USAGE: handleDel handle";
static char *tclHandleDel_hlp =
  "Delete a handle (but not the object to which it is bound)";

static int
tclHandleDel(
	     ClientData clientData,
	     Tcl_Interp *interp,
	     int argc,
	     char **argv
	     )
{
   if (argc != 2) {
      Tcl_SetResult(interp,tclHandleDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if(p_shTclHandleDel(interp,argv[1]) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclHandleNameGet_use =
  "USAGE: handleNameGet addr";
static char *tclHandleNameGet_hlp =
  "Get the name of a handle, given the address that it contains";

static int
tclHandleNameGet(
	     ClientData clientData,
	     Tcl_Interp *interp,
	     int argc,
	     char **argv
	     )
{
   long long addr;

   if (argc != 2) {
      Tcl_SetResult(interp,tclHandleNameGet_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   addr = strtoll(argv[1],NULL,0);
   if(p_shTclHandleNameGet(interp,(addr == 0 ? NULL : (void *)addr))
								   != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclHandlePrint_use =
  "USAGE: handlePrint handle";
static char *tclHandlePrint_hlp =
  "Return the address and type of a handle";

static int
tclHandlePrint(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
   HANDLE userHandle;
   void *userPtr;
   HANDLE *hand = &userHandle;

   if(argc != 2) {
      Tcl_SetResult(interp,tclHandlePrint_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if (shTclHandleExprEval(interp, argv[1], &userHandle, &userPtr) !=
	TCL_OK) return (TCL_ERROR);

   printf(PTR_FMT" %s\n",hand->ptr,shNameGetFromType(hand->type));

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclHandleGet_use =
  "USAGE: handleGet handle";
static char *tclHandleGet_hlp =
  "Return the address and type of a handle";

static int
tclHandleGet(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
   char buff[BUFFER_SIZE];
   HANDLE userHandle;
   void *userPtr;
   HANDLE *hand = &userHandle;

   if(argc != 2) {
      Tcl_SetResult(interp,tclHandleGet_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if (shTclHandleExprEval(interp, argv[1], &userHandle, &userPtr) !=
	TCL_OK) return (TCL_ERROR);

   sprintf(buff,PTR_FMT" %s",hand->ptr,shNameGetFromType(hand->type));
   Tcl_SetResult(interp,buff,TCL_VOLATILE);


   return(TCL_OK);
}

/*****************************************************************************/

static char *tclHandleDeref_use =
  "USAGE: handleDeref handle";
static char *tclHandleDeref_hlp =
  "Return whatever is pointed at as a handle; in C: `*(void **)handle'";

static int
tclHandleDeref(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
   char buff[BUFFER_SIZE];
   HANDLE userHandle;
   void *userPtr;
   HANDLE *hand = &userHandle;

   if(argc != 2) {
      Tcl_SetResult(interp,tclHandleDeref_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if (shTclHandleExprEval(interp, argv[1], &userHandle, &userPtr) !=
	TCL_OK) return (TCL_ERROR);
   if(hand->ptr == NULL) {
      Tcl_SetResult(interp,"Attempt to dereference a NULL handle",TCL_STATIC);
      return(TCL_ERROR);
   }
   sprintf(buff,PTR_FMT,*(void **)hand->ptr);
   Tcl_SetResult(interp,buff,TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclHandleDerefAndSet_use =
  "USAGE: handleDerefAndSet [-type] handle value";
static char *tclHandleDerefAndSet_hlp =
  "Set what a <handle> points to to a <value>;\n"
  "in C, *(type *)handle = value. Of course, the value can be a pointer.\n"
  "The type is set by the <type>; valid types are -i (int; default) and\n"
  "-f (float)";

static int
tclHandleDerefAndSet(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
   char buff[BUFFER_SIZE];
   int ival;				/* possible values */
   float fval;
   HANDLE userHandle;
   void *userPtr;
   HANDLE *hand = &userHandle;
   int type;				/* type of value */

   if(argc == 4) {			/* a type flag is present */
      if(argv[1][0] != '-') {
	 Tcl_SetResult(interp,"Flag must start with a -\n",TCL_STATIC);
	 Tcl_AppendResult(interp,tclHandleDerefAndSet_use,TCL_STATIC,
		(char *) NULL);
	 return(TCL_ERROR);
      }
      switch (argv[1][1]) {
       case 'f': case 'F':		/* float */
	 type = 'f'; break;
       case 'i': case 'I':		/* integer */
	 type = 'i'; break;
       default:
	 sprintf(buff,"Unknown flag %s\n",argv[1]);
	 Tcl_SetResult(interp,buff,TCL_VOLATILE);
	 Tcl_AppendResult(interp,tclHandleDerefAndSet_use,TCL_STATIC,
		(char *) NULL);
	 return(TCL_ERROR);
      }
      argv++;
      argc--;
   } else {
      type = 'i';			/* default is int */
   }

   if(argc != 3) {
      Tcl_SetResult(interp,tclHandleDerefAndSet_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if (shTclHandleExprEval(interp, argv[1], &userHandle, &userPtr) !=
	TCL_OK) return (TCL_ERROR);

   if(hand->ptr == NULL) {
      Tcl_SetResult(interp,"Attempt to dereference a NULL handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   switch (type) {
    case 'f':
      if(Tcl_GetFloat(interp,argv[2],&fval) != TCL_OK) {
	 return(TCL_ERROR);
      }
      *(float *)hand->ptr = fval;
      sprintf(buff,"%g",fval);
      break;
    case 'i':
      if(Tcl_GetInt(interp,argv[2],&ival) != TCL_OK) {
	 return(TCL_ERROR);
      }
      *(int *)hand->ptr = ival;
      sprintf(buff,"%d",ival);
      break;
    default:
      sprintf(buff,"Unknown type: %c",type);
      break;
   }

   Tcl_SetResult(interp,buff,TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclHandleExprEval_use =
  "USAGE: handleExprEval expression";
static char *tclHandleExprEval_hlp =
  "Return the value of an expression containing on handle and any number\n"
  "of the operators . -> * & ()";

static int
tclHandleExprEval(
		  ClientData clientData,
		  Tcl_Interp *interp,
		  int argc,
		  char **argv
	       )
{
   char buff[BUFFER_SIZE];
   HANDLE userHandle;
   void *userPtr;
   HANDLE *hand = &userHandle;

   if(argc != 2) {
      Tcl_SetResult(interp,tclHandleExprEval_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if (shTclHandleExprEval(interp, argv[1], &userHandle, &userPtr) !=
	TCL_OK) return (TCL_ERROR);

/* Malloc persistent space here for the pointer.  This is consistent with RHL's
** original implementation of shTclHandleExprEval which has since been
** modified. */
   if (userPtr != NULL) {
	userHandle.ptr = (void *)shMalloc (sizeof (void *));
	*(void **)userHandle.ptr= userPtr;
	}
   sprintf(buff,PTR_FMT,hand->ptr);
   Tcl_SetResult(interp,buff,TCL_VOLATILE);
   sprintf(buff,"%s",shNameGetFromType(hand->type));
   Tcl_AppendElement(interp,buff);

   return(TCL_OK);
}


/******************************************************************************
<AUTO EXTRACT>
 *
 *  ROUTINE: shTclCreateValueList
 *
 *  DESCRIPTION:
 *    Create a tcl list of the values at 'address' interpreted as described
 *    by 'sch_elm' and 'type'.  The aspect of the list is determined
 *    by 'enum', 'recurse', 'unfold', 'nostring' and 'nolabel'.  For an explanation of
 *    these parameters see their definition in the help of exprGet.
 *    The 'level' parameter indicates the level of recursion we are at!
 *    A value of zero is the first call to the routine. A value greater than
 *    zero is use in subsequent calls. If the level is greater than zero and
 *    recurse is false the struct are printed: {...} !
 *    The flat_array argument can take three values:
 *		0 For False
 *		1 For Flattening Array
 *		STARTED_ARRAY (2) When An array is being processed (and flattened).
 *
 *  NOTE:
 *    The list is APPENDED to the dynamic string. We do NOT free or init the
 *    dynamic string.
 *    We are using tcl smarts to determine whether or not an element should be
 *    in close in between curly brackets!
 *
 *  RETURN VALUES:
 *    TCL_OK in case of sucess
 *    TCL_ERROR otherwise.
 *
</AUTO> */

/* This define is for the flat_array argument, it helps us put brackets
   only once around a multi-dimentional array while still putting brackets
   around struct inside the array.
   I.e. if flat_array is STARTED_ARRAY we don't put brackets around an
   enventual array.
   */
#define STARTED_ARRAY 2
int
shTclCreateValueList ( Tcl_DString* result, char* address, SCHEMA_ELEM sch_elm,
		       int type, int noEnum, int recurse, int unfold, int nostring,
		       int quote, int nolabel, int flat_array, int limit, int level)
{
  char buff[BUFFER_SIZE], *temp;
  const SCHEMA *schema;
  int i,elem_type,offset,sub_size,size,dimension_number;
  char *addr_elem;

  shDebug ( level+1 ,"At level %d, printing: %s at "PTR_FMT,level, sch_elm.type,(void*)address);
  shDebug ( level+1 ,"nstar: %d, nelem "PTR_FMT,sch_elm.nstar,sch_elm.nelem);

  /* We are using the schema PTR to tell shPtrSprint how to print strings. This means
     that the pointer passed to shPtrSprint is interpreted as a char** !
     So for an array we need to pass &address instead of address !
   */
  /* We are expecting sch_elm.nelem to be either NULL either a pointer to string containing
     a list of the size of each dimension of the array separed by a 'white space'.
   */

  if (sch_elm.nelem != NULL) {

    /* We have an array */

    size = strtoul ( sch_elm.nelem, &temp, 10);    /* Size of the first dimension of the array */
    dimension_number = 1;
    if ( temp == sch_elm.nelem ) {
      shErrStackPush("shTclCreateValueList: No valid dimension found");
      return TCL_ERROR;			   /* No valid dimension found */
    };

    /* We print as a string if we have an array of char (no pointer) of only one
       dimension */
    if ((!nostring)&&(sch_elm.nstar==0)&&
	(shStrcmp(sch_elm.type,"CHAR",SH_CASE_INSENSITIVE)==0)&&
	(strtoul(temp,NULL,10) == 0) ) {

      if ( strlen(address) <= size ) {     /* Print the string only if it is contained
                                              within the limit of the array ! */
	if (quote) {
	  sprintf(buff,"\"%s\"", shPtrSprint(&address, shTypeGetFromName("STR")));
	  Tcl_DStringAppendElement( result, buff);
	} else
	  Tcl_DStringAppendElement( result, shPtrSprint(&address, shTypeGetFromName("STR")));
      } else {
	sprintf(buff,PTR_FMT,(void*)address);
	Tcl_DStringAppendElement( result, buff );
      };

    } /* End of analysis of arrays as String. */
    else {
      if (!unfold) {
	sprintf(buff,PTR_FMT,(void*)address);
	Tcl_DStringAppendElement( result, buff );
      } else {

	shDebug (level+1 ,"nelem is :%s",sch_elm.nelem);
	offset = 1;
	if ( strtoul ( temp, NULL, 10 ) == 0 ) {
	  sch_elm.nelem = NULL;            /* We exhausted all the dimension */
	} else {
	  sch_elm.nelem = temp;            /* Set for recursion */
	  while ( (sub_size=strtoul( temp, &temp, 10)) != 0 ) {
	    offset = offset * sub_size;
	    dimension_number++;
	  }
	}
	offset = offset * ((sch_elm.nstar == 0) ? sch_elm.size : sizeof(void *));

	if (level!=0 && flat_array != STARTED_ARRAY) Tcl_DStringStartSublist( result);
	for ( i=0, addr_elem = address; i<size; i++, addr_elem += offset ) {
	  /* We actually flattened an array only:
	     if flat_array is true         (-flat)
	     if limit == 0                 (unlimited flatenning)
	     if dimension_number <= limit  (only last dimensions are flatenned)
	     */
	  if ( (flat_array) &&
	       ((limit==0)||(dimension_number<=limit))) {
	    /* If the arrays append to be an array of string and/or enum
	       (and we don't have -enum) the list will look like a struct
	       unless we add a space in front :
	          {hdr {{modCnt 0} {hdrVec 0x0}}}
	       vs {array {{This a string} {and another} {and yet another}}}
	       So intead we do this:
	          {array { {This a string} {and another} {and yet another}}}
	       NOTE the space in front of the first string!
	       GIVEN The way exprPrint interpret the result from this routine
	       the print out will be the expected result
	       */
	    Tcl_DStringAppend( result, " ",1);
	    if ( shTclCreateValueList( result, addr_elem, sch_elm, type,
				       noEnum, recurse, unfold, nostring,
				       quote, nolabel, STARTED_ARRAY, limit,
				       level+1) != TCL_OK )
	      return TCL_ERROR;
	  } else {
	    Tcl_DStringStartSublist( result);
	    sprintf(buff,"<%d>",i);
	    Tcl_DStringAppendElement( result, buff );
	    if ( shTclCreateValueList( result, addr_elem, sch_elm, type,
				       noEnum, recurse, unfold, nostring,
				       quote, nolabel, flat_array, limit, 
				       level+1) != TCL_OK )
	    return TCL_ERROR;
	    Tcl_DStringEndSublist( result);
	  };
	}; /* End of loop of arrays element */
	if (level!=0 && flat_array != STARTED_ARRAY) Tcl_DStringEndSublist( result);

      }; /* End of unfolding analysis */
    }  /* End of common analysis of Array */
  } /* End of Array analysis */

  else if (sch_elm.nstar != 0) {

      /* We have a pointer */

      if ((!nostring)&&(sch_elm.nstar==1)&&(shStrcmp(sch_elm.type,"CHAR",SH_CASE_INSENSITIVE)==0)) {
	if (quote)
	  sprintf(buff,"\"%s\"", shPtrSprint(address, shTypeGetFromName("STR")));
	else
	  sprintf(buff,"%s",shPtrSprint(address, shTypeGetFromName("STR")));
      } else
	sprintf(buff,PTR_FMT,*(void**)address);
      Tcl_DStringAppendElement( result, buff );

    } /* End of pointer analysis */
    else {

      /* We have a raw type */
      schema = shSchemaGet(sch_elm.type);
      if ( schema != NULL ) {
	switch ( schema->type )
	  {
	  case UNKNOWN:
	    shErrStackPush("shTclCreateValueList: Unknown type of schema: %s.",sch_elm.type);
	    return TCL_ERROR;
	  case ENUM:
	    if (noEnum)
	      Tcl_DStringAppendElement( result, shPtrSprint(address, type)+7);
	                       /* the '+7' bypass the word 'enum' in the string */
	    else
	      Tcl_DStringAppendElement( result, shPtrSprint(address, type) );
	    break;
	  case PRIM:
	    Tcl_DStringAppendElement( result, shPtrSprint(address, type) );
	    break;
	  case STRUCT:
	    if ( (level==0) || (recurse) ) {
	      if (level!=0) Tcl_DStringStartSublist( result);
	      for(i = 0;i < schema->nelem;i++)
		{
		  if (!nolabel) {
		    Tcl_DStringStartSublist( result);
		    Tcl_DStringAppendElement( result,schema->elems[i].name);
		  };
		  if((addr_elem = shElemGet(address,&schema->elems[i],&elem_type)) != NULL) {
		    if ( shTclCreateValueList( result, addr_elem, schema->elems[i], elem_type,
					       noEnum, recurse, unfold, nostring, quote, nolabel,
					       flat_array, limit, level+1) != TCL_OK )
		      return TCL_ERROR;
		  } else {
		    Tcl_DStringAppendElement( result, "N/A" );
		  }
		  if (!nolabel) Tcl_DStringEndSublist( result);
		}
	      if (level!=0) Tcl_DStringEndSublist( result);
	    } else {
	      Tcl_DStringAppendElement( result, "..." );
	    };
	    break;
	  default:
	    shErrStackPush("shTclCreateValueList: Unknown type of schema: %s.",sch_elm.type);
	    return TCL_ERROR;
	  } /* End of Switch */
      } else { /* Invalid Schema Type */
	shErrStackPush("shTclCreateValueList: Unknown type of schema: %s.",sch_elm.type);
	return TCL_ERROR;
      };
    }; /* End of raw type analysis */

  return TCL_OK;
}
/****************************************************************************
**
** ROUTINE: tclExprGet
**
**<AUTO EXTRACT>
** TCL VERB: exprGet
**
**</AUTO>
** Routines called:
**	shTclHandleExprEvalWithInfo (to evaluate the handle expression)
**      shTclCreateValueList (to create the list of value of the handle expression)
*/
char g_exprGet[] = "exprGet";
ftclArgvInfo g_exprGetTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Evaluate an handle expression and return the associated elements and values."},
  {"-recurse", FTCL_ARGV_CONSTANT, (void *)TRUE, NULL,
   "recursively returns the contents of internal struct"},
  {"-nostring", FTCL_ARGV_CONSTANT, (void *)TRUE, NULL,
   "return the address of strings (char*, char[]) rather than their contents"},
  {"-header", FTCL_ARGV_CONSTANT, (void *)TRUE, NULL,
   "Add an additionnal element containing the description of the handle itself (eg. {type REGION})"},
  {"-enum", FTCL_ARGV_CONSTANT, (void *)TRUE, NULL,
   "Do not remove the '(enum)' in front of enums values"},
  {"-unfold", FTCL_ARGV_CONSTANT, (void *)TRUE, NULL,
   "Add the content of the arrays rather than the starting address"},
  {"-flat", FTCL_ARGV_CONSTANT, (void *)TRUE, NULL,
   "Present the arrays in a flat fashion rather than a nested way"},
  {"-limited n", FTCL_ARGV_INT, NULL, NULL,
   "Same as -flat but limit the 'flatening' of an array to the last n dimensions. n==0 means flatenning all the dimensions."},
  {"-nolabel", FTCL_ARGV_CONSTANT, (void *)TRUE, NULL,
   "Do not add the intermediate label. I.e create a simple list (handleSet style) rather than a keyed list"},
  {"<handle expression>", FTCL_ARGV_STRING, NULL, NULL,
   "handle expression to evaluate"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};


static int
tclExprGet(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  int a_recurse = FALSE;
  int a_nostring = FALSE;
  int a_header = FALSE;
  int a_enum = FALSE;
  int a_unfold = FALSE;
  int a_flat = FALSE;
  int a_limit = 0;
  int a_nolabel = FALSE;
  char *a_handle = NULL;
  char *address;
  SCHEMA_ELEM sch_elm;
  int type;

  Tcl_DString result;
  char buff[BUFFER_SIZE];

  g_exprGetTbl[1].dst = &a_recurse;
  g_exprGetTbl[2].dst = &a_nostring;
  g_exprGetTbl[3].dst = &a_header;
  g_exprGetTbl[4].dst = &a_enum;
  g_exprGetTbl[5].dst = &a_unfold;
  g_exprGetTbl[6].dst = &a_flat;
  g_exprGetTbl[7].dst = &a_limit;
  g_exprGetTbl[8].dst = &a_nolabel;
  g_exprGetTbl[9].dst = &a_handle;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_exprGetTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_exprGet)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  /* If limit is not zero we force the flat flag */
  /* Note: if flat is true and limit is zero it means
     infinite flatenning ! */
  if (a_limit != 0) a_flat = TRUE;

  if ((rstat = shTclHandleExprEvalWithInfo(interp, a_handle, &address,
					   &sch_elm, &type)) != TCL_OK ) {
    return(rstat);
  };

  shDebug (1,"tclExprGet: recurse: %d, nostring %d, header %d, enum %d. arrays %d, expr %s", a_recurse,a_nostring,a_header,a_enum,a_unfold,a_handle);
  shDebug (1,"tclExprGet: address: %p, type %s, %d", address, sch_elm.type, type );

  /* If we have an handle to a NULL address we just return without doing anything */
  if ( address == NULL) return TCL_OK;

  Tcl_DStringInit( &result);

  if (a_header) {
    if (!a_nolabel) Tcl_DStringStartSublist( &result);
    if (!a_nolabel) Tcl_DStringAppendElement( &result, "type" );
    if ( (sch_elm.nstar != 0) || (sch_elm.nelem != NULL) )
      if (  (sch_elm.nstar <= 1) && (sch_elm.nelem != NULL)
	    && (shStrcmp(sch_elm.type,"CHAR",SH_CASE_INSENSITIVE)==0) )
	sprintf(buff,"%s","STR");
      else
	sprintf(buff,"%s","PTR");
    else
      sprintf(buff,"%s",sch_elm.type);
    Tcl_DStringAppendElement( &result, buff);
    if (!a_nolabel) Tcl_DStringEndSublist( &result);
  };

  if (shTclCreateValueList ( &result, address, sch_elm, type, a_enum,
			     a_recurse, a_unfold, a_nostring, 0, a_nolabel, 
			     a_flat, a_limit, 0) != TCL_OK)
    {
    shTclInterpAppendWithErrStack(interp);
    return TCL_ERROR;
  };

  Tcl_DStringResult(interp, &result);

  return TCL_OK;

}

/****************************************************************************
**
** ROUTINE: tclExprPrint
**
**<AUTO EXTRACT>
** TCL VERB: exprPrint
**
**</AUTO>
** Routines called:
**	shTclHandleExprEvalWithInfo (to evaluate the handle expression)
**      shTclCreateValueList (to create the list of value of the handle expression)
**
**      We did not want to create a shTclPrintValueList that would print out
**      instead of building a list.  For this reason when printing the info
**      when don't know they type.  This is a difficulty only for pointers,
**      We wish to convert any pointers to an eventual handle bind to it.
**      For this reason we assume that a value starting with 0x is the value
**      of a pointer.
*/
char g_exprPrint[] = "exprPrint";
ftclArgvInfo g_exprPrintTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Evaluate an handle expression and do a formatted printing of the associated elements and values.\n"},
  {"-recurse",  FTCL_ARGV_CONSTANT, (void *)TRUE, NULL,
   "recursively returns the contents of internal struct"},
  {"-nostring", FTCL_ARGV_CONSTANT, (void *)TRUE, NULL,
   "return the address of strings (char*, char[]) rather than their contents"},
  {"-header",   FTCL_ARGV_CONSTANT, (void *)TRUE, NULL,
   "Add an additionnal element containing the description of the handle itself (eg. {type REGION})"},
  {"-enum",     FTCL_ARGV_CONSTANT, (void *)TRUE, NULL,
   "Do not remove the '(enum)' in front of enums values"},
  {"-unfold",   FTCL_ARGV_CONSTANT, (void *)TRUE, NULL,
   "Add the content of the arrays rather than the starting address"},
  {"-flat",     FTCL_ARGV_CONSTANT, (void *)TRUE, NULL,
   "Present the arrays and (sub)structs in a flat fashion rather than a nested way"},
  {"-limited n", FTCL_ARGV_INT, NULL, NULL,
   "Same as -flat but limit the 'flatening' of an array to the last n dimensions. n==0 means flatenning all the dimensions."},
  {"-noname",   FTCL_ARGV_CONSTANT, (void *)TRUE, NULL,
   "Don't try to interpret address as handle"},
  {"-noquote",  FTCL_ARGV_CONSTANT, (void *)TRUE, NULL,
   "Don't put double quotes around strings!"},
  {"<handle expression>", FTCL_ARGV_STRING, NULL, NULL,
   "handle expression to evaluate"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};


/*
** Routine: returnValue
**
** This function are for sole purpose to convert an address to
** an handle name if this is necessary and requested (noname set
** to FALSE).
**
** The return value is the original string (value) if this was
** not an address or if this was address not bound to an handle.
** The return value is the new string (created at 'location')
** if we found an handle bound to this address.
*/
static char*
returnValue ( char *value, char* location, int noname) {

  char *addr;
  char *temp;
  HANDLE * hndl;
  int    walkKeyPtr = -1;		/* must be -1 for initial call to */

  if ( noname==FALSE && strncmp(value,"0x",2)==0) {
    addr = (char*) strtoll(value,NULL,0);
    /*
     * In order to get the handle name, we have to traverse the entire table,
     * looking for the address specified in the parameter a_handle. Once found,
     * we translate the address to a handle name using Tcl_WalkKeyToHandle().
     * If not found, then tough luck.
     */
    while((hndl = (HANDLE *)Tcl_HandleWalk(g_tableHeader,&walkKeyPtr)) != NULL){
      if(hndl->ptr == addr) {
	Tcl_WalkKeyToHandle(g_tableHeader, walkKeyPtr, location);
	temp = (char*) malloc((strlen(location)+1)*sizeof(char));
	strcpy ( temp, location);
	sprintf(location,"(handle)%s",temp);
	free(temp);
	return(location);
      }
    }
  };
  return(value);
}

/*
** Routine: printElement
**
** This functions receive the 'value' of
** an handle expression in the form of a keyed tcl list.
** The printing will be done using the format given by "fmt". "fmt" si expected to
** to print two strings (%s) the first being the name of the element, the second its
** value.
** If the value is complexe (struct or array), printElement
** recurse.
**
** We expecting value to be of one of the form:
**	a simple value (eg: 0x1029)
**	a simple value compose a several words
**                      (eg: This is a Label)
**	a list of value from a struct
**                      (eg: {name Region1} {nrow 10})
**	a list of value from an array
**                      (eg: {<0> 10} {<1> 11})
**
** In the last two cases we expect 'value' to be a tcl
** list of pair {name/array_number value}
** In the last two cases the first character of value
** should be a {.
** In case of an array the second character should be a <
**
** We should NEVER have more than two elements only in case of
** a string!
**
** We need the tcl interpreter to split the tcl list!
** noname is FALSE if you want to try to interpret address as handle
*/

static int
printElement( Tcl_Interp *interp, char*fmt, char *name, char *value, int noname )
{
  char buff[BUFFER_SIZE];
  int elemc, subc, index;
  char **elemPtr = NULL;
  char **subPtr = NULL;

  if ( Tcl_SplitList(interp, value, &elemc, &elemPtr) != TCL_OK ) {
    shErrStackPush("tclHandle.c:printElement: incorrect tcl list returned by shTclCreateValueList: %s",value);
    shTclInterpAppendWithErrStack(interp);
    return TCL_ERROR;
  };

  /* Note that the checking of value[0] is expected by shTclCreateValueList
     when returning 'flattened' arrays */

  if ( (elemc == 1) ||                /* We reached a simple primitive value
					 or an (absurde) struct or array with a
					 single elements ! */
       (value[0] != '{') 	      /* We reached most probably a multi-word
					 string (at least not a struct nor
					 an unfolded array) */
      ) {
    fprintf(stdout,fmt,name,returnValue(value,buff,noname));
  } else {

    /* We have an unfolded array or a recursed struct. */

    for (index = 0; index < elemc; index++) {

      if ( Tcl_SplitList(interp, elemPtr[index], &subc, &subPtr) != TCL_OK ) {
	shErrStackPush("tclHandle.c:printElement: incorrect tcl list returned by shTclCreateValueList: %s",
		       elemPtr[index]);
	shTclInterpAppendWithErrStack(interp);
	free(elemPtr);
	return TCL_ERROR;
      }
      if (subc != 2) {
	shErrStackPush("tclHandle.c:printElement: incorrect values returned by shTclCreateValueList: %s",
		       elemPtr[index]);
	shTclInterpAppendWithErrStack(interp);
	free(subPtr);
	free(elemPtr);
	return TCL_ERROR;
      }
      if ( (elemPtr[index][0] == '<') || (elemPtr[index][0] == '[') )
	sprintf(buff,"%s%s",name,subPtr[0]);
      else
	sprintf(buff,"%s.%s",name,subPtr[0]);     /* We have a struct! */
      printElement(interp, fmt, buff, subPtr[1], noname);
    };
    free (subPtr);
  }
  free(elemPtr);
  return TCL_OK;
}

static int
tclExprPrint(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  int a_recurse = FALSE;
  int a_nostring = FALSE;
  int a_header = FALSE;
  int a_enum = FALSE;
  int a_unfold = FALSE;
  int a_flat = FALSE;
  int a_noname = FALSE;
  int a_noquote = FALSE;
  int a_limit = 0;
  char *a_handle = NULL;
  char *address;
  SCHEMA_ELEM sch_elm;
  int type;

  Tcl_DString result;
  char buff[BUFFER_SIZE],elem_name[BUFFER_SIZE];
  int i, index, elemc, dimension_number;
  char **listPtr, **elemPtr, *temp;

  char *fHeader = "%-20s %s\n";
  char *fTitle = "(%s).%s";
  char *fLine = "%-20s %s\n";
  /*  char *fFlat = "%-20s %s";*/
  char *fSingle = "%s\n";

  g_exprPrintTbl[1].dst = &a_recurse;
  g_exprPrintTbl[2].dst = &a_nostring;
  g_exprPrintTbl[3].dst = &a_header;
  g_exprPrintTbl[4].dst = &a_enum;
  g_exprPrintTbl[5].dst = &a_unfold;
  g_exprPrintTbl[6].dst = &a_flat;
  g_exprPrintTbl[7].dst = &a_limit;
  g_exprPrintTbl[8].dst = &a_noname;
  g_exprPrintTbl[9].dst = &a_noquote;
  g_exprPrintTbl[10].dst= &a_handle;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_exprPrintTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_exprPrint)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (a_limit != 0) a_flat = TRUE;

  if ((rstat = shTclHandleExprEvalWithInfo(interp, a_handle, &address,
					   &sch_elm, &type)) != TCL_OK ) {
    shTclInterpAppendWithErrStack(interp);
    return(rstat);
  };

  shDebug (1,"tclExprPrint: recurse: %d, nostring %d, header %d, enum %d. arrays %d, expr %s",
	   a_recurse,a_nostring,a_header,a_enum,a_unfold,a_handle);
  shDebug (1,"tclExprPrint: address: %p, type %s, %d",
	   address, sch_elm.type, type );

  /* If we have an handle to a NULL address we just return without doing anything */
  if ( address == NULL) return TCL_OK;

  Tcl_DStringInit( &result);

  if (a_header) {
    if ( (sch_elm.nstar != 0) || (sch_elm.nelem != NULL) )
      if (  (sch_elm.nstar <= 1) && (sch_elm.nelem != NULL)
	    && (shStrcmp(sch_elm.type,"CHAR",SH_CASE_INSENSITIVE)==0) )
	sprintf(buff,"%s","STR");
      else
	sprintf(buff,"%s","PTR");
    else
      sprintf(buff,"%s",sch_elm.type);
    fprintf(stdout,fHeader,a_handle,buff);
  };

  if ( shTclCreateValueList ( &result, address, sch_elm, type, a_enum,
			      a_recurse, a_unfold, a_nostring, !a_noquote, 
			      0,      /* We need to have the labels */
			      a_flat, a_limit, 0) != TCL_OK)
    {
      shTclInterpAppendWithErrStack(interp);
      return TCL_ERROR;
    };


  if ( sch_elm.nelem!= NULL) {
    temp = sch_elm.nelem;
    dimension_number=0;
    while ( strtoul(temp, &temp, 10) != 0 ) {
      dimension_number ++;
    }
  };
  if ( sch_elm.nelem!=NULL && a_unfold && a_flat &&
       ( (a_limit==0)||(a_limit>=dimension_number) ) ) {

    /* result contain a flat description
       of the array pointed to by the handle
       expression */
    if (!a_header)
      fprintf(stdout,fSingle,Tcl_DStringValue( &result ));
    else {
	sprintf(elem_name,fTitle,a_handle,"");
	fprintf(stdout,fLine,a_handle,Tcl_DStringValue( &result ));
      };

  } else {

    if ( Tcl_SplitList(interp,Tcl_DStringValue( &result ), &argc, &listPtr) != TCL_OK ) {
      Tcl_DStringFree( &result );
      shErrStackPush("tclExprPrint: incorrect tcl list returned by shTclCreateValueList");
      shTclInterpAppendWithErrStack(interp);
      return TCL_ERROR;
    };

    for (index=0; index<argc; index++) {
      if (strlen(listPtr[index]) > 2) {
	  if ( Tcl_SplitList(interp,listPtr[index], &elemc, &elemPtr) != TCL_OK ) {
	    free( listPtr );
	    shErrStackPush("tclExprPrint: incorrect tcl list returned by shTclCreateValueList");
	    shTclInterpAppendWithErrStack(interp);
	    return TCL_ERROR;
	  };
      } else {
	/* There is no point in trying to split a string that has 2 or less characters !! */
	elemc = 1;
	elemPtr = NULL;  /* Since we dont use it */
      };
      shDebug(1,"%d",elemc);
      if (elemPtr !=NULL) for (i=0;i<elemc;i++) shDebug(1,"%s",elemPtr[i]);
      if ( (elemc == 1) || ( Tcl_DStringValue( &result )[0] != '{' ) || (elemc == 0) ) {

	/* Since there is only one element in listPtr[index] we have to use it here.
	   elemPtr[0] is an 'interpreted' version of listPtr[index]!
	   In particular the " " could be executed by tcl */

	if (!a_header)
	  fprintf(stdout,fSingle,returnValue(listPtr[index],buff,a_noname));
	else {
	  sprintf(elem_name,fTitle,a_handle,"");
	  fprintf(stdout,fLine,a_handle,returnValue(listPtr[index],buff,a_noname));
	}
      } else {
	sprintf(elem_name,fTitle,a_handle,elemPtr[0]);
	if ( printElement( interp, fLine, elem_name, elemPtr[1], a_noname) != TCL_OK ) {
	  free(elemPtr);
	  free(listPtr);
	  Tcl_DStringFree( &result);
	  shTclInterpAppendWithErrStack(interp);
	  return TCL_ERROR;
	};
      }
      if (elemPtr != NULL) free(elemPtr);
    }
    free(listPtr);
  };
  Tcl_DStringFree( &result);
  return TCL_OK;
}

/******************************************************************************
 *
 * Get a handle to new chain of types.
 */
int
shTclChainHandleNew (Tcl_Interp *interp,
	char *chainName,    /* OUT: handlename, like h0 */
	char *strType,	    /* IN: type of chain to make, e.g. "FOO" */
	CHAIN **chain	    /* OUT: return C struct */
	) {

   HANDLE chainHandle;


   if(p_shTclHandleNew(interp,chainName) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

/* set handle parameters; this is a chain of OBS things */

   *chain = shChainNew(strType);
   chainHandle.ptr = (void *)*chain;
   chainHandle.type = shTypeGetFromName("CHAIN");
   if(p_shTclHandleAddrBind(interp, chainHandle, chainName) != TCL_OK) {
      shChainDel (*chain);
      Tcl_SetResult(interp,"Can't bind to new chain handle",TCL_STATIC);
      return(TCL_ERROR);
   }
   return (TCL_OK);
}

/****************************************************************************/
int
shTclHandleNew (Tcl_Interp *interp,
	char *name,	/* OUT: name of handle created, like h0 */
	char *strType,	/* IN: Type of thing to make a handle for,
			like "FOO" */
	void *thing	/* IN: pointer to the thing to be bound to the handle.
			This must already have been created */
	) {

   HANDLE handle;
   TYPE type;
/*
 * Get a handle to a new type
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

/* set handle parameters */

   type = shTypeGetFromName(strType);
   handle.ptr = thing;
   handle.type = type;

   if(p_shTclHandleAddrBind(interp, handle, name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new handle",TCL_STATIC);
      return(TCL_ERROR);
   }
   return (TCL_OK);
}

/***************************************************************************/

/* Translate the handle to a CHAIN */
int
shTclChainAddrGetFromName(
  Tcl_Interp *interp,        /* IN:  Tcl Interpreter */
  char       *name,          /* IN:  Handle expression to translate */
  char       *strType,       /* IN:  CHAIN must be of this type */
  CHAIN     **ppChain)       /* OUT: Pointer to a pointer to the chain */
{
   HANDLE  userHandle,
          *handlePtr;
   void   *userPtr;
   TYPE    type;

   handlePtr = &userHandle;

   if (shTclHandleExprEval(interp, name, &userHandle, &userPtr) != TCL_OK)
       return TCL_ERROR;

   /*
    * Make sure that it is a chain we're handed
    */
   if (handlePtr->type != shTypeGetFromName("CHAIN"))  {
       Tcl_AppendResult(interp, "handle ", name, " is a ",
                        shNameGetFromType(handlePtr->type), " not a CHAIN",
                        (char *) NULL);
       return TCL_ERROR;
   }

   *ppChain = (CHAIN *)handlePtr->ptr;
   type     = shTypeGetFromName(strType);
   if ((*ppChain)->type != type)  {
        Tcl_AppendResult(interp, "handle ", name, " is a chain of ",
                         shNameGetFromType((*ppChain)->type), " not ",
                         shNameGetFromType(type), (char *)NULL);
        return TCL_ERROR;
   }

   return TCL_OK;
}


/***************************************************************************/
int
shTclAddrGetFromName (Tcl_Interp *interp,
	char *name,	/* IN: name of the handle to be translated, like h0 */
	void **address,	/* OUT: C-accessible structure */
	char *strType	/* IN: checks to make sure structure is of this
			   type, like "FOO" */
	) {

   HANDLE userHandle;
   void *userPtr;
   HANDLE *handlePtr = &userHandle;
   TYPE type;

/* Get pointer to address */

   if (shTclHandleExprEval(interp, name, &userHandle, &userPtr) !=
	TCL_OK) return (TCL_ERROR);


/* Check to make sure this is a type object */

   type = shTypeGetFromName(strType);
   if(handlePtr->type != type) {
      Tcl_AppendResult(interp,"handle " ,name, " is a ",
                       shNameGetFromType(handlePtr->type)," not a ",
			shNameGetFromType(type),
                       (char *)NULL);
      return(TCL_ERROR);
   }

/* Pass back the pointer to the object as a void pointer */
   *address = handlePtr->ptr;

   return (TCL_OK);
}
/****************************************************************************/
int
shTclHandleGetFromName (Tcl_Interp *interp,
	char *name,	/* IN: Name of handle to be translated, like h0 */
	HANDLE *handle	/* OUT: Pointer to the handle - the structure is
			*handle->ptr, and the type is handle->type.  This
			is not normally used in coding - it can be used in
			error situations if you want to know what type the
			structure is anyway, if you just found out it is
			not a FOO like you thought. */
	) {

   HANDLE *handlePtr;

/* Get pointer to address */

      if(p_shTclHandleAddrGet(interp, name, &handlePtr) != TCL_OK) {
         return(TCL_ERROR);
   }
/* Caution!!! handlePtr contains volatile information upon return - i.e.,
** it points to a location in the handle table that can get rearranged if
** new handles are created (see XTCL documentation on handles).  So we
** will copy the contents to local storage */

   handle->type = handlePtr->type;
   handle->ptr  = handlePtr->ptr;

   return (TCL_OK);
}

/******************************************************************************
<AUTO EXTRACT>
 *
 *  ROUTINE: shTclHandleSet
 *
 *  DESCRIPTION:
 *    Set what is described by sch_elm and at 'address' with the content
 *    of values.
 *    If the object is of primitive type we expect a single value in 'values'
 *    If the object is 'complex' with expect a tcl list with the ordered new
 *    values.
 *
 *  RETURN VALUES:
 *    TCL_OK in case of sucess
 *    TCL_ERROR otherwise.
 *    We use the dervish error stack to store error messages.
 *
</AUTO> */
/*
 *  NOTE:
 *    shTclHandleEvalWithInfo store information in sch_elm (cf tclHandlSet () )
 *    One assumption with have made in shTclCreateValueList is that the offset
 *    is 0 (which is true after shTclHandleEvalWithInfo). So shTclCreateValueList
 *    never uses the offset field and assume that the address passed is the
 *    address of the object.
 *    However there is one case where the offset is not 0. It is when we get
 *    the sch_elm from the schema description of a struct.
 *    In shTclCreateValueList this is not a problem since we use shElemGet to
 *    calculate the address of the object for us.
 *    However shTclhandleSet is using shElemSet which takes in consideration the
 *    offset field of sch_elm! this means that shElemSet expect the address of
 *    the full struct and not the address of the individual object!!
 *
 *    Conclusion: we INFORCE that sch_elm.offset is always 0 !!
 *                we DEMAND address to be the exact address of the object !!
 */

int
shTclHandleSet ( Tcl_Interp *interp, char* address, SCHEMA_ELEM sch_elm, char* values )
{
  int i,elemc,elem_type;
  char **elemPtr;
  const SCHEMA *schema;
  char *addr_elem, *temp;
  int size,sub_size,offset;

#if SCHEMA_ELEMS_HAS_TTYPE
  sch_elm.ttype = UNKNOWN_SCHEMA;	/* we may change the schema, so
					   disable cached TYPE */
#endif

  shDebug ( 1 ,"setting: %s at "PTR_FMT" with %s", sch_elm.type,(void*)address,values);
  shDebug ( 1 ,"nstar: %d, nelem "PTR_FMT,sch_elm.nstar,sch_elm.nelem);

  /* This is a work around an inconstency between shTclHandleSet and shElemSet */

  sch_elm.offset = 0;

  /* We are expecting sch_elm.nelem to be either NULL either a pointer to string containing
     a list of the size of each dimension of the array separed by a 'white space'.
   */

   if (sch_elm.nelem != NULL) {

     /* We have an array */
     size = strtoul ( sch_elm.nelem, &temp, 10);    /* Size of the first dimension of the array */
     if ( temp == sch_elm.nelem ) {
       shErrStackPush("shTclHandleSet: No valid dimension found");
       return TCL_ERROR;			   /* No valid dimension found */
     };
     if ((sch_elm.nstar==0)&&
	 (shStrcmp(sch_elm.type,"CHAR",SH_CASE_INSENSITIVE)==0)&&
	 (strtoul(temp,NULL,10) == 0)) {
       if ( strlen(address) <= size ) {    /* Print the string only if it is contained
                                              within the limit of the array ! */
	 strcpy(address,values);
       }
       else {
	 shErrStackPush("shTclHandleSet: Not enough space in character arrays");
	 return TCL_ERROR;
       };
     } /* End of analysis of arrays as String. */
     else {
       if ( Tcl_SplitList(interp, values, &elemc, &elemPtr) != TCL_OK ) {
	 shErrStackPush("shTclHandleSet: incorrect tcl list");
	 return TCL_ERROR;
       }
       if ( elemc != size ) {
	 shErrStackPush("shTclHandleSet: You must specify all the values of an array");
	 free(elemPtr);
	 return TCL_ERROR;
       }

       /* Calculate the distance between elements. */

       offset = 1;
       if ( strtoul ( temp, NULL, 10 ) == 0 ) {
	 sch_elm.nelem = NULL;            /* We exhausted all the dimension */
       } else {
	 sch_elm.nelem = temp;            /* Set for recursion */
	 while ( (sub_size=strtoul( temp, &temp, 10)) != 0 ) {
	   offset = offset * sub_size;
	 }
       }
       offset = offset * ((sch_elm.nstar == 0) ? sch_elm.size : sizeof(void *));

       for ( i=0, addr_elem = address; i<size; i++, addr_elem += offset ) {
	 if ( shTclHandleSet (interp, addr_elem,
			      sch_elm, elemPtr[i]) != TCL_OK ) {
	   free(elemPtr);
	   return TCL_ERROR;
	 };
       }; /* End of loop of arrays element */
       free(elemPtr);
     }; /* End of common analysis of Array */
   } /* End of array analysis */

   else if (sch_elm.nstar != 0) {

      /* We have a pointer */

	if (shElemSet(address, &sch_elm, values) != SH_SUCCESS) {
	  return(TCL_ERROR);
	};

   } /* End of pointer analysis */
    else {

      /* We have a raw type */

      schema = shSchemaGet(sch_elm.type);
      switch ( schema->type )
	{
	case UNKNOWN:
	  break;
	case ENUM:
	case PRIM:
	  if(shElemSet(address, &sch_elm, values) != SH_SUCCESS) {
	    return(TCL_ERROR);
	  }
	  break;
	case STRUCT:
	  if ( Tcl_SplitList(interp, values, &elemc, &elemPtr) != TCL_OK ) {
	    shErrStackPush("shTclHandleSet: incorrect tcl list");
	    return TCL_ERROR;
	  }
	  if ( elemc != schema->nelem ) {
	    shErrStackPush("shTclHandleSet: You must specify all the values of a %s",
			   schema->name);
	    free(elemPtr);
	    return TCL_ERROR;
	  }
	  for (i=0;i<schema->nelem;i++) {
	    if((addr_elem = shElemGet(address,&schema->elems[i],&elem_type)) == NULL) {
	      continue;
	    }
	    if ( shTclHandleSet (interp, addr_elem,
				 schema->elems[i], elemPtr[i]) != TCL_OK ) {
	      free(elemPtr);
	      return TCL_ERROR;
	    };
	  };
	  free(elemPtr);
	  break;
	default:
          shErrStackPush("I don't understand the type: %s ", sch_elm.type);
	  return TCL_ERROR;
	}; /* End of Switch */
    }; /* End of raw type analysis */

  return TCL_OK;
}
/*****************************************************************************/

static char *tclHandleSet_use =
  "USAGE: handleSet handle value";
static char *tclHandleSet_hlp =
  "Set what a <handle> points to to a <value>.";

static int
tclHandleSet(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
   char *address;
   SCHEMA_ELEM sch_elm;
   int type;

   int rstatus;

   if (argc != 3) {
      Tcl_SetResult(interp,tclHandleSet_use,TCL_STATIC);
      return(TCL_ERROR);
   }

/* Decode the handle expression */
   if ((rstatus = shTclHandleExprEvalWithInfo(interp, argv[1], &address,
					      &sch_elm, &type)) != TCL_OK ) {
     return(rstatus);
   };

/*   if (shTclHandleExprEval(interp, argv[1], &userHandle, &userPtr) !=
	TCL_OK) return (TCL_ERROR);
*/
   if(address == NULL) {
      Tcl_SetResult(interp,"Attempt to dereference a NULL handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   if (shTclHandleSet( interp, address, sch_elm, argv[2]) == TCL_OK) {
     return TCL_OK;
   }

   shTclInterpAppendWithErrStack(interp);
   Tcl_AppendResult(interp,"Can't set ",argv[1]," to ",
	   argv[2],"\n",(char *)NULL);
   return(TCL_ERROR);
}

/*****************************************************************************/

static char *tclHandleSetFromHandle_use =
  "USAGE: handleSetFromHandle handle1 handle2";
static char *tclHandleSetFromHandle_hlp =
  "Copy contents of handle2 to handle1";

static int
tclHandleSetFromHandle(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
   void *userPtr1, *userPtr2;
   HANDLE hand1;
   HANDLE hand2;
   char *type_str;
   const SCHEMA *sch;

   if (argc != 3) {
      Tcl_SetResult(interp,tclHandleSetFromHandle_use,TCL_STATIC);
      return(TCL_ERROR);
   }

/* Decode the handle expressions */
/* Must store the handle explicitly since RHL insists on returning the
** result in volatile storage.  **/
   if (shTclHandleExprEval(interp, argv[1], &hand1, &userPtr1) !=
	TCL_OK) return (TCL_ERROR);
   if(hand1.ptr == NULL) {
      Tcl_AppendResult(interp, "Attempt to dereference a NULL handle",
	argv[1], (char *)NULL);
      return(TCL_ERROR);
   }
   if (shTclHandleExprEval(interp, argv[2], &hand2, &userPtr2) !=
	TCL_OK) return (TCL_ERROR);
   if(hand2.ptr == NULL) {
      Tcl_AppendResult(interp, "Attempt to dereference a NULL handle",
	argv[2], (char *)NULL);
      return(TCL_ERROR);
   }

   if (hand1.type != hand2.type) {
      Tcl_SetResult(interp,"Handles do not have same type",TCL_VOLATILE);
      return (TCL_ERROR);
   }

/* Get the schema for the handle */
   type_str = shNameGetFromType(hand1.type);
   if((sch = (SCHEMA *)shSchemaGet(type_str)) == NULL) {
      Tcl_AppendResult(interp,"Cant't retrieve schema for ",type_str,
	(char *)NULL);
      return(TCL_ERROR);
   }

/* Use memmove */
   memmove((char *)hand1.ptr, (const char *)hand2.ptr, (size_t)sch->size);
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclHandleList_use =
  "USAGE: handleList";
static char *tclHandleList_hlp =
  "Return a list of all handles and their types and addresses";

static int tclHandleList(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
		)
{
   HANDLE *hndl;
   int walkKey = -1;
   char handleName[80];
   char *typeName;
   char ptrVal[160];

   while ((hndl = (HANDLE *)Tcl_HandleWalk (g_tableHeader, &walkKey)) != NULL)
	{
/* Get handle name */
        Tcl_WalkKeyToHandle (g_tableHeader, walkKey, handleName);
/* Get handle type */
	typeName = shNameGetFromType (hndl->type);
/* Get handle pointer and format entire string */
	sprintf (ptrVal, "%s %s "PTR_FMT, handleName, typeName, hndl->ptr);
	Tcl_AppendElement (interp, ptrVal);
	}
   return TCL_OK;
}

/*
 * NAME: shHandleWalk()
 *
 * CALL: (HANDLE *) shHandleWalk(void);
 *
 * DESCRIPTION:
 *   shHandleWalk() traverses through all the handles being maintained by
 *   Dervish and returns a pointer to each in turn. On reaching the end of the
 *   handle list, a NULL is returned. Re-traversal is still possible after
 *   receiving a NULL. The routine will wrap around and get the first handle
 *   in that case.
 *
 * RETURNS:
 *   Pointer to a HANDLE structure : on success
 *   NULL : on reaching the end of the handle list
 */
HANDLE *shHandleWalk(void)
{
   HANDLE *pHandle;
   static int walkKey = -1;   /* Need to save between calls, hence static */

   pHandle = (HANDLE *)Tcl_HandleWalk(g_tableHeader, &walkKey);

   if (pHandle == NULL)  {
       walkKey = -1 ;  /* Get ready for re-walking */
   }

   return pHandle;
}


/*****************************************************************************/
/*
 * Get/set precision for exprGet
 */
static char *tclExprGetSetPrecision_use =
  "USAGE: exprGetSetPrecision [-double] ndigit";
#define tclExprGetSetPrecision_hlp \
  "Set the precision for exprGet, returning the old value"

static ftclArgvInfo tclExprGetSetPrecision_Tbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL, tclExprGetSetPrecision_hlp },
  {"<ndigit>", FTCL_ARGV_INT, NULL, NULL, "Desired number of digits"},
  {"-double", FTCL_ARGV_CONSTANT, (void *)FALSE, NULL,
     "Set the double (not float) precision"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclExprGetSetPrecision(ClientData clientData,
		       Tcl_Interp *interp,
		       int argc,
		       char **argv)
{
   char buff[BUFFER_SIZE];
   int ndigit = 0;
   int rstat;
   int set_float = TRUE;
   
   tclExprGetSetPrecision_Tbl[1].dst = &ndigit;
   tclExprGetSetPrecision_Tbl[2].dst = &set_float;
   
   if((rstat = shTclParseArgv(interp, &argc, argv, tclExprGetSetPrecision_Tbl,
			      FTCL_ARGV_NO_LEFTOVERS,
			      "exprGetSetPrecision")) != FTCL_ARGV_SUCCESS) {
      return(rstat);
   }
/*
 * Work
 */
   ndigit = shExprGetSetPrecision(ndigit, set_float);
/*
 * Return old precision
 */
   sprintf(buff, "%d", ndigit);
   Tcl_SetResult(interp, buff, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Declare verbs to tcl
 */
void
shTclHandleDeclare(Tcl_Interp *interp)
{
   int flags = FTCL_ARGV_NO_LEFTOVERS;

   shTclDeclare(interp,"handleNew",
		(Tcl_CmdProc *)tclHandleNew,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL,
		module, tclHandleNew_hlp, tclHandleNew_use);

   shTclDeclare(interp,"handleBind",
		(Tcl_CmdProc *)tclHandleBind,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL,
		module, tclHandleBind_hlp, tclHandleBind_use);

   shTclDeclare(interp,"handleBindFromHandle",
		(Tcl_CmdProc *)tclHandleBindFromHandle,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL,
		module, tclHandleBindFromHandle_hlp,
		  tclHandleBindFromHandle_use);

   shTclDeclare(interp,"handleDel",
		(Tcl_CmdProc *)tclHandleDel,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL,
		module, tclHandleDel_hlp, tclHandleDel_use);

   shTclDeclare(interp,"handleNameGet",
		(Tcl_CmdProc *)tclHandleNameGet,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL,
		module, tclHandleNameGet_hlp, tclHandleNameGet_use);

   shTclDeclare(interp,"handlePrint",
		(Tcl_CmdProc *)tclHandlePrint,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL,
		module, tclHandlePrint_hlp, tclHandlePrint_use);

   shTclDeclare(interp,"handleGet",
		(Tcl_CmdProc *)tclHandleGet,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL,
		module, tclHandleGet_hlp, tclHandleGet_use);

   shTclDeclare(interp,"handleDeref",
		(Tcl_CmdProc *)tclHandleDeref,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL,
		module, tclHandleDeref_hlp, tclHandleDeref_use);

   shTclDeclare(interp,"handleDerefAndSet",
		(Tcl_CmdProc *)tclHandleDerefAndSet,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL,
		module, tclHandleDerefAndSet_hlp, tclHandleDerefAndSet_use);

   shTclDeclare(interp,"handleExprEval",
		(Tcl_CmdProc *)tclHandleExprEval,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL,
		module, tclHandleExprEval_hlp, tclHandleExprEval_use);

  shTclDeclare(interp, g_exprGet,
               (Tcl_CmdProc *) tclExprGet,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, module,
               shTclGetArgInfo(interp, g_exprGetTbl, flags, g_exprGet),
               shTclGetUsage(interp, g_exprGetTbl, flags, g_exprGet));

  shTclDeclare(interp, g_exprPrint,
               (Tcl_CmdProc *) tclExprPrint,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, module,
               shTclGetArgInfo(interp, g_exprPrintTbl, flags, g_exprPrint),
               shTclGetUsage(interp, g_exprPrintTbl, flags, g_exprPrint));

   shTclDeclare(interp,"handleSet",
		(Tcl_CmdProc *)tclHandleSet,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL,
		module, tclHandleSet_hlp, tclHandleSet_use);

   shTclDeclare(interp,"handleSetFromHandle",
		(Tcl_CmdProc *)tclHandleSetFromHandle,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL,
		module, tclHandleSetFromHandle_hlp, tclHandleSetFromHandle_use);

   shTclDeclare(interp,"handleList",
		(Tcl_CmdProc *)tclHandleList,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL,
		module, tclHandleList_hlp, tclHandleList_use);

   shTclDeclare(interp,"exprGetSetPrecision",
		(Tcl_CmdProc *)tclExprGetSetPrecision,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL,
		module, tclExprGetSetPrecision_hlp,
		tclExprGetSetPrecision_use);
}
