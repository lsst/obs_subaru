/*****************************************************************************
******************************************************************************
 *<AUTO>
 * FILE: tclMasks.c
 *
 *<HTML>
 * This file contains the tcl interface to the expression evaluator.
 *</HTML>
 *</AUTO>
 *
 * ENTRY POINT          SCOPE   DESCRIPTION
 * -------------------------------------------------------------------------
 * shTclSEval           local    Call the expression evaluator from tcl.
 * shTclSEvalDeclare    public   Declare Tcl expression evaluator verbs.
 *
 * ENVIRONMENT:
 *    Ansi C (and, coming soon to a theatre, oops compiler near you, C++)
 *
 * AUTHOR
 *   Philippe Canal
 *   Creation date: April 27, 1996
 */

#include <string.h>
#include <dervish.h>
#include <shCSEval.h>

/*
 * Static (private) function declarations (used only in this source file)
 */
static int shTclSEval(ClientData clientdata,
			 Tcl_Interp* interp,
			 int argc,
			 char* argv[]);

#ifdef HTML
<HTML>
<HEAD>
<TITLE>TCL Database Verbs</TITLE>
<BODY>
<H1>TCL Database Verbs</H1>

<MENU>
<LI> <A HREF="#exprEval">exprEval</A>
</MENU>
#endif


/*============================================================================  
**============================================================================
** Routine shTclSEval
**
** <AUTO>
** TCL VERB:
**      exprEval
**
** <HTML>
** Evaluation an expression following the grammar described in 
** the <A HREF="TBA.html">"introduction to the expression evaluator.<\A>
** </HTML>
** TCL SYNTAX:
**     exprEval &lt;expression&gt; [-start start] [-end end]
** 
** RETURNS:
**     TCL_OK	Successful completion.
**     TCL_ERROR   Error occurred.  An error message is returned.
**
**</AUTO>
**============================================================================
*/

#ifdef HTML
<H2><A Name="exprEval">exprEval</A></H2>
Evaluate an expression.

<PRE>
TCL SYNTAX:
    exprEval &lt;expression&gt; [-start start] [-end end]

RETURNS:
    TCL_OK	Successful completion.
    TCL_ERROR   Error occurred.  An error message is returned.
</PRE>
#endif



static char* SEvalCmd = "exprEval";
static int SEvalFlg = FTCL_ARGV_NO_LEFTOVERS;
static ftclArgvInfo SEvalTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Evaluate an expression with optional looping\n"},
   {"<expression>", FTCL_ARGV_STRING, NULL, NULL,
    "String holding the expression"},
   {"-start", FTCL_ARGV_INT, NULL, NULL,
    "Starting value for loop index i"},
   {"-end", FTCL_ARGV_INT, NULL, NULL,
    "Ending value for loop index i"},
   {"-debug", FTCL_ARGV_INT, NULL, NULL,
    "Dump the code store without execution"},
   {"-time", FTCL_ARGV_INT, NULL, NULL,
    "measure the time in execution"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int shTclSEval(ClientData clientdata,
			 Tcl_Interp* interp,
			 int argc,
			 char* argv[])
{
/*  // Parse the arguments */
  char* expression = "";
  int start = 0;
  int end = 0;
  int debug = 0;
  int mtime = 0;
  int retStatus;

  SEvalTbl[1].dst = &expression;
  SEvalTbl[2].dst = &start;
  SEvalTbl[3].dst = &end;
  SEvalTbl[4].dst = &debug;
  SEvalTbl[5].dst = &mtime;
  retStatus = shTclParseArgv(interp, &argc, argv, SEvalTbl,
				 SEvalFlg, SEvalCmd);
  if (retStatus != FTCL_ARGV_SUCCESS) return retStatus;

  if (shSEval(interp, expression, start, end, debug, mtime))
    {
      Tcl_ResetResult(interp);
      shTclInterpAppendWithErrStack(interp);
      return TCL_ERROR;
    }
  return TCL_OK;
}

/*============================================================================  
**============================================================================
**
** ROUTINE: shTclSEvalDeclare
**
** DESCRIPTION:
**	Declare TCL verbs for this module.
**
** RETURN VALUES:
**	N/A
**
**============================================================================
*/

void shTclSEvalDeclare(Tcl_Interp *interp)
{
  char* tclHelpFacil = "shSe";

  shTclDeclare(interp, SEvalCmd, shTclSEval,
	       (ClientData) 0, (Tcl_CmdDeleteProc *) NULL, tclHelpFacil,
	       shTclGetArgInfo(interp, SEvalTbl,
			       SEvalFlg, SEvalCmd),
	       shTclGetUsage(interp, SEvalTbl,
			     SEvalFlg, SEvalCmd));
}

