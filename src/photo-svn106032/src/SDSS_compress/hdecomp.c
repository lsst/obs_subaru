/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* hdecomp.c	Decompress image file that was compressed using hcomp.
 *
 * This version of the program can produce output in 4 formats:
 *
 * (1) raw: I*2 pixel values with bytes in machine-dependent order,
 *		i.e. no byte swapping is done on output.  Image size may
 *		be specified with the -r and -c parameters.
 *
 * (2) net: I*2 pixel values with bytes with bytes in "network" order:
 *	high byte first, then low byte for each I*2 pixel.  Byte-swapping
 *	is done on output if needed.  Note that this is the same as
 *	raw format on some machines (e.g. Suns) but is different on
 *	others (e.g. VAXes).  Files in net format can be transferred
 *	from one machine to another without modification, but files
 *	in raw format cannot.
 *
 * (3) fits: FITS format image.  Header gives image size.  Bytes are
 *	in network order.
 *
 * (4) hhh: This is identical to raw format, but is included for consistency
 *	with the input formats used by hcomp.
 *
 * The compressed input file may have two different formats, which are
 * recognized automatically by the program.  If the compressed file was
 * produced from a FITS input file, then the compressed file includes the
 * (uncompressed) FITS header at the start of the file, followed by the
 * compressed image data.  Otherwise the compressed input file contains
 * only the compressed image data.
 *
 * If the compressed file has a FITS header, then the default output format
 * is fits.  Otherwise the default output format is raw.
 *
 * Programmer: R. White		Date: 30 June 1994
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "region.h"
#include "shiva_msg_c.h"
#include "phCompUtils.h"

int  optind;
char *optarg;

/*
 * a[nx][ny] is the image array (gets allocated in decode)
 * Note that ny is the fast-varying dimension
 *
 * scale is the scale factor for digitization
 */
int  verbose;
static int  *a, nx, ny;
static int  scale;
static int  smooth;
static char *format;

#ifdef VMS
/*
 * buffers to speed VMS IO
 */
#define BUFSIZE 64000
static char inbuf[BUFSIZE+1];
static char outbuf[BUFSIZE+1];

#endif

extern int
decode(MYFILE *infile, FILE *outfile, int **a, int *nx, int *ny, int *scale,
       char **format);

extern int
undigitize(int a[], int nx, int ny, int scale);

extern int 
hinv(int a[], int nx, int ny, int smooth, int scale);

extern void put_data(FILE *outfile, int a[], int nx, int ny, char *format);

extern int getopt(int argc, char **argv, char *opts);

static void get_args(int argc, char *argv[]);

static int fillbuffer(FILE *infile, MYFILE *buffer);

main (int argc, char *argv[])
{
MYFILE *buffer;

#ifdef VMS
	/*
	 * set buffer sizes to improve IO speeds on VMS
	 */
	if (setvbuf(stdin, inbuf, _IOFBF, BUFSIZE) != 0) {
		perror("setvbuf(stdin) failed");
	}
	if (setvbuf(stdout, outbuf, _IOFBF, BUFSIZE) != 0) {
		perror("setvbuf(stdout) failed");
	}
#endif
	/*
	 * Get command line arguments
	 */
	get_args(argc, argv);
	/*
	 * Read everything from stdin and fill buffer
	 */
	if ((buffer = (MYFILE *) malloc(sizeof(MYFILE))) == (MYFILE *) NULL) {
		fprintf(stderr, "hdecomp: insufficient memory\n");
		exit(-1);
	}
	if (fillbuffer(stdin,buffer)) {
		fprintf(stderr, "fillbuffer failed\n");
		exit(-1);
	}
	/*
	 * Read from buffer, passing header through to stdout for FITS format,
	 * and decode.  Returns address, size, scale,
	 * and (possibly) format
	 */
	decode(buffer,stdout,&a,&nx,&ny,&scale,&format);
	/*
	 * Un-Digitize
	 */
	undigitize(a,nx,ny,scale);
	/*
	 * Inverse H-transform
	 */
	hinv(a,nx,ny,smooth,scale);
	/*
	 * Write data
	 */
	put_data(stdout,a,nx,ny,format);
	free(a);
	buffree(buffer);
	free(buffer);
	if (verbose) {
		if (smooth) {
			fprintf(stderr,
		"Image size (%d,%d)  Scale factor %d  Smoothed  Output in %s format\n",
				ny,nx,scale,format);
		} else {
			fprintf(stderr,
		"Image size (%d,%d)  Scale factor %d  Output in %s format\n",
				ny,nx,scale,format);
		}
	}
	exit(0);
}

static void
usage(int argc, char *argv[])
{
	fprintf(stderr,
		"Usage: %s [-v] [-s] [-o raw|net|fits|hhh]\n",
		argv[0]);
	exit(-1);
}

/* GET COMMAND LINE ARGUMENTS */

static void
get_args(int argc, char *argv[])
{
int c;

	/*
	 * default values
	 */
	verbose = 0;
	smooth = 0;
	format = "";
	/*
	 * get options
	 */
	while ((c = getopt(argc,argv,"vso:")) != -1) {
		switch (c) {
		case 'v':
			/*
			 * verbose flag -v
			 */
			verbose = 1;
			break;
		case 's':
			/*
			 * smoothing flag -s
			 */
			smooth = 1;
			break;
		case 'o':
			/*
			 * -o <format> = raw, net, fits, or hhh
			 */
			format = optarg;
			if (strcmp(format,"raw")  != 0 &&
				strcmp(format,"net")  != 0 &&
				strcmp(format,"fits") != 0 &&
				strcmp(format,"hhh")  != 0) {
				fprintf(stderr, "Illegal input format %s\n", format);
				usage(argc,argv);
			}
			break;
		case '?':
			usage(argc,argv);
		}
	}
	/*
	 * make sure there aren't any trailing parameters being ignored
	 */
	if (optind < argc) {
		fprintf(stderr, "Too many parameters: %s ...\n", argv[optind]);
		usage(argc,argv);
	}
}

/*
 * fillbuffer.c   Read entire file into buffer.
 *
 * This is a bit clumsy, but it is only used for testing.
 */

/*
 * returns 0 for success, 1 for failure
 */
static int
fillbuffer(FILE *infile, MYFILE *buffer)
{
unsigned char *buf, *oldbuf;
int nbuf, nread, nblock = 64000;

	/*
	 * read a block at a time
	 */
	buf = (unsigned char *) malloc(nblock);
	if (buf == (unsigned char *)NULL) {
		fprintf(stderr, "fillbuffer: insufficient memory\n");
		return(1);
	}
	nread = fread(buf, 1, nblock, infile);
	nbuf = nread;
	while (nread == nblock) {
		oldbuf = buf;
		buf = (unsigned char *) realloc(buf, nbuf+nblock);
		if (buf == (unsigned char *) NULL) {
			fprintf(stderr, "fillbuffer: insufficient memory in loop\n");
			fprintf(stderr, "fillbuffer: nbuf = %d\n", nbuf);
			free(oldbuf);
			return(1);
		}
		nread = fread(buf+nbuf, 1, nblock, infile);
		nbuf = nbuf+nread;
	}
	buffer->start = buf;
	buffer->end = buf+nbuf;
	buffer->current = buf;
	return(0);
}
