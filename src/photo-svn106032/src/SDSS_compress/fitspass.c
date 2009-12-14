/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* fitspass.c	Read FITS header from infile and pass it to outfile,
 *				Adds blank lines to make header a multiple of 36 lines.
 *				This may exit with error status if some problem is found.
 *
 * Programmer: R. White		Date: 16 April 1992
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "region.h"
#include "dervish_msg_c.h"
#include "phCompUtils.h"

extern int myread(MYFILE *file, char buffer[], int n);

extern void
fitspass(MYFILE *infile, int passthru, FILE *outfile)
{
char line[80];
int i, j;

	/*
	 * Note that the SIMPLE line has already been stripped off and written
	 * to outfile for FITS files, so we start at i=1
	 */
	for (i = 1; ; i++) {
		if (myread(infile, line, 80) != 80) {
			fprintf(stderr,"FITS header has no END statement\n");
			exit(-1);
		}

		if (passthru) {
			if (fwrite(line,1,80,outfile) != 80) {
				perror("stdout");
				fprintf(stderr, "fitspass: error writing output file\n");
				exit(-1);
			}
		}

		if (strncmp(line,"END ",4) == 0) break;
	}
	/*
	 * write blank lines to make a multiple of 36 lines in header
	 * number of lines written so far is i+1
	 */
	if (passthru) {
		for (j=0; j<80; j++) line[j]=' ';
		for (i = 35 - (i % 36); i>0; i--) {
			if (fwrite(line,1,80,outfile) != 80) {
				perror("stdout");
				fprintf(stderr, "fitspass: error writing output file\n");
				exit(-1);
			}
		}
	}
}
