/*****************************************************************************
 ******************************************************************************
 **
 ** FILE:
 **     Xdither.c
 **
 **<HTML>
 **<H1><A NAME="xdither">xdither</A></H1>
 **     Xdither is a program which reads a bitmap from
 **     an Xwindow, and genterates a postscript file. The image is created
 **     using Floyd-Steinburg dithering.
 **<p>
 ** The -gamma switch is used to lighten (higher values) or darken (lower 
 ** values) the image the default is 0.0.
 ** 
 **<p> This command is a shell command, and is available from TCL in the
 ** same fashion that other UNIX commands are.
 **<PRE>
 ** COMMAND SYNTAX:
 **    xdither  [file] [-title] [-width] [-height] [-noreverse]
 **                              [-gamma] [-dpi] [-thresh] [-sat]
 **    [file]       Optional: File to put Postcript into (def: stdout)
 **    [-title]     Optional: Title to print (def: gamma used.)
 **    [-width]     Optional: width of paper in inches (def: 8.5)
 **    [-height]    Optional: Height of paper in inches (def: 11.0)
 **    [-reverse]   Optional: If present do reverse the image
 **    [-gamma]     Optional: Lighten or darken the image (def: 0.0)
 **    [-dpi]       Optional: Target resolution of printer (def: 300)
 ** 
 **</PRE>
 **</HTML>
 **
 ** ENVIRONMENT:
 **     ANSI C.
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

#include "ftcl.h"
#include "tcl.h"
#include "tclExtend.h"
#include "dervish_msg_c.h"    /* Murmur message file for Dervish */
#include "region.h"         /* For definition of PIXDATATYPE */
#include "shCSchema.h"         /* For prototype of shSchemaLoadFromDervish */
#include "shTclRegSupport.h"
#include "shTclHandle.h"
#include "shCSao.h"
#include "shTclVerbs.h"
#include "shCArray.h"

#include "Xresource.h"
#include "shCRegPrint.h"

#define MAXARGCHAR 100
static char theDisplayName[MAXARGCHAR+1];
#define MAXTITLE 132
static char theTitle[MAXTITLE+1];

static Display *dis;
static Screen  *scr;
static Window rws, root, child;
static XWindowAttributes attr;
static XImage *ximage;
#define NCMAP 256
static XColor cmap[NCMAP];
static float myGamma;
static int dpi;
static int reverse = 0;
static FILE *file;

/* forward declarations */

static float intensity (int r, int col);
static void parseArgs (int argc, char **argv);
static void usage (void);
 

/*********************************************************************************/

int main (int argc, char *argv[])
{
   int scri;			/* Scratch Int for args we don't care about */
   unsigned int scru;		/* Scratch unsigned for args we don't care about*/
   unsigned int keys_buttons = 0;
   int i;
   int nRowIn, nColIn;		/* Number on pixels in an input row or col */
   int pndw=dpi*7.5;            /* Width of the paper in dots */
   int pndh=dpi*10;		/* Heigh of the paper in dots */
   int dither = 1;              /* Dither the image. */
   int numPixLabs = 0;          /* Number of pixel labels on the image. */
   PIXLABEL *pxText = NULL;     /* The pixel labels. */
    
   parseArgs (argc, argv); 
   pndw=dpi*8.5;            /* assume paper width is 8.5 inches */
   pndh=dpi*11;		    /* assume paper height is 11 inches */

   if ((dis=XOpenDisplay(theDisplayName)) == (Display*)NULL) {
      fprintf (stderr,"Cannot open display \n"); exit(1);
   }
   scr = ScreenOfDisplay (dis, 0);
   rws = RootWindowOfScreen(scr);
   root = rws;

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

   shBitMapPrint (SH_DEF_IN_WIDE, SH_DEF_IN_HIGH, dpi,
		  ximage->height, ximage->width,
		  theTitle, file, 
		  intensity, myGamma, reverse,
		  dither, numPixLabs, pxText);


   return 0;
}


/*****************************************************************************/
/* X window specific helper routines*/

/* use the global colormap array, cmap, to get the total intensity of a pixel
 * expressed as an float ranging from 0 to 1
*/
static float intensity (int r, int c)
{
   float inten;
   unsigned int px;
   px = *(ximage->data + r*ximage->bytes_per_line + c);
   assert (px < NCMAP);
   /*Each intensity ranges from 0..65535*/
   inten  =  ((float)(cmap[px].red) +
              (float)(cmap[px].green) + (float)(cmap[px].blue)) /
				(3.0 * 65535);
   assert (inten >= 0.0);
   assert (inten <= 1.0);
   return inten;

}

/*****************************************************************************/
/* Command line switch error: print error messge and exit*/
static void usage (void) {
    fprintf (stderr, 
	      "xdither [-display DISPLAY] [-dpi 300] [-gamma 2.7] [-title title] \n"
	      "[-reverse] [filename | -]\n");
     exit (1);

}

/*****************************************************************************/
/* Use horrible X window Parsing mechanism to get options and parameters*/

static void parseArgs (int argc, char **argv) {

   static XrmDatabase db;
   static int nopts = 5;
   static XrmOptionDescRec optTable[] = {
     {"-dpi",       ".dpi",       XrmoptionSepArg,     (caddr_t) "300" },
     {"-display",   ".display",   XrmoptionSepArg,     (caddr_t) 0     },
     {"-title",     ".title",     XrmoptionSepArg,     (caddr_t) 0     },
     {"-gamma",     ".gamma",     XrmoptionSepArg,     (caddr_t) "2.7" },
     {"-reverse",   ".reverse",   XrmoptionNoArg,      (caddr_t) "1"   }
   };
   char *str_type[20];		/* Not sure why this is dimensioned to 20 */
   XrmValue value;
   char *tail;

   XrmParseCommand (&db, optTable, nopts, argv[0], &argc, argv);


   /* output file is the only parameter if absent use stdout */
   if (argc  > 2) { usage(); }
   file = stdout;
   if (argc == 2) {
      if ((file = fopen (argv[1], "w"))== (FILE *)0) {
         fprintf (stderr,"error opening %s: %s \n", argv[1], strerror(errno));
         exit(1);
      }
   }

   if (XrmGetResource (db, 
	   "xdither.display", "Xdither.Display", str_type, &value) == True) {
     assert (((int)value.size <= MAXARGCHAR));
     (void) strncpy (theDisplayName, value.addr, (int) value.size);
   }else{
      ;
   }


   if (XrmGetResource (db, 
	   "xdither.dpi", "Xdither.Dpi", str_type, &value) == True) {
     assert (((int)value.size <= MAXARGCHAR));
     dpi = strtol (value.addr, &tail ,0);
     if (*tail != (char)0) usage();
   }else{
     dpi = 300;
   }

   if (XrmGetResource (db, 
	   "xdither.gamma", "Xdither.Gamma", str_type, &value) == True) {
     assert (((int)value.size <= MAXARGCHAR));
     myGamma = strtod (value.addr, &tail);
     if (*tail != (char)0) usage();
   }else{
     myGamma = 2.7;
   }

   if (XrmGetResource (db, 
	   "xdither.title", "Xdither.Title", str_type, &value) == True) {
     assert (((int)value.size <= MAXTITLE));
     (void) strncpy (theTitle, value.addr, (int) value.size);
   }else{
      sprintf (theTitle, "xdither: assumed dpi = %d, gamma = %f ", dpi, myGamma);
   }

   if (XrmGetResource (db, 
	   "xdither.reverse", "Xdither.Reverse", str_type, &value) == True) {
     reverse = 1;
   }else{
     reverse = 0;
   }

}
