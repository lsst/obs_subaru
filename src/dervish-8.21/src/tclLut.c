/*****************************************************************************
******************************************************************************
**
** FILE:
**	tclLut.c
**
** ABSTRACT:
**	This file contains routines that support the definition of TCL verbs
**	that manipulate lookup tables.
**
** ENTRY POINT		SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
** shTclLutNew		local	Create a new 8 bit lookup table
** shTclLutDel		local	Delete an 8 bit lookup table
** shTclLutSet  	local	Set the values in the lookup table
** shTclLutGet  	local	Get the values in the lookup table
** shTclLutDeclare	public	Declare TCL verbs for this module
**
** ENVIRONMENT:
**	ANSI C.
**
** REQUIRED PRODUCTS:
**	FTCL
**
** AUTHORS:
**	Eileen Berman
**	Creation date:  Dec 14, 1993
**
******************************************************************************
******************************************************************************
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include "ftcl.h"
#include "shCLut.h"
#include "shTclHandle.h"
#include "shTclUtils.h"
#include "shCErrStack.h"
#include "shTclErrStack.h"

/*============================================================================
**============================================================================
**
** LOCAL DEFINITIONS
**
**============================================================================
*/

/*
** TCL COMMAND SYNTAX
*/
static char *TCLUSAGE_LUTNEW = "USAGE: lutNew [-name lookupTableName]\n";
static char *TCLHELP_LUTNEW  = "Create a new lookup table.\n";
static char *TCLUSAGE_LUTDEL = "USAGE: lutDel <lookupTableHandle>\n";
static char *TCLHELP_LUTDEL  = "Delete a lookup table.\n";
static char *TCLUSAGE_LUTSET = 
   "USAGE: lutSet <lookupTableHandle> [tableEntries] <-t tclScript | -p tclProcedure | -l tclList>\n";
static char *TCLHELP_LUTSET  =
   "Set some or all of the entries of a lookup table.\n";
static char *TCLUSAGE_LUTGET = 
   "USAGE: lutGet <lookupTableHandle> [tableEntries]\n";
static char *TCLHELP_LUTGET  =
   "Return, as a Tcl list, some or all of the values of a lookup table.\n";

/*
** MAXIMUM ASCII NAME FOR REGION (DON'T CONFUSE WITH MAXTCLREGNAMELEN)
*/
#define L_MAXLUTNAMELEN	70

/*============================================================================
**============================================================================
**
** ROUTINE: getEntryVal
**
** DESCRIPTION:
**	Get an element from the 'entry' list.  If no list was passed to
**      us, return the value of the counter.
**
** RETURN VALUES:
**	TCL_OK	    Successful completion.
**	TCL_ERROR   Error occurred.  The Interp result string will contain
**		    an error string.
**
**============================================================================
*/
int getEntryVal
   (
   Tcl_Interp *a_interp,
   int        a_entryArgc,
   char       **a_entryArgv,
   int        a_i,
   int        *a_entryVal
   )   
{
int  rstatus = TCL_OK;
char ebuf[100];

/* If we are to set the whole lookup table, just use the value of i as
   the entry. */
if (a_entryArgc == LUT8TOP)
   {*a_entryVal = a_i;}

/* Transform entry list element into an integer */
else if ((rstatus = Tcl_GetInt(a_interp, a_entryArgv[a_i], a_entryVal))
	 == TCL_OK)
   {
   /* Make sure the value is within the limits */
   if ((*a_entryVal < LUT8MIN) || (*a_entryVal > LUT8MAX))
      {
      rstatus = TCL_ERROR;
      sprintf(ebuf,
	      "Entry list element %d is outside of limits (< %d or > %d).\n",
	      a_i, LUT8MIN, LUT8MAX);
      Tcl_AppendResult(a_interp, ebuf, (char *)NULL);
      }
   }

return(rstatus); 
}

/*============================================================================
**============================================================================
**
** ROUTINE: getLUT
**
** DESCRIPTION:
**	Get the requested lookup table entry values.
**
** RETURN VALUES:
**	TCL_OK	    Successful completion. The interp string will contain
**                  a list of the entry values.
**	TCL_ERROR   Error occurred.  The Interp result string will contain
**		    an error string.
**
**============================================================================
*/
int getLUT
   (
   Tcl_Interp *a_interp,
   int        a_entryArgc,
   char       **a_entryArgv,
   LUT        *a_lutPtr
   )   
{
int   rstatus = TCL_OK;
int   i, entryVal;
char  anum[20];

/* Get the requested list of values */
for (i = 0 ; i < a_entryArgc ; ++ i)
   {
   /* Get the number of the lookup table entry to get the value of */
   if ((rstatus = getEntryVal(a_interp, a_entryArgc, a_entryArgv, i,
			      &entryVal)) == TCL_OK)
      {
      /* Get the value of the corresponding lookup table entry and convert it
	 to a string. */
      sprintf(anum, "%d", a_lutPtr->lutArray[entryVal]);

      /* Pass it to the interpreter to build into a tcl list */
      Tcl_AppendElement(a_interp, anum);
      }
   else
      {break;}   /* Error while getting the value - get out of the for loop. */
   }

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: setLUTWProc
**
** DESCRIPTION:
**	Set the lookup table using the passed tcl procedure.
**
** RETURN VALUES:
**	TCL_OK	    Successful completion.
**	TCL_ERROR   Error occurred.  The Interp result string will contain
**		    an error string.
**
**============================================================================
*/
int setLUTWProc
   (
   Tcl_Interp *a_interp,
   int        a_entryArgc,
   char       **a_entryArgv,
   char       **a_tclListArgv,
   char       *a_tclProc,
   LUT        *a_lutPtr
   )   
{
int   rstatus = TCL_OK;
int   i, entryVal;
char  anum[100];
char  errmsg[80];
double result;

/* Use the supplied list of values */
for (i = 0 ; i < a_entryArgc ; ++ i)
   {
   /* Get the number  of the lookup table entry to set */
   if ((rstatus = getEntryVal(a_interp, a_entryArgc, a_entryArgv, i,
			      &entryVal)) != TCL_OK)
      {break;}           /* error while getting entryVal from passed list */

   sprintf(anum, "%s %d", a_tclProc, entryVal);
   if ((rstatus = Tcl_Eval(a_interp, anum)) == TCL_OK)
      {
      if ((rstatus = Tcl_GetDouble(a_interp, a_interp->result, &result))
	  == TCL_OK)
         {
         /* Allow the result to be = 0, which always means transparent */
         if (((int )result < LUT8MIN) || ((int )result > LUT8MAX))
            {
            /* Result is out of range */
            rstatus = TCL_ERROR;
            sprintf(errmsg, "%d to %d or 0.", LUT8MIN, LUT8MAX);
            Tcl_AppendResult(a_interp, "\nResults from '", a_tclProc, " ",
			     anum, "' must be in the range of ",
			     errmsg, "\n", (char *)NULL);
            break;
            }
         a_lutPtr->lutArray[entryVal] = (short int )result;/* put in lookup table */
         }
      else
         {break;}                 /* Error in GetDouble */
      }
   else
      {break;}                  /* Error in Tcl script */
   }

/* The interp->result string is set by Tcl_Eval.  If we got here with no
   errors, reset this so nothing is output to the user. */
if (rstatus == TCL_OK)
   {Tcl_ResetResult(a_interp);}

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: setLUTWScript
**
** DESCRIPTION:
**	Set the lookup table using the passed tcl script.
**
** RETURN VALUES:
**	TCL_OK	    Successful completion.
**	TCL_ERROR   Error occurred.  The Interp result string will contain
**		    an error string.
**
**============================================================================
*/
int setLUTWScript
   (
   Tcl_Interp *a_interp,
   int        a_entryArgc,
   char       **a_entryArgv,
   char       **a_tclListArgv,
   char       *a_tclScript,
   LUT        *a_lutPtr
   )   
{
int rstatus = TCL_OK, tstatus;
int i, entryVal;
char   anum[10];
char   errmsg[80];
char   *savedResult;
char   *savedArray = NULL;
int    tclFlags = 0;
double result;

/* Since we are going to be resetting the variable lutVal, save the current
   value and restore it on exit. */
savedResult = Tcl_GetVar(a_interp, "lutVal", tclFlags);
if (savedResult != (char *)NULL)
   {
   /* Only get the value if the variable exists. Malloc room for the saved
      value. */
   savedArray = (char *)alloca(strlen(savedResult)+1);
   if (savedArray == (char *)NULL)
      {
      /* Error in malloc */
      rstatus = TCL_ERROR;
      Tcl_AppendResult(a_interp,
                    "\nError getting space for saved value of maskVal.\n",
                       (char *)NULL);
      }
   else
      {
      /* save a copy of the value, as this value is overwritten the next time
         the variable is set (later on in this routine). */
      strcpy(savedArray, savedResult);
      }
   }

if (rstatus == TCL_OK)			/* No errors yet */
   {
   /* Use the supplied list of values */
   for (i = 0 ; i < a_entryArgc ; ++ i)
      {
      /* Get the number  of the lookup table entry to set */
      if ((rstatus = getEntryVal(a_interp, a_entryArgc, a_entryArgv, i,
				 &entryVal)) != TCL_OK)
	 {break;}           /* error while getting entryVal from passed list */
   
      /* First set the TCL variable 'lutVal' */
      sprintf(anum, "%d", entryVal);
      if (Tcl_SetVar(a_interp, "lutVal", anum, tclFlags) != (char *)NULL)
         {
         if ((rstatus = Tcl_Eval(a_interp, a_tclScript))
	     == TCL_OK)
            {
            if ((rstatus = Tcl_GetDouble(a_interp, a_interp->result, &result))
		== TCL_OK)
               {
	       /* Make sure the result is within LUT8MIN and LUT8MAX */
               if (((int )result < LUT8MIN) || ((int )result > LUT8MAX))
                  {
                  /* Result is out of range */
                  rstatus = TCL_ERROR;
                  sprintf(errmsg, "%d to %d or 0.", LUT8MIN, LUT8MAX);
                  Tcl_AppendResult(a_interp, "\nResults from '",
                                   a_tclScript, "' with lutVal = ", anum,
                                   ", must be in the range of ",
                                   errmsg, "\n", (char *)NULL);
                  break;
                  }
	       /* Put in the lookup table */
               a_lutPtr->lutArray[entryVal] = (short int )result;
               }
            else
               {break;}               /* Error in GetDouble */
            }
         else
            {break;}                   /* Error in Tcl script */
         }
      else
         {
	 rstatus = TCL_ERROR;
	 break;                      /* Error in setting variable value */
	 }
      }

   /* Restore the original value of lutVal only if it existed prior to
      calling this routine */
   if (savedResult != (char *)NULL)
      {
      /* The variable existed before this routine was called */
      if (Tcl_SetVar(a_interp, "lutVal", savedArray, tclFlags) == (char *)NULL)
         {rstatus = TCL_ERROR;}		/* Error in Tcl script */
      }
   else
      {
      /* The variable did not exist, so get rid of it - use tstatus to not
         write over rstatus if it is TCL_ERROR */
      tstatus = Tcl_UnsetVar(a_interp, "lutVal", tclFlags);
      }
   }

/* The interp->result string is set by Tcl_Eval.  If we got here with no
   errors, reset this so nothing is output to the user. */
if (rstatus == TCL_OK)
   {Tcl_ResetResult(a_interp);}

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: setLUTWList
**
** DESCRIPTION:
**	Set the lookup table using the passed tcl list.
**
** RETURN VALUES:
**	TCL_OK	    Successful completion.
**	TCL_ERROR   Error occurred.  The Interp result string will contain
**		    an error string.
**
**============================================================================
*/
int setLUTWList
   (
   Tcl_Interp *a_interp,
   int        a_entryArgc,
   char       **a_entryArgv,
   char       **a_tclListArgv,
   LUT        *a_lutPtr
   )   
{
int  rstatus = TCL_OK;
int  i, entryVal, tclListVal;
char ebuf[100];

/* Use the supplied list of values */
for (i = 0 ; i < a_entryArgc ; ++ i)
   {
   if ((rstatus = getEntryVal(a_interp, a_entryArgc, a_entryArgv, i,
			      &entryVal)) != TCL_OK)
      {break;}              /* error while getting entryVal from passed list */
   
   /* Transform tcl list element into an integer */
   if ((rstatus = Tcl_GetInt(a_interp, a_tclListArgv[i], &tclListVal))
       != TCL_OK)
      {break;}    /* Error while doing integer transformation */
   else
      {
      /* Make sure the value is within the limits */
      if ((tclListVal < 0) || (tclListVal > LUT8TOP))
	 {
	 rstatus = TCL_ERROR;
	 sprintf(ebuf,
	    "Tcl list element %d is outside of limits (it is < %d or > %d).\n",
		 entryVal, 0, LUT8TOP);
	 Tcl_AppendResult(a_interp, ebuf, (char *)NULL);
	 break;
	 }
      }
   
   /* Set the lookup table entry with the entered value */
   a_lutPtr->lutArray[entryVal] = tclListVal;
   }

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: shTclLutAddrGetFromName
**
** DESCRIPTION:
**	Get the address of a 8 bit lookup table from the handlename.
**
** RETURN VALUES:
**	TCL_OK	    Successful completion.
**	TCL_ERROR   Error occurred.  The Interp result string will contain
**		    an error string.
**
**============================================================================
*/
int shTclLutAddrGetFromName
   (
   Tcl_Interp *a_interp,
   char const *a_lutName,
   LUT        **a_lut
   )   
{
int rstatus;
HANDLE realHandle;
HANDLE *hand = &realHandle;
void   *userPtr;

/* Find lut address */
if ((rstatus = shTclHandleExprEval(a_interp,(char *)a_lutName,hand,&userPtr))
    != TCL_OK)
   {
   Tcl_AppendResult(a_interp, "\n-----> Unknown lookup table: ", 
                                  a_lutName, "\n", (char *) NULL);
   }
else
   {
   if (hand->type != shTypeGetFromName("LUT"))
      {
      Tcl_AppendResult(a_interp,a_lutName," is a ",
		       shNameGetFromType(hand->type),
		       " not a LUT\n",(char *)NULL);
      rstatus = TCL_ERROR;
      }
   else
      {
      /* Return lookup table address */
      *a_lut = (LUT *)hand->ptr;
      }
   }

return(rstatus);

}

/*============================================================================
**============================================================================
**
** ROUTINE: shTclLutNew
**
** DESCRIPTION:
**	Create a new 8 bit lookup table.
**
** RETURN VALUES:
**	TCL_OK	    Successful completion.  The Interp result string will
**		    contain the new lookup table name.
**	TCL_ERROR   Error occurred.  The Interp result string will contain
**		    an error string.
**
**============================================================================
*/
static int shTclLutNew
   (
   ClientData	a_clientdata,	/* IN : TCL parameter - not used */
   Tcl_Interp	*a_interp,	/* OUT: TCL Interpreter structure */
   int		argc,		/* IN : TCL argument count */
   char		**argv		/* IN : TCL argument */
   )   
{
int      rstatus = TCL_OK;
int	 i;
HANDLE   hand;
LUT	 *lutPtr;
LUTTYPE  type = LUT8;
char	 *nameASCII;
char     ebuf[100];
char	 tclLutName[MAXTCLLUTNAMELEN];
char	 passedName[L_MAXLUTNAMELEN+1];

nameASCII = tclLutName;           /* ASCII name for lookup table */

/*
** PARSE ARGUMENTS
*/
for (i = 1 ; i < argc ; ++i)
   {
   /* Handle switches */
   if (argv[i][0] == '-')
      {
      /* ASCII name for lookup table */
      if (!strcmp("-name", argv[i]))
         {
         strncpy(passedName, argv[++i], L_MAXLUTNAMELEN);
         passedName[L_MAXLUTNAMELEN] = (char)0;
         nameASCII = passedName;
         }
      /* Invalid switch */
      else
         {
	 sprintf(ebuf, "Unknown switch '%s'\n", argv[i]);
	 Tcl_SetResult(a_interp, ebuf, TCL_VOLATILE);
	 Tcl_AppendResult(a_interp, TCLUSAGE_LUTNEW, (char *)NULL);
	 rstatus = TCL_ERROR;
         }
      }
   else
      {
      /* Invalid.  No parameters are accepted by this command. */
      sprintf(ebuf, "Unknown parameter '%s'\n", argv[i]);
      Tcl_SetResult(a_interp, ebuf, TCL_VOLATILE);
      Tcl_AppendResult(a_interp, TCLUSAGE_LUTNEW, (char *)NULL);
      rstatus = TCL_ERROR;
      }
   }

/* Done with parsing - Only continue if there were no errors */
if (rstatus == TCL_OK)
   {
   /* Get new TCL lookup table name */
   if ((rstatus = p_shTclHandleNew(a_interp, tclLutName)) == TCL_OK)
      {
      /* Create new lookup table (using TCL lookup table name if no name
	 was specified) */
      if ((lutPtr = shLutNew(nameASCII, type)) != NULL)
	 {
	 /* Associate lookup table with TCL lookup table name */
	 hand.type = shTypeGetFromName("LUT");   /* Fill in handle structure */
	 hand.ptr = lutPtr;
	 if ((rstatus = p_shTclHandleAddrBind(a_interp, hand, tclLutName))
	     == TCL_OK)
	    {
	    /* Return the lookup table name to the user */
	    Tcl_SetResult(a_interp, tclLutName, TCL_VOLATILE);
	    }
	 else
	    {
	    /* Could not associate name with lookup table */
	    shLutDel(lutPtr);             /* Delete the lookup table */
	    p_shTclHandleDel(a_interp, tclLutName);  /* delete the handle */
	    }
	 }
      else
	 {
	 /* Error getting new lookup table */
	 p_shTclHandleDel(a_interp, tclLutName);  /* delete the handle */
	 shTclInterpAppendWithErrStack(a_interp);   /* Get errors from stack */
	 rstatus = TCL_ERROR;
	 }
      }
   }

return(rstatus);			/* We made it ! */
}

/*============================================================================
**============================================================================
**
** ROUTINE: shTclLutDel
**
** DESCRIPTION:
**	Delete an 8 bit lookup table.
**
** RETURN VALUES:
**	TCL_OK	    Successful completion.
**	TCL_ERROR   Error occurred.  The Interp result string will contain
**		    an error string.
**
**============================================================================
*/
static int shTclLutDel
   (
   ClientData	a_clientdata,	/* IN : TCL parameter - not used */
   Tcl_Interp	*a_interp,	/* OUT: TCL Interpreter structure */
   int		argc,		/* IN : TCL argument count */
   char		**argv		/* IN : TCL argument */
   )   
{
int      rstatus = TCL_OK;
int      numArgs = 2;
char     *lutName;
LUT      *lutPtr;

/* Parse the arguments */
if (argc != numArgs)
   {
   /* The number of args is 2, and 2 shall be the number of args. */
   Tcl_AppendResult(a_interp, TCLUSAGE_LUTDEL, (char *)NULL);
   rstatus = TCL_ERROR;
   }
else
   {
   /* The only passed parameter had better be the lut name */
   lutName = argv[1];

   /* Get the address to the handle from the name */
   if ((rstatus = shTclLutAddrGetFromName(a_interp, lutName, &lutPtr)) ==
       TCL_OK)
      {
      /* Got handle address  - delete the lookup table */
      shLutDel(lutPtr);

      /* Disassociate the tcl name from the lookup table */
      p_shTclHandleDel(a_interp, lutName);
      }
   }

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: shTclLutSet
**
** DESCRIPTION:
**	Set all or part of an 8 bit lookup table.
**
** RETURN VALUES:
**	TCL_OK	    Successful completion.
**	TCL_ERROR   Error occurred.  The Interp result string will contain
**		    an error string.
**
**============================================================================
*/
static int shTclLutSet
   (
   ClientData	a_clientdata,	/* IN : TCL parameter - not used */
   Tcl_Interp	*a_interp,	/* OUT: TCL Interpreter structure */
   int		argc,		/* IN : TCL argument count */
   char		**argv		/* IN : TCL argument */
   )   
{
int      rstatus = TCL_OK;
int	 i;
int      maxArgs = 5;
int      gotList = 0, gotProc = 0, gotScript = 0;
int      entryArgc = 0, tclListArgc;
char     **entryArgv = NULL, **tclListArgv = NULL;
char     *tclList = NULL;
char     *tclProc = NULL;
char     *tclScript = NULL;
char     *entryPtr = NULL;
char     *lutName = NULL;
LUT      *lutPtr = NULL;
char     ebuf[100];

/* Parse the arguments */
if (argc > maxArgs)
   {
   /* Too many arguments */
   Tcl_SetResult(a_interp, "Invalid command format - too many arguments.\n",
		 TCL_VOLATILE);
   Tcl_AppendResult(a_interp, TCLUSAGE_LUTSET, (char *)NULL);
   rstatus = TCL_ERROR;
   }
else
   {
   for (i = 1 ; i < argc ; ++i)
      {
      switch (argv[i][0])
         {
         case '-':
            switch (argv[i][1])
               {
               case 't':
                  /* This is the TCL script */
                  tclScript = argv[++i];	/* Point to the Tcl script */
		  ++gotScript;
                  break;
               case 'p':
                  /* This is the TCL procedure */
                  tclProc = argv[++i];		/* Point to the Tcl proc */
		  ++gotProc;
                  break;
               case 'l':
                  /* This is the Tcl list */
		  tclList = argv[++i];
		  ++gotList;
                  break;
               default:
                  /* This is an invalid parameter */
                  rstatus = TCL_ERROR;
                  Tcl_SetResult(a_interp, "Invalid option entered - ",
                                TCL_VOLATILE);
                  Tcl_AppendResult(a_interp, argv[i], "\n", (char *)NULL);
		  Tcl_AppendResult(a_interp, TCLUSAGE_LUTSET, (char *)NULL);
               }
            break;
         default:
            /* This is either the lookup table name or the list of entries in
	       the table that we are to set. The lookup table name must come
	       first. */
	    if (lutName == 0)
	       {
	       /* We have not gotten this yet. */
	       lutName = argv[i];
	       }
	    else
	       {
	       /* We got the lookup table, so the second parameter must be the
		  entry to fill in. */
	       entryPtr = argv[i];
	       }
         }
      }

   /* Check to make sure we got all the parameters we need */
   if (lutName == 0)
      {
      /* We must have a lookup table name */
      rstatus = TCL_ERROR;
      Tcl_AppendResult(a_interp,
		       "A lookup table name MUST be entered.\n", (char *)NULL);
      Tcl_AppendResult(a_interp, TCLUSAGE_LUTSET, (char *)NULL);
      }
   else if ((gotScript | gotProc | gotList) != 1)
      {
      /* One of these must be entered. And only 1. */
      rstatus = TCL_ERROR;
      Tcl_AppendResult(a_interp,
	"One Tcl script, one Tcl procedure or one Tcl list MUST be entered.\n",
		       (char *)NULL);
      Tcl_AppendResult(a_interp, TCLUSAGE_LUTSET, (char *)NULL);
      }
   else if (entryPtr == 0)
      {
      /* Did not get any entries, set the whole lookup table. */
      entryArgc = LUT8TOP;
      }

   /* Only continue on if we have not gotten an error so far. */
   if (rstatus == TCL_OK)
      {
      /* Get the address of the lookup table from the name */
      if ((rstatus = shTclLutAddrGetFromName(a_interp, lutName, &lutPtr))
	  == TCL_OK)
	 {
	 /* Break the entered entries (the parts of the lookup table to be
	    set up), they are now a tcl list. This is only true if we have
	    not set entryArgc to be the max number of elements in which case 
	    no entry parameter was specified and we must do the whol lookup
	    table. */
	 if (entryArgc == 0)
	    {
	    /* We were given entries,  this may be a list so try and split
	       them */
	    if ((rstatus = Tcl_SplitList(a_interp, entryPtr, &entryArgc,
					 &entryArgv)) != TCL_OK)
	       {
	       Tcl_AppendResult(a_interp,
				"Error from Tcl_SplitList when trying to split entry parameter.\n",
				(char *)NULL);
	       goto exit;
	       }

	    /* Make sure we were not given too many entries. */
	    if (entryArgc > LUT8TOP)
	       {
	       rstatus = TCL_ERROR;
	       sprintf(ebuf, "Too many entries, must be < or = %d\n", LUT8TOP);
	       Tcl_AppendResult(a_interp, ebuf, (char *)NULL);
	       goto exit;
	       }
	    }
	    
	 /* If we were given a tcl list of values to set the lookup table
	    entries to, try to split this list. There had better be the
	    same number of entries in both lists */
	 if (gotList != 0)
	    {
	    if ((rstatus = Tcl_SplitList(a_interp, tclList, &tclListArgc,
					 &tclListArgv)) != TCL_OK)
	       {
	       Tcl_AppendResult(a_interp,
				"Error from Tcl_SplitList when trying to split tclList parameter.\n",
				(char *)NULL);
	       goto exit;
	       }
	       
	    /* Check that the number of list elements is the same for both
	       lists. */
	    else if (tclListArgc != entryArgc)
	       {
	       rstatus = TCL_ERROR;
	       Tcl_SetResult(a_interp,
			     "The Tcl and Entry lists MUST have the same number of elements.\n",
			     TCL_VOLATILE);
	       goto exit;
	       }
	    }

	 /* Set the lookup table */
	 if (gotProc != 0)
	    {
	    /* Use a tcl procedure */
	    rstatus = setLUTWProc(a_interp, entryArgc, entryArgv,
				  tclListArgv, tclProc, lutPtr);
	    }
	 else if (gotScript != 0)
	    {
	    /* Use a tcl script */
	    rstatus = setLUTWScript(a_interp, entryArgc, entryArgv,
				    tclListArgv, tclScript, lutPtr);
	    }
	 else
	    {
	    rstatus = setLUTWList(a_interp, entryArgc, entryArgv,
				  tclListArgv, lutPtr);
	    }
	 
	 /* Free the arrays allocated by Tcl_SplitList */
	 if (entryArgv != NULL) 
	    free((char *)entryArgv);
	 if (tclListArgv != NULL)
	    free((char *)tclListArgv);
	 }
      }
   }

exit:
return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: shTclLutGet
**
** DESCRIPTION:
**	Get all or part of an 8 bit lookup table.
**
** RETURN VALUES:
**	TCL_OK	    Successful completion. The Interp result string will
**                  contain a list of all the requested entries.
**	TCL_ERROR   Error occurred.  The Interp result string will contain
**		    an error string.
**
**============================================================================
*/
static int shTclLutGet
   (
   ClientData	a_clientdata,	/* IN : TCL parameter - not used */
   Tcl_Interp	*a_interp,	/* OUT: TCL Interpreter structure */
   int		argc,		/* IN : TCL argument count */
   char		**argv		/* IN : TCL argument */
   )   
{
int      rstatus = TCL_OK;
int	 i;
int      maxArgs = 3;
int      entryArgc = 0;
char     ebuf[50];
char     **entryArgv = NULL;
char     *entryPtr = NULL;
char     *lutName = NULL;
LUT      *lutPtr = NULL;

/* Parse the arguments */
if (argc > maxArgs)
   {
   /* Too many arguments */
   Tcl_SetResult(a_interp, "Invalid command format - too many arguments.",
		 TCL_VOLATILE);
   Tcl_AppendResult(a_interp, TCLUSAGE_LUTGET, (char *)NULL);
   rstatus = TCL_ERROR;
   }
else
   {
   for (i = 1 ; i < argc ; ++i)
      {
      switch (argv[i][0])
         {
         case '-':
	    /* This is an invalid parameter */
	    rstatus = TCL_ERROR;
            Tcl_SetResult(a_interp, "No options accepted by this command.",
			  TCL_VOLATILE);
	    Tcl_AppendResult(a_interp, TCLUSAGE_LUTGET, (char *)NULL);
            break;
         default:
            /* This is either the lookup table name or the list of entries in
	       the table that we are to set. The lookup table name must come
	       first. */
	    if (lutName == 0)
	       {
	       /* We have not gotten this yet. */
	       lutName = argv[i];
	       }
	    else
	       {
	       /* We got the lookup table, so the second parameter must be the
		  entry to fill in. */
	       entryPtr = argv[i];
	       }
         }
      }

   /* Check to make sure we got all the parameters we need */
   if (lutName == 0)
      {
      /* We must have a lookup table name */
      rstatus = TCL_ERROR;
      Tcl_AppendResult(a_interp,
		       "A lookup table name MUST be entered.\n", (char *)NULL);
      Tcl_AppendResult(a_interp, TCLUSAGE_LUTGET, (char *)NULL);
      }
   else if (entryPtr == 0)
      {
      /* Did not get any entries, get the whole lookup table. */
      entryArgc = LUT8TOP;
      }

   /* Only continue on if we have not gotten an error so far. */
   if (rstatus == TCL_OK)
      {
      /* Get the address of the lookup table from the name */
      if ((rstatus = shTclLutAddrGetFromName(a_interp, lutName, &lutPtr))
	  == TCL_OK)
	 {
	 /* Break the entered entries (the parts of the lookup table to be
	    gotten), they are now a tcl list. This is only true if we have
	    not set entryArgc to be the max number of elements in which case 
	    no entry parameter was specified and we must get the whole lookup
	    table. */
	 if (entryArgc == 0)
	    {
	    /* We were given entries,  this may be a list so try and split
	       them */
	    if ((rstatus = Tcl_SplitList(a_interp, entryPtr, &entryArgc,
					 &entryArgv)) != TCL_OK)
	       {
	       Tcl_AppendResult(a_interp,
				"Error from Tcl_SplitList when trying to split entry parameter.\n",
				(char *)NULL);
	       goto exit;
	       }
	    
	    /* Make sure we were not given too many entries. */
	    if (entryArgc > LUT8TOP)
	       {
	       rstatus = TCL_ERROR;
	       sprintf(ebuf, "Too many entries, must be < or = %d\n", LUT8TOP);
	       Tcl_AppendResult(a_interp, ebuf, (char *)NULL);
	       goto exit;
	       }
	    }
	 
	 /* Get the lookup table entries and build a Tcl list */
	 rstatus = getLUT(a_interp, entryArgc, entryArgv, lutPtr);

	 /* Free the arrays allocated by Tcl_SplitList */
	 if (entryArgv != NULL)
	    free((char *)entryArgv);
	 }
      }
   }

exit:
return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: shTclLutDeclare
**
** DESCRIPTION:
**	Declare all the Tcl verbs.
**
** RETURN VALUES:
**	TCL_OK	    Successful completion.  The Interp result string will
**		    contain the new lookup table name.
**	TCL_ERROR   Error occurred.  The Interp result string will contain
**		    an error string.
**
**============================================================================
*/
void shTclLutDeclare
   (
   Tcl_Interp	*a_interp
   )   
{
char *helpFacil = "shLut";
 
shTclDeclare
  (a_interp, "lutNew", (Tcl_CmdProc *) shTclLutNew, (ClientData) 0,
             (Tcl_CmdDeleteProc *) 0, helpFacil, 
	     TCLHELP_LUTNEW, TCLUSAGE_LUTNEW);
 
shTclDeclare(a_interp, "lutDel", (Tcl_CmdProc *) shTclLutDel,
	     (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
	     TCLHELP_LUTDEL, TCLUSAGE_LUTDEL);
 
shTclDeclare(a_interp, "lutSet", (Tcl_CmdProc *) shTclLutSet,
	     (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
	     TCLHELP_LUTSET, TCLUSAGE_LUTSET);
 
shTclDeclare(a_interp, "lutGet", (Tcl_CmdProc *) shTclLutGet,
	     (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
	     TCLHELP_LUTGET, TCLUSAGE_LUTGET);
 
return;
}



