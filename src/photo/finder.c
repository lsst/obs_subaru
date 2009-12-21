#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <alloca.h>
#include "dervish.h"
#include "phSkyUtils.h"
#include "phPeaks.h"
#include "phConsts.h"
#include "phObjects.h"

volatile int phTimeLeft = 1;		/* time until timer should go off */

static void
sort_spans_y(SPAN *s, int n)
	;

static CHAIN *
gather_objs(OBJMASK *spans,		/* the pspans detected */
	    int **plist,
	    int nlevel,			/* number of levels */
	    const REGION *reg,		/* the region where they were found */
	    int npixel_min_level,	/* The level to which npixel_min applies */
	    int npixel_min,		/* min number of pixels in an object */
	    int npeak_max,		/* max number of peaks per object */
	    int smoothed)		/* is data smoothed? */
	;

static CHAIN *
find_objs(const REGION *reg,		/* input region */
	  const OBJMASK *om,		/* only search within om; can be NULL*/
	  int nlevel,			/* number of levels */
	  const unsigned short *levels, /* the levels in question */
	  int npixel_min_level,		/* The level to apply npixel_min to */
	  int npixel_min,		/* min number of pixels in an object */
	  int npeak_max,		/* max number of peaks per object */
	  int smoothed)			/* is data smoothed? */
	;

typedef struct {			/* used in phObjectsFind */
   int nspan;
   int *s;
} SPANPVEC;

static void
growspanvec(OBJMASK *spans, int **plist, int *maxspans)
	;

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

/*
 * <AUTO EXTRACT>
 * De/Increase the number of PEAKs in a PEAKS
 *
 * See description of phPeaksRealloc() for the difference between the
 * two functions.
 */
void
phPeaksRenew(PEAKS *peaks,		/* PEAKS to expand/contract */
	     int n)			/* desired number of peaks */
{
   realloc_peaks(peaks, n, 0);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * Delete an OBJECT
 */
void
phObjectDel(OBJECT *obj)
{
   int lev;

   if(obj == NULL) return;

   if(obj->size > 0) {
      for(lev = 0;lev < obj->size;lev++) {
	 phObjmaskDel(obj->sv[lev]);
      }
      shFree(obj->sv);
   }
   phPeaksDel(obj->peaks);

   shFree(obj);
}

/*****************************************************************************/
/*
 * <AUTO>
 * Make a new OBJECT
 * </AUTO>
 */
OBJECT *
phObjectNew(
	    int nlevel			/* number of levels */
	    )
{
   int i;
   OBJECT *new;

   new = shMalloc(sizeof(OBJECT));
   if(nlevel == 0) {
      new->sv = NULL;
   } else {
      new->sv = shMalloc(nlevel*sizeof(OBJMASK *));
      for(i = 0;i < nlevel;i++) {
	 new->sv[i] = phObjmaskNew(0);
      }
   }
   new->flags = new->flags2 = new->flags3 = 0x0;
   new->size = new->nlevel = nlevel;
   new->rowc = new->colc = VALUE_IS_BAD;
   new->rowcErr = new->colcErr = VALUE_IS_BAD;
   new->peaks = phPeaksNew(1);
   
   return(new);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Like phObjectsFind, but only search points within the OBJMASK.
 * Don't set any NOTCHECKED bits.
 *
 * reg's {row,col}0 values are obeyed, but om's are ignored. The output
 * object's peak positions do _not_ allow for {row,col}0; this
 * is a bug, but one that would take quite careful analysis to eradicate,
 * as there are quite a few places where the peak centres can be set.
 */
CHAIN *
phObjectsFindInObjmask(const REGION *reg, /* input region */
		       const OBJMASK *om, /* search in this OBJMASK only */
		       int nlevel,	/* number of levels */
		       const unsigned short *levels, /* levels in question */
		       int npixel_min_level, /* the level that npixel_min_level applies */
		       int npixel_min,	/* min number of pixels in an object
					   (<= 0 => no limit is applied) */
		       int npeak_max)	/* max number of peaks per object
					   (-ve => ignore) */
{
   int smoothed = 1;			/* is data smoothed? XXX */
   
   shAssert(om->cmin >= reg->col0 && om->cmax < reg->col0 + reg->ncol);
   shAssert(om->rmin >= reg->row0 && om->rmax < reg->row0 + reg->nrow);
   
   return(find_objs(reg, om, nlevel, levels, npixel_min_level, npixel_min, npeak_max, smoothed));
}

/*****************************************************************************/
/*
 * do the actual work of finding a set of OBJECTs
 *
 * reg's {row,col}0 values are obeyed, but om's are ignored
 */
static CHAIN *
find_objs(const REGION *reg,		/* input region */
	  const OBJMASK *om,		/* only search within om; can be NULL*/
	  int nlevel,			/* number of levels */
	  const unsigned short *levels, /* the levels in question */
	  int npixel_min_level,		/* The level to apply npixel_min to */
	  int npixel_min,		/* min number of pixels in an object */
	  int npeak_max,		/* max number of peaks per object */
	  int smoothed)			/* is data smoothed? */
{
   SPANPVEC curr;			/* indices of spans on current row */
   int i,j;
   int nspan_om;			/* number of spans in om */
   int inspan;				/* are we within a span, and at which
					   level? */
   int ir, ic;				/* row and column indices */
   int iprev;
   int *maxspans;			/* number of currently allocated spans
					   per level */
   CHAIN *objs;				/* the CHAIN to return */
   int **plist;				/* indices for parent trees, one array
					   for each level. See gather_objs's
					   comment for details */
   SPANPVEC prev;			/* indices of spans on previous row */
   OBJMASK *spans;			/* array of array of spans, one array
					   for each level */
   int spans_per_row;			/* max. no. of spans per row of data */
   int x1, x2;				/* first and last pixel of line
					   currently being considered */
   shAssert(npixel_min == 0 || npixel_min_level >= 0);
/*
 * The following variables are unpacked to prevent the compiler thinking
 * that they are aliased
 */
   const int level0 = levels[0];
   const int row0 = reg->row0; const int col0 = reg->col0;
   const int nrow = reg->nrow; const int ncol = reg->ncol;
   PIX *row;
   
   spans_per_row = ncol/2 + 1;		/* limit is achieved by "1010101" */
   if(spans_per_row == 0) {
      return(shChainNew("OBJECT"));	/* don't bother searching single col.*/
   }
   
   maxspans = shMalloc(nlevel*sizeof(*maxspans));
   spans = shMalloc(nlevel*sizeof(*spans));
   plist = shMalloc(nlevel*sizeof(*plist));

   for(i = 0; i < nlevel; i++) {
      maxspans[i] = ncol;	/* initial guess at number of spans needed */
      spans[i].nspan = 0;
      spans[i].s = shMalloc(maxspans[i]*sizeof(*spans->s));
      plist[i] = shMalloc(maxspans[i]*sizeof(**plist));
   }

   prev.s = shMalloc(spans_per_row*sizeof(*prev.s));
   curr.s = shMalloc(spans_per_row*sizeof(*curr.s));
   curr.nspan = prev.nspan = 0;

   if(om == NULL) {
      nspan_om = nrow;
   } else {
      nspan_om = om->nspan;
   }
   
   for(j = 0; phTimeLeft >= 0 && j < nspan_om; j++) {
      if(j > 0 && (om == NULL || om->s[j].y != om->s[j - 1].y)) {
	 SPANPVEC tmp = prev;
	 prev = curr;
	 curr = tmp;

	 curr.nspan = 0;
      }
      
      inspan = 0;
      iprev = 0;
      if(om == NULL) {
	 ir = j + row0;
	 x1 = col0; x2 = col0 + ncol - 1;
      } else {
	 ir = om->s[j].y;
	 x1 = om->s[j].x1; x2 = om->s[j].x2;
      }
      row = reg->ROWS[ir - row0] - col0; /* allow for possible subregion */
      for(ic = x1; ic <= x2; ic++) {
	 if(row[ic] < level0) {
	    if(!inspan)	{	/* below threshold: easy case. */
	       continue;
	    } else {
/*
 * We were in a span and are now below the lowest threshold: complete any
 * outstanding spans and merge level 0 span.
 */
	       do {
		  inspan--;
		  //shAssert(spans[inspan].nspan < maxspans[inspan]); /* XXX slow */
		  spans[inspan].s[spans[inspan].nspan].x2 = ic-1;
		  spans[inspan].nspan++;
	       } while (inspan > 0);

	       /* do any merging. */
	       spans[0].nspan--;	/* adjust to refer to the last
					   completed span. N.b. x1-1 changes
					   to x1 if diagonals are not
					   "touching" */
	       while(iprev < prev.nspan
		     && spans[0].s[prev.s[iprev]].x2
					   < spans[0].s[spans[0].nspan].x1-1) {
		  iprev++;
	       }
	       while(iprev < prev.nspan
		     && spans[0].s[prev.s[iprev]].x1 <=
		     spans[0].s[spans[0].nspan].x2+1) {
	       /* x2+1 changes to x2 if diagonals are not "touching" */
		  int currclass;
		  int prevclass;

		  currclass = spans[0].nspan;
		  /* walk up parent tree */
		  while(plist[0][currclass] != -1)
		    currclass = plist[0][currclass];
		  prevclass = prev.s[iprev];
		  while(plist[0][prevclass] != -1)
		    prevclass = plist[0][prevclass];
		  if(prevclass != currclass)
		    plist[0][currclass] = prevclass;
		  iprev++;
	       }
	       if(iprev)
		 iprev--;

	       //shAssert(spans[0].nspan < maxspans[0]); /* XXX slow */
	       spans[0].nspan++;
	       curr.nspan++;
	       inspan = 0;
	    }
	 } else {
/*
 * above threshold and not in a span: start a new level 0 span
 */
	    if(!inspan) {
	       if(spans[0].nspan == maxspans[0]) {
		  growspanvec(&spans[0], &plist[0], &maxspans[0]);
	       }
	       spans[0].s[spans[0].nspan].y = ir;
	       spans[0].s[spans[0].nspan].x1 = ic;
	       //shAssert(curr.nspan < spans_per_row); /* expensive... */
	       curr.s[curr.nspan] = spans[0].nspan;
	       plist[0][spans[0].nspan] = -1;	/* put it in its own class
						   for now */
/*
 * look for starts of upper level spans
 */
	       for(inspan = 1; inspan < nlevel; inspan++) {
		  if(row[ic] < levels[inspan]) {
		     break;
		  } else {
		     if(spans[inspan].nspan == maxspans[inspan]) {
			growspanvec(&spans[inspan], &plist[inspan],
				    &maxspans[inspan]);
		     }
		     spans[inspan].s[spans[inspan].nspan].y = ir;
		     spans[inspan].s[spans[inspan].nspan].x1 = ic;
		     /* class is same as level 0 span */
		     plist[inspan][spans[inspan].nspan] = spans[0].nspan;
		  }
	       }
	    } else {
/*
 * in a span and above threshold: check for higher threshold crossings.
 */
	       if(inspan != nlevel && row[ic] >= levels[inspan]) {
		  for(; inspan < nlevel; inspan++) { /* look for starts of
							upper level spans */
		     if(row[ic] < levels[inspan]) {
			break;
		     } else {
			if(spans[inspan].nspan == maxspans[inspan]) {
			   growspanvec(&spans[inspan], &plist[inspan],
				       &maxspans[inspan]);
			}
			spans[inspan].s[spans[inspan].nspan].y = ir;
			spans[inspan].s[spans[inspan].nspan].x1 = ic;
			/* class is same as level 0 span */
			plist[inspan][spans[inspan].nspan] = spans[0].nspan;
		     }
		  }
	       } else {
		  while(inspan > 0) { /* look for ends of upper
					 level spans */
		     if(row[ic] >= levels[inspan-1]) {
		       break;
		     } else {
			inspan--;
			//shAssert(spans[inspan].nspan < maxspans[inspan]); /* XXX slow */
			spans[inspan].s[spans[inspan].nspan].x2 = ic-1;
			spans[inspan].nspan++;
		     }
		  }
	       }
	    }
	 }
      }
      if(inspan) {		/* finish any outstanding spans */
	 while(inspan > 0) {
	    inspan--;
	    //shAssert(spans[inspan].nspan < maxspans[inspan]); /* XXX slow */
	    spans[inspan].s[spans[inspan].nspan].x2 = ic-1;
	    spans[inspan].nspan++;
	 }
	 /* do merging */
	 spans[0].nspan--;	/* adjust to refer to the last */
	 /* completed span. */
	 /* x1-1 changes to x1 if diagonals are not "touching" */
	 while(iprev < prev.nspan
	       && spans[0].s[prev.s[iprev]].x2
	       < spans[0].s[spans[0].nspan].x1-1) {
	    iprev++;
	 }
	 /* x2+1 changes to x2 if diagonals are not "touching" */
	 while(iprev < prev.nspan
	       && spans[0].s[prev.s[iprev]].x1 <=
	       spans[0].s[spans[0].nspan].x2+1) {
	    int currclass;
	    int prevclass;

	    currclass = spans[0].nspan;
	    /* walk up parent tree */
	    while(plist[0][currclass] != -1)
	      currclass = plist[0][currclass];
	    prevclass = prev.s[iprev];
	    while(plist[0][prevclass] != -1) {
	       prevclass = plist[0][prevclass];
	    }
	    if(prevclass != currclass) {
	       plist[0][currclass] = prevclass;
	    }
	    iprev++;
	 }
	 //shAssert(spans[0].nspan < maxspans[0]); /* XXX slow */
	 spans[0].nspan++;
	 curr.nspan++;
      }
      shAssert(curr.nspan <= spans_per_row);
   }

   objs = gather_objs(spans, plist, nlevel, reg,
		      npixel_min_level, npixel_min, npeak_max, smoothed);
/*
 * cleanup
 */
   for(i = 0; i < nlevel; i++) {
      shFree(spans[i].s);
      shFree(plist[i]);
   }
   shFree(curr.s);
   shFree(prev.s);
   shFree(maxspans);
   shFree(spans);
   shFree(plist);

   return(objs);
}

/*****************************************************************************/
/*
 * allocate more memory for spans.
 */
static void
growspanvec(OBJMASK *spans, int **plist, int *maxspans)
{
  *maxspans = *maxspans*M_SQRT2 + 1;	/* "+1" so maxspans is increased even in <= 2 */
  spans->s = shRealloc(spans->s, *maxspans*sizeof(*(spans->s)));
  *plist = shRealloc(*plist, *maxspans*sizeof(**plist));
}

/*****************************************************************************/
/*
 * Take the array of spans, the parent tree array and
 * the number of levels as input.  From this it creates an array of
 * OBJECTs and copies the spans into arrays for each level in each OBJECT
 *
 * The coordinates in the SPANs are converted to allow for the origin
 * of the original REGION
 *
 * The spans (i.e. y:x1,x2 triples) are found in order, starting at the
 * lower left corner of the image. The way that spans fit together into
 * objects is specified by the plist array. Let us consider the lowest
 * level first, corresponding to plist[0]. After the nth span is found,
 * plist[0][n] is set to -1, indicating that the nth span belongs to the
 * object called `n'. If it is later found that n really belongs to object
 * m, plist[0][n] is set to m. If object m is later found to belong to l,
 * plist[0][m] is set to l, and so on. We can thus find the nth span's
 * object number by following plist[0] back until we find an entry -1.
 *
 * Higher levels are simply referred back to plist[0], so if the ith
 * object at level 2 is part of object n, plist[2][i] is set to n, and
 * the search as decribed above follows.
 *
 * Reasonably early in this function we follow all these links, and also
 * convert the names of objects to the final ones (i.e. remove the ones
 * that turned out to be merged).
 */
static CHAIN *
gather_objs(OBJMASK *spans,		/* the pspans detected */
	    int **plist,
	    int nlevel,			/* number of levels */
	    const REGION *reg,		/* the region where they were found */
	    int npixel_min_level,	/* The level to which npixel_min applies */
	    int npixel_min,		/* min number of pixels in an object */
	    int npeak_max,		/* max number of peaks per object */
	    int smoothed)		/* is data smoothed? */
{
   int *objnum;				/* index from span to object number */
   int i;
   int nobjs;				/* number of OBJECTs */
   int *ispan;				/* spans per level per object */
   int npeak;				/* number of peaks in an object */
   CHAIN *objChain;
   OBJECT **objs;			/* array of objects found */
   int ilev;
   SPANMASK *sm = (SPANMASK *)reg->mask;
   SPAN *span_rhs, *span_lhs;		/* used in creating the OBJECTs */
   char *object_type = (char *)shTypeGetFromName("OBJECT");
   
   shAssert(nlevel > 0);
   shAssert(sm == NULL || sm->cookie == SPAN_COOKIE);
   
   objnum = alloca(spans[0].nspan*sizeof(int));
   ispan = alloca(spans[0].nspan*sizeof(int));

   for(i = 0; i < spans[0].nspan; i++) {
      objnum[i] = -1;
   }
/*
 * count objects, determine mapping to object numbers, and follow the
 * plist[0] object IDs back through all aliases. We map to objnum at
 * the same time
 */
   nobjs = 0;
   for(i = 0; i < spans[0].nspan; i++) {
      int fi = i;

      while(plist[0][fi] != -1) {
	 fi = plist[0][fi];
      }
      
      if(objnum[fi] == -1) {
	 objnum[fi] = nobjs++;
      }

      ispan[i] = objnum[fi];		/* save it in ispan until the end of
					   this loop */
   }
   memcpy(plist[0],ispan,spans[0].nspan*sizeof(int));
/*
 * resolve the parent IDs in plist
 */
   for(ilev = 1; ilev < nlevel; ilev++) {
      for(i = 0; i < spans[ilev].nspan; i++) {
	 int fi = plist[ilev][i];	/* original object ID ... */
	 plist[ilev][i] = plist[0][fi];	/* is now resolved to this */
      }
   }
/*
 * At this point, plist[i][j] is the final object number for span j of the
 * ith level.
 *
 * Now that we know nobjs, we can allocate the OBJECTs
 */
   shAssert(nobjs <= spans[0].nspan);	/* i.e. ispan is large enough */
   objs = shMalloc(nobjs*sizeof(OBJECT *));

   for(i = 0; i < nobjs; i++) {
      objs[i] = phObjectNew(nlevel);
   }
/*
 * Now loop through each level, moving the spans into the objects where
 * they belong
 */
   for(ilev = 0; ilev < nlevel; ilev++) {
      for(i = 0; i < nobjs; i++) {
	 ispan[i] = 0;
      }

      for(i = 0; i < spans[ilev].nspan; i++) { /* count spans for this level
						  for each object */
	 int fi = plist[ilev][i];

	 objs[fi]->sv[ilev]->nspan++;
      }

      for(i = 0; i < nobjs; i++) {	/* allocate memory for objects */
	 int nspan = objs[i]->sv[ilev]->nspan;
	 if(nspan) {			/* if a lower level is non-existant,
					   all levels above will be as well. */

	    shAssert(objs[i]->nlevel > ilev);
	    objs[i]->sv[ilev]->s = shMalloc(nspan*sizeof(SPAN));
	    objs[i]->sv[ilev]->size = nspan;
	 } else {		/* found maximum level for this object */
	    if(ilev < objs[i]->nlevel) {
	       objs[i]->nlevel = ilev;	/* we should realy realloc objs[i].sv[]
					   but the overhead's too high */
	    }
	 }
      }
/*
 * copy spans into object structures. Note that ispan[n] is a counter for
 * the number of spans copied into object n at this level.
 */
      for(i = 0; i < spans[ilev].nspan; i++) {
	 int fi = plist[ilev][i];	/* the index of the output OBJECT */

	 span_lhs = &(objs[fi]->sv[ilev]->s[ispan[fi]]);
	 span_rhs = &spans[ilev].s[i];
	 span_lhs->x1 = span_rhs->x1;
	 span_lhs->x2 = span_rhs->x2;
	 span_lhs->y = span_rhs->y;

	 ispan[fi]++;
      }
   }
/*
 * Convert output array to a CHAIN. The shChainElementAddByPos call cannot fail
 */
   objChain = shChainNew("OBJECT");
   for(i = 0; i < nobjs; i++) {
      int j;
      
      for(j = 0;j < objs[i]->nlevel;j++) {
	 sort_spans_y(objs[i]->sv[j]->s,objs[i]->sv[j]->nspan);
	 phCanonizeObjmask(objs[i]->sv[j],1);
      }
/*
 * check if object has too few pixels
 */
      if(npixel_min > 0 && objs[i]->sv[npixel_min_level]->npix < npixel_min) {
	 phObjectDel(objs[i]); objs[i] = NULL;
	 continue;
      }
/*
 * check if object is entirely contained in a NOTCHECKED area, and, if so,
 * delete it
 */
      if(sm != NULL) {
	 OBJMASK *intersection = phObjmaskIntersection(objs[i]->sv[0], 
						       sm->masks[S_MASK_NOTCHECKED]);
	 if(intersection != NULL &&
	    intersection->npix > 0.98*objs[i]->sv[0]->npix) { /* XXX */
	    phObjectDel(objs[i]); objs[i] = NULL;
	 }

	 phObjmaskDel(intersection);

	 if(objs[i] == NULL) {
	    continue;
	 }
      }
/*
 * find all the peak pixels in the (usually smoothed) object
 */
      npeak = phObjectPeaksFind(reg, objs[i]->sv[0], (smoothed ? 1 : 3),
						    objs[i]->peaks, npeak_max);
      shAssert(npeak > 0);

      (void)shChainElementAddByPos(objChain,objs[i],object_type,TAIL,AFTER);
   }

   shFree(objs);			/* just the array, not the contents */

   return(objChain);
}

/*****************************************************************************/
/*
 * This function sorts spans, on the assumption that they are either already
 * sorted, or at least long sections are sorted and need to be merged
 */
static void
sort_spans_y(SPAN *s, int n)
{
   int i0,j0;
   int i,j,k;
   int kmax;
   SPAN tmp;
   int yend;				/* largest y value in i-section */

   i0 = 0;
   for(;;) {
/*
 * find the first section that's sorted, the i-section
 */
      for(i = i0;i < n - 1 && s[i].y <= s[i + 1].y;i++) {
	 continue;
      }
      if(i == n - 1) {
	 break;
      }
/*
 * and now the next sorted section, the j-section
 */
      j0 = i + 1;
      yend = s[i].y;
      for(j = j0;j < n - 1 && s[j].y <= s[j + 1].y;j++) {
	 if(s[j + 1].y > yend) {
	    break;
	 }
      }
/*
 * and now merge the i- and j-sections. As each swap moves an element
 * into its final position, this is linear
 */
      i = i0; j = j0;
      for(k = i0, kmax = j + 1;k <= kmax;k++) {
	 if(s[i].y <= s[j].y) {
	    if(k == i) continue;
	    tmp = s[k];
	    s[k] = s[i];
	    s[i] = tmp;
	    i++;
	 } else {
	    shAssert(k != j);
	    tmp = s[k];
	    s[k] = s[j];
	    s[j] = tmp;
	    j++;
	 }
      }
   }
#if !defined(NDEBUG)			/* i.e. shAssert is active */
/*
 * Check that we are sorted.
 */
   for(i = 1;i < n;i++) {
      if(s[i].y < s[i-1].y) {
	 shFatal("Not sorted in sort_spans_y!!");
	 break;
      }
   }
#endif
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Like phObjectsFindInObjmask(), but only return a single OBJMASK containing
 * the point (row, col), or NULL
 *
 * reg's {row,col}0 values are obeyed, but om's are ignored
 */
OBJMASK *
phObjmaskFindInObjmask(const REGION *reg, /* input region */
		       const OBJMASK *om, /* search in this OBJMASK only */
		       int row, int col, /* point to lie in objmask */
		       int threshold)	/* the desired level */
{
   const unsigned short level = threshold; /* for passing to find_objs */
   int i;
   int nobj;				/* == objs->nElements */
   OBJECT *obj;				/* a detected object */
   CHAIN *objs;				/* all detected objects */
   OBJMASK *ret = NULL;			/* desired OBJMASK */
   int smoothed = 1;			/* is data smoothed? XXX */
   
   shAssert(om->cmin >= reg->col0 && om->cmax < reg->col0 + reg->ncol);
   shAssert(om->rmin >= reg->row0 && om->rmax < reg->row0 + reg->nrow);
   
   objs = find_objs(reg, om, 1, &level, -1, 0, 0, smoothed);
   nobj = objs->nElements;
   for(i = 0; i < nobj; i++) {
      obj = shChainElementRemByPos(objs, HEAD);
      if(phPixIntersectObjmask(obj->sv[0], col, row)) {
	 ret = obj->sv[0]; obj->sv[0] = NULL;
	 phObjectDel(obj);
	 break;
      }
      phObjectDel(obj);
   }
   shChainDestroy(objs, (void (*)(void *))phObjectDel);
   
   return(ret);
}   

