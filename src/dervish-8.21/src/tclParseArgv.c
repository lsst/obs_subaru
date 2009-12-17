/****************************************************************************
*****************************************************************************
**<AUTO>
** FILE:
**	tclParseArgv.c
**
** ABSTRACT:
**	This file contains software that parses Tcl verbs and formats error
**      or help statments in a standard way.  It uses the ftcl package
**      ftcl_ParseArgv and associated routines.
**</AUTO>
**
** ENTRY POINT		SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
** shTclGetArgInfo      Public  Return the requested verbs' help string
** shTclGetUsage        Public  Return the requested verbs' usage string
** shTclGetHelp         Public  Return the requested verbs' help & usage string
** shTclParseArgv	Public  Parse the Tcl command line
**
**
** ENVIRONMENT:
**	ANSI C.
**
** REQUIRED PRODUCTS:
**	FTCL	 	TCL + Fermi extension
**
**
** AUTHORS:     Creation date: Jan. 30, 1995
**              Eileen Berman
**
*****************************************************************************
*****************************************************************************
*/

#include "shTclParseArgv.h"
#include "ftclParseArgv.h"
#include "string.h"
#include "stdio.h"

/*============================================================================
**============================================================================
**
** LOCAL MACROS, DEFINITIONS, ETC.
**
**============================================================================
*/

/*----------------------------------------------------------------------------
**
** LOCAL DEFINITIONS
*/
int g_argInfoIndent = 0;            /* # of spaces to indent help message */
char *g_argInfoIndAsc = "0";        /* # of spaces to indent help message */
int g_argUsageIndent = 0;           /* # of spaces to indent usage message */
char *g_argUsageIndAsc = "0";       /* # of spaces to indent usage message */
char g_usagePrefix[] = "USAGE: ";   /* Prefix to usage string */
char g_usageSuffix[] = "";          /* Suffix to usage string */
char g_helpSuffix[] = "\n";         /* Suffix to help string */

/******************************************************************************
******************************************************************************/

/*============================================================================
**============================================================================
**<AUTO EXTRACT>
** ROUTINE: shTclGetArgInfo
**
** DESCRIPTION:
**<HTML>
**	Output help for a Tcl verb constructed from the arg table.
**</HTML>
**
** RETURN VALUES:
**	HELP STRING  - success
**	NULL STRING  -   Failed miserably. Error stack is set.
**	
**</AUTO>
** CALLS TO:
**	
**============================================================================
*/
char *shTclGetArgInfo
   (
   Tcl_Interp   *a_interp,         /* IN: Tcl Interpreter */
   ftclArgvInfo *a_argTable,       /* IN: The arg table */
   int          a_flags,           /* IN: Flags as for ftcl_ArgInfo */
   char         *a_cmdName         /* IN: Name of command */
   )
{
/* Get the properly formatted help string from the arg table */
return(ftcl_GetArgInfo(a_interp, a_argTable, a_flags, a_cmdName,
			g_argInfoIndent));
}

/*============================================================================
**============================================================================
**<AUTO EXTRACT>
** ROUTINE: shTclGetUsage
**
** DESCRIPTION:
**<HTML>
**	Output the usage string for a Tcl verb constructed from the arg 
**      table.
**</HTML>
**
** RETURN VALUES:
**	USAGE STRING   -   Successful completion.
**	NULL  STRING   -   Failed miserably. Error stack is set.
**</AUTO>	
**
** CALLS TO:
**	
**============================================================================
*/
char *shTclGetUsage
   (
   Tcl_Interp   *a_interp,         /* IN: Tcl Interpreter */
   ftclArgvInfo *a_argTable,       /* IN: The arg table */
   int          a_flags,           /* IN: Flags as for ftcl_GetUsage */
   char         *a_cmdName         /* IN: Name of command */
   )
{
/* Get the properly formatted help string from the arg table */
return (ftcl_GetUsage(a_interp, a_argTable, a_flags, a_cmdName,
		      g_argUsageIndent, g_usagePrefix, g_usageSuffix));
}

/*============================================================================
**============================================================================
**<AUTO EXTRACT>
** ROUTINE: shTclGetHelp
**
** DESCRIPTION:
**<HTML>
**	Ouput help and usage information for a Tcl verb that is constructed
**      from the arg table.
**</HTML>
**
** RETURN VALUES:
**	HELP/USAGE STRING - success
**	NULL STRING       -   Failed miserably. Error stack is set.
**</AUTO>	
**
** CALLS TO:
**	
**============================================================================
*/
char *shTclGetHelp
   (
   Tcl_Interp   *a_interp,         /* IN: Tcl Interpreter */
   ftclArgvInfo *a_argTable,       /* IN: The arg table */
   int          a_flags,           /* IN: Flags to ftcl_GetHelp */
   char         *a_cmdName         /* IN: Name of command */
   )
{
/* Get the properly formatted help and usage strings from the arg table */
return (ftcl_GetHelp(a_interp, a_argTable, a_flags, a_cmdName,
		     g_argUsageIndent, g_usagePrefix, g_usageSuffix,
		     g_helpSuffix));
}

/*============================================================================
**============================================================================
**<AUTO EXTRACT>
** ROUTINE: shTclParseArgv
**
** DESCRIPTION:
**<HTML>
**	Call the dervish parser to parse the command line and load the interp
**      result string on error or on a help request.
**</HTML>
**
** RETURN VALUES:
**	FTCL_ARGV_SUCCESS  -   Successful completion.
**      TCL_OK             -   Help requested and loaded into error stack
**      TCL_ERROR          -   Failed miserably.  Interp->result contains the
**                             error string and the usage string.
**</AUTO>	
**
** CALLS TO:
**	
**============================================================================
*/
int shTclParseArgv
   (
   Tcl_Interp   *a_interp,         /* IN: Tcl Interpreter */
   int          *a_argc,           /* IN: Number of command line arguments */
   char         *a_argv[],         /* IN: Command line argugment array */
   ftclArgvInfo *a_argTable,       /* IN: The arg table */
   int          a_flags,           /* IN: Flags to ftcl_ParseArgv */
   char         *a_cmdName         /* IN: Name of command */
   )
{
int rstatus;

/* Parse the command line */
if ((rstatus = ftcl_ParseArgv(a_interp, a_argc, a_argv, a_argTable, a_flags))
     != FTCL_ARGV_SUCCESS)
   {
   /* Check to see if we got a request for help */
   if (rstatus == FTCL_ARGV_GIVEHELP)
      {
      /* Load the help string into the interp result */
      Tcl_AppendResult(a_interp,
		       shTclGetArgInfo(a_interp, a_argTable, a_flags,
				       a_cmdName),
		       (char *)NULL);
      rstatus = TCL_OK;
      }
   else 
      {
      /* ERROR: Load the usage string into the interp result */
      Tcl_AppendResult(a_interp, "\n",
		       shTclGetUsage(a_interp, a_argTable, a_flags, a_cmdName),
		       (char *)NULL);
      rstatus = TCL_ERROR;
      }
   }

return(rstatus);
}


/*============================================================================
**============================================================================
**<AUTO EXTRACT>
** ROUTINE: shTclParseArg
**
**<HTML>
**      Call the ftcl parser, ftclParseArg,  to parse a TCL command line 
**      and load the interp result string on error or on a help request.
** TCL params are:
**      $args              - The arguements to the TCL procedure
**      formal_args        - The formal argument definitions 
**                           see man ftclParseArg
**                           See also $DERVISH_DIR/etc/shplot.tcl for an example
**      command_name       - The name of the command being parsed
**
** RETURN VALUES:
**      TCL_OK             -   Parse completed normally
**      TCL_RETURN         -   Help requested and loaded into the result
**      TCL_ERROR          -   Failed miserably.  Interp->result contains the
**                             error string and the usage string.
**</HTML>
**</AUTO>
**
**============================================================================
*/
int shTclParseArg
   (
   ClientData clientData,
   Tcl_Interp   *a_interp,         /* IN: Tcl Interpreter */
   int          a_argc,            /* IN: Number of command line arguments */
   char         *a_argv[]          /* IN: Command line argugment array */
   )
{
int rstatus;
char *argv[10];
int argc = 3;
argv[0] = a_argv[0];
argv[1] = a_argv[1];
argv[2] = a_argv[2];
argv[3] = 0;

/* Parse the command line */
rstatus = ftclParseArg(clientData, a_interp, argc, argv);
if (rstatus == TCL_OK)
   {
   /* Check to see if we got a request for help */
   if (strcmp(a_interp->result,"help") == 0)
      {
      /* Load the help string into the interp result */
      Tcl_ResetResult(a_interp);		/* remove "help" */
      argv[0] = a_argv[0];
      argv[1] = a_argv[3];
      argv[2] = a_argv[2];
      argv[3] = g_argInfoIndAsc;
      argv[4] = 0;
      argc = 4;
      ftclGetArgInfo(clientData, a_interp, argc, argv );
      return(TCL_RETURN);
      }
   }
else
   {
      /* ERROR: Load the usage string into the interp result */
      argv[0] = a_argv[0];
      argv[1] = a_argv[3];
      argv[2] = a_argv[2];
      argv[3] = g_argUsageIndAsc;
      argv[4] = g_usagePrefix;
      argv[5] = g_helpSuffix;
      argv[6] = 0;
      argc = 6;
      ftclGetUsage(clientData, a_interp, argc, argv );
   }
return(rstatus);
}
