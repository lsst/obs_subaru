/*
 * <INTRO>
 *
 * Merge Colors works on the object list ONLY, and does not use any frames
 * or their masks.  It leaves the objects in an unsettled state, with a
 * master mask but with no OBJECT1 regions or masks at all. The positions of
 * each OBJECT1 are converted to the canonical coordinate system (usually r')
 */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <phSignals.h>
#include "dervish.h"
#include "phMaskbittype.h"
#include "phMergeColors.h"
#include "phPeaks.h"
#include "phObjc.h"
#include "phMeasureObj.h"
#include "phUtils.h"

static void clean_peaks(PEAKS *peaks);

static int
do_next_color(int color, CHAIN *objects,
	      const FIELDPARAMS *fiparams, CHAIN *merged);

static OBJC *
make_new_objc(OBJECT1 *obj, int ncolor, int color, float drow, float dcol);

static void
merge_obj_into_objc(OBJC *objc, OBJECT1 *obj, const FIELDPARAMS *fiparams, int color,
		    float drow, float dcol, float eps);

static void
merge_objcs(OBJC *objc1, OBJC *objc2, const FIELDPARAMS *fiparams, float eps);

static void
merge_families(OBJC *objc1, OBJC *objc2);

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * the Initialization function of the Merge Colors module. 
 * It reads information from parameter files and allocates any needed
 * memory.
 *
 * return: SH_SUCCESS          always
 */
   
RET_CODE
phInitMergeColors(
		  int ncolor		/* number of colors of images */
		  )
{
   ncolor = ncolor;

   return(SH_SUCCESS);
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * the Fini function of the Merge Colors module.  De-allocates any
 * memory we've allocated.
 *
 * return: SH_SUCCESS          always
 */
RET_CODE
phFiniMergeColors(void)
{
   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * 
 * the main function of the Merge Colors module.  It creates OBJCs from
 * lists of one-color OBJECT1s, places the OBJECT1s into the OBJCs,
 * and creates new OBJECT1s in colors in which objects weren't detected.
 * Also, it creates the 'master_mask', showing the extent of each
 * object in the union of all colors.
 *
 * If cut_atlas_images is true, and val is >= 0, the pixels in the atlas
 * images will have their values replaced by val after the atlas images
 * are extracted. If sigma is greater than zero, the values will have
 * N(0,sigma^2) noise added. In this case, rand must be non-NULL
 *
 * output: merged              a CHAIN of OBJCs, containing merged OBJECT1s
 * 
 * Note that the input "objects" chains will be depopulated and emptied
 * by this function -- their objects will be transferred to the output
 * "merged" chain.
 *
 * return: SH_SUCCESS          always
 *
 */
RET_CODE
phMergeColors(CHAIN **objects,           /* I: ncolor CHAINs of OBJECT1s,
					    objects found in each frame
					    separately */
	      const FIELDPARAMS *fparams, /* properties of field */
	      CHAIN *merged,		/* O: a CHAIN of OBJCs, containing
					   merged OBJECT1s */
	      int cut_atlas_images,	/* cut atlas images? */
	      int no_restore_ai,	/* don't restore old atlas images */
	      int val,			/* value to replace atlas images by
					 (if < 0 && != -1, use SOFT_BIAS + FRAMEPARAMS.bkgd) */
	      float sigma,		/* val's standard deviation */
	      RANDOM *rand)		/* random numbers for sigma */
{
   CURSOR_T crsr;
   int c;
   int ncolor;
   OBJC *objc;

   shAssert(objects != NULL);
   shAssert(fparams != NULL);
   ncolor = fparams->ncolor;
   for(c = 0; c < ncolor; c++) {
      shAssert(objects[c]->type == shTypeGetFromName("OBJECT1"));
      shAssert(fparams->frame[c].data != NULL);
      shAssert(fparams->frame[c].toGCC != NULL);
   }
   shAssert(merged != NULL);
   shAssert(merged->type == shTypeGetFromName("OBJC"));
/*
 * If we are cutting atlas images, and if there are old ones, restore them
 * before cutting new ones. This is important if we are e.g. merging binned
 * detections with a faint ones, as the objects are removed before binning
 */
   if(cut_atlas_images && !no_restore_ai && merged != NULL) {
      crsr = shChainCursorNew(merged);
      while((objc = shChainWalk(merged, crsr, NEXT)) != NULL) {
	 if(objc->aimage == NULL) {
	    objc->aimage = phAtlasImageNew(objc->ncolor);
	 }
	 for(c = 0; c < ncolor;c++) {
	    if(objc->aimage->pix[c] != NULL) {
	       phRegionSetFromAtlasImage(objc->aimage, c,
					 (REGION *)fparams->frame[c].data, 0, 0, 0, '\0', 0);
	    }
	 }
      }
      shChainCursorDel(merged, crsr);
   }
   /*
    * Grow to superpixel boundaries before merging.  If we don't do this,
    * then we end up with overlapping atlas images, which causes problems
    * when we cut them (PR 7296)
    */
   if(cut_atlas_images) {
       crsr = shChainCursorNew(merged);
       while((objc = shChainWalk(merged, crsr, NEXT)) != NULL) {
	   OBJMASK *grown = phObjmaskGrowToSuperpixel(objc->aimage->master_mask,
						      fparams->binfac);
	   shAssert(sigma < 0 || rand != NULL);
	   
	   phAtlasImageDel(objc->aimage, 1);
	   objc->aimage = phAtlasImageNew(objc->ncolor);
	   objc->aimage->master_mask = grown;
       }
       shChainCursorDel(merged, crsr);
   }
/* 
 * Go through all the colors, merging each into the "merged"
 * list of OBJCs as we go.
 */
   for(c = 0; c < ncolor; c++) {
      if(do_next_color(c, objects[c], fparams, merged) < 0) {
	 return(SH_GENERIC_ERROR);
      }
   }
/*
 * Set the peak pixel in each band.
 *
 * We also see if the object's pixels have certain bits set, and set
 * flag bits if so (the mask will end up in the atlas image, and the flags
 * in the main output file so this is a Good Thing to do). These flags
 * are also used in finding the canonical centre
 */
   crsr = shChainCursorNew(merged);
   while((objc = shChainWalk(merged, crsr, NEXT)) != NULL) {
      phObjcCenterCalc(objc, fparams, 0); /* find canonical centre */

      phObjcPeaksSetFromObject1s(objc, fparams);
/*
 * If requested, replace the detected pixels with the soft bias; we save their
 * true values in the atlas image
 */
      if(cut_atlas_images) {
	 phAtlasImageCut(objc, -1, fparams, val, sigma, rand);
      }
   }
   shChainCursorDel(merged, crsr);
      
   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * merge the PEAK lists in the individual OBJECT1s into the OBJC's list,
 * after setting the bit in each PEAK->flag that tells us whence it comes
 */
void
phObjcPeaksSetFromObject1s(OBJC *objc,	/* the object in question */
			   const FIELDPARAMS *fiparams) /*properties of field*/
{
   int c, i;
   float drow, dcol;			/* convert to reference colour */
   int npeak;				/* number of peaks in OBJC */
   OBJECT1 *obj1;			/* == objc->color[c] */

   shAssert(fiparams != NULL);

   npeak = 0;
   for(c = 0;c < objc->ncolor;c++) {
      if((obj1 = objc->color[c]) == NULL || obj1->peaks == NULL) {
	 continue;
      }

      if(obj1->peaks->npeak > npeak) {
	 npeak = obj1->peaks->npeak;
      }
      for(i = 0;i < obj1->peaks->npeak;i++) { /* set PEAK_BAND[0-4]*/
	 obj1->peaks->peaks[i]->flags |= (1 << c);
      }
   }

   if(objc->peaks == NULL) {
      objc->peaks = phPeaksNew(npeak + 2);
   }
   
   for(c = 0;c < objc->ncolor;c++) {
      if((obj1 = objc->color[c]) == NULL || obj1->peaks == NULL) {
	 continue;
      }
      
      phOffsetDo(fiparams, objc->rowc, objc->colc, 
		 c, fiparams->ref_band_index,
		 0, NULL, NULL, &drow, NULL, &dcol, NULL);
      (void)phPeaksMerge(objc->peaks, obj1->peaks, drow, dcol,
			 fiparams->frame[c].peak_min_sep, 1, PEAK_IN_BRIGHT);
   }
/*
 * all coordinates in OBJC->peaks are now in the reference coordinate system
 */
   for(i = 0;i < objc->peaks->npeak;i++) {
      PEAK *peak = objc->peaks->peaks[i];
      for(;peak != NULL;peak = (PEAK *)peak->next) {
	 peak->flags |= PEAK_CANONICAL;
      }
   }
/*
 * Check if PEAKs are in the same band and very close; if so we are
 * probably merging bright and faint detections of the same peak,
 * and we needn't keep both
 */
   clean_peaks(objc->peaks);

   objc->flags &= ~OBJECT1_BLENDED;	/* may no longer be true */
   for(c = 0;c < objc->ncolor;c++) {
      if((obj1 = objc->color[c]) != NULL) {
	 obj1->flags &= ~OBJECT1_BLENDED;
      }
   }
   
   if(objc->peaks->npeak > 1) {
      objc->flags |= OBJECT1_BLENDED;
   }
}   

/*****************************************************************************/
/*
 * given a chain of OBJECT1s in color 'color', and a set of TRANS in
 * an FIELDPARAMS, take each OBJECT1 and find the offset into the 'standard'
 * coord system for its origin.
 *
 * Then, for each OBJC in the 'merged' chain, see if its master_mask overlaps
 * with this OBJECT1.
 *
 * If it does, merge the OBJECT1 with the OBJC's object in the same color,
 * otherwise simply make the OBJC point to the OBJECT1.
 *
 * If it belongs in no OBJC, make a new one, transfer the OBJECT1 to the
 * new OBJC, and place the OBJC on the 'merged' chain.
 *
 * Finally create or update the OBJC's master_mask from the OBJECT1's mask
 * 
 * Note that the 'objects' chain will be emptied by this function.
 */
static int
do_next_color(int color,		/* color under consideration */
	      CHAIN *objects,		/* chain of OBJECT1s */
	      const FIELDPARAMS *fiparams, /* e.g. convert coords */
              CHAIN *merged)		/* chain of merged OBJCs */
{
   CURSOR_T crsr;
   float drow, dcol;
   float eps;   
   OBJMASK *mmask;			/* `master mask' for new OBJECT1,
					   in reference coordinate frame */
   OBJECT1 *obj;
   OBJC *objc;
   OBJC *objs_objc;			/* the OBJC containing the OBJECT1
					   that we are trying to merge */

   shAssert(fiparams != NULL);
   shAssert(color >= 0 && color < fiparams->ncolor);

   eps = fiparams->frame[color].peak_min_sep;
   crsr = shChainCursorNew(merged);

   while (phTimeLeft >= 0 &&
	  (obj = shChainElementRemByPos(objects, HEAD)) != NULL) {
      int rowc = obj->rowc, colc = obj->colc;
      shAssert(obj->mask != NULL);
      phPeaksSort(obj->peaks);
      
      if(obj->peaks != NULL && obj->peaks->npeak > 0) {
	 const PEAK *peak = obj->peaks->peaks[0];
	 rowc = peak->rowc; colc = peak->colc;
      }
      
      phOffsetDo(fiparams, rowc, colc,
		 color, fiparams->ref_band_index,
		 0, NULL, NULL, &drow, NULL, &dcol, NULL);

      objs_objc = NULL;			/* we don't yet know where obj goes */
      mmask = phObjmaskCopy(obj->mask, 0, 0);

      shChainCursorSet(merged, crsr, HEAD);
      while((objc = (OBJC *)shChainWalk(merged, crsr, NEXT)) != NULL) {
	 shAssert(objc->aimage->master_mask != NULL);

	 if(phObjmaskIntersect(objc->aimage->master_mask, mmask, drow, dcol)) {
	    if(objs_objc == NULL) {
	       merge_obj_into_objc(objc, obj, fiparams, color, drow, dcol, eps);
	       objs_objc = objc;
	    } else {
	       /* 
	        * here, two OBJCs in the chain could hold the current OBJECT1.
	        * we'll merge the two, and delete the current OBJC, leaving
	        * only a single OBJC to represent this OBJECT1.
	        */
	       {
		  OBJC *foo = shChainElementRemByCursor(merged, crsr);
		  shAssert(foo == objc);
	       }

	       if(objc->flags3 & OBJECT3_MEASURE_BRIGHT) { /* keep objc */
		  int i;
		  int nel = shChainSize(merged);

		  merge_objcs(objc, objs_objc, fiparams, eps);

		  for(i = 0; i < nel; i++) {
		     if(shChainElementGetByPos(merged, i) == objs_objc) {
			shChainElementChangeByPos(merged, i, objc);
			break;
		     }
		  }
		  shAssert(i < nel);	/* we found objs_objc */

		  phObjcDel(objs_objc, 0);
		  objs_objc = objc;
	       } else {			/* keep objs_obj */
		  merge_objcs(objs_objc, objc, fiparams, eps);
		  phObjcDel(objc, 0);
	       }
	    }
	 }
      }
      phObjmaskDel(mmask);

      /*
       * if the current 'obj' is _still_ not part of an OBJC, then create
       * a new OBJC, cause it to point to this 'obj', and add it to the
       * chain of merged OBJCs.
       */
      if(objs_objc == NULL) {
	 objs_objc = make_new_objc(obj, fiparams->ncolor, color, drow, dcol);
	 shChainElementAddByPos(merged, objs_objc, "OBJC", TAIL, AFTER);
      }

   }
   /*
    * It can happen (as shown by PR 7600) that we can have objects without
    * any PEAKs.  This is possible if a star is subtracted from a frame,
    * resulting in a new peak within the BRIGHT mask.  These objects
    * are spurious, and cause assertion failures later.
    */
   shChainCursorSet(merged, crsr, HEAD);
   while((objc = (OBJC *)shChainWalk(merged, crsr, NEXT)) != NULL) {
       int i;
       for (i = 0; i < objc->ncolor; i++) {
	   if (objc->color[i] && objc->color[i]->peaks && objc->color[i]->peaks->npeak > 0) {
	       break;
	   }
       }
       if (i == objc->ncolor) {		/* no PEAKs in any band */
	   (void)shChainElementRemByCursor(merged, crsr);
	   phObjcDel(objc, 1);
       }
   }

   shChainCursorDel(merged, crsr);

   return(phTimeLeft < 0 ? -1 : 0);
}

/*****************************************************************************/
/*
 * create a new OBJC to hold the given OBJECT1.  When creating the new
 * master_mask, be sure to shift the OBJECT1's mask origin by 
 * (drow, dcol).
 *
 * return: new OBJC structure        with master mask set and color'th 
 *                                   field of OBJC pointing to OBJECT1
 *					(always succeeds)
 */
static OBJC *
make_new_objc(OBJECT1 *obj,		/* OBJECT1 to put in new OBJC  */
	      int ncolor,		/* number of colours in OBJC */
	      int color,		/* which one this is */
	      float drow, float dcol)	/* shift OBJC's master mask by
					   this relative to obj's origin */
{
   OBJC *objc;

   objc = phObjcNew(ncolor);
   objc->color[color] = obj;
   objc->aimage->master_mask = phObjmaskCopy(obj->mask, drow, dcol);
   objc->flags3 |= (obj->flags3 & OBJECT3_MEASURE_BRIGHT);

   return(objc);
}

/*****************************************************************************/
/*
 * condense a PEAKS, which may contain NULLs, so that all the
 * non-NULL elements are contiguous
 */
static void
condense_peaks(PEAKS *peaks)
{
   int i, j;
   
   for(i = j = 0;i < peaks->size;i++) {
      if(peaks->peaks[i] != NULL) {
	 peaks->peaks[j] = peaks->peaks[i];
	 if(j < i) peaks->peaks[i] = NULL;
	 j++;
      }
   }
   peaks->size = j;
}

/*****************************************************************************/
/*
 * first a struct used in the merging
 */
struct opeak {				/* "One PEAK" */
   int i;				/* initial index into merged[] */
   int first;				/* does PEAK belong to first list? */
   PEAK *peak;				/* the PEAK */
   float row, col;			/* position of peak */
   int flags;				/* flags associated with merged peak */
};

/*
 * comparison function for qsort(); sort into increasing row order
 */
static int
compar_row(const void *a, const void *b)
{
   const struct opeak *pa = *(struct opeak **)a;
   const struct opeak *pb = *(struct opeak **)b;

   return(pa->row < pb->row ? -1 : (pa->row == pb->row ? 0 : 1));
}

/*
 * <AUTO EXTRACT>
 *
 * Given two lists of PEAKs, merge them into peak1's list. If two peaks
 * have the same centre, only keep one copy (either we are merging
 * two colours, or the same object was detected as both bright and faint)
 *
 * If a bit in bitmask is set, destroy the peak. This is typically used to
 * remove PEAK_IN_BRIGHT peaks as they were found within an area that had
 * already been searched for peaks.
 *
 * peak2's list is destroyed in the process unless copy is true, in which
 * case it is not touched.
 *
 * If copy is true, then peaks in peaks2 that are merged into peaks1 will
 * be linked into the peaks1->peaks[]->next lists
 *
 * We assume that the lists are sorted in order of decreasing peak value,
 * and we preserve this in the merged list
 *
 * Return the number of peaks in the merged list
 */
int
phPeaksMerge(PEAKS *peaks1,		/* the peaks */
	     PEAKS *peaks2,		/*           in question */
	     float drow,		/* offset of peaks1 wrt peaks2, */
	     float dcol,		/*  so (0,0) in 2 is (dr, dc) in 1 */
	     float eps,			/* if peaks of objects are closer
					   than this, they are the same */
	     int copy,			/* copy peaks2? */
	     int bitmask)		/* destroy peaks with these bits set */
{
   float d2;				/* distance^2 between two peaks */
   int i1, i2;				/* counters in peaks[12] */
   int i, j;
   int npeak;
   int npeak1, npeak2;			/* == peaks[12]->npeak */
   struct opeak **merged;		/* merged PEAK list */
   struct opeak *merged_s;		/* space for merged[] */
   float row2, col2;			/* peak2's centre, transformed to
					   peak1's coordinate system */
   PEAK *peak1, *peak2;			/* peaks from peaks[12] */
   PEAK *tmp;				/* used in following linked lists */

   shAssert(peaks1 != NULL && peaks2 != NULL);
   
#if 0					/* useful for debugging */
   for(j = 0; j < peaks1->size; j++) {
      if(peaks1->peaks[j] != NULL) {
	 peaks1->peaks[j]->flags |= 0x100000;
      }
   }
   for(j = 0; j < peaks2->size; j++) {
      if(peaks2->peaks[j] != NULL) {
	 peaks2->peaks[j]->flags |= 0x200000;
      }
   }
#endif
/*
 * check that input chains are sorted
 */
#if 0 && !defined(NDEBUG)
   for(i1 = 0;i1 < peaks1->npeak - 1;i1++) {
      shAssert(peaks1->peaks[i1]->peak >= peaks1->peaks[i1 + 1]->peak);
   }
   for(i2 = 0;i2 < peaks2->npeak - 1;i2++) {
      shAssert(peaks2->peaks[i2]->peak >= peaks2->peaks[i2 + 1]->peak);
   }
#endif
/*
 * Destroy peaks with bits set in bitmask
 */
   npeak1 = peaks1->npeak;
   for(i = 0;i < peaks1->npeak;i++) {
      if(peaks1->peaks[i]->flags & bitmask) {
	 phPeakDel(peaks1->peaks[i]); peaks1->peaks[i] = NULL;
	 npeak1--;
      }
   }
   peaks1->npeak = npeak1;
   condense_peaks(peaks1);
   
   npeak2 = peaks2->npeak;
   for(i = 0;i < peaks2->npeak;i++) {
      if(peaks2->peaks[i]->flags & bitmask) {
	 phPeakDel(peaks2->peaks[i]); peaks2->peaks[i] = NULL;
	 npeak2--;
      }
   }
   peaks2->npeak = npeak2;
   condense_peaks(peaks2);
/*
 * OK, all undesirable peaks are gone. Merge the lists
 */
   if(npeak1 == 0) {
      if(copy) {
	 phPeaksRenew(peaks1, npeak2);
	 for(j = 0; j < npeak2; j++) {
	    (void)phPeakCopy(peaks1->peaks[j], peaks2->peaks[j]);
	 }
	 peaks1->npeak = npeak2;
      } else {
	 phPeaksRenew(peaks1,0);
	 
	 *peaks1 = *peaks2;
	 peaks2->size = 0; peaks2->peaks = NULL; /* they're all in peaks1 */
      }
      for(j = 0; j < npeak2; j++) {
	 peaks1->peaks[j]->rowc += drow;
	 peaks1->peaks[j]->colc += dcol;

	 if(drow != 0.0 || dcol != 0.0) {
	    peaks1->peaks[j]->flags |= PEAK_CANONICAL;
	 }
      }
      return(peaks1->npeak);
   } else if(npeak2 == 0) {
      if(!copy) {
	 phPeaksRenew(peaks2,0);
      }
      return(peaks1->npeak);
   }
/*
 * merge together the two lists into merged[];
 */
   merged = alloca((npeak1 + npeak2)*sizeof(struct opeak *));
   merged_s = alloca((npeak1 + npeak2)*sizeof(struct opeak));
   for(i = 0;i < npeak1 + npeak2;i++) {
      merged[i] = &merged_s[i];
      merged[i]->i = i;
      merged[i]->flags = 0;
   }

   i1 = 0; peak1 = peaks1->peaks[i1];
   i2 = 0; peak2 = peaks2->peaks[i2];
   for(npeak = 0;;npeak++) {
      if(i1 == npeak1) {		/* all the rest are from peaks2 */
	 for(j = 0;j < npeak2 - i2;j++) {
	    merged[npeak + j]->peak = peaks2->peaks[i2 + j];

	    if(copy) {
	       merged[npeak + j]->peak =
				    phPeakNewFromPeak(merged[npeak + j]->peak);
	    }

	    merged[npeak + j]->peak->rowc += drow;
	    merged[npeak + j]->peak->colc += dcol;
	    if(drow != 0.0 || dcol != 0.0) {
	       merged[npeak + j]->peak->flags |= PEAK_CANONICAL;
	    }
	    
	    merged[npeak + j]->first = 0;
	    merged[npeak + j]->row = merged[npeak + j]->peak->rowc;
	    merged[npeak + j]->col = merged[npeak + j]->peak->colc;
	 }
	 npeak += (npeak2 - i2);

	 break;
      } else if(i2 == npeak2) {		/* all the rest are from peaks1 */
	 for(j = 0;j < npeak1 - i1;j++) {
	    merged[npeak + j]->first = 1;
	    merged[npeak + j]->peak = peaks1->peaks[i1 + j];
	    merged[npeak + j]->row = merged[npeak + j]->peak->rowc;
	    merged[npeak + j]->col = merged[npeak + j]->peak->colc;
	 }
	 npeak += (npeak1 - i1);
	 
	 break;
      }

      if(peak1->peak > peak2->peak) {
	 merged[npeak]->peak = peak1;

	 merged[npeak]->first = 1;
	 merged[npeak]->row = merged[npeak]->peak->rowc;
	 merged[npeak]->col = merged[npeak]->peak->colc;
	 
	 peak1 = peaks1->peaks[++i1];
      } else {
	 merged[npeak]->peak = peak2;

	 if(copy) {
	    merged[npeak]->peak = phPeakNewFromPeak(merged[npeak]->peak);
	 }

	 merged[npeak]->peak->rowc += drow;
	 merged[npeak]->peak->colc += dcol;
	 if(drow != 0.0 || dcol != 0.0) {
	    merged[npeak]->peak->flags |= PEAK_CANONICAL;
	 }
	 
	 merged[npeak]->first = 0;
	 merged[npeak]->row = merged[npeak]->peak->rowc;
	 merged[npeak]->col = merged[npeak]->peak->colc;

	 peak2 = peaks2->peaks[++i2];
      }
   }
   shAssert(npeak == npeak1 + npeak2);

   qsort(merged, npeak, sizeof(struct opeak *), compar_row);
/*
 * Now go through merged[] looking for objects from peaks2 that coincide
 * with objects from peaks1
 */
   for(i = 0;i < npeak;i++) {
      if(merged[i] == NULL || merged[i]->first) continue;

      peak2 = merged[i]->peak;
      col2 = merged[i]->col;
      row2 = merged[i]->row;
      
      for(j = i - 1; j >= 0;j--) {	/* look for start of possible matches*/
	 if(merged[j]->row < row2 - eps) break;	/* too far away to match */
      }
      j++;

      for(; j < npeak;j++) {		/* look up for matches with peaks1 */
	 if(merged[j]->row > row2 + eps) break;	/* too far away to match */

	 if(i == j ||			/* peak we're matching */
	    merged[j]->peak == NULL ||	/* matched and discarded */
	    !merged[j]->first) {	/* from the wrong list */
	    continue;
	 }
/*
 * see if they are close enough together to be merged.
 */
	 d2 = pow(merged[j]->row - row2,2) + pow(merged[j]->col - col2,2);
	 if(d2 < eps*eps) {
	    peak1 = merged[j]->peak;

	    if(peak1->peak > peak2->peak) { /* merge peak2 into peak1 */
	       if(merged[i]->peak == NULL) { /* already merged */
		  continue;
	       }

	       merged[i]->peak = NULL;
#if 0
	       if((peak1->flags & PEAK_IN_BRIGHT) &&
					    !(peak2->flags & PEAK_IN_BRIGHT)) {
		  peak1->flags &= ~PEAK_IN_BRIGHT;
	       }
#endif
	       if (peak2->catID != 0 && peak1->catID != peak2->catID) {	/* Peak may be a known object */
		   if (peak1->catID != 0) {
		       fprintf(stderr,"Replacing peak1's catID %d with %d (d = %.2g)\n",
			       peak1->catID, peak2->catID, sqrt(d2));
		   }
		   peak1->catID = peak2->catID;
	       }

	       if(copy) {
		  tmp = peak1;
		  while(tmp->next != NULL) tmp = (PEAK *)tmp->next;
		  tmp->next = peak2;
		  peak2->flags |= PEAK_DANGLING;
	       } else {
		  phPeakDel(peak2);
	       }
	       npeak2--;
	    } else {			/* merge peak1 into peak2 */
	       merged[j]->peak = NULL;
#if 0
	       if((peak2->flags & PEAK_IN_BRIGHT) &&
					    !(peak1->flags & PEAK_IN_BRIGHT)) {
		  peak2->flags &= ~PEAK_IN_BRIGHT;
	       }
#endif
	       if (peak1->catID != 0 && peak2->catID != peak1->catID) {	/* Peak may be a known object */
		   if (peak2->catID != 0) {
		       fprintf(stderr,"Replacing peak2's catID %d with %d (d = %.2g)\n",
			       peak2->catID, peak1->catID, sqrt(d2));
		   }
		   peak2->catID = peak1->catID;
	       }

	       if(copy) {
		  tmp = peak2;
		  while(tmp->next != NULL) tmp = (PEAK *)tmp->next;
		  tmp->next = peak1;
		  peak1->flags |= PEAK_DANGLING;
	       } else {
		  phPeakDel(peak1);
	       }
	       npeak1--;
	    }
	 }
      }
   }
/*
 * merged[]->peak now has NULLs for all the peaks which have been merged
 * with other peaks, and is in increasing row order. We know what index
 * the entries had in the original decreasing-peak sorted order, so revert
 * merged[] to that order
 */
   for(i = 0;i < npeak;i++) {
      if(merged[i]->i != i) {
	 struct opeak *tmp = merged[i];
	 merged[i] = merged[tmp->i];
	 merged[tmp->i] = tmp;
	 i--;				/* try again as merged[i] changed */
      }
   }
/*
 * Finally put all the surviving peaks into peaks1. If we are copying
 * peaks2, we have to create a PEAK to copy into
 */
   for(i = peaks1->npeak; i < peaks1->size;i++) {
      phPeakDel(peaks1->peaks[i]);
   }

   peaks1->peaks = shRealloc(peaks1->peaks, (npeak1 + npeak2)*sizeof(PEAK *));
   peaks1->size = peaks1->npeak = npeak1 + npeak2;

   if(!copy) {
      for(i = peaks2->npeak; i < peaks2->size;i++) {
	 phPeakDel(peaks2->peaks[i]);
      }
      shFree(peaks2->peaks); peaks2->peaks = NULL;
      peaks2->npeak = peaks2->size = 0;	/* we've copied them all to peaks1 */
   }

   j = 0;
   for(i = 0;i < npeak;i++) {
      if(merged[i]->peak != NULL) {
	 peaks1->peaks[j] = merged[i]->peak;
	 j++;
      }
   }
   shAssert(j <= peaks1->size);
/*
 * Check if PEAKs are in the same band and very close; if so we are
 * probably merging bright and faint detections of the same peak,
 * and we needn't keep both
 */
   clean_peaks(peaks1);
/*
 * check that they are still sorted
 */
#if !defined(NDEBUG)
   for(i1 = 0;i1 < peaks1->npeak - 1;i1++) {
      shAssert(peaks1->peaks[i1]->peak >= peaks1->peaks[i1 + 1]->peak);
   }
#endif
   
   return(peaks1->npeak);
}

/*****************************************************************************/
/*
 * Go through a PEAKS, removing multiple detections of the same PEAK.
 * This can occur when merging bright and faint detections of the
 * same object.
 *
 * The criterion for removal is that the reduced chi^2 of the separation
 * (in either direction) should be less than nsig^2
 *
 * Note that we keep the _first_ detection, which will be the bright one
 *
 * We also have to deal with multiple PEAKs in the same band on the ->next list
 * if the astrometry was bad enough that we were forced to use a very lenient
 * merging criterion.  We have to fix this now, as otherwise we'll be passing
 * on multiple peaks within a child to the deblender.
 *
 * Note that these multiple peaks are not believed to be real; if they were,
 * we wouldn't have merged them. We don't use the statistical errors in their
 * positions to decide whether to clean them, as they were merged based
 * on systematic errors
 */
static void
clean_different_peaks(PEAKS *peaks)
{
   float colc, rowc;			/* == peak->{col,row}c */
   float nsig;				/* criterion for peaks being the same*/
   const float nsig_aa = 3;		/* nsig if both in same band, a=apple*/
   const float nsig_ao = 1.5;		/* nsig if different bands o=orange */
   int flags;				/* == peak->flags */
   int i, j;
   int npeak = peaks->npeak;		/* initial number of peaks */
   PEAK *peak;				/* the peak under consideration */
   PEAK *peak2;				/* another peak under consideration */
   const PEAK *ptr;			/* utility pointer to PEAKs */
   int same_band;			/* peaks were detected in same band */
   float sigma_ad = 2;			/* sigma to use if {row,col}cErr
					   is < 0 */
   float var_r, var_c;			/* variances in {row,col} separation */
   float var1, var2;			/* components of var_[rc] */

   for(i = 0;i < peaks->npeak;i++) {
      if((peak = peaks->peaks[i]) == NULL) {
	 continue;			/* already merged */
      }
      flags = peak->flags;
      rowc = peak->rowc; colc = peak->colc;

      for(j = i + 1;j < peaks->npeak;j++) {
	 if((peak2 = peaks->peaks[j]) == NULL) {
	    continue;			/* already merged */
	 }

	 same_band = ((flags&PEAK_BANDMASK) == (peak2->flags&PEAK_BANDMASK));
	 if(same_band ||
	    ((flags & PEAK_CANONICAL) && (peak2->flags & PEAK_CANONICAL))) {
	    var1 = pow(((peak->rowcErr <0) ? sigma_ad : peak->rowcErr ), 2);
	    var2 = pow(((peak2->rowcErr<0) ? sigma_ad : peak2->rowcErr), 2);
	    var_r = var1 + var2;
	    
	    var1 = pow(((peak->colcErr <0) ? sigma_ad : peak->colcErr ), 2);
	    var2 = pow(((peak2->colcErr<0) ? sigma_ad : peak2->colcErr), 2);
	    var_c = var1 + var2;
/*
 * merge peaks within the same saturated core
 */
	    if((peak->flags & PEAK_SATUR) || (peak2->flags & PEAK_SATUR)) {
	       var_r += 3*3;		/* XXX */
	       var_c += 3*3;
	    }

	    nsig = same_band ? nsig_aa : nsig_ao;
	    if(pow((peak2->rowc - rowc),2) < nsig*nsig*var_r &&
	       pow((peak2->colc - colc),2) < nsig*nsig*var_c) {
	       for(ptr = peak; ptr->next != NULL; ptr = ptr->next) continue;
	       
	       ((PEAK *)ptr)->next = peak2;
	       peaks->peaks[j] = NULL;
	       peak2->flags |= PEAK_DANGLING;
	       npeak--;
	    }
	 }
      }
   }
   peaks->npeak = npeak;

   condense_peaks(peaks);
}

static void
clean_peaks(PEAKS *peaks)
{
   int flags;				/* == peak->flags */
   int i;
   PEAK *peak;				/* the peak under consideration */
   PEAK *peak2;				/* another peak under consideration */
   const PEAK *ptr;			/* utility pointer to PEAKs */
/*
 * First consider different PEAKs in peaks[]
 */
   clean_different_peaks(peaks);
/*
 * and now PEAKs on each peak->next list, i.e. detections in different bands
 * or multiple known object matches
 */
   for(i = 0;i < peaks->npeak;i++) {
      for(peak = peaks->peaks[i];peak != NULL;peak = (PEAK *)peak->next) {
	 flags = peak->flags;
	   
	 ptr = peak;
	 peak2 = (PEAK *)ptr->next;
	 while(peak2 != NULL) {
	    if((flags & PEAK_BANDMASK) == (peak2->flags & PEAK_BANDMASK)) {
	       if (peak2->catID != 0 && peak->catID != peak2->catID) {	/* Peak may be a known object */
		   float d2 = pow(peak->rowc - peak2->rowc,2) + pow(peak->colc - peak2->colc,2);
		   if (peak->catID != 0) {
		       fprintf(stderr,"Replacing peak's catID %d with %d in clean_peaks (d = %.2g)\n",
			       peak->catID, peak2->catID, sqrt(d2));
		   }
		   peak->catID = peak2->catID;
	       }
		
	       ((PEAK *)ptr)->next = peak2->next;
	       peak2->next = NULL;
	       peak2->flags &= ~PEAK_DANGLING;
	       phPeakDel(peak2);
	       peak2 = (PEAK *)ptr;
	    }
	    ptr = peak2;
	    peak2 = (PEAK *)peak2->next;
	 }
      }
   }
}

/*****************************************************************************/
/*
 * given an OBJC, an OBJECT1 which overlaps the OBJC's master mask,
 * the color of the OBJECT1, and the offsets one must add to the
 * OBJECT1 to bring it into the standard coord system, put the
 * OBJECT1 into the proper slot in the OBJC.  If that slot is already
 * occupied by another OBJECT1, then 
 * 
 *     - make sure that we merge the current OBJECT1 into the master mask
 *     - set the object center (colc, rowc) to be whichever original centre
 *       had a higher value
 *     - merge the PEAK lists together
 *     - account for catIDs
 *     - delete the current OBJECT1 and set whatever points to it to NULL
 *     - delete the input OBJECT1
 * 
 * We will re-calculate all other object parameters later on
 *
 * If the appropriate slot in the OBJC is _not_ yet occupied, then
 * simply place the OBJECT1 into that slot. Note that this means that
 * you must set the obj's previous home to NULL after calling this
 * function (if it has one, e.g. as part of an OBJC)
 *
 * output: none               (although we modify OBJC mask, and possibly
 *                             the (colc, rowc) fields of OBJC's OBJECT1)
 */
static void
merge_obj_into_objc(OBJC *objc,		/* OBJC in which ... */
		    OBJECT1 *obj,	/* ... this OBJECT1 falls */
		    const FIELDPARAMS *fiparams, /* properties of field */
		    int color,		/* color of the OBJECT1 */
		    float drow,		/* add this to the OBJECT1 to */
		    float dcol,		/*         transform to OBJC's frame */
		    float eps)		/* peaks this close are merged */
{
   OBJECT1 *other;
/*
 * Does the objc have an models subtracted from the image? If so,
 * we'll need to grow the non-modeled detection. Note that models
 * are associated with _children_ when the deblender ran
 */
   {
       int has_model = 0;
       const OBJC *ptr = phObjcDescendentNext(objc);
       while(ptr != NULL) {
	   if(ptr->color[color] != NULL && ptr->color[color]->model != NULL) {
	       has_model++;
	       break;
	   }
	   ptr = phObjcDescendentNext(NULL);
       }

       if(has_model) {
	   OBJMASK *onew = phObjmaskGrow(obj->mask,
					 fiparams->frame[color].data, 10); /* 10 XXX */
	   phObjmaskDel(obj->mask);
	   obj->mask = onew;
       }
   }
/*
 * merge the obj's mask into the OBJC's master mask
 */
   phObjmaskMerge(objc->aimage->master_mask, obj->mask, drow, dcol);
   
   if(objc->color[color] == NULL) {
      objc->color[color] = obj; 
      objc->flags3 |= (obj->flags3 & OBJECT3_MEASURE_BRIGHT);
   } else {
      other = objc->color[color];
      phObjmaskMerge(other->mask, obj->mask, 0, 0);
/*
 * which object should we choose? If only one's detected it is easy:
 * use that one. If one didn't have its position measured,
 * again we use the other one.
 *
 * If these tests fail, we have two viable candidates. Use the one with
 * a brighter peak
 */
      if((other->flags & OBJECT1_DETECTED) &&
	 (!(obj->flags & OBJECT1_DETECTED) ||
	  (obj->flags & OBJECT1_CANONICAL_CENTER))) {
	 ;				/* use other, no need to copy rowc */
	 obj->flags &= ~OBJECT1_CANONICAL_CENTER; /* we didn't use it */
      } else if((obj->flags & OBJECT1_DETECTED) &&
		(!(other->flags & OBJECT1_DETECTED) ||
		 (other->flags & OBJECT1_CANONICAL_CENTER))) {
	 other->colc = obj->colc;	/* use obj */
	 other->rowc = obj->rowc;
	 other->flags &= ~OBJECT1_CANONICAL_CENTER; /* we didn't use it */
      } else {				/* either both or neither are good */
	 if(obj->peaks->npeak > 0 && other->peaks->npeak > 0 &&
	    obj->peaks->peaks[0]->peak > other->peaks->peaks[0]->peak) {
	    other->colc = obj->colc;	/* use obj */
	    other->rowc = obj->rowc;
	    other->flags &= ~OBJECT1_CANONICAL_CENTER; /* we didn't use it */
	 } else {
	    ;				/* use other, no need to copy rowc */
	    obj->flags &= ~OBJECT1_CANONICAL_CENTER; /* we didn't use it */
	 }
      }

      other->flags |= obj->flags;
      other->flags2 |= obj->flags2;
      if(obj->flags3 & OBJECT3_HAS_SATUR_DN) {
	 other->flags3 |= OBJECT3_HAS_SATUR_DN;
	 other->satur_DN += obj->satur_DN;
      }
      if(phPeaksMerge(other->peaks, obj->peaks,
					   0, 0, eps, 0, PEAK_IN_BRIGHT) > 1) {
	 other->flags |= OBJECT1_BLENDED;
      }

      if(other->model == NULL) {
	  other->model = obj->model;	/* may be NULL */
	  obj->model = NULL;		/* it is now! Don't free it */
      } else if(obj->model != NULL) {
	  other->model = shRegIntLincom(other->model, obj->model, 0, 1, 1, LINCOM_UNION);
      }

      objc->flags3 |= (obj->flags3 & OBJECT3_MEASURE_BRIGHT);
      
      phObject1Del(obj);
   }
}

/*****************************************************************************/
/*
 * given an OBJC which is _already_ part of the merged chain, and one
 * which isn't, merge the two so that we end up with a single OBJC
 * in the output CHAIN which represents the union of both OBJCs in
 * its master mask.
 *
 * we leave the second OBJC untouched (although we destroy or remove its
 * OBJECT1s), but we must remember to remove it from the CHAIN and delete it;
 * it is completely superfluous once this routine has finished.
 *
 * output: none               (although we modify objc1's mask, and also
 *                             the (colc, rowc) fields of its OBJECT1s)
 */
static void
merge_objcs(OBJC *objc1,		/* an object on the merged chain */
	    OBJC *objc2,		/* an object that isn't */
	    const FIELDPARAMS *fiparams, /* e.g. TRANS for astrometry */
	    float eps)			/* peaks this close are merged */
{
   int color;
   float inrow, incol, drow, dcol;
   OBJECT1 *obj;

   merge_families(objc1, objc2);

   for (color = 0; color < fiparams->ncolor; color++) {
      if ((obj = objc2->color[color]) != NULL) {

	 /* figure out the offset for this obj */
	 inrow = obj->mask->rmin;
         incol = obj->mask->cmin;
	 phOffsetDo(fiparams, inrow, incol, 
		    color, fiparams->ref_band_index,
		    0, NULL, NULL, &drow, NULL, &dcol, NULL);

	 merge_obj_into_objc(objc1, objc2->color[color], fiparams,
						   color, drow, dcol, eps);
	 objc2->color[color] = NULL;
      }
   }

   objc1->flags |= objc2->flags;
   objc1->flags2 |= objc2->flags2;
   objc1->flags3 |= objc2->flags3;
}

/************************************************************************************************************/
/*
 * Merge objc2's family into objc1
 */
static void
merge_families(OBJC *objc1, OBJC *objc2)
{
    OBJC *ptr;

    for(ptr = objc1; ptr->sibbs != NULL; ptr = ptr->sibbs) continue;

    ptr->sibbs = objc2->sibbs;
    for(; ptr != NULL; ptr = ptr->sibbs) {
	if(ptr->parent == objc2) {
	   shMemRefCntrDecr(ptr->parent);
	   ptr->parent = objc1;
	   shMemRefCntrIncr(ptr->parent);
	}
    }
    objc2->sibbs = NULL;
    
    for(ptr = objc1; ptr->children != NULL; ptr = ptr->children) continue;

    ptr->children = objc2->children;
    if(ptr->children != NULL) {
	if(ptr->children->parent == objc2) {
	   shMemRefCntrDecr(ptr->children->parent);
	   ptr->children->parent = objc1;
	   shMemRefCntrIncr(ptr->children->parent);
	}

	for(ptr = ptr->children->sibbs; ptr != NULL; ptr = ptr->sibbs) {
	    if(ptr->parent == objc2) {
	       shMemRefCntrDecr(ptr->parent);
	       ptr->parent = objc1;
	       shMemRefCntrIncr(ptr->parent);
	    }
	}
    }
    objc2->children = NULL;

    objc1->nchild += objc2->nchild;
    objc2->nchild = 0;
}
