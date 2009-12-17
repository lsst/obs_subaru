/*<AUTO>
 * FILE:
 *   tclSaoMaskDisplay.c
 *
 * ABSTRACT:
 *<HTML>
 *   This file contains the routines used to display a mask on FSAO through
 *   TCL.
 *</HTML>
 *</AUTO>
 *
 * ENTRY POINT             SCOPE    DESCRIPTION
 * ----------------------------------------------------------------------------
 * genLUTWProc             local    Generate the LUT using a TCL procedure.
 * genLUTWScript           local    Generate the LUT using a TCL script.
 * shTclSaoMaskDisplay     local    Display masks on the selected FSAO.
 * shTclSaoMaskColorSet    local    Set the colors associated w/ the LUT values.
 * shTclSaoMaskGlyphSet    local    Set the glyph used for zoomed masked pixels.
 * shTclSaoMaskDisplayDeclare local Declare the TCL verbs to TCL.    
 *
 * AUTHORS:
 *   Gary Sergey
 *   Eileen Berman
 *   Tom Nicinski
 *
 *<HTML>
  <H2><A NAME=ColorGen>Generating the Colors</A></H2>
   A user supplied TCL script or procedure is used to translate the mask values
   to color values.  Eight color values (plus transparent) are allowed.  Thus
   the 256 possible mask values (0 - 255) must be mapped onto 9 values (0 - 8).
   The colors specified in the <A HREF="#saoMaskColorSet">saoMaskColor Set</A>
   command map directly onto values 1 - 8.  Value 0 is always configured to be
   transparent.  The function of the user supplied TCL script or procedure is to
   take as input a single mask value (0 - 255) and return a single color value 
   (0 - 8).  In this manner the meaning of the mask is interpreted in a user 
   defined way.  These generated color values are used to display the mask in 
   FSAO.  The user supplied TCL procedure must take the mask value as it's
   last argument and return the color value.  The user supplied TCL script
   must use the variable 'maskVal' to mean the mask value. For example, if the
   following commands were entered (This is after a region and accompanying
   mask have been generated.) -
   <LISTING>
      dervish>saoMaskColorSet {red yellow red green blue blue yellow green}
      dervish>saoMaskDisplay h0 -t {expr {($maskVal&amp;1) ? 1 : 0}}
   </LISTING>
   This script would result in only the colors red (corresponding to 1) or
   transparent being displayed.  All mask values that had the low bit set would
   appear as red and all others would not be masked.
 
   <P>Or when using a TCL procedure -
   <LISTING>
      dervish>saoMaskColorSet {red yellow red green blue blue yellow green}
      dervish>saoMaskDisplay h0 -p myProcedure
   </LISTING>
 * </HTML>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>       /* For prototype of atoi() */
#include <sys/types.h>
#include <alloca.h>

#include "imtool.h"
#include "tcl.h"
#include "tk.h"
#include "region.h"
#include "shCErrStack.h"
#include "shCUtils.h"
#include "shTclUtils.h"
#include "shCFitsIo.h"
#include "dervish_msg_c.h"
#include "shCSao.h"
#include "shTclHandle.h"
#include "shTclErrStack.h"

/*
 * Global variables
 */
char       lut[LUTTOP];			/* Look up table */
char       colorTable[CHARSPERCOLOR*NUMCOLOR];	/* list of colors for mask */
int        colorTableComplete = FALSE;
/*
 * External variables referenced in this file
 */
extern SAOCMDHANDLE g_saoCmdHandle;
extern int g_saoMouseVerbose;

/*
 * Locally used function prototypes. These functions are used in this source
 * file only.
 */
static int shTclSaoMaskDisplay(ClientData, Tcl_Interp *, int, char **);


/*
 * ROUTINE:
 *   genLUTWProc
 * 
 * CALL:
 *   (static int) grnLUTWProc(Tcl_Interp *interp, char *tclProc);
 *
 * DESCRIPTION:
 *   Generate the LUT from the passed in TCL procedure.
 *
 * RETURN VALUES:
 *   TCL_OK
 *   TCL_ERROR
 */
static int genLUTWProc
   (
   Tcl_Interp *a_interp,
   char       *a_tclProc
   )
{
int   tstatus = TCL_OK;
char  anum[100];
char  errmsg[80];
int   i;
double result;

/* Generate the look up table using the TCL procedure */
for (i = 0 ; i < LUTTOP ; ++i)
   {
   sprintf(anum, "%s %d", a_tclProc, i);
   if(Tcl_Eval(a_interp, anum) == TCL_OK)
      {
      if (Tcl_GetDouble(a_interp, a_interp->result, &result) == TCL_OK)
         {
         /* Allow the result to be = 0, which always means transparent */
         if ((((int )result < BOTTOMCOLOR) || ((int )result > TOPCOLOR))
             && ((int )result != 0))
            {
            /* Result is out of range */
            tstatus = TCL_ERROR;
            sprintf(errmsg, "%d to %d or 0.", BOTTOMCOLOR, TOPCOLOR);
            Tcl_AppendResult(a_interp, "\nResults from '", a_tclProc,
                            " ", anum,
                            "' must be in the range of ", errmsg, (char *)NULL);
            break;
            }
         lut[i] = (char)result;	/* store transform in look up table */
         }
      else
         {
         /* Error in Tcl script */
         tstatus = TCL_ERROR;
         break;
         }
      }
   else
      {
      /* Error in Tcl script */
      tstatus = TCL_ERROR;
      break;
      }
   }

return(tstatus);
}

/*
 * ROUTINE:
 *   genLUTWScript
 * 
 * CALL:
 *   (static int) genLUTWScript(Tcl_Interp *interp, char *tclScript);
 *
 * DESCRIPTION:
 *   Generate the LUT from the passed in TCL script.
 *
 * RETURN VALUES:
 *   TCL_OK
 *   TCL_ERROR
 */
static int genLUTWScript
   (
   Tcl_Interp *a_interp,
   char       *a_tclScript
   )
{
int    tstatus = TCL_OK;
char   anum[10];
char   errmsg[80];
char   *savedResult;
char   *savedArray = 0;
int    i;
int    tclFlags = 0;
double result;

/* Since we are going to be resetting the variable maskVal, save the current
   value and restore it on exit. */
savedResult = Tcl_GetVar(a_interp, "maskVal", tclFlags);
if (savedResult != (char *)NULL)
   {
   /* Only get the value if the variable exists. Malloc room for the saved
      value. */
   savedArray = (char *)alloca(strlen(savedResult)+1);
   if (savedArray == (char *)NULL)
      {
      /* Error in malloc */
      tstatus = TCL_ERROR;
      Tcl_AppendResult(a_interp,
                    "\nerror getting space for saved value of maskVal.",
                       (char *)NULL);
      }
   else
      {
      /* save a copy of the value, as this value is overwritten the next time
         the variable is set (later on in this routine). */
      strcpy(savedArray, savedResult);
      }
   }

if (tstatus == TCL_OK)			/* No errors yet */
   {
   for (i = 0 ; i < LUTTOP ; ++i)
      {
      /* First set the TCL variable 'maskVal' */
      sprintf(anum, "%d", i);
      if (Tcl_SetVar(a_interp, "maskVal", anum, tclFlags) != (char *)NULL)
         {
         if (Tcl_Eval(a_interp, a_tclScript) == TCL_OK)
            {
            if (Tcl_GetDouble(a_interp, a_interp->result, &result) == TCL_OK)
               {
               /* Allow the result to be = 0, which always means transparent */
               if ((((int )result < BOTTOMCOLOR) || ((int )result > TOPCOLOR))
                   && ((int )result != 0))
                  {
                  /* Result is out of range */
                  tstatus = TCL_ERROR;
                  sprintf(errmsg, "%d to %d or 0.", BOTTOMCOLOR, TOPCOLOR);
                  Tcl_AppendResult(a_interp, "\nResults from '",
                                   a_tclScript, "' with maskVal = ", anum,
                                   ", must be in the range of ",
                                   errmsg, (char *)NULL);
                  break;
                  }
               lut[i] = (char)result;	/* store in look up table */
               }
            else
               {
               /* Error in Tcl script */
               tstatus = TCL_ERROR;
               break;
               }
            }
         else
            {
            /* Error in Tcl script */
            tstatus = TCL_ERROR;
            break;
            }
         }
      else
         {
         /* Error in setting variable value */
         tstatus = TCL_ERROR;
         break;
         }
      }

   /* Restore the original value of maskVal only if it existed prior to
      calling this routine */
   if (savedResult != (char *)NULL)
      {
      /* The variable existed before this routine was called */
      if (Tcl_SetVar(a_interp, "maskVal", savedArray, tclFlags) == (char *)NULL)
         {tstatus = TCL_ERROR;}		/* Error in Tcl script */
      }
   else
      {
      /* The variable did not exist, so get rid of it */
      tstatus = Tcl_UnsetVar(a_interp, "maskVal", tclFlags);
      }
   }

return(tstatus);
}

static char *shTclSaoMaskDisplayHelp = "Display mask data in FSAOimage.";
static char *shTclSaoMaskDisplayUse =
 "USAGE: saoMaskDisplay <mask name> [-t TCL script | -p TCL procedure] [-s FSAO #]";
/*
 * ROUTINE:
 *   shTclSaoMaskDisplay
 *
 * RETURN VALUES:
 *   TCL_OK
 *   TCL_ERROR
 *<AUTO>
 * TCL VERB:
 *   saoMaskDisplay
 *
 * DESCRIPTION:
 *<HTML>
 * The saoMaskDisplay command will display the specified mask,
 * using FSAOimage.  The following must be true before
 * attempting to display a mask -
 * <DIR>
 * <LI>A copy of FSAOimage must be running.
 * <LI>The command <A HREF="#saoMaskColorSet">saoMaskColorSet</A>
 * must have been executed.
 * <LI>The specified mask must be the same size as the region
 * currently being displayed.
 * </DIR>
 * After issuing the saoMaskDisplay command the masked region
 * is displayed immediately. Clicking on the right mouse button
 * (on a right handed mouse) will toggle between displaying the
 * masked region and the unmasked region.  FSAO must be in SCALE
 * mode for this to work. NOTE: this means the right mouse button
 * cannot be used for blinking.
 *<P>
 * The values of BOTTOMCOLOR and TOPCOLOR are defined in
 * <A HREF="../../include/imtool.h">imtool.h</A>.
 * <P>See also <A HREF="#ColorGen">Generating the mask colors</A>.
 *</HTML>
 *
 * SYNTAX:
 *  saoMaskDisplay  <name> [-t TCL script | -p TCL procedure] [-s FSAO #]
 *  	<name>          Name of mask to display.
 *
 *	[-t TCL script | -p TCL procedure]
 *			Optional TCL script or procedure used to
 *                      translate the mask values into color values for
 *                      display. The default is to assume that the mask
 *                      associated with the region already specifies the
 *                      color values.  These values must be between 1 and 8.
 *                      For an example of the TCL script see the top of this
 *                      page.  The TCL procedure can be any user written 
 *                      procedure that takes as a parameter an index into the
 *                      mask and returns the value of the mask which must be
 *                      between BOTTOMCOLOR and TOPCOLOR.
 *	[-s FSAO #]     Optional FSAOimage program to display mask in.  The
 *			default is to draw the glyph in the lowest
 *                      numbered FSAOimage program belonging to the current
 *                      DERVISH process.
 *</AUTO>
 */
static int shTclSaoMaskDisplay
   (
   ClientData clientData, 
   Tcl_Interp *interp, 
   int argc, 
   char *argv[]
   )
{
int         tstatus;
MASK        *maskPtr = NULL;
char        *transMaskPtr = NULL, *tempMaskPtr;
char        *tclProc = NULL;
char        *tclScript = NULL;
int         saoindex  = SAOINDEXINITVAL;
char        errmsg[80];
char        asaonum[8];                     /* Ascii version of saonumber */
char        *maskName = NULL;
int         i;

/*---------------------------------------------------------------------------
**
** CHECK COMMAND ARGUMENTS
*/
if (argc < 2)
   {
   /* Invalid command entered. */
   tstatus = TCL_ERROR;
   Tcl_SetResult(interp, shTclSaoMaskDisplayUse, TCL_VOLATILE);
   }
else
   {
   /* Check for optional arguments.  This is either the TCL procedure , script 
      and/or the FSAO number to send the mask to. */
   for (i = 1 ; i < argc ; ++i)
      {
      switch (argv[i][0])
         {
         case '-':
            switch (argv[i][1])
               {
               case 't':
                  /* This is the TCL script */
                  if (tclProc != 0)
                     {
                     /* Cannot have both a script and a procedure entered. */
                     tstatus = TCL_ERROR;
                     Tcl_SetResult(interp,
                     "Enter EITHER a TCL script or a TCL procedure, not both\n",
                                   TCL_VOLATILE);
                     goto exit;
                     }
                  tclScript = argv[++i];	/* Point to the Tcl script */
                  break;
               case 'p':
                  /* This is the TCL procedure */
                  if (tclScript != 0)
                     {
                     /* Cannot have both a script and a procedure entered. */
                     tstatus = TCL_ERROR;
                     Tcl_SetResult(interp,
                     "Enter EITHER a TCL script or a TCL procedure, not both\n",
                                   TCL_VOLATILE);
                     goto exit;
                     }
                  tclProc = argv[++i];		/* Point to the Tcl proc */
                  break;
               case 's':
                  /* This is the FSAO number - of the form : -s num */
                  saoindex = atoi(argv[i+1]) - 1;    /* Index starts at 0 not 1 */
		  i++;
                  break;
               default:
                  /* This is an invalid parameter */
                  tstatus = TCL_ERROR;
                  Tcl_SetResult(interp, "ERROR : Invalid option entered - ",
                                TCL_VOLATILE);
                  Tcl_AppendResult(interp, argv[i], (char *)NULL);
                  goto exit;
               }
            break;
         default:
            /* This must be the mask handle */
            maskName = argv[i];
         }
      }

   /* Check the saoindex value to see if there is an fsao out there belonging
      to us */
   if (p_shCheckSaoindex(&saoindex) != SH_SUCCESS)
      {
      /* OOPS this is not a valid value for saoindex */
      shTclInterpAppendWithErrStack(interp);
      tstatus = TCL_ERROR;
      goto exit;
      }

   /* Check if this is a valid mask by trying to get the pointer to it */
   if ((shTclAddrGetFromName(interp, maskName,
                   (void **)&maskPtr, "MASK") == TCL_OK) || (maskName == NULL))
      {
      /* Make sure the mask is the same size as the region being displayed in
         FSAO. */
      if ((g_saoCmdHandle.saoHandle[saoindex].rows == maskPtr->nrow) &&
          (g_saoCmdHandle.saoHandle[saoindex].cols == maskPtr->ncol))
         {
         /* Always copy the MASK over to a contiguous area. */
         transMaskPtr = (char *)alloca(maskPtr->nrow * maskPtr->ncol);
         if (transMaskPtr == NULL)
            {
            /* Could not alloca space for the new mask. */
            tstatus = TCL_ERROR;
            Tcl_SetResult(interp,
                          "Could not alloca space for the temporary mask.",
                          TCL_VOLATILE);
            goto exit;
            }
         tempMaskPtr = transMaskPtr;
         for (i = 0 ; i < maskPtr->nrow ; ++i)
            {
            memcpy(tempMaskPtr, maskPtr->rows[i], maskPtr->ncol);
            tempMaskPtr += maskPtr->ncol;		/* go to next row */
            }
         if (tclProc != 0)
            {
            /* We have a TCL procedure to use */
            if ((tstatus = genLUTWProc(interp, tclProc)) == TCL_ERROR)
               {goto exit;}	/* Error message is set in procedure. */
            }
         else if (tclScript != 0)
            {
            /* We have a TCL script to use */
            if ((tstatus = genLUTWScript(interp, tclScript)) == TCL_ERROR)
               {goto exit;}	/* Error message is set in procedure. */
            }
         else
            {
            /* No transforms to do - LUT reflects index values. */
            for (i = 0; i < LUTTOP; i++) { lut[i] = i; }
            }

         /* Send mask to FSAOimage */
	 if (shSaoMaskDisplay(transMaskPtr, maskPtr, lut, saoindex) != SH_SUCCESS)
            {
            /* Error when writing to FSAO - get the error message */
            shTclInterpAppendWithErrStack(interp);
            tstatus = TCL_ERROR;
            }
         else
            {
            tstatus = TCL_OK;
            sprintf(asaonum, "%d", saoindex+1);	/* output the sao # */
            Tcl_SetResult(interp, asaonum, TCL_VOLATILE);
            }
         }
      else
         {
         /* Size of mask must be equal to size of region and it is not. */
         tstatus = TCL_ERROR;
         Tcl_SetResult(interp, "Size of mask ", TCL_VOLATILE);
         sprintf(errmsg, "(%d x %d) must equal size of region (%d x %d).",
                         maskPtr->nrow, maskPtr->ncol,
                         g_saoCmdHandle.saoHandle[saoindex].rows,
                         g_saoCmdHandle.saoHandle[saoindex].cols);
         Tcl_AppendResult(interp, errmsg, (char *)NULL);
         }
      }
   else
      {
      /* Invalid mask specified */
      tstatus = TCL_ERROR;
      Tcl_AppendResult(interp, "\nInvalid mask '", argv[1],
                               "' specified.\n", (char *)NULL);
      }
   }

/*---------------------------------------------------------------------------
**
** RETURN
*/
exit:
return(tstatus);

}

static char *shTclSaoMaskColorSetHelp ="Set the colors for displaying the mask";
static char *shTclSaoMaskColorSetUse ="USAGE: saoMaskColorSet <TCL Color List>";
/*
 * ROUTINE:
 *   shTclSaoMaskColorSet
 * 
 *<AUTO>
 * TCL VERB:
 *   saoMaskColorSet
 *
 * DESCRIPTION:
 *<HTML>
 *  The saoMaskColorSet command allows the user to specify the colors to be used
 *  when displaying the mask in FSAO.  The color names should be those
 *  recognized by X (e.g. RED, BLUE, GREEN...) and are not case sensitive. It
 *  should be noted that X color names may differ from machine to machine.  An
 *  exact color match is not guaranteed.  An attempt will be made to get the
 *  closest available color to the requested color.  The specified colors will
 *  be used to display all masks until this command is entered with new color 
 *  values.
 *<P>
 *  Eight colors (plus transparent) can be used to display a mask. By default,
 *  the mask color on a monochrome display is white.
 *</HTML>
 * SYNTAX:
 *  saoMaskColorSet  <tclColorList>
 *	<tclColorList>  TCL list of colors to be used when displaying a
 *			mask.  Must contain 8 color values.  For example,
 *			{RED BLUE GREEN YELLOW RED BLUE GREEN YELLOW}
 *</AUTO>
 */
static int shTclSaoMaskColorSet
   (
   ClientData clientData, 
   Tcl_Interp *interp, 
   int argc, 
   char *argv[]
   )
{
int   tstatus = TCL_OK;
int   listArgc;
char  **listArgv;
char  errmsg[20];

/* Check the parameters */
if (argc == 2)
   {
   /* Expand the parameter list out. */
   if (Tcl_SplitList(interp, argv[1], &listArgc, &listArgv) == TCL_OK)
      {
      /* The number of elements in the list better equal the number of LUT
         color values */
      if (listArgc == NUMCOLOR)
         {
	 if (shSaoMaskColorSet(listArgv) != SH_SUCCESS)
	    {
	    tstatus = TCL_ERROR;
	    shTclInterpAppendWithErrStack(interp);
	    }
         }
      else
         {
         /* Too many or too little color values in the list. */
         tstatus = TCL_ERROR;
         sprintf(errmsg, "%d", NUMCOLOR);
         Tcl_SetResult(interp, "Invalid number of colors entered.",
                       TCL_VOLATILE);
         Tcl_AppendResult(interp, "\nNumber of Colors should = ", errmsg,
                          (char *)NULL);
         }

      /* free the space allocated by Tcl_SplitList */
      free((char *)listArgv);
      }
   else
      {
      tstatus = TCL_ERROR;
      Tcl_AppendResult(interp, "\nCannot split TCL list.", TCL_VOLATILE);
      }
   }
else
   {
   /* Wrong number of parameters */
   tstatus = TCL_ERROR;
   Tcl_SetResult(interp, "Invalid command format.", TCL_VOLATILE);
   Tcl_AppendResult(interp, "\n", shTclSaoMaskColorSetUse, (char *)NULL);
   }

return(tstatus);
}

static char *shTclSaoMaskGlyphSetHelp ="Set the glyph for displaying the mask";
static char *shTclSaoMaskGlyphSetUse ="USAGE: saoMaskGlyphSet <glyph> [-s FSAO #]";
/*
 * ROUTINE:
 *   shTclSaoMaskGlyphSet
 * 
 *<AUTO>
 * TCL VERB:
 *   saoMaskGlyphSet
 *
 * DESCRIPTION:
 *<HTML>
 *  The saoMaskGlyphSet command allows the user to specify the glyph to be used
 *  when displaying the mask in FSAO.  The glyph is displayed when the image is
 *  magnified larger than lifesize.  The possible glyphs are "x" or "box".
 *  saoMaskGlyphSet can be called only after FSAO is started with
 *  <A HREF="#saoMaskDisplay">saoMaskDisplay</A> <EM>and</EM> applies only to
 *  that particular FSAOimage.  By default, FSAO uses the "x" glyph.
 *</HTML>
 * SYNTAX:
 *  saoMaskGlyphSet  <glyph> [-s FSAO #]
 *	<glyph>         The glyph name -- either X or BOX.
 *
 *	[-s FSAO #]     Optional FSAOimage program to display mask in.  The
 *			default is to draw the glyph in the lowest
 *                      numbered FSAOimage program belonging to the current
 *                      DERVISH process.
 *</AUTO>
 */
static int shTclSaoMaskGlyphSet
   (
   ClientData clientData, 
   Tcl_Interp *interp, 
   int argc, 
   char *argv[]
   )
{
int   tstatus  = TCL_OK;
int   saoindex = SAOINDEXINITVAL;
int   i;
int   paramCnt = 0;
int         glyph = M_GLYPH_X;	/* Init to appease compiler warning gods */
SHKEYENT    glyphList[] = {	{"x",   M_GLYPH_X},
				{"box", M_GLYPH_BOX},
				{ NULL,   0  }};

if (argc < 2)
   {
   /* Invalid command entered. */
   tstatus = TCL_ERROR;
   Tcl_SetResult(interp, shTclSaoMaskGlyphSetUse, TCL_VOLATILE);
   }
else
   {
   /* Check for optional arguments. This is the FSAO number to send the glyph
      to. */
   for (i = 1 ; i < argc ; ++i)
      {
      switch (argv[i][0])
         {
         case '-':
            switch (argv[i][1])
               {
               case 's':
                  /* This is the FSAO number - of the form : -s num */
                  saoindex = atoi(argv[i+1]) - 1;    /* Index starts at 0 not 1 */
		  i++;
                  break;
               default:
                  /* This is an invalid parameter */
                  tstatus = TCL_ERROR;
                  Tcl_SetResult(interp, "ERROR : Invalid option entered - ",
                                TCL_VOLATILE);
                  Tcl_AppendResult(interp, argv[i], (char *)NULL);
                  goto exit;
               }
            break;
         default: /* Assume to be the glyph */
            if (++paramCnt == 2)	/* Display message once */
               {
               /* Wrong number of parameters */
               tstatus = TCL_ERROR;
               Tcl_SetResult(interp, "Invalid command format.", TCL_VOLATILE);
               Tcl_AppendResult(interp, "\n", shTclSaoMaskGlyphSetUse, (char *)NULL);
               break;
               }
               /* Check the supported glyph types. */
               if (shKeyLocate (argv[1], glyphList, (unsigned int *)&glyph, SH_CASE_INSENSITIVE) != SH_SUCCESS)
                  {
                  tstatus = TCL_ERROR;
                  Tcl_SetResult(interp, "Invalid glyph", TCL_VOLATILE);
                  }
            break;
         }
      }

   /* Check the saoindex value to see if there is an fsao out there belonging
      to us */
   if (p_shCheckSaoindex(&saoindex) != SH_SUCCESS)
      {
      /* OOPS this is not a valid value for saoindex */
      shTclInterpAppendWithErrStack(interp);
      tstatus = TCL_ERROR;
      goto exit;
      }
   }
   if (tstatus == TCL_OK)
      {
      if (shSaoMaskGlyphSet(glyph, saoindex) != SH_SUCCESS)
         {
         tstatus = TCL_ERROR;
         shTclInterpAppendWithErrStack(interp);
         }
      }

exit : ;

return(tstatus);
}

/*
 * ROUTINE:
 *   shTclSaoMaskDisplayDeclare
 *
 * CALL:
 *   (void) shTclSaoMaskDisplayDeclare(
 *               Tcl_Interp *a_interp  // IN: TCL interpreter
 *          )
 *
 * DESCRIPTION:
 *   The TCL world's access to SAO mask display function.
 *
 * RETURN VALUE:
 *   Nothing
 */
void
shTclSaoMaskDisplayDeclare(Tcl_Interp *a_interp)
{
   char *helpFacil = "shFsao";
 
   shTclDeclare(a_interp, "saoMaskDisplay", (Tcl_CmdProc *) shTclSaoMaskDisplay,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                shTclSaoMaskDisplayHelp, shTclSaoMaskDisplayUse);
 
   shTclDeclare(a_interp, "saoMaskColorSet",
                (Tcl_CmdProc *) shTclSaoMaskColorSet,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                shTclSaoMaskColorSetHelp, shTclSaoMaskColorSetUse);

   shTclDeclare(a_interp, "saoMaskGlyphSet",
                (Tcl_CmdProc *) shTclSaoMaskGlyphSet,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                shTclSaoMaskGlyphSetHelp, shTclSaoMaskGlyphSetUse);
 
   return;
}
