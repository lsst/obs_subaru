/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* makefits.c	Write a simple FITS header for a 2-D image to outfile.
 *
 * Programmer: R. White		Date: 24 April 1992
 *
 * example of header
 * lines must be exactly 80 characters, with no newline at the end
 *
0123456789 123456789 123456789 123456789
SIMPLE  =                    T /Standard FITS format
BITPIX  =                   16 /
NAXIS   =                    2 /Number of axes
NAXIS1  =                  256 /
NAXIS2  =                  256 /
DATATYPE= 'INTEGER*2'          /
END
 *
 */
#include <stdio.h>
#include <string.h>

void
makefits(
	 FILE *outfile,
	 int nx, int ny,
	 int bitpix,
	 char *datatype
	 )
{
char line[81];
int i;

	fprintf(outfile, "%-80.80s",
		"SIMPLE  =                    T /Standard FITS format");

	strcpy(line,"BITPIX  =                      /");
	sprintf(&line[10], "%20d", bitpix);
	line[30] = ' ';
	fprintf(outfile, "%-80.80s", line);

	fprintf(outfile, "%-80.80s",
		"NAXIS   =                    2 /Number of axes");

	strcpy(line,"NAXIS1  =                      /");
	sprintf(&line[10], "%20d", ny);
	line[30] = ' ';
	fprintf(outfile, "%-80.80s", line);

	strcpy(line,"NAXIS2  =                      /");
	sprintf(&line[10], "%20d", nx);
	line[30] = ' ';
	fprintf(outfile, "%-80.80s", line);

	strcpy(line,"DATATYPE=                      /");
	sprintf(&line[10], "'%*.*s'", 
			(int)strlen(datatype),(int)strlen(datatype),datatype);
	line[12+strlen(datatype)] = ' ';
	fprintf(outfile, "%-80.80s", line);

	fprintf(outfile, "%-80.80s", "END");

	/*
	 * pad with blank lines to get multiple of 36 (2880 bytes)
	 */
	for (i=0; i<80; i++) line[i] = ' ';
	line[80] = '\0';
	for (i=7; i<36; i++) fputs(line,outfile);
}
