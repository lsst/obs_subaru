#include <stdio.h>
#include "shCFitsIo.h"

RET_CODE shRegWriteAsFits
   (
   REGION      *a_reg,		/* IN:  Name of region */
   char        *a_file,		/* IN:  Name of FITS file */
   FITSFILETYPE a_type,		/* IN:  Type of FITS file, either STANDARD or
				   non-standard */
   int		a_naxis,	/* IN:  Number of axes (FITS NAXIS keyword) */
   DEFDIRENUM   a_FITSdef,	/* IN:  Which default file specs to apply */
   FILE        *a_outPipePtr,   /* IN:  If != NULL then a pipe to write to */
   int          a_writetape     /* IN:  If != 0, fork/exec off a "dd" to pipe data to */
	   ) {
	printf("fake shRegWriteAsFits(%s)\n", a_file);
	return 0;
}

