/*****************************************************************************
******************************************************************************
** <AUTO>
   FILE:
  	shChainContrib.c
   <HTML> 
   ABSTRACT:
   The following files are contributions made by Steve to Dervish Chains. They
   have been moved from his ts package and put in $DERVISH_DIR/contrib. They
   have also been dervish'ized in that the function names have been changed to
   reflect the Dervish conventions. However, the one major drawback is that
   not all TCL extensions have the appropriate C API.
  
   Files are : $DERVISH_DIR/contrib/shChainContrib.c
               $DERVISH_DIR/contrib/tclChainContrib.c
               $DERVISH_DIR/contrib/shChainUtils.c
               $DERVISH_DIR/contrib/shChainContrib.h
  
   - Vijay K. Gurbani
     Feb. 08 1995
</HTML>
</AUTO> */

/************************************************************************
**
**	This file contains many routines to manipulate chains and objects
**	on chains.  Most routines allow one to operate on elements of
**	objects in chains.  The elements can be accessed using the full
**	power of handle expressions.
**
** ENTRY POINT		SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
** shChainMatch	      	local	Match 2 chains of objects based on a set of
**				predicates and return a new chain of
**				matched objects
** shTclChainMatch	public  public interface to shChainMatch
**
** shTclChainScale      public  Scale an attribute for all objects in a chain.
** shTclChainSearch     public  Search an unsorted chain of objects based on a 
**                              set of predicates
** shTclChainSet        public  Set a specified element of all objects on a chain to a 
**                              fixed value
** shTclChainSetFromHandle public Like chainSet, but fixed value is passed as a handle, 
**                                not a Tcl string 
** shTclChainSort       public  Sort a chain of objects.
** shTclChainTransXY    public  Apply a trans to all objects in a chain.
** shTclChainSelect     public  Do a binary search a sorted chain of objects based on a 
**                              single predicate
** shTclChainTransZXY   public   Perform a computation on elements of a chain.  
**                              The calculation is z = a + b*x + c*y   
**                              where z, x, y are elements of objects on the chain.
**
** tclVstrIndexSearch   local   Next to top level routine to implement the search. 
** lowerIndex           local   Search for the pixel with the lowest index that 
**                              is greater than the target value.
** upperIndex           local   Search for the pixel with the highest index that is 
**                              less than the target value 
** tclVstrIndexSelect   local   Next to top level routine to implement the select
**
**  
** ENVIRONMENT:
**	ANSI C.
**
** REQUIRED PRODUCTS:
**	Dervish
**
** AUTHORS:
**	Creation date:  Early part of 1995
**	Steve Kent (with dervish-ization by Vijay Gurbani)
**
******************************************************************************
******************************************************************************
*/

#include <math.h>
#include <string.h>

#include "dervish.h"
#include "shGeneric.h"
#include "shChainContrib.h"
#include "shChainMerge.h"
#include "shCProgram.h"

/*============================================================================  
**============================================================================
**
** LOCAL MACROS, DEFINITIONS, ETC.
**
**============================================================================
*/

#ifndef MIN
#define MIN(x,y)    ((x) < (y)) ? x : y
#endif

#ifndef MAX
#define MAX(x,y)    ((x) > (y)) ? x : y
#endif

/*
** Local function prototypes
*/

static int tclVstrIndexSelect (Tcl_Interp *, void **, int, PREDTERM **, 
           const SCHEMA *, int, CHAIN *);
static int tclVstrIndexSearch (Tcl_Interp *, CHAIN *, PREDTERM **, 
           const SCHEMA *, int, CHAIN *);

/***************************************************************************/
/*********************** shTclChainMatch *************************************/
/***************************************************************************/

/* Array of attributes, windows, and wrap-around value */

typedef struct attrvstr {
	char *attr[2];
	double data[2];
/* Array of pointers to 2 compare functions */
	int (*compare[2])(BUFDESC *buff1, BUFDESC *buff2);
/* Array of pointers to 2 extraction functions */
	double (*extr[2])(void *);
/* Array of pointers to 2 set functions */
	void (*set[2])(void *, double);
	double window;
	double wrap;
	} ATTRVSTR;

static int 
shChainMatch (Tcl_Interp *interp, CHAIN *chain[2],
	PREDTERM **predVstr[2], ATTRVSTR *attrVstr, int nattr,
	CHAIN *outChain, char *typeName, int noSort) {

   int size[2];
   VTYPE vtype;
   const SCHEMA *schema[2];
   CHAIN *sortChain, *selChain, *searchChain;
   void **array;
   void *c1, *c2;
   SHMERGE *cout;
   TYPE chainType[2];
   char *chainTypeName[2];
   CURSOR_T cursor, cursor2;
   PREDTERM *predterm;
   int isel;
   void *dataBuff;
   int i, j;
   int indx;
   double limit1, limit2;
   double minDiff, sum;
   double diff;
   int nelem;

/* Get top level schema for chain type */
   for (isel=0; isel<2; isel++) {
	chainType[isel] = shChainTypeDefine(chain[isel]);
	chainTypeName[isel] = shNameGetFromType(chainType[isel]);
	size[isel] = shChainSize(chain[isel]);
	if ((schema[isel] = shSchemaGet(shNameGetFromType(chainType[isel])))
	   == NULL) {
	   Tcl_AppendResult (interp, "Unable to find schema for chain type ",
	   shNameGetFromType(chainType[isel]), NULL);
	   return TCL_ERROR;
	   }
	for (i=0; i<nattr; i++) {
/* Fetch attribute parameters of 1st attribute for each chain*/
	   vtype = (predVstr[isel])[i]->type;
	   if (vtype == SHORT) {
		attrVstr[i].compare[isel] = p_shShortCompare;
		attrVstr[i].extr[isel] = p_shShortExtract;
		attrVstr[i].set[isel] = p_shShortSet;
		}
	   else if (vtype == USHORT) {
		attrVstr[i].compare[isel] = p_shUshortCompare;
		attrVstr[i].extr[isel] = p_shUshortExtract;
		attrVstr[i].set[isel] = p_shUshortSet;
		}
	   else if (vtype == INT) {
		attrVstr[i].compare[isel] = p_shIntCompare;
		attrVstr[i].extr[isel] = p_shIntExtract;
		attrVstr[i].set[isel] = p_shIntSet;
		}
	   else if (vtype == UINT) {
		attrVstr[i].compare[isel] = p_shUintCompare;
		attrVstr[i].extr[isel] = p_shUintExtract;
		attrVstr[i].set[isel] = p_shUintSet;
		}
	   else if (vtype == LONG) {
		attrVstr[i].compare[isel] = p_shLongCompare;
		attrVstr[i].extr[isel] = p_shLongExtract;
		attrVstr[i].set[isel] = p_shLongSet;
		}
	   else if (vtype == ULONG) {
		attrVstr[i].compare[isel] = p_shUlongCompare;
		attrVstr[i].extr[isel] = p_shUlongExtract;
		attrVstr[i].set[isel] = p_shUlongSet;
		}
	   else if (vtype == FLOAT) {
		attrVstr[i].compare[isel] = p_shFltCompare;
		attrVstr[i].extr[isel] = p_shFltExtract;
		attrVstr[i].set[isel] = p_shFltSet;
		}
	   else if (vtype == DOUBLE) {
		attrVstr[i].compare[isel] = p_shDblCompare;
		attrVstr[i].extr[isel] = p_shDblExtract;
		attrVstr[i].set[isel] = p_shDblSet;
		}
	   else {
		Tcl_AppendResult (interp, "Invalid type. Bizarre.", NULL);
		return TCL_ERROR;
		}
	   }
	}
   if (size[0] == 0 || size[1] == 0) return TCL_OK;
/* Sort the shortest chain - this turns out to be the fastest strategy */
/*   if (size[0] < size[1]) isel=0; else isel=1; */
/* Well, forget speed.  Sort the 2nd chain always. */
   isel=1;
/* Copy the chain to be sorted.  This is not very efficient of storage,
 * but it lets me re-use existing routines */


/* do not sort the chain if told not to. */
   if (!noSort) {
     sortChain = shChainCopy(chain[isel]);
     if (shTclChainSort (interp, sortChain, attrVstr[0].attr[isel], 0)
	 != TCL_OK) {
       shChainDel(sortChain);
       return TCL_ERROR;
     }
   }
   else {
     sortChain = chain[isel];          /* no need to do the copy */
   }

/* Convert chain to array */
   array = (void **)shMalloc(size[isel]*sizeof(void *));
   cursor = shChainCursorNew(sortChain);
   for (i=0; i<size[isel]; i++) {
	c1 = shChainWalk(sortChain, cursor, NEXT);
	array[i] = c1;
	}
   if (!noSort) {
     shChainDel (sortChain);
   }
/* Fiddle with output chain type if a new typeName was specified - user must
 * be careful that new name makes sense!!! */
   if (typeName == NULL) {
	typeName = shNameGetFromType(shChainTypeDefine(outChain));
   } else {
	shChainTypeSet (outChain, typeName);
	}

/* Now do a select on the attributes */
   cursor = shChainCursorNew(chain[1-isel]);
   nelem=0;
   while ((c1 = shChainWalk (chain[1-isel], cursor, NEXT)) != NULL) {
	nelem++;
	for (i=0; i<nattr; i++) {
	   predterm = (predVstr[1-isel])[i];
/* Get a pointer to the data item on 1st list */
	   if ((dataBuff = shProgramEval (c1, predterm->program)) == NULL)
		return TCL_ERROR;
/* Set up limits in predicate vstrs */
/* Decode as a double */
	   attrVstr[i].data[1-isel] = attrVstr[i].extr[1-isel](dataBuff);
	   attrVstr[i].data[isel] = attrVstr[i].data[1-isel];
/* Limits */
	   predterm = (predVstr[isel])[i];
	   limit1 = attrVstr[i].data[isel] - attrVstr[i].window;
	   if (limit1 < 0 && attrVstr[i].wrap != 0) limit1 += attrVstr[i].wrap;
	   if (limit1 >= attrVstr[i].wrap && attrVstr[i].wrap != 0)
		limit1 -= attrVstr[i].wrap;
	   limit2 = attrVstr[i].data[isel] + attrVstr[i].window;
	   if (limit2 < 0 && attrVstr[i].wrap != 0) limit2 += attrVstr[i].wrap;
	   if (limit2 >= attrVstr[i].wrap && attrVstr[i].wrap != 0)
		limit2 -= attrVstr[i].wrap;
/* Set the predicate limits as the same type as the sorted string attribute */
	   attrVstr[i].set[isel] (predterm->key[0].data, limit1);
	   attrVstr[i].set[isel] (predterm->key[1].data, limit2);
/* Must reset operator flags each time because the select function messes
 * with them! */
	   predterm->oper[0] = O_LE;
	   predterm->oper[1] = O_LE;
	   }
	selChain = shChainNew(chainTypeName[isel]);
	if (tclVstrIndexSelect (interp, array, size[isel], predVstr[isel],
	   schema[isel], 1, selChain) != TCL_OK) {
		shChainDel(selChain);
		shFree(array);
		return TCL_ERROR;
		}
	if (shChainSize (selChain) == 0) {
	   shChainDel (selChain);
	   continue;
	   }
/* Now do search on remaining predicates */
	if (nattr > 1) {
	   searchChain = shChainNew(chainTypeName[isel]);
	   if (tclVstrIndexSearch (interp, selChain, &(predVstr[isel])[1],
	      schema[isel], nattr-1, searchChain) != TCL_OK) {
		shChainDel(searchChain);
		shChainDel(selChain);
		shFree(array);
		return TCL_ERROR;
		}
	   } else {searchChain = shChainCopy(selChain);}
/* If chain size is 0, do nothing.  If chain size is >1, find the closest
 * match
*/
	if (shChainSize (searchChain) == 0) {
	   shChainDel (searchChain);
	   shChainDel (selChain);
	   continue;
	   }
	cursor2 = shChainCursorNew (searchChain);
	if (shChainSize (searchChain) > 1) {
	   minDiff = 1.e30;
	   indx = -1;
	   for (j=0; j<shChainSize(searchChain); j++) {
		c2 = shChainWalk(searchChain, cursor2, NEXT);
		sum = 0.;
		for (i=0; i<nattr; i++) {
		   predterm = (predVstr[isel])[i];
/* Extract data value */
		   if ((dataBuff = shProgramEval (c2, predterm->program))
			== NULL) return TCL_ERROR;
/* Decode as a double */
		   attrVstr[i].data[isel] = attrVstr[i].extr[isel](dataBuff);
		   diff = (attrVstr[i].data[isel]-attrVstr[i].data[1-isel]);
		   if (attrVstr[i].wrap > 0) {
			if (fabs(diff) > attrVstr[i].wrap/2)
			   diff = fmod(diff + attrVstr[i].wrap,
				attrVstr[i].wrap);
			}
		   if (attrVstr[i].window != 0.)
			sum = sum + pow( diff/attrVstr[i].window, 2);
		   else
			sum = sum + pow(diff, 2);
		   }
		if (sum < minDiff) {
		   minDiff = sum;
		   indx = j;
		   }
		}
	   } else {indx = 0;}
	shChainCursorSet (searchChain, cursor2, indx);
	c2 = shChainWalk (searchChain, cursor2, THIS);

	cout = (void *)shGenericNew(typeName);

	shChainElementAddByPos (outChain, (void *)cout,
	   typeName, TAIL, AFTER);
	shChainCursorDel(searchChain, cursor2);
/* c1 is from 1-isel; c2 is from isel */
	if (isel == 0) {
	   cout->object1 = c2;
	   cout->object2 = c1;
	   }
	else {
	   cout->object1 = c1;
	   cout->object2 = c2;
	   }
	shChainDel (searchChain);
	shChainDel (selChain);
	}
   shFree(array);
   shChainCursorDel (chain[1-isel], cursor);
   return TCL_OK;
   }


/******************************************************************************
 ******************************************************************************
<AUTO EXTRACT>
 * 
 *  ROUTINE: shTclChainMatch
 *
 *  DESCRIPTION:
 * Top level subroutine to perform the match function.
 * This routine takes the input from the command line and formats it
 * in a form that shChainMatch can do its job.  Much error checking
 * is done
 *
 *  RETURN VALUES:
 *	TCL_OK: success
 *      TCL_ERROR: something went wrong. An error message is on the tcl interp.
 *
</AUTO> */

int shTclChainMatch (Tcl_Interp *interp,
	CHAIN *chain[2], char *predList, CHAIN *outchain, char *typeName,
	int noSort) {

   int i, j;
   int splitArgc;
   char **splitArgv;
   int splitSplitArgc;
   char **splitSplitArgv;
   PREDTERM **predVstr[2] = {NULL, NULL};
   PREDTERM *predterm;
   int ierr;
   TYPE type[2];
   const SCHEMA *schema[2];
   ATTRVSTR *attrVstr=NULL;
   int isel;

/* Get the chain type */
   for (isel=0; isel<2; isel++) {
	type[isel] = shChainTypeDefine(chain[isel]);
/* Get the schema pointer */
	if ((schema[isel] = shSchemaGet (shNameGetFromType(type[isel])))
	  == NULL) {
	   Tcl_SetResult (interp, "Can't find schema for chain type", TCL_STATIC);
	   return TCL_ERROR;
	   }
	}
/* Predicates come in as a list of lists */
/* Each predicate is of the form {chain1arg_i  chain2arg_i window [wrap]}
 * wrap is optional and allows for wrap-around (for angular coordinates
 * such as RA that wrap around at 12 hrs or 360 degrees)
*/
/* Split predicate list of lists */
   Tcl_SplitList (interp, predList, &splitArgc, &splitArgv);
   if (splitArgc > 0) {
	predVstr[0] = (PREDTERM **)shMalloc (splitArgc * sizeof (PREDTERM *));
	predVstr[1] = (PREDTERM **)shMalloc (splitArgc * sizeof (PREDTERM *));
	attrVstr = (ATTRVSTR *)shMalloc (splitArgc * sizeof (ATTRVSTR));
/* Zero the arrays */
	for (i=0; i<splitArgc; i++) {
	   (predVstr[0])[i]=NULL;
	   (predVstr[1])[i]=NULL;
	   }
        } else {
	Tcl_SetResult (interp, "No attributes provided for matchup", TCL_STATIC);
	return TCL_ERROR;
	}
   ierr = TCL_OK;
/* Allocate all buffers, except the key data */
   for (i = 0; i < splitArgc; i++) {
/* Split predicate term list */
/* 1st element is arg1, 2nd element is arg2, 3rd element is window value,
 * 4th element is wrap value (optional) */
	Tcl_SplitList (interp, splitArgv[i], &splitSplitArgc, &splitSplitArgv);
	if (splitSplitArgc != 3 && splitSplitArgc != 4) {
	   Tcl_AppendResult (interp, "Badly formed argument list ",splitArgv[i],
		NULL);
/* Free the buffer */
	   free(splitSplitArgv);
	   ierr = TCL_ERROR;
	   break;
	   }
	for (isel=0; isel<2; isel++) {
	   predterm = (PREDTERM *)shMalloc(sizeof(PREDTERM));
	   (predVstr[isel])[i] = predterm;
/*Initialize other dynamic fields */
	   predterm->key[0].data = NULL;
	   predterm->key[1].data = NULL;
	   predterm->attribute = NULL;
	   predterm->program = NULL;
/* We will always do range searches */
	   predterm->range = 1;
/* Fetch attribute parameters */
	   if (p_shTclParamGet(interp, schema[isel], splitSplitArgv[isel],
		&predterm->type, &type[isel]) != TCL_OK) {
/* Free the buffer */
		free(splitSplitArgv);
		ierr = TCL_ERROR;
		break;
		}
/* Create a compiled program */
	   if ((predterm->program = shProgramCreateBySchema (schema[isel],
		splitSplitArgv[isel])) 	== NULL) {
/* Free the buffer */
		free(splitSplitArgv);
		ierr = TCL_ERROR;
		break;
		}
	   predterm->not[0] = 0;
/* Decode the window.  This value is over-written later, but I at least
 * can allocate the buffers. */
	   if (p_shTclKeyData(predterm->type, type[isel], splitSplitArgv[2],
		&(predterm->key[0])) != TCL_OK) {
		Tcl_AppendResult(interp, "Error decoding field: ",
		   splitSplitArgv[2], NULL);
/* Free the buffer */
		free(splitSplitArgv);
		ierr = TCL_ERROR;
		break;
		}
	   if (p_shTclKeyData(predterm->type, type[isel], splitSplitArgv[2],
		&(predterm->key[1])) != TCL_OK) {
		Tcl_AppendResult(interp, "Error decoding field: ",
		   splitSplitArgv[2], NULL);
/* Free the buffer */
		free(splitSplitArgv);
		ierr = TCL_ERROR;
		break;
		}
/* Store the attribute */
	   predterm->attribute = shMalloc(strlen(splitSplitArgv[isel])+1);
	   strcpy (predterm->attribute, splitSplitArgv[isel]);
/* Set up attrvstr */
	   attrVstr[i].attr[isel] = shMalloc(strlen(splitSplitArgv[isel])+1);
	   strcpy (attrVstr[i].attr[isel], splitSplitArgv[isel]);
	   if (Tcl_GetDouble(interp, splitSplitArgv[2], &attrVstr[i].window)
		!= TCL_OK) {
		free(splitSplitArgv);
		ierr = TCL_ERROR;
		break;
		}
	   if (splitSplitArgc == 4) {
		if (Tcl_GetDouble(interp, splitSplitArgv[3], &attrVstr[i].wrap)
		   != TCL_OK) {
		   free(splitSplitArgv);
		   ierr = TCL_ERROR;
		   break;
		   }
		}
	   else attrVstr[i].wrap=0.;
	   }
	free(splitSplitArgv);
	}

/* Free the buffer */
   free(splitArgv);

/* Carry out the operation */
   if (ierr == TCL_OK) {	/* Successful parse */
	ierr = shChainMatch (interp, chain, predVstr, attrVstr, splitArgc,
	   outchain, typeName, noSort);
	} else {
	ierr = TCL_ERROR;
	Tcl_AppendResult(interp, " Unable to parse predicate list", NULL);
	}
/* Free up storage */
   for (i=0; i<splitArgc; i++) {
	for (isel=0; isel<2; isel++) {
	   if (predVstr[isel] == NULL) break;
	   predterm = ((PREDTERM **)predVstr[isel])[i];
	   if (predterm == NULL) break;
	   if (predterm->attribute != NULL) shFree(predterm->attribute);
	   if (predterm->program != NULL) shProgramDel (predterm->program);
	   for (j=0; j<2; j++) {
		if (predterm->key[j].data != NULL)
			shFree (predterm->key[j].data);
		}
	   if (attrVstr[i].attr[isel] != NULL) shFree(attrVstr[i].attr[isel]);
	   shFree (predterm);
	   }
	}
   if (attrVstr != NULL) shFree(attrVstr);
   if (predVstr[0] != NULL) shFree(predVstr[0]);
   if (predVstr[1] != NULL) shFree(predVstr[1]);
   return ierr;
   }

/***************************************************************************/
/****************************** shTclChainScale ****************************/
/***************************************************************************/

/* Scale an attribute for all objects in a chain.  The transformation is done
 * in place (not ideal but it gets the job done) */

/* Include a file that defines all necesary other include files and lots of
 * static support functions */

int shTclChainScale (Tcl_Interp *interp, CHAIN *chain, char *xattr,
	double a, double b) {
   TYPE xtype;
   VTYPE xvtype;
   const SCHEMA *schema;
   const SCHEMA *xlSchema;
   void *c1;
   TYPE chainType;
   CURSOR_T cursor;
   PROGRAM *program;
   void *xattrPtr;
   double x, result;
   double (*xextr)(void *);
   void (*xset)(void *, double);

/* Get top level schema for chain type */
   chainType = shChainTypeDefine(chain);
   if ((schema = shSchemaGet(shNameGetFromType(chainType)))
	== NULL) {
	Tcl_SetResult (interp, "Unable to find schema for chain type",
	   TCL_STATIC);
	return TCL_ERROR;
	}
/* Fetch attribute parameters */
   if (p_shTclParamGet(interp, schema, xattr, &xvtype,
	&xtype)  != TCL_OK) {
	Tcl_AppendResult (interp, "Cannot get schema for attribute ",
	   xattr, NULL);
	return TCL_ERROR;
	}

/* Compile program */
   if ((program = shProgramCreateBySchema(schema, xattr)) == NULL) {
	Tcl_SetResult(interp, "Cannot compile program", TCL_STATIC);
	return TCL_ERROR;
	}

/* For use later, get the local schema */
   if ((xlSchema = shSchemaGet(shNameGetFromType(xtype))) == NULL) {
	Tcl_SetResult (interp, "Cannot find schema for attribute", TCL_STATIC);
	return TCL_ERROR;
	}

/* Get proper comparison function */

        if (xvtype == SHORT) {xextr = p_shShortExtract; xset = p_shShortSet;}
        else if (xvtype == USHORT) {xextr = p_shUshortExtract; xset = p_shUshortSet;}
        else if (xvtype == INT) {xextr = p_shIntExtract; xset = p_shIntSet; }
        else if (xvtype == UINT) {xextr = p_shUintExtract; xset = p_shUintSet;}
        else if (xvtype == LONG) {xextr = p_shLongExtract; xset = p_shLongSet;}
        else if (xvtype == ULONG) {xextr = p_shUlongExtract; xset = p_shUlongSet;}
        else if (xvtype == FLOAT) {xextr = p_shFltExtract; xset = p_shFltSet;}
        else if (xvtype == DOUBLE) {xextr = p_shDblExtract; xset = p_shDblSet;}
        else {
           Tcl_AppendResult (interp, "Invalid x type. Bizarre.", NULL);
           return TCL_ERROR;
           }

/* Loop through the chain.  */

   cursor = shChainCursorNew(chain);

   while ((c1 = shChainWalk (chain, cursor, NEXT)) != NULL) {

/* Get pointer to the attribute in object c1. */
	if ((xattrPtr = shProgramEval (c1, program)) == NULL)
		return TCL_ERROR;
	x = xextr(xattrPtr);
	result = a + b*x;
	xset (xattrPtr, result);
	} /* End of loop through all objects */

/* Cleanup */
   shProgramDel(program);
   shChainCursorDel (chain, cursor);
   return TCL_OK;
   }

/***************************************************************************/
/****************************** shTclChainSearch ***************************/
/***************************************************************************/

/* Search an unsorted chain of objects based on a set of predicates */

/* Include a file that defines all necesary other include files and lots of
 * static support functions */

/* Next to top level routine to implement the search */

static int tclVstrIndexSearch (Tcl_Interp *interp, CHAIN *chain,
	PREDTERM **predVstr, const SCHEMA *schema, int npred, CHAIN *outchain)
{
   VTYPE type;
   PREDTERM *predterm;
   CURSOR_T cursor;
   int ipred;
   int i;
   BUFDESC dbuff; /* qbuff; */
   void *c1;
   int keep;
   char *chainTypeName = shNameGetFromType(shChainTypeDefine(chain));
   for (ipred = 0; ipred < npred; ipred++) {
	predterm = ((PREDTERM **)predVstr)[ipred];

/* Get proper comparison function */

	type = predterm->type;
	if ( (predterm->oper[0] == O_BA) || (predterm->oper[0] == O_BNA) ) {
	  if (type == INT) predterm->compare = p_shIntBitCompare;
	  else {
	     Tcl_AppendResult (interp, "Invalid type. Bizarre.", NULL);
	     return TCL_ERROR;
	     }
	} else {
	  if (type == CHAR) predterm->compare = p_shCharCompare;
	  else if (type == UCHAR) predterm->compare = p_shUcharCompare;
	  else if (type == STR) predterm->compare = p_shStrCompare;
	  else if (type == SHORT) predterm->compare = p_shShortCompare;
	  else if (type == USHORT) predterm->compare = p_shUshortCompare;
	  else if (type == INT) predterm->compare = p_shIntCompare;
	  else if (type == UINT) predterm->compare = p_shUintCompare;
	  else if (type == LONG) predterm->compare = p_shLongCompare;
	  else if (type == ULONG) predterm->compare = p_shUlongCompare;
	  else if (type == FLOAT) predterm->compare = p_shFltCompare;
	  else if (type == DOUBLE) predterm->compare = p_shDblCompare;
	  else if (type == EENUM) predterm->compare = p_shEnumCompare;
	  else {
	     Tcl_AppendResult (interp, "Invalid type. Bizarre.", NULL);
	     return TCL_ERROR;
	     }
	}

/* For a range, reverse the sense of the 1st operator. */
	if (predterm->range) {
	   if (predterm->oper[0] == O_LT) predterm->oper[0] = O_GT;
	   else if (predterm->oper[0] == O_LE) predterm->oper[0] = O_GE;
	   else if (predterm->oper[0] == O_GE) predterm->oper[0] = O_LE;
	   else if (predterm->oper[0] == O_GT) predterm->oper[0] = O_LT;
/* For a range, the equal, not equal, and bitwise operators are stupid */
	   if (predterm->oper[0] == O_EQ || predterm->oper[0] == O_NE ||
	       predterm->oper[1] == O_EQ || predterm->oper[1] == O_NE ||
	       predterm->oper[0] == O_BNA || predterm->oper[1] == O_BNA ||
	       predterm->oper[0] == O_BA || predterm->oper[1] == O_BA ) {
		Tcl_SetResult (interp, "Bad operator type specified for a range",
		TCL_STATIC);
		return TCL_ERROR;
		}
	   }

/* Convert the LE, NE, and GE operators to their complements with
 * a NOT function
*/
	for (i = 0; i <= predterm->range; i++) {
	   if (predterm->oper[i] == O_LE) {
		predterm->oper[i] = O_GT;
		predterm->not[i] = 1;
		}
	   else if (predterm->oper[i] == O_NE) {
		predterm->oper[i] = O_EQ;
		predterm->not[i] = 1;
		}
	   else if (predterm->oper[i] == O_GE) {
		predterm->oper[i] = O_LT;
		predterm->not[i] = 1;
		}
	   else {
		predterm->not[i] = 0;
		}
/* Recode the operators types */
	   if (predterm->oper[i] == O_LT) predterm->opcode[i] = -1;
	   else if (predterm->oper[i] == O_EQ) predterm->opcode[i] = 0;
	   else if (predterm->oper[i] == O_GT) predterm->opcode[i] = 1;
	   else if (predterm->oper[i] == O_BNA) predterm->opcode[i] = 0;
	   else if (predterm->oper[i] == O_BA) predterm->opcode[i] = 1;
	   }
/* For a range, compare the limits. Determine if we use wrap-around */
	if (predterm->range) {
	   if (p_shBufCompare(predterm->key[1], predterm->key[0], predterm, 0)
		== 0 && p_shBufCompare(predterm->key[0], predterm->key[1],
		predterm, 1) == 0) predterm->combine = 1;
	   else predterm->combine = 0;
	   }
	}   /* End of loop through predicates */

/* Now do the selects */
   cursor = shChainCursorNew(chain);
   while ((c1 = shChainWalk (chain, cursor, NEXT)) != NULL) {
	keep = 1;
	for (ipred = 0; ipred < npred; ipred++) {
	   predterm = ((PREDTERM **)predVstr)[ipred];
	   if ((dbuff.data = shProgramEval (c1, predterm->program)) == NULL) {
		keep = 0;
		break;
		}
	   if (p_shPredCompare (dbuff, predterm) == 0) {
		keep = 0;
		break;
		}
	   }	/* End of loop through all predicates */
	if (keep == 1) shChainElementAddByPos(outchain, c1,
		chainTypeName, TAIL, AFTER);
	} /* End of loop through all objects */
   shChainCursorDel (chain, cursor);
   return TCL_OK;
}

/* Top level subroutine to perform the search function */

int shTclChainSearch (Tcl_Interp *interp,
	CHAIN *chain, char *predList, CHAIN *outchain) {

   int i, j;
   int splitArgc;
   char **splitArgv;
   int splitSplitArgc;
   char **splitSplitArgv;
   PREDTERM **predVstr = NULL;
   int nalloc;
   PREDTERM *predterm;
   int ierr;
   TYPE type;
   const SCHEMA *schema;

/* Get the chain type */
   type = shChainTypeDefine(chain);
/* Get the schema pointer */
   if ((schema = shSchemaGet (shNameGetFromType(type))) == NULL) {
	Tcl_SetResult (interp, "Can't find schema for chain type", TCL_STATIC);
	return TCL_ERROR;
	}
/* Predicates come in as a list of lists */
/* Split predicate list of lists */
   Tcl_SplitList (interp, predList, &splitArgc, &splitArgv);
   if (splitArgc > 0) {
	predVstr = (PREDTERM **)shMalloc (splitArgc * sizeof (PREDTERM *));
        } else {
	predVstr = NULL;
	}
/* Allocate all buffers except the key data */
   nalloc = 0;
   for (i = 0; i < splitArgc; i++) {
/* Split predicate term list */
/* 1st element is name, 2nd element is operation, 3rd element is value */
	Tcl_SplitList (interp, splitArgv[i], &splitSplitArgc, &splitSplitArgv);
	if (splitSplitArgc != 3 && splitSplitArgc != 5) {
	   Tcl_AppendResult (interp, "Badly formed predicate: ",splitArgv[i],
		NULL);
/* Free the buffer */
	   free(splitSplitArgv);
	   break;
	   }
	predterm = (PREDTERM *)shMalloc(sizeof(PREDTERM));
	predVstr[i] = predterm;
/*Initialize other dynamic fields */
	predterm->key[0].data = NULL;
	predterm->key[1].data = NULL;
	predterm->attribute = NULL;
	predterm->program = NULL;
	nalloc++;
/* Predicate construction depends on whether we have 3 arg or 5 arg format */
	if (splitSplitArgc == 3) {
/* Conventional type predicate */
	   predterm->range = 0;
/* Fetch attribute parameters */
	   if (p_shTclParamGet(interp, schema, splitSplitArgv[0],
		&predterm->type, &type) != TCL_OK) {
/* Free the buffer */
		free(splitSplitArgv);
		break;
		}
/* Create a compiled program */
	   if ((predterm->program = shProgramCreateBySchema (schema,
		splitSplitArgv[0])) == NULL) {
/* Free the buffer */
		free(splitSplitArgv);
		break;
		}
/* Set the operator type */
	   if (p_shTclOp(splitSplitArgv[1], &(predterm->oper[0])) != TCL_OK) {
		Tcl_AppendResult(interp, "Bad operatortype: ",splitSplitArgv[1],
		   NULL);
/* Free the buffer */
		free(splitSplitArgv);
		break;
		}
	   predterm->not[0] = 0;
/* Decode the data */
	   if (p_shTclKeyData(predterm->type, type, splitSplitArgv[2],
		&(predterm->key[0])) != TCL_OK) {
		Tcl_AppendResult(interp, "Error decoding field: ",
		   splitSplitArgv[2], NULL);
/* Free the buffer */
		free(splitSplitArgv);
		break;
		}
/* Store the attribute */
	   predterm->attribute = shMalloc(strlen(splitSplitArgv[0])+1);
	   strcpy (predterm->attribute, splitSplitArgv[0]);
	   } else {
/* Range type predicate */
	   predterm->range = 1;
/* First half */
/* Fetch attribute parameters */
	   if (p_shTclParamGet(interp, schema, splitSplitArgv[2], &predterm->type,
		&type) 	!= TCL_OK) {
/* Free the buffer */
		free(splitSplitArgv);
		break;
		}
/* Create a compiled program */
	   if ((predterm->program = shProgramCreateBySchema (schema,
		splitSplitArgv[2])) == NULL) {
/* Free the buffer */
		free(splitSplitArgv);
		break;
		}
/* Set the operator type */
	   if (p_shTclOp(splitSplitArgv[1], &(predterm->oper[0])) != TCL_OK) {
		Tcl_AppendResult(interp, "Bad operatortype: ",splitSplitArgv[1],
		   NULL);
/* Free the buffer */
		free(splitSplitArgv);
		break;
		}
	   predterm->not[0] = 0;
/* Decode the data */
	   if (p_shTclKeyData(predterm->type, type, splitSplitArgv[0],
		&(predterm->key[0])) != TCL_OK) {
		Tcl_AppendResult(interp, "Error decoding field: ",
		   splitSplitArgv[0], NULL);
/* Free the buffer */
		free(splitSplitArgv);
		break;
		}
/* Second half */
/* Set the operator type */
	   if (p_shTclOp(splitSplitArgv[3], &(predterm->oper[1])) != TCL_OK) {
		Tcl_AppendResult(interp, "Bad operatortype: ",splitSplitArgv[3],
		   NULL);
/* Free the buffer */
		free(splitSplitArgv);
		break;
		}
	   predterm->not[1] = 0;
/* Decode the data */
	   if (p_shTclKeyData(predterm->type, type, splitSplitArgv[4],
		&(predterm->key[1])) != TCL_OK) {
		Tcl_AppendResult(interp, "Error decoding field: ",
		   splitSplitArgv[4], NULL);
/* Free the buffer */
		free(splitSplitArgv);
		break;
		}
/* Store the attribute */
	   predterm->attribute = shMalloc(strlen(splitSplitArgv[2])+1);
	   strcpy (predterm->attribute, splitSplitArgv[2]);
	   }
	free(splitSplitArgv);
	}

/* Free the buffer */
   free(splitArgv);
   if (i == splitArgc) {	/* Successful parse */
	ierr = tclVstrIndexSearch (interp, chain, predVstr, schema, splitArgc,
	   outchain);
	} else {
	ierr = TCL_ERROR;
	Tcl_AppendResult(interp, " Unable to parse predicate list", NULL);
	}
/* Free up storage */
   for (i=0; i<nalloc; i++) {
	predterm = ((PREDTERM **)predVstr)[i];
	if (predterm->attribute != NULL) shFree(predterm->attribute);
	if (predterm->program != NULL) shProgramDel (predterm->program);
	for (j=0; j<2; j++) {
	   if (predterm->key[j].data != NULL) {
		shFree (predterm->key[j].data);
		}
	   }
	shFree (predterm);
	}
   if (predVstr != NULL) shFree(predVstr);
   return ierr;
   }

/***************************************************************************/
/****************************** shTclChainSelect ***************************/
/***************************************************************************/

/* Do a binary search a sorted chain of objects based on a single predicate */

/*** Search for the pixel with the lowest index that is greater than the
** target value */

static int lowerIndex (void **array, int ilen, const SCHEMA *schema,
	PREDTERM *predterm, int ikey) {

   int istart, imid;
   BUFDESC dbuff;
   void *c1;

   istart = 0;
/** Begin binary search.  Divide the interval istart to istart + ilen -1 into
*** halves and see which half the starting index lies **/
   while (ilen > 1) {
	imid = (ilen-1)/2 + istart;
	c1 = array[imid];
	if ((dbuff.data = shProgramEval (c1, predterm->program)) == NULL)
		return TCL_ERROR;
	if (predterm->compare(&dbuff, &predterm->key[ikey]) > 0) {
/* This index is OK. Try lower as well, but include this index in new range  */
	    ilen = imid - istart + 1;
	    }
/*** This index is too low.  Try higher ***/
	else {
	    istart = imid + 1;
	    ilen = ilen/2;
	    }
	}
/* One last test if we are at the top of the range */
   c1 = array[istart];
   if ((dbuff.data = shProgramEval (c1, predterm->program)) == NULL)
		return TCL_ERROR;
   if (predterm->compare(&dbuff, &predterm->key[ikey]) > 0) {
/* This index is OK. */
	    }
/*** This index is too low.  Go one higher ***/
	else {++istart;}
   return istart;
    }

/*** Search for the pixel with the highest index that is less than the
** target value */

static int upperIndex (void **array, int ilen, const SCHEMA *schema,
	PREDTERM *predterm, int ikey) {

   int istart, imid;
   BUFDESC dbuff;
   void *c1;

   istart = 0;
/** Begin binary search.  Divide the interval istart to istart + ilen -1 into
*** halves and see which half the starting index lies **/
   while (ilen > 1) {
	imid = (ilen)/2 + istart;
	c1 = array[imid];
	if ((dbuff.data = shProgramEval (c1, predterm->program)) == NULL)
		return TCL_ERROR;
	if (predterm->compare(&dbuff, &predterm->key[ikey]) < 0) {
/* This index is OK. Try higher as well, but include this index in new range  */
	    istart = imid;
	    ilen = (ilen+1)/2;
	    }
/*** This index is too high.  Try lower ***/
	else {
	    ilen = imid - istart;
	    }
	}
/* One last test if we are at the bottom of the range */
   c1 = array[istart];
   if ((dbuff.data = shProgramEval (c1, predterm->program)) == NULL)
		return TCL_ERROR;
   if (predterm->compare(&dbuff, &predterm->key[ikey]) < 0) {
/* This index is OK. */
	    }
/*** This index is too high. Go one lower ***/
	else {--istart;}
   return istart;
   }

static int tclVstrIndexSelect (Tcl_Interp *interp, void **array,
	int size, PREDTERM **predVstr, const SCHEMA *schema, int npred,
	CHAIN *outchain)
{
   VTYPE type;
   PREDTERM *predterm;
   int ipred;
   int i;
   void *c1;
   int high[2], low[2];
   int bottom, top;

   for (ipred = 0; ipred < npred; ipred++) {
	predterm = ((PREDTERM **)predVstr)[ipred];

/* Get proper comparison function */

	type = predterm->type;
	if (type == CHAR) predterm->compare = p_shCharCompare;
	else if (type == UCHAR) predterm->compare = p_shUcharCompare;
	else if (type == STR) predterm->compare = p_shStrCompare;
	else if (type == SHORT) predterm->compare = p_shShortCompare;
	else if (type == USHORT) predterm->compare = p_shUshortCompare;
	else if (type == INT) predterm->compare = p_shIntCompare;
	else if (type == UINT) predterm->compare = p_shUintCompare;
	else if (type == LONG) predterm->compare = p_shLongCompare;
	else if (type == ULONG) predterm->compare = p_shUlongCompare;
	else if (type == FLOAT) predterm->compare = p_shFltCompare;
	else if (type == DOUBLE) predterm->compare = p_shDblCompare;
	else if (type == EENUM) predterm->compare = p_shEnumCompare;
	else {
	   Tcl_AppendResult (interp, "Invalid type. Bizarre.", NULL);
	   return TCL_ERROR;
	   }
/* For a range, reverse the sense of the 1st operator. */
	if (predterm->range) {
	   if (predterm->oper[0] == O_LT) predterm->oper[0] = O_GT;
	   else if (predterm->oper[0] == O_LE) predterm->oper[0] = O_GE;
	   else if (predterm->oper[0] == O_GE) predterm->oper[0] = O_LE;
	   else if (predterm->oper[0] == O_GT) predterm->oper[0] = O_LT;
/* For a range, the equal and not equal operators are stupid */
	   if (predterm->oper[0] == O_EQ || predterm->oper[0] == O_NE ||
	       predterm->oper[1] == O_EQ || predterm->oper[1] == O_NE) {
		Tcl_SetResult (interp, "Bad operator type specified for a range",
		TCL_STATIC);
		return TCL_ERROR;
		}
	   }

/* Convert the LE, NE, and GE operators to their complements with
 * a NOT function
*/
	for (i = 0; i <= predterm->range; i++) {
	   if (predterm->oper[i] == O_LE) {
		predterm->oper[i] = O_GT;
		predterm->not[i] = 1;
		}
	   else if (predterm->oper[i] == O_NE) {
		predterm->oper[i] = O_EQ;
		predterm->not[i] = 1;
		}
	   else if (predterm->oper[i] == O_GE) {
		predterm->oper[i] = O_LT;
		predterm->not[i] = 1;
		}
	   else {
		predterm->not[i] = 0;
		}
/* Recode the operators types */
	   if (predterm->oper[i] == O_LT) predterm->opcode[i] = -1;
	   else if (predterm->oper[i] == O_EQ) predterm->opcode[i] = 0;
	   else if (predterm->oper[i] == O_GT) predterm->opcode[i] = 1;
	   }
/* For a range, compare the limits. Determine if we use wrap-around */
	if (predterm->range) {
	   if (p_shBufCompare(predterm->key[1], predterm->key[0], predterm, 0)
		== 0 && p_shBufCompare(predterm->key[0], predterm->key[1],
		predterm, 1) == 0) predterm->combine = 1;
	   else predterm->combine = 0;
	   }

	}   /* End of loop through predicates */

/************** End setup ***************/

   predterm = predVstr[0];
/* Now do the selects */
   for (i = 0; i <= predterm->range; i++) {
	if (predterm->oper[i] == O_LT) {
	   low[i] = 0;
	   high[i] = upperIndex (array, size, schema, predterm, i);
	   if (predterm->not[i] != 0) {
		low[i] = high[i] + 1;
		high[i] = size - 1;
		}
	   }
	else {
	   low[i] = lowerIndex (array, size, schema, predterm, i);
	   high[i] = size-1;
	   if (predterm->not[i] != 0 ) {
		high[i] = low[i] - 1;
		low[i] = 0;
		} /* End of NOT operation */
	   }	/* End of GT operation */
	}	/* End of loop through 1 or 2 predicates */
/* If not a range, just copy the 1st set of indices */
   if (predterm->range == 0) {
	if (high[0] - low[0] + 1 > 0) {
	   for (i=low[0]; i<=high[0]; i++) {
		c1 = array[i];
		shChainElementAddByPos (outchain, c1, schema->name, TAIL,
			AFTER);
		}
	   }
	}
   else {
/* If ranges overlap, use intersection; else use union */
/* Bottom can never be less than 0; it can be bigger than size-1 */
/* Top can never be greater than size-1; it can be less than 0 */
	bottom = MAX (low[0],low[1]);
	top = MIN (high[0], high[1]);
/* Test by use of the combine flag */
	if (!predterm->combine) {
	   for (i=bottom; i<=top; i++) {
		c1 = array[i];
		shChainElementAddByPos (outchain, c1, schema->name, TAIL,
			AFTER);
		}
	   }
	else {
	   if (high[0] - low[0] + 1 > 0) {
		for (i=low[0]; i<=high[0]; i++) {
		   c1 = array[i];
		   shChainElementAddByPos (outchain, c1, schema->name, TAIL,
			AFTER);
		   }
		}
	   if (high[1] - low[1] + 1 > 0) {
		for (i=low[1]; i<=high[1]; i++) {
		   c1 = array[i];
		   shChainElementAddByPos (outchain, c1, schema->name, TAIL,
			AFTER);
		   }
		}
	   }
	}
   return TCL_OK;
}

/* Top level subroutine to perform the select function */
/* Parse predicate list and check for errors */

int shTclChainSelect (Tcl_Interp *interp,
	CHAIN *chain, char *predList, CHAIN *outchain) {

   int i, j;
   int splitArgc;
   char **splitArgv;
   int splitSplitArgc;
   char **splitSplitArgv;
   PREDTERM **predVstr = NULL;
   int nalloc;
   PREDTERM *predterm;
   int ierr;
   TYPE type;
   const SCHEMA *schema;
   void **array;
   int size;
   CURSOR_T cursor;
   void *c1;

/* Get the chain type */
   type = shChainTypeDefine(chain);
/* Get the schema pointer */
   if ((schema = shSchemaGet (shNameGetFromType(type))) == NULL) {
	Tcl_SetResult (interp, "Can't find schema for chain type", TCL_STATIC);
	return TCL_ERROR;
	}
/* Predicates come in as a list of lists */
/* Split predicate list of lists */
   Tcl_SplitList (interp, predList, &splitArgc, &splitArgv);
/* Use same code ChainSelect except only one predicate is allowed */
   if (splitArgc == 1) {
	predVstr = (PREDTERM **)shMalloc (splitArgc * sizeof (PREDTERM *));
        } else {
	predVstr = NULL;
	free (splitArgv);
	Tcl_AppendResult (interp, " Only one predicate allowed for select",
		NULL);
	return TCL_ERROR;
	}
/* Allocate all buffers except the key data */
   nalloc = 0;
   for (i = 0; i < splitArgc; i++) {
/* Split predicate term list */
/* 1st element is name, 2nd element is operation, 3rd element is value */
	Tcl_SplitList (interp, splitArgv[i], &splitSplitArgc, &splitSplitArgv);
	if (splitSplitArgc != 3 && splitSplitArgc != 5) {
	   Tcl_AppendResult (interp, "Badly formed predicate: ",splitArgv[i],
		NULL);
/* Free the buffer */
	   free(splitSplitArgv);
	   break;
	   }
	predterm = (PREDTERM *)shMalloc(sizeof(PREDTERM));
	predVstr[i] = predterm;
/*Initialize other dynamic fields */
	predterm->key[0].data = NULL;
	predterm->key[1].data = NULL;
	predterm->attribute = NULL;
	predterm->program = NULL;
	nalloc++;
/* Predicate construction depends on whether we have 3 arg or 5 arg format */
	if (splitSplitArgc == 3) {
/* Conventional type predicate */
	   predterm->range = 0;
/* Fetch attribute parameters */
	   if (p_shTclParamGet(interp, schema, splitSplitArgv[0],
		&predterm->type, &type) != TCL_OK) {
/* Free the buffer */
		free(splitSplitArgv);
		break;
		}
/* Create a compiled program */
	   if ((predterm->program = shProgramCreateBySchema (schema,
		splitSplitArgv[0])) == NULL) {
/* Free the buffer */
		free(splitSplitArgv);
		ierr = TCL_ERROR;
		break;
		}
/* Set the operator type */
	   if (p_shTclOp(splitSplitArgv[1], &(predterm->oper[0])) != TCL_OK) {
		Tcl_AppendResult(interp, "Bad operatortype: ",splitSplitArgv[1],
		   NULL);
/* Free the buffer */
		free(splitSplitArgv);
		break;
		}
	   predterm->not[0] = 0;
/* Decode the data */
	   if (p_shTclKeyData(predterm->type, type, splitSplitArgv[2],
		&(predterm->key[0])) != TCL_OK) {
		Tcl_AppendResult(interp, "Error decoding field: ",
		   splitSplitArgv[2], NULL);
/* Free the buffer */
		free(splitSplitArgv);
		break;
		}
/* Store the attribute */
	   predterm->attribute = shMalloc(strlen(splitSplitArgv[0])+1);
	   strcpy (predterm->attribute, splitSplitArgv[0]);
	   } else {
/* Range type predicate */
	   predterm->range = 1;
/* First half */
/* Fetch attribute parameters */
	   if (p_shTclParamGet(interp, schema, splitSplitArgv[2], &predterm->type,
		&type) 	!= TCL_OK) {
/* Free the buffer */
		free(splitSplitArgv);
		break;
		}
/* Create a compiled program */
	   if ((predterm->program = shProgramCreateBySchema (schema,
		splitSplitArgv[2])) == NULL) {
/* Free the buffer */
		free(splitSplitArgv);
		ierr = TCL_ERROR;
		break;
		}
/* Set the operator type */
	   if (p_shTclOp(splitSplitArgv[1], &(predterm->oper[0])) != TCL_OK) {
		Tcl_AppendResult(interp, "Bad operatortype: ",splitSplitArgv[1],
		   NULL);
/* Free the buffer */
		free(splitSplitArgv);
		break;
		}
	   predterm->not[0] = 0;
/* Decode the data */
	   if (p_shTclKeyData(predterm->type, type, splitSplitArgv[0],
		&(predterm->key[0])) != TCL_OK) {
		Tcl_AppendResult(interp, "Error decoding field: ",
		   splitSplitArgv[0], NULL);
/* Free the buffer */
		free(splitSplitArgv);
		break;
		}
/* Second half */
/* Set the operator type */
	   if (p_shTclOp(splitSplitArgv[3], &(predterm->oper[1])) != TCL_OK) {
		Tcl_AppendResult(interp, "Bad operatortype: ",splitSplitArgv[3],
		   NULL);
/* Free the buffer */
		free(splitSplitArgv);
		break;
		}
	   predterm->not[1] = 0;
/* Decode the data */
	   if (p_shTclKeyData(predterm->type, type, splitSplitArgv[4],
		&(predterm->key[1])) != TCL_OK) {
		Tcl_AppendResult(interp, "Error decoding field: ",
		   splitSplitArgv[4], NULL);
/* Free the buffer */
		free(splitSplitArgv);
		break;
		}
/* Store the attribute */
	   predterm->attribute = shMalloc(strlen(splitSplitArgv[2])+1);
	   strcpy (predterm->attribute, splitSplitArgv[2]);
	   }
	free(splitSplitArgv);
	}

/* Free the buffer */
   free(splitArgv);

   if (i == splitArgc) {	/* Successful parse */
/* Convert chain to an array */
	size = shChainSize(chain);
	array = (void **)shMalloc(size*sizeof(void *));
	cursor = shChainCursorNew(chain);
	for (i=0; i<size; i++) {
	   c1 = shChainWalk(chain, cursor, NEXT);
	   array[i] = c1;
	   }
	shChainCursorDel (chain, cursor);
	ierr = tclVstrIndexSelect (interp, array, size, predVstr, schema,
	splitArgc, outchain);
	shFree(array);
	}
   else {
	ierr = TCL_ERROR;
	Tcl_AppendResult(interp, " Unable to parse predicate list", NULL);
	}
/* Free up storage */
   for (i=0; i<nalloc; i++) {
	predterm = ((PREDTERM **)predVstr)[i];
	if (predterm->attribute != NULL) shFree(predterm->attribute);
	if (predterm->program != NULL) shProgramDel (predterm->program);
	for (j=0; j<2; j++) {
	   if (predterm->key[j].data != NULL) {
		shFree (predterm->key[j].data);
		}
	   }
	shFree (predterm);
	}
   if (predVstr != NULL) shFree(predVstr);
   return ierr;
   }

/***************************************************************************/
/****************************** shTclChainSet  *****************************/
/***************************************************************************/

/* Set a specified element of all objects on a chain to a fixed value */

int shTclChainSet (Tcl_Interp *interp, CHAIN *chain, char *attribute,
	char *field) {
   TYPE type;
   VTYPE vtype;
   PROGRAM *program;
   const SCHEMA *schema;
   const SCHEMA *localSchema;
   void *c1;
   TYPE chainType;
   CURSOR_T cursor;
   void *attrPtr;

/* Get top level schema for chain type */
   chainType = shChainTypeDefine(chain);
   if ((schema = shSchemaGet(shNameGetFromType(chainType)))
	== NULL) {
	Tcl_SetResult (interp, "Unable to find schema for chain type",
	   TCL_STATIC);
	return TCL_ERROR;
	}
/* Fetch attribute parameters */
   if (p_shTclParamGet(interp, schema, attribute, &vtype,
	&type)  != TCL_OK) {
	Tcl_AppendResult (interp, "Cannot get schema for attribute ",
	   attribute, NULL);
	return TCL_ERROR;
	}

/* Compile program */
   if ((program = shProgramCreateBySchema (schema, attribute)) == NULL) {
	Tcl_SetResult (interp, "Cannot compile program for attribute",
	   TCL_STATIC);
	return TCL_ERROR;
	}

/* For use later, get the local schema */
   if ((localSchema = shSchemaGet(shNameGetFromType(type))) == NULL) {
	Tcl_SetResult (interp, "Cannot find schema for attribute", TCL_STATIC);
	return TCL_ERROR;
	}

/* I could decode the data, but I don't have an independent way to set
 * an attribute of arbitrary type.  Instead, I will use shElemSet, even 
 * though it is slower(since the field is decoded each time). */

/*   if (p_shTclKeyData(vtype, type, field, &keybuff) != TCL_OK) {
	Tcl_AppendResult(interp, "Error decoding field: ",
	splitSplitArgv[4], NULL);
	return TCL_ERROR;
	}
*/


/* Loop through the chain.  */

   cursor = shChainCursorNew(chain);
   while ((c1 = shChainWalk (chain, cursor, NEXT)) != NULL) {

/* Get pointer to the attribute in object c1. */
	if ((attrPtr = shProgramEval (c1, program)) == NULL)
		return TCL_ERROR;

/* Copy the attribute. */
	if(shElemSet(attrPtr, &localSchema->elems[0], field) != SH_SUCCESS) {
	   Tcl_SetResult (interp, "Error decoding the field value", TCL_STATIC);
	   return TCL_ERROR;
	   }
	} /* End of loop through all objects */

/* Cleanup */
   shChainCursorDel (chain, cursor);
/*   if (keybuff.data != NULL) shFree (keybuff.data); */
   shProgramDel(program);
   return TCL_OK;
   }

/***************************************************************************/
/****************************** shTclChainSetFromHandle ********************/
/***************************************************************************/

/* Like chainSet, but fixed value is passed as a handle, not a Tcl string */

int shTclChainSetFromHandle (Tcl_Interp *interp, CHAIN *chain, char *attribute,
	void *fieldPtr, TYPE fieldType) {
   TYPE type;
   const SCHEMA *schema;
   const SCHEMA *localSchema;
   PROGRAM *program;
   void *c1;
   TYPE chainType;
   CURSOR_T cursor;
   void *attrPtr;

/* Get top level schema for chain type */
   chainType = shChainTypeDefine(chain);
   if ((schema = shSchemaGet(shNameGetFromType(chainType)))
	== NULL) {
	Tcl_SetResult (interp, "Unable to find schema for chain type",
	   TCL_STATIC);
	return TCL_ERROR;
	}
   if ((program = shProgramCreateBySchema (schema, attribute)) == NULL) {
	Tcl_SetResult (interp, "Unable to compile program", TCL_STATIC);
	return TCL_ERROR;
	}
/* Fetch attribute type */
   type = program->type;
   if (program->nstar != 0 || program->ndim != 0) {
	Tcl_SetResult (interp, "Attribute is not a single element", TCL_STATIC);
	return TCL_ERROR;
	}
   if (type != fieldType) {
	Tcl_SetResult(interp, "Attribute and field types do not match",
	   TCL_STATIC);
	return TCL_ERROR;
	}
/* For use later, get the local schema */
   if ((localSchema = shSchemaGet(shNameGetFromType(type))) == NULL) {
	Tcl_SetResult (interp, "Cannot find schema for attribute", TCL_STATIC);
	return TCL_ERROR;
	}

/* Loop through the chain.  */

   cursor = shChainCursorNew(chain);

   while ((c1 = shChainWalk (chain, cursor, NEXT)) != NULL) {

/* Get pointer to the attribute in object c1. */
	if ((attrPtr = shProgramEval (c1, program)) == NULL)
		return TCL_ERROR;
/* Copy the attribute. */
/* Use memmove */
	memmove((char *)attrPtr, (const char *)fieldPtr,
		(size_t)localSchema->size);
	} /* End of loop through all objects */

/* Cleanup */
   shChainCursorDel (chain, cursor);
   shProgramDel(program);
   return TCL_OK;
   }

/***************************************************************************/
/****************************** shTclChainSort *****************************/
/***************************************************************************/

/* Sort a chain of objects.  More general and faster than Vijay's version,
** except that descending ordering is not yet in here
*/

int shTclChainSort (Tcl_Interp *interp, CHAIN *chain, char *attribute,
	int reverse) {
   int size;
   TYPE type;
   VTYPE vtype;
   const SCHEMA *schema;
/*   int (*qcompare)(const void *buff1, const void *buff2); */
   int (*compare)(BUFDESC *buff1, BUFDESC *buff2);
   BUFDESC *dbuff;
   void *c1;
   PROGRAM *program;
   TYPE chainType;
   CURSOR_T cursor;
   int iobj;
/*   void **ptr1, **ptr2; */
   int i;
   BUFDESC temp;

/* Get top level schema for chain type */
   chainType = shChainTypeDefine(chain);
   if ((schema = shSchemaGet(shNameGetFromType(chainType)))
	== NULL) {
	Tcl_SetResult (interp, "Unable to find schema for chain type",
	   TCL_STATIC);
	return TCL_ERROR;
	}

/* Fetch attribute parameters */
   if (p_shTclParamGet(interp, schema, attribute, &vtype, &type) != TCL_OK)
	return TCL_ERROR;
/* Create a compiled program */
   if ((program = shProgramCreateBySchema (schema, attribute)) == NULL)
	   return TCL_ERROR;
   size = shChainSize(chain);
   if (size <= 1) {
     /* Delete the compiled program we just created */
        shProgramDel(program);
	return TCL_OK;
   }

   if (vtype == CHAR) compare = p_shCharCompare;
   else if (vtype == UCHAR) compare = p_shUcharCompare;
   else if (vtype == STR) compare = p_shStrCompare;
   else if (vtype == SHORT) compare = p_shShortCompare;
   else if (vtype == USHORT) compare = p_shUshortCompare;
   else if (vtype == INT) compare = p_shIntCompare;
   else if (vtype == UINT) compare = p_shUintCompare;
   else if (vtype == LONG) compare = p_shLongCompare;
   else if (vtype == ULONG) compare = p_shUlongCompare;
   else if (vtype == FLOAT) compare = p_shFltCompare;
   else if (vtype == DOUBLE) compare = p_shDblCompare;
   else if (vtype == EENUM) compare = p_shEnumCompare;
   else {
	Tcl_AppendResult (interp, "Invalid type. Bizarre.", NULL);
     /* Delete the compiled program we just created */
        shProgramDel(program);	
	return TCL_ERROR;
	}

/* Loop through the chain.  Create an array of BUFDESCRs.  These will contain
 * pointers to the data fields and back to the top of each object.  This
 * takes more storage than storing just an array of pointers to each object,
 * but execution is much faster on large chains.
*/

/* Allocate space for the buffers */
   dbuff = (BUFDESC *)shMalloc(size*sizeof(BUFDESC));
   cursor = shChainCursorNew(chain);

/* Now do the selects */
   iobj=0;
   while ((c1 = shChainWalk (chain, cursor, NEXT)) != NULL) {

     if ((dbuff[iobj].data = shProgramEval (c1, program)) == NULL) {
                /* Delete the compiled program we just created 
		   as well as the buffer space. */
                shProgramDel(program);
		shChainCursorDel (chain, cursor);
		shFree (dbuff);
		return TCL_ERROR;
                }
	dbuff[iobj].source = c1;
	iobj++;
	} /* End of loop through all objects */

   qsort ((void *)dbuff, size, sizeof (BUFDESC), 
                  (int (*)(const void *, const void *)) compare);

/* Reverse ordering if requested */
   if (reverse) {
	for (i=0; i<size/2; i++) {
	   temp = dbuff[i];
	   dbuff[i] = dbuff[size-i-1];
	   dbuff[size-i-1] = temp;
	   }
	}

/* Now clean out the chain and reconstruct.  Cleaning out is safer than
  than deleting, since we don't have to worry about rebinding handles */

/* We should be able to run the cursor around again! */
   while (shChainWalk (chain, cursor, NEXT) != NULL) {
	shChainElementRemByCursor (chain, cursor);
	}
   shChainCursorDel (chain, cursor);

   for (iobj=0; iobj<size; iobj++) {
	shChainElementAddByPos (chain, dbuff[iobj].source,
	   shNameGetFromType(chainType), TAIL, AFTER);
	}
   shFree (dbuff);
/* Delete program */
   if (program != NULL) shProgramDel(program);
   return TCL_OK;
   }

/***************************************************************************/
/****************************** shTclChainTransXY **************************/
/***************************************************************************/

/* Apply a trans to all objects in a chain.  The transformation is done
 * in place (not ideal but it gets the job done) */

/* x(out) = a + b*x + c*y
** y(out) = d + e*x + f*y
*/

int shTclChainTransXY (Tcl_Interp *interp, CHAIN *chain, char *xattr,
	char *yattr, double a, double b, double c, double d, double e,
	double f) {
   TYPE xtype, ytype;
   VTYPE xvtype, yvtype;
   const SCHEMA *schema;
   const SCHEMA *xlSchema, *ylSchema;
   PROGRAM *xprogram;
   PROGRAM *yprogram;
   void *c1;
   TYPE chainType;
   CURSOR_T cursor;
   void *xattrPtr, *yattrPtr;
   double x, y, result;
   double (*xextr)(void *);
   double (*yextr)(void *);
   void (*xset)(void *, double);
   void (*yset)(void *, double);

/* Get top level schema for chain type */
   chainType = shChainTypeDefine(chain);
   if ((schema = shSchemaGet(shNameGetFromType(chainType)))
	== NULL) {
	Tcl_SetResult (interp, "Unable to find schema for chain type",
	   TCL_STATIC);
	return TCL_ERROR;
	}
/* Fetch attribute parameters */
   if (p_shTclParamGet(interp, schema, xattr, &xvtype,
	&xtype)  != TCL_OK) {
	Tcl_AppendResult (interp, "Cannot get schema for attribute ",
	   xattr, NULL);
	return TCL_ERROR;
	}
   if (p_shTclParamGet(interp, schema, yattr, &yvtype,
	&ytype)  != TCL_OK) {
	Tcl_AppendResult (interp, "Cannot get schema for attribute ",
	   yattr, NULL);
	return TCL_ERROR;
	}
/* Create programs */
   if ((xprogram = shProgramCreateBySchema (schema, xattr)) == NULL) {
	Tcl_SetResult (interp, "Unable to compile program for xattr",
	   TCL_STATIC);
	return TCL_ERROR;
	}
   if ((yprogram = shProgramCreateBySchema (schema, yattr)) == NULL) {
	Tcl_SetResult (interp, "Unable to compile program for yattr",
	   TCL_STATIC);
	return TCL_ERROR;
	}
/* For use later, get the local schema */
   if ((xlSchema = shSchemaGet(shNameGetFromType(xtype))) == NULL) {
	Tcl_SetResult (interp, "Cannot find schema for attribute", TCL_STATIC);
	return TCL_ERROR;
	}
   if ((ylSchema = shSchemaGet(shNameGetFromType(ytype))) == NULL) {
	Tcl_SetResult (interp, "Cannot find schema for attribute", TCL_STATIC);
	return TCL_ERROR;
	}

/* Get proper comparison function */

        if (xvtype == SHORT) {xextr = p_shShortExtract; xset = p_shShortSet;}
        else if (xvtype == USHORT) {xextr = p_shUshortExtract; xset = p_shUshortSet;}
        else if (xvtype == INT) {xextr = p_shIntExtract; xset = p_shIntSet; }
        else if (xvtype == UINT) {xextr = p_shUintExtract; xset = p_shUintSet;}
        else if (xvtype == LONG) {xextr = p_shLongExtract; xset = p_shLongSet;}
        else if (xvtype == ULONG) {xextr = p_shUlongExtract; xset = p_shUlongSet;}
        else if (xvtype == FLOAT) {xextr = p_shFltExtract; xset = p_shFltSet;}
        else if (xvtype == DOUBLE) {xextr = p_shDblExtract; xset = p_shDblSet;}
        else {
           Tcl_AppendResult (interp, "Invalid x type. Bizarre.", NULL);
           return TCL_ERROR;
           }

        if (yvtype == SHORT) {yextr = p_shShortExtract; yset = p_shShortSet;}
        else if (yvtype == USHORT) {yextr = p_shUshortExtract; yset = p_shUshortSet;}
        else if (yvtype == INT) {yextr = p_shIntExtract; yset = p_shIntSet; }
        else if (yvtype == UINT) {yextr = p_shUintExtract; yset = p_shUintSet;}
        else if (yvtype == LONG) {yextr = p_shLongExtract; yset = p_shLongSet;}
        else if (yvtype == ULONG) {yextr = p_shUlongExtract; yset = p_shUlongSet;}
        else if (yvtype == FLOAT) {yextr = p_shFltExtract; yset = p_shFltSet;}
        else if (yvtype == DOUBLE) {yextr = p_shDblExtract; yset = p_shDblSet;}
        else {
           Tcl_AppendResult (interp, "Invalid x type. Bizarre.", NULL);
           return TCL_ERROR;
           }

/* Loop through the chain.  */

   cursor = shChainCursorNew(chain);

   while ((c1 = shChainWalk (chain, cursor, NEXT)) != NULL) {

/* Get pointer to the attribute in object c1. */
	if ((xattrPtr = shProgramEval(c1, xprogram)) == NULL)
	   return TCL_ERROR;
	x = xextr(xattrPtr);
	if ((yattrPtr = shProgramEval(c1, yprogram)) == NULL)
	   return TCL_ERROR;
	y = yextr(yattrPtr);
	result = a + b*x + c*y;
	xset (xattrPtr, result);
	result = d + e*x + f*y;
	yset (yattrPtr, result);
	} /* End of loop through all objects */

/* Cleanup */
   shChainCursorDel (chain, cursor);
   shProgramDel(xprogram);
   shProgramDel(yprogram);
   return TCL_OK;
   }

/*****************************************************************************/
/**************************** tsChainTransZXY ********************************/
/*****************************************************************************/

/* Perform a computation on elements of a chain.  The calculation is
** z = a + b*x + c*y   where z, x, y are elements of objects on the chain
*/

int shTclChainTransZXY (Tcl_Interp *interp, CHAIN *chain, char *zattr,
	char *xattr, char *yattr, double a, double b, double c) {

   TYPE xtype, ytype, ztype;
   VTYPE xvtype, yvtype, zvtype;
   const SCHEMA *schema;
   const SCHEMA *xlSchema, *ylSchema, *zlSchema;
   PROGRAM *xprogram;
   PROGRAM *yprogram;
   PROGRAM *zprogram;
   void *c1;
   TYPE chainType;
   CURSOR_T cursor;
   void *xattrPtr, *yattrPtr, *zattrPtr;
   double x, y, result;
   double (*xextr)(void *);
   double (*yextr)(void *);
   double (*zextr)(void *);
   void (*xset)(void *, double);
   void (*yset)(void *, double);
   void (*zset)(void *, double);

/* Get top level schema for chain type */
   chainType = shChainTypeDefine(chain);
   if ((schema = shSchemaGet(shNameGetFromType(chainType)))
	== NULL) {
	Tcl_SetResult (interp, "Unable to find schema for chain type",
	   TCL_STATIC);
	return TCL_ERROR;
	}
/* Fetch attribute parameters */
   if (p_shTclParamGet(interp, schema, zattr, &zvtype,
	&ztype)  != TCL_OK) {
	Tcl_AppendResult (interp, "Cannot get schema for attribute ",
	   zattr, NULL);
	return TCL_ERROR;
	}
   if (p_shTclParamGet(interp, schema, xattr, &xvtype,
	&xtype)  != TCL_OK) {
	Tcl_AppendResult (interp, "Cannot get schema for attribute ",
	   xattr, NULL);
	return TCL_ERROR;
	}
   if (p_shTclParamGet(interp, schema, yattr, &yvtype,
	&ytype)  != TCL_OK) {
	Tcl_AppendResult (interp, "Cannot get schema for attribute ",
	   yattr, NULL);
	return TCL_ERROR;
	}

/* Create programs */
   if ((xprogram = shProgramCreateBySchema (schema, xattr)) == NULL) {
	Tcl_SetResult (interp, "Unable to compile program for xattr",
	   TCL_STATIC);
	return TCL_ERROR;
	}
   if ((yprogram = shProgramCreateBySchema (schema, yattr)) == NULL) {
	Tcl_SetResult (interp, "Unable to compile program for yattr",
	   TCL_STATIC);
	return TCL_ERROR;
	}
   if ((zprogram = shProgramCreateBySchema (schema, zattr)) == NULL) {
	Tcl_SetResult (interp, "Unable to compile program for zattr",
	   TCL_STATIC);
	return TCL_ERROR;
	}

/* For use later, get the local schema */
   if ((zlSchema = shSchemaGet(shNameGetFromType(ztype))) == NULL) {
	Tcl_SetResult (interp, "Cannot find schema for attribute", TCL_STATIC);
	return TCL_ERROR;
	}
   if ((xlSchema = shSchemaGet(shNameGetFromType(xtype))) == NULL) {
	Tcl_SetResult (interp, "Cannot find schema for attribute", TCL_STATIC);
	return TCL_ERROR;
	}
   if ((ylSchema = shSchemaGet(shNameGetFromType(ytype))) == NULL) {
	Tcl_SetResult (interp, "Cannot find schema for attribute", TCL_STATIC);
	return TCL_ERROR;
	}

/* Get proper comparison function */

	if (zvtype == SHORT) {zextr = p_shShortExtract; zset = p_shShortSet;}
	else if (zvtype == USHORT) {zextr = p_shUshortExtract; zset = p_shUshortSet;}
	else if (zvtype == INT) {zextr = p_shIntExtract; zset = p_shIntSet; }
	else if (zvtype == UINT) {zextr = p_shUintExtract; zset = p_shUintSet;}
	else if (zvtype == LONG) {zextr = p_shLongExtract; zset = p_shLongSet;}
	else if (zvtype == ULONG) {zextr = p_shUlongExtract; zset = p_shUlongSet;}
	else if (zvtype == FLOAT) {zextr = p_shFltExtract; zset = p_shFltSet;}
	else if (zvtype == DOUBLE) {zextr = p_shDblExtract; zset = p_shDblSet;}
	else {
	   Tcl_AppendResult (interp, "Invalid z type. Bizarre.", NULL);
	   return TCL_ERROR;
	   }

	if (xvtype == SHORT) {xextr = p_shShortExtract; xset = p_shShortSet;}
	else if (xvtype == USHORT) {xextr = p_shUshortExtract; xset = p_shUshortSet;}
	else if (xvtype == INT) {xextr = p_shIntExtract; xset = p_shIntSet; }
	else if (xvtype == UINT) {xextr = p_shUintExtract; xset = p_shUintSet;}
	else if (xvtype == LONG) {xextr = p_shLongExtract; xset = p_shLongSet;}
	else if (xvtype == ULONG) {xextr = p_shUlongExtract; xset = p_shUlongSet;}
	else if (xvtype == FLOAT) {xextr = p_shFltExtract; xset = p_shFltSet;}
	else if (xvtype == DOUBLE) {xextr = p_shDblExtract; xset = p_shDblSet;}
	else {
	   Tcl_AppendResult (interp, "Invalid x type. Bizarre.", NULL);
	   return TCL_ERROR;
	   }

	if (yvtype == SHORT) {yextr = p_shShortExtract; yset = p_shShortSet;}
	else if (yvtype == USHORT) {yextr = p_shUshortExtract; yset = p_shUshortSet;}
	else if (yvtype == INT) {yextr = p_shIntExtract; yset = p_shIntSet; }
	else if (yvtype == UINT) {yextr = p_shUintExtract; yset = p_shUintSet;}
	else if (yvtype == LONG) {yextr = p_shLongExtract; yset = p_shLongSet;}
	else if (yvtype == ULONG) {yextr = p_shUlongExtract; yset = p_shUlongSet;}
	else if (yvtype == FLOAT) {yextr = p_shFltExtract; yset = p_shFltSet;}
	else if (yvtype == DOUBLE) {yextr = p_shDblExtract; yset = p_shDblSet;}
	else {
	   Tcl_AppendResult (interp, "Invalid y type. Bizarre.", NULL);
	   return TCL_ERROR;
	   }

/* Loop through the chain.  */

   cursor = shChainCursorNew(chain);

   while ((c1 = shChainWalk (chain, cursor, NEXT)) != NULL) {

/* Get pointer to the attribute in object c1. */
	if ((xattrPtr = shProgramEval(c1, xprogram)) == NULL)
	   return TCL_ERROR;
	if ((yattrPtr = shProgramEval(c1, yprogram)) == NULL)
	   return TCL_ERROR;
	if ((zattrPtr = shProgramEval(c1, zprogram)) == NULL)
	   return TCL_ERROR;
	x = xextr(xattrPtr);
	y = yextr(yattrPtr);
	result = a + b*x + c*y;
	zset (zattrPtr, result);
	} /* End of loop through all objects */

/* Cleanup */
   shChainCursorDel (chain, cursor);
   shProgramDel(xprogram);
   shProgramDel(yprogram);
   shProgramDel(zprogram);
   return TCL_OK;
   }


