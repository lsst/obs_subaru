/*++
 * FACILITY:	DERVISH
 *
 * ABSTRACT:	Tcl interfaces to error stack routines.
 *
 *   shTclErrStackPush		Format and push          message onto err stack.
 *   shTclErrStackGetNext	Get next youngest (LIFO) message off  err stack.
 *   shTclErrStackGetEarliest	Get      youngest (LIFO) message off  err stack.
 *   shTclErrStackClear		Clear the error stack.
 *   shTclErrStackOverflow	Return count of error stack overflows.
 *   shErrStackTclTrace         Invoked during Tcl_Eval tracing.
 *
 *   shTclInterpAppendWithErrStack	Display error stack messages.
 *   shTclErrStackDeclare	Declare error stack Tcl commands.
 *
 * ENVIRONMENT:	ANSI C.
 *		tclErrStack.c
 *
 * AUTHOR:	Tom Nicinski, Creation date: 28-Oct-1993
 *--
 ******************************************************************************/

/******************************************************************************/
/*
 * Declarations.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include        <alloca.h>

#include	"ftcl.h"

#include        "region.h"	/* For shTclRegSupport.h      */
#include	"shTclUtils.h"  /* For shTclDeclare ()        */
#include	"shCErrStack.h"
#include        "prvt/errStackP.h"              /* For "errStack" definition */
#include        "shTclErrStack.h"

#include	"shCSao.h"	/* For shTclVerbs.h           */
#include	"shTclVerbs.h"	/* Control C++ name mangling  */
				/* of shTclErrStackDeclare () */

/******************************************************************************/
/*
 * Define CLI commands.
 */

static char	*shTclErrStackPushHelp        = "Format string and push onto error stack",
		*shTclErrStackPushUse         = "USAGE: errStackPush <format> [arg1 [...]]";
static char	*shTclErrStackGetNextHelp     = "Get next youngest (LIFO) message off error stack",
		*shTclErrStackGetNextUse      = "USAGE: errStackGetNext";
static char	*shTclErrStackGetEarliestHelp = "Get youngest message off error stack",
		*shTclErrStackGetEarliestUse  = "USAGE: errStackGetEarliest";
static char	*shTclErrStackClearHelp       = "Clear error stack",
		*shTclErrStackClearUse        = "USAGE: errStackClear";
static char	*shTclErrStackOverflowHelp    = "Return count of error stack overflows",
		*shTclErrStackOverflowUse     = "USAGE: errStackOverflow";

/******************************************************************************/

static int	 shTclErrStackPush
   (
   ClientData	 clientData,		/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	*interp,		/* INOUT: Tcl interpreter structure   */
   int		 argc,			/* IN:    Tcl argument count          */
   char		*argv []		/* IN:    Tcl arguments               */
   )

/*++
 * ROUTINE:	 shTclErrStackPush
 *
 * DESCRIPTION:
 *
 *	Tcl command routine jacketing shErrStackPush ().
 *
 *	The formatting of the error string is done using Tcl_Eval () rather than
 *	in shErrStackPush () itself.  This is simpler (and safer) than parsing
 *	the format string conversion specifications and converting arguments to
 *	binary so that shErrStackPush () can simply do its sprintf ().
 *
 * RETURN VALUES:
 *
 *   TCL_OK		Success.
 *			The error string was formatted and pushed onto the error
 *			stack.  No indication is given if the resultant string
 *			was truncated due to error stack space limitations. The
 *			interp result string is null.
 *
 *   TCL_ERROR		Failure.
 *			The error string was not placed onto the error stack.
 *			The interp result string contains an error message.
 *
 * SIDE EFFECTS:
 *
 *   o	Not all sprintf () conversion spefications are handled by Tcl's format
 *	command.  See the Tcl documentation/man page.
 *
 * SYSTEM CONCERNS:
 *
 *   o	The Tcl interpreter result string is assumed to be empty (null) when
 *	this routine is called.
 *--
 ******************************************************************************/

   {	/* shTclErrStackPush */

   /*
    * Declarations.
    */

                  int	 lcl_status = TCL_OK;
                  char	*lcl_fmt;
   unsigned long  int	 lcl_fmtSize;
   unsigned long  int	 lcl_fmtLen;
   unsigned long  int	 lcl_argIdx;

static            char	*lcl_fmtCmd = "format";

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Parse out the arguments.
    */

   if (argc == 1)
      {
      lcl_status = TCL_ERROR;
      Tcl_SetResult    (interp, "Syntax error (<format> missing)\n", TCL_VOLATILE);
      Tcl_AppendResult (interp, shTclErrStackPushUse, (char *)(NULL));
      }
   else
     {
       /*
	* Invoke the routine.
	*
	*   o   Count up the overall length of the Tcl command to format the string.
	*
	*   o   Build up the                       Tcl command to format the string.
	*
	*          o   argv strings are quoted with braces.
	*
	* NOTE:  strcpy () is used throughout for efficiency (rather than strcat),
	*        since the string lengths are known.
	*/

       for (lcl_fmtSize = ((argc - 1) * 3), lcl_argIdx = 1; lcl_argIdx < argc; lcl_argIdx++)
	 {
	   lcl_fmtSize += strlen (argv [lcl_argIdx]);
	 }
       lcl_fmtLen   = strlen (lcl_fmtCmd);
       lcl_fmtSize += lcl_fmtLen + 1;		/* Space for null terminaltor */

       if ((lcl_fmt = (char *)alloca ((unsigned int )lcl_fmtSize)) == (char *)(NULL))
	 {
	   Tcl_SetResult (interp, "Insufficient memory (alloca failure)", TCL_VOLATILE);
	   lcl_status = TCL_ERROR;
	 }
       else
	 {
	   
	   strcpy (lcl_fmt, lcl_fmtCmd);
	   for (                          lcl_argIdx = 1; lcl_argIdx < argc; lcl_argIdx++)
	     {
	       lcl_fmt [lcl_fmtLen++] = ' ';
	       lcl_fmt [lcl_fmtLen++] = '{';
	       strcpy (&(lcl_fmt [lcl_fmtLen]),         argv [lcl_argIdx]);
	       lcl_fmtLen += strlen (argv [lcl_argIdx]);
	       lcl_fmt [lcl_fmtLen++] = '}';
	     }
	   lcl_fmt [lcl_fmtLen]   = (char)(0);	/* Null terminate     */

	   if ((lcl_status = Tcl_Eval (interp, lcl_fmt)) == TCL_OK)
	     {
	       shErrStackPush ("%s", interp->result);
	       Tcl_ResetResult (interp);				/* No result string   */
	     }
	 };
     };
   /*
    * All done.
    */

   return (lcl_status);

   }	/* shTclErrStackPush */

/******************************************************************************/

static int	 shTclErrStackGetNext
   (
   ClientData	 clientData,		/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	*interp,		/* INOUT: Tcl interpreter structure   */
   int		 argc,			/* IN:    Tcl argument count          */
   char		*argv []		/* IN:    Tcl arguments               */
   )

/*++
 * ROUTINE:	 shTclErrStackGetNext
 *
 * DESCRIPTION:
 *
 *	Tcl command routine jacketing shErrStackGetNext ().
 *
 * RETURN VALUES:
 *
 *   TCL_OK		Success.
 *			The interp result string contains the error string
 *			returned by shErrStackGetNext ().
 *
 *   TCL_ERROR		Failure.
 *			The interp result string contains an error message.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	The Tcl interpreter result string is assumed to be empty (null) when
 *	this routine is called.
 *--
 ******************************************************************************/

   {	/* shTclErrStackGetNext */

   /*
    * Declarations.
    */

                  int	 lcl_status = TCL_OK;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Parse out the arguments.
    */

   if (argc != 1)
      {
      lcl_status = TCL_ERROR;
      Tcl_SetResult    (interp, "Syntax error (too many arguments)\n", TCL_VOLATILE);
      Tcl_AppendResult (interp, shTclErrStackGetNextUse, (char *)(NULL));
      }
   else 
     {
       /*
	* Invoke the routine.
	*/

       Tcl_SetResult (interp, (char *)(shErrStackGetNext ()), TCL_VOLATILE);

       /*
	* All done.
	*/
     }

   return (lcl_status);

   }	/* shTclErrStackGetNext */

/******************************************************************************/

static int	 shTclErrStackGetEarliest
   (
   ClientData	 clientData,		/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	*interp,		/* INOUT: Tcl interpreter structure   */
   int		 argc,			/* IN:    Tcl argument count          */
   char		*argv []		/* IN:    Tcl arguments               */
   )

/*++
 * ROUTINE:	 shTclErrStackGetEarliest
 *
 * DESCRIPTION:
 *
 *	Tcl command routine jacketing shErrStackGetEarliest ().
 *
 * RETURN VALUES:
 *
 *   TCL_OK		Success.
 *			The interp result string contains the error string
 *			returned by shErrStackGetEarliest ().
 *
 *   TCL_ERROR		Failure.
 *			The interp result string contains an error message.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	The Tcl interpreter result string is assumed to be empty (null) when
 *	this routine is called.
 *--
 ******************************************************************************/

   {	/* shTclErrStackGetEarliest */

   /*
    * Declarations.
    */

                  int	 lcl_status = TCL_OK;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Parse out the arguments.
    */

   if (argc != 1)
      {
      lcl_status = TCL_ERROR;
      Tcl_SetResult    (interp, "Syntax error (too many arguments)\n", TCL_VOLATILE);
      Tcl_AppendResult (interp, shTclErrStackGetEarliestUse, (char *)(NULL));
      }
   else
     {
       /*
	* Invoke the routine.
	*/

       Tcl_SetResult (interp, (char *)(shErrStackGetEarliest ()), TCL_VOLATILE);
       
       /*
	* All done.
	*/
     };

   return (lcl_status);

   }	/* shTclErrStackGetEarliest */

/******************************************************************************/

static int	 shTclErrStackClear
   (
   ClientData	 clientData,		/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	*interp,		/* INOUT: Tcl interpreter structure   */
   int		 argc,			/* IN:    Tcl argument count          */
   char		*argv []		/* IN:    Tcl arguments               */
   )

/*++
 * ROUTINE:	 shTclErrStackClear
 *
 * DESCRIPTION:
 *
 *	Tcl command routine jacketing shErrStackClear ().
 *
 * RETURN VALUES:
 *
 *   TCL_OK		Success.
 *			The interp result string is empty.
 *
 *   TCL_ERROR		Failure.
 *			The interp result string contains an error message.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	The Tcl interpreter result string is assumed to be empty (null) when
 *	this routine is called.
 *--
 ******************************************************************************/

   {	/* shTclErrStackClear */

   /*
    * Declarations.
    */

                  int	 lcl_status = TCL_OK;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Parse out the arguments.
    */

   if (argc != 1)
      {
      lcl_status = TCL_ERROR;
      Tcl_SetResult    (interp, "Syntax error (too many arguments)\n", TCL_VOLATILE);
      Tcl_AppendResult (interp, shTclErrStackClearUse, (char *)(NULL));
      }
   else
     {
       /*
	* Invoke the routine.
	*/

       shErrStackClear ();

       /*
	* All done.
	*/
     };

   return (lcl_status);

   }	/* shTclErrStackClear */

/******************************************************************************/

static int	 shTclErrStackOverflow
   (
   ClientData	 clientData,		/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	*interp,		/* INOUT: Tcl interpreter structure   */
   int		 argc,			/* IN:    Tcl argument count          */
   char		*argv []		/* IN:    Tcl arguments               */
   )

/*++
 * ROUTINE:	 shTclErrStackOverflow
 *
 * DESCRIPTION:
 *
 *	Tcl command routine jacketing shErrStackOverflow ().
 *
 * RETURN VALUES:
 *
 *   TCL_OK		Success.
 *			The interp result string is empty.
 *
 *   TCL_ERROR		Failure.
 *			The interp result string contains an error message.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	The Tcl interpreter result string is assumed to be empty (null) when
 *	this routine is called.
 *--
 ******************************************************************************/

   {	/* shTclErrStackOverflow */

   /*
    * Declarations.
    */

                  int	 lcl_status = TCL_OK;
                  char	 lcl_ovfCnt [11 + 1];	/* strlen ("-2147483648")     */

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Parse out the arguments.
    */

   if (argc != 1)
      {
      lcl_status = TCL_ERROR;
      Tcl_SetResult    (interp, "Syntax error (too many arguments)\n", TCL_VOLATILE);
      Tcl_AppendResult (interp, shTclErrStackOverflowUse, (char *)(NULL));
      }
   else
     {
       /*
	* Invoke the routine.
	*/

       sprintf (lcl_ovfCnt, "%d", shErrStackOverflow ());
       Tcl_SetResult (interp, lcl_ovfCnt, TCL_VOLATILE);
       
       /*
	* All done.
	*/
     };

   return (lcl_status);

   }	/* shTclErrStackOverflow */


void shErrStackTclTrace
   (
   ClientData	 clientData,
   Tcl_Interp	*interp,
   int		 level,
   char		*command,
   Tcl_CmdProc	*cmdProc,
   ClientData	 cmdClientData,
   int		 argc,
   char		*argv []
   )

{	/* shErrStackTclTrace */

   /*
    * Reset the flag to have the error stack cleared upon the next push.
    *
    * NOTE:  This routine is used as a callout from Tcl_Eval (), just prior to
    *        the execution of a command by Tcl_Eval ().  The call to Tcl_Create
    *        Trace () should specify the Tcl interpreter levels at which this
    *        routine is called.  This is the "proper" means of controlling auto-
    *        matic error stack clearing. The most sensible Tcl interpreter level
    *        is 1, the top level.  This flag should not be set at lower levels
    *        unless shErrStackPush () is modified to NOT clear the error stack
    *        as it is bouncing back up the Tcl interpreter levels.
    */

   errStack.flags |= shErrStack_ClearOnPush;

}	/* shErrStackTclTrace */



/*============================================================================ 
**============================================================================
**
** ROUTINE: shTclInterpAppendWithErrStack
**
** DESCRIPTION:
**	It gets message strings off the error stack and sets the TCL
**	result buffer. Messages display is managed by TCL.
**
** RETURN VALUES:
**	Not applicable
**
** CALLS TO:
**	shErrStackGetEarliest
**	shErrStackGetNext
**	Tcl_AppendResult
**	Tcl_SetResult
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
void shTclInterpAppendWithErrStack
   (
   Tcl_Interp *a_interp
   )   
{
   const char *msg;
/*
**	loop over all messages on the stack
*/
   for (msg = shErrStackGetEarliest(); msg != NULL; msg = shErrStackGetNext()){
      Tcl_AppendResult(a_interp, msg, (char *)NULL);
   }
/*
**	Clear the error stack
*/
      shErrStackClear();
}

/******************************************************************************/

void		 shTclErrStackDeclare
   (
   Tcl_Interp	*interp
   )

/*++
 * ROUTINE:	 shTclErrStackDeclare
 *
 * DESCRIPTION:		Declare Tcl commands for this module.
 *
 * RETURN VALUES:	None
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	This routine should only be called once per task.
 *--
 ******************************************************************************/

   {	/* shTclErrStackDeclare */

   /*
    * Declarations.
    */

#define	shTclErrStackCLI(TclCmd, rtn, TclHelp, TclUse)\
   shTclDeclare (interp, TclCmd, (Tcl_CmdProc *)(rtn), (ClientData)(0), (Tcl_CmdDeleteProc *)(NULL), "shErrStack", TclHelp, TclUse)

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   shTclErrStackCLI ("errStackPush",        shTclErrStackPush,        shTclErrStackPushHelp,        shTclErrStackPushUse);
   shTclErrStackCLI ("errStackGetNext",     shTclErrStackGetNext,     shTclErrStackGetNextHelp,     shTclErrStackGetNextUse);
   shTclErrStackCLI ("errStackGetEarliest", shTclErrStackGetEarliest, shTclErrStackGetEarliestHelp, shTclErrStackGetEarliestUse);
   shTclErrStackCLI ("errStackClear",       shTclErrStackClear,       shTclErrStackClearHelp,       shTclErrStackClearUse);
   shTclErrStackCLI ("errStackOverflow",    shTclErrStackOverflow,    shTclErrStackOverflowHelp,    shTclErrStackOverflowUse);

   }	/* shTclErrStackDeclare */

/******************************************************************************/
