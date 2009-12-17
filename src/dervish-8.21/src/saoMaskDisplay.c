/* <AUTO>
 *
 * FILE:
 *   saoMaskDisplay.c
 * <HTML>
 *   This file contains the routines used to display a mask on FSAO through
 *   C.
 * </HTML>
 * </AUTO>
 *
 * ENTRY POINT             SCOPE    DESCRIPTION
 * ----------------------------------------------------------------------------
 * shSaoMaskDisplay        global   Display masks on the selected FSAO.
 * shSaoMaskColorSet       global   Set the colors associated w/ LUT values.
 * shSaoMaskGlyphSet       global   Set the glyph used for zoomed masked pixels.
 *
 * AUTHORS:
 *   Gary Sergey
 *   Eileen Berman
 *   Tom Nicinski
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "region.h"
#include "shCFitsIo.h"
#include "dervish_msg_c.h"
#include "shCSao.h"
#include "shCErrStack.h"

/*
 * Global variables
 */
char       g_colorTable[CHARSPERCOLOR*NUMCOLOR];  /* list of colors for mask */
int        g_colorTableComplete = FALSE;
/*
 * External variables referenced in this file
 */
extern SAOCMDHANDLE g_saoCmdHandle;


/* <AUTO EXTRACT>
 * ROUTINE:
 *   shSaoMaskDisplay
 * 
 * DESCRIPTION:
 * <HTML>
 * This routine will display a mask in the requested fSAO display.  The mask is
 * not altered in any way but is sent as is to fSAO. The lookup table is used by
 * fSAO to translate mask values to one of the colors as set by saoMaskColorSet.
 * In addition -
 * <DIR>
 * <LI>A copy of FSAOimage must be running.
 * <LI>The command <A HREF="tclSaoMaskDisplay.c.html#saoMaskColorSet">saoMaskColorSet</A>
 * must have been executed. (Or the equivalent C routine 
 * <A HREF="#shSaoMaskColorSet">shSaoMaskColorSet</A> called.
 * <LI>The specified mask must be the same size as the region
 * currently being displayed.
 * </DIR>
 * After calling this routine the masked region
 * is displayed immediately. Clicking on the right mouse button
 * (on a right handed mouse) will toggle between displaying the
 * masked region and the unmasked region.  FSAO must be in SCALE
 * mode for this to work. NOTE: this means the right mouse button
 * cannot be used for blinking.
 * </HTML>
 *
 * RETURN VALUES:
 *   SH_SUCCESS           - Successful completion
 *   SH_NO_COLOR_ENTRIES  - The Tcl command saoMaskColorSet was not executed or
 *                          the routine shSaoMaskColorSet was not called.
 *   SH_SIZE_MISMATCH     - The mask is not the same size as the displayed
 *                          region.
 *   SH_SAO_NUM_INV       - Invalid saoindex specified.
 *   SH_NO_FSAO           - No fSAO process associated with this process.
 *   SH_RANGE_ERR         - Saoindex value is too large.
 *   SH_PIPE_WRITE_MAX    - Could not write to fSAO process.
 *   SH_PIPE_WRITEERR     - Error writing to pipe.
 *
 * </AUTO> 
 */
RET_CODE shSaoMaskDisplay
   (
    char *a_maskData,	/* IN: Pointer to area of memory where the mask
                               data is stored. */
    MASK *a_maskPtr,	/* IN: Pointer to themask structure. */
    char  a_maskLUT[LUTTOP], /* IN: Mask lookup table. */
    int  a_saoindex	/* IN: The sao number of where to display mask. */
   )
{
RET_CODE    rstatus;


/* Check saoindex.  If not passed as option (= -1), use the first one that
   points to an fsao that belongs to us */
if ((rstatus = p_shCheckSaoindex(&a_saoindex)) == SH_SUCCESS)
   {
   /* Check to see if the colorTable has been filled out by a previous call
      to saoMaskColorSet. */
   if (!g_colorTableComplete)
      {
      /* No color table entries. */
      rstatus = SH_NO_COLOR_ENTRIES;
      shErrStackPush("No colors associated with this mask.\n Please set the mask colors first.");
      }
   else
      {
      /* Make sure the mask is the same size as the region being displayed in
         FSAO. */
      if ((g_saoCmdHandle.saoHandle[a_saoindex].rows == a_maskPtr->nrow) &&
          (g_saoCmdHandle.saoHandle[a_saoindex].cols == a_maskPtr->ncol))
         {
         /* Send mask to FSAOimage */
         rstatus = p_shSaoSendMaskMsg(&(g_saoCmdHandle.saoHandle[a_saoindex]),
				      a_maskLUT, a_maskData, a_maskPtr->nrow,
				      a_maskPtr->ncol, g_colorTable);
         }
      else
         {
         /* Size of mask must be equal to size of region and it is not. */
         rstatus = SH_SIZE_MISMATCH;
	 shErrStackPush("Size of mask (%d x %d) must equal size of displayed region (%d x %d).",
			a_maskPtr->nrow, a_maskPtr->ncol,
			g_saoCmdHandle.saoHandle[a_saoindex].rows,
			g_saoCmdHandle.saoHandle[a_saoindex].cols);
         }
      }
   }

/*---------------------------------------------------------------------------
**
** RETURN
*/
return(rstatus);

}

/* <AUTO EXTRACT>
 *
 * ROUTINE:
 *   shSaoMaskColorSet
 * 
 * DESCRIPTION:
 * <HTML>
 * This routine allows the user to specify the colors to be used when displaying
 * the mask in FSAO.  The color names should be those recognized by X (e.g. RED,
 * BLUE, GREEN...) and are not case sensitive.  It should be noted that X color 
 * names may differ from machine to machine.  An exact color match is not 
 * guaranteed.  An attempt will be made to get the closest available color to 
 * the requested color.  The specified colors will be used to display all masks
 * until this command is entered with new color values.  
 * <P>Eight colors (plus transparent) can be used to display a mask.  By 
 * default, the mask color on a monochrome display is white.
 * </HTML>
 *
 * RETURN VALUES:
 *   SH_SUCCESS            - Successful completion.
 *   SH_NAMES_TOO_LONG     - The total size of the string containing all the
 *                           specified color values is too long.
 * </AUTO>
 */
RET_CODE shSaoMaskColorSet
   (
    char **colorList	/* IN: argv style argument where each pointer points
                    	       to a string specifiying a specific color. */
   )
{
RET_CODE rstatus = SH_SUCCESS;
int      i;
int      emptyRoom;

/* Construct the character string of the color values which will be
   sent to FSAO with the mask display command. */
g_colorTable[0] = '\0';                         /* start at beginning */
emptyRoom = CHARSPERCOLOR * (NUMCOLOR - 1);     /* leave space at end of table
						   for one last color */

for (i = 0 ; i < NUMCOLOR ; ++i)
   {
   /* See if there is enough room left to add in another color */
   if (strlen(g_colorTable) < emptyRoom)
      {
      strcat(g_colorTable, colorList[i]);	/* add in color name */
      strcat(g_colorTable, COLORDELIMITER);	/* add in delimiter */
      }
   else
      {
      /* There is not enough room.  Let the user know that the color
	 names are too long. ps - we should never get here. */
      rstatus = SH_NAMES_TOO_LONG;
      shErrStackPush("Total length of colors too long.");
      break;
      }
   }

/* Only mark the color table as complete if we successfully got here */
if (rstatus == SH_SUCCESS)
   {g_colorTableComplete = TRUE;}

return(rstatus);
}

/* <AUTO EXTRACT>
 *
 * ROUTINE:
 *   shSaoMaskGlyphSet
 * 
 * DESCRIPTION:
 * <HTML>
 * This routine allows the user to specify the glyph to be used when displaying
 * the mask in FSAO.  The glyph is displayed when the image is magnified larger
 * than lifesize.  The possible glyphs are M_GLYPH_X or M_GLYPH_BOX.  This
 * routine can be called only after FSAO is started with
 * <A HREF="#shSaoMaskDisplay">shSaoMaskDisplay</A> <EM>and</EM> applies only
 * to that FSAOimage.  By default, FSAO uses the M_GLYPH_X glyph.
 * </HTML>
 *
 * RETURN VALUES:
 *   SH_SUCCESS            - Successful completion.
 *   SH_SAO_NUM_INV       - Invalid saoindex specified.
 *   SH_NO_FSAO           - No fSAO process associated with this process.
 *   SH_RANGE_ERR         - Saoindex value is too large.
 *   SH_PIPE_WRITE_MAX    - Could not write to fSAO process.
 *   SH_PIPE_WRITEERR     - Error writing to pipe.
 * </AUTO>
 */
RET_CODE shSaoMaskGlyphSet
   (
    int  a_glyph,	/* IN: the glyph to use. */
    int  a_saoindex	/* IN: The sao number of where to display mask. */
   )
{
RET_CODE rstatus;

/* Check saoindex.  If not passed as option (= -1), use the first one that
   points to an fsao that belongs to us */
if ((rstatus = p_shCheckSaoindex(&a_saoindex)) == SH_SUCCESS)
   {
   /* Send glyph request */
   rstatus = p_shSaoSendMaskGlyphMsg(&(g_saoCmdHandle.saoHandle[a_saoindex]), a_glyph);
   }
return(rstatus);
}

