/*++
 * FACILITY:	Dervish
 *
 * ABSTRACT:	Data alignment handling under Tcl.
 *
 *   Public
 *   --------------------------	------------------------------------------------
 *
 *   Private
 *   --------------------------	------------------------------------------------
 *   p_shTclAlignPrimElemRet	Return a single primitive element.
 *
 * ENVIRONMENT:	ANSI C.
 *		tclAlign.c
 *
 * AUTHOR: 	Tom Nicinski, Creation date: 12-Mar-1994
 *--
 ******************************************************************************/

/******************************************************************************/
/*						.----------------------------.
 * Outstanding Issues / Problems / Etc.		|  Last Update: 12-Mar-1994  |
 *						`----------------------------'
 *
 * This section lists out all the current problems with alignment processing and
 * any suggested fixes and enhancements. Please keep this section C U R R E N T.
 *
 *- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * Routine:
 *
 * Problem:
 *
 * Solution:
 *
 *- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 ******************************************************************************/

/******************************************************************************/

#include	<alloca.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>

#include	"ftcl.h"

#include	"shCSchema.h"

#include	"shTclUtils.h"  		/* For shTclDeclare ()        */
#include	"shCSao.h"			/* For shTclVerbs.h           */
#include	"shTclVerbs.h"			/* Control C++ name mangling  */
						/* ... of shTclAlignDeclare ()*/
#include	"shCUtils.h"
#include	"shTclAlign.h"
#include	"shCAlign.h"

/******************************************************************************/

/******************************************************************************/

int		 p_shTclAlignPrimElemRet
   (
   Tcl_Interp		 *interp,	/* INOUT: Tcl interpreter structure   */
   SHALIGN_TYPE		  alignType,	/* IN:    Alignment type              */
   TYPE			  schemaType,	/* IN:    For error messages          */
            long  int	  alignStrMax,	/* IN:    STR schema: max string len  */
   unsigned       char	 *elemPtr,	/* IN:    Pointer to data             */
                  char	 *prefix	/* IN:    Prefix character            */
   )

/*++
 * ROUTINE:	 p_shTclAlignPrimElemRet
 *
 *	Return a primitive data type.  Character strings, represented by an
 *	object schema type of STR, are handled.  They are considered to be
 *	null-terminated.  Their output length can be limited by a non-zero
 *	alignStrMax value.
 *
 * RETURN VALUES:
 *
 *   TCL_OK		Success.
 *			The interp result string had the primitve element
 *			appended.
 *
 *   TCL_ERROR		Failure.
 *			A data type that is not handled by this code was
 *			encountered.  The interp result string contains an
 *			error message.
 *
 * SIDE EFFECTS:
 *
 *   o	No single numeric value, when converted to its character representation,
 *	may be larger than 255 characters.  If this does occur, data will be
 *	overwritten beyond the automatic variable lcl_num with unpredictable
 *	results.
 *
 *   o	Floating-point NaNs are returned as "NaN".
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* p_shTclAlignPrimElemRet */

   /*
    * Declarations.
    */

                  int	 lcl_status   = TCL_OK;
                  char	 lcl_num[255];		/* Big enuf to handle any size*/
                  char	*lcl_numPos;		/* char representation of a #.*/
                  char	*lcl_alignStr;

#define	lcl_ForceRealTrail\
	if ((lcl_numPos = strchr (lcl_num, '.')) != ((char *)0))\
	   {\
	   if (*(++lcl_numPos) == 0) { *lcl_numPos = '0'; *(++lcl_numPos) = 0; }\
	   }\
	else\
	   {\
		if ( strchr(lcl_num,'e') == ((char*)0)) \
	   strcat (lcl_num, ".0");\
	   }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   if (schemaType == shTypeGetFromName ("STR"))
      {
      /*
       * Character strings must be null-terminated.
       */

      if (alignStrMax <= 0) { alignStrMax = strlen (((char *)elemPtr)); }
      if ((lcl_alignStr = ((char *)alloca (alignStrMax + 1))) == ((char *)0))
         {
         lcl_status = TCL_ERROR;
         Tcl_SetResult    (interp, "", TCL_VOLATILE); /* Clear out for err msg */
         Tcl_AppendResult (interp, "Unable to allocate space for building character string", ((char *)0));
         goto rtn_return;
         }
      strncpy (lcl_alignStr, ((char *)elemPtr), alignStrMax); lcl_alignStr[alignStrMax] = '\000';
      Tcl_AppendResult (interp, lcl_alignStr, ((char *)0));
      }
   else
      {
      /*
       *   o   Floating point values are output with the %g format specifier.
       *       The precision depends on the type:
       *
       *           float    8 decimal digits
       *           double  17 decimal digits
       *
       *       The %g format specifier cuts off trailing zeros, even the decimal
       *       point if appropriate;  but, this code will output, at a minimum,
       *       a trailing decimal point and zero (lcl_ForceRealTrail).
       */

      switch (alignType)
         {
         case SHALIGNTYPE_U8   : sprintf (lcl_num, "%u",     *((unsigned       char *)elemPtr)); break;
         case SHALIGNTYPE_S8   : sprintf (lcl_num, "%d",     *((  signed       char *)elemPtr)); break;
         case SHALIGNTYPE_U16  : sprintf (lcl_num, "%hu",    *((unsigned short int  *)elemPtr)); break;
         case SHALIGNTYPE_S16  : sprintf (lcl_num, "%hd",    *((         short int  *)elemPtr)); break;
         case SHALIGNTYPE_U32  : sprintf (lcl_num, "%u",     *((unsigned       int  *)elemPtr)); break;
         case SHALIGNTYPE_S32  : sprintf (lcl_num, "%d",     *((               int  *)elemPtr)); break;
         case SHALIGNTYPE_FL32 : if (!shNaNFloatTest  (      *((float               *)elemPtr))) {
                                 sprintf (lcl_num, "%1.8g",  *((float               *)elemPtr)); lcl_ForceRealTrail;
                                 } else {
                                    strcpy (lcl_num, "NaN");
                                 }
                                 break;
         case SHALIGNTYPE_FL64 : if (!shNaNDoubleTest (      *((double              *)elemPtr))) {
                                 sprintf (lcl_num, "%1.17g", *((double              *)elemPtr)); lcl_ForceRealTrail;
                                 } else {
                                    strcpy (lcl_num, "NaN");
                                 }
                                 break;
         default               :
              lcl_status = TCL_ERROR;
              Tcl_SetResult    (interp, "", TCL_VOLATILE); /* Clear out for error message */
              Tcl_AppendResult (interp, "Unsupported data type, ", shNameGetFromType (schemaType), ((char *)0));
              goto rtn_return;
         }
      Tcl_AppendResult (interp, prefix, lcl_num, ((char *)0));
      }

rtn_return : ;

   return (lcl_status);

#undef	lcl_ForceRealTrail

   }	/* p_shTclAlignPrimElemRet */

/******************************************************************************/

void		 p_shTclAlignDeclare
   (
   Tcl_Interp	*interp			/* IN:    Tcl Interpreter structure   */
   )

/*++
 * ROUTINE:	 p_shTclAlignDeclare
 *
 * DESCRIPTION:
 *
 *	Declare Tcl commands for this module.
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

   {	/* p_shTclAlignDeclare */

   /*
    * Declarations.
    */

#define	shTclAlignCLI(TclCmd, rtn, TclHelp, TclUse)\
   shTclDeclare (interp, TclCmd, (Tcl_CmdProc *)(rtn), (ClientData)(0), (Tcl_CmdDeleteProc *)(NULL), "shAlign", TclHelp, TclUse)

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#undef	shTclAlignCLI

   }	/* p_shTclAlignDeclare */

/******************************************************************************/
