#ifndef SHCREGPRINT_H
#define SHCREGPRINT_H
/*****************************************************************************
 ******************************************************************************
 **
 ** FILE:
 **     shCRegPrint.h
 **
 ** ABSTRACT:
 **     Header file and constants for  functions which generate a postscript file
 **     from an in-memory description of data:
 **
 ** ENVIRONMENT:
 **     ANSI C.
 ******************************************************************************
 ******************************************************************************
 */
/* Reasionable defaults for our users */
/* Use these as arguments to the functions below if you wish*/
#define SH_DEF_IN_HIGH (11.0)	/* Use the size of the paper, not its printable area */
#define SH_DEF_IN_WIDE (8.5)
#define SH_DEF_DPI     (300)	/* 300 dpi is laserwriter II, and old-technology printers */
#define SH_DEF_GAMMA   (0.0)	/* More positive makes things brighter */

typedef struct pixlabel {
	int row, col, width, col2;
	char *text;
} PIXLABEL;

void shBitMapPrint(float inWide,  /* width of paper in inches */
		    float inHigh, /* height of paper in inches */
		    int dpi,	  /* assumed dots per inch */
		    int imRows,   /* number of rows in input image */
		    int imCols,	  /* number of columns in input image */
		    char *title,  /* title to print   */
		    FILE *file,   /* file to print to */
		    float (*pxl)(int, int), /* user-supplied function to supply a
					        floating -point number 0..1 representing
					        the non-gamma corrected intensity at
					        pixel (r,c)*/
		    float gamma,    /* Value of gamma to use */
		    int reverse,    /* if true reverse the image, else do not */
		    int dither,     /* if 1 dither the image, else do not */
		    int numPixLabs, /* the number of pixel labels to put on image */
		    PIXLABEL pxText[]);     /* the pixel labels */



void shRegPrint (REGION *reg,			/* pointer to region */
		 FILE *file,			/* descriptor to file */
		 float inWide, float inHigh, 	/* assume this size of paper */
		 int dpi, float gamma, 		/* assumed dpi of printer, gamma-correction*/
		 float *thresh, float *sat, 	/* shRegPrint will compute thresh,
					     		sat if thresh>=sat */
		 char *title,  			/* title to print   */
		 int reverse,    		/* if true reverse the image, else do not */
  		 int sqrtIn,     		/* if 1 use sqrt intensity map, else linear */
		 int dither,    		/* if 1 dither the image, else do not */
		 int numPixLabs,    		/* the number of pixel labels to put on image */
		 PIXLABEL pxText[]);   		/* the pixel labels */




#endif
