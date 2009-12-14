/*****************************************************************************
******************************************************************************
**
** FILE:
**	tclDither.c
**
** ABSTRACT:
**	This file contains code to simulate the image processsing needed
**	to square-root encode and then restore - with dithering of an
**	image.
** NOT FINISHED:
**	The random number generator is internal and comes from numerical
**	recipes.  It should be replaced.
**
** ENTRY POINT		SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
** phTclSqrtDeclare	public	declare all the verbs defined in this module
** tclSqrt		static	Square root unsigned short int image
** tclSqrDither		static  Reverse above
** tclSim		static	Simulate a noisy image
**
** ENVIRONMENT:
**	ANSI C.
**
** REQUIRED PRODUCTS:
**	FTCL	 	TCL + Fermi extension
**      DERVISH
**	PHOTO
**
** AUTHORS:
**	Creation date:  Oct. 1993
**	Steve Kent
**
******************************************************************************
******************************************************************************
*/
#include <stdio.h>
#include <string.h>
#include "dervish.h"
#include "phDither.h"

static char *module = "tclDither.c";		/* name of this set of code */


/*============================================================================  
**============================================================================
**
** TCL VERB: tclSqrt
**
** DESCRIPTION:
**	Call routine to take sqrt of each pixel in an unsigned short int image.
**	This processing is done to enhance compressibility of an image.
**
** RETURN VALUES:
**	TCL_OK -   Successful completion.
**	TCL_ERROR -   Failed miserably.
**
** CALLS TO:
**	imgSqrt
**
**============================================================================
*/
/*
 * Input is handle to image, read noise, gain, quantization value
 * No return
 */

static char *tclSqrt_use =
  "USAGE: phSqrt <region-handle> rnoise gain quant";
static char *tclSqrt_hlp =
  "Take Square root of pixel values of unsigned short ints";

static int
tclSqrt(
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{
   REGION *reg = NULL;
   UDATA **rows;
   int nr, nc;
   double rnoise, gain, quant;

   shErrStackClear();

   if(argc != 5) {
      Tcl_SetResult(interp,tclSqrt_use,TCL_STATIC);
      return(TCL_ERROR);
   }

/* Get the region pointer */

   if (shTclAddrGetFromName(interp, argv[1], (void **)&reg, "REGION") != TCL_OK)
	return (TCL_ERROR);


/* Get addr of unsigned int rows */

   if (reg -> type != TYPE_U16) {
        Tcl_SetResult (interp, "Must be unsigned 16 bit integers.", TCL_STATIC);
        return (TCL_ERROR);
        }
   rows = reg ->rows_u16;
   nr = reg -> nrow;
   nc = reg -> ncol;

/* Get the remaining parameters */

   if (Tcl_GetDouble (interp, argv[2], &rnoise)  != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetDouble (interp, argv[3], &gain)  != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetDouble (interp, argv[4], &quant)  != TCL_OK) return(TCL_ERROR);


/* Done with the handles, etc. Now the action begins */

   phSqrt (rows, nr,nc, rnoise, gain, quant);
   return(TCL_OK);
}


/*============================================================================  
**============================================================================
**
** TCL VERB: tclSqr
**
** DESCRIPTION:
**	Call routine to take sqr of each pixel in an unsigned short int image.
**	This is restoration of imgSqrt
**
** RETURN VALUES:
**	TCL_OK -   Successful completion.
**	TCL_ERROR -   Failed miserably.
**
** CALLS TO:
**	imgSqr
**
**============================================================================
*/
/*
 * Input is handle to image, read noise, gain, quantization value
 * No return
 */

static char *tclSqr_use =
  "USAGE: phSqr <region-handle> rnoise gain quant";
static char *tclSqr_hlp =
  "Take Square of pixel values";

static int
tclSqr(
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{

   REGION *reg = NULL;
   UDATA **rows;
   int nr, nc;
   double rnoise, gain, quant;

   shErrStackClear();

   if(argc != 5) {
      Tcl_SetResult(interp,tclSqr_use,TCL_STATIC);
      return(TCL_ERROR);
   }

/* Get the region pointer */

   if (shTclAddrGetFromName(interp, argv[1], (void **)&reg, "REGION") != TCL_OK)
	return (TCL_ERROR);


/* Get addr of unsigned int rows */

   if (reg -> type != TYPE_U16) {
        printf ("Must be unsigned 16 bit integers.\n");
        printf ("Type is %d\n",reg->type);
        return (TCL_ERROR);
        }
   rows = reg ->rows_u16;
   nr = reg -> nrow;
   nc = reg -> ncol;

/* Get the remaining parameters */

   if (Tcl_GetDouble (interp, argv[2], &rnoise)  != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetDouble (interp, argv[3], &gain)  != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetDouble (interp, argv[4], &quant)  != TCL_OK) return(TCL_ERROR);


/* Done with the handles, etc. Now the action begins */

   phSqrDither (rows, nr,nc, rnoise, gain, quant); 
   return(TCL_OK);
}


/*============================================================================  
**============================================================================
**
** TCL VERB: tclSim
**
** DESCRIPTION:
**	Call routine to simulate a noisy image.
**
** RETURN VALUES:
**	TCL_OK -   Successful completion.
**	TCL_ERROR -   Failed miserably.
**
** CALLS TO:
**	imgSim
**
**============================================================================
*/
/*
 * Input is handle to image, read noise, gain, quantization value
 * No return
 */

static char *tclSim_use =
  "USAGE: phSim <region-handle> rnoise gain sky";
static char *tclSim_hlp =
  "Simulate a noisy image";

static int
tclSim(
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{
   REGION *reg = NULL;
   UDATA **rows;
   int nr, nc;
   double rnoise, gain, sky;

   shErrStackClear();

   if(argc != 5) {
      Tcl_SetResult(interp,tclSim_use,TCL_STATIC);
      return(TCL_ERROR);
   }

/* Get the region pointer */

   if (shTclAddrGetFromName(interp, argv[1], (void **)&reg, "REGION") != TCL_OK)
	return (TCL_ERROR);


/* Get addr of unsigned int rows */

   if (reg -> type != TYPE_U16) {
        Tcl_SetResult (interp, "Must be unsigned 16 bit integers.", TCL_STATIC);
        return (TCL_ERROR);
        }
   rows = reg ->rows_u16;
   nr = reg -> nrow;
   nc = reg -> ncol;

/* Get the remaining parameters */

   if (Tcl_GetDouble (interp, argv[2], &rnoise)  != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetDouble (interp, argv[3], &gain)  != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetDouble (interp, argv[4], &sky)  != TCL_OK) return(TCL_ERROR);


/* Done with the handles, etc. Now the action begins */

   phSim (rows, nr,nc, rnoise, gain, sky);
   return(TCL_OK);
}


/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclDitherDeclare(Tcl_Interp *interp) 
{
   shTclDeclare(interp,"phSqrt", 
		(Tcl_CmdProc *)tclSqrt,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclSqrt_hlp, tclSqrt_use);

   shTclDeclare(interp,"phSqr", 
		(Tcl_CmdProc *)tclSqr,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclSqr_hlp, tclSqr_use);

   shTclDeclare(interp,"phSim", 
		(Tcl_CmdProc *)tclSim,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclSim_hlp, tclSim_use);

   return;
}

