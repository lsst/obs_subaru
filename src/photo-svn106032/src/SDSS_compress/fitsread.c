/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* fitsread.c	Read FITS header from infile and (optionally) pass it to
 *				outfile.  Find naxis1 and naxis2 on the way, returning them as
 *				ny and nx.
 *				This will exit with error status if image is not 2-D or if
 *				image has either group parameters or multiple groups.
 *
 * Based on the fitspass program written by Saul Perlmutter
 *
 * Programmer: R. White				Date: 16 April 1992
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void
fitsread(FILE *infile, char *inname, FILE *outfile, int *nx, int *ny, 
	 int passthru, int padded, int nlterm)
/* FILE *infile;	        input file pointer	
 * char *inname;	       input file name		
 * FILE *outfile;	       output file pointer	
 * int  *nx, *ny;	       image is NX x NY		
 * int  passthru;	       non-zero -> copy header to outfile
 * int  padded;		       non-zero -> header is padded to 2880 bytes
 * int  nlterm;		       non-zero -> lines are terminated with newlines
 */
{
char line[82];
int naxis1, naxis2, val;
int i, j, noend, nline;

	/*
	 * make sure we find NAXIS1 and NAXIS2
	 */
	naxis1 = 0;
	naxis2 = 0;
	noend=1;
	nline = 81 + (nlterm != 0);
	while (noend) {
		/*
		 * continue looping until we've read END statement or
		 * until we've read a multiple of 36 lines past END (if padded != 0)
		 */
		for (i=0; (i<36) && (noend || padded); i++) {
			if (fgets(line,nline,infile)==NULL) {
				fprintf(stderr,"FITS header not complete in %s\n",inname);
				exit(-1);
			}
			/* If newline is in buffer, get rid of it */
			line[80] = '\0';
			/* Replace line of nulls with line of blanks. */
			if (strlen(line)!=80) {
				for (j=strlen(line); j<80; j++) line[j]=' ';
			}

			/*
			 * copy to outfile if passthru != 0 and we have not reached END
			 */
			if (passthru && noend) fputs(line,outfile); 

			if (strncmp(line,"END ",4) == 0) {
				noend=0;
			} else if (strncmp(line,"NAXIS1  =",9) == 0) {
				if(sscanf(&line[10], " %d", &naxis1) != 1) {
					fprintf(stderr, "Error reading NAXIS1 = %20.20s from %s\n",
						&line[10], inname);
					exit(-1);
				}
			} else if (strncmp(line,"NAXIS2  =",9) == 0) {
				if(sscanf(&line[10], " %d", &naxis2) != 1) {
					fprintf(stderr, "Error reading NAXIS2 = %20.20s from %s\n",
						&line[10], inname);
					exit(-1);
				}
			} else if (strncmp(line,"NAXIS   =",9) == 0) {
				if(sscanf(&line[10], " %d", &val) != 1) {
					fprintf(stderr, "Error reading NAXIS = %20.20s from %s\n",
						&line[10], inname);
					exit(-1);
				}
				if (val != 2) {
					fprintf(stderr, "Not a 2-dimensional image in %s\n",inname);
					exit(-1);
				}
			} else if (strncmp(line,"DATATYPE=",9) == 0) {
				if (strncmp(&line[11],"INTEGER*2",9) != 0) {
					fprintf(stderr, "Not an INTEGER*2 image in %s\n",inname);
					exit(-1);
				}
			} else if (strncmp(line,"PSIZE   =",9) == 0) {
				/*
				 * check PSIZE == 0
				 */
				if(sscanf(&line[10], " %d", &val) != 1) {
					fprintf(stderr, "Error reading PSIZE = %20.20s from %s\n",
						&line[10], inname);
					exit(-1);
				}
				if (val != 0) {
					fprintf(stderr, 
						"Image %s has group parameters, I can't compress it\n",
						inname);
					exit(-1);
				}
			} else if (strncmp(line,"GCOUNT  =",9) == 0) {
				/*
				 * check for GCOUNT > 1
				 */
				if(sscanf(&line[10], " %d", &val) != 1) {
					fprintf(stderr, "Error reading GCOUNT = %20.20s from %s\n",
						&line[10], inname);
					exit(-1);
				}
				if (val > 1) {
					fprintf(stderr,
						"Image %s has %d groups, I can't compress it\n",
						inname,val);
					exit(-1);
				}
			}
		}
	}
	if (naxis1 <= 0 || naxis2 <= 0) {
		fprintf(stderr,
			"Did not find legal values for NAXIS1 and NAXIS2 in %s\n",inname);
		exit(-1);
	}

	*ny = naxis1;
	*nx = naxis2;
}
