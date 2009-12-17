/*****************************************************************************
 ******************************************************************************
 **<AUTO>
 **
 ** FILE: regPrint.c
 **<HTML>
 ** This file contains printing routines used to create PostScript files 
 ** depicting images.
 ** Floyd-Steinberg dithering is used. The code in DERVISH is derived from
 ** Steve Kent's CCD program.
 **<P>
 ** Useful images can be often be obtained even in the case where one dot 
 ** on the page corresponds to one pixel in the image.
 **<P>
 ** While the user supplies the resolution of their printer in dots per inch,
 ** the resulting PostScript file may be printed on any postscript
 ** printer, though the quality of the image is sometimes degraded.
 ** In fact, these routines generate encapsulated postscript. One may attempt
 ** to include the output in LaTeX, for example.
 **<P>
 ** Headers for these functions are available in
 ** <A HREF="../../include/shCRegPrint.h>shCRegPrint.h"</A>
 **
 **</HTML>
 **</AUTO>
 **
 ** ENVIRONMENT:
 **     ANSI C.
 ******************************************************************************
 ******************************************************************************
 */

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>
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
#include "shCAssert.h"
#include "shCEnvscan.h"
#include "shTclRegSupport.h"
#include "shCSao.h"
#include "shTclVerbs.h"
#include "shTk.h"
#include "shTclHandle.h"
#include "shCGarbage.h"
#include "shCRegPrint.h"

static void psClose(char* psLine);
static void psOpen (
	   float inHigh, float inWide, int dpi, 	/* Paper size and resolution of printer */
	   int dotsWide, int dotsHigh, 		    	/* Size of image in dots */
	   float scale,					/* Scale of image, multiplicative */
   	   char **psLine,	 		  	/* a line of the image in hex format */
	   char *title, FILE *file, int dither);        /* Misc inputs */
static void psLabel ( float inHigh, float inWide, int dpi, int dotsWide, int dotsHigh, 
	   int reverse, float scale, char *title, FILE *file, int dither, /* same as above */
	   int numPixLabs, PIXLABEL pxText[]);		/* labeling inputs */
static void psRowDone(char *psLine, int col);
static void psNextBit(int bit, char *psLine, int *col);
static void psBit(float bit, char *psLine, int col);
static void psPrintEscapedString (char * string);
static void fsInit (int ncol);
static int fslkup (float px, int col);
static void fsNewRow (void);
static void fsDone(void);


/**************************** shBitMapPrint***********************************/
/*****************************************************************************/


/*============================================================================
**============================================================================
**<AUTO EXTRACT>
** ROUTINE: shBitMapPrint
**
**<HTML>
**<P>
** Print a PostScript image to the file described by the file-descriptor FILE.
** File descriptors are obtained by calling the ANSI C function fopen(). The
** caller must close the file.
**<P>
** The caller supplies PXL(R,C), a function which returns a pixel in the image
** at row R and column C.
**<P>
** GAMMA is a parameter which controls the overall brightness of the image.
** A reasonable value when using a sqrt map is between 2 and 3.
**</HTML>
**</AUTO>
*/
void shBitMapPrint(float inWide, /* width of paper in inches */
		    float inHigh,  /* height of paper in inches */
		    int dpi,	   /* assumed dots per inch */
		    int imRows,    /* number of rows in input image */
		    int imCols,	   /* number of columns in input image */
		    char *title,   /* title to print   */
		    FILE *file,    /* open file to print to*/
		    float (*pxl)(int, int), /* user-supplied function to supply a
					         floating -point number 0..1 representing
					         the non-gamma corrected intensity at
					         pixel (r,c)*/
		    float gamma,    /* Value of gamma to use */
		    int reverse,    /* if true reverse the image, else do not */
		    int dither,     /* if true dither the image, else do not */
                    int numPixLabs, /* the number of pixel labels to put on image */
                    PIXLABEL pxText[]   /* the pixel labels */
	)

{
   int pxFactor;		/* Number of Bits in an output-row/input-pixel */
   int rowFactor, colFactor;	/* First guess at number of dots/row/pixel, etc. */
   int rIn, cIn, rRep, cRep;	/* Iterators: input row, col, repeat counts */
   char *psLine;	   	/* a line of the image in hex format */
   int ditherCounter=0;		/* a kludge, but it counts how many hax strings are written */
   double px;
   double lcl_gamma = gamma;
   float *rowInten = (float *)alloca(sizeof(float)*imCols);
   imRows = imRows/8*8; /*on some printers, #rows must be multiple of 8 */
   assert (rowInten != (float *)0);


/* select largest scaling factor compatible with the aspect ratio of the 
 * paper and the image.
 * we will dither one pixel into pxFactor*pxFactor dots
*/
/*  Nickolai Kouropatkine 09/04/2001 Just put protection on zero divide
    in case of zero put inCols or imRows to 1. I am not sure that it has any
    reason to continue in suct case, but only make a worning message now.   */

    if (imCols != 0 ) {
   rowFactor = ((int)inWide*dpi)/imCols;
   } else {
           printf(" Warning imCols = 0 was set to 1 \n");
	   imCols = 1;
           rowFactor = ((int)inWide*dpi); }
    if (imRows != 0 ) {	   
   colFactor = ((int)inHigh*dpi)/imRows;
    } else {
           printf(" Warning imRows = 0 was set to 1 \n");
	   imRows = 1;
	   colFactor = ((int)inHigh*dpi); }  
   pxFactor  = (rowFactor < colFactor ? rowFactor : colFactor );
   if (pxFactor > 250) pxFactor=250;	/* Don't go ape on really small things */

/* Generate the output file. pxFactor*pxFactor dots per pixel in the input image
 * The loop is structured so that each input row is looked-up only once, and
 * that each individual pixel is reversed and gamma-corrected only once.
*/

   psOpen(inHigh, inWide, dpi, imCols, imRows, pxFactor, &psLine, title, file, dither);
   fsInit (imCols*pxFactor);

   if (dither) {
      for (rIn=0; rIn<imRows; rIn++) {
        for (cIn=0; cIn<imCols; cIn++) { 
          /*Call the user-supplied function to get a row of pixels from*/
          /*the input image, reverse and gamma-correct an input row only once! */
          px = pxl(rIn, cIn);	        /* get intensity from user suppled function*/
          if (reverse) px =  1.0 - px;
          px = ((1+lcl_gamma)*px)/(1+lcl_gamma*px);   /* this is kent's correction*/
                              
          assert (px>=0.0); assert (px<=1.0);
          rowInten[cIn] = px; 
        }
        for (rRep = 0; rRep<pxFactor; rRep++) {
 	    for (cIn=0; cIn<imCols; cIn++) {
               px = rowInten[cIn];
	       for (cRep=0; cRep<pxFactor; cRep++){
	          psNextBit(fslkup(px, cIn*pxFactor+cRep), psLine, &ditherCounter);  
	       }
	     }
	    fsNewRow();
	    psRowDone(psLine, ditherCounter);
	    ditherCounter=0;
         }/* for each row in the output */
      }
   } else {		/* no dithering; use raw pixel to hex formater */
      for (rIn=0; rIn<imRows; rIn++) {
        for (cIn=0; cIn<imCols; cIn++) { 
	   px = pxl(rIn, cIn);	        /* get intensity from user suppled function*/
	   if (reverse) px =  1.0 - px; 
           px = ((1+lcl_gamma)*px)/(1+lcl_gamma*px);   /* this is kent's correction*/

	   assert (px>=0.0); assert (px<=1.0); 
	   psBit(px, psLine, cIn); 
        }
        psRowDone(psLine, cIn);
      }
   }

   if (numPixLabs > 0) {
	psLabel(inHigh, inWide, dpi, imCols, imRows, reverse, pxFactor, 
		title, file, dither, numPixLabs, pxText);
   }
   psClose(psLine);
   fsDone();

   return;
}

/******************************************************************************/
/*****fs routines for Floyd-Steinberg Dithering *******************************/
/******************************************************************************/
/* Floyd-Steinberg dithering:*/

static float *rowErr;
static float *prevErr;
static int saveNcol;

/******************************************************************************/
/* Init floyd-steinberg for ncolOut bits in a row on the output device*/
/* The algoritn wants to reference  rowErr[-1], and rowErr[nColOut] so malloc two extra*/
static void fsInit (int ncolOut)
{
   int ncp2 = ncolOut + 2;
   int i;
   saveNcol = ncolOut;
   rowErr =  (float *)shMalloc ((ncp2)*sizeof(float)); assert (rowErr  != (float *)0);
   prevErr = (float *)shMalloc ((ncp2)*sizeof(float)); assert (prevErr != (float *)0);
   for (i=0; i<ncp2; i++) {rowErr[i] = prevErr[i] = 0.5;}
   rowErr++; prevErr++;   /*make rowErr[-1] &cet a valid reference*/
}

/******************************************************************************/
/*Done dithering. free allocated space */
static void fsDone(void)
{
   shFree(--rowErr );  /* point back to begining of malloced space */   
   shFree(--prevErr);  
}

/******************************************************************************/
/*Generate one bit for the output device
 *Input pixel is in the range 0..1.
*/
static int fslkup (float px, int col)
{
   float t;
   assert (col < saveNcol); assert (col >= 0);
   t = px +
          (7.*rowErr [col-1]   +
                    1.*prevErr[col-1] +
		    5.*prevErr[col]   +
		    3.*prevErr[col+1])/16.0;

   if (t<=0.5) {
      rowErr[col] = t;
      return 0;		       
   }else{
      t -= 1.0;
      rowErr[col] = t;
      assert (t >= -0.5); assert (t <= 0.5);    
      return 1;		       
   }
}

/******************************************************************************/
/* Done with the current row, shift pointer to begin new row*/
static void fsNewRow (void)
{
   float *t;
   t = rowErr;
   rowErr = prevErr;
   prevErr = t;
}

/*****************************************************************************/
/*****************POSTSCRIPT GENERTING OUTPUT ROUTINES*************************/
/*****************************************************************************/
/* Postscript OUTPUT routines*/

/* Static data which lives across procedure invocations*/
static int bib;			/* bit in byte */
static int bir;			/* bit in row */
static int nbir;		/* number of bits put into this row */
static int px8;			/* four adjacent pixels in for an output row */
static FILE *psFile;		/* file descriptor */


/*****************************************************************************/
/* write the Initial Postscript
 * 
 * Write out header the top of the header. Include the following:
 * - Proper comments for ENcapsulated postscript. Though the file is 
 *   generated for a specific size and printer resolution, the program
 *   is formally cotrect and will work on any printer.
 *
 * - Procedures which we may need
 *
 * - Title and box about the image
 *
 * - The image itself. You should no fool with this code without reading the
 *   Postscript reference material. Basically the input data is placed as hex
 *   following th ecommand. One subtlty which is dealt with is that readhexstring. 
 *   the procedure which image calls for input, reads an _Even_ number of hex
 *   characters, hence the suporting code is carful to add pad bytes, should
 *   8 not divide dotsWide. 
 * 
 * 
*/
static void psOpen (
	   float inHigh, float inWide, int dpi, 	/* Paper size and resolution of printer */
	   int imgCols, int imgRows,  		    	/* Size of image in dots */
	   float scale,					/* Scale of image, multiplicative */
   	   char **psLine,	      			/* a line of the image in hex format */
	   char *title, FILE *file, int dither) 	/* Misc inputs */
{
   int dotsWide, dotsHigh;    /* Size of image in dots; reverse image? */
   int wip, hip;              /* image width, height in points */
   int textip = 12;   	      /* size of text in points */
   int spaceip  = 3;	      /* spacing between lines in points */
   char *font   = "Courier-BoldOblique";   
   int imllxip;               /* image lower left x position in points */
   int imurxip;  	      /* image upper right x position in points */
   int imuryip;               /* image upper right position in points -- assume 5% border */
   int imllyip;	              /* image lower left y position in points*/
   int bitsPerPix;	      /* final image bits per pixel. this is 1 if dithering */
   int digitsPerLine;	      /* how many hex digits per line */

   if (dither) {
	dotsWide = (int)(imgCols * scale);		/* convert image cols to dots per inch */
	dotsHigh = (int)(imgRows * scale);
	dotsWide = ((dotsWide+7)/8)*8; 		/* Round width to multiple of 8 to */ 
						/* accomodate the Poscript procedure readhexstring */
   	wip = (dotsWide*72/dpi);		
   	hip = (dotsHigh*72/dpi);		
   } else {
	dotsWide = imgCols;			/* convert image cols to dots per inch */
	dotsHigh = imgRows;
   	wip = (int)((dotsWide*72/dpi)*scale);		/* image width  in points, scaled to size */
   	hip = (int)((dotsHigh*72/dpi)*scale);		/* image height in points, scaled to size */
   }

/* computer a bunch of related things before they become inconsistent*/
   imllxip = (int)(inWide*72-wip)/2;  
   imurxip = imllxip + wip;	      
   imuryip = (int)(0.95*inHigh*72); 
   imllyip = imuryip - hip;	    


/* initialze persistent state */
   nbir = dotsWide;		
   bib = 8;
   bir = 0;
   px8 = 0x00;
   if (dither) { bitsPerPix = 1; }
   else        { bitsPerPix = 8; }

   psFile = file;

/* write Stuctured comments for eps -- I have not tested that this works as eps */
   fprintf (psFile, "%s", "%!PS-Adobe-1.0\n");
   fprintf (psFile, "%s %d %d %d %d\n","%%BoundingBox", imllxip, imllyip, imurxip, imuryip);
   fprintf (psFile, "%s %s \n", "%%Title", title);
   fprintf (psFile, "%s", "%%Pages 1\n");
   fprintf (psFile, "%s %s\n", "%%DocumentFonts", font); 
   fprintf (psFile, "%s", "%%Creator: dervish\n");
   fprintf (psFile, "%s", "%%EndComments\n");

/* Some comments to help us debug complaints*/
   fprintf (psFile, "%s inHigh, inWide, dpi, dotsHigh, dotsWide:  %f %f %d %d %d \n \n", 
                    "%",inHigh, inWide, dpi, dotsHigh, dotsWide);

/* Define our procedures*/

/* Procedure cshow, print a string at the current Y, but centered */
   fprintf (psFile,       "/halfwidth %f 72 mul 2 div def\n", inWide);
   fprintf (psFile, "%s", "/cshow \n");
   fprintf (psFile, "%s", "  { dup \n");
   fprintf (psFile, "%s", "    stringwidth \n");
   fprintf (psFile, "%s", "    pop                        %Get rid of the width in y.\n");
   fprintf (psFile, "%s", "    2 div                      %Get 1/2 width in x.\n");
   fprintf (psFile, "%s", "    halfwidth exch sub         %Get x position for beginning\n");
   fprintf (psFile, "%s", "    currentpoint exch pop      %Have new x, y on stack\n");
   fprintf (psFile, "%s", "    moveto\n");
   fprintf (psFile, "%s", "    show\n");
   fprintf (psFile, "%s", "  } def\n");
   fprintf (psFile, "%s", "\n");
   fprintf (psFile, "%s", "\n");
 
   fprintf (psFile, "%s", "%%EndProlog\n");
   fprintf (psFile, "%s", "%%Page 0 1\n");

/*print the title*/

   fprintf (psFile,       "  gsave\n");
   /* set it against the right border and below image */
   fprintf (psFile,       "  %d %d  moveto\n", (int)(.05*inWide*72), imllyip-spaceip-textip); 
   fprintf (psFile,       "  /%s findfont %d scalefont setfont\n", font, textip);
   /* Certain things in a postscript string must be escaped i.e. put "dog(" into file as (dog\() */
   psPrintEscapedString (title); 
   fprintf (psFile,       "  cshow\n");
   fprintf (psFile,       "  grestore\n");

/*draw a box about the image */
   fprintf (psFile,       "  gsave\n");
   fprintf (psFile, "%d %d moveto\n", imllxip-1, imllyip-1);
   fprintf (psFile, "%d %d lineto\n", imllxip-1, imuryip+1);
   fprintf (psFile, "%d %d lineto\n", imurxip+1, imuryip+1);
   fprintf (psFile, "%d %d lineto\n", imurxip+1, imllyip-1);
   fprintf (psFile, "%d %d lineto\n", imllxip-1, imllyip-1);
   fprintf (psFile, "stroke\n");
   
   fprintf (psFile, "  grestore\n");

/* dump the image*/
   fprintf (psFile,       "  gsave\n");
   if (dither) {
	digitsPerLine = (nbir+1)/4;
   } else {
	digitsPerLine = nbir;
   }
   fprintf (psFile,       "  /picstr %d string def\n", digitsPerLine); 
   *psLine = (char*)shMalloc( (2*dotsWide+1)*sizeof(char));

   fprintf (psFile, "%s", "  \n");
   fprintf (psFile,       "  %d %d  translate\n", imllxip, imllyip);
   fprintf (psFile,       "  %d %d  scale\n", wip, hip);
   fprintf (psFile, "%s", "  \n");
   fprintf (psFile,       "  %d %d %d\n", dotsWide, dotsHigh, bitsPerPix);
   fprintf (psFile,       "  [%d 0 0 -%d 0 %d]\n", dotsWide, dotsHigh, dotsHigh); 
   fprintf (psFile, "%s", "  {currentfile\n");
   fprintf (psFile, "%s", "     picstr readhexstring pop}\n");
   fprintf (psFile, "%s", "  image\n");

 
}

/*****************************************************************************/
/* Write out pixel labels to file */

static void psLabel (
	   float inHigh, float inWide, int dpi, 	/* Paper size and resolution of printer */
	   int imgCols, int imgRows, int reverse,       /* Size of image; reverse image? */
	   float scale,					/* Scale of image, multiplicative */
	   char *title, FILE *file, int dither, 	/* Misc inputs */
	   int numPixLabs, PIXLABEL pxText[])		/* labelling inputs */
{
   int dotsWide, dotsHigh;    /* Size of image in dots; reverse image? */
   int wip, hip;              /* image width, height in points */
   int imllxip;               /* image lower left x position in points */
   int imurxip;  	      /* image upper right x position in points */
   int imuryip;               /* image upper right position in points -- assume 5% border */
   int imllyip;	              /* image lower left y position in points*/
   float r, c, width, col2;   /* r,c and sometimes width and col2 of label to place on image */
   char *text;		      /* text to place on image */
   int textip = 10;   	      /* size of text in points */
   char *font   = "Times-Roman";   
   int i;

   if (dither) {
	dotsWide = (int)(imgCols * scale);		/* convert image cols to dots per inch */
	dotsHigh = (int)(imgRows * scale);
	dotsWide = ((dotsWide+7)/8)*8; 		/* Round width to multiple of 8 to */ 
						/* accomodate the Poscript procedure readhexstring */
   	wip = (dotsWide*72/dpi);		
   	hip = (dotsHigh*72/dpi);		
   } else {
	dotsWide = imgCols;			/* convert image cols to dots per inch */
	dotsHigh = imgRows;
   	wip =(int) ((dotsWide*72/dpi)*scale);		/* image width  in points, scaled to size */
   	hip = (int)((dotsHigh*72/dpi)*scale);		/* image height in points, scaled to size */
   }

/* computer a bunch of related things before they become inconsistent*/
   imllxip = (int)(inWide*72-wip)/2;  
   imurxip = imllxip + wip;	      
   imuryip = (int)(0.95*inHigh*72); 
   imllyip = imuryip - hip;	    

/*put pixel labels on file */
   fprintf (psFile, "  grestore\n");  		/* there is no restore after the image command */
   fprintf (psFile,       "  gsave\n");
   fprintf (psFile,       "  /%s findfont %d scalefont setfont\n", font, textip);
   if (reverse == 0) {reverse = 1;} 		/* 1=reverse=black, 0=noreverse=white */
   	        else {reverse = 0;}
   fprintf (psFile,       "  %d setgray\n", reverse);   
   for (i=0; i < numPixLabs; i++) {
	   r = pxText[i].row;	
	   c = pxText[i].col;	
	   text = pxText[i].text;	
	   r = imuryip - (r/imgRows)*(imuryip - imllyip);
	   c = imllxip + (c/imgCols)*(imurxip - imllxip); 
						/* Ansi C's lovely backwards syntax: */ 
						/* !strncmp means the pair are the same */
	   if (!strncmp(text,"circle",6)  || !strncmp(text,"box",3)||
	       !strncmp(text,"triangle",8)|| !strncmp(text,"line",4) || !strncmp(text,"rect",4))
	     {
		width = pxText[i].width;	
		width = (width/imgRows)*(imuryip - imllyip);
		if (!strncmp(text,"circle",6)) {
		   width = width/2;	
   		   fprintf (psFile,       "  %5.1f %5.1f %5.1f 0 360 arc stroke\n", c, r, width);
		}
		if (!strncmp(text,"box",3)) {
		   r = r - (width/2);
		   c = c - (width/2);
   		   fprintf (psFile,       "  %5.1f %5.1f %5.1f %5.1f rectstroke\n", c, r, width, width);
		}
		if (!strncmp(text,"rect",4)) {
		   /* using col2 to store column width */
		   col2 = pxText[i].col2;
		   col2 = (col2/imgCols)*(imurxip - imllxip);
		   r = r - (width/2);
		   c = c - (col2/2); 
   		   fprintf (psFile,       "  %5.1f %5.1f %5.1f %5.1f rectstroke\n", c, r, col2, width);
		}
		if (!strncmp(text,"triangle",8)) {
   		   fprintf (psFile,       "  newpath\n");
   		   fprintf (psFile,       "  %5.1f %5.1f  moveto\n", c-(width/2),r-(width/2)); 
   		   fprintf (psFile,       "  %5.1f %5.1f  lineto\n", c,r+(width/2)); 
   		   fprintf (psFile,       "  %5.1f %5.1f  lineto\n", c+(width/2),r-(width/2)); 
   		   fprintf (psFile,       "  closepath\n");
   		   fprintf (psFile,       "  stroke\n");
		}
		if (!strncmp(text,"line",4)) {
		   /* here width is row2 */
		   width = imuryip - width;
		   col2 = pxText[i].col2;	
		   col2 = imllxip + (col2/imgCols)*(imurxip - imllxip);
   		   fprintf (psFile,       "  newpath\n");
   		   fprintf (psFile,       "  %5.1f %5.1f  moveto\n", c,r); 
   		   fprintf (psFile,       "  %5.1f %5.1f  lineto\n", col2,width); 
   		   fprintf (psFile,       "  stroke\n");
		}
	   } else {
   		fprintf (psFile,       "  %5.1f %5.1f  moveto\n", c,r); 
   		psPrintEscapedString (text); 
   		fprintf (psFile,       "  show\n");
   		fprintf (psFile,       "  newpath\n");
	   }
    }
    fprintf (psFile, "  grestore\n");

}

/*****************************************************************************/
/* Finish printing and close the file*/
static void psClose (char *psLine)
{
/* clean up graphics context from image */

   fprintf (psFile, "  grestore\n");

   fprintf (psFile, "%s\n","  showpage");
   fprintf (psFile, "%s", "%%Trailer\n");
/* free memory */
   shFree(psLine); 
}

/*****************************************************************************/
static void psRowDone (char *psLine, int col)
{
   nbir = 0;
   if (bib != 8) {
     /* flush what's left for the output*/
     /* Set remaining bite to 1 */
      while (bib) px8 |= 1 << --bib;
      sprintf ((psLine+(2*col)), "%02x",px8);
   }
   px8 = 0x00;
   bib = 8;
   fprintf (psFile, "%s\n", psLine);
}

/*****************************************************************************/
/* for fs dithering */
static void psNextBit (int bit, char *psLine, int *col)
{
   assert (bit==0 || bit==1);   
   px8 |= bit << --bib;
   if (bib == 0) {
      sprintf ((psLine+((*col)*2)), "%02x",px8);
      px8 = 0x00;
      bib = 8;
      *col= *col+1;
   }
}

/*****************************************************************************/
/* raw  pixel to hex formater */
static void psBit(float bit, char *psLine, int col)
{
   int ibit;
   assert (bit>=0 && bit<=1);   
   ibit = (int)(bit * 255.0);
   sprintf ((psLine+(2*col)), "%02x",ibit);
}

/*****************************************************************************/
/* print a PostScript string -- Escape ( ) and \ */
static void psPrintEscapedString (char * string) 
{
   char c;
   fprintf (psFile, "(");
   while ((c = *string++) != (char)0) {
     switch(c) {
       case '(':
       case ')':
       case '\\': fprintf (psFile,"%c",'\\');       
	 break;
       default: ;
     }
     fprintf (psFile, "%c", c);
   }
   fprintf (psFile, ")");
}

/********************************************************************/
/****************** shRegPrint **************************************/
/********************************************************************/


/* forwards declarations for this part of the file*/
static int cmpDbl (const void *d1, const void *d2);
static float intensity (int r, int c);
static float sqrtIntensity (int r, int c);


static REGION *g_region;	   /* Pointer to region  */
static double g_thresh;		   /* threshold */
static double g_sat;		   /* saturation */
#define SPERC 300		   /* sample per row/ col */
#define NSAMPLES (SPERC*SPERC)  /* total number of sample */


/*============================================================================
**============================================================================
**<AUTO EXTRACT>
** ROUTINE: shRegPrint
**
**<HTML>
** Render an image of the region to the named postscript file.
** By default, the image is printed reversed: (sky is white).
**<P>
** By default, or if THRESH is equal to or larger than SAT, shRegPrint samples
** the image and computes saturation and threshhold values.
** Note that the calculated values will be returned, as the SAT and
** THRESH parameters are passed by reference.
**<P>

** If using a sqrt map, a value for gamma of 3.2 is useful.
**<P>
** Labels may be placed on the image by filling an array of PIXLABEL's and 
** passing it and the number of labels to the routine. There are limitations 
** on the labels: the format is either: r, c, text, where r,c is the row 
** and column where the label is to be placed, or if text=="circle", "box", 
** or "triangle" then r, c, text, width where width is the (diameter, width, 
** base length) of the (circle, box, triangle) in pixels. If the text=="line", 
** a line is drawn between r,c and width,col2. If the text=="rect", the format
** is r c text dr dc, where dr and dc are the width in rows and columns respectivly.
**</HTML>
**</AUTO>
*/
    void shRegPrint
    (
    REGION *reg,       /* IN: pointer to region to print */
    FILE *file,	       /* IN: pointer to FILE structure for PS output */
    float inWide,      /* IN: Prepare image for paper this wide */
    float inHigh,      /* IN: "        "     "    "    "   high */
    int dpi,	       /* IN: Expected printer resolution */
    float gamma,       /* IN: High values lighten the image */
    float *threshptr,  /* MOD: Pixels smaller then this are treated as thresh */
    float *satptr,     /* MOD: Pixels larger than this are treated as sat */
    char *title,       /* IN: Print this title on the page */
    int reverse,       /* IN: Reverse black and white if not 0 */
    int sqrtIn,        /* IN: use sqrt intensity map if 1; else linear */
    int dither,        /* IN: Dither image if 1; else do not */
    int numPixLabs,    /* the number of pixel labels to put on image */
    PIXLABEL pixText[]  /* the pixel labels */
    )
{
   int r,c;

   g_region = reg;

   /* if user set thresh >= sat, then this is a flag that we are to compute it*/
   if (*threshptr >= *satptr) {
      float rdelta=(float) reg->nrow/SPERC;
      float cdelta=(float) reg->ncol/SPERC;
      double *samples =  (double *)malloc(NSAMPLES*sizeof(double));
      double *sampl = samples;
      assert (samples != (double *)0);
      shAssert((reg->nrow * reg->ncol) > 0);   /* prevent printing an empty region */
      /* Sample the image, set thresh and sat to the kent-tiles. */
      sampl = samples;
      for (r=0; r<SPERC; r++) {
         for (c=0; c<SPERC; c++) *sampl++ = shRegPixGetAsDbl(reg, (int)(r*rdelta), (int)(c*cdelta));
      }
      qsort (samples, NSAMPLES, sizeof(double), cmpDbl);
      g_thresh = samples[(int)(NSAMPLES*0.060)]; /* Ignore the pathologically low */
      g_sat    = samples[(int)(NSAMPLES*0.986)]; /* Ignore the pathologically high */
      if (g_sat <= g_thresh+256) g_sat = g_thresh+256; /* spread of at least 256 */
      *satptr=g_sat;          /* Return calculated saturation */
      *threshptr=g_thresh;    /* Return calculated threshold */
    }else{
      g_sat = *satptr; g_thresh = *threshptr;
    }


   if (sqrtIn) { 
   	shBitMapPrint (inWide, inHigh, dpi,
		  g_region->nrow, g_region->ncol,
		  title, file,                /* comment, file descriptor */
		  sqrtIntensity, gamma,       /* pixel fetching funct, gamma  */
		  reverse, dither,  	      /* reverse and dither pixel?  */
		  numPixLabs, pixText);     /* the pixel labels: number and entities */
   } else {
   	shBitMapPrint (inWide, inHigh, dpi,
		  g_region->nrow, g_region->ncol,
		  title, file,                /* comment, file descriptor */
		  intensity, gamma,           /* pixel fetching funct, gamma  */
		  reverse, dither, 	      /* reverse and dither pixel?  */
		  numPixLabs, pixText);     /* the pixel labels: number and entities */
   }

   return;
}


/********************************************************************/
/* qsort helper function. Final list is ordered from smallest to largest*/
static int cmpDbl (const void *d1, const void *d2)
{
    double *f1 = (double *)d1;
    double *f2 = (double *)d2;
    if (*f1 > *f2) return ( 1);
    if (*f1 < *f2) return (-1);
    return ( 0);
}

/********************************************************************/
/* return a number in the range 0..1 corresponding to the intensitiy
 * of the pixel at row=r, col=c
 */
static float intensity (int r, int c)
{
   float inten;
   inten = (shRegPixGetAsDbl(g_region, r, c) - g_thresh)/(g_sat - g_thresh);
   if (inten < 0.0) return 0.0;
   if (inten > 1.0) return 1.0;
   return inten;
}

/********************************************************************/
/* return a number in the range 0..1 corresponding to the sqrt of the intensitiy
 * of the pixel at row=r, col=c.
 */
static float sqrtIntensity (int r, int c)
{
   float inten;
   inten = (shRegPixGetAsDbl(g_region, r, c) - g_thresh)/(g_sat - g_thresh);
   if (inten < 0.0) return 0.0;
   if (inten > 1.0) return 1.0;
   return sqrt(inten);
}
