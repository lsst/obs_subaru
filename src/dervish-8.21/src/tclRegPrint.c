/*<AUTO>
**
** FILE:
**	tclRegPrint.c
**<HTML>
** Contains TCL verb routine to print images as PostScript files.
** This routine operates on regions whose pixels are of any
** supported type.  It uses the
** <A HREF="regPrint.c.html">
** C primitives for printing images </A> built into dervish,
** which, in turn, use Floyd-Stienberg dithering.
**
** <P>For operations on arbitrary X window displays of images, see
** <A HREF="Xdither.c.html">xdither</A>.
**</HTML>
**</AUTO>
******************************************************************************
******************************************************************************
*/
#include "errno.h"
#include "string.h"
#include "stdio.h"
#include <assert.h>
#include <alloca.h>

#include "tclExtend.h"
#include "ftcl.h"
#include "dervish_msg_c.h"
#include "region.h"
#include "shC.h"
#include "shCUtils.h"
#include "shCHdr.h"
#include "shCFitsIo.h"
#include "shCErrStack.h"
#include "shCRegUtils.h"
#include "shCRegPrint.h"
#include "shCAssert.h"
#include "shCEnvscan.h"
#include "shTclRegSupport.h"
#include "shTclUtils.h"
#include "shTclVerbs.h"
#include "shTk.h"
#include "shTclHandle.h"
#include "shTclParseArgv.h"

#include "shCRegPrint.h"

/*============================================================================
**============================================================================
**
** ROUTINE: shTclRegPrint
**
**<AUTO EXTRACT>
** TCL VERB: regPrint
**
**<HTML>
** Render an image of the region to the named postscript file.
** By default, the image is printed reversed: (sky is white).
** The -gamma switch is used to lighten/darken (higher values/lower values)
** Its default value is 0.0 (apply no gamma correction).
**
** <P>
** By default (or if thresh is equal to or larger than sat), the
** image is sampled and saturation and threshold values are calculated.
** The saturation and threshold values are always returned as a list 
** (e.g., {thresh 500.0000} {sat 1000.0000}) even if explicitly set.
** 
** <P>
** The sqrt option applies a sqrt to the image intensity when turning the pixels
** into postscript. It is a useful option for quick look printing.
** When using the sqrt option, a value for gamma of 3.2 is useful.
**
** <P>
** Labels may be applied anywhere on the image by passing a list to the routine.
** The format is {row col text}. The text will appear at that row and col.
** If the text has whitespace between words,
** then the syntax is: {row col {text}}.
** If the text is "circle","box", or "triangle" then the syntax is:
** {row col text width},
** where width is the  (diameter, width, base length) of the
** (circle, box, triangle) in pixels.
** The glyph will be drawn centered upon the target row and col.
** If the text is "rect" then the syntax is:
** {row col drow dcol} where drow and dcol are the row and column widths.
** If the text is "line" then the syntax is:
** {row col text row2 col2} and the line will be drawn between the points.
** In detail, if the first characters
** of the text are these keywords, then the glyphs will be drawn.
**</HTML>
**
** The postscript file is an attempt at Encapsulated PostScript,
** but it acts somewhat strangely when used in this form in TeX file.
** To use the file in a TeX file, the following sequence will be useful:
**
** \input epsf                         % get encapsulated postscript macros
** % To match the centering of the original regPrint file, recall that
** % TeX has 1in default margins, and use:
** \hoffset=-0.5in
** \voffset=-0.5in
** \epsfile[40 264 572 752]{coma.ps}   % put *.ps file into document
** % you will have to get the exact numbers from the ps file itself,
** % at the like similar to %%BoundingBox 40 264 572 752
** \vskip 0.5cm                 % put a decent space between image and text
**
**</AUTO>
*/
static char *g_regPrint = "regPrint";
static ftclArgvInfo regPrintArgTable[] = {
       {NULL, FTCL_ARGV_HELP, NULL, NULL, 
        "Print a region to a postscript file\n" },
       {"<region>",   FTCL_ARGV_STRING, NULL, NULL,
	"Handle to region" },
       {"<file>",   FTCL_ARGV_STRING, NULL, NULL,
	"File to put Postcript into" },
       {"-title",   FTCL_ARGV_STRING, NULL, NULL,
	"Title to print (def: name of region)" },
       {"-width",   FTCL_ARGV_DOUBLE, NULL, NULL,
	"Width of paper in inches (def: 8.5)" },
       {"-height",   FTCL_ARGV_DOUBLE, NULL, NULL,
	"Height of paper in inches (def: 11.0)" },
       {"-gamma",   FTCL_ARGV_DOUBLE, NULL, NULL,
	"Lighten or darken the image (def: 0.0)" },
       {"-dpi",   FTCL_ARGV_INT, NULL, NULL,
	"Target resolution of printer (def: 300)" },
       {"-thresh",   FTCL_ARGV_DOUBLE, NULL, NULL,
	"Threshhold (def: compute threshold)" },
       {"-sat",   FTCL_ARGV_DOUBLE, NULL, NULL,
	"Saturation (def: computer saturation)" },
       {"-noreverse",   FTCL_ARGV_CONSTANT, (void*) 0, NULL,
	"If present do not reverse the image" },
       {"-sqrt",   FTCL_ARGV_CONSTANT, (void*) 1, NULL,
	"Apply sqrt intensity mapping (def: linear mapping)" },
       {"-nodither",   FTCL_ARGV_CONSTANT, (void*) 0, NULL,
	"If present do not apply FS dithering to image" },
       {"-pixlabels",   FTCL_ARGV_STRING, NULL, NULL,
	"If present use the list of lists provided to label "
	"image (def: no labels)" },
       {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
    };
 
static int shTclRegPrint(ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{

  REGION *reg;
  double width=8.5, height=11.0, gamma=0.0;
  double sat =0., thresh = 1.;    
  float fsat, fthresh;
  int dpi=300;
  int reverse = 1, sqrtIn = 0, dither = 1;
  int l_argc, i, j, numPixLabs;
  int status;
  int textLen;
  char satthresh[50];
  char **l_argv;
  char **textPtr = NULL;
  char *text = NULL;
  PIXLABEL *pxText = NULL;

  int flags=FTCL_ARGV_NO_LEFTOVERS;

  char *tclRegionNamePtr= NULL;
  char *pixLabelsPtr = "";

  char *fileNamePtr=NULL;
  FILE *file;
  char *titlePtr = NULL;

  regPrintArgTable[1].dst = &tclRegionNamePtr;
  regPrintArgTable[2].dst = &fileNamePtr;
  regPrintArgTable[3].dst = &titlePtr;
  regPrintArgTable[4].dst = &width;
  regPrintArgTable[5].dst = &height;
  regPrintArgTable[6].dst = &gamma;
  regPrintArgTable[7].dst = &dpi;
  regPrintArgTable[8].dst = &thresh;
  regPrintArgTable[9].dst = &sat;
  regPrintArgTable[10].dst = &reverse;
  regPrintArgTable[11].dst = &sqrtIn;
  regPrintArgTable[12].dst = &dither;
  regPrintArgTable[13].dst = &pixLabelsPtr;
  
  if ((status = shTclParseArgv(interp, &argc, argv, regPrintArgTable, 
       flags, g_regPrint)) != FTCL_ARGV_SUCCESS) {
    return(status);
  } else { 

    if (shTclRegAddrGetFromName(interp, tclRegionNamePtr, &reg) != TCL_OK) return(TCL_ERROR);

    if ((reg->nrow == 0) || (reg->ncol == 0)) {
       Tcl_AppendResult(interp, "empty region ", tclRegionNamePtr, (char *)0);
       return(TCL_ERROR);
    }

    if ((file = fopen (fileNamePtr, "w")) == (FILE *)0) {
       Tcl_AppendResult(interp, "error opening ", fileNamePtr, strerror(errno), (char *)0);
       return(TCL_ERROR);
    }
    if (titlePtr==NULL) {
      if ((titlePtr = alloca(50)) == NULL) {
	shFatal("Error allocaing 50 bytes for title.");
      }
      sprintf(titlePtr,"Region Named: %s",reg->name);
    }

    textLen = strlen(pixLabelsPtr);
    if (textLen == 0) {
      textLen = 50;   /* KLludge this so Tcl_SplitList does not bomb */
    }
    if ((text = alloca(textLen)) == NULL) {
       shFatal("Error allocaing %d bytes for text labels.", textLen);
    }
    strcpy(text,pixLabelsPtr);
  }
/*
 * extract pixel labeling strings
 */

  if (Tcl_SplitList(interp, text, &l_argc, &l_argv) == TCL_ERROR) {
    Tcl_SetResult(interp,"Error parsing list of pixel labels",TCL_VOLATILE);
    goto errorCleanup;
  }
/* An empty list means no labeling */
  if ( !strncmp(text,"{}",2) || !strncmp(text,"",1) ) {		
	numPixLabs = 0;
  } else {
/* Check for a leading "{"; if true than it is a multiple entry list */
        if ( !strncmp(text,"{",1) ) {		
	   numPixLabs = l_argc;
           if ((pxText = (PIXLABEL *)alloca(numPixLabs*sizeof(PIXLABEL)))
	       == NULL) {
	     shFatal("Error allocaing %d bytes for individual text label structures.", (numPixLabs*sizeof(PIXLABEL)));
	   }
	   for (i=0; i < l_argc; i++) {
		j = 1;
  		Tcl_SplitList(interp, l_argv[i], &j, &textPtr);
		if (j<3) {
    			Tcl_SetResult(interp,"Too few label parameters for text",TCL_VOLATILE);
			goto errorCleanup;
		}
		pxText[i].row = (int)strtod(textPtr[0],textPtr);
		pxText[i].col = (int)strtod(textPtr[1],textPtr);
		pxText[i].text = textPtr[2];
		if(!strncmp(textPtr[2],"circle",6) || 
		   !strncmp(textPtr[2],"box",3) || !strncmp(textPtr[2],"triangle",8)) {
			if (j<4) {
    				Tcl_SetResult(interp,"Too few label parameters for a glyph",TCL_VOLATILE);
				goto errorCleanup;
			}
			pxText[i].width = (int)strtod(textPtr[3],textPtr);
		}
		if(!strncmp(textPtr[2],"line",4) || !strncmp(textPtr[2],"rect",4)) {
			if (j<5) {
    				Tcl_SetResult(interp,"Too few label parameters for a line",TCL_VOLATILE);
				goto errorCleanup;
			}
			pxText[i].width = (int)strtod(textPtr[3],textPtr);
			pxText[i].col2  = (int)strtod(textPtr[4],textPtr);
		}
	   }
/* If there was not a leading "{", then it is a single label list */
	} else {	
	   if (l_argc<3) {
    		Tcl_SetResult(interp,"Too few label parameters",TCL_VOLATILE);
		goto errorCleanup;
	   }
	   numPixLabs = 1; 	
           if ((pxText = (PIXLABEL *)alloca(numPixLabs*sizeof(PIXLABEL)))
	       == NULL) {
	     shFatal("Error allocaing %d bytes for individual text label structures.", (numPixLabs*sizeof(PIXLABEL)));
	   }
	   pxText[0].row = (int)strtod(l_argv[0],textPtr);
	   pxText[0].col = (int)strtod(l_argv[1],textPtr);
	   pxText[0].text = l_argv[2];
	   if(!strncmp(l_argv[2],"circle",6) || 
	      !strncmp(l_argv[2],"box",3) || !strncmp(l_argv[2],"triangle",8)) {
		if (l_argc<4) {
    			Tcl_SetResult(interp,"Too few label parameters",TCL_VOLATILE);
			goto errorCleanup;
		}
		pxText[0].width = (int)strtod(l_argv[3],textPtr);
	   }
 	   if(!strncmp(l_argv[2],"line",4) || !strncmp(l_argv[2],"rect",4)) {
		if (l_argc<5) {
    			Tcl_SetResult(interp,"Too few label parameters",TCL_VOLATILE);
			goto errorCleanup;
		}
		pxText[0].width = (int)strtod(l_argv[3],l_argv);
		pxText[0].col2  = (int)strtod(l_argv[4],l_argv);
	   }
	}
  }

  /* Convert sat and thresh to float - shRegPrint should be converted to
     use double's some day */
  fsat = sat;         
  fthresh = thresh;
  shRegPrint (reg, file, width, height, dpi, gamma, &fthresh, &fsat, titlePtr, 
              reverse, sqrtIn, dither, numPixLabs, pxText);

  sprintf(satthresh, "thresh %.5f", fthresh);
  Tcl_AppendElement(interp, satthresh);
  sprintf(satthresh, "sat %.5f", fsat);
  Tcl_AppendElement(interp, satthresh);
  
  fclose(file);

  return (TCL_OK);

errorCleanup:
  fclose(file);
  return (TCL_ERROR);
}
/***************** Declaration of TCL verbs in this module *****************/

static char *shtclRegPrintFacil = "shRegion";

void shTclRegPrintDeclare (Tcl_Interp *interp) 
{
	char *shtclRegPrint_hlp = shTclGetArgInfo(interp, regPrintArgTable,
	              FTCL_ARGV_NO_LEFTOVERS, g_regPrint);
	char *shtclRegPrint_use = shTclGetUsage(interp, regPrintArgTable, 
	              FTCL_ARGV_NO_LEFTOVERS, g_regPrint);
	shTclDeclare (interp, g_regPrint,
                      (Tcl_CmdProc *)shTclRegPrint,
	              (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
                      shtclRegPrintFacil,
                      shtclRegPrint_hlp,
                      shtclRegPrint_use
                      );

}
