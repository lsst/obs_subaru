#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <alloca.h>
#include "dervish.h"
#include "phSkyUtils.h"
#include "phPeaks.h"

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
