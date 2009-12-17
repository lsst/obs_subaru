/*
 *
 * This is privateTclVerbs.c to illustrate how to write a TCL verb in c
 * 
 * Chris Stoughton   May 11, 1993
 *
 */
#include <stdio.h>        /* needed for the sprintf function */
#include <termio.h>       
#include <sys/types.h>
#include "dervish.h"


/*
 * This is the code that does the actual work:
 *
 * The format of the verb is:
 *    privateRegAvg handle 
 *
 */

int privateRegAvg
  (ClientData clientData, Tcl_Interp *interp, int argc, char**argv)
{

/* declare variables we will use */

  char *formalCmd="handle";       /* this is the prototype for the arguments */
  REGION *regPtr;                 /* region pointer */
  char  *regName;                 /* region name */	
  int   i, j;
  int   nrow, ncol;
  float sum;
  float average;               /* the answer */
  char answer[20];             /* the answer, as a string, returned to TCL */

/* parse information from the command */

/* create and initialize a new interpreter */
  ftclParseSave("ftclParsePrivateRegAvg");

/* parse the command line to the arguments */
if (ftclFullParseArg(formalCmd, argc, argv))
   {

    /* successful parse */

    /* get the arguments we need */
    regName = ftclGetStr("handle");
    if (shTclRegAddrGetFromName(interp, regName, &regPtr) != TCL_OK)
      {return (TCL_ERROR);}

    /* get the number of rows and columns in the image */
    nrow = regPtr->nrow;
    ncol = regPtr->ncol;

    /* do the calculation */
    sum =0;
    for (i=0; i<nrow; i++)
       {
       for (j=0; j<ncol; j++)
          {
          sum += shRegPixGetAsDbl(regPtr, i, j);
          }
       }
    average = sum/(nrow*ncol);

    /* send the answer back to tcl */
    sprintf(answer,"%f",average);
    Tcl_SetResult(interp, answer, TCL_VOLATILE);

    /* clean up and get out of here */
    ftclParseRestore("ftclParsePrivateRegAvg");
    return (TCL_OK);

   } /* successful parse */
 else
   {

    /* not a successful parse */
    Tcl_SetResult(interp, "Usage: privateRegAvg handle", TCL_VOLATILE);
    ftclParseRestore("ftclParsePrivateRegAvg");
    return (TCL_ERROR);

   } /* not a successful parse */

} /* privateRegAvg */

/*
 * This is the code that does the actual work:
 *
 * The format of the verb is:
 *    privateRegAvgFast handle 
 *
 */

int privateRegAvgFast
  (ClientData clientData, Tcl_Interp *interp, int argc, char**argv)
{

/* declare variables we will use */

  char *formalCmd="handle";       /* this is the prototype for the arguments */
  REGION *regPtr;                 /* region pointer */
  char  *regName;                 /* region name */	
  int   i, j;
  int   nrow, ncol;
  char answer[20];             /* the answer, as a string, returned to TCL */
  float sum=0;
  float average=0;             /* the answer */

/* parse information from the command */

/* create and initialize a new interpreter */
  ftclParseSave("ftclParsePrivateAvgFast");

/* parse the command line to the arguments */
if (ftclFullParseArg(formalCmd, argc, argv))
   {

    /* successful parse */

    /* get the arguments we need */
    regName = ftclGetStr("handle");
    if (shTclRegAddrGetFromName(interp, regName, &regPtr) != TCL_OK)
      {return (TCL_ERROR);}

    /* get the number of rows and columns in the image */
    nrow = regPtr->nrow;
    ncol = regPtr->ncol;

    switch(regPtr->type)
      {
      case TYPE_U8:
         for (i=0; i<nrow; i++)
            {for (j=0; j<ncol; j++) {sum += regPtr->rows_u8[i][j];}}
         break;
      case TYPE_S8:
         for (i=0; i<nrow; i++)
            {for (j=0; j<ncol; j++) {sum += regPtr->rows_s8[i][j];}}
         break;
      case TYPE_U16:
         for (i=0; i<nrow; i++)
            {for (j=0; j<ncol; j++) {sum += regPtr->rows_u16[i][j];}}
         break;
      case TYPE_S16:
         for (i=0; i<nrow; i++)
            {for (j=0; j<ncol; j++) {sum += regPtr->rows_s16[i][j];}}
         break;
      case TYPE_U32:
         for (i=0; i<nrow; i++)
            {for (j=0; j<ncol; j++) {sum += regPtr->rows_u32[i][j];}}
         break;
      case TYPE_S32:
         for (i=0; i<nrow; i++)
            {for (j=0; j<ncol; j++) {sum += regPtr->rows_s32[i][j];}}
         break;
      case TYPE_FL32:
         for (i=0; i<nrow; i++)
            {for (j=0; j<ncol; j++) {sum += regPtr->rows_fl32[i][j];}}
         break;
      default:
         Tcl_SetResult(interp, "Serious Internal error", TCL_VOLATILE);
         return(TCL_ERROR);
         break;
      }
 
    /* do the average calculation */
    average = sum/(nrow*ncol);

    /* send the answer back to tcl */
    sprintf(answer,"%f",average);
    Tcl_SetResult(interp, answer, TCL_VOLATILE);

    /* clean up and get out of here */
    ftclParseRestore("ftclParsePrivateAvgFast");
    return (TCL_OK);

   } /* successful parse */
 else
   {

    /* not a successful parse */
    Tcl_SetResult(interp, "Usage: privateRegAvgFast handle", TCL_VOLATILE);
    ftclParseRestore("ftclParsePrivateAvgFast");
    return (TCL_ERROR);

   } /* not a successful parse */

} /* privateRegAvgFast */


/*
 * This is the declaration stage, called by the main program
 */

void privateTclDeclare(Tcl_Interp *interp)

{

Tcl_CreateCommand(interp, "privateRegAvg", privateRegAvg, (ClientData) 0,
		  (Tcl_CmdDeleteProc *) NULL);

Tcl_CreateCommand(interp, "privateRegAvgFast", privateRegAvgFast,
                  (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
}













