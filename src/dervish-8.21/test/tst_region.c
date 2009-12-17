/*
 * Test region.c
 *
 * Robert Lupton (rhl@astro.princeton.edu)
 */
#include <stdio.h>
#include "tst_utils.h"
#include "region.h"
#include "prvt/region_p.h"

/*****************************************************************************/
/*
 * Here's the main programme for testing. It returns the number of
 * tests failed. All test output goes through the function report
 * and should be filtered out and examined
 */
int
main(void)
{
   MASK *mask, *submask;
   REGION *reg, *subreg, *subreg2;
   PIXDATATYPE types[] = { TYPE_U8, TYPE_S8, TYPE_U16, TYPE_S16,
			   TYPE_U32, TYPE_S32, TYPE_FL32 };
   int i;
   int ntype = sizeof(types)/sizeof(types[0]);

   load_libfits(1);		/* force ld to load some symbols;
				   usually called by f_fopen() */

   INIT_PROTECT;
/*
 * allocate and free all types, checking that suitable pointers are valid
 */
   for(i = 0;i < ntype;i++) {
      if((reg = shRegNew("test1",100,100,types[i])) == NULL) {
	 status = FAIL;
	 report("getting region of %dth type",types[i]);
	 continue;
      }
/*
 * see if rows is setup for TYPE_U16
 */
      if(reg->rows != NULL && types[i] != TYPE_U16) {
	 status = FAIL;
      }
      report("seeing if rows is invalid for type %d");
/*
 * Now check delRegion
 */
      if(shRegDel(reg) != SH_SUCCESS) {
	 status = FAIL;
      }
      report("freeing region of %dth type",types[i]);
   }
/*
 * We've allocated one of each of the valid types, now try to get a
 * region of an invalid type
 */
   START_PROTECT;
   (void)shRegNew("test1",100,100,-1000);
   END_PROTECT;
   report("creating a region of an illegal type");
/*
 * now test deleting an illegal region. We have to lie a bit to do this
 */
   if((reg = shRegNew("reg",100,100,TYPE_S16)) == NULL) {
      status = FAIL;
      report("Can't get TYPE_S16 region a second time");
   }
   *(int *)&reg->type = -1000;		/* an illegal type... */
   START_PROTECT;
   shRegDel(reg);
   END_PROTECT;
   report("freeing region of an illegal type");
/*
 * We need a region to test code on. Just use one, s16, out of laziness.
 * Don't use u16, as it has a valid rows pointer
 */
   if((reg = shRegNew("reg",100,100,TYPE_S16)) == NULL) {
      status = FAIL;
      report("Can't get TYPE_S16 region a third time");
   }
/*
 * check clearRegion
 */
   if(reg->rows_s16 == NULL) {
      status = FAIL;
      report("Rows not valid for TYPE_S16",types[i]);
   } else {
      reg->rows_s16[reg->nrow/2][reg->ncol/2] = 123;
      shRegClear(reg);
      if(reg->rows_s16[reg->nrow/2][reg->ncol/2] != 0) {
	 status = FAIL;
      }
      report("testing shRegClear for TYPE_S16");
   }
/*
 * Proceed to subregions
 */
   if((subreg = shSubRegNew("sub",reg,30,40,20,40,COPY_MASK)) == NULL) {
      status = FAIL;
   }
   report("getting valid subregion");

   subreg->rows_s16[10][20] = 100;
   if(reg->rows_s16[20 + 10][40 + 20] != 100) {
      status = FAIL;
   }
   report("is subregion correctly extracted");

   if((subreg2 = shSubRegNew("sub2",subreg,50,50,-10,-5,COPY_MASK)) == NULL) {
      status = FAIL;
   }
   report("growing a subregion");
   if(subreg2->rows_s16[10 - (-10)][20 - (-5)] != 100) {
      status = FAIL;
   }
   report("is grown subregion correctly extracted");
   (void)shRegDel(subreg2);
      
   if(shRegDel(subreg) != SH_SUCCESS) {
      status = FAIL;
   }
   report("freeing subregion");

   if((subreg = shSubRegNew("sub",reg,40,40,10,80,COPY_MASK)) != NULL) {
      status = FAIL;
   }
   report("getting invalid subregion");

   if(shRegDel(subreg) != SH_SUCCESS) {
      status = FAIL;
   }
   report("freeing subregion");
/*
 * Now let's lie, and see if the code notices
 */
   reg->prvt->type = SHREGPHYSICAL;
   if(shRegDel(reg) != SH_FREE_NONVIRTUAL_REG) {
      status = FAIL;
   }
   report("freeing physical subregion");
   reg->prvt->type = SHREGVIRTUAL;
/*
 * So far so good. On to masks
 */
   if((mask = shMaskNew("mask",100,100)) == NULL) {
      status = FAIL;
   }
   report("getting mask");
   if((submask = shSubMaskNew(NULL,mask,20,80,50,10,NO_FLAGS)) == NULL) {
      status = FAIL;
   }
   report("getting submask");

   if ((subreg = shSubRegNew("sub",reg,20,80,50,10,NO_FLAGS)) == NULL ) {
      status = FAIL;
   }
   if (subreg->mask != NULL ) {
      status = FAIL;
   }
   report("getting subregion with no mask copy");

   subreg->mask = submask;
   if ((subreg2 = shSubRegNew("sub2",subreg,10,10,0,20,COPY_MASK)) == NULL ) {
      status = FAIL;
   }
   if (subreg2->mask == NULL) {
      status = FAIL;
   }
   report("getting subregion of subregion with mask copy");

   if (shRegDel(subreg2) != SH_SUCCESS) {
     status = FAIL;
   };
   report("freeing subregion of subregion with copied mask");

   subreg->mask = NULL;
   if (shRegDel(subreg) != SH_SUCCESS)  {
     status = FAIL;
   };
   report("freeing subregion with no mask copy");

   reg->mask = mask;
   if ((subreg = shSubRegNew("sub",reg,20,80,50,10,COPY_MASK)) == NULL ) {
      status = FAIL;
   }
   if (subreg->mask == NULL ) {
      status = FAIL;
   }
   report("getting subregion with mask copy");

   if (shRegDel(subreg) != SH_SUCCESS) {
     status = FAIL;
   };
   report("freeing subregion with mask copy");
   reg->mask = NULL;
   
   mask->rows[60][30] = 100;
   if(submask->rows[60 - 50][30 - 10] != 100) {
      status = FAIL;
   }
   report("extraction of submask");
   shMaskClear(mask);
   if(mask->rows[60][30] != 0) {
      status = FAIL;
   }
   report("clearing mask");

   if(shMaskDel(submask) != SH_SUCCESS) {
      status = FAIL;
   }
   report("freeing submask");
   if(shMaskDel(mask) != SH_SUCCESS) {
      status = FAIL;
   }
   report("freeing mask");
   if(shMaskDel(NULL) != SH_SUCCESS) {
      status = FAIL;
   }
   report("freeing NULL mask");

/*
 * OK, clean up and exit
 */
   if(shRegDel(reg) != SH_SUCCESS) {
      status = FAIL;
      report("freeing region");
   }

   return(test_errors == 0 ? 0 : 1);
}
