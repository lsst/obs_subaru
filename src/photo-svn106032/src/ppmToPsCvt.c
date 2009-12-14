/*
 * Convert a PPM file to postscript
 *
 * Why did I write this? Because the standard ppmtops has no facility
 * to specify the output size, or that CMYK is desired.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h> 
#include <assert.h>
#include <unistd.h>

#define LSIZE 100			/* lenght of input line */

/*****************************************************************************/
/*
 * memory stuff
 */
static void *
shMalloc(long n)
{
   void *ptr = malloc(n);
   if(ptr == NULL) {
      fprintf(stderr,"Failed to allocate 0x%x bytes", n);
      abort();
   }

   return(ptr);
}

static void *
shRealloc(void *ptr, long n)
{
   ptr = realloc(ptr, n);
   
   if(ptr == NULL) {
      fprintf(stderr,"Failed to reallocate 0x%x bytes", n);
      abort();
   }

   return(ptr);
}

static void
shFree(void *ptr)
{
   free(ptr);
}

static void
usage(FILE *fd)
{
   char **hptr;
   char *help[] = {
      "Usage: ppmToPsCvt [options] ppmfile [psfile]",
      "If psfile is omitted (or \"-\"), write to stdout",
      "Options:",
      "   -h, -?   Print this message",
      "   -b       Explicitly handle black",
      "   -l       Make a landscape plot",
      "   -r       Write image with \"raw\" chars (i.e. nonprinting)",
      "   -s #     Make largest dimension of postscript output # inches",
      "   -v       Be chatty",
      "",
      "The -b flag is useful with e.g. some apple printers",
      "The -r flag makes the output files about 2* smaller",
      NULL
   };

   for(hptr = help; *hptr != NULL; hptr++) {
      fprintf(fd, "%s\n", *hptr);
   }
}

/*****************************************************************************/
/*
 * Strings we need to write postscript [color]images
 */
static char header[] = "\
%%!PS-Adobe-2.0 EPSF-2.0\n\
%%%%Creator: Photo\n\
%%%%BoundingBox: %d %d %d %d\n\
%%%%Pages: 1 1\n\
%%%%DocumentFonts:\n\
%%%%EndComments\n";

static char preamble[] = "\
15 dict begin\n\
/buff %d string def\n\
%%\n\
%d %d translate\n\
%d dup scale\n\
gsave\n\
%%%%Page: 1 1\n";

static char set_landscape[] = "\
%d %d translate 90 rotate\n";

static char black_correction[] = "\
{ } setblackgeneration    %% map min(CMY) to black\n\
{ } setundercolorremoval  %% correct for removing min(CMY)\n";

static char draw_it[] = "\
%d %d 8 [ %d 0 0 %d 0 0 ]\n\
{ currentfile buff read%sstring pop }\n\
false %d\n\
colorimage\n";

static char print_it[]="\n\
grestore\nshowpage\n\
end\n\
%%%%Trailer\n\
%%%%EOF\n";

/*****************************************************************************/

int
main(int ac, char *av[])
{
   int binary = 0;			/* write image as binary file */
   int correct_black = 0;		/* specify a setblackgeneration proc */
   int csize, rsize;			/* col- and row-size of PPM file */
   int i,j,k;
   int landscape = 0;			/* produce a landscape plot? */
   char line[LSIZE], *lptr;		/* input file */
   int maxval;				/* maximum pixel value in file */
   const int ncolor = 3;		/* number of colours; 3 for RGB */
   char *ppmfile = NULL;		/* PPM file to read */
   FILE *ppm;				/* file descriptor for PPM file */
   char *psfile = NULL;			/* postscript file to write */
   FILE *ps;				/* file descriptor for postscript */
   unsigned char *row;			/* buffer to read a row */
   float size = 8.0;			/* size of output, in inches */
   int size_pt;				/* size of image, in 1/72" */
   int verbose = 0;			/* be chatty? */
   int xoff_pt, yoff_pt;		/* origin of bounding box in 1/72" */
   float xoff = 0.5, yoff = 0.5;	/* origin of Bounding Box in inches */
   int xsize_pt, ysize_pt;		/* size of Bounding box, in 1/72" */
   
   while(ac >= 2 && av[1][0] == '-' && av[1][1] != '\0') {
      switch (av[1][1]) {
       case 'h': case '?':
	 usage(stderr);
	 exit(0);
       case 'b':
	 correct_black = 1;
	 break;
       case 'l':
	 landscape = 1;
	 break;
       case 'r':
	 binary = 1;
	 break;
       case 's':
	 if(ac <= 2) {
	    fprintf(stderr,"Please provide an argument with %s\n", av[1]);
	    continue;
	 }
	 {
	    float tmp;
	    if((tmp = atof(av[2])) == 0.0) {
	       fprintf(stderr,"%.3f is not a valid size\n", tmp);
	       continue;
	    }
	    size = tmp;
	 }
	 
	 ac--; av++;
	 
	 break;
       case 'v':
	 verbose = 1;
	 break;
       default:
	 fprintf(stderr,"Unknown flag %s\n", av[1]);
	 usage(stderr);
	 exit(1);
      }
      ac--; av++;
   }
   
   if(ac > 1) {
      ppmfile = av[1];
      ac--; av++;

      if(strcmp(ppmfile,"-") == 0) {	/* use stdout */
	 ppmfile = NULL;
      }
   }
   if(ac > 1) {
      psfile = av[1];
      ac--; av++;

      if(strcmp(psfile,"-") == 0) {	/* use stdout */
	 psfile = NULL;
      }
   }
/*
 * open PPM file
 */
   if(ppmfile == NULL) {
      ppm = stdin;
      ppmfile = "-";
   } else {
      if((ppm = fopen(ppmfile,"r")) == NULL) {
	 fprintf(stderr,"Can't open %s\n", ppmfile);
	 exit(1);
      }
   }
   if(fgets(line, LSIZE, ppm) == NULL) {
      fprintf(stderr,"Failed to read header for PPM file %s\n", ppmfile);
      exit(1);
   }
   {
      char magic[LSIZE];
      if(sscanf(line, "%s %d %d %d", magic, &csize, &rsize, &maxval) != 4) {
	 fprintf(stderr,"Invalid header for PPM file %s\n:%s",
		 ppmfile, line);
	 exit(1);
      }
      assert(maxval <= 255);		/* required by binary PPM */
   }
/*
 * write postscript header
 */
   if(psfile == NULL) {
      ps = stdout;
      psfile = "-";
   } else {
      if((ps = fopen(psfile,"w")) == NULL) {
	 fprintf(stderr,"Can't open %s\n",psfile);
	 exit(1);
      }
   }

   xoff_pt = 72*xoff;			/* convert to PS's 1/72" points */
   yoff_pt = 72*yoff;
   size_pt = 72*size;
   xsize_pt = size_pt*(rsize > csize ? csize/(float)rsize : 1);
   ysize_pt = size_pt*(rsize > csize ? 1 : rsize/(float)csize);

   if(landscape) {
      fprintf(ps, header, xoff_pt, yoff_pt,
	      xoff_pt + ysize_pt, yoff_pt + xsize_pt);
      fprintf(ps, set_landscape, 2*yoff_pt + ysize_pt, 0);
   } else {
      fprintf(ps, header, xoff_pt, yoff_pt,
	      xoff_pt + xsize_pt, yoff_pt + ysize_pt);
   }
   fprintf(ps, preamble, 3*csize, xoff_pt, yoff_pt, size_pt);
   if(correct_black) {
      fprintf(ps, black_correction);
   }
   fprintf(ps, draw_it, csize, rsize, (rsize > csize ? rsize : csize),
	   (rsize > csize ? rsize : csize), (binary ? "" : "hex"), ncolor);
/*
 * Read PPM file and convert to PS
 */
   row = shMalloc(3*csize);

   for(i = 0; i < rsize; i++) {
      if(fread(row, csize, 3, ppm) != 3) {
	 fprintf(stderr,"Error reading from %s", ppmfile);
	 perror(" ");
	 fclose(ppm);
	 break;
      }

      if(binary) {
	 if(fwrite(row, csize, 3, ps) != 3) {
	    fprintf(stderr,"Error writing to %s", psfile);
	    perror(" ");
	    fclose(ppm);
	    break;
	 }
      } else {
	 for(j = k = 0; j < 3*csize; j += 3, k++) {
	    fprintf(ps, "%02x%02x%02x",row[j], row[j + 1], row[j + 2]);
	    fflush(ps);
	    if(k == 12) {
	       fputc('\n', ps);
	       k = -1;			/* it'll be ++ed in a moment */
	    }
	 }
      }
      if(k > 0) {
	 fputc('\n', ps);
      }
   }

   fprintf(ps,print_it);

   if(ppm != stdin) {
      fclose(ppm);
   }

   if(ps != stdout) {
      fclose(ps);
   }

   return(0);
}
