#include <math.h>
#include "phFake.h"

#include <stdarg.h>
void trace(const char* fmt, ...) {
    va_list lst;
	printf("%s:%i ", __FILE__, __LINE__);
    va_start(lst, fmt);
	vprintf(fmt, lst);
    va_end(lst);
}

int
phFitCellAsPsfFake(OBJC *objc,		/* Object to fit */
	       int color,		/* color of object */
	       const CELL_STATS *stats_obj, /* cell array */
	       const FIELDPARAMS *fiparams, /* describe field */
	       int nannuli,		/* number of annuli to use */
	       int sky_noise_only,	/* only consider sky noise */
	       float *I0,		/* central intensity; can be NULL */
	       float *bkgd)		/* the sky level; can be NULL */
{
	printf("faking phFitCellAsPsf.\n");
	return 0;
}


/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Make an OBJC's children, returning the number created (may be 0)
 *
 * Once an OBJC has been through phObjcMakeChildren() it has a non-NULL
 * children field which points at one of its children; in addition its
 * OBJECT1_BLENDED bit is set, and nchild is one or more.
 *
 * Each child has a non-NULL parent field which points to its parent, and may
 * have a sibbs field too. It has its OBJECT1_CHILD bit set.
 */
int
phObjcMakeChildrenFake(OBJC *objc,		/* give this OBJC a family */
					   const FIELDPARAMS *fiparams) /* misc. parameters */
{
	const PEAK *cpeak;			/* == objc->peaks->peaks[] */
	int i, j;
	int nchild;				/* number of children created */
	PEAK *peak;				/* a PEAK in the OBJC */
	float min_peak_spacing;

	shAssert(objc != NULL && objc->peaks != NULL && fiparams != NULL);
	min_peak_spacing = fiparams->deblend_min_peak_spacing;
	nchild = objc->peaks->npeak;
	/*
	 * Done with moving objects. See if any of the surviving peaks are
	 * too close
	 */
	trace("objc->peaks->npeak %i\n", objc->peaks->npeak);
	for(i = 0; i < objc->peaks->npeak; i++) {
		PEAK *const peak_i = objc->peaks->peaks[i];
		PEAK *peak_j;
		float rowc_i, colc_i;		/* == peak_i->{col,row}c */
		float rowcErr_i, colcErr_i;	/* == peak_i->{col,row}cErr */
		float rowc_j, colc_j;		/* == peak_j->{col,row}c */
		float rowcErr_j, colcErr_j;	/* == peak_j->{col,row}cErr */

		if(peak_i == NULL) {
			continue;
		}
		shAssert(peak_i->flags & PEAK_CANONICAL);
      
		rowc_i = peak_i->rowc;
		colc_i = peak_i->colc;
		rowcErr_i = peak_i->rowcErr;
		colcErr_i = peak_i->colcErr;
		for(j = i + 1; j < objc->peaks->npeak; j++) {
			if(objc->peaks->peaks[j] == NULL) {
				continue;
			}

			peak_j = objc->peaks->peaks[j];
			rowc_j = peak_j->rowc;
			colc_j = peak_j->colc;
			rowcErr_j = peak_j->rowcErr;
			colcErr_j = peak_j->colcErr;
			if(pow(fabs(rowc_i - rowc_j) - rowcErr_i - rowcErr_j, 2) +
			   pow(fabs(colc_i - colc_j) - colcErr_i - colcErr_j, 2) <
			   min_peak_spacing*min_peak_spacing) {
				objc->flags2 |= OBJECT2_PEAKS_TOO_CLOSE;
				/*
				 * If the two peaks are in the same band, delete peak_j otherwise add
				 * it to peak_i's ->next list.  If there's already a peak on the ->next
				 * list in the same band, average their positions
				 */
				phMergePeaks(peak_i, peak_j);

				objc->peaks->peaks[j] = NULL;
				nchild--;

				i--;			/* reconsider the i'th peak */
				break;
			}
		}
	}
	/*
	 * We demand that the children are detected in at least deblend_min_detect
	 * bands; reject peaks that fail this test
	 */
	trace("objc->peaks->npeak %i\n", objc->peaks->npeak);
	for(i = 0; i < objc->peaks->npeak; i++) {
		int n;				/* number of detections */

		if(objc->peaks->peaks[i] == NULL) {
			continue;
		}

		for(n = 0, peak = objc->peaks->peaks[i]; peak != NULL;
			peak = (PEAK *)peak->next) {
			n++;
		}
		if(n < fiparams->deblend_min_detect) {
			objc->flags2 |= OBJECT2_TOO_FEW_DETECTIONS;
	 
			phPeakDel(objc->peaks->peaks[i]);
			objc->peaks->peaks[i] = NULL;
			nchild--;
		}
	}
	/*
	 * condense the peaks list
	 */
	trace("objc->peaks->npeak %i\n", objc->peaks->npeak);
	if(nchild != objc->peaks->npeak) {
		for(i = j = 0; i < objc->peaks->npeak; i++) {
			if(objc->peaks->peaks[i] != NULL) {
				objc->peaks->peaks[j++] = objc->peaks->peaks[i];
			}
		}
		shAssert(j == nchild);
		for(i = nchild; i < objc->peaks->npeak; i++) {
			objc->peaks->peaks[i] = NULL;	/* it's been moved down */
		}
		phPeaksRealloc(objc->peaks, nchild);
	}
	/*
	 * and create the desired children
	 */
	trace("objc->peaks->npeak %i\n", objc->peaks->npeak);
	if(objc->peaks->npeak > fiparams->nchild_max) { /* are there too many? */
		objc->flags |= OBJECT1_DEBLEND_TOO_MANY_PEAKS;
		phPeaksRealloc(objc->peaks, fiparams->nchild_max);
	}

	trace("objc->peaks->npeak %i\n", objc->peaks->npeak);
	for(i = 0;i < objc->peaks->npeak;i++) { /* create children */
		cpeak = objc->peaks->peaks[i];
		(void)phObjcChildNew(objc, cpeak, fiparams, 1);
	}

	return(objc->peaks->npeak + ((objc->sibbs != NULL) ? 1 : 0));
}
