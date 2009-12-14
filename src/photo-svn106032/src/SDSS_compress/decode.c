/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* decode.c		read codes from infile and call either rice_decode or doqtreedec
 *
 * Programmer: R. White		Date: 27 July 1994
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "region.h"
#include "dervish_msg_c.h"
#include "phCompUtils.h"

static int verbose;

static char qcode_magic[2] = { (char)0xDD, (char)0x99 };
static char rcode_magic[2] = { (char)0xEE, (char)0xAA };

extern int qread (MYFILE *infile, char *a, int n);

extern int readint (MYFILE *infile);

extern int readshort (MYFILE *infile);

extern int
doencode(MYFILE *outfile, int *a, int nx, int ny,unsigned char nbitplanes[3]);

extern void fitspass(MYFILE *infile, int passthru, FILE *outfile);

extern void 
makefits(FILE *outfile, int nx, int ny, int bitpix, char *datatype);

extern int myread(MYFILE *file, char *buffer, int n);

extern void doqtreedec(MYFILE *infile, int *a, int nx, int ny);

extern void doricedec(MYFILE *infile, int *a, int nx, int ny);

extern int
decode(MYFILE *infile, FILE *outfile, int **a, int *nx, int *ny, int *scale,
       char **format)
{
  int nel, sumall, newfits = 0;
  char tmagic[2], line[81];
  
  /*
   * File starts either with special 2-byte magic code or with
   * FITS keyword "SIMPLE  ="
   */
  qread(infile, tmagic, sizeof(tmagic));
  /*
   * Check for FITS
   */
  if (strncmp(tmagic,"SI",2)==0) {
    /*
     * read rest of line and make sure the whole keyword is correct
     */
    strncpy(line,"SI",2);
    if ((myread(infile, &line[2], 78) != 78) || 
	(strncmp(line, "SIMPLE  =", 9) != 0) ) {
      fprintf(stderr, "bad file format\n");
      exit(-1);
    }
    /*
     * set output format to default "fits" if it is empty
     */
    if ((*format)[0] == '\0') *format = "fits";
    /*
     * if fits output format and outfile != NULL, write this line to
     * outfile and then copy rest of FITS header; else just read past
     * FITS header.
     */
    if (strcmp(*format, "fits") == 0 && outfile != (FILE *) NULL) {
      if (fwrite(line,1,80,outfile) != 80) {
	perror("stdout");
	fprintf(stderr, "decode: error writing output file\n");
	exit(-1);
      }
      fitspass(infile,1,outfile);
    } else {
      fitspass(infile,0,outfile);
    }
    /*
     * now read the first two characters again -- this time they
     * MUST be the magic code!
     */
    qread(infile, tmagic, sizeof(tmagic));
  } else {
    /*
     * set default format to raw if it is not specified
     */
    if((*format)[0] == '\0') *format = "raw";
    /*
     * if input format is not FITS but output format is FITS, set
     * a flag so we generate a FITS header once we know how big
     * the image must be.
     */
    if (strcmp(*format, "fits") == 0) newfits = 1;
  }
  /*
   * check for one of correct magic code values
   */
  if ( (memcmp(tmagic,qcode_magic,sizeof(qcode_magic)) != 0) &&
      (memcmp(tmagic,rcode_magic,sizeof(rcode_magic)) != 0) ) {
    fprintf(stderr, "bad file format\n");
    exit(-1);
  }
  *nx =readshort(infile);				/* x size of image					*/
  *ny =readshort(infile);				/* y size of image					*/
  *scale=readshort(infile);			/* scale factor for digitization	*/
  /*
   * write the new fits header to outfile if needed
   */
  if (newfits && (outfile != (FILE *) NULL))
    makefits(outfile,*nx,*ny,16,"INTEGER*2");
  /*
   * allocate memory for array
   * use calloc so it gets initialized to zero
   */
  nel = (*nx) * (*ny);
  *a = (int *) calloc(nel,sizeof(int));
  if (*a == (int *) NULL) {
    fprintf(stderr, "decode: insufficient memory\n");
    exit(-1);
  }
  /* sum of all pixels	*/
  sumall=readint(infile);
  
  /*
   * now call the selected decoding routine
   */
  if (memcmp(tmagic,qcode_magic,sizeof(qcode_magic)) == 0) {
    
    if (verbose) fprintf(stderr, "qtree decoding\n"); 
    doqtreedec(infile,*a,*nx,*ny);
    
  } else if (memcmp(tmagic,rcode_magic,sizeof(rcode_magic)) == 0) {
    
    if (verbose) fprintf(stderr, "Rice  decoding\n"); 
    doricedec(infile,*a,*nx,*ny);
    
  }
  /*
   * put sum of all pixels back into pixel 0
   */
  (*a)[0] = sumall;
  return(1);
}
