/*****************************************************************************
 ******************************************************************************
 **
 ** FILE:
 **     tclRegX.c
 **
 ** ABSTRACT:
 **     This file contains a tcl Extension capturing a bitmap from
 **     an Xwindow, and generating a region contining that bitmap/
 **
 ** ENVIRONMENT:
 **     DERVISH/ANSI C/Xwindows
 ******************************************************************************
 ******************************************************************************
 */

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "Xlib.h"
#include "Xutil.h"

#include "ftcl.h"
#include "tcl.h"
#include "tclExtend.h"
#include "dervish_msg_c.h"    /* Murmur message file for Dervish */
#include "dervish.h"

#include "Xresource.h"

#define MAXARGCHAR 100
static char theDisplayName[MAXARGCHAR+1];
static Display *dis;
static Screen  *scr;
static Window rws, root, child;
static XWindowAttributes attr;
static XImage *ximage;
#define NCMAP 256
static XColor cmap[NCMAP];

/*============================================================================  
**============================================================================
**
** ROUTINE: tclRegNewFromX
**
** DESCRIPTION:
**	Create a new region from an X window.
**
** RETURN VALUES:
**	TCL_OK	    Successful completion.  The Interp result string will
**		    contain the new region name.
**	TCL_ERROR   Error occurred.  The Interp result string will contain
**		    an error string.
**
**============================================================================
*/

static ftclArgvInfo argTable[10];

static int tclRegNewFromX
   (
   ClientData	clientdata,	/* IN : TCL parameter - not used */
   Tcl_Interp	*interp,	/* OUT: TCL Interpreter structure */
   int		argc,		/* IN : TCL argument count */
   char		**argv		/* IN : TCL argument */
   )   
{
   /* auto variables relatng to the parsing of our arguments*/
   int i =0;
   int status;
   int flags = FTCL_ARGV_NO_LEFTOVERS;

   /* auto variables relating to the creation of a region*/
   char	    tclRegionName[MAXTCLREGNAMELEN];
   REGION	    *reg;
   char * regTypeStr= "U8";
   PIXDATATYPE regType;
   char * regName   = "";

   /* auto variables relating to X-windows*/
   int scri;			/* Scratch Int for args we don't care about */
   unsigned int scru;		/* Scratch unsigned for args we don't care about*/
   unsigned int keys_buttons = 0;
   int nRowIn, nColIn;		/* Number on pixels in an input row or col */

   
   shErrStackClear();			    /* Clear error stack first */

   argTable[i].key  = NULL; argTable[i].type = FTCL_ARGV_HELP;
   argTable[i].src  = NULL; argTable[i].dst  = NULL;
   argTable[i].help = 
                 "Make an new region, with data populated from an X window\n";
   i++;

   argTable[i].key  = "-type"; argTable[i].type = FTCL_ARGV_STRING;
   argTable[i].src  = NULL;  argTable[i].dst  = &regTypeStr;
   argTable[i].help = "Type of region (U8(DEF) FL32, U8, etc.)";
   i++;

   argTable[i].key  = "-name"; argTable[i].type = FTCL_ARGV_STRING;
   argTable[i].src  = NULL;  argTable[i].dst  = &regName;
   argTable[i].help = "Name for region";
   i++;

   argTable[i].key  = (char *)NULL;  argTable[i].type = FTCL_ARGV_END;
   argTable[i].src  = NULL; argTable[i].dst  = NULL;
   argTable[i].help = (char *)NULL;

   status = ftcl_ParseArgv(interp, &argc, argv, argTable, flags);
   if (status != FTCL_ARGV_SUCCESS)  { 
       Tcl_AppendResult(interp,
             ftcl_GetUsage(interp, argTable, flags, "regNewFromX", 0, 
                           "\nUsage:", "\n"),
             ftcl_GetArgInfo(interp, argTable, flags, "chainNew", 8), 
             (char *)NULL);
       return TCL_ERROR;
   }

   /* Convert the region type specified by user to an enumeration */

   if (shTclRegTypeGetAsEnum(interp, regTypeStr, &regType) != TCL_OK) {
      Tcl_AppendResult (interp, " regNewFromX ");
      return (TCL_ERROR);
   }
   if ((dis=XOpenDisplay(theDisplayName)) == (Display*)NULL) {
      Tcl_SetResult (interp, "Cannot open display", TCL_STATIC);
      return (TCL_ERROR);
   }
   scr = ScreenOfDisplay (dis, 0);
   rws = RootWindowOfScreen(scr);
   root = rws;

/* +++++++++++++++++++GO and get the image and colormap +++++++++++++++++ */
   /*Set root to be the Window Id holding the image to get*/
   /*Go on to get the image from the window in ZPixmap form*/
   /*Ring Bell to clue user. Would like to change cursor or something...*/
   
   XBell(dis, 0); XFlush(dis);
   fprintf (stderr, 
	    "Move the cursor to the image, \n" 
            "then HOLD any mouse button until you hear a double bell\n");
   while (keys_buttons == 0) {
     sleep(1);
     XQueryPointer (dis, rws, &rws, &child, &scri, 
		    &scri, &scri, &scri, &keys_buttons);
   }
   while (XQueryPointer (dis, rws, &rws, &child, &scri, &scri, &scri, &scri, &scru)) {
      if (child == 0) break;
      root = child; rws=child;
   }
 
   XGetWindowAttributes (dis, root, &attr);
   nRowIn = attr.height;
   nColIn = attr.width;
   ximage = XGetImage (dis, root, 0, 0, attr.width, attr.height, 0xFFFFFFFF, ZPixmap);
   XBell(dis, 0); XBell(dis, 0); XFlush(dis);


   for (i=0; i<NCMAP; i++) {
      cmap[i].pixel=i;
      cmap[i].flags = DoRed | DoGreen | DoBlue;
   }
   XQueryColors (dis, attr.colormap, cmap, NCMAP);


/* ============= Got the image, now go get a region ================== */
   /* A handle */
   if (shTclRegNameGet(interp, tclRegionName) != TCL_OK)
      return (TCL_ERROR);

   /* A region */
   reg = shRegNew(regName, ximage->height, ximage->width, regType);

   /* bind handle to region */

   if (shTclRegNameSetWithAddr(interp, reg, tclRegionName) != TCL_OK) {
      shRegDel(reg);
      shTclRegNameDel(interp, tclRegionName);
      return (TCL_ERROR);
      }

/* Populate region from image ========================================= */

   {
      unsigned int px;
      double inten;
      int r,c;
      for (r=0; r<reg->nrow; r++)
	for (c=0; c<reg->ncol; c++) {
	   px = *(ximage->data + r*ximage->bytes_per_line + c);
	   assert (px < NCMAP);
	   /*Each intensity ranges from 0..65535*/
	   inten  =  ((float)(cmap[px].red) +
		      (float)(cmap[px].green) + (float)(cmap[px].blue)) /
			(3.0 * 65535);
	   assert (inten >= 0.0);
	   assert (inten <= 1.0);
	   inten *= 256.0;
	   shRegPixSetWithDbl (reg, r, c, inten);
	}
   }
   XDestroyImage (ximage);
   XCloseDisplay (dis);
   Tcl_SetResult (interp, tclRegionName, TCL_VOLATILE);
   return TCL_OK;
}

void
shTclRegXDeclare(Tcl_Interp *a_interp)
{
   char *pHelp,
        *pUse; 
   char *tclHelpFacil = "contrib";
   
   pHelp = "Red an X window into a region";
   pUse  = "regNewFromX [-type TYPE] [-name NAME]";
   shTclDeclare(a_interp, "regNewFromX", 
                (Tcl_CmdProc *)tclRegNewFromX,
                (ClientData)0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                pHelp, pUse);


}






