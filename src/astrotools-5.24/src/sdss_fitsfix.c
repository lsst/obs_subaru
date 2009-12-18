/* Short program to convert sdss data files from SIMPLE=F unsigned 16 bit ints to
 * SIMPLE=T signed 16 bit ints.  One could use Derivsh to do the conversion,
 * but it seems easier to write a stand-alone program
 *
 * This program is written to be stand-alone and work on all SDSS supported
 * platforms.
 * 
 * To compile,		"cc -o sdssfix sdssfix.c"
 * To run,		"sdssfix *.fit"
 *
 * Files already converted are left untouched.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MIN(x,y)  x<y ? (x) : (y)


int
main(int argc, char **argv) {

/* Storage for one header block */

   typedef struct {
	char key[9];
	char pad1[1];
	char arg[21];
	char pad2[49];
	} LINE;

   LINE hdrbuf[36];

/* Decoded things - we trim out spaces in these */
   char key[36][9];
   char arg[36][21];

/* Decoded parameters */

/* Storage for one block of data */
   unsigned char data[2880];

   int i;
   int j;
   int n;
   int verbose = 1;
   int flag;
   int nmax;

/* Temp. variables used by seek and tell */

   fpos_t pos1, pos2;

   FILE *fptr = NULL;

   int bitpix;
   int naxis;
   int naxis1, naxis2;

/* Inputs:
 *	Flags
 *		-h	Print help and exit
 *		-q	Quiet (normally, file names are echoed to stdout)
 *		--	Treat all subsequent aruments as file names.
 *	File names	0 or more.  Use the shell to perform wild card expansion
 *
 * Typical usage:
 *	sdssfix *.fit
*/

/* Process flags */
   i = 0;
   while (i < argc-1) {
	i++;
	if (strlen(argv[i]) == 0) continue;
	if (argv[i][0] != '-') break;
	if (strcmp(argv[i],"--") == 0) {
		i++;
		break;
		}
	if (strcmp(argv[i], "-h") == 0) {
	   printf ("sdssfix [flags] [files]\n");
	   printf ("   Flags:\n");
	   printf ("      -h   Print help and exit\n");
	   printf ("      -q   Quiet mode\n");
	   printf ("      --   Treat all subsequent arguments as file names\n");
	   exit(0);
	   }
	if (strcmp(argv[i], "-q") == 0) {
	   verbose = 0;
	   continue;
	   }
	printf ("Warning: Unknown option %s\n", argv[i]);
	}
/* The index i should point to the first file name */

   fptr = NULL;
   for (; i<argc; i++) {

/* Clean-up if a file is still open */
	if (fptr != NULL) {
	   fclose (fptr);
	   fptr = NULL;
	   }
	fptr = fopen (argv[i],"r+");
	if (fptr == NULL) {
	   printf ("Warning: Cannot open file %s in read/write mode; \
bypassing.\n", argv[i]);
	   continue;
	   }

/* Read the 1st 2880 bytes. Check for an SDSS header */
	fgetpos (fptr, &pos1);
	n = fread(hdrbuf, 80, 36, fptr);
	fgetpos (fptr, &pos2);
/* Got enough bytes? */
	if (n != 36) {
	   printf ("Read %d header records; bypassing %s\n", n, argv[i]);
	   continue;
	   }
/* Decode header */
	for (j=0; j<36; j++) {
	   hdrbuf[j].key[8] = '\0';
	   hdrbuf[j].arg[20] = '\0';
	   sscanf (hdrbuf[j].key, "%s", key[j]);
	   sscanf (hdrbuf[j].arg, "%s", arg[j]);
/*	   printf ("Key = %s, arg = %s.\n", key[j], arg[j]);            */
	   }
/* Now look for magic keywords.  First, are we a FITS header with SIMPLE = F? */
	if (strcmp (key[0], "SIMPLE") != 0) continue;
	if (strcmp (arg[0], "F") != 0) continue;
/* Update header to true */
	hdrbuf[0].arg[19] = 'T';
/* Be maniacal and check for all standard FITS keywords in proper order */
	if (strcmp(key[1], "BITPIX") != 0) continue;
/* BITPIX must be either 16 or -16 */
	bitpix = atol (arg[1]);
	if (bitpix != 16 && bitpix != -16) continue;
/* Update BITPIX to +16 */
	sprintf (hdrbuf[1].arg, "%20d", 16);
	if (strcmp(key[2], "NAXIS") != 0) continue;
/* AXES and dimensions */
	naxis = atol (arg[2]);
	if (naxis != 2) continue;
	if (strcmp(key[3], "NAXIS1") != 0) continue;
	naxis1 = atol (arg[3]);
	if (strcmp(key[4], "NAXIS2") != 0) continue;
	naxis2 = atol (arg[4]);
/* The following keywords are not required to be in exactly this order, but I
 * make the big gamble that they are.
*/
	if (strcmp(key[5], "SDSS") != 0) continue;
	if (strcmp(key[6], "UNSIGNED") != 0) continue;
	if (verbose) printf ("%s\n", argv[i]);
/* Replace these two keywords with BZERO and BSCALE.  Be sure to pad with spaces */
	strncpy (hdrbuf[5].key, "BZERO   ", 8);
	sprintf (hdrbuf[5].arg, "%20f", 32768.);
	strncpy (hdrbuf[6].key, "BSCALE  ", 8);
	sprintf (hdrbuf[6].arg, "%20f", 1.);
/* Clear out the NULLs that we inserted to make parsing easier */
	for (j=0; j<36; j++) {
	   hdrbuf[j].key[8] = '=';
	   hdrbuf[j].arg[20] = ' ';
	   }
/* Re-write the header */
	fsetpos (fptr, &pos1);
/* And rewrite */
	fwrite (hdrbuf, 80, 36, fptr);
	fsetpos (fptr, &pos2);
/* Now look for END */
	while (1) {
	   flag = 0;
	   for (j=0; j<36; j++) {
		if (strcmp(key[j],"END") == 0) {
		   flag = 1;
		   break;
		   }
		}
	   if (flag == 1) break;

/* Read the next 2880 bytes. */
	   n = fread(hdrbuf, 80, 36, fptr);
/* Got enough bytes? */
	   if (n != 36) {
		printf ("Panic in %s: ran out of header records in a \
valid FITS file!", argv[i]);
		goto next;
		}
/* Decode header keywords only */
	   for (j=0; j<36; j++) {
		hdrbuf[j].key[8] = '\0';
		sscanf (hdrbuf[j].key, "%s", key[j]);
		}
	   }

/* Now read image pixels and XOR the msb of the 1st byte of each 2-byte integer */
/* j is number of data bytes - NOT pixels */
	for (j=0; j< 2*naxis1*naxis2; j += 2880) {
	   fgetpos (fptr, &pos1);
	   n = fread (data, 2880, 1, fptr);
	   fgetpos (fptr, &pos2);
	   if (n != 1) {
		printf ("Panic in %s: ran out of data records in a \
valid FITS file!\n", argv[i]);
		goto next;
		}
/* Only operate on pixels up to the end of valid data; leave extra bytes alone */
/* n is index of pixel, NOT byte */
	   nmax = MIN((2*naxis1*naxis2 - j)/2, 1440);
	   for (n=0; n < nmax; n++) {
		data[2*n] += 128;
		}
	   fsetpos (fptr, &pos1);
	   fwrite (data, 2880, 1, fptr);
	   fsetpos (fptr, &pos2);
	   }
/* There may be additional data in the file (allowed by the FITS standard), which
 * remains untouched.
*/

next:	continue;
	}
   if (fptr != NULL) fclose(fptr);

   return(0);
   }

