/*****************************************************************************/
/*
 * Handle phFootprints
 *
 * The origin of this code is that I (RHL) wrote it for Pan-STARRS based on
 * the SDSS object finder.  I _could_ convert it back to look like SDSS
 * code, but that hardly seems worth it.  I did make the changes needed
 * to compile with a C89 compiler (e.g. no just-in-time declarations)
 */
#include <assert.h>
#include <string.h>
#include "phFootprint.h"
#include "phPeaks.h"
#include "phMathUtils.h"

void phSpanDel(phSpan *tmp) {
    shFree(tmp);
}

/*
 * phSpanNew()
 */
phSpan *phSpanNew(int y, int x0, int x1)
{
    phSpan *span = (phSpan *)shMalloc(sizeof(phSpan));

    span->y = y;
    span->x0 = x0;
    span->x1 = x1;

    return(span);
}

/*
 *  Sort phSpans by y, then x0, then x1
 */
int phSpanSortByYX(const void **a, const void **b) {
    const phSpan *sa = *(const phSpan **)a;
    const phSpan *sb = *(const phSpan **)b;

    if (sa->y < sb->y) {
	return -1;
    } else if (sa->y == sb->y) {
	if (sa->x0 < sb->x0) {
	    return -1;
	} else if (sa->x0 == sb->x0) {
	    if (sa->x1 < sb->x1) {
		return -1;
	    } else if (sa->x1 == sb->x1) {
		return 0;
	    } else {
		return 1;
	    }
	} else {
	    return 1;
	}
    } else {
	return 1;
    }
}

/************************************************************************************************************/

void phFootprintDel(phFootprint *tmp)
{
   if (!tmp) {
        return;
   }

   shFree(tmp->spans);
   shFree(tmp->peaks);

   shFree(tmp);
}

/*
 * phFootprintNew(): allocate the phFootprint structure
 */
phFootprint *phFootprintNew(int nspan, /*  number of spans expected in footprint */
			      const REGION *image) /*  region footprint lives in */
{
    static int id = 1;
    phFootprint *footprint = (phFootprint *)shMalloc(sizeof(phFootprint));
    *(int *)&footprint->id = id++;

    footprint->normalized = 0;

    assert(nspan >= 0);
    footprint->npix = 0;
    footprint->spans = psArrayNewEmpty(nspan, (psArrayDelFunc)phSpanDel);
    footprint->peaks = psArrayNew(0, (psArrayDelFunc)phPeakDel);

    footprint->bbox.x0 = footprint->bbox.y0 = 0;
    footprint->bbox.x1 = footprint->bbox.y1 = -1;

    if (image == NULL) {
	footprint->region.x0 = footprint->region.y0 = 0;
	footprint->region.x1 = footprint->region.y1 = -1;
    } else {
	footprint->region.x0 = image->col0;
	footprint->region.x1 = image->col0 + image->ncol - 1;
	footprint->region.y0 = image->row0;
	footprint->region.y1 = image->row0 + image->nrow - 1;
    }

    return(footprint);
}

static int
phPeakSortBySN(void *va, void *vb)
{
    const PEAK *a = va;
    const PEAK *b = vb;

    assert (a != NULL && b != NULL);

    abort();
}

phFootprint *phFootprintNormalize(phFootprint *fp) {
    if (fp != NULL && !fp->normalized) {
	fp->peaks = psArraySort(fp->peaks, phPeakSortBySN);
	fp->normalized = 1;
    }

    return fp;
}

/*
 *  Add a span to a footprint, returning the new span
 */
static phSpan *phFootprintAddSpan(phFootprint *fp, /*  the footprint to add to */
				  const int y, /*  row to add */
				  int x0,      /*  range of */
				  int x1) {    /*           columns */

    if (x1 < x0) {
	int tmp = x0;
	x0 = x1;
	x1 = tmp;
    }

    phSpan *sp = phSpanNew(y, x0, x1);
    psArrayAdd(fp->spans, 1, sp);
    shFree(sp);
    
    fp->npix += x1 - x0 + 1;

    if (fp->spans->n == 1) {
	fp->bbox.x0 = x0;
	fp->bbox.x1 = x1;
	fp->bbox.y0 = y;
	fp->bbox.y1 = y;
    } else {
	if (x0 < fp->bbox.x0) fp->bbox.x0 = x0;
	if (x1 > fp->bbox.x1) fp->bbox.x1 = x1;
	if (y < fp->bbox.y0) fp->bbox.y0 = y;
	if (y > fp->bbox.y1) fp->bbox.y1 = y;
    }

    return sp;
}

void phFootprintSetBBox(phFootprint *fp) {
    int i;
    phSpan *sp;
    int x0, x1, y0, y1;

    assert (fp != NULL);
    if (fp->spans->n == 0) {
	return;
    }
    sp = fp->spans->data[0];
    x0 = sp->x0;
    x1 = sp->x1;
    y0 = sp->y;
    y1 = sp->y;

    for (i = 1; i < fp->spans->n; i++) {
	sp = fp->spans->data[i];
	
	if (sp->x0 < x0) x0 = sp->x0;
	if (sp->x1 > x1) x1 = sp->x1;
	if (sp->y < y0) y0 = sp->y;
	if (sp->y > y1) y1 = sp->y;
    }

    fp->bbox.x0 = x0;
    fp->bbox.x1 = x1;
    fp->bbox.y0 = y0;
    fp->bbox.y1 = y1;
}

int phFootprintSetNpix(phFootprint *fp) {
   int i;
   int npix = 0;
   assert (fp != NULL);
   for (i = 0; i < fp->spans->n; i++) {
       phSpan *span = fp->spans->data[i];
       npix += span->x1 - span->x0 + 1;
   }
   fp->npix = npix;

   return npix;
}

/************************************************************************************************************/

typedef struct {			/* run-length code for part of object*/
   int id;				/* ID for object */
   int y;				/* Row wherein WSPAN dwells */
   int x0, x1;				/* inclusive range of columns */
} WSPAN;

/*
 * comparison function for qsort; sort by ID then row
 */
static int
compar(const void *va, const void *vb)
{
   const WSPAN *a = va;
   const WSPAN *b = vb;

   if(a->id < b->id) {
      return(-1);
   } else if(a->id > b->id) {
      return(1);
   } else {
      return(a->y - b->y);
   }
}

/*
 * Follow a chain of aliases, returning the final resolved value.
 */
static int
resolve_alias(const int *aliases,	/* list of aliases */
	      int id)			/* alias to look up */
{
   int resolved = id;			/* resolved alias */

   while(id != aliases[id]) {
      resolved = id = aliases[id];
   }

   return(resolved);
}

/*
 * Go through an image, finding sets of connected pixels above threshold
 * and assembling them into phFootprints;  the resulting set of objects
 * is returned as a psArray
 */
psArray *
phFindFootprints(const REGION *img,	/*  image to search */
		 const float threshold,	/*  Threshold */
		 const int npixMin)	/*  minimum number of pixels in an acceptable phFootprint */
{
   int i;
   int i0;				/* initial value of i */
   int id;				/* object ID */
   int in_span;				/* object ID of current WSPAN */
   int nspan = 0;			/* number of spans */
   int nobj = 0;			/* number of objects found */
   int x0 = 0;			        /* unpacked from a WSPAN */
   int *tmp;				/* used in swapping idc/idp */

   assert(img != NULL);

   int F32 = 0;			/*  is this an F32 image? */
   if (img->type == TYPE_FL32) {
       F32 = 1;
   } else if (img->type == TYPE_S32) {
       F32 = 0;
   } else {				/*  N.b. You can't trivially add more cases here; F32 is just a boolean */
       shErrStackPush("Unsupported REGION type: %d", img->type);
       return NULL;
   }
   FL32 *imgRowF32 = NULL;		/*  row pointer if F32 */
   S32 *imgRowS32 = NULL;		/*   "   "   "  "  !F32 */
   
   const int row0 = img->row0;
   const int col0 = img->col0;
   const int nrow = img->nrow;
   const int ncol = img->ncol;
/*
 * Storage for arrays that identify objects by ID. We want to be able to
 * refer to idp[-1] and idp[ncol], hence the (ncol + 2)
 */
   int *id_s = shMalloc(2*(ncol + 2)*sizeof(int));
   memset(id_s, '\0', 2*(ncol + 2)*sizeof(int)); assert(id_s[0] == 0);
   int *idc = id_s + 1;			/*  object IDs in current/ */
   int *idp = idc + (ncol + 2);	/*                        previous row */

   int size_aliases = 1 + nrow/20;	/*  size of aliases[] array */
   int *aliases = shMalloc(size_aliases*sizeof(int)); /*  aliases for object IDs */

   int size_spans = 1 + nrow/20;	/*  size of spans[] array */
   WSPAN *spans = shMalloc(size_spans*sizeof(WSPAN)); /*  row:x0,x1 for objects */
/*
 * Go through image identifying objects
 */
   for (i = 0; i < nrow; i++) {
      int j;
      tmp = idc; idc = idp; idp = tmp;	/* swap ID pointers */
      memset(idc, '\0', ncol*sizeof(int));
      
      imgRowF32 = img->rows_fl32[i];	/*  only one of */
      imgRowS32 = img->rows_s32[i];	/*       these is valid! */

      in_span = 0;			/* not in a span */
      for (j = 0; j < ncol; j++) {
	 double pixVal = F32 ? imgRowF32[j] : imgRowS32[j];
	 if (pixVal < threshold) {
	    if (in_span) {
	       if(nspan >= size_spans) {
		  size_spans *= 2;
		  spans = shRealloc(spans, size_spans*sizeof(WSPAN));
	       }
	       spans[nspan].id = in_span;
	       spans[nspan].y = i;
	       spans[nspan].x0 = x0;
	       spans[nspan].x1 = j - 1;
	       
	       nspan++;

	       in_span = 0;
	    }
	 } else {			/* a pixel to fix */
	    if(idc[j - 1] != 0) {
	       id = idc[j - 1];
	    } else if(idp[j - 1] != 0) {
	       id = idp[j - 1];
	    } else if(idp[j] != 0) {
	       id = idp[j];
	    } else if(idp[j + 1] != 0) {
	       id = idp[j + 1];
	    } else {
	       id = ++nobj;

	       if(id >= size_aliases) {
		  size_aliases *= 2;
		  aliases = shRealloc(aliases, size_aliases*sizeof(int));
	       }
	       aliases[id] = id;
	    }

	    idc[j] = id;
	    if(!in_span) {
	       x0 = j; in_span = id;
	    }
/*
 * Do we need to merge ID numbers? If so, make suitable entries in aliases[]
 */
	    if(idp[j + 1] != 0 && idp[j + 1] != id) {
	       aliases[resolve_alias(aliases, idp[j + 1])] =
						    resolve_alias(aliases, id);
	       
	       idc[j] = id = idp[j + 1];
	    }
	 }
      }

      if(in_span) {
	 if(nspan >= size_spans) {
	    size_spans *= 2;
	    spans = shRealloc(spans, size_spans*sizeof(WSPAN));
	 }

	 assert(nspan < size_spans);	/* we checked for space above */
	 spans[nspan].id = in_span;
	 spans[nspan].y = i;
	 spans[nspan].x0 = x0;
	 spans[nspan].x1 = j - 1;
	 
	 nspan++;
      }
   }

   shFree(id_s);
/*
 * Resolve aliases; first alias chains, then the IDs in the spans
 */
   for (i = 0; i < nspan; i++) {
      spans[i].id = resolve_alias(aliases, spans[i].id);
   }

   shFree(aliases);
/*
 * Sort spans by ID, so we can sweep through them once
 */
   if(nspan > 0) {
      qsort(spans, nspan, sizeof(WSPAN), compar);
   }
/*
 * Build phFootprints from the spans
 */
   psArray *footprints = psArrayNew(nobj, (psArrayDelFunc)phFootprintDel);
   int n = 0;			/*  number of phFootprints */

   if(nspan > 0) {
      id = spans[0].id;
      i0 = 0;
      for (i = 0; i <= nspan; i++) {	/* nspan + 1 to catch last object */
	 if(i == nspan || spans[i].id != id) {
	    phFootprint *fp = phFootprintNew(i - i0, img);
	    
	    for(; i0 < i; i0++) {
		phFootprintAddSpan(fp, spans[i0].y + row0,
				   spans[i0].x0 + col0, spans[i0].x1 + col0);
	    }

	    if (fp->npix < npixMin) {
	       shFree(fp);
	    } else {
	       footprints->data[n++] = fp;
	    }
	 }
	 
	 if (i < nspan) {
	     id = spans[i].id;
	 }
      }
   }

   footprints = psArrayRealloc(footprints, n);
/*
 * clean up
 */
   shFree(spans);

   return footprints;
}

/************************************************************************************************************/
/*
 * A data structure to hold the starting point for a search for pixels above threshold,
 * used by phFindFootprintAtPoint
 *
 * We don't want to find this span again --- it's already part of the footprint ---
 * so we set appropriate mask bits
 */
/*
 * An enum for what we should do with a Startspan
 */
typedef enum {PM_SSPAN_DOWN = 0,	/*  scan down from this span */
	      PM_SSPAN_UP,		/*  scan up from this span */
	      PM_SSPAN_RESTART,		/*  restart scanning from this span */
	      PM_SSPAN_DONE		/*  this span is processed */
} PM_SSPAN_DIR;				/*  How to continue searching */
/*
 * An enum for mask's pixel values.  We're looking for pixels that are above threshold, and
 * we keep extra book-keeping information in the PM_SSPAN_STOP plane.  It's simpler to be
 * able to check for
 */
enum {
    PM_SSPAN_INITIAL = 0x0,		/*  initial state of pixels. */
    PM_SSPAN_DETECTED = 0x1,		/*  we've seen this pixel */
    PM_SSPAN_STOP = 0x2			/*  you may stop searching when you see this pixel */
};
/*
 *  The struct that remembers how to [re-]start scanning the image for pixels
 */
typedef struct {
    const phSpan *span;			/*  save the pixel range */
    PM_SSPAN_DIR direction;		/*  How to continue searching */
    int stop;				/*  should we stop searching? */
} Spartspan;

static void startspanDel(Spartspan *sspan) {
    shFree((void *)sspan->span);
    shFree(sspan);
}

static Spartspan *
SpartspanNew(const phSpan *span,	/*  The span in question */
	     MASK *mask,		/*  Pixels that we've already detected */
	     const PM_SSPAN_DIR dir	/*  Should we continue searching towards the top of the image? */
    ) {
    int i;
    Spartspan *sspan = shMalloc(sizeof(Spartspan));

    sspan->span = span; p_shMemRefCntrIncr((void *)span);
    sspan->direction = dir;
    sspan->stop = 0;
    
    if (mask != NULL) {			/*  remember that we've detected these pixels */
	unsigned char *mpix = &mask->rows[span->y - mask->row0][span->x0 - mask->col0];

	for (i = 0; i <= span->x1 - span->x0; i++) {
	    mpix[i] |= PM_SSPAN_DETECTED;
	    if (mpix[i] & PM_SSPAN_STOP) {
		sspan->stop = 1;
	    }
	}
    }
    
    return sspan;
}

/*
 * Add a new Spartspan to an array of Spartspans.  Iff we see a stop bit, return 1
 */
static int add_startspan(psArray *startspans, /*  the saved Spartspans */
			  const phSpan *sp, /*  the span in question */
			  MASK *mask, /*  mask of detected/stop pixels */
			  const PM_SSPAN_DIR dir) { /*  the desired direction to search */
    if (dir == PM_SSPAN_RESTART) {
	if (add_startspan(startspans, sp, mask,  PM_SSPAN_UP) ||
	    add_startspan(startspans, sp, NULL, PM_SSPAN_DOWN)) {
	    return 1;
	}
    } else {
	Spartspan *sspan = SpartspanNew(sp, mask, dir);
	if (sspan->stop) {		/*  we detected a stop bit */
	    shFree(sspan);		/*  don't allocate new span */

	    return 1;
	} else {
	    psArrayAdd(startspans, 1, sspan);
	    shFree(sspan);		/*  as it's now owned by startspans */
	}
    }

    return 0;
}

/************************************************************************************************************/
/*
 * Search the image for pixels above threshold, starting at a single Spartspan.
 * We search the array looking for one to process; it'd be better to move the
 * ones that we're done with to the end, but it probably isn't worth it for
 * the anticipated uses of this routine.
 *
 * This is the guts of phFindFootprintAtPoint
 */
static int do_startspan(phFootprint *fp, /*  the footprint that we're building */
			 const REGION *img, /*  the REGION we're working on */
			 MASK *mask,	/*  the associated masks */
			 const float threshold,	/*  Threshold */
			 psArray *startspans) {	/*  specify which span to process next */
    int i, j;
    int F32 = 0;			/*  is this an F32 image? */
    if (img->type == TYPE_FL32) {
	F32 = 1;
    } else if (img->type == TYPE_S32) {
	F32 = 0;
    } else {				/*  N.b. You can't trivially add more cases here; F32 is just a boolean */
	shError("Unsupported REGION type: %d", img->type);
	return 0;
    }

    FL32 *imgRowF32 = NULL;		/*  row pointer if F32 */
    S32 *imgRowS32 = NULL;		/*   "   "   "  "  !F32 */
    unsigned char *maskRow = NULL;	/*   masks's row pointer */
    
    const int row0 = img->row0;
    const int col0 = img->col0;
    const int nrow = img->nrow;
    const int ncol = img->ncol;
    
    /********************************************************************************************************/
    
    Spartspan *sspan = NULL;
    for (i = 0; i < startspans->n; i++) {
	sspan = startspans->data[i];
	if (sspan->direction != PM_SSPAN_DONE) {
	    break;
	}
	if (sspan->stop) {
	    break;
	}
    }
    if (sspan == NULL || sspan->direction == PM_SSPAN_DONE) { /*  no more Spartspans to process */
	return 0;
    }
    if (sspan->stop) {			/*  they don't want any more spans processed */
	return 0;
    }
    /*
     * Work
     */
    const PM_SSPAN_DIR dir = sspan->direction;
    /*
     * Set initial span to the startspan
     */
    int x0 = sspan->span->x0 - col0, x1 = sspan->span->x1 - col0;
    /*
     * Go through image identifying objects
     */
    int nx0, nx1 = -1;			/*  new values of x0, x1 */
    const int di = (dir == PM_SSPAN_UP) ? 1 : -1; /*  how much i changes to get to the next row */
    int stop = 0;			/*  should I stop searching for spans? */

    for (i = sspan->span->y -row0 + di; i < nrow && i >= 0; i += di) {
	imgRowF32 = img->rows_fl32[i];	/*  only one of */
	imgRowS32 = img->rows_s32[i];	/*       these is valid! */
	maskRow = mask->rows[i];
	/*
	 * Search left from the pixel diagonally to the left of (i - di, x0). If there's
	 * a connected span there it may need to grow up and/or down, so push it onto
	 * the stack for later consideration
	 */
	nx0 = -1;
	for (j = x0 - 1; j >= -1; j--) {
	    double pixVal = (j < 0) ? threshold - 100 : (F32 ? imgRowF32[j] : imgRowS32[j]);
	    if ((maskRow[j] & PM_SSPAN_DETECTED) || pixVal < threshold) {
		if (j < x0 - 1) {	/*  we found some pixels above threshold */
		    nx0 = j + 1;
		}
		break;
	    }
	}

	if (nx0 < 0) {			/*  no span to the left */
	    nx1 = x0 - 1;		/*  we're going to resume searching at nx1 + 1 */
	} else {
	    /*
	     *  Search right in leftmost span
	     */
	    /* nx1 = 0;			// make gcc happy */
	    for (j = nx0 + 1; j <= ncol; j++) {
		double pixVal = (j >= ncol) ? threshold - 100 : (F32 ? imgRowF32[j] : imgRowS32[j]);
		if ((maskRow[j] & PM_SSPAN_DETECTED) || pixVal < threshold) {
		    nx1 = j - 1;
		    break;
		}
	    }
	    
	    const phSpan *sp = phFootprintAddSpan(fp, i + row0, nx0 + col0, nx1 + col0);
	    
	    if (add_startspan(startspans, sp, mask, PM_SSPAN_RESTART)) {
		stop = 1;
		break;
	    }
	}
	/*
	 * Now look for spans connected to the old span.  The first of these we'll
	 * simply process, but others will have to be deferred for later consideration.
	 *
	 * In fact, if the span overhangs to the right we'll have to defer the overhang
	 * until later too, as it too can grow in both directions
	 *
	 * Note that column ncol exists virtually, and always ends the last span; this
	 * is why we claim below that sx1 is always set
	 */
	int first = 0;		/*  is this the first new span detected? */
	for (j = nx1 + 1; j <= x1 + 1; j++) {
	    double pixVal = (j >= ncol) ? threshold - 100 : (F32 ? imgRowF32[j] : imgRowS32[j]);
	    if (!(maskRow[j] & PM_SSPAN_DETECTED) && pixVal >= threshold) {
		int sx0 = j++;		/*  span that we're working on is sx0:sx1 */
		int sx1 = -1;		/*  We know that if we got here, we'll also set sx1 */
		for (; j <= ncol; j++) {
		    double pixVal = (j >= ncol) ? threshold - 100 : (F32 ? imgRowF32[j] : imgRowS32[j]);
		    if ((maskRow[j] & PM_SSPAN_DETECTED) || pixVal < threshold) { /*  end of span */
			sx1 = j;
			break;
		    }
		}
		assert (sx1 >= 0);

		const phSpan *sp;
		if (first) {
		    if (sx1 <= x1) {
			sp = phFootprintAddSpan(fp, i + row0, sx0 + col0, sx1 + col0 - 1);
			if (add_startspan(startspans, sp, mask, PM_SSPAN_DONE)) {
			    stop = 1;
			    break;
			}
		    } else {		/*  overhangs to right */
			sp = phFootprintAddSpan(fp, i + row0, sx0 + col0, x1 + col0);
			if (add_startspan(startspans, sp, mask, PM_SSPAN_DONE)) {
			    stop = 1;
			    break;
			}
			sp = phFootprintAddSpan(fp, i + row0, x1 + 1 + col0, sx1 + col0 - 1);
			if (add_startspan(startspans, sp, mask, PM_SSPAN_RESTART)) {
			    stop = 1;
			    break;
			}
		    }
		    first = 0;
		} else {
		    sp = phFootprintAddSpan(fp, i + row0, sx0 + col0, sx1 + col0 - 1);
		    if (add_startspan(startspans, sp, mask, PM_SSPAN_RESTART)) {
			stop = 1;
			break;
		    }
		}
	    }
	}

	if (stop || first == 0) {	/*  we're done */
	    break;
	}

	x0 = nx0; x1 = nx1;
    }
    /*
     * Cleanup
     */

    sspan->direction = PM_SSPAN_DONE;
    return stop ? 0 : 1;
}

/*
 * Go through an image, starting at (row, col) and assembling all the pixels
 * that are connected to that point (in a chess kings-move sort of way) into
 * a phFootprint.
 *
 * This is much slower than phFindFootprints if you want to find lots of
 * footprints, but if you only want a small region about a given point it
 * can be much faster
 *
 * N.b. The returned phFootprint is not in "normal form"; that is the phSpans
 * are not sorted by increasing y, x0, x1.  If this matters to you, call
 * phFootprintNormalize()
 */
phFootprint *
phFindFootprintAtPoint(const REGION *img,	/*  image to search */
		       const float threshold,	/*  Threshold */
		       const psArray *peaks, /*  array of peaks; finding one terminates search for footprint */
		       int row, int col) { /*  starting position (in img's parent's coordinate system) */
   int i;
   assert(img != NULL);

   int F32 = 0;				/*  is this an F32 image? */
   if (img->type == TYPE_FL32) {
       F32 = 1;
   } else if (img->type == TYPE_S32) {
       F32 = 0;
   } else {				/*  N.b. You can't trivially add more cases here; F32 is just a boolean */
       shError("Unsupported REGION type: %d", img->type);
       return NULL;
   }
   FL32 *imgRowF32 = NULL;		/*  row pointer if F32 */
   S32 *imgRowS32 = NULL;		/*   "   "   "  "  !F32 */
   
   const int row0 = img->row0;
   const int col0 = img->col0;
   const int nrow = img->nrow;
   const int ncol = img->ncol;
/*
 * Is point in image, and above threshold?
 */
   row -= row0; col -= col0;
   if (row < 0 || row >= nrow ||
       col < 0 || col >= ncol) {
       shError("row/col == (%d, %d) are out of bounds [%d--%d, %d--%d]",
		row + row0, col + col0, row0, row0 + nrow - 1, col0, col0 + ncol - 1);
       return NULL;
   }

   double pixVal = F32 ? img->rows_fl32[row][col] : img->rows_s32[row][col];
   if (pixVal < threshold) {
       return phFootprintNew(0, img);
   }
   
   phFootprint *fp = phFootprintNew(1 + img->nrow/10, img);
/*
 * We need a mask for two purposes; to indicate which pixels are already detected,
 * and to store the "stop" pixels --- those that, once reached, should stop us
 * looking for the rest of the phFootprint.  These are generally set from peaks.
 */
   MASK *mask = shMaskNew("", ncol, nrow);
   mask->row0 = row0; mask->col0 = col0;
   shMaskClear(mask);
   assert(mask->rows[0][0] == PM_SSPAN_INITIAL);
   /*
    * Set stop bits from peaks list
    */
   assert (peaks == NULL || peaks->n == 0);
   if (peaks != NULL) {
       for (i = 0; i < peaks->n; i++) {
	   PEAK *peak = peaks->data[i];
	   mask->rows[peak->rpeak - mask->row0][peak->cpeak - mask->col0] |= PM_SSPAN_STOP;
       }
   }
/*
 * Find starting span passing through (row, col)
 */
   psArray *startspans = psArrayNewEmpty(1, (psArrayDelFunc)startspanDel); /*  spans where we have to restart the search */
   
   imgRowF32 = img->rows_fl32[row];	/*  only one of */
   imgRowS32 = img->rows_s32[row];	/*       these is valid! */
   unsigned char *maskRow = mask->rows[row];
   {
       int i;
       for (i = col; i >= 0; i--) {
	   pixVal = F32 ? imgRowF32[i] : imgRowS32[i];
	   if ((maskRow[i] & PM_SSPAN_DETECTED) || pixVal < threshold) {
	       break;
	   }
       }
       int i0 = i;
       for (i = col; i < ncol; i++) {
	   pixVal = F32 ? imgRowF32[i] : imgRowS32[i];
	   if ((maskRow[i] & PM_SSPAN_DETECTED) || pixVal < threshold) {
	       break;
	   }
       }
       int i1 = i;
       const phSpan *sp = phFootprintAddSpan(fp, row + row0, i0 + col0 + 1, i1 + col0 - 1);

       (void)add_startspan(startspans, sp, mask, PM_SSPAN_RESTART);
   }
   /*
    * Now workout from those Spartspans, searching for pixels above threshold
    */
   while (do_startspan(fp, img, mask, threshold, startspans)) continue;
   /*
    * Cleanup
    */
   shFree(mask);
   shFree(startspans);			/*  restores the image pixel */

   return fp;				/*  phFootprint really */
}

/************************************************************************************************************/
/*
 * Worker routine for the phSetFootprintArrayIDs/phSetFootprintID (and phMergeFootprintArrays)
 */
static void
set_footprint_id(REGION *idImage,	/*  the image to set */
		 const phFootprint *fp, /*  the footprint to insert */
		 const int id) {	/*  the desired ID */
   int j, k;
   const int col0 = fp->region.x0;
   const int row0 = fp->region.y0;

   for (j = 0; j < fp->spans->n; j++) {
       const phSpan *span = fp->spans->data[j];
       S32 *imgRow = idImage->rows_s32[span->y - row0];
       for(k = span->x0 - col0; k <= span->x1 - col0; k++) {
	   imgRow[k] += id;
       }
   }
}

static void
set_footprint_array_ids(REGION *idImage,
			const psArray *footprints, /*  the footprints to insert */
			const int relativeIDs) { /*  show IDs starting at 0, not phFootprint->id */
   int i;
   int id = 0;				/*  first index will be 1 */
   for (i = 0; i < footprints->n; i++) {
       const phFootprint *fp = footprints->data[i];
       if (relativeIDs) {
	   id++;
       } else {
	   id = fp->id;
       }
       
       set_footprint_id(idImage, fp, id);
   }
}

/*
 * Set an image to the value of footprint's ID whever they may fall
 */
REGION *phSetFootprintArrayIDs(const psArray *footprints, /*  the footprints to insert */
				const int relativeIDs) { /*  show IDs starting at 1, not phFootprint->id */
   assert (footprints != NULL);

   if (footprints->n == 0) {
       shError("You didn't provide any footprints");
       return NULL;
   }
   const phFootprint *fp = footprints->data[0];
   const int ncol = fp->region.x1 - fp->region.x0 + 1;
   const int nrow = fp->region.y1 - fp->region.y0 + 1;
   const int col0 = fp->region.x0;
   const int row0 = fp->region.y0;
   assert (ncol >= 0 && nrow >= 0);
   
   REGION *idImage = shRegNew("", nrow, ncol, TYPE_S32);
   idImage->row0 = row0; idImage->col0 = col0;
   shRegClear(idImage);
   /*
    * do the work
    */
   set_footprint_array_ids(idImage, footprints, relativeIDs);

   return idImage;
   
}

/*
 * Set an image to the value of footprint's ID whever they may fall
 */
REGION *phSetFootprintID(const phFootprint *fp, /*  the footprint to insert */
			  const int id) {	/*  the desired ID */
   assert(fp != NULL);
   const int ncol = fp->region.x1 - fp->region.x0 + 1;
   const int nrow = fp->region.y1 - fp->region.y0 + 1;
   const int col0 = fp->region.x0;
   const int row0 = fp->region.y0;
   assert (ncol >= 0 && nrow >= 0);
   
   REGION *idImage = shRegNew("", ncol, nrow, TYPE_S32);
   idImage->row0 = row0; idImage->col0 = col0;
   shRegClear(idImage);
   /*
    * do the work
    */
   set_footprint_id(idImage, fp, id);

   return idImage;
   
}

/************************************************************************************************************/
 /*
  * Examine the peaks in a phFootprint, and throw away the ones that are not sufficiently
  * isolated.  More precisely, for each peak find the highest coll that you'd have to traverse
  * to reach a still higher peak --- and if that coll's more than nsigma DN below your
  * starting point, discard the peak.
  */
int phFootprintCullPeaks(const REGION *img, /*  the image wherein lives the footprint */
			 const float stdev, /*  standard deviation of image */
			 phFootprint *fp, /*  Footprint containing mortal peaks */
			 const float nsigma_delta, /*  how many sigma above local background a peak */
			 		/*  needs to be to survive */
			 const float min_threshold) { /*  minimum permitted coll height */
    int i;
    
    assert (img != NULL); assert (img->type == TYPE_FL32);
    assert (fp != NULL);

    if (fp->peaks == NULL || fp->peaks->n == 0) { /*  nothing to do */
	return 0;
    }

    const REGION *subImg = shSubRegNew("subImg", img,
				       fp->bbox.y1 - fp->bbox.y0 + 1,
				       fp->bbox.x1 - fp->bbox.x0 + 1,
				       fp->bbox.y0, fp->bbox.x0, NO_FLAGS);
    /*
     * We need a psArray of peaks brighter than the current peak.  We'll fake this
     * by reusing the fp->peaks but lying about n.
     *
     * We do this for efficiency (otherwise I'd need two peaks lists), and we are
     * rather too chummy with psArray in consequence.  But it works.
     */
    psArray *brightPeaks = psArrayNew(0, (psArrayDelFunc)phPeakDel);
    shFree(brightPeaks->data);
    brightPeaks->data = fp->peaks->data; p_shMemRefCntrIncr(fp->peaks->data);/*  use the data from fp->peaks */
    /*
     * The brightest peak is always safe; go through other peaks trying to cull them
     */
    for (i = 1; i < fp->peaks->n; i++) { /*  n.b. fp->peaks->n can change within the loop */
	const PEAK *peak = fp->peaks->data[i];
	int x = peak->cpeak - subImg->col0;
	int y = peak->rpeak - subImg->row0;
	/*
	 * Find the level nsigma below the peak that must separate the peak
	 * from any of its friends
	 */
	assert (x >= 0 && x < subImg->ncol && y >= 0 && y < subImg->nrow);
	float threshold = subImg->rows_fl32[y][x] - nsigma_delta*stdev;
	if (threshold < min_threshold) {
#if 1					/*  min_threshold is assumed to be below the detection threshold, */
					/*  so all the peaks are phFootprint, and this isn't the brightest */
	    (void)psArrayRemoveIndex(fp->peaks, i);
	    i--;			/*  we moved everything down one */
	    continue;
#else
#error n.b. We will be running LOTS of checks at this threshold, so only find the footprint once
	    threshold = min_threshold;
#endif
	}
	if (threshold > subImg->rows_fl32[y][x]) {
	    threshold = subImg->rows_fl32[y][x] - 10*EPSILON_f;
	}

	const int peak_id = 1;		/*  the ID for the peak of interest */
	brightPeaks->n = i;		/*  only stop at a peak brighter than we are */
	phFootprint *peakFootprint = phFindFootprintAtPoint(subImg, threshold, brightPeaks, peak->rpeak, peak->cpeak);
	brightPeaks->n = 0;		/*  don't double free */
	REGION *idImg = phSetFootprintID(peakFootprint, peak_id);
	shFree(peakFootprint);

	int j;
	for (j = 0; j < i; j++) {
	    const PEAK *peak2 = fp->peaks->data[j];
	    int x2 = peak2->cpeak - subImg->col0;
	    int y2 = peak2->rpeak - subImg->row0;
	    const int peak2_id = idImg->rows_s32[y2][x2]; /*  the ID for some other peak */

	    if (peak2_id == peak_id) {	/*  There's a brighter peak within the footprint above */
		;			/*  threshold; so cull our initial peak */
		(void)psArrayRemoveIndex(fp->peaks, i);
		i--;			/*  we moved everything down one */
		break;
	    }
	}
	if (j == i) {
	    j++;
	}

	shFree(idImg);
    }

    brightPeaks->n = 0; shFree(brightPeaks);
    shFree((REGION *)subImg);

    return 0;
}

/*
 * Cull an entire psArray of phFootprints
 */
int
phFootprintArrayCullPeaks(const REGION *img, /*  the image wherein lives the footprint */
			  const float stdev, /*  standard deviation of image */
			  psArray *footprints, /*  array of phFootprints */
			  const float nsigma_delta, /*  how many sigma above local background a peak */
    					/*  needs to be to survive */
			  const float min_threshold) { /*  minimum permitted coll height */
    int i;
    
    for (i = 0; i < footprints->n; i++) {
	phFootprint *fp = footprints->data[i];
	if (phFootprintCullPeaks(img, stdev, fp, nsigma_delta, min_threshold)) {
	    shErrStackPush("Culling phFootprint %d", fp->id);
	    return 1;
	}
    }
    
    return 0;
}

/************************************************************************************************************/
/*
 * Support for psArrays
 */
psArray *psArrayNew(const int n, psArrayDelFunc del)
{
    int i;
    psArray *arr = shMalloc(sizeof(psArray));
    arr->n = arr->size = n;
    arr->del = del;

    arr->data = shMalloc(n*sizeof(void *));
    for (i = 0; i < n; i++) {
	arr->data[i] = NULL;
    }

    return arr;
}

psArray *psArrayNewEmpty(const int size, psArrayDelFunc del)
{
    psArray *arr = psArrayNew(size, del);
    arr->n = 0;

    return arr;
    
}

psArray *psArrayRealloc(psArray *arr, const int n)
{
    int i;
    
    assert (arr != NULL);		/* otherwise we'd need a del function here too */

    for (i = arr->n - 1; i > n; i--) {
	arr->del(arr->data[i]);
    }

    arr->data = shRealloc(arr->data, n*sizeof(void));
    arr->size = n;

    return arr;
}

void psArrayDel(psArray *arr)
{
    int i;
    for (i = 0; i < arr->n; i++) {
	arr->del(arr->data[i]);
    }

    shFree(arr);
}

void psArrayAdd(psArray *arr, const int dsize, void *elem)
{
    ;
}

void *psArrayRemoveIndex(psArray *arr, const int i)
{
    return NULL;
}

psArray *psArraySort(psArray *arr, int (*compar)(void *a, void *b))
{
    return arr;
}

