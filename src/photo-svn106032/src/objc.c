/*
 * <INTRO>
 *
 * Support for OBJCs, OBJC_IOs, ATLAS_IMAGES, and TEST_INFOs
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dervish.h"
#include "phPeaks.h"
#include "phObjc.h"
#include "phTestInfo.h"

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * create a new OBJC structure and return a pointer to it.
 *
 * return: pointer to new structure
 */
OBJC *
phObjcNew(int ncolor)                   /* number of OBJECT1s in OBJC */
{
   OBJC *objc = (OBJC *) shMalloc(sizeof(OBJC));
   int i;
   static int id = 0;

   *(int *) &(objc->id) = id++;

   *(int *) &(objc->ncolor) = ncolor;

   objc->color = shMalloc(ncolor * sizeof(OBJECT1 *));
   for (i = 0; i < ncolor; i++) {
      objc->color[i] = NULL;
   }

   objc->type = OBJ_UNK;
   objc->prob_psf = VALUE_IS_BAD;
   objc->aimage = phAtlasImageNew(ncolor);
   objc->test = NULL;
   objc->rowc = objc->rowcErr = VALUE_IS_BAD;
   objc->colc = objc->colcErr = VALUE_IS_BAD;
   objc->rowv = objc->rowvErr = VALUE_IS_BAD;
   objc->colv = objc->colvErr = VALUE_IS_BAD;
   objc->catID = 0;
   objc->flags = objc->flags2 = objc->flags3 = 0x0;
   objc->peaks = NULL;

   objc->nchild = 0;
   objc->parent = objc->sibbs = objc->children = NULL;

   return (objc);
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * delete the OBJECT1s and ATLAS_IMAGE associated with the given OBJC,
 * then delete the OBJC itself.
 */

void
phObjcDel(OBJC *objc,			/* OBJC to delete */
	  int deep)			/* should we destroy siblings
					   and children? */
{
   int i;

   if(objc == NULL) return;
   
   if(deep) {
      phObjcDel(objc->children, 1);	/* n.b. will recurse */
      phObjcDel(objc->sibbs, 1);	/*      down lists */

      objc->children = objc->sibbs = NULL;      
   }

   if(p_shMemRefCntrGet(objc) > 0) {	/* still referenced somewhere */
      shMemRefCntrDecr(objc);
      return;
   }

   phObjcDel(objc->parent, 0);		/* may be NULL */
   
   if (objc->color != NULL) {		/* now kill objc itself */
      for (i = 0; i < objc->ncolor; i++) {
	 phObject1Del(objc->color[i]);
      }
      shFree(objc->color);
   }
   phAtlasImageDel(objc->aimage,1);
   phTestInfoDel(objc->test,1);
   phPeaksDel(objc->peaks);

   shFree((char *) objc);
}

/*****************************************************************************/
/*
 * make a copy of an OBJC.
 *
 * If deep is true, make copies of the sub-components (OBJECT1s, ATLAS_IMAGES)
 *
 * If move_ai is true, move (not copy) the ATLAS_IMAGE
 */
OBJC *
phObjcNewFromObjc(const OBJC *objc,	/* OBJC to copy */
		  int deep,		/* copy object1s, ATLAS_IMAGEs etc? */
		  int copy_ai)		/* copy atlas images? */
{
   int c;
   int id;				/* id number of new OBJECT1s */
   OBJC *nobjc;
		  
   if(objc == NULL) {
      return(NULL);
   }

   nobjc = phObjcNew(objc->ncolor);
   if(deep) {				/* we'll make a copy of objc's below */
      phAtlasImageDel(nobjc->aimage, 1);
   }

   nobjc->type = objc->type;
   nobjc->prob_psf = objc->prob_psf;

   nobjc->rowc = objc->rowc;
   nobjc->rowcErr = objc->rowcErr;
   nobjc->colc = objc->colc;
   nobjc->colcErr = objc->colcErr;

   nobjc->rowv = objc->rowv;
   nobjc->rowvErr = objc->rowvErr;
   nobjc->colv = objc->colv;
   nobjc->colvErr = objc->colvErr;

   nobjc->catID = objc->catID;
   nobjc->flags = objc->flags;
   nobjc->flags2 = objc->flags2;

   if(deep) {
      nobjc->peaks = objc->peaks;
      if(objc->peaks != NULL) {
	 p_shMemRefCntrIncr(objc->peaks);
      }

      nobjc->test = objc->test;
      if(objc->test != NULL) {
	 p_shMemRefCntrIncr(objc->test);
      }

      if(copy_ai) {
	 nobjc->aimage = phAtlasImageCopy(objc->aimage, 1);
      } else {
	 nobjc->aimage = objc->aimage;
	 if(objc->aimage != NULL) {
	    p_shMemRefCntrIncr(objc->aimage);
	 }
      }

      for(c = 0; c < objc->ncolor; c++) {
	 if(objc->color[c] != NULL) {
	    nobjc->color[c] = phObject1New();
/*
 * we'd like to simply say
 *   *nobjc->color[c] = *objc->color[c]
 * but OBJECT1->id is declared const; hence the memcpy
 */
	    id = nobjc->color[c]->id;
	    memcpy(nobjc->color[c], objc->color[c], sizeof(OBJECT1));
	    *((int *)&nobjc->color[c]->id) = id; /* cast away const */

	    if(objc->color[c]->mask != NULL) {
		objc->color[c]->mask->refcntr++;
	    }
	    nobjc->color[c]->region = NULL;

	    if(objc->color[c]->peaks != NULL) {
	       p_shMemRefCntrIncr(objc->color[c]->peaks);
	    }

	    if(objc->color[c]->model != NULL) {
	       p_shMemRefCntrIncr(objc->color[c]->model);
	    }
	 }
      }
   }

   return(nobjc);
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * delete the REGIONs associated with the given OBJC and its family
 */
void
phRegDelFromObjc(OBJC *objc,	/* OBJC whose REGIONs are to be deleted */
		 int deep)	/* should we destroy siblings
				   and children? */
{
   int c;
   OBJECT1 *obj1;

   if(objc == NULL) return;
   
   if(deep) {
      phRegDelFromObjc(objc->children, 1); /* n.b. will recurse */
      phRegDelFromObjc(objc->sibbs, 1); /*         down lists */
   }

   for(c = 0;c < objc->ncolor;c++) {
      if((obj1 = objc->color[c]) != NULL && obj1->region != NULL) {
	 SPANMASK *regmask = (SPANMASK *)obj1->region->mask;
	 if(p_shMemRefCntrGet(regmask) > 0) { /* still referenced somewhere */
	    p_shMemRefCntrDecr(regmask);
	 } else {
	    shAssert(regmask != NULL && regmask->cookie == SPAN_COOKIE);
	    phSpanmaskDel(regmask); obj1->region->mask = NULL;
	 }
	 if(obj1->region != NULL &&
	    p_shMemRefCntrGet(obj1->region) > 0) { /* still referenced */
	    p_shMemRefCntrDecr(obj1->region);
	 } else {
	    shRegDel(obj1->region); obj1->region = NULL;
	 }
      }
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * traverse the maze of an OBJC and all of its descendents, returning each
 * in turn, starting with the OBJC itself, then its children, then its
 * siblings. This is repeated recursively, so grandchildren will be listed
 * after children, but before siblings --- which in turn come before nieces
 *
 * Usage:
 *	(void)phObjcDescendentNext(objc0);     (returns objc0)
 *	objc = phObjcDescendentNext(NULL);
 *	objc = phObjcDescendentNext(NULL);
 * until eventually a NULL is returned
 */
OBJC *
phObjcDescendentNext(const OBJC *objc)
{
   typedef struct OBJC_STACK {
       struct OBJC_STACK *next;		/* next OBJC_STACK with a family of OBJCs */
       OBJC *objc;			/* root of a family */
   } OBJC_STACK;			/* stack to save pending families */

   static OBJC_STACK *stack = NULL;	/* stack of families of OBJCs waiting to be processed */
   static const OBJC *next = NULL;	/* the objc we'll return next time */
   static const OBJC *prev = NULL;	/* the previous objc; useful in gdb */
   static const OBJC *pprev = NULL;	/* the previous-previous objc; useful in gdb */

   prev = next;
   if(objc != NULL) {			/* reset the saved stack */
       while (stack != NULL) {
	   OBJC_STACK *tmp = stack->next;
	   shFree(stack);
	   stack = tmp;
       }
   } else {				/* return next family member */
      if (next != NULL) {
	  objc = next;
      } else {				/* no-one's left in this family */
	  if(stack == NULL) {		/* and there are no more families */
	      return(NULL);
	  }

	  objc = stack->objc;		/* there might be a whole family here */
	  {				/* pop stack */
	      OBJC_STACK *tmp = stack->next;
	      shFree(stack);
	      stack = tmp;
	  }
	  shAssert (objc != NULL);
      }
   }

   if(objc->children != NULL) {
       if(objc->sibbs != NULL) {	/* push sibbs onto the stack */
	   OBJC_STACK *tmp = stack;
	   stack = shMalloc(sizeof(OBJC_STACK));
	   stack->next = tmp;
	   stack->objc = objc->sibbs;
       }

      next = objc->children;
   } else if(objc->sibbs != NULL) {
      next = objc->sibbs;
   } else {
       next = NULL;
   }

   pprev = prev;
   prev = next;

   return((OBJC *)objc);
}
	 
/*****************************************************************************/
/*
 * Create or destroy an OBJC structure modified for IO. 
 */
OBJC_IO *
phObjcIoNew(int ncolor)
{
   OBJC_IO *new;
   
   shAssert(ncolor <= NCOLOR);
   new = shMalloc(sizeof(OBJC_IO));
   *(int *)&new->ncolor = ncolor;
   new->aimage = phAtlasImageNew(ncolor);
   new->test = phTestInfoNew(ncolor);

   return(new);
}

OBJC_IO *
phObjcIoNewFromObjc(const OBJC *objc)
{
   int c;
   OBJC_IO *new;
   OBJECT1 *obj1;

   if((objc = phObjcDescendentNext(objc)) == NULL) {
      return(NULL);
   }

   new = shMalloc(sizeof(OBJC_IO));

   new->id = objc->id;
   new->parent = (objc->parent == NULL) ? -1 : objc->parent->id;
   *(int *)&new->ncolor = objc->ncolor;
   new->nchild = objc->nchild;
   new->objc_type = objc->type;
   new->objc_prob_psf = objc->prob_psf;
   new->catID = objc->catID;
   new->objc_flags = objc->flags;
   new->objc_flags2 = objc->flags2;
   new->objc_rowc = objc->rowc;
   new->objc_rowcErr = objc->rowcErr;
   new->objc_colc = objc->colc;
   new->objc_colcErr = objc->colcErr;
   new->rowv = objc->rowv;
   new->rowvErr = objc->rowvErr;
   new->colv = objc->colv;
   new->colvErr = objc->colvErr;
   new->aimage = objc->aimage;
   if(objc->test == NULL) {
      new->test = phTestInfoNew(objc->ncolor);
   } else {
      new->test = objc->test;
      p_shMemRefCntrIncr(objc->test);
   }
   phTestInfoSetFromObjc(objc, new->test);

   for(c = 0;c < objc->ncolor;c++) {
      obj1 = objc->color[c];
      if (obj1 == NULL) { continue; }	// can happen if skip_deblend is true
      shAssert(obj1 != NULL);

      new->rowc[c] = obj1->rowc;
      new->rowcErr[c] = obj1->rowcErr;
      new->colc[c] = obj1->colc;
      new->colcErr[c] = obj1->colcErr;
      new->sky[c] = obj1->sky;
      new->skyErr[c] = obj1->skyErr;
      new->psfCounts[c] = obj1->psfCounts;
      new->psfCountsErr[c] = obj1->psfCountsErr;
      new->fiberCounts[c] = obj1->fiberCounts;
      new->fiber2Counts[c] = obj1->fiber2Counts;
      new->fiberCountsErr[c] = obj1->fiberCountsErr;
      new->petroCounts[c] = obj1->petroCounts;
      new->petroCountsErr[c] = obj1->petroCountsErr;
      new->petroRad[c] = obj1->petroRad;
      new->petroRadErr[c] = obj1->petroRadErr;
      new->petroR50[c] = obj1->petroR50;
      new->petroR50Err[c] = obj1->petroR50Err;
      new->petroR90[c] = obj1->petroR90;
      new->petroR90Err[c] = obj1->petroR90Err;
      new->Q[c] = obj1->Q;
      new->U[c] = obj1->U;
      new->QErr[c] = obj1->QErr;
      new->UErr[c] = obj1->UErr;
      new->M_e1[c] = obj1->M_e1;
      new->M_e2[c] = obj1->M_e2;
      new->M_e1e1Err[c] = obj1->M_e1e1Err;
      new->M_e1e2Err[c] = obj1->M_e1e2Err;
      new->M_e2e2Err[c] = obj1->M_e2e2Err;
      new->M_rr_cc[c] = obj1->M_rr_cc;
      new->M_rr_ccErr[c] = obj1->M_rr_ccErr;
      new->M_cr4[c] = obj1->M_cr4;
      new->M_e1_psf[c] = obj1->M_e1_psf;
      new->M_e2_psf[c] = obj1->M_e2_psf;
      new->M_rr_cc_psf[c] = obj1->M_rr_cc_psf;
      new->M_cr4_psf[c] = obj1->M_cr4_psf;
      new->nprof[c] = obj1->nprof;
      memcpy(new->profMean[c],obj1->profMean,NANN*sizeof(obj1->profMean[0]));
      memcpy(new->profErr[c],obj1->profErr,NANN*sizeof(obj1->profErr[0]));
      new->iso_rowc[c] = obj1->iso_rowc;
      new->iso_rowcErr[c] = obj1->iso_rowcErr;
      new->iso_rowcGrad[c] = obj1->iso_rowcGrad;
      new->iso_colc[c] = obj1->iso_colc;
      new->iso_colcErr[c] = obj1->iso_colcErr;
      new->iso_colcGrad[c] = obj1->iso_colcGrad;
      new->iso_a[c] = obj1->iso_a;
      new->iso_aErr[c] = obj1->iso_aErr;
      new->iso_aGrad[c] = obj1->iso_aGrad;
      new->iso_b[c] = obj1->iso_b;
      new->iso_bErr[c] = obj1->iso_bErr;
      new->iso_bGrad[c] = obj1->iso_bGrad;
      new->iso_phi[c] = obj1->iso_phi;
      new->iso_phiErr[c] = obj1->iso_phiErr;
      new->iso_phiGrad[c] = obj1->iso_phiGrad;
      new->r_deV[c] = obj1->r_deV;
      new->r_deVErr[c] = obj1->r_deVErr;
      new->ab_deV[c] = obj1->ab_deV;
      new->ab_deVErr[c] = obj1->ab_deVErr;
      new->phi_deV[c] = obj1->phi_deV;
      new->phi_deVErr[c] = obj1->phi_deVErr;
      new->counts_deV[c] = obj1->counts_deV;
      new->counts_deVErr[c] = obj1->counts_deVErr;
      new->r_exp[c] = obj1->r_exp;
      new->r_expErr[c] = obj1->r_expErr;
      new->ab_exp[c] = obj1->ab_exp;
      new->ab_expErr[c] = obj1->ab_expErr;
      new->phi_exp[c] = obj1->phi_exp;      
      new->phi_expErr[c] = obj1->phi_expErr;      
      new->counts_exp[c] = obj1->counts_exp;      
      new->counts_expErr[c] = obj1->counts_expErr;      
      new->counts_model[c] = obj1->counts_model;      
      new->counts_modelErr[c] = obj1->counts_modelErr;      
      new->star_L[c] = obj1->star_L; new->star_lnL[c] = obj1->star_lnL;
      new->exp_L[c] = obj1->exp_L; new->exp_lnL[c] = obj1->exp_lnL;
      new->deV_L[c] = obj1->deV_L; new->deV_lnL[c] = obj1->deV_lnL;
      new->fracPSF[c] = obj1->fracPSF;
      new->texture[c] = obj1->texture;
      new->flags[c] = obj1->flags;
      new->flags2[c] = obj1->flags2;
      new->type[c] = obj1->type;
      new->prob_psf[c] = obj1->prob_psf;
   }

   return(new);
}

/*
 * Destroy an OBJC_IO. If deep is true, all structures allocated within the
 * OBJC_IO will be freed; if it's false they will not
 */
void
phObjcIoDel(OBJC_IO *objc_io, int deep)
{
   if(objc_io == NULL) return;

   if(deep) {
      phAtlasImageDel(objc_io->aimage,deep);
   }
   phTestInfoDel(objc_io->test,deep);

   shFree(objc_io);
}

/*****************************************************************************/
/*
 * Create or destroy a TEST_INFO
 */
TEST_INFO *
phTestInfoNew(int ncolor)
{
   TEST_INFO *new_ti = shMalloc(sizeof(TEST_INFO));

   shAssert(ncolor <= NCOLOR);
   *(int *)&new_ti->ncolor = ncolor;
   new_ti->id = -1;

#if TEST_ASTROM_BIAS
   {
      int i;
      for (i = 0; i < ncolor; i++) {
	 new_ti->row_bias[i] = new_ti->col_bias[i] = VALUE_IS_BAD;
      }
   }
#endif
   
   return(new_ti);
}

void
phTestInfoSetFromObjc(const OBJC *objc, TEST_INFO *tst)
{
   int c;
   int i, j;
   OBJECT1 *obj1;

   shAssert(objc != NULL&& tst != NULL);

   tst->id = objc->id;
   *(int *)&tst->ncolor = objc->ncolor;

   tst->objc_npeak = (objc->peaks == NULL) ? 0 : objc->peaks->npeak;
   for(i = 0;i < NPEAK;i++) {
      if(objc->peaks == NULL || i >= objc->peaks->npeak) {
	 tst->objc_peak[i] = VALUE_IS_BAD;
	 tst->objc_peak_col[i] = tst->objc_peak_row[i] = VALUE_IS_BAD;
      } else {
	 tst->objc_peak[i] = objc->peaks->peaks[i]->peak;
	 tst->objc_peak_col[i] = objc->peaks->peaks[i]->colc;
	 tst->objc_peak_row[i] = objc->peaks->peaks[i]->rowc;
      }
   }

   for(c = 0;c < objc->ncolor;c++) {
      obj1 = objc->color[c];
      if (obj1 == NULL) {
	  continue;
      }

      tst->obj1_id[c] = obj1->id;

      tst->nu_star[c] = obj1->nu_star;
      tst->chisq_star[c] = obj1->chisq_star;
      tst->nu_deV[c] = obj1->nu_deV;
      tst->chisq_deV[c] = obj1->chisq_deV;
      tst->nu_exp[c] = obj1->nu_exp;
      tst->chisq_exp[c] = obj1->chisq_exp;
      
      tst->npeak[c] = 0;
      for(i = 0;i < NPEAK;i++) {
	 tst->peak[c][i] = VALUE_IS_BAD;
	 tst->peak_col[c][i] = tst->peak_row[c][i] = VALUE_IS_BAD;
      }
      if(obj1->peaks != NULL) {
	 tst->npeak[c] = obj1->peaks->npeak;

	 for(i = j = 0;i < obj1->peaks->npeak;i++) {
	    tst->peak[c][j] = obj1->peaks->peaks[i]->peak;
	    tst->peak_col[c][j] = obj1->peaks->peaks[i]->colc;
	    tst->peak_row[c][j] = obj1->peaks->peaks[i]->rowc;

	    if(++j == NPEAK) {
	       break;
	    }
	 }
      }
   }
}

/*
 * Destroy an TEST_INFO. If deep is true, all structures allocated within the
 * TEST_INFO will be freed; if it's false they will not
 */
void
phTestInfoDel(TEST_INFO *test, int deep)
{
   int c;
   
   if(test == NULL) return;

   if(deep) {
      for(c = 0;c < test->ncolor;c++) {
	 ;
      }
   }

   shFree(test);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Destroy a chain of OBJCs and all the elements on it
 *
 * return: nothing
 */

void
phObjcChainDel(CHAIN *chain,		/* chain of OBJCs to destroy */
	       int deep)		/* wreak deep destruction? */
{
   OBJC *objc;
   int nel;

   shAssert(chain != NULL &&
	    shChainTypeGet(chain) == shTypeGetFromName("OBJC"));

   nel = chain->nElements;
   
   while(--nel >= 0) {
      objc = shChainElementRemByPos(chain, nel);
      phObjcDel(objc, deep);
   }

   shChainDel(chain);
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * print out information on most of the fields of each
 * color OBJECT1 for each OBJC in the given list; do all color[0]
 * objects first, then all color[1] objects, etc.  Send the
 * output to stdout, if fname = "", or to files of form
 * "fname".0 (color[0]), "fname".1 (color[1]), etc. 
 *
 * return: nothing
 */

void
phObjcListPrint(
   CHAIN *objclist,            /* I: chain of OBJCs to print */
   char *fname                 /* I: where to place output.  if "", then */
                               /*     send to stdout; otherwise, place */
                               /*     color[0] info in "fname.0", etc. */
   )
{
   int i, ncolor;
   char file[50];
   FILE *fp;
   OBJECT1 *obj;
   OBJC *objc;
   CURSOR_T crsr;

   shAssert(shChainTypeGet(objclist) == shTypeGetFromName("OBJC"));

   /* figure out the maximum number of colors any OBJC has */
   ncolor = 0;
   crsr = shChainCursorNew(objclist);
   while ((objc = (OBJC *) shChainWalk(objclist, crsr, NEXT)) != NULL) {
      if (objc->ncolor > ncolor)
	 ncolor = objc->ncolor;
   }

   for (i = 0; i < ncolor; i++) {
      if (strcmp(fname, "") == 0) {
	 fp = stdout;
      }
      else {
	 sprintf(file, "%s.%d", fname, i);
	 if ((fp = fopen(file, "w")) == NULL) {
	    shError("phObjcListPrint: can't open file %s for writing",
	       file);
	    return;
	 }
      }
      while ((objc = (OBJC *) shChainWalk(objclist, crsr, NEXT)) != NULL) {
	 if ((objc->color != NULL) && ((obj = objc->color[i]) != NULL)) {
	    phObject1PrintTerse(obj, fp);
	 }
      }
      if (fp != stdout)
	 fclose(fp);
   }

   shChainCursorDel(objclist, crsr);
}


/***************************************************************************
 * <AUTO EXTRACT>
 *
 * print out a pretty list of information on some of the fields of each
 * color OBJECT1 for the given OBJC; do color[0]
 * object first, then color[1] object, etc.  Send the
 * output to stdout, if fname = "", or to files of form
 * "fname".0 (color[0]), "fname".1 (color[1]), etc. 
 *
 * return: nothing
 */

void
phObjcPrintPretty(
   OBJC *objc,                 /* I: OBJC whose info we print */
   char *fname                 /* I: where to place output.  if "", then */
                               /*     send to stdout; otherwise, place */
                               /*     color[0] info in "fname.0", etc. */
   )
{
   int i, ncolor;
   char file[50];
   FILE *fp;
   OBJECT1 *obj;

   if (objc == NULL) {
      return;
   }
   ncolor = objc->ncolor;

   for (i = 0; i < ncolor; i++) {
      if (strcmp(fname, "") == 0) {
	 fp = stdout;
      }
      else {
	 sprintf(file, "%s.%d", fname, i);
	 if ((fp = fopen(file, "w")) == NULL) {
	    shError("phObjcPrintPretty: can't open file %s for writing",
	       file);
	    return;
	 }
      }
      if ((objc->color == NULL) || (objc->color[i] == NULL)) {
	    continue;
      }
      obj = objc->color[i];
      phObject1PrintPretty(obj, fp);
      if (fp != stdout)
	 fclose(fp);
   }
}

/************************************************************************
 * <AUTO EXTRACT>
 *
 * compare two OBJCs; return 0 if they are the same, or non-zero if not.
 * use phObject1Compare to do the comparison on corresponding OBJECT1s.
 *
 * return: 0                   if the two are the same
 *         1                   if not
 */
int
phObjcCompare(
	      const OBJC *objc1,                /* first OBJC to compare */
	      const OBJC *objc2                 /* second OBJC to compare */
	      )
{
   int i;

   if (objc1->ncolor != objc2->ncolor) {
      return(1);
   }
   for (i = 0; i < objc1->ncolor; i++) {
      if (phObject1Compare(objc1->color[i], objc2->color[i]) != 0) {
	 return(1);
      }
   }
   return(0);
}


/************************************************************************
 * <AUTO EXTRACT>
 *
 * ROUTINE: phObjcClosest
 *
 * DESCRIPTION:
 * given an (x,y) pair, compare the (rowc, colc) of all OBJECT1s in the 
 * given "OBJC list" to it; find the closest OBJC overall, and return it.
 * here, "OBJC list" means an OBJC and all members of the linked
 * list that start with its 'next' field.
 *
 * return: OBJC * to closest OBJC  
 *
 * </AUTO>
 */

OBJC *
phObjcClosest(
   CHAIN *chain,              /* I: search these OBJCs for closest one */
   float xc,                  /* I: desired row coordinate */
   float yc,                  /* I: desired column coordinate */
   int color                  /* I: examine only OBJECT1s in this color */
   )
{
   int col;
   float dr, dc, dist, min, num;
   OBJECT1 *obj;
   OBJC *objc, *best;
   CURSOR_T crsr;

   min = 1e6;
   crsr = shChainCursorNew(chain);
   best = NULL;	/* Note: will return NULL if no objc closer than sqrt(min)*/
   while ((objc = (OBJC *) shChainWalk(chain, crsr, NEXT)) != NULL) {
      dist = 0.0;
      num = 0.0;
#if 0
      for (col = 0; col < objc->ncolor; col++) {
#endif
	  col=color;
	 if ((obj = objc->color[col]) == NULL) {
	    continue;
	 }
	 if ((obj = objc->color[col]) != NULL) {	 
	     dr = obj->rowc - xc;
	     dc = obj->colc - yc;
	     dist += (dr*dr + dc*dc);
	     num++;
	 }
#if 0
      }
#endif
      if (num == 0.0) {
	  continue;
      }
      dist /= num;
      if (dist < min) {
	 min = dist;
	 best = objc;
      }
   }
   shChainCursorDel(chain, crsr);

   return(best);
}
