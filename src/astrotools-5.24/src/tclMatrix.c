/* <AUTO>
   FILE: tclMatrix
   ABSTRACT:
<HTML>
   Routines for using vectors as matricies
</HTML>
   </AUTO>
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>
#include "dervish.h"
#include "tcl.h"
#include "shCUtils.h"
#include "shCErrStack.h"
#include "shTclHandle.h"
#include "tclMath.h"
#include "atMatrix.h"


/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Given a list of vectors that constitute the rows of a square matrix,
 * invert the matrix, placing the rows on the inverted matrix in the vectors.
 *
 */

static ftclArgvInfo atVectorMatrixInvertTable[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, 
 "given a list of vectors that comprise the rows of a matrix, invert the matrix,\
	and return the inverted matrix as rows in the same vectors."},
   {"<vectorList>", FTCL_ARGV_STRING, NULL, NULL, "list of vectors"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static char *atVectorMatrixInvert_name = "vMatrixInvert";


static int
atVectorMatrixInvert(ClientData cd, Tcl_Interp *interp, int argc, char **argv)
{

   char *vList;		
   int ac;
   char **av;
   int i, j;
   VECTOR *vector = NULL;
   VECTOR_TYPE **array = NULL;
   VECTOR_TYPE *b = NULL;	/* atMatrixInvert can solve array*b=x, but we don't
			   use that capability, so we set b = NULL */
   int returnFlag;

   atVectorMatrixInvertTable[1].dst = &vList ;

   if((returnFlag = shTclParseArgv(
        		interp, &argc, argv, atVectorMatrixInvertTable, 0, 
			atVectorMatrixInvert_name)) != FTCL_ARGV_SUCCESS) {
        return(returnFlag);
  }


   /* Make the ascii list accessible to C */
   if(Tcl_SplitList(interp, vList, &ac, &av) == TCL_ERROR) {
      shErrStackPush("Error parsing input list of vectors");
      return(-1);
   }

  /*
   * 	Get first vector, check size
   */
   
   if (shTclAddrGetFromName (interp, av[0], (void **) &vector, "VECTOR") 
		!= TCL_OK) {
    Tcl_SetResult(interp, "bad VECTOR name",TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (vector->dimen != ac) {
	Tcl_SetResult(interp, 
		"vMatrixInvert: matrix must be square.",TCL_VOLATILE);
     	return TCL_ERROR;

  }

  /*
   *	allocate array memory
   */

   array = shMalloc(ac*sizeof(VECTOR_TYPE *));

  /*
   *	adjust pointers to make the matrix elements be the vector's elements
   */

   for(i = 0; i < ac; i++) {

   	if (shTclAddrGetFromName (interp, av[i], (void **) &vector, "VECTOR") 
			!= TCL_OK) {
	 	free(av);
	 	shFree(array);
    		Tcl_SetResult(interp, 
			"vMatrixInvert: bad VECTOR name",TCL_VOLATILE);
    		return TCL_ERROR;
  	}

  	if (vector->dimen != ac) {
	 	free(av);
	 	shFree(array);
		Tcl_SetResult(interp, 
			"vMatrixInvert: matrix must be square.",TCL_VOLATILE);
     		return TCL_ERROR;
  	}
	array[i] = vector->vec;
   }


  /*
   *	invert matrix
   */
#if 0
   shAssert(sizeof(VECTOR_TYPE) == sizeof(float));
   returnFlag = atMatrixInvert(array, b, ac);
#else
   shAssert(sizeof(VECTOR_TYPE) == sizeof(double));
   returnFlag = atDMatrixInvert(array, b, ac);
#endif

  /*
   *	clean up
   */
   shFree(array);
   free(av);
   
   if (returnFlag != 0) { 
	Tcl_SetResult(interp, 
		"vMatrixInvert: singular matrix.",TCL_VOLATILE);
     		return TCL_ERROR;
   }
   return(TCL_OK);
}


/*----------------------------------------------------------------------------*/
void atTclMatrixDeclare(Tcl_Interp *interp)
{

    static char *module = "astrotools";

   shTclDeclare(interp,"vMatrixInvert", 
		(Tcl_CmdProc *)atVectorMatrixInvert, 
		(ClientData) 0, (Tcl_CmdDeleteProc *)NULL, module,
		shTclGetArgInfo(interp, atVectorMatrixInvertTable, 0,
			atVectorMatrixInvert_name),
		shTclGetUsage(interp, atVectorMatrixInvertTable, 0,
			atVectorMatrixInvert_name)
		);
}
