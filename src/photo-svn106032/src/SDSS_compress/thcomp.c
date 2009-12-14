/* thcomp.c
 *
 * Given I*2 input image:
 * (1) Compress using H-transform compression
 * (2) Uncompress
 * (3) Return modified pixel values in place of original image and
 *		return number of bytes required for compressed image as
 *		function value.
 *
 * On error returns -1.
 *
 * This version also tries Rice compression of the thresholded H-transform
 * and returns number of bytes used by Rice if smaller than the quadtree
 * number.
 *
 * R. White, 27 July 1994
 */
#include <stdio.h>
#include <stdlib.h>
#include "region.h"
#include "dervish_msg_c.h"
#include "phCompUtils.h"

static int verbose;

extern int htrans(int a[], int nx, int ny);
extern int digitize(int a[], int nx, int ny, int scale);
extern int encode(MYFILE *outfile, int a[], int nx, int ny,int scale);
extern int decode(MYFILE *infile, FILE *outfile, int **a, int *nx, 
		  int *ny, int *scale, char **format);
extern int undigitize(int a[], int nx, int ny, int scale);
extern int hinv(int a[], int nx, int ny, int smooth, int scale);
extern int rice_encode(MYFILE *outfile, int a[] , int nx, int ny, int scale);

extern int
thcomp(short *sa, int nx, int ny, int scale)
{
int *a, i, newnx, newny, newscale, nqtree, nrice, nbytes;
MYFILE *qtree_buffer, *rice_buffer, *buffer;
char *format;

/* ------------ initialization ------------ */

	/*
	 * copy input array to long int array
	 */
	a = (int *) malloc(nx*ny*sizeof(int));
	if (a == (int *) NULL) {
		fprintf(stderr, "thcomp: insufficient memory\n");
		return(-1);
	}
	for (i=0; i<nx*ny; i++) a[i] = sa[i];

/* ------------ compression ------------ */

	/*
	 * H-transform
	 */
	htrans(a,nx,ny);
	/*
	 * Digitize
	 */
	digitize(a,nx,ny,scale);
	/*
	 * allocate buffers for qtree, Rice compressed data
	 */
	qtree_buffer = (MYFILE *) malloc(sizeof(MYFILE));
	if (qtree_buffer == (MYFILE *) NULL) {
		fprintf(stderr, "thcomp: insufficient memory\n");
		return(-1);
	}
	if (bufalloc(qtree_buffer, 4*nx*ny+100) == (unsigned char *) NULL) {
		fprintf(stderr, "thcomp: insufficient memory\n");
		return(-1);
	}
	rice_buffer = (MYFILE *) malloc(sizeof(MYFILE));
	if (rice_buffer == (MYFILE *) NULL) {
		fprintf(stderr, "thcomp: insufficient memory\n");
		return(-1);
	}
	if (bufalloc(rice_buffer, 4*nx*ny+100) == (unsigned char *) NULL) {
		fprintf(stderr, "thcomp: insufficient memory\n");
		return(-1);
	}
	/*
	 * Rice encode and write to buffer
	 */
	rice_encode(rice_buffer,a,nx,ny,scale);
	if (verbose) fprintf(stderr, "thcomp: finished Rice encoding\n");
	/*
	 * Qtree encode and write to buffer
	 */
	encode(qtree_buffer,a,nx,ny,scale);
	/*
	 * array will get re-allocated in decode
	 */
	free(a);
	/*
	 * save number of bytes required, then set buffer
	 * pointers to show proper array size
	 */
	nqtree = qtree_buffer->current - qtree_buffer->start;
	qtree_buffer->end = qtree_buffer->current;
	qtree_buffer->current = qtree_buffer->start;
	if (verbose) fprintf(stderr, "thcomp: qtree encoding %d bytes\n", nqtree);

	nrice = rice_buffer->current - rice_buffer->start;
	rice_buffer->end = rice_buffer->current;
	rice_buffer->current = rice_buffer->start;
	if (verbose) fprintf(stderr, "thcomp: Rice  encoding %d bytes\n", nrice);
	if (verbose) fprintf(stderr, "thcomp: compression done\n");

	/*
	 * select best compression & free other buffer
	 */
	if (nqtree > nrice) {
		/* Rice is best */
		buffree(qtree_buffer);
		free(qtree_buffer);
		buffer = rice_buffer;
		nbytes = nrice;
		if (verbose) fprintf(stderr, "Selecting Rice\n");
	} else {
		/* Qtree is best */
		buffree(rice_buffer);
		free(rice_buffer);
		buffer = qtree_buffer;
		nbytes = nqtree;
		if (verbose) fprintf(stderr, "Selecting Qtree\n");
	}

	/*
	 * if compressed version takes more bytes than original,
	 * return immediately
	 */
	if (nbytes > 2*nx*ny) {
		if (verbose)
			fprintf(stderr,
				"thcomp: %d x %d image required %d bytes, no compression\n",
				nx, ny, nqtree);
		/* free storage */
		buffree(buffer);
		free(buffer);
		free(a);

		/* return number of bytes required for original image */

		return(2*nx*ny);
	}

/* ------------ decompression ------------ */

	/*
	 * Read from buffer and decode.  Returns address, size, scale,
	 * and (possibly) format
	 */
	format = "\0";
	decode(buffer,(FILE *)NULL,&a,&newnx,&newny,&newscale,&format);
	/*
	 * check nx,ny,scale for consistency
	 */
	if (newnx != nx || newny != ny || newscale != scale) {
		fprintf(stderr, "thcomp: decode values inconsistent with input\n");
		fprintf(stderr,
			"Input: nx %d ny %d scale %d   Output: nx %d ny %d scale %d\n",
			nx, ny, scale, newnx, newny, newscale);
		return(-1);
	}
	/*
	 * Un-Digitize
	 */
	undigitize(a,nx,ny,scale);
	/*
	 * Inverse H-transform
	 */
	hinv(a,nx,ny,0,scale);

/* ------------ finish ------------ */

	/* convert back to short, checking range */
	for (i=0; i<nx*ny; i++) {
		if (a[i] > 32767) {
			sa[i] = 32767;
		} else if (a[i] < -32768) {
			sa[i] = 32768;
		} else {
			sa[i] = a[i];
		}
	}
	/* free storage */
	buffree(buffer);
	free(buffer);
	free(a);
	if (verbose) fprintf(stderr, "thcomp: decompression done\n");

	/* return number of bytes required by best compression method */

	return(nbytes);
}
