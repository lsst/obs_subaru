/*
 * Find all objects in input catalog located on or near frame.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dervish.h"
#include "phUtils.h"
#include "atTrans.h"
#include "phKnownobj.h"
#include "phObjc.h"
#include "phOffset.h"
#include "phMaskbittype.h"

#define FIRSTSIZE 20
#define MAXSPANS 100

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * Search through the catalogs finding, for each frame,
 * all objects that lie on or overlap the frame. "Overlap" is loosely
 * defined and depends on the type of object and its size. 
 *
 * LEVEL0 version: finds only objects with center on frame (ignoring
 * positional errors).
 *
 * Returns a list of KNOWNOBJ structuresfor the frame
 */
CHAIN *
phFindKnownObjects(const CHAIN *catalog,     /* Catalog of KNOWNOBJs */
		   const TRANS *frametrans,  /* refcolor to survey coo. */
		   const TRANS *surveytrans, /* survey to equatorial */
		   int nrow,	       /* no. of rows in corrected frame */
		   int ncol,	       /* no. of cols in corrected frame */
		   int overlap)	       /* number of rows in overlap reg */
{
   shAssert(catalog != NULL && frametrans != NULL && surveytrans != NULL);
#if 1					/* XXX */
   shAssert(0*nrow*ncol*overlap != 1);	/* avoid compiler warnings */
   return(shChainNew("KNOWNOBJ"));
#else
    CURSOR_T crsr;
    CHAIN *knownobjs = shChainNew("KNOWNOBJ");
    KNOWNOBJ *ko, *obj;
    TRANS *skytoccd = atTransNew();
    float r, c;

    /* Add error checking */

    /* Make trans from equatorial coord to row and col */
    atTransMultiply((TRANS *)frametrans,(TRANS *)surveytrans,skytoccd);
    atTransInvert(skytoccd,skytoccd);

    crsr = shChainCursorNew(catalog);
    while ((ko = (KNOWNOBJ *) shChainWalk(catalog, crsr, NEXT)) != NULL) {
	r = skytoccd->a + (ko->ra * skytoccd->b) + (ko->dec * skytoccd->c);
	c = skytoccd->d + (ko->ra * skytoccd->e) + (ko->dec * skytoccd->f);
	if ((c >= 0 && c <= ncol) && (r >= 0 && r < nrow + overlap)) {
	    /* We have a winner! */
	    obj = phKnownobjNew();

	    /* 
	     * we want to copy the contents of "ko", but we have to 
	     * be careful; there are no pointer elements, so all is well
	     */
	    *obj = *ko;

	    /* We can also set the row and column center for the object */
	    obj->row = r;
	    obj->col = c;

	    /* For the moment, assume all objects are FIRST objects */
	    obj->nrow = FIRSTSIZE;
	    obj->ncol = FIRSTSIZE;

	    shChainElementAddByPos(knownobjs,obj,"KNOWNOBJ",TAIL,AFTER);
	}
    }
    shChainCursorDel(catalog,crsr);
#endif
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * Make OBJCs for all relevant objects on known object list. 
 * (Currently cuts for all objects, doesn't select by catalog or type)
 */
CHAIN *				        /* O: objcs for known objects */
phCutKnownObjects(const CHAIN *knownobjs, /* chain of KNOWNOBJS */
		  const FIELDPARAMS *fiparams) /* e.g. TRANS info */

{
    KNOWNOBJ *ko;
    int i;
    int kband;				/* band that this object is "detected" in */
    CHAIN *objcs = shChainNew("OBJC");
    OBJC *objc;
    const REGION *reg;
    OBJECT1 *obj1;

    shAssert(knownobjs != NULL && fiparams != NULL);
    kband = fiparams->ref_band_index;

    for(i = shChainSize(knownobjs) - 1; i >= 0; i--) {
       ko = shChainElementGetByPos(knownobjs, i);
	
	/* Make objc  */
	objc = phObjcNew(fiparams->ncolor);
	objc->type = OBJ_KNOWNOBJ;
	objc->catID = atoi(ko->id);

	ko->objc_id = objc->id;

	/* Make mask */
	/*   All this is temporary. At some point the input KNOWNOBJ will */
	/*   contain a mask For the moment, assume all have same, square size*/
	objc->aimage->master_mask = 
	  phObjmaskFromRect(ko->col - ko->ncol/2,ko->row - ko->nrow/2,
			    ko->col + ko->ncol/2,ko->row + ko->nrow/2);
	
	objc->rowc = ko->row; objc->colc = ko->col;
	objc->rowcErr = objc->colcErr = 0;
	objc->flags3 |= OBJECT3_HAS_CENTER;

	/* Setup an OBJECT1 for ko, this should probably really be in
	   the detection band, but use the reference band for now */

	objc->color[kband] = obj1 = phObject1New();
	obj1->type = OBJ_KNOWNOBJ;
	
	obj1->rowc = objc->rowc;
	obj1->rowcErr = objc->rowcErr;
	obj1->colc = objc->colc;
	obj1->rowcErr = objc->colcErr;

	obj1->mask = phObjmaskFromRect(ko->col - ko->ncol/2,ko->row - ko->nrow/2,
				       ko->col + ko->ncol/2,ko->row + ko->nrow/2);

	obj1->peaks = phPeaksNew(1);
	obj1->peaks->peaks[0]->catID = objc->catID;
	obj1->peaks->peaks[0]->rpeak = 
	  obj1->peaks->peaks[0]->rowc = obj1->rowc;
	obj1->peaks->peaks[0]->cpeak = 
	  obj1->peaks->peaks[0]->colc = obj1->colc;

	obj1->peaks->peaks[0]->rowcErr = obj1->rowcErr;
	obj1->peaks->peaks[0]->colcErr = obj1->colcErr;

	shAssert(fiparams->frame[kband].data != NULL);
	reg = fiparams->frame[kband].data;
	if (fiparams->run_overlap == 0) { /* we have no_overlap set */
	    if ((int)obj1->rowc >= reg->nrow ||
		(int)obj1->colc >= reg->ncol) {
		phObjcDel(objc, 1);
		continue;
	    }
	}
	shAssert((int)obj1->rowc >= 0 && reg->nrow > (int)obj1->rowc);
	shAssert((int)obj1->colc >= 0 && reg->ncol > (int)obj1->colc);
	obj1->peaks->peaks[0]->peak = 
	   reg->rows[(int)obj1->rowc][(int)obj1->colc];

	obj1->peaks->npeak++;

	shChainElementAddByPos(objcs,objc,"OBJC",TAIL,AFTER);
    }

    return(objcs);
}
