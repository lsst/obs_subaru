/*
 * Assemble a set of PPM files into a mosaic
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h> 
#include <assert.h>
#include <unistd.h>

#define LSIZE 300			/* lenght of input line */

typedef struct {
   char file[100];			/* name of file */
   int corig, rorig;			/* requested origin in mosaic */
   int rsize, csize;			/* size of file */
   int nrow, ncol;			/* number of rows and columns desired*/
   int row0, col0;			/* origin of desired part */
} PPMFILE;

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
      "Usage: assemblePPM [options] descripfile [outfile]",
      "If outfile is omitted (or \"-\"), write to stdout",
      "Options:",
      "   -h, -?   Print this message",
      "   -v       Be chatty",
      "",
      "The format of the descripfile is:",
      "  filename start-row start-col [nrow ncol [row0 col0]]",
      "Where <start-{row,col}> specify the origin in the output file where",
      "<filename> should be placed, <n{row,col}> are the size of the desired",
      "part of the input file, and <{row,col}0> are the origin of this part.",
      "",
      "If corig/rorig is -1, it's taken to be ncol/nrow greater than the",
      "previous value",
      "",
      "Three special `<filename>'s are understood:",
      "  dimen <nrow> <ncol>",
      "gives the dimension of the desired part of the input files; all",
      "dimensions except <row0> and <col0> will be interpreted as multiples",
      "of this dimension, and",
      "  mosaic <rorig> <corig> <nrow> <ncol>",
      "which gives the total dimension of the output mosaic.",
      "  offsets <row0> <col0>",
      "which gives the offset of the desired images within the input images.",
      "",
      "e.g.",
      "mosaic			0 0  3 1",
      "dimen			1361 2000",
      "offsets			64 24",
      "file-0.ppm		 0 0",
      "file-1.ppm		-1 0",
      "file-2.ppm		-1 0",
      "",
      "The photo command make_assemblePPM can write this input file for you",
      "",
      "",
      "N.b. you may find the command",
      "    convert mosaic.ppm mosaic.tiff",
      "from the imageMagick package useful.",
      NULL
   };

   for(hptr = help; *hptr != NULL; hptr++) {
      fprintf(fd, "%s\n", *hptr);
   }
}

int
main(int ac, char *av[])
{
   int csize = -1, rsize = -1;		/* size of mosaic */
   FILE *descrip;			/* file describing desired mosaic */
   char *descripfile;			/* file containing description */
   FILE *fd;				/* file descriptor for PPM file */
   PPMFILE **files = NULL;		/* input files */
   int files_size = 0;			/* allocated size of files array */
   int i,j;
   char line[LSIZE], *lptr;		/* input file */
   int lineno;				/* line number in descrip file */
   char magic[LSIZE];			/* magic number */
   int maxval;				/* maximum pixel value in file */
   int nf;				/* number of fields read from line */
   int nfile;				/* number of input files */
   int nrow = 1, ncol = 1;		/* dimension of each input PPM file */
   int ocorig = -1, ororig = -1;	/* old value of [cr]orig */
   char *outfile = NULL;		/* desired output file */
   char **output;			/* output data */
   int rorig0 = 0, corig0 = 0;		/* origin of output PPM file */
   int row0 = 0, col0 = 0;		/* origin within each input file */
   int verbose = 0;			/* be chatty? */

   while(ac >= 2 && av[1][0] == '-' && av[1][1] != '\0') {
      switch (av[1][1]) {
       case 'h': case '?':
	 usage(stderr);
	 exit(0);
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
   
   if(ac < 2) {
      usage(stderr);
      exit(1);
   }
   if(ac >= 2) {
      descripfile = av[1];
      ac--; av++;
   } 
   if(ac >= 2) {
      outfile = av[1];
      ac--; av++;

      if(strcmp(outfile,"-") == 0) {	/* use stdout */
	 outfile = NULL;
      }
   }

   if((descrip = fopen(descripfile,"r")) == NULL) {
      fprintf(stderr,"Can't open %s\n",descripfile);
      return(1);
   }
/*
 * read input file
 */
   nfile = 0;
   for(lineno = 1; (lptr = fgets(line, LSIZE, descrip)) != NULL; lineno++) {
      while(isspace(*lptr)) lptr++;
      if(*lptr == '#' || *lptr == '\0') { /* comment or blank */
	 continue;
      }

      if(files == NULL) {
	 files_size = 20;
	 files = shMalloc(files_size*sizeof(PPMFILE *));
      } else if(nfile == files_size) {
	 files_size += 20;
	 files = shRealloc(files, files_size*sizeof(PPMFILE *));
      }

      files[nfile] = shMalloc(sizeof(PPMFILE));

      files[nfile]->nrow = files[nfile]->ncol = -1;
      files[nfile]->row0 = row0; files[nfile]->col0 = col0;
      
      nf = sscanf(lptr, "%s %d %d %d %d %d %d",
		  files[nfile]->file,
		  &files[nfile]->rorig, &files[nfile]->corig,
		  &files[nfile]->nrow, &files[nfile]->ncol,
		  &files[nfile]->row0, &files[nfile]->col0);

      if(nf < 3) {
	 fprintf(stderr,"Please specify a file and origin, %s, line %d\n",
		 descripfile, lineno);
	 exit(1);
      }
/*
 * file "dimen" is the size of a single input file; if provided all
 * dimensions except offsets in input file will be interpreted in units
 * of nrow/ncol
 */
      if(strcmp(files[nfile]->file, "dimen") == 0) {
	 if(files[nfile]->nrow >= 0) {
	    fprintf(stderr,
		    "Please specify only two numbers with \"dimen\"\n");
	 }

	 nrow = files[nfile]->rorig;
	 ncol = files[nfile]->corig;

	 continue;
      }
/*
 * file "mosaic" is the entire file
 */
      if(strcmp(files[nfile]->file, "mosaic") == 0) {
	 if((corig0 = files[nfile]->corig) < 0) {
	    fprintf(stderr,
		    "You mustn't specify a negative value for corig0: %d\n",
								       corig0);
	 }
	 if((rorig0 = files[nfile]->rorig) < 0) {
	    fprintf(stderr,
		    "You mustn't specify a negative value for rorig0: %d\n",
								       rorig0);
	 }

	 rsize = files[nfile]->nrow;
	 csize = files[nfile]->ncol;

	 rsize *= nrow;
	 csize *= ncol;

	 continue;
      }
/*
 * "offsets" are the row/column offsets of the desired regions within
 * the input files
 */
      if(strcmp(files[nfile]->file, "offsets") == 0) {
	 if(files[nfile]->nrow >= 0) {
	    fprintf(stderr,
		    "Please specify only two numbers with \"offsets\"\n");
	 }

	 row0 = files[nfile]->rorig;
	 col0 = files[nfile]->corig;

	 continue;
      }
/*
 * read file
 */
      files[nfile]->nrow *= nrow;
      if(files[nfile]->rorig == -1) {
	 if(ororig < 0) {
	    ororig = 0;
	 } else {
	    ororig += nrow;
	 }
	 files[nfile]->rorig = ororig;
      } else {
	 files[nfile]->rorig *= nrow;
      }
      ororig = files[nfile]->rorig;
      
      files[nfile]->ncol *= ncol;
      if(files[nfile]->corig == -1) {
	 if(ocorig < 0) {
	    ocorig = 0;
	 } else {
	    ocorig += ncol;
	 }
	 files[nfile]->corig = ocorig;
      } else {
	 files[nfile]->corig *= ncol;
      }
      ocorig = files[nfile]->corig;

      if((fd = fopen(files[nfile]->file,"r")) == NULL) {
	 fprintf(stderr,"Can't open %s\n",files[nfile]->file);
	 exit(1);
      }
      
      if(fscanf(fd, "%s %d %d %d", magic, &files[nfile]->csize,
					 &files[nfile]->rsize, &maxval) != 4 ||
	 strncmp(magic, "P6", 2) != 0) {
	 fprintf(stderr,"Invalid header for PPM file %s\n:%s",
		 files[nfile]->file, line);
	 exit(1);
      }
      assert(maxval <= 255);		/* required by binary PPM */

      fclose(fd);
/*
 * set other fields
 */
      if(files[nfile]->nrow < 0) {
	 files[nfile]->nrow = (nrow > 1) ? nrow : files[nfile]->rsize;
      }
      if(files[nfile]->ncol < 0) {
	 files[nfile]->ncol = (ncol > 1) ? ncol : files[nfile]->csize;
      }
/*
 * see if this enlarges the required mosaic
 */
      if(csize > 0) {			/* we have a requested size */
	 if(csize < files[nfile]->corig + files[nfile]->ncol) {
	    if((files[nfile]->ncol = csize - files[nfile]->corig) < 0) {
	       continue;		/* entirely outside mosaic */
	    }
	 }
      } else if(csize < files[nfile]->corig + files[nfile]->ncol) {
	 csize = files[nfile]->corig + files[nfile]->ncol;
      }

      if(rsize > 0) {			/* we have a requested size */
	 if(rsize < files[nfile]->rorig + files[nfile]->nrow) {
	    if((files[nfile]->nrow = rsize - files[nfile]->rorig) < 0) {
	       continue;		/* entirely outside mosaic */
	    }
	 }
      } else if(rsize < files[nfile]->rorig + files[nfile]->nrow) {
	 rsize = files[nfile]->rorig + files[nfile]->nrow;
      }

      nfile++;
   }

   if(verbose) {
      fprintf(stderr,"mosaic is %dx%d\n", csize, rsize);
      for(i = 0; i < nfile; i++) {
	 fprintf(stderr,"%s is %dx%d; want %dx%d+%d+%d at %d+%d\n",
		 files[i]->file,
		 files[i]->csize, files[i]->rsize,
		 files[i]->ncol, files[i]->nrow,
		 files[i]->col0, files[i]->row0,
		 files[i]->corig, files[i]->rorig);
      }
   }
/*
 * build mosaic
 */
   output = shMalloc(rsize*sizeof(char *));
   output[0] = shMalloc(3*rsize*csize*sizeof(char));
   for(i = 1; i < rsize; i++) {
      output[i] = output[i-1] + 3*csize;
   }
   memset(output[0], '\0', rsize*csize);

   for(i = 0; i < nfile; i++) {
      fd = fopen(files[i]->file,"r");
      assert(fd != NULL);		/* we checked last time */

      fscanf(fd, "%*s %*d %*d %*d");	/* skip header */
      (void)fgetc(fd);			/* skip newline */

      if(fseek(fd, 3*(files[i]->row0*files[i]->csize + files[i]->col0),
							       SEEK_CUR) < 0) {
	 fprintf(stderr,"Cannot seek to start of data in %s",
		 files[i]->file);
	 perror(" ");
	 fclose(fd);
	 continue;
      }

      for(j = 0; j < files[i]->nrow; j++) {
	 if(j > 0 &&
	    fseek(fd, 3*(files[i]->csize - files[i]->ncol), SEEK_CUR) < 0) {
	    fprintf(stderr,"Cannot seek to start of %dth line of data in %s",
		    files[i]->row0 + j, files[i]->file);
	    perror(" ");
	    fclose(fd);
	    continue;
	 }
	 if(fread(&output[files[i]->rorig + j][3*files[i]->corig],
		  3*files[i]->ncol, 1, fd) != 1) {
	    fprintf(stderr,"Cannot read %dth line from %s",
		    files[i]->row0 + j, files[i]->file);
	    perror(" ");
	    fclose(fd);
	    break;
	 }
      }
   }
/*
 * write PPM file
 */
   if(outfile == NULL) {
      fd = stdout;
   } else {
      if((fd = fopen(outfile,"w")) == NULL) {
	 fprintf(stderr,"Can't open %s\n",outfile);
	 shFree(output[0]);
	 shFree(output);
	 exit(1);
      }
   }

   fprintf(fd, "P6 %d %d %d\n", csize - corig0, rsize - rorig0, maxval);
   for(i = rorig0; i < rsize; i++) {
      if(fwrite(&output[i][3*corig0], 3*(csize-corig0), 1, fd) != 1) {
	 fprintf(stderr,"Cannot write %dth output line",i);
	 perror(" ");
	 break;
      }
   }

   if(fd != stdout) {
      fclose(fd);
   }

   return(0);
}
