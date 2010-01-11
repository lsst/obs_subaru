#include <stdio.h>
#include <assert.h>
#include <arpa/inet.h> // htons
#include "shCFitsIo.h"

RET_CODE shRegWriteAsFits
   (
   REGION      *reg,		/* IN:  Name of region */
   char        *fn,		/* IN:  Name of FITS file */
   FITSFILETYPE type,		/* IN:  Type of FITS file, either STANDARD or
				   non-standard */
   int		naxis,	/* IN:  Number of axes (FITS NAXIS keyword) */
   DEFDIRENUM   FITSdef,	/* IN:  Which default file specs to apply */
   FILE        *outPipePtr,   /* IN:  If != NULL then a pipe to write to */
   int          writetape     /* IN:  If != 0, fork/exec off a "dd" to pipe data to */
	   ) {
	FILE* fid;
	int i,j;
	char zero;
	printf("fake shRegWriteAsFits(%s)\n", fn);

	if (reg->type != TYPE_U16) {
		printf("fake shRegWriteAsFits(%s) only writes U16s\n", fn);
		return -1;
	}
	if (outPipePtr) {
		printf("fake shRegWriteAsFits(%s) only writes to files (not pipes)\n", fn);
		return -1;
	}
	// ACK, I can't believe I'm writing this..... someone slap my wrist!
	fid = fopen(fn, "wb");
	assert(naxis == 2);
	i = fprintf(fid,
				"SIMPLE  =                    T / Standard FITS file                             "
				"BITPIX  =                   16 / U16 pixels                                     "
				"NAXIS   =                    2 /                                                "
				"NAXIS1  =                 % 4i /                                                "
				"NAXIS2  =                 % 4i /                                                "
				"EXTEND  =                    T / There may be FITS ext                          "
				"END                                                                             ",
				reg->nrow, reg->ncol);
	// HACK -- pad
	for (; i<2880; i++)
		fprintf(fid, " ");

	for (j=0; j<reg->nrow; j++) {
		for (i=0; i<reg->ncol; i++) {
			U16 pix = reg->rows_u16[j][i];
			// big-endian
			pix = htons(pix);
			fwrite(&pix, 2, 1, fid);
		}
	}
	zero = '\0';
	for (i=(reg->nrow * reg->ncol * 2) % 2880; i<2880; i++)
		fwrite(&zero, 1, 1, fid);

	fclose(fid);

	return 0;
}

