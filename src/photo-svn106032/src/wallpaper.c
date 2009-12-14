#include <stdio.h>
#include <string.h>
#include "dervish.h"
#include "phConsts.h"
#include "phObjects.h"
#include "phSpanUtil.h"
#include "phWallpaper.h"

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Convert a U16 region to U8, using a linear transformation
 */
REGION *
phRegIntToU8Linear(const REGION *reg,	/* region to convert */
		   int base,		/* pixel value to map to 0x0 */
		   int top)		/* pixel value to map to 0xff */
{
   int i, j;
   U8 *lookup;				/* lookup table to do conversion */
   int lsize;				/* dimension of lookup */
   int nrow, ncol;			/* == reg->n{row,col} */
   U16 *row;				/* == reg->row[i] */
   float scale;				/* scaling factor from U16-base to U8*/
   REGION *ureg;			/* U8 region to return */
   U8 *urow;				/* == ureg->row_u8[i] */
   int val;				/* scaled values of reg[123] */

   shAssert(reg != NULL && reg->type == TYPE_U16);
   nrow = reg->nrow; ncol = reg->ncol;

   {
      char buff[60];
      sprintf(buff,"%s -> U8, linear", reg->name);
      ureg = shRegNew(buff, nrow, ncol, TYPE_U8);
   }
/*
 * build the lookup table
 */
   lsize = (top > base) ? top - base + 1 : (base - top + 1);
   lookup = shMalloc(lsize);
   scale = 0xff/(float)(lsize - 1);
   if(top > base) {
      for(i = 0; i < lsize; i++) {
	 lookup[i] = i*scale + 0.5;
      }
      lookup[lsize - 1] = 0xff;		/* fix possible rounding error */
   } else {
      for(i = 0; i < lsize; i++) {
	 lookup[i] = (lsize - 1 - i)*scale + 0.5;
      }
      lookup[0] = 0x0ff;		/* fix possible rounding error */
   }

   lookup -= base;
/*
 * Do the conversion.
 */
   for(i = 0;i < nrow;i++) {
      row = reg->rows_u16[i];
      urow = ureg->rows_u8[i];
      for(j = 0;j < ncol; j++) {
	 val = row[j];
	 urow[j] = (val <= base) ? 0x00 : ((val >= top) ? 0xff : lookup[val]);
      }
   }

   shFree(&lookup[base]);
   
   return(ureg);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Create a U8 region from a single U16 one, using a LUT, as created by
 * phLUTByHistogramEqualize() or phLUTByAnalyticStretch(), or equivalently
 * the TCL binding u16ToU8LUTGet
 *
 * See also phRegIntRGBToU8LUT for the simultaneous conversion of three
 * regions, assumed to contain the R, G, and B components of a true-colour
 * image
 */
REGION *
phRegIntToU8LUT(const REGION *reg,	/* region to convert */
		const REGION *lutreg)	/* the LUT */
{
   int i, j;
   const U8 *lookup;			/* lookup table to do conversion */
   int nrow, ncol;			/* == reg->n{row,col} */
   U16 *row;				/* == reg->row[i] */
   REGION *ureg;			/* U8 region to return */
   U8 *urow;				/* == ureg->row_u8[i] */
   int val;				/* scaled values of reg[123] */

   shAssert(reg != NULL && reg->type == TYPE_U16);
   shAssert(lutreg != NULL && lutreg->type == TYPE_U8);
   shAssert(lutreg->nrow == 1 && lutreg->ncol == MAX_U16 + 1);
   nrow = reg->nrow; ncol = reg->ncol;

   {
      char buff[60];
      sprintf(buff,"%s -> U8, LUT", reg->name);
      ureg = shRegNew(buff, nrow, ncol, TYPE_U8);
   }
/*
 * unpack the lookup table
 */
   lookup = lutreg->rows_u8[0];
/*
 * Do the conversion.
 */
   for(i = 0;i < nrow;i++) {
      row = reg->rows_u16[i];
      urow = ureg->rows_u8[i];
      for(j = 0;j < ncol; j++) {
	 val = row[j];
	 urow[j] = lookup[val];
      }
   }
   
   return(ureg);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Given three U16 regions, and a LUT for the total intensity (R + G + B)/3,
 * return three U8 regions
 *
 * Note that the array (of length 3) returned is static to this file, although
 * the regions are allocated for you
 */
REGION **
phRegIntRGBToU8LUT(const REGION *regR,	/* R-region to convert */
		   const REGION *regG,	/* G-region to convert */
		   const REGION *regB,	/* B-region to convert */
		   const REGION *lutreg, /* the LUT */
		   int separate_bands)	/* scale each band separately? */
{
   int i, j;
   int half;				/* "1/2 << nbit" */
   const U8 *lookup;			/* lookup table to do conversion */
   int nbit = 10;			/* how many bits to scale up scale */
   int nrow, ncol;			/* == reg->n{row,col} */
   U16 *rowR, *rowG, *rowB;		/* == reg[RGB]->row[i] */
   float scale;				/* (intensity scaling)*2^nbit */
   REGION *uregR, *uregG, *uregB;	/* U8 regions to return */
   static REGION *uregions[3];		/* array pointing to ureg[RGB] */
   U8 *urowR, *urowG, *urowB;		/* == ureg[RGB]->row_u8[i] */
   int uval;				/* (R + G + B)/3.0 converted to U8 */
   int val;				/* (R + G + B)/3.0 */
#if 1
   int valR, valG, valB;
#endif

   shAssert(regR != NULL && regR->type == TYPE_U16);
   shAssert(regG != NULL && regG->type == TYPE_U16);
   shAssert(regB != NULL && regB->type == TYPE_U16);
   shAssert(lutreg != NULL && lutreg->type == TYPE_U8);
   shAssert(lutreg->nrow == 1 && lutreg->ncol == MAX_U16 + 1);
   nrow = regR->nrow; ncol = regR->ncol;
   shAssert(regG->nrow == nrow && regB->nrow == nrow);
   shAssert(regG->ncol == ncol && regB->ncol == ncol);

   {
      char buff[60];
      sprintf(buff,"%s -> U8, LUT", regR->name);
      uregR = uregions[0] = shRegNew(buff, nrow, ncol, TYPE_U8);
      sprintf(buff,"%s -> U8, LUT", regG->name);
      uregG = uregions[1] = shRegNew(buff, nrow, ncol, TYPE_U8);
      sprintf(buff,"%s -> U8, LUT", regB->name);
      uregB = uregions[2] = shRegNew(buff, nrow, ncol, TYPE_U8);
   }
/*
 * unpack the lookup table
 */
   lookup = lutreg->rows_u8[0];
/*
 * Do the conversion.
 */
   half = 1 << (nbit - 1);

   for(i = 0;i < nrow;i++) {
      rowR = regR->rows_u16[i];
      rowG = regG->rows_u16[i];
      rowB = regB->rows_u16[i];
      urowR = uregR->rows_u8[i];
      urowG = uregG->rows_u8[i];
      urowB = uregB->rows_u8[i];
      if(separate_bands) {
	 for(j = 0;j < ncol; j++) {
	    val = rowR[j];
	    if(val == 0) {
	       urowR[j] = 0;
	    } else {
	       uval = lookup[val];
	       scale = (uval << nbit)/(float)val;
	       val = scale*rowR[j] + half;
	       urowR[j] = (val >= (255 << nbit)) ? 255 : (val >> nbit);
	    }

	    val = rowG[j];
	    if(val == 0) {
	       urowG[j] = 0;
	    } else {
	       uval = lookup[val];
	       scale = (uval << nbit)/(float)val;
	       val = scale*rowG[j] + half;
	       urowG[j] = (val >= (255 << nbit)) ? 255 : (val >> nbit);
	    }

	    val = rowB[j];
	    if(val == 0) {
	       urowB[j] = 0;
	    } else {
	       uval = lookup[val];
	       scale = (uval << nbit)/(float)val;
	       val = scale*rowB[j] + half;
	       urowB[j] = (val >= (255 << nbit)) ? 255 : (val >> nbit);
	    }
	 }
      } else {
	 for(j = 0;j < ncol; j++) {
	    val = (rowR[j] + rowG[j] + rowB[j])/3.0 + 0.5;
#if 0
	    if(val == 0) {
	       urowR[j] = urowG[j] = urowB[j] = 0;
	       continue;
	    }
#endif
	    uval = lookup[val];
	    scale = (uval << nbit)/(float)val;
#if 0
	    val = scale*rowR[j] + half;
	    urowR[j] = (val >= (255 << nbit)) ? 255 : (val >> nbit);
	    val = scale*rowG[j] + half;
	    urowG[j] = (val >= (255 << nbit)) ? 255 : (val >> nbit);
	    val = scale*rowB[j] + half;
	    urowB[j] = (val >= (255 << nbit)) ? 255 : (val >> nbit);
#else
	    valR = scale*rowR[j] + half;
	    valG = scale*rowG[j] + half;
	    valB = scale*rowB[j] + half;
	    if(valR > valG) {
	       if(valR > valB) {
		  if(valR >= (255 << nbit)) {
		     urowR[j] = 255;
		     urowG[j] = 255*valG/valR;
		     urowB[j] = 255*valB/valR;
		     
		     continue;
		  }
	       } else {
		  if(valB >= (255 << nbit)) {
		     urowR[j] = 255*valR/valB;
		     urowG[j] = 255*valG/valB;
		     urowB[j] = 255;
		     
		     continue;
		  }
	       }
	    } else {
	       if(valG > valB) {
		  if(valG >= (255 << nbit)) {
		     urowR[j] = 255*valR/valG;
		     urowG[j] = 255;
		     urowB[j] = 255*valB/valG;
		     
		     continue;
		  }
	       } else {
		  if(valB >= (255 << nbit)) {
		     urowR[j] = 255*valR/valB;
		     urowG[j] = 255*valG/valB;
		     urowB[j] = 255;
		     
		     continue;
		  }
	       }
	    }

	    urowR[j] = valR >> nbit;
	    urowG[j] = valG >> nbit;
	    urowB[j] = valB >> nbit;
#endif
	 }
      }
   }
   
   return(uregions);
}

/*****************************************************************************/
/*
 * An extern and some static functions to deal with sets of U8 regions
 * representing an RGB separation of an image
 */
typedef struct {			/* run-length code for part of object*/
   int id;				/* ID for object */
   int row;				/* Row wherein WSPAN dwells */
   int col0, col1;			/* inclusive range of columns */
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
      return(a->row - b->row);
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
 * Go through a set of three U8 images, replacing regions where at least
 * one band has a value of MAX_U8; such regions are referred to as
 * `objects' in the comments.  They are also referred to as `saturated';
 * this means that they've saturated the conversion to U8, not the CCD.
 *
 * If the regU16[RGB] regions are provided, they are taken to be
 * the U16 data corresponding to the reg[RGB] regions, and are used
 * to determine the correct colour of `saturated' objects. The value
 * minU16 corresponds to the min value used to create the LUT that
 * was used to create the U8 regions in the first place
 */
void
phFixSaturatedU8Regs(REGION *regR,	/* R-region to fix */
		     REGION *regG,	/* G-region to fix */
		     REGION *regB,	/* B-region to fix */
		     REGION *regU16R,	/* U16 data for R-region, or NULL */
		     REGION *regU16G,	/* U16 data for G-region, or NULL */
		     REGION *regU16B,	/* U16 data for B-region, or NULL */
		     int minU16,	/* value to subtract from U16 regs */
		     int fixupLevel,	/* fix pixels above this in U16 */
		     int fix_all_satur,	/* fix all saturated pixels */
		     int use_mean_color) /* If true, use mean of (U8)-saturated
					    pixels; else use pixels just
					    outsize saturated region */
{
   int *aliases;			/* aliases for object IDs */
   int *id_s;				/* storage for id[cp] */
   int *idc, *idp;			/* object IDs in current/previous row*/
   int i, j;
   int i0;				/* initial value of i */
   int id;				/* object ID */
   int in_span;				/* object ID of current WSPAN */
   int nspan = 0;			/* number of spans */
   int nobj = 0;			/* number of objects found */
   int nrow, ncol;			/* == reg->n{row,col} */
   U8 R, G, B;				/* corrected pixel values */
   U8 *rowR, *rowG, *rowB;		/* == reg[RGB]->rows_u8[i] */
   U16 *rowU16R = NULL, *rowU16G = NULL, *rowU16B = NULL;
   					/* == reg[RGB]->rows_u16[i] */
   int row, col0 = 0, col1;		/* unpacked from a WSPAN */
   CHAIN *satR, *satG, *satB;		/* saturated pixels, or NULL */
   int row0R, col0R;			/* origin of regU16R (if non-NULL) */
   int row0G, col0G;			/* origin of regU16G (if non-NULL) */
   int row0B, col0B;			/* origin of regU16B (if non-NULL) */
   int size_aliases = 0;		/* size of aliases[] array */
   int size_spans = 0;			/* size of spans[] array */
   float sumR, sumG, sumB;		/* not int (rule out overflow) */
   WSPAN *spans;			/* row:col0,col1 for objects */
   int *tmp;				/* used in swapping idc/idp */

   shAssert(regR != NULL && regR->type == TYPE_U8);
   shAssert(regG != NULL && regG->type == TYPE_U8);
   shAssert(regB != NULL && regB->type == TYPE_U8);

   nrow = regR->nrow; ncol = regR->ncol;
   shAssert(regG->nrow == nrow && regG->ncol == ncol);
   shAssert(regB->nrow == nrow && regB->ncol == ncol);

   if(regU16R == NULL) {
      shAssert(regU16G == NULL && regU16B == NULL);
      shAssert(fixupLevel == 0);
      satR = satG = satB = NULL;	/* not used; make gcc happy */
      col0R = row0R = col0G = row0G = col0B = row0B = 0;
   } else {
      shAssert(regU16R != NULL && regU16R->type == TYPE_U16);
      shAssert(regU16G != NULL && regU16G->type == TYPE_U16);
      shAssert(regU16B != NULL && regU16B->type == TYPE_U16);

      shAssert(regU16R->nrow == nrow && regU16R->ncol == ncol);
      shAssert(regU16G->nrow == nrow && regU16G->ncol == ncol);
      shAssert(regU16B->nrow == nrow && regU16B->ncol == ncol);

      if (regU16R->mask == NULL) {
	 shError("phFixSaturatedU8Regs: reg->mask is NULL");
	 satR = satG = satB = NULL;
      } else {
	 satR = ((SPANMASK *)regU16R->mask)->masks[S_MASK_SATUR];
	 satG = ((SPANMASK *)regU16G->mask)->masks[S_MASK_SATUR];
	 satB = ((SPANMASK *)regU16B->mask)->masks[S_MASK_SATUR];
      }

      if(satR == NULL) {
	 shAssert(satG == NULL && satB == NULL);
	 shAssert(!fix_all_satur);
      } else {
	 shAssert(satG != NULL && satB != NULL);
      }

      col0R = regU16R->col0;
      row0R = regU16R->row0;
      col0G = regU16G->col0;
      row0G = regU16G->row0;
      col0B = regU16B->col0;
      row0B = regU16B->row0;

      if(fix_all_satur && satR != NULL) { /* label too-bright pixels as
					     having saturated the LUT */
	 phRegionSetValFromObjmaskChain(regR, satR, MAX_U8);
	 phRegionSetValFromObjmaskChain(regG, satG, MAX_U8);
	 phRegionSetValFromObjmaskChain(regB, satB, MAX_U8);
      }
   }
/*
 * Storage for arrays that identify objects by ID. We want to be able to
 * refer to idp[-1] and idp[ncol], hence the (ncol + 2)
 */
   id_s = shMalloc(2*(ncol + 2)*sizeof(int));
   memset(id_s, '\0', 2*(ncol + 2)*sizeof(int)); shAssert(id_s[0] == 0);
   idc = id_s + 1; idp = idc + (ncol + 2);

   size_aliases = 1 + nrow/20;
   aliases = shMalloc(size_aliases*sizeof(int));

   size_spans = 1 + nrow/20;
   spans = shMalloc(size_spans*sizeof(WSPAN));
/*
 * Go through image identifying objects
 */
   for(i = 0; i < nrow; i++) {
      tmp = idc; idc = idp; idp = tmp;	/* swap ID pointers */
      memset(idc, '\0', ncol*sizeof(int));
      
      rowR = regR->rows_u8[i];
      rowG = regG->rows_u8[i];
      rowB = regB->rows_u8[i];
      if(regU16R != NULL) {
	 rowU16R = regU16R->rows_u16[i];
	 rowU16G = regU16G->rows_u16[i];
	 rowU16B = regU16B->rows_u16[i];
      }

      in_span = 0;			/* not in a span */
      for(j = 0; j < ncol; j++) {
	 if((fixupLevel &&
	     rowU16R[j] < fixupLevel &&
	     rowU16G[j] < fixupLevel && rowU16B[j] < fixupLevel) ||
	    (rowR[j] != MAX_U8 && rowG[j] != MAX_U8 && rowB[j] != MAX_U8)) {
	    if(in_span) {
	       if(nspan >= size_spans) {
		  size_spans *= 2;
		  spans = shRealloc(spans, size_spans*sizeof(WSPAN));
	       }
	       spans[nspan].id = in_span;
	       spans[nspan].row = i;
	       spans[nspan].col0 = col0;
	       spans[nspan].col1 = j - 1;
	       
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
	       id = nobj++;

	       if(id >= size_aliases) {
		  size_aliases *= 2;
		  aliases = shRealloc(aliases, size_aliases*sizeof(int));
	       }
	       aliases[id] = id;
	    }

	    idc[j] = id;
	    if(!in_span) {
	       col0 = j; in_span = id;
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

	 shAssert(nspan < size_spans);	/* we checked for space above */
	 spans[nspan].id = in_span;
	 spans[nspan].row = i;
	 spans[nspan].col0 = col0;
	 spans[nspan].col1 = j - 1;
	 
	 nspan++;
      }
   }

   shFree(id_s);
/*
 * Resolve aliases; first alias chains, then the IDs in the spans
 */
   for(i = 0; i < nspan; i++) {
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
 * OK, we know where all the objects are. Find their corrected
 * values in each band, and correct the input images
 */
#define DEBUG_ID 0			/* set pixels to IDs when debugging */
#if DEBUG_ID
   shRegClear(regR); shRegClear(regB); shRegClear(regB); /* RHL */
#endif

   if(nspan > 0) {
      sumR = sumG = sumB = 0;
      id = spans[0].id;
      i0 = 0;
      for(i = 0; i <= nspan; i++) {	/* nspan + 1 to catch last object */
	 if(i == nspan || spans[i].id != id) {
/*
 * Find desired colour for this object
 */
	    if(sumR + sumB + sumG > 0) {
	       if(sumR > sumG) {
		  if(sumR > sumB) {
		     R = MAX_U8;
		     G = (MAX_U8*sumG)/sumR;
		     B = (MAX_U8*sumB)/sumR;
		  } else {
		     R = (MAX_U8*sumR)/sumB;
		     G = (MAX_U8*sumG)/sumB;
		     B = MAX_U8;
		  }
	       } else {
		  if(sumG > sumB) {
		     R = (MAX_U8*sumR)/sumG;
		     G = MAX_U8;
		     B = (MAX_U8*sumB)/sumG;
		  } else {
		     R = (MAX_U8*sumR)/sumB;
		     G = (MAX_U8*sumG)/sumB;
		     B = MAX_U8;
		  }
	       }
/*
 * correct all the pixels in this object
 */
	       for(; i0 < i; i0++) {
		  row = spans[i0].row;
		  col0 = spans[i0].col0;
		  col1 = spans[i0].col1;
#if DEBUG_ID
		  R = G = B = spans[i0].id%200;
#endif
		  rowR = regR->rows_u8[row];
		  rowG = regG->rows_u8[row];
		  rowB = regB->rows_u8[row];
		  
		  for(j = col0; j <= col1; j++) {
		     rowR[j] = R;
		     rowG[j] = G;
		     rowB[j] = B;
		  }
	       }
	    }
	    
	    id = spans[i].id;
	    sumR = sumG = sumB = 0;
	 }
/*
 * OK, on to next object
 */
	 if(i == nspan) {		/* there is no next object */
	    break;
	 }

	 row = spans[i].row;
	 col0 = spans[i].col0;
	 col1 = spans[i].col1;

	 if(use_mean_color) {		/* use mean of saturated area */
#define MEAN_INTENSITY 0
#if MEAN_INTENSITY			/* use mean of intensities */
	    for(j = col0; j <= col1; j++) {
	       if(regU16R == NULL) {
		  sumR += regR->rows_u8[row][j];
		  sumG += regG->rows_u8[row][j];
		  sumB += regB->rows_u8[row][j];
	       } else {
		  if(satR != NULL &&
		     (phPixIntersectMask(satR, j - col0R, row - row0R) ||
		      phPixIntersectMask(satG, j - col0G, row - row0G) ||
		      phPixIntersectMask(satB, j - col0B, row - row0B))) {
		     continue;
		  }

		  sumR += regU16R->rows_u16[row][j] - minU16;
		  sumG += regU16G->rows_u16[row][j] - minU16;
		  sumB += regU16B->rows_u16[row][j] - minU16;
	       }
	    }
#else					/* use mean of colours */
	    int iR, iG, iB;		/* pixel values */
	    float intensity;		/* intensity of pixel */

	    for(j = col0; j <= col1; j++) {
	       if(regU16R == NULL) {
		  iR = regR->rows_u8[row][j];
		  iG = regG->rows_u8[row][j];
		  iB = regB->rows_u8[row][j];
	       } else {
		  if(satR != NULL &&
		     (phPixIntersectMask(satR, j - col0R, row - row0R) ||
		      phPixIntersectMask(satG, j - col0G, row - row0G) ||
		      phPixIntersectMask(satB, j - col0B, row - row0B))) {
		     continue;
		  }
		  iR = regU16R->rows_u16[row][j]; if(iR > minU16) iR -= minU16;
		  iG = regU16G->rows_u16[row][j]; if(iG > minU16) iG -= minU16;
		  iB = regU16B->rows_u16[row][j]; if(iB > minU16) iB -= minU16;
	       }

	       intensity = iR + iG + iB;
	       if(intensity != 0) {
		  sumR += iR/intensity;
		  sumG += iG/intensity;
		  sumB += iB/intensity;
	       }
	    }
#endif
	 } else {			/* use pixels just beyond the edge */
	    if(col0 > 0) {
	       if(regU16R == NULL) {
		  sumR += regR->rows_u8[row][col0 - 1];
		  sumG += regG->rows_u8[row][col0 - 1];
		  sumB += regB->rows_u8[row][col0 - 1];
	       } else {
		  sumR += regU16R->rows_u16[row][col0 - 1] - minU16;
		  sumG += regU16G->rows_u16[row][col0 - 1] - minU16;
		  sumB += regU16B->rows_u16[row][col0 - 1] - minU16;
	       }
	    }
	    
	    if(col1 < ncol - 1) {
	       if(regU16R == NULL) {
		  sumR += regR->rows_u8[row][col1 + 1];
		  sumG += regG->rows_u8[row][col1 + 1];
		  sumB += regB->rows_u8[row][col1 + 1];
	       } else {
		  sumR += regU16R->rows_u16[row][col1 + 1] - minU16;
		  sumG += regU16G->rows_u16[row][col1 + 1] - minU16;
		  sumB += regU16B->rows_u16[row][col1 + 1] - minU16;
	       }
	    }
	 }
      }
/*
 * Blur edges of fixed-up region a little
 */
#if 1
      for(i = 0; i < nspan; i++) {
	 float fac1 = 0.5;
	 float fac2 = 0.75;
	 row = spans[i].row;
	 col0 = spans[i].col0;
	 col1 = spans[i].col1;

	 rowR = regR->rows_u8[row];
	 rowG = regG->rows_u8[row];
	 rowB = regB->rows_u8[row];

	 if(col0 > 0) {
	    rowR[col0] = fac1*rowR[col0] + (1 - fac1)*rowR[col0 - 1];
	    rowG[col0] = fac1*rowG[col0] + (1 - fac1)*rowG[col0 - 1];
	    rowB[col0] = fac1*rowB[col0] + (1 - fac1)*rowB[col0 - 1];

	    if(col0 < ncol - 1) {
	       rowR[col0 + 1] = fac2*rowR[col0 + 1] + (1- fac2)*rowR[col0 - 1];
	       rowG[col0 + 1] = fac2*rowG[col0 + 1] + (1- fac2)*rowG[col0 - 1];
	       rowB[col0 + 1] = fac2*rowB[col0 + 1] + (1- fac2)*rowB[col0 - 1];
	    }
	 }
	 if(col1 < ncol - 1) {
	    rowR[col1] = fac1*rowR[col1] + (1 - fac1)*rowR[col1 + 1];
	    rowG[col1] = fac1*rowG[col1] + (1 - fac1)*rowG[col1 + 1];
	    rowB[col1] = fac1*rowB[col1] + (1 - fac1)*rowB[col1 + 1];

	    if(col1 > 0) {
	       rowR[col1 - 1] = fac2*rowR[col1 - 1] + (1- fac2)*rowR[col1 + 1];
	       rowG[col1 - 1] = fac2*rowG[col1 - 1] + (1- fac2)*rowG[col1 + 1];
	       rowB[col1 - 1] = fac2*rowB[col1 - 1] + (1- fac2)*rowB[col1 + 1];
	    }
	 }
      }
#endif
   }
/*
 * clean up
 */
   shFree(spans);
}

/*****************************************************************************/
/*
 * Strings we need to write postscript [color]images
 */
static char header[] = "\
%%!PS-Adobe-2.0 EPSF-2.0\n\
%%%%Creator: Photo\n\
%%%%BoundingBox: %d %d %d %d\n\
%%%%Pages: 1 1\n\
%%%%DocumentFonts:\n\
%%%%EndComments\n";

static char preamble[] = "\
15 dict begin\n\
/buff %d string def\n\
%%\n\
%d %d translate\n\
%d dup scale\n\
gsave\n\
%%%%Page: 1 1\n";

static char draw_it[] = "\
%d %d 8 [ %d 0 0 %d 0 0 ]\n\
{ currentfile buff readhexstring pop }\n";

static char print_it[]="\n\
grestore\nshowpage\n\
end\n\
%%%%Trailer\n\
%%%%EOF\n";

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Convert a set of three U8 REGIONs to true-colour postscript,
 * written to <file>
 *
 * Returns 0 if OK, -1 in case of trouble
 */
int
phTruecolorPostscriptWrite(const char *file, /* file to write */
			   const REGION *reg1, /* U8 regions: R */
			   const REGION *reg2, /*             G */
			   const REGION *reg3, /*             B */
			   float size,	/* size of output, in inches */
                           float xoff,	/* origin of Bounding Box */
                           float yoff)	/*                        in inches */
{
   FILE *fd;				/* output file descriptor */
   int i, j, k;
   int nrow, ncol;			/* == reg->n{row,col} */
   const int ncolor = 3;		/* number of colours; 3 for RGB */
   U8 *row1, *row2, *row3;		/* == reg[123]->row_u8[i] */
   int size_pt;				/* size of image, in 1/72" */
   int xoff_pt, yoff_pt;		/* origin of bounding box in 1/72" */

   shAssert(reg1 != NULL && reg1->type == TYPE_U8);
   shAssert(reg2 != NULL && reg2->type == TYPE_U8);
   shAssert(reg3 != NULL && reg3->type == TYPE_U8);
   nrow = reg1->nrow; ncol = reg1->ncol;
   shAssert(reg2->nrow == nrow && reg2->ncol == ncol);
   shAssert(reg3->nrow == nrow && reg3->ncol == ncol);

   if((fd = fopen(file,"w")) == NULL) {
      shError("Can't open %s",file);
      return(-1);
   }
/*
 * write the postscript
 */
   xoff_pt = 72*xoff;			/* convert to PS's 1/72" points */
   yoff_pt = 72*yoff;
   size_pt = 72*size;

   fprintf(fd, header, xoff_pt, yoff_pt, xoff_pt + size_pt, yoff_pt + size_pt);
   fprintf(fd, preamble, ncol, xoff_pt, yoff_pt, size_pt);
   fprintf(fd, draw_it, ncol, nrow,
	   (nrow > ncol ? nrow : ncol), (nrow > ncol ? nrow : ncol));
   fprintf(fd, "false %d\n colorimage\n", ncolor);

   for(i = 0;i < nrow;i++) {
      row1 = reg1->rows_u8[i];
      row2 = reg2->rows_u8[i];
      row3 = reg3->rows_u8[i];
      for(j = k = 0;j < ncol; j++, k++) {
	 fprintf(fd, "%02x%02x%02x",row1[j], row2[j], row3[j]);
	 if(k == 12) {
	    fputc('\n', fd);
	    k = -1;			/* it'll be ++ed in a moment */
	 }
      }
      if(k > 0) {
	 fputc('\n', fd);
      }
   }
   fprintf(fd,print_it);

   fclose(fd);
   
   return(0);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Convert a U8 REGION to postscript, written to <file>
 *
 * Returns 0 if OK, -1 in case of trouble
 */
int
phOnecolorPostscriptWrite(const char *file, /* file to write */
			  const REGION *reg1, /* U8 region */
			  float size,	/* size of output, in inches */
			  float xoff,	/* origin of Bounding Box */
			  float yoff)	/*                        in inches */
{
   FILE *fd;				/* output file descriptor */
   int i, j, k;
   int nrow, ncol;			/* == reg->n{row,col} */
   U8 *row1;				/* == reg1->row_u8[i] */
   int size_pt;				/* size of image, in 1/72" */
   int xoff_pt, yoff_pt;		/* origin of bounding box in 1/72" */

   shAssert(reg1 != NULL && reg1->type == TYPE_U8);
   nrow = reg1->nrow; ncol = reg1->ncol;

   if((fd = fopen(file,"w")) == NULL) {
      shError("Can't open %s",file);
      return(-1);
   }
/*
 * write the postscript
 */
   xoff_pt = 72*xoff;			/* convert to PS's 1/72" points */
   yoff_pt = 72*yoff;
   size_pt = 72*size;

   fprintf(fd, header, xoff_pt, yoff_pt, xoff_pt + size_pt, yoff_pt + size_pt);
   fprintf(fd, preamble, ncol, xoff_pt, yoff_pt, size_pt);
   fprintf(fd, draw_it, ncol, nrow,
	   (nrow > ncol ? nrow : ncol), (nrow > ncol ? nrow : ncol));
   fprintf(fd, "image\n");

   for(i = 0;i < nrow;i++) {
      row1 = reg1->rows_u8[i];
      for(j = k = 0;j < ncol; j++, k++) {
	 fprintf(fd, "%02x",row1[j]);
	 if(k == 3*12) {
	    fputc('\n', fd);
	    k = -1;			/* it'll be ++ed in a moment */
	 }
      }
      if(k > 0) {
	 fputc('\n', fd);
      }
   }
   fprintf(fd,print_it);

   fclose(fd);
   
   return(0);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Convert a U8 REGION to PGM format, written to <file>
 *
 * Returns 0 if OK, -1 in case of trouble
 */
int
phOnecolorPGMWrite(const char *file,	/* file to write */
		   const REGION *reg1)	/* U8 region */
{
   FILE *fd;				/* output file descriptor */
   int i, j;
   int nrow, ncol;			/* == reg->n{row,col} */
   U8 *row1;				/* == reg1->row_u8[i] */

   shAssert(reg1 != NULL && reg1->type == TYPE_U8);
   nrow = reg1->nrow; ncol = reg1->ncol;

   if((fd = fopen(file,"w")) == NULL) {
      shError("Can't open %s",file);
      return(-1);
   }
/*
 * write the PPM file
 */
   fprintf(fd, "P5 %d %d %d\n", ncol, nrow, 255);

   for(i = nrow - 1;i >= 0;i--) {
      row1 = reg1->rows_u8[i];
      for(j = 0;j < ncol; j++) {
	 fprintf(fd, "%c",row1[j]);
      }
   }
   fprintf(fd,"\n");

   fclose(fd);
   
   return(0);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Convert a set of three U8 REGIONs to PPM format, written to <file>
 *
 * Returns 0 if OK, -1 in case of trouble
 */
int
phTruecolorPPMWrite(const char *file,	/* file to write */
		    const REGION *reg1, /* U8 regions: R */
		    const REGION *reg2, /*             G */
		    const REGION *reg3) /*             B */
{
   FILE *fd;				/* output file descriptor */
   int i, j;
   int nrow, ncol;			/* == reg->n{row,col} */
   U8 *row1, *row2, *row3;		/* == reg[123]->row_u8[i] */
   U8 r, g, b;				/* RGB values for a pixel */

   shAssert(reg1 != NULL && reg1->type == TYPE_U8);
   shAssert(reg2 != NULL && reg2->type == TYPE_U8);
   shAssert(reg3 != NULL && reg3->type == TYPE_U8);
   nrow = reg1->nrow; ncol = reg1->ncol;
   shAssert(reg2->nrow == nrow && reg2->ncol == ncol);
   shAssert(reg3->nrow == nrow && reg3->ncol == ncol);

   if((fd = fopen(file,"w")) == NULL) {
      shError("Can't open %s",file);
      return(-1);
   }
/*
 * write the PPM file
 */
   fprintf(fd, "P6 %d %d %d\n", ncol, nrow, 255);

   for(i = nrow - 1;i >= 0;i--) {
      row1 = reg1->rows_u8[i];
      row2 = reg2->rows_u8[i];
      row3 = reg3->rows_u8[i];
      for(j = 0;j < ncol; j++) {
	 r = row1[j]; g = row2[j]; b = row3[j];
	 fprintf(fd, "%c%c%c", r, g, b);
      }
   }
   fprintf(fd,"\n");

   fclose(fd);
   
   return(0);
}

/************************************************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Given a set of 3 U8 regions representing R, G, and B separations of an image, median filter
 * some aspect of the composite colour image
 */

#define INSERT(V) \
{ \
    float v = V; \
    int k,l; \
\
    vals[n] = v;			/* will be overwritten if in wrong place */\
    for(k = 0; k < n; k++) {\
	if(vals[k] < v) {\
	    for(l = n; l > k; l--) {\
		vals[l] = vals[l - 1];\
	    }\
	    vals[k] = v;\
	    break;\
	}\
    }\
    n++;\
}\

#define CALC_ROW(WHAT) \
   switch (which) { \
   case 0:				/* hue */ \
     for(j = 0; j < ncol; j++) {\
       WHAT[j] = (float)(row1[j] + row2[j] + row3[j])/3;\
     }\
     break;\
   case 1:				/* saturation */ \
     for(j = 0; j < ncol; j++) {\
       WHAT[j] = (float)(row1[j] + row2[j] + row3[j])/3;\
     }\
     break;\
   case 2:				/* intensity */ \
     for(j = 0; j < ncol; j++) {\
       WHAT[j] = (float)(row1[j] + row2[j] + row3[j])/3;\
     }\
     break;\
    default:\
     shError("phRegU8Median: filtering with which == %s isn't supported",\
	     which);\
     return -1;\
   }

int
phRegU8Median(REGION *reg1,		/* U8 regions: R */
	      REGION *reg2,		/*             G */
	      REGION *reg3,		/*             B */
	      int size,			/* size of desired filter */
	      int which)		/* 0:hue, 1:saturation, 2:intensity */
{
   int i, j;
   int nrow, ncol;			/* == reg->n{row,col} */
   U8 *row1, *row2, *row3;		/* == reg[123]->row_u8[i] */
   float I;				/* median intensity */
   U8 *inten0;				/* Memory for intensities of three lines of data */
   float *prev, *this, *next;		/* pointers to intensities for previous, current, and next line */
   float valR, valG, valB;		/* RGB values for a pixel */

   shAssert(reg1 != NULL && reg1->type == TYPE_U8);
   shAssert(reg2 != NULL && reg2->type == TYPE_U8);
   shAssert(reg3 != NULL && reg3->type == TYPE_U8);
   nrow = reg1->nrow; ncol = reg1->ncol;
   shAssert(reg2->nrow == nrow && reg2->ncol == ncol);
   shAssert(reg3->nrow == nrow && reg3->ncol == ncol);

   if(size != 3) {
       shError("phRegU8Median: Sorry, only 3x3 filters are supported");
       size = 3;
   }

   inten0 = shMalloc(3*ncol*sizeof(float));
   prev = (void *)inten0;
   this = prev + ncol;
   next = this + ncol;

   row1 = reg1->rows_u8[0]; row2 = reg2->rows_u8[0]; row3 = reg3->rows_u8[0];

   CALC_ROW(this);
   
   for(i = 1; i < nrow - 1; i++) {
       /*
	* Update the intensity pointers
	*/
       U8 *tmp = (void *)prev;
       prev = this;
       this = next;
       next = (void *)tmp;

       row1 = reg1->rows_u8[i + 1];
       row2 = reg2->rows_u8[i + 1];
       row3 = reg3->rows_u8[i + 1];
       
       CALC_ROW(next);
       /*
	* OK, we're ready to median filter
	*/
       row1 = reg1->rows_u8[i]; row2 = reg2->rows_u8[i]; row3 = reg3->rows_u8[i];

       for(j = 1; j < ncol - 1; j++) {
	   float vals[9];			/* sorted values */
	   int n = 0;				/* number of values inserted */

	   INSERT(prev[j - 1]);
	   INSERT(prev[j   ]);
	   INSERT(prev[j + 1]);

	   INSERT(this[j - 1]);
	   INSERT(this[j   ]);
	   INSERT(this[j + 1]);

	   INSERT(next[j - 1]);
	   INSERT(next[j   ]);
	   INSERT(next[j + 1]);

	   I = vals[4];			/* median intensity */

	   if(this[j] == 0) {		/* we don't know the colour; leave it black */
	       valR = valG = valB = 0;      
	   } else {
	       valR = row1[j]; valG = row2[j]; valB = row3[j];
	       valR *= I/this[j];
	       valG *= I/this[j];
	       valB *= I/this[j];

	       {
		   float valMax = (valR > valG) ? valR : valG;
		   if(valB > valMax) {
		       valMax = valB;
		   }
		   if(valMax >= 255) {
		       valMax /= 255;
		       valR /= valMax;
		       valG /= valMax;
		       valB /= valMax;
		   }
	       }
	       shAssert ((int)valR <= 255 && (int)valG <= 255 && (int)valB <= 255);
	   }

	   row1[j] = (int)valR; row2[j] = (int)valG; row3[j] = (int)valB;
       }
   }

   shFree(inten0);

   return 0;
}

/*****************************************************************************/
/*
 * Set the outline of a chain of OBJECTs (possibly with more than
 * one level set) to a given value
 */
void
phObjectChainSetInRegion(REGION *reg,	/* region to set */
			 const CHAIN *ch, /* from these OBJECTs */
			 int val,	/* set region's pixels to this value */
			 int drow, int dcol) /* offset OBJECTs this much */
{
   int i, j, k;
   int nrow, ncol;			/* == reg->n{row,col} */
   const OBJECT *obj;			/* == element of ch */
   const OBJMASK *om;			/* == om->sv[] */
   int x1, x2, y;			/* unpacked from om->s[] */
   
   shAssert(reg != NULL);
   shAssert(reg->type == TYPE_PIX);
   shAssert(ch != NULL && ch->type == shTypeGetFromName("OBJECT"));

   nrow = reg->nrow; ncol = reg->ncol;
   for(i = 0; i < shChainSize(ch); i++) {
      obj = shChainElementGetByPos(ch, i);
      for(j = 0; j < obj->nlevel; j++) {
	 om = obj->sv[j];
	 for(k = 0; k < om->nspan; k++) {
	    y = om->s[k].y + drow;
	    if(y < 0 || y >= nrow) {
	       continue;
	    }
	    x1 = om->s[k].x1 + dcol;
	    x2 = om->s[k].x2 + dcol;

	    if(x1 >= 0 && x1 < ncol) {
	       reg->rows_u16[y][x1] = val;
	    }
	    if(x2 >= 0 && x2 < ncol) {
	       reg->rows_u16[y][x2] = val;
	    }
	 }
      }
   }
}
