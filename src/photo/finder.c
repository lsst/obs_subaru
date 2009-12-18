#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <alloca.h>
#include "dervish.h"
#include "phSkyUtils.h"
#include "phPeaks.h"
#include "phConsts.h"

/*
 * Maintain a special freelist for PEAKs
 */
static PEAK *peak_freelist = NULL;

/*
 * <AUTO EXTRACT>
 *
 * destroy a PEAKS
 */
void
phPeaksDel(PEAKS *peaks)
{
   int i;

   if(peaks == NULL) return;

   if(p_shMemRefCntrGet(peaks) > 0) {	/* still referenced somewhere */
      p_shMemRefCntrDecr(peaks);
      return;
   }

   if(peaks->peaks != NULL) {
      for(i = 0;i < peaks->size;i++) {
	 phPeakDel(peaks->peaks[i]);
      }
      shFree(peaks->peaks);
   }
   shFree(peaks);
}

/*
 * <AUTO EXTRACT>
 *
 * Destroy a PEAK, which must _not_ have its PEAK_DANGLING bit set.
 *
 * The PEAK->next chain is examined for PEAKs with the PEAK_DANGLING bit set;
 * they are also destroyed.
 */
void
phPeakDel(PEAK *peak)
{
   PEAK *next;

   if(peak == NULL) return;

   if(p_shMemRefCntrGet(peak) > 0) {	/* still referenced somewhere */
      p_shMemRefCntrDecr(peak);
      return;
   }
   
   shAssert(!(peak->flags & PEAK_DANGLING));

   next = (PEAK *)peak->next;
   while(next != NULL) {
      PEAK *tmp = (PEAK *)next->next;
      if(next->flags & PEAK_DANGLING) {
	 next->flags &= ~PEAK_DANGLING;	/* turn off before calling phPeakDel */
	 next->next = NULL;
	 phPeakDel(next);
      }
      next = tmp;
   }
   
   peak->next = peak_freelist;
   peak->flags |= PEAK_FREE;
   peak_freelist = peak;
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Make peak1 a copy of peak2, returning peak1
 */
PEAK *
phPeakCopy(PEAK *peak1, const PEAK *peak2)
{
   const int id1 = peak1->id;
   memcpy(peak1, peak2, sizeof(PEAK));
   *(int *)&peak1->id = id1;		/* cast away const */

   return(peak1);
}

/*****************************************************************************/
/*
 * Workhorse for phPeaksRealloc/phPeaksRenew
 */
static void
realloc_peaks(PEAKS *peaks,		/* PEAKS to expand/contract */
	      int n,			/* desired number of peaks */
	      int keep_next)		/* should we keep the ->next chains? */
{
   int i;

   shAssert(peaks != NULL);

   if(peaks->size == 0) {
      if(peaks->peaks == NULL) {
	 peaks->peaks = shMalloc(n*sizeof(PEAK *));
      } else {
	 peaks->peaks = shRealloc(peaks->peaks, n*sizeof(PEAK *));
      }
   } else {
      /* free unwanted PEAKs, if any */
      if(keep_next) {
	 for(i = n;i < peaks->size;i++) {
	    phPeakDel(peaks->peaks[i]);
	 }
      } else {
	 for(i = 0;i < peaks->size;i++) {
	    if(i < n) {
	       shAssert(peaks->peaks[i] != NULL);
	       if(peaks->peaks[i]->next != NULL) {
		  ((PEAK *)(peaks->peaks[i]->next))->flags &= ~PEAK_DANGLING;
		  phPeakDel((PEAK *)peaks->peaks[i]->next);
		  peaks->peaks[i]->next = NULL;
	       }
	    } else {
	       phPeakDel(peaks->peaks[i]);
	    }
	 }
      }

      if(n == 0) {
	 shFree(peaks->peaks); peaks->peaks = NULL; peaks->size = 0;
      } else {
	 peaks->peaks = shRealloc(peaks->peaks,n*sizeof(PEAK *));
      }
   }
   
   for(i = peaks->size;i < n;i++) {	/* allocate new ones */
      peaks->peaks[i] = phPeakNew();
   }
   
   peaks->size = n;

   if(peaks->npeak > n) {
      peaks->npeak = n;			/* we freed the rest */
   }
}

/*
 * <AUTO EXTRACT>
 * De/Increase the number of PEAKs in a PEAKS
 *
 * If npeak < size, we must ensure that any ->next chains are deleted in
 * PEAKs that are to be reused.
 *
 * Note that you should only use this function if you want to keep (some
 * of) the old peaks. If you want to say, "Now make this PEAKS contain n
 * PEAKs for me to set at my whim", use phPeaksRenew(). The difference is
 * that this function preserves ->next chains, while phPeaksRenew frees them
 */
void
phPeaksRealloc(PEAKS *peaks,		/* PEAKS to expand/contract */
	       int n)			/* desired number of peaks */
{
   realloc_peaks(peaks, n, 1);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Create a new PEAK
 */
PEAK *
phPeakNew(void)
{
   static int id = 0;			/* unique ID for a peak */
   PEAK *peak;

// dstn
/*
   if(peak_freelist != NULL) {
      peak = peak_freelist;
      peak_freelist = (PEAK *)peak_freelist->next;
   } else {
      peak = shMalloc(sizeof(PEAK));
 }
 */
   peak = shMalloc(sizeof(PEAK));
  
   *(int *)&peak->id = id++;		/* cast away const */
   peak->catID = 0;
   peak->flags = PEAK_NOFLAGS;
   peak->peak = VALUE_IS_BAD;
   peak->rpeak = peak->cpeak = VALUE_IS_BAD;
   peak->rowc = peak->colc = VALUE_IS_BAD;
   peak->rowcErr = peak->colcErr = VALUE_IS_BAD;
   peak->next = NULL;

   return(peak);
}

/*****************************************************************************/
/*
 * Return the band that a peak was found in
 */
int
phPeakBand(const PEAK *peak)
{
   int c;
   
   switch (peak->flags & PEAK_BANDMASK) {
    case PEAK_BAND0: c = 0; break;
    case PEAK_BAND1: c = 1; break;
    case PEAK_BAND2: c = 2; break;
    case PEAK_BAND3: c = 3; break;
    case PEAK_BAND4: c = 4; break;
    default:
      shFatal("phPeakBand: "
	      "Impossible value of peak->flags & BANDMASK: 0x%x",
	      peak->flags & PEAK_BANDMASK);
      return(0);			/* NOTREACHED */
   }
   
   return(c);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Create a new PEAKS
 */
PEAKS *
phPeaksNew(int n)			/* initial number of PEAKs */
{
   int i;
   
   PEAKS *peaks = shMalloc(sizeof(PEAKS));

   peaks->size = n;
   if(n == 0) {
      peaks->peaks = NULL;
   } else {
      peaks->peaks = shMalloc(peaks->size*sizeof(PEAK *));
      for(i = 0;i < peaks->size;i++) {
	 peaks->peaks[i] = phPeakNew();
      }
   }
   peaks->npeak = 0;

   return(peaks);
}

