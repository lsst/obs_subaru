/****************************************************************************
*****************************************************************************
**<AUTO>
** FILE:
**	tclMaskUtils.c
**
**<HTML>
** <P>Procedures which
** take more than one mask as parameters require that all
** masks have the same number of rows and columns.
**
** <P>Before a result is stored back into a pixel the following conversions
** are performed: 1) If the result is smaller (largest) than the
** smallest (largest) value representable in a pixel, it is set to that
** small (large) value. 
**
** <H2>Rows and Columns</H2>
**
** <P>ROW and COLUMN operands which locate specific pixels in a mask
** may be specified as either floating point or decimal numbers. 0.0,
** 0, 0.5, or 0.9 all refer to the row or column 0.
**</HTML>
**</AUTO>
** ABSTRACT:
**	This file contains software making TCL verbs from the
**	pure C primitives in maskUtils.c. Region Programmer's
**	tools are used from tclRegSupport.c
**
** ENTRY POINT		SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
** shTclMaskUtilDeclare	Public  Declare all the extensions in this file to TCL 
**
**
** ENVIRONMENT:
**	ANSI C.
**	utils		Vanilla C utilities in this package.
**
** REQUIRED PRODUCTS:
**	FTCL	 	TCL + Fermi extension
**
**
** AUTHORS:     Creation date: Jan 24, 1996
**              Heidi Newberg
**		Don Petravick
**              Gary Sergey
**              Eileen Berman
**
*****************************************************************************
*****************************************************************************
*/
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <alloca.h>
#include "ftcl.h"
#include "region.h"
#include "shTclHandle.h"
#include "shTclRegSupport.h"
#include "shTclUtils.h"
#include "shTclVerbs.h"
#include "shCMaskUtils.h"
/*============================================================================  
**============================================================================
**
** LOCAL MACROS, DEFINITIONS, ETC.
**
**============================================================================
*/
#ifndef TRUE
#define TRUE            1
#endif
#ifndef FALSE
#define FALSE           0
#endif


/*----------------------------------------------------------------------------
**
** LOCAL DEFINITIONS
*/
static char *shtclMaskUtilsFacil = "shMask";

/******************************************************************************
******************************************************************************/


/*============================================================================
**============================================================================
**
** ROUTINE: shTclMaskAddrGetFromExpr
**
** DESCRIPTION:
**      Returns a mask address given a handle expression
**
** RETURN VALUES:
**      TCL_OK          success
**      TCL_ERROR       error
**
** CALLS TO:
**      shTclHandleExprEval
**      Tcl_AppendResult
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
int shTclMaskAddrGetFromExpr
   (
   Tcl_Interp *a_interp,
   char       *a_maskExpr,
   MASK       **a_mask
   )   
{
   HANDLE  hand;
   void   *handPtr;
/*
** Find header address
*/
   if (shTclHandleExprEval(a_interp,a_maskExpr,&hand,&handPtr) != TCL_OK) {
      Tcl_AppendResult(a_interp, "\n-----> Unknown mask: ", 
                                  a_maskExpr, (char *) NULL);
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("MASK")) {
      Tcl_AppendResult(a_interp,a_maskExpr," is a ",
                   shNameGetFromType(hand.type)," not a mask (MASK)",(char *)NULL);
      return(TCL_ERROR);
   }
/*
**      Return mask address
*/
   *a_mask = (MASK *)hand.ptr;
   return(TCL_OK);
}

/*============================================================================
**============================================================================
**
**
** DESCRIPTION:
**	Get one pixel from a mask. 
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**============================================================================
*/
static char *shtclMaskGetAsPix_hlp     = "Get One pixel from a mask ";
static char *shtclMaskGetAsPix_use     = "USAGE: maskGetAsPix mask row col";

static int shTclMaskGetAsPix(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   int status;
   MASK *mask;
   int r, c; 
   char s[30];

   if (argc != 4) {
      Tcl_SetResult (interp, shtclMaskGetAsPix_use, TCL_STATIC);
      return (TCL_ERROR);
   }

   if (shTclMaskAddrGetFromExpr(interp, argv[1], &mask) != TCL_OK) return (TCL_ERROR);
   if ((status=shTclRowColStrGetAsInt(interp, argv[2], &r)) != TCL_OK) return (status);
   if ((status=shTclRowColStrGetAsInt(interp, argv[3], &c)) != TCL_OK) return (status);
   if (mask == (MASK *)NULL) {
      Tcl_SetResult (interp, "Mask is NULL", TCL_STATIC);
      return (TCL_ERROR);
   }
   if (r < mask->nrow && c< mask->ncol && r>=0 && c >=0) {
      sprintf (s, "%x", mask->rows[r][c]);
      Tcl_SetResult (interp, s, TCL_VOLATILE);
   }else{
      Tcl_SetResult (interp, "maskGetAsPix: row or col is out of bounds", TCL_STATIC);
      return (TCL_ERROR);
   }
   return (TCL_OK);
}

/*============================================================================
**============================================================================
**
**
** DESCRIPTION:
**	Set one pixel in a mask
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**============================================================================
*/
static char *shtclMaskSetAsPix_hlp     = "Set value of one pixel in a mask";
static char *shtclMaskSetAsPix_use     = "USAGE: maskSetAsPix mask row col value";

static int shTclMaskSetAsPix(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   int status;
   MASK *mask;
   int r, c; 
   int value;
     
   if (argc != 5) {
      Tcl_SetResult (interp, shtclMaskSetAsPix_use, TCL_STATIC);
      return (TCL_ERROR);
   }

   if ((status=shTclMaskAddrGetFromExpr(interp, argv[1], &mask)) != TCL_OK) return (status);
   if ((status=shTclRowColStrGetAsInt(interp, argv[2], &r))    != TCL_OK) return (status);
   if ((status=shTclRowColStrGetAsInt(interp, argv[3], &c))    != TCL_OK) return (status);
   if ((status=Tcl_GetInt(interp, argv[4], &value))            != TCL_OK) return (status);

   if (mask == (MASK *)NULL) {
      Tcl_SetResult (interp, "Mask is NULL", TCL_STATIC);
      return (TCL_ERROR);
   }
   if (r<mask->nrow && c<mask->ncol  && r>=0 && c>=0) {
      mask->rows[r][c] = value;
   }else{
      Tcl_SetResult (interp, "maskSetAsPix: row or col is out of bounds", TCL_STATIC);
      return (TCL_ERROR);
   }
   return (TCL_OK);
}


/*============================================================================
**============================================================================
**
** DESCRIPTION:
**	Logically AND a value into a mask;  "mask &= val"
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**      TCL_ERROR   -   Failed to acquire mask
**============================================================================
*/
static char *shtclMaskAndEqualsValue_hlp     = "AND a mask with an integer, \"mask &= val\"";
static char *shtclMaskAndEqualsValue_use     = "USAGE: maskAndEqualsValue mask value";

static int shTclMaskAndEqualsValue(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   int status;
   MASK *mask;
   int value;
   int npix = 0;			/* number of pixels that are non-zero */
     
   if (argc != 3) {
      Tcl_SetResult (interp, shtclMaskAndEqualsValue_use, TCL_STATIC);
      return (TCL_ERROR);
   }

   if ((status=shTclMaskAddrGetFromExpr(interp, argv[1], &mask)) != TCL_OK) return (status);
   if ((status=Tcl_GetInt(interp, argv[2], &value))              != TCL_OK) return (status);

   if (mask == (MASK *)NULL) {
      Tcl_SetResult (interp, "Mask is NULL", TCL_STATIC);
      return (TCL_ERROR);
   }

   npix = shMaskAndEqualsValue(mask, value);

   {
       char result[10];
       sprintf(result, "%d", npix);
       Tcl_SetResult(interp, result, TCL_VOLATILE);
   }

   return (TCL_OK);
}

/*============================================================================
**============================================================================
**
**
** DESCRIPTION:
**	Copy one mask to another pixel-by-pixel
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**============================================================================
*/
static char *shtclMaskCopy_hlp    = 
                               "Copy one mask to another pixel-by-pixel";
static char *shtclMaskCopy_use    = "USAGE: maskCopy maskIn maskOut";

static int shTclMaskCopy(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   MASK *maskIn;
   MASK *maskOut;

   if (argc != 3) {	
      Tcl_SetResult (interp, shtclMaskCopy_use, TCL_STATIC);
      return (TCL_ERROR);
   }
   if (shTclMaskAddrGetFromExpr(interp, argv[1], &maskIn)  != TCL_OK) return (TCL_ERROR);
   if (shTclMaskAddrGetFromExpr(interp, argv[2], &maskOut) != TCL_OK) return (TCL_ERROR);
   if (maskIn == (MASK *)NULL || maskOut == (MASK *)NULL ) {
      Tcl_SetResult (interp, "Mask is NULL", TCL_STATIC);
      return (TCL_ERROR);
   }
   shMaskCopy(maskIn, maskOut);

   return (TCL_OK);
}
/*============================================================================
**============================================================================
**
**
** DESCRIPTION:
**	Set a mask pixels to specified value.
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**============================================================================
*/
static char *shtclMaskSet_hlp    = "Set mask pixels to specified value ";
static char *shtclMaskSet_use    = "USAGE: maskSet maskIn number";

static int shTclMaskSet(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   MASK *maskIn;
   int number;
   int i;

   if (argc != 3) {	
      Tcl_SetResult (interp, shtclMaskSet_use, TCL_STATIC);
      return (TCL_ERROR);
   }
   if (shTclMaskAddrGetFromExpr(interp, argv[1], &maskIn)  != TCL_OK) return (TCL_ERROR);
   if (Tcl_GetInt(interp, argv[2], &number)              != TCL_OK) return (TCL_ERROR);


   if (maskIn == NULL || maskIn->rows == NULL ||
      maskIn->nrow <= 0 || maskIn->ncol <= 0) {
      Tcl_SetResult (interp, "maskSet: mask is NULL", TCL_STATIC);
      return (TCL_ERROR);
   }

   memset((void *)maskIn->rows[0], number, sizeof(maskIn->rows[0][0])*maskIn->ncol);
   for(i = 1;i < maskIn->nrow;i++) {
      memcpy ((void *)maskIn->rows[i], (void *)maskIn->rows[0],
             sizeof(maskIn->rows[0][0])*maskIn->ncol);
   }

   return (TCL_OK);
}
/*************** Declaration of TCL verbs in this module *****************/

void shTclMaskUtilDeclare(Tcl_Interp *interp) 
{
int flags = 0;

   shTclDeclare (interp, "maskSet", 
                 (Tcl_CmdProc *)shTclMaskSet, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclMaskUtilsFacil , shtclMaskSet_hlp,
                 shtclMaskSet_use);

   shTclDeclare (interp, "maskCopy", 
                 (Tcl_CmdProc *)shTclMaskCopy, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclMaskUtilsFacil , shtclMaskCopy_hlp,
                 shtclMaskCopy_use);

   shTclDeclare (interp, "maskSetAsPix", 
                 (Tcl_CmdProc *)shTclMaskSetAsPix, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclMaskUtilsFacil , shtclMaskSetAsPix_hlp,
                 shtclMaskSetAsPix_use);

   shTclDeclare (interp, "maskGetAsPix", 
                 (Tcl_CmdProc *)shTclMaskGetAsPix, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclMaskUtilsFacil , shtclMaskGetAsPix_hlp,
                 shtclMaskGetAsPix_use);

   shTclDeclare (interp, "maskAndEqualsValue", 
                 (Tcl_CmdProc *)shTclMaskAndEqualsValue, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclMaskUtilsFacil , shtclMaskAndEqualsValue_hlp,
                 shtclMaskSetAsPix_use);
} 
