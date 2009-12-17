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
 *    privateAvg x y -flag
 *  where x and y are floats to be averaged
 */

int privateAvg
  (ClientData clientData, Tcl_Interp *interp, int argc, char**argv)
{

/* declare variables we will use */

  char *formalCmd="x y";       /* this is the prototype for the arguments */
  double x, y;                 /* these are the two input values */
  float average;               /* the answer */
  char answer[20];             /* the answer, as a string, returned to TCL */

/* parse information from the command */

/* create and initialize a new interpreter */
  ftclParseSave("ftclParsePrivateAvg");

/* parse the command line to the arguments */
if (ftclFullParseArg(formalCmd, argc, argv))
   {

    /* successful parse */

    /* get the arguments we need */
    x = ftclGetDouble("x");
    y = ftclGetDouble("y");

    /* do the calculation */
    average = (x+y)/2.0;

    /* send the answer back to tcl */
    sprintf(answer,"%f",average);
    Tcl_SetResult(interp, answer, TCL_VOLATILE);

    /* clean up and get out of here */
    ftclParseRestore("ftclParsePrivateAvg");
    return (TCL_OK);

   } /* successful parse */
 else
   {

    /* not a successful parse */
    Tcl_SetResult(interp, "Usage: privateAvg x y", TCL_VOLATILE);
    ftclParseRestore("ftclParsePrivateAvg");
    return (TCL_ERROR);

   } /* not a successful parse */

} /* privateAvg */


/*
 * This is the declaration stage, called by the main program
 */

void privateTclDeclare(Tcl_Interp *interp)

{

Tcl_CreateCommand(interp, "privateAvg", privateAvg, (ClientData) 0,
		  (Tcl_CmdDeleteProc *) NULL);

}













