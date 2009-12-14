/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/*
 * test version for code that writes to buffer rather than file
 */
/* hcomp.c	Compress image file using
 *					(1) H-transform
 *					(2) Optimal digitization
 *					(3) Thresholding
 *					(4) Quadtree coding
 *
 * This version of the program can accept input in 4 formats:
 *
 * (1) raw: (DEFAULT) I*2 pixel values with bytes in machine-dependent
 *			order, i.e. no byte swapping is done on input.  Image size may
 *			be specified with the -r and -c parameters.
 *
 * (2) net: I*2 pixel values with bytes with bytes in "network" order:
 *			high byte first, then low byte for each I*2 pixel.  Byte-swapping
 *			is done on input if needed.  Note that this is the same as
 *			raw format on some machines (e.g. Suns) but is different on
 *			others (e.g. VAXes).  Files in net format can be transferred
 *			from one machine to another without modification, but files
 *			in raw format cannot.
 *
 * (3) fits: FITS format image.  Header gives image size.  Bytes are
 *			in network order.
 *
 * (4) hhh: Fits-type header (but without padding lines making header
 *			multiple of 2880 bytes), followed by I*2 pixel values in
 *			machine-dependent order.  No byte-swapping is done on input.
 *
 * For FITS input, the compressed output has the FITS header at the beginning.
 * For the other formats no header is included.  Note that the hhh header
 * is stored in a different file from the data; for this program the hhh
 * and hhd files may be concatenated into stdin, or they may be given
 * as command line arguments.
 *
 * The compressed output file is machine independent and can be transferred
 * between machines (e.g. using binary FTP).
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
 * a[nx][ny] is the image array (gets allocated in get_data)
 * Note that ny (set by command line parameter -r) is the fast-varying dimension
 *
 * scale is the scale factor for digitization
 *
 * format describes the input format
 */
int  verbose;
static FILE *infile[2];
static char *inname[2];
static int  *a, nx, ny;
static int  scale;
static char *format;

#ifdef VMS
/*
 * buffers to speed IO on VMS
 */
#define BUFSIZE 64000
static char inbuf[BUFSIZE+1];
static char outbuf[BUFSIZE+1];

#endif

extern void get_data(FILE **, char **, FILE *, int **, int *, int *, char *);

extern int htrans(int *, int , int);
extern int digitize(int *, int , int , int);
extern int encode(MYFILE *, int *, int, int, int);
extern int getopt(int argc, char **argv, char *opts);

static void hcinit(int argc, char **argv);
static void usage(int argc,char **arg);

main (int argc, char *argv[])
{
MYFILE *buffer;
int n;

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
	 * Get command line arguments, open input file(s) if necessary
	 */
	hcinit(argc, argv);
	/*
	 * Read data, return address & size
	 * FITS header will get written to stdout
	 */
	get_data(infile, inname, stdout, &a, &nx, &ny, format);
	if (verbose) {
		fprintf(stderr, "Image size (%d,%d)  Scale factor %d\n",
			ny,nx,scale);
	}
	/*
	 * H-transform
	 */
	htrans(a,nx,ny);
	/*
	 * Digitize
	 */
	digitize(a,nx,ny,scale);
	/*
	 * allocate output buffer
	 */
	buffer = (MYFILE *) malloc(sizeof(MYFILE));
	if (buffer == (MYFILE *) NULL) {
		fprintf(stderr, "hcomp: insufficient memory\n");
		exit(-1);
	}
	if (bufalloc(buffer, 4*nx*ny) == (unsigned char *) NULL) {
		fprintf(stderr, "hcomp: insufficient memory\n");
		exit(-1);
	}
	/*
	 * Encode and write to buffer
	 */
	encode(buffer,a,nx,ny,scale);
	/*
	 * Write buffer to stdout
	 */
	n = (buffer->current) - (buffer->start);
	if (fwrite(buffer->start, 1, n, stdout) != n) {
		fprintf(stderr, "hcomp: write to stdout failed\n");
		exit(-1);
	}
	buffree(buffer);
	free(buffer);
	free(a);
	exit(0);
}

static void
usage(int argc,char *argv[])
{
	fprintf(stderr, "Usage: %s options [data-file [header-file]]\n",
		argv[0]);
	fprintf(stderr, "   where options = %s\n",
		"[-v] [-i raw|net|fits|hhh] [-s scale] [-r rows] [-c columns]");
	exit(-1);
}


/* GET COMMAND LINE ARGUMENTS */
static void
hcinit(int argc, char *argv[])
{
int c;

	/*
	 * default values
	 */
	nx = -1;
	ny = -1;
	verbose = 0;
	scale = 1024;
	format = "raw";
	/*
	 * get options
	 */
	while ((c = getopt(argc,argv,"vr:c:s:i:")) != -1) {
		switch (c) {
		case 'v':
			/*
			 * verbose flag -v
			 */
			verbose = 1;
			break;
		case 'r':
			/*
			 * -r <row-size>
			 */
			if (sscanf(optarg,"%d",&ny) != 1) {
				fprintf(stderr, "bad row size: %s\n", optarg);
				usage(argc,argv);
			}
			break;
		case 'c':
			/*
			 * -c <column-size>
			 */
			if (sscanf(optarg,"%d",&nx) != 1) {
				fprintf(stderr, "bad column size: %s\n", optarg);
				usage(argc,argv);
			}
			break;
		case 's':
			/*
			 * -s <scale>
			 */
			if (sscanf(optarg,"%d",&scale) != 1) {
				fprintf(stderr, "bad scale: %s\n", optarg);
				usage(argc,argv);
			}
			break;
		case 'i':
			/*
			 * -i <format> = raw, net, fits, or hhh
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
	 * get optional input file names from trailing parameters and open files
	 */
	if (optind < argc) {
		inname[0] = argv[optind++];
		if ((infile[0] = fopen(inname[0], "r")) == (FILE *) NULL) {
			fprintf(stderr, "Cannot open data file %s\n", inname[0]);
			exit(-1);
		}
	} else {
		/*
		 * default input is from stdin
		 */
		infile[0] = stdin;
		inname[0] = "stdin";
	}
	/*
	 * header file is only used for format "hhh"
	 */
	if (strcmp(format,"hhh") == 0) {
		if (optind < argc) {
			inname[1] = argv[optind++];
			if ((infile[1] = fopen(inname[1], "r")) == (FILE *) NULL) {
				fprintf(stderr, "Cannot open header file %s\n", inname[1]);
				exit(-1);
			}
		} else {
			/*
			 * default input is from same source as data
			 */
			infile[1] = infile[0];
			inname[1] = inname[0];
		}
	}
	/*
	 * make sure there are not too many parameters
	 */
	if (optind < argc) {
		fprintf(stderr, "Too many parameters: %s ...\n", argv[optind]);
		usage(argc,argv);
	}
	/*
	 * it is an error to specify row or column sizes for fits or hhh
	 * input formats
	 */
	if (strcmp(format,"fits") == 0 || strcmp(format,"hhh") == 0) {
		if (nx != -1 || ny != -1) {
			fprintf(stderr, "Cannot specify rows or columns with format %s\n",
				format);
			usage(argc,argv);
		}
	} else {
		/*
		 * default for row, column sizes:
		 * if one is specified they are equal
		 * if neither is specified then both = 256
		 */
		if (nx == -1) {
			if (ny == -1) {
				nx = 256;
				ny = 256;
			} else {
				nx = ny;
			}
		} else if (ny == -1) {
			ny = nx;
		}
	}
}
