/*
 * test thcomp, in-memory compression procedure
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int  optind;
char *optarg;

int  verbose = 0;
static int  scale;
static char format[] = "fits";

extern int  thcomp(short *sa, int nx, int ny, int scale);

extern get_data(MYFILE *infile[2], char *inname[2], FILE *outfile, 
		int **a, int *nx, int *ny, char *format);
extern int getopt(int argc, char **argv, char *opts);

static void hcinit(int argc, char *argv[]);
static void usage(int argc, char *argv[]);

main (int argc, char *argv[])
{
int  *a, nx, ny, i, dd, nbytes, max;
short *sa;
double rms;
FILE *infile[2];
char *inname[2];

	/*
	 * Get command line arguments, open input file(s) if necessary
	 */
	hcinit(argc, argv);
	/*
	 * Read data, return address & size
	 */
	infile[0] = stdin;
	inname[0] = "stdin";
	get_data(infile, inname, (FILE *)NULL, &a, &nx, &ny, format);
	/*
	 * copy A to short array
	 */
	sa = (short *) malloc(nx*ny*sizeof(short));
	if (sa == (short *)NULL) {
		fprintf(stderr, "test_thcomp: insufficient memory\n");
		exit(-1);
	}
	for (i=0; i<nx*ny; i++) sa[i] = a[i];
	/*
	 * do compression and decompression
	 */
	nbytes = thcomp(sa,nx,ny,scale);
	if (nbytes < 0) { 
		fprintf(stderr, "error in thcomp\n");
		exit(-1);
	}
	/*
	 * compute RMS, maximum difference
	 */
	rms = 0.0;
	max = 0;
	for (i=0; i<nx*ny; i++) {
		dd = abs(a[i] - sa[i]);
		if (dd > max) max = dd;
		rms = rms + ((double) dd)*((double) dd);
	}
	rms = sqrt(rms/(nx*ny));
	fprintf(stderr,
	"%dx%d  Scale %d  Bytes %d Bits/pixel %6.2f  RMS %f  Max Diff %d\n",
		nx, ny, scale, nbytes, ((double) nbytes*8)/(nx*ny), rms, max);
	free(a);
	exit(0);
}

static void
usage(int argc, char *argv[])
{
	fprintf(stderr, "Usage: %s options\n",
		argv[0]);
	fprintf(stderr, "   where options = %s\n",
		"[-v] [-s scale]");
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
	verbose = 0;
	scale = 0;
	/*
	 * get options
	 */
	while ((c = getopt(argc,argv,"vs:")) != -1) {
		switch (c) {
		case 'v':
			/*
			 * verbose flag -v
			 */
			verbose = 1;
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
		case '?':
			usage(argc,argv);
		}
	}
	/*
	 * make sure there are not too many parameters
	 */
	if (optind < argc) {
		fprintf(stderr, "Too many parameters: %s ...\n", argv[optind]);
		usage(argc,argv);
	}
}
