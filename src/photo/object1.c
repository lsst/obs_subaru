#include "string.h"

/*
 * <INTRO>
 *
 * Support for OBJECT1s
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dervish.h"
#include "phObjects.h"
#include "phObjc.h"
//#include "atConversions.h"

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * create a new OBJECT1 and return a pointer to it.
 *
 * return: OBJECT1 * pointer to new OBJECT1
 */
OBJECT1 *
phObject1New(void)
{
   int i;
   static int id = 0;
   OBJECT1 *new_object = shMalloc(sizeof(OBJECT1));

   *(int *) &(new_object->id) = id++;
   new_object->comp = 0;
   new_object->region = NULL;
   new_object->mask = NULL;
   new_object->rowc = new_object->rowcErr = VALUE_IS_BAD;
   new_object->colc = new_object->colcErr = VALUE_IS_BAD;
   new_object->sky = new_object->skyErr = VALUE_IS_BAD;
   new_object->Q = new_object->U = VALUE_IS_BAD;
   new_object->QErr = new_object->UErr = VALUE_IS_BAD;

   new_object->M_e1 = new_object->M_e2 = new_object->M_rr_cc = VALUE_IS_BAD;
   new_object->M_e1e1Err = new_object->M_e1e2Err = new_object->M_e2e2Err =
								  VALUE_IS_BAD;
   new_object->M_rr_ccErr = VALUE_IS_BAD;
   new_object->M_cr4 = VALUE_IS_BAD;
   new_object->M_e1_psf = new_object->M_e2_psf =
		new_object->M_rr_cc_psf = new_object->M_cr4_psf = VALUE_IS_BAD;

   new_object->type = OBJ_UNK;

   new_object->psfCounts = new_object->psfCountsErr = VALUE_IS_BAD;
   new_object->fiberCounts = new_object->fiberCountsErr = VALUE_IS_BAD;
   new_object->petroCounts = new_object->petroCountsErr = VALUE_IS_BAD;
   new_object->petroRad = new_object->petroRadErr = VALUE_IS_BAD;
   new_object->petroR50 = new_object->petroR50Err = VALUE_IS_BAD;
   new_object->petroR90 = new_object->petroR90Err = VALUE_IS_BAD;

   new_object->majaxis = new_object->aratio = VALUE_IS_BAD;
   new_object->nprof = 0;
   for (i = 0; i < NANN; i++) {
      new_object->profMean[i] = VALUE_IS_BAD;
      new_object->profMed[i] = VALUE_IS_BAD;
      new_object->profErr[i] = VALUE_IS_BAD;
   }

   new_object->star_L = new_object->exp_L = new_object->deV_L = VALUE_IS_BAD;
   new_object->star_lnL = new_object->exp_lnL = new_object->deV_lnL =
								  VALUE_IS_BAD;
   new_object->chisq_star = new_object->chisq_exp = new_object->chisq_deV =
								  VALUE_IS_BAD;
   new_object->nu_star = new_object->nu_exp = new_object->nu_deV =
								  VALUE_IS_BAD;
   new_object->fracPSF = VALUE_IS_BAD;
   new_object->prob_psf = VALUE_IS_BAD;

   new_object->iso_rowc = new_object->iso_rowcErr = VALUE_IS_BAD;
   new_object->iso_rowcGrad = VALUE_IS_BAD;
   new_object->iso_colc = new_object->iso_colcErr = VALUE_IS_BAD;
   new_object->iso_colcGrad = VALUE_IS_BAD;
   new_object->iso_a = new_object->iso_aErr = VALUE_IS_BAD;
   new_object->iso_aGrad = VALUE_IS_BAD;
   new_object->iso_b = new_object->iso_bErr = VALUE_IS_BAD;
   new_object->iso_bGrad = VALUE_IS_BAD;
   new_object->iso_phi = new_object->iso_phiErr = VALUE_IS_BAD;
   new_object->iso_phiGrad = VALUE_IS_BAD;
   
   new_object->r_deV = new_object->counts_deV = VALUE_IS_BAD;
   new_object->ab_deV = new_object->phi_deV = VALUE_IS_BAD;
   new_object->r_deVErr = new_object->counts_deVErr = VALUE_IS_BAD;
   new_object->ab_deVErr = new_object->phi_deVErr = VALUE_IS_BAD;

   new_object->r_exp = new_object->r_expErr = VALUE_IS_BAD;
   new_object->counts_exp = new_object->counts_expErr = VALUE_IS_BAD;
   new_object->ab_exp = new_object->ab_expErr = VALUE_IS_BAD;
   new_object->phi_exp = new_object->phi_expErr = VALUE_IS_BAD;

   new_object->counts_model = new_object->counts_modelErr = VALUE_IS_BAD;

   new_object->texture = VALUE_IS_BAD;

   new_object->peaks = NULL;

   new_object->flags = new_object->flags2 = new_object->flags3 = 0x0;

   new_object->npix = 0;
   new_object->satur_DN = 0;

   new_object->model = NULL;

   return (new_object);
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * Delete the OBJECT1 given as argument
 */
void
phObject1Del(
	     OBJECT1 *object1		/* OBJECT1 to delete */
	     )
{
   if (object1 != NULL) {
      if (object1->region != NULL) {
	 if(p_shMemRefCntrGet(object1->region) > 0) { /* still referenced */
	    p_shMemRefCntrDecr(object1->region);
	 } else {
	    if(object1->region->mask != NULL) {
	       SPANMASK *sm;
	       sm = (SPANMASK *)object1->region->mask;
	       shAssert(sm->cookie == SPAN_COOKIE);
	       phSpanmaskDel(sm);
	       object1->region->mask = NULL;
	    }
	    shRegDel(object1->region);
	 }
      }
      phObjmaskDel(object1->mask);
      phPeaksDel(object1->peaks);

      if(object1->model != NULL) {
	 if(p_shMemRefCntrGet(object1->model) > 0) { /* still referenced */
	    p_shMemRefCntrDecr(object1->model);
	 } else {
	    phSpanmaskDel((SPANMASK *)object1->model->mask); object1->model->mask = NULL;
	    shRegDel(object1->model);
	 }
      }

      shFree(object1);
   }
}

/*****************************************************************************/

#define PIXDIFF     0.02      /* positions agree within this many pixels */
#define COUNTDIFF   0.02      /* fluxes ratio agrees within 1 +/- this */

/*
 * <AUTO EXTRACT>
 *
 * Compare two OBJECT1s; return 0 if they are "equal", or 1 if not.
 * By "equal", we mean that the values of a selected number of fields
 * are 'close enough'.  Currently these fields are:
 * 
 *       rowc, colc        same to within PIXDIFF pixels
 *       npix              identical
 *
 * return: 0                   if they are "equal"
 *         1                   if not
 */
int
phObject1Compare(
		 const OBJECT1 *obj1,	/* first OBJECT1 to compare */
		 const OBJECT1 *obj2	/* second OBJECT1 to compare */
   )
{
   double x;
   
   if ((obj1 == NULL) && (obj2 == NULL)) {
      return(0);
   }
   else if ((obj1 == NULL) || (obj2 == NULL)) {
      shDebug(2, "phObject1Compare: one obj is NULL, other isn't");
      return(1);
   }

   if (fabs((double)obj1->rowc - obj2->rowc) > PIXDIFF) {
      shDebug(2, "phObject1Compare: objs %d and %d differ in rowc by %f",
	    obj1->id, obj2->id, obj1->rowc - obj2->rowc);
      return(1);
   }
   if (fabs((double)obj1->colc - obj2->colc) > PIXDIFF) {
      shDebug(2, "phObject1Compare: objs %d and %d differ in colc by %f",
	    obj1->id, obj2->id, obj1->colc - obj2->colc);
      return(1);
   }
   if (obj1->npix != obj2->npix) {
      shDebug(2, "phObject1Compare: objs %d and %d differ in npix by %f",
	    obj1->id, obj2->id, obj1->npix - obj2->npix);
      return(1);
   }

   if ((obj1->fiberCounts == 0.0) && (obj2->fiberCounts == 0.0)) {
      x = 0.0;   /* is okay; this line just a placeholder */
   } else if ((obj1->fiberCounts == 0.0) || (obj2->fiberCounts == 0.0)) {
      shDebug(2, "phObject1Compare: objs %d and %d differ in fiberCounts",
	    obj1->id, obj2->id);
      return(1);
   } else {
      if (((x = fabs((double)obj1->fiberCounts - obj2->fiberCounts) /
	          obj1->fiberCounts)) > COUNTDIFF) {
         shDebug(2, "phObject1Compare: objs %d and %d differ in fiberCounts by %f",
	       obj1->id, obj2->id, x);
         return(1);
      }
   }

   return(0);
}



/**************************************************************************
 * <AUTO EXTRACT>
 *
 * print out info on (almost) all the fields of an OBJECT1, putting
 * all the info on a single line with no explanation.
 */

void
phObject1PrintTerse(
		    const OBJECT1 *obj, /* object about which to print info */
		    FILE *fp		/* file-pointer into which to write  */
   )
{
   int n;

   if (obj == NULL) {
      return;
   }

   fprintf(fp, "%6d   %8.2f %8.2f ", obj->id, obj->rowc, obj->colc);
   fprintf(fp, "%4.0f %4.1f %8d ", obj->sky, obj->skyErr, obj->npix);
   fprintf(fp, "%6.1f %6.1f %6.1f ", obj->Q, obj->U, 1 - obj->Q);
   fprintf(fp, "%4d ", obj->type);
   fprintf(fp, "%9.0f %6.2f ", obj->fiberCounts, obj->fiberCountsErr);
   fprintf(fp, "%9.0f %9.0f %9.0f %6.0f", obj->psfCounts,
      obj->fiberCounts, obj->petroCounts, obj->petroRad);
   fprintf(fp, "%5.0f %5.0f ", obj->majaxis, obj->aratio);
   fprintf(fp, "%2d ", obj->nprof);
   for (n = 0; n < obj->nprof; n++) 
      fprintf(fp, "%6.0f ", obj->profMean[n]);
   for (n = 0; n < obj->nprof; n++) 
      fprintf(fp, "%6.0f ", obj->profMed[n]);
   for (n = 0; n < obj->nprof; n++) 
      fprintf(fp, "%6.1f ", obj->profErr[n]);

   fprintf(fp, "\n");
}

/**************************************************************************
 * <AUTO EXTRACT>
 *
 * print out info on a few of the fields of an OBJECT1, putting
 * all info on several lines with a bit of explanation.
 */

void
phObject1PrintPretty(
		     const OBJECT1 *obj, /* OBJECT1 about which to print info*/
		     FILE *fp		/* file-pointer into which to write */
   )
{
   int n;

   if (obj == NULL) {
      return;
   }

   fprintf(fp, "ID: %-6d position: (%8.2f, %8.2f) \n", obj->id, 
      obj->rowc, obj->colc);
   fprintf(fp, "  sky %4.0f +- %4.1f   npix %8d\n", obj->sky, 
      obj->skyErr, obj->npix);
   fprintf(fp, "  <ab/r^2> %6.1f %6.1f %6.1f ", obj->Q, obj->U, 1-obj->Q);
   /*
	fprintf(fp, "  a/b %6.1f  Position Angle %6.4f", obj->aratio,
	obj->majaxis*at_rad2Deg);
	*/
   fprintf(fp, "  types 0x%4x\n", obj->type);
   fprintf(fp, "  fiber %9.0f   Petrosian %9.0f in rad %6.0f\n",
      obj->fiberCounts, obj->petroCounts, obj->petroRad);
   fprintf(fp, "  PSF counts %9.0f +- %.2f\n", 
      obj->psfCounts, obj->psfCountsErr);
   fprintf(fp, "  profMean   %2d ", obj->nprof);
   for (n = 0; n < obj->nprof; n++) 
      fprintf(fp, "%6.0f ", obj->profMean[n]);
   fprintf(fp, "\n");
   fprintf(fp, "  profMedian %2d ", obj->nprof);
   for (n = 0; n < obj->nprof; n++) 
      fprintf(fp, "%6.0f ", obj->profMed[n]);
   fprintf(fp, "\n\n");
}



/**************************************************************************
 * <AUTO EXTRACT>
 *
 * print out info on the cell profiles of an OBJECT1 .
 */

void
phObject1PrintCellProf(
		     const OBJECT1 *obj, /* OBJECT1 about which to print info*/
		     FILE *fp		/* file-pointer into which to write */
   )
{
   int n;

   if (obj == NULL) {
      return;
   }

   fprintf(fp, "  profMean   %2d ", obj->nprof);
   for (n = 0; n < obj->nprof; n++) 
      fprintf(fp, "%6.0f ", obj->profMean[n]);
   fprintf(fp, "\n");
   fprintf(fp, "  profMedian %2d ", obj->nprof);
   for (n = 0; n < obj->nprof; n++) 
      fprintf(fp, "%6.0f ", obj->profMed[n]);
   fprintf(fp, "\n\n");
}
