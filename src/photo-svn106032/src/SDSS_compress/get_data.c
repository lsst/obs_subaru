/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* get_data.c	Read I*2 image data from infile and return as int
 *
 * Format may be raw, net, fits, or hhh.  For FITS input format, header
 * lines get written to outfile.
 *
 * Programmer: R. White		Date: 17 April 1992
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
extern int  test_swap(void);

extern void swap_bytes(unsigned char *a, int n);

static void get_raw(FILE *infile, char *inname, int **a, int nx, 
		    int ny, int swap);

static void get_fits(FILE *infile, char *inname, FILE *outfile, int **a, 
		     int *nx, int *ny);

static void get_hhh(FILE *infile[2], char *inname[2], int **a, int *nx, int *ny);

extern void fitsread(FILE *infile, char *inname, FILE *outfile, int *nx, 
		     int *ny, int passthru, int padded, int nlterm);

extern void
get_data(FILE *infile[2], char *inname[2], FILE *outfile, int **a, int *nx,
int *ny, char *format)
{
	if (strcmp(format,"raw") == 0) {
		get_raw(infile[0], inname[0], a, *nx, *ny, 0);
	} else if (strcmp(format,"net") == 0) {
		get_raw(infile[0], inname[0], a, *nx, *ny, 1);
	} else if (strcmp(format,"fits") == 0) {
		get_fits(infile[0], inname[0], outfile, a, nx, ny);
	} else if (strcmp(format,"hhh") == 0) {
		get_hhh(infile, inname, a, nx, ny);
	} else {
		fprintf(stderr, "get_data: unknown format %s\n", format);
		exit(-1);
	}
	/*
	 * Close files.  Note that two files are used only for format hhh.
	 * infile[0] and infile[1] may point to the same file (usually stdin).
	 */
	fclose(infile[0]);
	if (strcmp(format,"hhh") == 0 && infile[1] != infile[0]) fclose(infile[1]);
}

static void
get_raw(FILE *infile, char *inname, int **a, int nx, int ny, int swap)
{
int i, j, k, nread;
short *sa;
int tswap;

	*a = (int *) malloc(nx * ny * sizeof(int));
	/*
	 * read a row at a time to minimize page faulting problems
	 */
	sa = (short *) malloc(ny * sizeof(short));
	if (sa == (short *) NULL || *a == (int *) NULL) {
		fprintf(stderr, "insufficient memory\n");
		exit(-1);
	}
	/*
	 * see if byte swapping will be needed
	 */
	if (swap) {
		tswap = test_swap();
	} else {
		tswap = 0;
	}
	/*
	 * read rows
	 */
	for (i=0; i<nx; i++) {
	    /* might require several reads to get ny elements under VMS */
	    j = 0;
		while (j < ny) {
	    	nread = fread(&sa[j], sizeof(short), ny-j, infile);
			if (nread < 0) {
				fprintf(stderr,
					"error reading I*2 image (size %d %d) from %s\n",
					nx, ny, inname);
				exit(-1);
			}
	        j += nread;
	    }
		/*
		 * swap if necessary
		 */
		if (tswap) swap_bytes((unsigned char *) sa, ny*sizeof(short));
		/*
		 * copy to array A, converting to int
		 */
		k=i*ny;
		for (j=0; j<ny; j++) (*a)[k++] = sa[j];
	}
	free(sa);
}

/* FILE *infile;       input file pointer	
 * char *inname;       input file name		
 * FILE *outfile;      output file (NULL for none)
 * int  **a;	       pointer to image array (returned)    
 * int  *nx, *ny;      image is NX x NY				
 */
static void
get_fits(FILE *infile, char *inname, FILE *outfile, int **a, int *nx, int *ny)
{
	/*
	 * read fits header
	 */
	if (outfile != (FILE *) NULL) {
		/*
		 * 1: pass lines through to outfile
		 * 1: header is multiple of 2880 bytes
		 * 0: lines are not terminated by newline
		 */
		fitsread(infile, inname, outfile, nx, ny, 1, 1, 0);
	} else {
		/*
		 * no output file, don't pass lines through to outfile
		 */
		fitsread(infile, inname, outfile, nx, ny, 0, 1, 0);
	}
	/*
	 * read raw pixel data with byte swapping
	 */
	get_raw(infile, inname, a, *nx, *ny, 1);
}

static void
get_hhh(FILE *infile[2], char *inname[2], int **a, int *nx, int *ny)
{
	/*
	 * read fits header
	 * 0: don't pass lines through to output file
	 * 0: header is not multiple of 2880 bytes
	 * 1: lines are terminated by newline
	 */
	fitsread(infile[1], inname[1], (FILE *) NULL, nx, ny, 0, 0, 1);
	/*
	 * read raw pixel data with no byte swapping
	 */
	get_raw(infile[0], inname[0], a, *nx, *ny, 0);
}
