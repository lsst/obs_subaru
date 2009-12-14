/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* put_data.c	Write int image data to outfile as I*2, swapping bytes
 *				if requested
 *
 * Programmer: R. White		Date: 17 April 1992
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int  test_swap(void);
extern void swap_bytes(unsigned char *a, int n);

static void put_raw(FILE *outfile, int *a, int nx, int ny, int swap);
static void put_fits(FILE *outfile, int *a, int nx, int ny);

extern void
put_data(FILE *outfile, int a[], int nx, int ny, char *format)
{
	if (strcmp(format,"raw") == 0 || strcmp(format,"hhh") == 0) {
		put_raw(outfile, a, nx, ny, 0);
	} else if (strcmp(format,"net") == 0) {
		put_raw(outfile, a, nx, ny, 1);
	} else if (strcmp(format,"fits") == 0) {
		put_fits(outfile, a, nx, ny);
	} else {
		fprintf(stderr, "put_data: unknown format %s\n", format);
		exit(-1);
	}
}

static void
put_raw(FILE *outfile, int a[], int nx, int ny, int swap)
{
int i, j, k;
short *sa;
int tswap;

	/*
	 * see if byte-swapping will be needed
	 */
	if (swap) {
		tswap = test_swap();
	} else {
		tswap = 0;
	}
	/*
	 * write a row at a time to minimize page faulting problems
	 */
	sa = (short *) malloc(ny * sizeof(short));
	if (sa == (short *) NULL) {
		fprintf(stderr, "insufficient memory\n");
		exit(-1);
	}
	for (i=0; i<nx; i++) {
		k = i*ny;
		for (j=0; j<ny; j++) sa[j] = a[k++];
		if (tswap) swap_bytes((unsigned char *) sa,ny*sizeof(short));
		if(fwrite(sa, 1, ny*sizeof(short), outfile) != ny*sizeof(short)) {
			perror("put_data");
			exit(-1);
		}
	}
	free(sa);
}

static void
put_fits(FILE *outfile, int a[], int nx, int ny)
{
int n;
short *sa;

	/*
	 * write data with swapping
	 */
	put_raw(outfile,a,nx,ny,1);
	/*
	 * add zeroes to get up to multiple of 2880 bytes
	 * number of padding short ints to write is between 0 and 1439
	 */
	n = 1439 - ((nx*ny-1) % 1440);
	if (n > 0) {
		/*
		 * allocate a block of zeros
		 */
		sa = (short *) calloc(n , sizeof(short));
		if (sa == (short *) NULL) {
			fprintf(stderr, "insufficient memory\n");
			exit(-1);
		}
		if(fwrite(sa, 1, n*sizeof(short), outfile) != n*sizeof(short)) {
			perror("put_data");
			exit(-1);
		}
		free(sa);
	}
}
