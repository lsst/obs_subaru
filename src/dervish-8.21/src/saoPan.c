/*
******************************************************************************
**
** FILE: saoPan.c
**   ENTRY POINT	    SCOPE   DESCRIPTION
**   -------------------------------------------------------------------------
**   saoCenter	            local   send centering command to FSAOimage.
**   saoZoom	            local   Send zooming command to FSAOimage.
**   saoCenterWrite	    local   Write centering information to FSAOimage.
**   p_shSaoIndexCheck	    local   Check SAO index to see if it is valid
**
** ENVIRONMENT:
**	ANSI C.
**
** AUTHORS:     Creation date: June 5, 1997
**
******************************************************************************
******************************************************************************
*/
#include "math.h"                 /* for fmod call */
#include "region.h"
#include "shCSao.h"
#include "dervish_msg_c.h"        /* Murmur message file for Dervish */
#include "shCUtils.h"
#include "shCGarbage.h"
#include "shCErrStack.h"

/*============================================================================
**============================================================================
**
** LOCAL PROTOTYPES, DEFINITIONS, MACROS, ETC.
** (NOTE: NONE OF THESE WILL BE VISIBLE OUTSIDE THIS MODULE)
**
**============================================================================
*/

/*----------------------------------------------------------------------------
**
** LOCAL FUNCTION PROTOTYPES
*/
static RET_CODE saoCenterWrite(CMD_SAOHANDLE *);
static RET_CODE saoPanWrite(CMD_SAOHANDLE *, int, int);
static RET_CODE saoZoomWrite(CMD_SAOHANDLE *, double);

/*----------------------------------------------------------------------------
**
** LOCAL DEFINITIONS
*/

/* External variables referenced here */
extern SAOCMDHANDLE g_saoCmdHandle;

/*
**============================================================================
**
** ROUTINE: saoCenterWrite
**
** DESCRIPTION:
**  Routine to write SAO region centering information to SAOimage.
**
** RETURN VALUES:
**  SH_SUCCESS
**  status from shSaoSendImMsg
**
** CALLED BY:
**  shSaoCenter
**
** CALLS TO:
**  shSaoSendImMsg
**
**============================================================================
*/
RET_CODE saoCenterWrite
   (
   CMD_SAOHANDLE *a_shl    /* IN: Cmd SAO handle (internal structure ptr)*/
   )   
{
RET_CODE	    rstatus;
struct imtoolRec    im;		/* Command structure sent to SAOimage */

   /*
   ** TELL SAOIMAGE WE'RE GOING TO SEND AN SAO REGION INSTRUCTION
   */
   /* Setup command structure that is recognized by SAOimage.  SAOimage expects
      commands ala iraf's imtool. */
   im.tid      = 0;
   im.subunit  = CENTER;	/* Type of fsao command */
   im.z        = 0;		/* unused */
   im.checksum = 0;		/* will get filled in later */
   im.t	    = 123;		/* ??? Gary's kluge */
   im.x        = 0;
   im.y        = 0;                
   im.z        = 0;
   im.thingct  = 0;

   /* Send if off to SAOimage */
   rstatus = shSaoSendImMsg(&im, a_shl->wfd);
   return(rstatus);
}

/*
**============================================================================
**
** ROUTINE: saoPanWrite
**
** DESCRIPTION:
**  Routine to write SAO region panning information to SAOimage.
**
** RETURN VALUES:
**  SH_SUCCESS
**  status from shSaoSendImMsg
**
** CALLED BY:
**  shSaoCenter
**
** CALLS TO:
**  shSaoSendImMsg
**
**============================================================================
*/
RET_CODE saoPanWrite
   (
   CMD_SAOHANDLE *a_shl,   /* IN: Cmd SAO handle (internal structure ptr)*/
   int            a_x,     /* IN: x coordinate on image */
   int            a_y      /* IN: y coordinate on image */
   )   
{
RET_CODE	    rstatus;
struct imtoolRec    im;		/* Command structure sent to SAOimage */

   /*
   ** TELL SAOIMAGE WE'RE GOING TO SEND AN SAO REGION INSTRUCTION
   */
   /* Setup command structure that is recognized by SAOimage.  SAOimage expects
      commands ala iraf's imtool. */
   im.tid      = 0;
   im.subunit  = PAN;   	/* Type of fsao command */
   im.z        = 0;		/* unused */
   im.checksum = 0;		/* will get filled in later */
   im.t	    = 123;		/* ??? Gary's kluge */
   im.x        = a_x;
   im.y        = a_y;                
   im.z        = 0;
   im.thingct  = 0;

   /* Send if off to SAOimage */
   rstatus = shSaoSendImMsg(&im, a_shl->wfd);
   return(rstatus);
}

/*
**============================================================================
**
** ROUTINE: saoZoomWrite
**
** DESCRIPTION:
**  Routine to write SAO region zooming information to SAOimage.
**
** RETURN VALUES:
**  SH_SUCCESS
**  status from shSaoSendImMsg
**
** CALLED BY:
**  shSaoZoom
**
** CALLS TO:
**  shSaoSendImMsg
**
**============================================================================
*/
RET_CODE saoZoomWrite
   (
   CMD_SAOHANDLE *a_shl,    /* IN: Cmd SAO handle (internal structure ptr)*/
   double        zoomF      /* IN: Zoom factor */
   )   
{
RET_CODE	    rstatus;
struct imtoolRec    im;		/* Command structure sent to SAOimage */

   /*
   ** TELL SAOIMAGE WE'RE GOING TO SEND AN SAO REGION INSTRUCTION
   */
   /* Setup command structure that is recognized by SAOimage.  SAOimage expects
      commands ala iraf's imtool. */
   im.tid      = 0;
   im.subunit  = ZOOM;  	/* Type of fsao command */
   im.z        = 0;		/* unused */
   im.checksum = 0;		/* will get filled in later */
   im.t	    = 123;		/* ??? Gary's kluge */
   if (zoomF == .25)
     {
     im.x = ZOOM14;
     }
   else if (zoomF == .50) 
     {
     im.x = ZOOM12;
     }
   else if (zoomF == 1.0) 
     {
     im.x = ZOOM1;
     }
   else if (zoomF == 2.0) 
     {
     im.x = ZOOM2;
     }
   else if (zoomF == 4.0) 
     {
     im.x = ZOOM4;
     }
   else
     {
     /* must be a ,ultiple of 2 */
     im.x = zoomF;
     }

   im.y        = 0;                
   im.z        = 0;
   im.thingct  = 0;

   /* Send if off to SAOimage */
   rstatus = shSaoSendImMsg(&im, a_shl->wfd);
   return(rstatus);
}

/*<AUTO EXTRACT>
**============================================================================
**
** ROUTINE: shSaoCenter
**
** DESCRIPTION:
**<HTML>
**  The shSaoCenter routine enables the user to center the FSAOimage display.
**  This routine performs the same function as the SAOimage center button.
**</HTML>
**
** RETURN VALUES:
**   SH_SUCCESS           - Successful completion
**   SH_BAD_ZOOM          - An invalid zoom factor was specified.
**   SH_SAO_NUM_INV       - An invalid saoindex was specified.
**   SH_RANGE_ERR         - The specified saoindex value was too large.
**   SH_NO_FSAO           - No fSAO process associated with this process
**
**</AUTO>
**============================================================================
*/
RET_CODE shSaoCenter
  (
   int a_saoindex    /* IN: fSAO program to send the center command to */
  )   
{ /* shSaoCenter */

RET_CODE rstatus;

/* Check saoindex.  If not passed as option (= -1), use the first one that
   points to an fsao that belongs to us */
if ((rstatus = p_shCheckSaoindex(&a_saoindex)) == SH_SUCCESS)
   {
   /* Send the command to fsao */
   rstatus = saoCenterWrite(&(g_saoCmdHandle.saoHandle[a_saoindex]));
   if (rstatus != SH_SUCCESS)
      {
      shErrStackPush("Trouble when writing to FSAOimage.\n");
      rstatus = SH_PIPE_WRITEERR;
      }
   }

return(rstatus);
} /* shSaoCenter */

/*<AUTO EXTRACT>
**============================================================================
**
** ROUTINE: shSaoPan
**
** DESCRIPTION:
**<HTML>
**  The shSaoPan routine enables the user to pan the FSAOimage display to the
**  entered coordinates.  This routine performs the same function as panning
**  in the FSAOimage window.
**</HTML>
**
** RETURN VALUES:
**   SH_SUCCESS           - Successful completion
**   SH_SAO_NUM_INV       - An invalid saoindex was specified.
**   SH_RANGE_ERR         - The specified saoindex value was too large.
**   SH_NO_FSAO           - No fSAO process associated with this process
**
**</AUTO>
**============================================================================
*/
RET_CODE shSaoPan
  (
   int a_saoindex,   /* IN: fSAO program to send the pan command to */
   int a_x,          /* IN: x coordinate on image */
   int a_y           /* IN: y coordinate on image */
  )   
{ /* shSaoPan */

RET_CODE rstatus;

/* Check saoindex.  If not passed as option (= -1), use the first one that
   points to an fsao that belongs to us */
if ((rstatus = p_shCheckSaoindex(&a_saoindex)) == SH_SUCCESS)
   {
   /* Send the command to fsao */
   rstatus = saoPanWrite(&(g_saoCmdHandle.saoHandle[a_saoindex]), a_x, a_y);
   if (rstatus != SH_SUCCESS)
      {
      shErrStackPush("Trouble when writing to FSAOimage.\n");
      rstatus = SH_PIPE_WRITEERR;
      }
   }

return(rstatus);
} /* shSaoPan */

/*<AUTO EXTRACT>
**============================================================================
**
** ROUTINE: shSaoZoom
**
** DESCRIPTION:
**<HTML>
**  The shSaoZoom routine enables the user to zoom on the FSAOimage display.
**  This routine performs the same function as the SAOimage zoom buttons.
**</HTML>
**
** RETURN VALUES:
**   SH_SUCCESS           - Successful completion
**   SH_BAD_ZOOM          - An invalid zoom factor was specified.
**   SH_SAO_NUM_INV       - An invalid saoindex was specified.
**   SH_RANGE_ERR         - The specified saoindex value was too large.
**   SH_NO_FSAO           - No fSAO process associated with this process
**
**</AUTO>
**============================================================================
*/
RET_CODE shSaoZoom
  (
   int a_saoindex,    /* IN: fSAO program to send the zoom command to */
   double a_zoomF,    /* IN: zoom factor */
   int a_reset        /* IN: if true, do the equivalent of a zoom 1 */
  )   
{ /* shSaoZoom */

RET_CODE rstatus = SH_SUCCESS;
double zoom;

/* Check saoindex.  If not passed as option (= -1), use the first one that
   points to an fsao that belongs to us */
if ((rstatus = p_shCheckSaoindex(&a_saoindex)) == SH_SUCCESS)
   {
   zoom = a_zoomF;
   /* Check the zoom to make sure it is an allowed value */
   if ( (zoom != .25) &&
	(zoom != .50) &&
	(zoom != 1.0) &&
	(zoom != 2.0) &&
	(zoom != 4.0) &&
	(fmod(zoom, (double )2.0) != 0.0))     /*  Check if multiple of 2 */
     {
     shErrStackPush("Error: zoom factor must be .25, .5, 1.0, 2.0, or 4.0 \n\tor a multiple of 2");
     return(SH_BAD_ZOOM);
     }
   else if (zoom == 1.0)
     {
       /* This is basically a NOP, do not bomb out. */
     return(rstatus);
     }

   /* check for a reset to no zooming, fsao expects this to be a 1.0 */
   if (a_reset)
     {
     zoom = 1.0;
     }

   /* Send the command to fsao */
   rstatus = saoZoomWrite(&(g_saoCmdHandle.saoHandle[a_saoindex]), zoom);
   if (rstatus != SH_SUCCESS)
      {
      shErrStackPush("Trouble when writing to FSAOimage.\n");
      rstatus = SH_PIPE_WRITEERR;
      }
   }

return(rstatus);
} /* shSaoZoom */

