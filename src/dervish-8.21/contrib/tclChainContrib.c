/* <AUTO>
   FILE:
  	tclChainContrib.c
   <HTML> 
 * The following files are contributions made by Steve to Dervish Chains. They
 * have been moved from his ts package and put in $DERVISH_DIR/contrib. They
 * have also been dervish'ized in that the function names have been changed to
 * reflect the Dervish conventions. However, the one major drawback is that
 * not all TCL extensions have the appropriate C API. 
 *
 * Files are : $DERVISH_DIR/contrib/shChainContrib.c
 *             $DERVISH_DIR/contrib/tclChainContrib.c
 *             $DERVISH_DIR/contrib/shChainUtils.c
 *             $DERVISH_DIR/contrib/shChainContrib.h
 *
 * - Vijay K. Gurbani
 *   Feb. 08 1995
 *
 * MODIFICATIONS:
 * 
 *   Added table based parsing                                 Vijay  02/09/95
 * </HTML>
 * </AUTO>
 */
/************************************************************************
**
**	This file contains many routines to manipulate chains and objects
**	on chains.  Most routines allow one to operate on elements of
**	objects in chains.  The elements can be accessed using the full
**	power of handle expressions. 
**
** ENTRY POINT		SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
** tclChainSearch	local   Do a search on an unsorted chain.  Return a new chain with the selected
**				objects only.  A compound predicate can be input.
** tclChainSelect	local   Do a select on an unsorted chain.  Return a new chain with the selected
**				objects only.  A simple predicate only can be input.  Input chain is
**				sorted as a by-product.
** tclChainSort		local   Sort a chain. (Faster version)
** tclChainMatch
** tclChainSet
** tclChainSetFromHandle
** tclChainExtract
** tclChainTransXY
** tclChainTransZXY
** tclChainScale
*/

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "dervish.h"
#include "shTclParseArgv.h"
#include "shTclHandle.h"
#include "shChain.h"
#include "shChainContrib.h"
#include "shChainMerge.h"

/*****************************************************************************/
/*************************** tclChainSearch  **********************************/
/*****************************************************************************/
/*
 * Do a search on an unsorted chain.  Return a new chain with the selected
 * objects only.  A compound predicate can be input.
 */

static char *g_chainSearch = "chainSearch";
static ftclArgvInfo g_chainSearchTbl[] = {
  { NULL, FTCL_ARGV_HELP, NULL, NULL, 
    "Search on an unsorted chain; return a new chain with selected items" },
  { "<CHAIN>", FTCL_ARGV_STRING, NULL, NULL, "Handle bound to chain" },
  { "<PREDICATE>", FTCL_ARGV_STRING, NULL, NULL, "Search condition" },
  { NULL, FTCL_ARGV_END, NULL, NULL, NULL} 
};

static int
tclChainSearch (
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{
   CHAIN *chain = NULL, *outchain = NULL;
   char *pChainName = NULL, *pPredicate = NULL;
   TYPE type = 0;
   char chainName[HANDLE_NAMELEN];
   int  retStatus;

   pChainName = pPredicate = NULL;
   g_chainSearchTbl[1].dst = &pChainName;
   g_chainSearchTbl[2].dst = &pPredicate;

   if ((retStatus = shTclParseArgv(interp, &argc, argv, g_chainSearchTbl, 
                  FTCL_ARGV_NO_LEFTOVERS, g_chainSearch)) != FTCL_ARGV_SUCCESS)
        return retStatus;
   
   if (shTclAddrGetFromName(interp, pChainName, (void *)&chain, "CHAIN") !=
                                                                        TCL_OK)
	return TCL_ERROR;

/* Make type of output chain the same as the input type.  */
   type = shChainTypeDefine(chain);
/* Make a new handle to a CHAIN */
   if (shTclChainHandleNew (interp, chainName, shNameGetFromType(type),
	&outchain) != TCL_OK) return(TCL_ERROR);

   if (shTclChainSearch (interp, chain, pPredicate, outchain) != TCL_OK)
	return TCL_ERROR;

   Tcl_SetResult (interp, chainName, TCL_VOLATILE);
   return TCL_OK;
}

/*****************************************************************************/
/*************************** tclChainSelect **********************************/
/*****************************************************************************/
/*
 * Do a select on an unsorted chain.  Return a new chain with the selected
 * objects only.  A simple predicate only can be input.  Input chain is
 * sorted as a by-product.
 */
static char *g_chainSelect = "chainSelect";
static ftclArgvInfo g_chainSelectTbl[] = {
  { NULL, FTCL_ARGV_HELP, NULL, NULL, 
    "Select on an sorted chain; return a new chain with selected items" },
  { "<CHAIN>", FTCL_ARGV_STRING, NULL, NULL, "Handle bound to chain" },
  { "<PREDICATE>", FTCL_ARGV_STRING, NULL, NULL, "Selection predicate" }, 
  { NULL, FTCL_ARGV_END, NULL, NULL, NULL }
};

static int
tclChainSelect (
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{
   CHAIN *chain = NULL, *outchain = NULL;
   TYPE   type = 0;
   char  *pChainName = NULL, *pPredicate = NULL;
   int    retStatus;
   char   chainName[HANDLE_NAMELEN];

   pChainName = pPredicate = NULL;
   g_chainSelectTbl[1].dst = &pChainName;
   g_chainSelectTbl[2].dst = &pPredicate;

   if ((retStatus = shTclParseArgv(interp, &argc, argv, g_chainSelectTbl,
                  FTCL_ARGV_NO_LEFTOVERS, g_chainSelect)) != FTCL_ARGV_SUCCESS)
        return retStatus;

   if (shTclAddrGetFromName(interp, pChainName, (void *)&chain, "CHAIN") 
                                                                     != TCL_OK)
	return TCL_ERROR;

/* Make type of output chain the same as the input type.  */
   type = shChainTypeDefine(chain);
/* Make a new handle to a CHAIN */
   if (shTclChainHandleNew (interp, chainName, shNameGetFromType(type),
	&outchain) != TCL_OK) return(TCL_ERROR);

   if (shTclChainSelect (interp, chain, pPredicate, outchain) != TCL_OK)
	return TCL_ERROR;

   Tcl_SetResult (interp, chainName, TCL_VOLATILE);
   return TCL_OK;
}

/*****************************************************************************/
/*************************** tclChainSort **********************************/
/*****************************************************************************/
/*
 * Sort a chain. (Faster version)
 */

static char *g_chainFsort = "chainFSort";
static ftclArgvInfo g_chainFsortTbl[] = {
    {NULL, FTCL_ARGV_HELP, NULL, NULL, "Sort a chain (faster version)"},
    {"<CHAIN>",  FTCL_ARGV_STRING, NULL, NULL, "Handle to the chain"},
    {"<ATTRIBUTE>", FTCL_ARGV_STRING, NULL, NULL, "Attribute to sort on"},
    {"-reverse", FTCL_ARGV_CONSTANT, (void *)1, NULL, "If specified,"
                 " sort from ascending to descending"},
    {NULL,       FTCL_ARGV_END, NULL, NULL, NULL}
   };

static int
tclChainSort (
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{
   CHAIN *chain = NULL;
   char *chainName = NULL;
   char *attrName = NULL;
   int reverse = 0,
       flags = FTCL_ARGV_NO_LEFTOVERS,
       retStatus;

   g_chainFsortTbl[1].dst = &chainName;
   g_chainFsortTbl[2].dst = &attrName;
   g_chainFsortTbl[3].dst = &reverse;


   if ((retStatus = shTclParseArgv(interp, &argc, argv, g_chainFsortTbl, flags,
                         g_chainFsort)) != FTCL_ARGV_SUCCESS)
        return retStatus;

   if (shTclAddrGetFromName(interp, chainName, (void *)&chain, "CHAIN") != 
                                                                        TCL_OK)
	return TCL_ERROR;

   if (shTclChainSort (interp, chain, attrName, reverse) != TCL_OK) {
	Tcl_AppendResult (interp, " Failure while sorting chain", NULL);
	return TCL_ERROR;
	}

   Tcl_SetResult (interp, chainName, TCL_VOLATILE);
   return TCL_OK;
}

/*****************************************************************************/
/*************************** tclChainMatch ***********************************/
/*****************************************************************************/
/*
** ROUTINE: tclChainMatch
**
**<AUTO EXTRACT>
** TCL VERB: chainMatch
**
**</AUTO>
 * 
 * Match up 2 chains.  Chains can be different.  Arbitrary number of arguments
 * to match up allowed.  Can be of different types.
 */
static char *g_chainMatch = "chainMatch";
static ftclArgvInfo g_chainMatchTbl[] = {
    {NULL, FTCL_ARGV_HELP, NULL, NULL,
     "Match 2 chains based on arbitary set of arguments, each set with"
     "own window"},
    {"<CHAIN1>",  FTCL_ARGV_STRING, NULL, NULL, "Handle of first chain"},
    {"<CHAIN2>",  FTCL_ARGV_STRING, NULL, NULL, "Handle of second chain"},
    {"<PREDICATE>", FTCL_ARGV_STRING, NULL, NULL, 
                    "List of matching conditions of the form: "
		    "{chain1arg_i  chain2arg_i window [wrap]} "
		    "wrap is optional and allows for wrap-around (for angular"
		    "coordinates * such as RA that wrap around at 12 hrs or 360"
		    "degrees)"},
    {"-type",     FTCL_ARGV_STRING, NULL, NULL, "Type of the chains" },
    {"-nosort",   FTCL_ARGV_CONSTANT, (void *)1, NULL,
                    "Do not sort the second chain" },
    {NULL,        FTCL_ARGV_END, NULL, NULL, NULL}
   };


static int
tclChainMatch (
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{
   static char *chain1Name;
   static char *chain2Name;
   static char *pred;
   static char *typeName;
   int retStatus;
   int noSort = 0;                    /* option to not sort the 2nd chain */

   CHAIN *chain[2], *outchain;
   char chainName[HANDLE_NAMELEN];

   /* These must be initialized every time here because the variables are
      declared static and will only be automatically initialized once. */
   chain1Name = NULL;
   chain2Name = NULL;
   pred = NULL;
   typeName = NULL;

   g_chainMatchTbl[1].dst = &chain1Name;
   g_chainMatchTbl[2].dst = &chain2Name;
   g_chainMatchTbl[3].dst = &pred;
   g_chainMatchTbl[4].dst = &typeName;
   g_chainMatchTbl[5].dst = &noSort;

/* Reset the optional parameter */
   typeName = NULL;
   if ((retStatus = shTclParseArgv(interp, &argc, argv, g_chainMatchTbl,
                      0, g_chainMatch)) != FTCL_ARGV_SUCCESS)
        return retStatus;

   if (shTclAddrGetFromName(interp, chain1Name, (void *)&chain[0], "CHAIN") !=
                                                                        TCL_OK)
	return TCL_ERROR;

   if (shTclAddrGetFromName(interp, chain2Name, (void *)&chain[1], "CHAIN") !=
                                                                        TCL_OK)
	return TCL_ERROR;

   if (typeName == NULL) typeName = "SHMERGE";
/* Make a new handle to a CHAIN */
   if (shTclChainHandleNew (interp, chainName, typeName,
	&outchain) != TCL_OK) return(TCL_ERROR);
   if (shTclChainMatch (interp, chain, pred, outchain, typeName, noSort)
       != TCL_OK)
	return TCL_ERROR;

   Tcl_SetResult (interp, chainName, TCL_VOLATILE);
   return TCL_OK;
}

/*****************************************************************************/
/*************************** tclChainSet **********************************/
/*****************************************************************************/
/*
 * Set a parameter in a chain.
 */
static char *g_chainSet = "chainSet";
static ftclArgvInfo g_chainSetTbl[] = {
  { NULL, 	FTCL_ARGV_HELP,	     NULL, NULL, "Set an attribute in a chain" },
  { "<CHAIN>",	FTCL_ARGV_STRING,    NULL, NULL, "Handle to the chain" },
  { "<ATTRIBUTE>", FTCL_ARGV_STRING, NULL, NULL, "Attribute name to set" },
  { "<VALUE>",	FTCL_ARGV_STRING,    NULL, NULL, "Attribute value" },
  { NULL,	FTCL_ARGV_END,	     NULL, NULL, NULL }
};

static int
tclChainSet (
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{
   CHAIN *chain = NULL;
   char  *pChainName = NULL, *pAttrib = NULL, *pValue = NULL;
   int    retStatus;

   pChainName = pAttrib = pValue = NULL;

   g_chainSetTbl[1].dst = &pChainName;
   g_chainSetTbl[2].dst = &pAttrib;
   g_chainSetTbl[3].dst = &pValue;

   if ((retStatus = shTclParseArgv(interp, &argc, argv, g_chainSetTbl, 0,
                      g_chainSet)) != FTCL_ARGV_SUCCESS)
        return retStatus;
   if (shTclAddrGetFromName(interp, pChainName, (void *)&chain, "CHAIN") != 
                                                                        TCL_OK)
	return TCL_ERROR;
   if (shTclChainSet (interp, chain, pAttrib, pValue) != TCL_OK) {
	Tcl_AppendResult (interp, " Failure while setting chain", NULL);
	return TCL_ERROR;
	}

   return TCL_OK;
}

/*****************************************************************************/
/*************************** tclChainSetFromHandle ***************************/
/*****************************************************************************/
/*
 * Set a parameter in a chain with the input coming via a handle
 */
static char *g_chainSetFromHandle = "chainSetFromHandle";
static ftclArgvInfo g_chainSetFromHandleTbl[] = {
  { NULL, FTCL_ARGV_HELP, NULL, NULL, 
    "Set a specified element in all objects on a chain to a specified value."
    " The input is given by a handle" },
  { "<CHAIN>", FTCL_ARGV_STRING, NULL, NULL, "Handle to the chain" },
  { "<ARGUMENT>", FTCL_ARGV_STRING, NULL, NULL, "Argument to set" },
  { "<VALUE>", FTCL_ARGV_STRING, NULL, NULL, 
               "Handle to the value to which argument is set" },
  { NULL, FTCL_ARGV_END, NULL, NULL, NULL }
};

static int
tclChainSetFromHandle (
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{
   CHAIN  *chain = NULL;
   HANDLE  handle;
   void   *userPtr = NULL;
   char   *pChainName = NULL;
   char   *pArgument = NULL;
   char   *pValue = NULL;
   int     retStatus;

   pChainName = pArgument = pValue = NULL;

   g_chainSetFromHandleTbl[1].dst = &pChainName;
   g_chainSetFromHandleTbl[2].dst = &pArgument;
   g_chainSetFromHandleTbl[3].dst = &pValue;

   if ((retStatus = shTclParseArgv(interp, &argc, argv, 
      g_chainSetFromHandleTbl, 0, g_chainSetFromHandle)) != FTCL_ARGV_SUCCESS)
      return retStatus;
          
   if (shTclAddrGetFromName(interp, pChainName, (void *)&chain, "CHAIN") !=
                                                                        TCL_OK)
	return TCL_ERROR;

/* Decode the value handle expression */

   if (shTclHandleExprEval(interp, pValue, &handle, &userPtr) !=
        TCL_OK) return (TCL_ERROR);
   if (handle.ptr == NULL) {
	Tcl_AppendResult(interp, "Attempt to dereference a NULL handle",
	pValue, (char *)NULL);
	return(TCL_ERROR);
	}

   if (shTclChainSetFromHandle (interp, chain, pArgument, handle.ptr,
	handle.type) != TCL_OK) {
	Tcl_AppendResult (interp, " Failure while setting chain", NULL);
	return TCL_ERROR;
	}

   return TCL_OK;
}

/*****************************************************************************/
/*************************** tclChainExtract **********************************/
/*****************************************************************************/
/*
 * Extract a sub-chain from a chain.  Kind of like the inverse of chainJoin.
 */
static char *g_chainExtract = "chainExtract";
static ftclArgvInfo g_chainExtractTbl[] = {
  { NULL, FTCL_ARGV_HELP, NULL, NULL, 
    "Extract a portion of a chain as specified by starting index and size" },
  { "<CHAIN>", FTCL_ARGV_STRING, NULL, NULL, "Handle to the chain" },
  { "<START>", FTCL_ARGV_INT, NULL, NULL, "Starting index" },
  { "<LENGTH>", FTCL_ARGV_INT, NULL, NULL, "Length of the portion to extract"},
  { NULL, FTCL_ARGV_END, NULL, NULL, NULL }
};
  
static int
tclChainExtract (
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{
   CHAIN *chain = NULL, *outchain = NULL;
   CURSOR_T cursor;
   TYPE type;
   int start = 0, last, len = 0;
   char chainName[HANDLE_NAMELEN];
   int i;
   void *c1;
   char *pChainName = NULL;

   start = len = 0;
   pChainName = NULL;
   g_chainExtractTbl[1].dst = &pChainName;
   g_chainExtractTbl[2].dst = (int *) &start;
   g_chainExtractTbl[3].dst = (int *) &len;

   if ((i = shTclParseArgv(interp, &argc, argv, g_chainExtractTbl, 0, 
                           g_chainExtract)) != FTCL_ARGV_SUCCESS)
        return i;

   if (shTclAddrGetFromName(interp, pChainName, (void *)&chain, "CHAIN") != 
        TCL_OK)
	return TCL_ERROR;

/* Make type of output chain the same as the input type.  */
   type = shChainTypeDefine(chain);
/* Make a new handle to a CHAIN */
   if (shTclChainHandleNew (interp, chainName, shNameGetFromType(type),
	&outchain) != TCL_OK) return(TCL_ERROR);

   last = start+fabs(len) - 1;
   if (start < 0 || start > shChainSize(chain) - 1) {
	Tcl_SetResult (interp, "invalid start index", TCL_STATIC);
	return TCL_ERROR;
	}
   if (last < 0 || last > shChainSize(chain) - 1) {
	Tcl_SetResult (interp, 
           "invalid length index: must be <= count of the remaining"
           " elements on\nthe chain (beyond the start index)", TCL_STATIC);
	return TCL_ERROR;
	}
   if (len == 0) return TCL_OK;
   cursor=shChainCursorNew(chain);
   if (len > 0) {
	for (i=start; i<=last; i++) {
	   shChainCursorSet (chain, cursor, i);
	   c1 = shChainWalk(chain, cursor, THIS);
	   shChainElementAddByPos(outchain, c1,
		shNameGetFromType(shChainElementTypeGetByCursor(chain,
		cursor)), TAIL, AFTER);
	   }
   } else {
	for (i=last; i>=start; i--) {
	   shChainCursorSet (chain, cursor, i);
	   c1 = shChainWalk(chain, cursor, THIS);
	   shChainElementAddByPos(outchain, c1,
		shNameGetFromType(shChainElementTypeGetByCursor(chain,
		cursor)), TAIL, AFTER);
	   }
	}
   Tcl_SetResult (interp, chainName, TCL_VOLATILE);
   return TCL_OK;
}

/*****************************************************************************/
/*************************** tclChainTransXY *********************************/
/*****************************************************************************/
/*
 * Perform a TRANS operation on two attributes in each chain element in place
 */

static char *g_chainTransXY = "chainTransXY";
static ftclArgvInfo g_chainTransXYTbl[] = {
  { NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Perform a TRANS on a chain in place: x(out)=a+b*x+c*y; y(out)=d+e*x+f*y" },
  { "<CHAIN>", FTCL_ARGV_STRING, NULL, NULL, "Handle of the chain" },
  { "<XARG>", FTCL_ARGV_STRING, NULL, NULL, 
              "Argument on which operation is performed" },
  { "<YARG>", FTCL_ARGV_STRING, NULL, NULL,
              "Argument on which operation is performed" },
  { "<A>", FTCL_ARGV_DOUBLE, NULL, NULL, "Transformation coefficient" },
  { "<B>", FTCL_ARGV_DOUBLE, NULL, NULL, "Transformation coefficient" },
  { "<C>", FTCL_ARGV_DOUBLE, NULL, NULL, "Transformation coefficient" },
  { "<D>", FTCL_ARGV_DOUBLE, NULL, NULL, "Transformation coefficient" },
  { "<E>", FTCL_ARGV_DOUBLE, NULL, NULL, "Transformation coefficient" },
  { "<F>", FTCL_ARGV_DOUBLE, NULL, NULL, "Transformation coefficient" },
  { NULL, FTCL_ARGV_END, NULL, NULL, NULL },
};

static int
tclChainTransXY (
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{
   CHAIN *chain = NULL;
   double  a = 0, b = 0, c = 0, d = 0, e = 0, f = 0;
   char  *pChainName = NULL, *pX = NULL, *pY = NULL;
   int    i;

   pChainName = pX = pY = NULL;
   g_chainTransXYTbl[1].dst = &pChainName;
   g_chainTransXYTbl[2].dst = &pX;
   g_chainTransXYTbl[3].dst = &pY;
   g_chainTransXYTbl[4].dst = (double *) &a;
   g_chainTransXYTbl[5].dst = (double *) &b;
   g_chainTransXYTbl[6].dst = (double *) &c;
   g_chainTransXYTbl[7].dst = (double *) &d;
   g_chainTransXYTbl[8].dst = (double *) &e;
   g_chainTransXYTbl[9].dst = (double *) &f;

   if ((i = shTclParseArgv(interp, &argc, argv, g_chainTransXYTbl, 0, 
                           g_chainTransXY)) != FTCL_ARGV_SUCCESS)
        return i;

   if (shTclAddrGetFromName(interp, pChainName, (void *)&chain, "CHAIN") !=
         TCL_OK)
	return TCL_ERROR;

   if (shTclChainTransXY (interp, chain, pX, pY, a,b,c,d,e,f) != TCL_OK) 
       return TCL_ERROR;

   Tcl_SetResult (interp, pChainName, TCL_VOLATILE);
   return TCL_OK;
}

/*****************************************************************************/
/*************************** tclChainTransZXY ********************************/
/*****************************************************************************/
/*
 * Perform a TRANS operation on two attributes in each chain element;
 * store in a third attribute
 */

static char *g_chainTransZXY = "chainTransZXY";
static ftclArgvInfo g_chainTransZXYTbl[] = {
  { NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Perform a TRANS on a chain in place: z(out)=a+b*x+c*y" },
  { "<CHAIN>", FTCL_ARGV_STRING, NULL, NULL, "Handle of the chain" },
  { "<ZARG>", FTCL_ARGV_STRING, NULL, NULL, 
              "Argument where result is stored" },
  { "<XARG>", FTCL_ARGV_STRING, NULL, NULL, 
              "Argument on which operation is performed" },
  { "<YARG>", FTCL_ARGV_STRING, NULL, NULL,
              "Argument on which operation is performed" },
  { "<A>", FTCL_ARGV_DOUBLE, NULL, NULL, "Transformation coefficient" },
  { "<B>", FTCL_ARGV_DOUBLE, NULL, NULL, "Transformation coefficient" },
  { "<C>", FTCL_ARGV_DOUBLE, NULL, NULL, "Transformation coefficient" },
  { NULL, FTCL_ARGV_END, NULL, NULL, NULL },
};

static int
tclChainTransZXY (
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{
   CHAIN *chain = NULL;
   double  a = 0, b = 0, c = 0;
   char  *pChainName = NULL, *pZ = NULL, *pX = NULL, *pY = NULL;
   int    i;

   pChainName = pZ = pX = pY = NULL;
   g_chainTransZXYTbl[1].dst = &pChainName;
   g_chainTransZXYTbl[2].dst = &pZ;
   g_chainTransZXYTbl[3].dst = &pX;
   g_chainTransZXYTbl[4].dst = &pY;
   g_chainTransZXYTbl[5].dst = (double *) &a;
   g_chainTransZXYTbl[6].dst = (double *) &b;
   g_chainTransZXYTbl[7].dst = (double *) &c;

   if ((i = shTclParseArgv(interp, &argc, argv, g_chainTransZXYTbl, 0, 
                           g_chainTransZXY)) != FTCL_ARGV_SUCCESS)
        return i;

   if (shTclAddrGetFromName(interp, pChainName, (void *)&chain, "CHAIN") !=
         TCL_OK)
	return TCL_ERROR;

   if (shTclChainTransZXY (interp, chain, pZ, pX, pY, a, b, c) != TCL_OK) 
       return TCL_ERROR;

   Tcl_SetResult (interp, pChainName, TCL_VOLATILE);
   return TCL_OK;
}

/*****************************************************************************/
/*************************** tclChainScale **********************************/
/*****************************************************************************/
/*
 * Perform a Scale operation on an attribute in each chain element in place
 */

static char *g_chainScale = "chainScale";
static ftclArgvInfo g_chainScaleTbl[] = {
   { NULL, FTCL_ARGV_HELP, NULL, NULL, 
     "Scale a specified argument for all objects on a chain by specified"
     " coefficients in place: x(out) = a + b * x" },
   { "<CHAIN>", FTCL_ARGV_STRING, NULL, NULL, "Handle bound to chain" },
   { "<ARGUMENT>", FTCL_ARGV_STRING, NULL, NULL, 
                   "Argument on which scaling is performed" },
   { "<A>", FTCL_ARGV_DOUBLE, NULL, NULL, "Transformation coefficient" },
   { "<B>", FTCL_ARGV_DOUBLE, NULL, NULL, "Transformation coefficient" },
   { NULL, FTCL_ARGV_END, NULL, NULL, NULL }
};

static int
tclChainScale (
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{
   CHAIN  *chain = 0;
   double  a = 0, b = 0;
   char   *pChainName = 0, *pArg = 0;
   int     i;

   pChainName = pArg = NULL;
   a = b = 0.0;

   g_chainScaleTbl[1].dst = &pChainName;
   g_chainScaleTbl[2].dst = &pArg;
   g_chainScaleTbl[3].dst = (double *) &a;
   g_chainScaleTbl[4].dst = (double *) &b;

   if ((i = shTclParseArgv(interp, &argc, argv, g_chainScaleTbl, 0, 
                 g_chainScale)) != FTCL_ARGV_SUCCESS)
        return i;

   if (shTclAddrGetFromName(interp, pChainName, (void *)&chain, "CHAIN") != 
        TCL_OK)
	return TCL_ERROR;

   if (shTclChainScale (interp, chain, pArg, a, b)
	!= TCL_OK) return TCL_ERROR;

   Tcl_SetResult (interp, pChainName, TCL_VOLATILE);
   return TCL_OK;
}

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
shTclChainOpsDeclare(Tcl_Interp *interp) 
{
   char *module = "contrib";	/* name of this set of code */
   int   flags  = FTCL_ARGV_NO_LEFTOVERS;

   shTclDeclare(interp, g_chainSearch, 	(Tcl_CmdProc *)tclChainSearch, 
	(ClientData) 0, (Tcl_CmdDeleteProc *)NULL, module,
        shTclGetArgInfo(interp, g_chainSearchTbl, flags, g_chainSearch),
        shTclGetUsage(interp, g_chainSearchTbl, flags, g_chainSearch));

   shTclDeclare(interp, g_chainSelect, (Tcl_CmdProc *)tclChainSelect, 
	(ClientData) 0, (Tcl_CmdDeleteProc *)NULL, module,
        shTclGetArgInfo(interp, g_chainSelectTbl, flags, g_chainSelect),
        shTclGetUsage(interp, g_chainSelectTbl, flags, g_chainSelect));

   shTclDeclare(interp, g_chainFsort, (Tcl_CmdProc *)tclChainSort, 
	(ClientData) 0,	(Tcl_CmdDeleteProc *)NULL, module,
        shTclGetArgInfo(interp, g_chainFsortTbl, flags, g_chainFsort),
        shTclGetUsage(interp, g_chainFsortTbl, flags, g_chainFsort));

   shTclDeclare(interp, g_chainMatch, (Tcl_CmdProc *)tclChainMatch, 
	(ClientData) 0, (Tcl_CmdDeleteProc *)NULL, module,
        shTclGetArgInfo(interp, g_chainMatchTbl, 0, g_chainMatch),
        shTclGetUsage(interp, g_chainMatchTbl, 0, g_chainMatch));

   shTclDeclare(interp, g_chainSet, (Tcl_CmdProc *)tclChainSet, 
	(ClientData) 0, (Tcl_CmdDeleteProc *)NULL, module,
        shTclGetArgInfo(interp, g_chainSetTbl, 0, g_chainSet),
        shTclGetUsage(interp, g_chainSetTbl, 0, g_chainSet));        

   shTclDeclare(interp, g_chainSetFromHandle, 	
        (Tcl_CmdProc *)tclChainSetFromHandle, (ClientData) 0,
	(Tcl_CmdDeleteProc *)NULL, module,
        shTclGetArgInfo(interp, g_chainSetFromHandleTbl, 0, 
                        g_chainSetFromHandle),
        shTclGetUsage(interp, g_chainSetFromHandleTbl, 0, 
                        g_chainSetFromHandle));

   shTclDeclare(interp, g_chainExtract, (Tcl_CmdProc *)tclChainExtract, 
	(ClientData) 0, (Tcl_CmdDeleteProc *)NULL, module,
        shTclGetArgInfo(interp, g_chainExtractTbl, 0, g_chainExtract),
        shTclGetUsage(interp, g_chainExtractTbl, 0, g_chainExtract));

   shTclDeclare(interp, g_chainTransXY, (Tcl_CmdProc *)tclChainTransXY,
	(ClientData) 0, (Tcl_CmdDeleteProc *)NULL, module,
        shTclGetArgInfo(interp, g_chainTransXYTbl, 0, g_chainTransXY),
        shTclGetUsage(interp, g_chainTransXYTbl, 0, g_chainTransXY));

   shTclDeclare(interp, g_chainTransZXY, (Tcl_CmdProc *)tclChainTransZXY,
	(ClientData) 0, (Tcl_CmdDeleteProc *)NULL, module,
        shTclGetArgInfo(interp, g_chainTransZXYTbl, 0, g_chainTransZXY),
        shTclGetUsage(interp, g_chainTransZXYTbl, 0, g_chainTransZXY));

   shTclDeclare(interp, g_chainScale, (Tcl_CmdProc *)tclChainScale, 
	(ClientData) 0, (Tcl_CmdDeleteProc *)NULL, module,
	shTclGetArgInfo(interp, g_chainScaleTbl, 0, g_chainScale),
	shTclGetUsage(interp, g_chainScaleTbl, 0, g_chainScale));

   return;
}

