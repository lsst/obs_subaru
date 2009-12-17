/*****************************************************************************
******************************************************************************
**
** FILE
**	utils.c
**
** ABSTRACT:
**      This file contains routines that support manipulation of regions,
** 	even if they contain pixles of differing type
**
**      In general, this module converts pixels to double, and performs
**      the requested operations.
**
**      THIS MODULE IS NOT YET UP TO SPECIFICATION SINCE:
**         It has NOT been tested AT ALL.
**         Enhance performance in the cases where the regions are of the
**            same type.
**         Add a compare function, mostly to allow regression testing of this
**            and other modules
**
**	
**				NYI == Not yet implemented
** ENTRY POINT			SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
**  shRegTypeSwitch             public  Switch the pixel type of the region.
**  shRegSetWithDbl		public  Set all pixels in a region to scaler.
**  shRegSub			public	Subtract two regions
**  shRegXor			public	Exclusive-OR two regions
**  shRegAddWithDbl		public	Add scaler to a region
**  shRegAdd			public  Add two regions together
**  shRegAddRow			public	Add row vector to each row of region
**  shRegAddCol			public	Add column vector to each col of region
**  shRegSubCol			public	Subtract col vector from each region col
**  shRegSubRow			public	Subtract row vector from each region row
**  shRegPixCopy		public	Copy pixels from one region to another
**  shRegPixSetWithDbl	       	public  Set one pixel in a region
**  shRegPixGetAsDbl	       	public  Get one pixle in a region
**  shRegComp			public	Compare two regions
**  shRegFlip		    public	Flips the region CW 90 degrees
**  shRegRowFlip		public	Flips the rows of a region by copying 
**  shRegColFlip		public  Flips the columns of a region by copying
**  shRegMultWithDbl		public	Mulitplys a region by a scaler
**  shRegMult			public	Mupltiply two regions together
**  shRegMultWithRow		public	Mupltiply a region and a row together
**  shRegMultWithCol		public	Mupltiply a region and a column together
**  shRegDiv	 		public  Divide one region by another
**  shRegDivByRow	 	public  Divide a region by a row
**  shRegDivByCol	 	public  Divide a region by a column
**  shRegDivZero	 	public  Divide one region by another, set divide by zero with value
**
** ENVIRONMENT:
**	ANSI C.
**
** REQUIRED PRODUCTS:
**	NONE
**
** AUTHORS:
**	Creation date:  May 7, 1993
**	Don Petravick
******************************************************************************
******************************************************************************/


#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <shCAssert.h>
#include <string.h>
#include <stdio.h>

#include "shCRegUtils.h"
#include "shCMaskUtils.h"
#include "shCErrStack.h"

/*============================================================================
**============================================================================
**
** LOCAL MACROS, DEFINITIONS, ETC.
**
**============================================================================
*/

/* macro to "round to the nearest" integer 
** It actually increase the absolute value by .5, 
** and if cast to an integer type the result will be the the integer
** part of this result (thus the nearest integer)
** Nota Bene: XXXX.5 is rounded to XXXX (even if odd)
*/ 
#define ROUND_NEAREST(d) ((d)>= 0.0 ? (d) + 0.5: -(-(d) + 0.5))


/************************* Macro Framework for region - region operations */
/**************************************************************************/

/* i.e. FOR_EACH_TYPE3(sub3, minuend, subtrahend, ans) */
#define FOR_EACH_TYPE3(OP, r1, r2, rout) \
{ \
   switch (r1->type) { \
   case (TYPE_S8): \
     FOREACHPIX3(OP, S8, s8, r1, r2, rout); \
     break; \
   case (TYPE_U8): \
     FOREACHPIX3(OP, U8, u8, r1, r2, rout); \
     break; \
   case (TYPE_S16): \
     FOREACHPIX3(OP, S16, s16, r1, r2, rout); \
     break; \
   case (TYPE_U16): \
     FOREACHPIX3(OP, U16, u16, r1, r2, rout); \
     break; \
   case (TYPE_FL32): \
     FOREACHPIX3(OP, FL32, fl32, r1, r2, rout); \
     break; \
   case (TYPE_S32): \
     FOREACHPIX3(OP, S32, s32, r1, r2, rout); \
     break; \
   case (TYPE_U32): \
     FOREACHPIX3(OP, U32, u32, r1, r2, rout); \
     break; \
   default: \
     shFatal ("Missing case"); \
   } \
}

/* i.e. FOREACHPIX3(sub3, FL32. fl32, minuend, subtrahend, ans) */
#define FOREACHPIX3(OP, TYPE, TSTEM,  r1, r2, rout) \
{ \
   TYPE *ppr1, *ppr2, *pprout,*pppast; \
   int r; \
   for (r=0; r<r1->nrow; r++) { \
     ppr1   = r1->rows_##TSTEM[r]; /* pointer to a pixel in r1 */ \
     ppr2   = r2->rows_##TSTEM[r]; /*   ""  */ \
     pprout = rout->rows_##TSTEM[r]; /* "" */ \
     pppast = &(rout->rows_##TSTEM[r][rout->ncol]); /* just past end */ \
       while (pprout < pppast) { \
          /*i.e. call macro sub3fl32(...), for example */ \
          OP##TSTEM(*ppr1, *ppr2, *pprout); \
          ppr1++; ppr2++; pprout++; \
       } \
   } \
}

/******** Subtraction ***********/
/* Here is a set of macros implementing subtraction, for
   the above framework. Remeber, all of the pixels are
   the same type 
*/

#define sub3u(minuend, subtrahend, ans)  \
if (subtrahend > minuend) { \
   ans = 0; \
}else{ \
   ans = minuend - subtrahend; \
}

#define sub3s(max, min, WIDETYPE, minuend, subtrahend, ans)  \
{ WIDETYPE t; \
  t = minuend - subtrahend; \
  t = (t > (WIDETYPE)max ? (WIDETYPE)max : t); \
  t = (t < (WIDETYPE)min ? (WIDETYPE)min : t); \
  ans = t; \
}

#define sub3s8(minuend, subtrahend, ans)  sub3s(S8_MAX, S8_MIN, int, minuend, subtrahend, ans)  
#define sub3u8(minuend, subtrahend, ans)  sub3u(minuend, subtrahend, ans)  

#define sub3s16(minuend, subtrahend, ans)  sub3s(S16_MAX, S16_MIN, int, minuend, subtrahend, ans)  
#define sub3u16(minuend, subtrahend, ans)  sub3u(minuend, subtrahend, ans)  

#define sub3fl32(minuend, subtrahend, ans) ans = minuend - subtrahend;
#define sub3s32(minuend, subtrahend, ans)  sub3s(S32_MAX, S32_MIN, double, minuend, subtrahend, ans)  
#define sub3u32(minuend, subtrahend, ans)  sub3u(minuend, subtrahend, ans)  

/******** Addition ***********/
/* Here is a set of macros implementing addition, for
   the above framework. Remeber, all of the pixels are
   the same type 
*/

#define add3u(max,  addend1, addend2, ans)  \
{ unsigned int  t; \
  t = addend1 + addend2; \
  t = (t > max ? max : t); \
  ans = t; \
}

#define add3s(max, min, WIDETYPE, addend1, addend2, ans)  \
{ WIDETYPE t; \
  t = addend1 + addend2; \
  t = (t > (WIDETYPE)max ? (WIDETYPE)max : t); \
  t = (t < (WIDETYPE)min ? (WIDETYPE)min : t); \
  ans = t; \
}

#define add3s8(addend1, addend2, ans)  add3s(S8_MAX, S8_MIN, int, addend1, addend2, ans)  
#define add3u8(addend1, addend2, ans)  add3u(U8_MAX, addend1, addend2, ans)  

#define add3s16(addend1, addend2, ans)  add3s(S16_MAX, S16_MIN, int, addend1, addend2, ans)  
#define add3u16(addend1, addend2, ans)  add3u(U16_MAX, addend1, addend2, ans)  

#define add3fl32(addend1, addend2, ans) ans = addend1 + addend2;
#define add3s32(addend1, addend2, ans)  add3s(S32_MAX, S32_MIN, double, addend1, addend2, ans)  
#define add3u32(addend1, addend2, ans)  add3u(U32_MAX, addend1, addend2, ans)  


/******** division , which does not yet handle scale ***********/

#define div3(max, min, WIDETYPE,  dividend, divisor, ans)  \
{ WIDETYPE t; \
  t = (dividend + (divisor/2)) / divisor; \
  t = (t > max ? max : t); \
  t = (t < min ? min : t); \
  ans = t; \
}
#define div3s8(dividend, divisor, ans)  div3(S8_MAX, S8_MIN, int, dividend, divisor,  ans)  
#define div3u8(dividend, divisor, ans)  div3(U8_MAX, U8_MIN, int, dividend, divisor,  ans)  
#define div3s16(dividend, divisor, ans)  div3(S16_MAX, S16_MIN, int, dividend, divisor,  ans)  
#define div3u16(dividend, divisor, ans)  div3(U16_MAX, U16_MIN, int,  dividend, divisor,  ans)  
#define div3fl32(dividend, divisor, ans)  ans = dividend / divisor;
#define div3s32(dividend, divisor, ans)  div3(S32_MAX, S32_MIN, double, dividend, divisor,  ans)  
#define div3u32(dividend, divisor, ans)  div3(U32_MAX, U32_MIN, double,  dividend, divisor,  ans)  

/******** Exclusive-OR ***********/
/* Here is a set of macros implementing exclusive-OR, for
   the above framework. Remeber, all of the pixels are
   the same type 
*/

#define xor3u(pix1, pix2, ans)  \
   ans = ((unsigned)pix1) ^ ((unsigned)pix2)

#define xor3s(max, min, WIDETYPE, pix1, pix2, ans) xor3u(pix1, pix2, ans)

#define xor3s8(pix1, pix2, ans)  xor3s(S8_MAX, S8_MIN, int, pix1, pix2, ans)  
#define xor3u8(pix1, pix2, ans)  xor3u(pix1, pix2, ans)  

#define xor3s16(pix1, pix2, ans)  xor3s(S16_MAX, S16_MIN, int, pix1, pix2, ans)  
#define xor3u16(pix1, pix2, ans)  xor3u(pix1, pix2, ans)  

#define xor3fl32(pix1, pix2, ans) ans = ((unsigned)pix1) ^ ((unsigned)pix2)
#define xor3s32(pix1, pix2, ans)  xor3s(S32_MAX, S32_MIN, double, pix1, pix2, ans)  
#define xor3u32(pix1, pix2, ans)  xor3u(pix1, pix2, ans)  

/******** Macro Framework for constant operating on a region **************/
/**************************************************************************/

static void utilerror (char *string) {printf ("%s \n", string); abort ();}

/*Check if all regions have the same number of rows and cols*/
static int samesize3 (const REGION *reg1,const REGION *reg2,const REGION *reg3)
{
   if (reg1->nrow == reg2->nrow &&
       reg1->nrow == reg3->nrow &&
       reg1->ncol == reg2->ncol &&
       reg1->ncol == reg3->ncol) return 1;
   return 0;
}

/*Check if all regions have the same number of rows and cols*/
int p_samesize2 (const REGION *reg1, const REGION *reg2)
{   
   if (reg1->nrow == reg2->nrow &&
       reg1->ncol == reg2->ncol) return 1;
   return 0;   
}

/*Check if two regions have the same number of rows */
static int samerows2 (const REGION *reg1, const REGION *reg2)
{   
   if (reg1->nrow == reg2->nrow) return 1;
   return 0;   
}

/*Check if two regions have the same number of columns */
static int samecols2 (const REGION *reg1, const REGION *reg2)
{   
   if (reg1->ncol == reg2->ncol) return 1;
   return 0;   
}

/*Check if three regions have the same type */
static int sametype3 (const REGION *reg1, 
                      const REGION *reg2, const REGION *reg3)
{   
   if (reg1->type == reg2->type &&
       reg1->type == reg3->type) return 1;
   return 0;   
}

/*Check if three regions have the same type */
static int sametype2 (const REGION *reg1,  const REGION *reg2)
{   
   if (reg1->type == reg2->type) return 1;
   return 0;   
}

static U8 set_u8 (double dbl)
{
   if (dbl < (double)U8_MIN) return (U8_MIN);
   if (dbl > (double)U8_MAX) return (U8_MAX);
   return ROUND_NEAREST(dbl);
}

static S8 set_s8 (double dbl)
{
   if (dbl < (double)S8_MIN) return (S8_MIN);
   if (dbl > (double)S8_MAX) return (S8_MAX);
   return ROUND_NEAREST(dbl);
}

static U16 set_u16 (double dbl)
{
   if (dbl < (double)U16_MIN) return (U16_MIN);
   if (dbl > (double)U16_MAX) return (U16_MAX);
   return ROUND_NEAREST(dbl);
}

static S16 set_s16 (double dbl)
{
   if (dbl < (double)S16_MIN) return (S16_MIN);
   if (dbl > (double)S16_MAX) return (S16_MAX);
   return ROUND_NEAREST(dbl);
}

static U32 set_u32 (double dbl)
{
   if (dbl < (double)U32_MIN) return (U32_MIN);
   if (dbl > (double)U32_MAX) return (U32_MAX);
   return ROUND_NEAREST(dbl);
}

static S32 set_s32 (double dbl)
{
   if (dbl < (double)S32_MIN) return (S32_MIN);
   if (dbl > (double)S32_MAX) return (S32_MAX);
   return ROUND_NEAREST(dbl);
}

static FL32 set_fl32 (double dbl)
{
   return (FL32)(dbl);
}


static double pxGetAsDbl (const REGION *reg, int r, int c)
{
   switch (reg->type) {
    case TYPE_U8: 	return ((double) reg->rows_u8[r][c]);
    case TYPE_S8: 	return ((double) reg->rows_s8[r][c]);
    case TYPE_U16:	return ((double) reg->rows   [r][c]);
    case TYPE_S16:	return ((double) reg->rows_s16[r][c]);
    case TYPE_U32:	return ((double) reg->rows_u32[r][c]);
    case TYPE_S32:	return ((double) reg->rows_s32[r][c]);
    case TYPE_FL32:	return ((double) reg->rows_fl32[r][c]);
    default:		utilerror ("region type is unsupported");
   }
   return (0.0);
}


static void pxSetWithDbl (REGION *reg, int r, int c, double dbl)
{
   switch (reg->type) {
    case TYPE_U8: 	reg->rows_u8[r][c]   = set_u8(dbl);   return;
    case TYPE_S8: 	reg->rows_s8[r][c]   = set_s8(dbl);   return;
    case TYPE_U16:	reg->rows   [r][c]   = set_u16(dbl);  return;
    case TYPE_S16:	reg->rows_s16[r][c]  = set_s16(dbl);  return;
    case TYPE_U32:	reg->rows_u32[r][c]  = set_u32(dbl);  return;
    case TYPE_S32:	reg->rows_s32[r][c]  = set_s32(dbl);  return;
    case TYPE_FL32:	reg->rows_fl32[r][c] = set_fl32(dbl); return;
    default:		utilerror ("region type is unsupported"); 
   }
   return;
}

static void ** shRegionRowVector (const REGION *reg)
{
   switch (reg->type) {
    case TYPE_U8: 	return (void **) reg->rows_u8;
    case TYPE_S8: 	return (void **) reg->rows_s8;
    case TYPE_U16:	return (void **) reg->rows;
    case TYPE_S16:	return (void **) reg->rows_s16;
    case TYPE_U32:	return (void **) reg->rows_u32;
    case TYPE_S32:	return (void **) reg->rows_s32;
    case TYPE_FL32:	return (void **) reg->rows_fl32;
    default:		utilerror ("region type is unsupported"); 
   }
   return (void **)0;
}

static size_t shSizeOfPix(PIXDATATYPE pxType)
{
   switch (pxType) {
    case TYPE_U8: 	return sizeof(U8);
    case TYPE_S8: 	return sizeof(S8);
    case TYPE_U16: 	return sizeof(U16);
    case TYPE_S16:	return sizeof(S16);
    case TYPE_U32:	return sizeof(U32);
    case TYPE_S32:	return sizeof(S32);
    case TYPE_FL32:	return sizeof(FL32);
    default:		utilerror ("region type is unsupported"); 
   }
   return (0);
}

/*============================================================================
**============================================================================
**
**
** DESCRIPTION:
**	Change the pixel type of the region to the requested type.  Only the
**      changes are allowed -
**	                S8  <-> U8
**	                S16 <-> U16
**	                S32 <-> U32
**
** RETURN VALUES:
**	SH_SUCCESS  -  success
**
**	
**
** CALLS TO:
**
**============================================================================
*/
RET_CODE shRegTypeSwitch
   (
   REGION      *a_regPtr,
   PIXDATATYPE a_type
   )
{
RET_CODE rStatus = SH_SUCCESS;

switch (a_regPtr->type)
  {
  case TYPE_U8:
    if ((a_type != a_regPtr->type) && (a_type != TYPE_S8))
      {
      /* Requested switch cannot be done */
      rStatus = SH_CANT_SWITCH_TYPE;
      shErrStackPush("ERROR : Can only switch region of type U8 to S8");
      }
    else
      {
      /* Only do switch if the rows are now not of that type. */
      if (a_type == TYPE_S8)
	{
        a_regPtr->rows_s8 = (S8 **)a_regPtr->rows_u8;
        a_regPtr->rows_u8 = (U8 **)NULL;
        a_regPtr->type = a_type;
        }
      }
    break;
  case TYPE_S8:
    if ((a_type != a_regPtr->type) && (a_type != TYPE_U8))
      {
      /* Requested switch cannot be done */
      rStatus = SH_CANT_SWITCH_TYPE;
      shErrStackPush("ERROR : Can only switch region of type S8 to U8");
      }
    else
      {
      /* Only do switch if the rows are now not of that type. */
      if (a_type == TYPE_U8)
	{
        a_regPtr->rows_u8 = (U8 **)a_regPtr->rows_s8;
        a_regPtr->rows_s8 = (S8 **)NULL;
        a_regPtr->type = a_type;
        }
      }
    break;
  case TYPE_U16:
    if ((a_type != a_regPtr->type) && (a_type != TYPE_S16))
      {
      /* Requested switch cannot be done */
      rStatus = SH_CANT_SWITCH_TYPE;
      shErrStackPush("ERROR : Can only switch region of type U16 to S16");
      }
    else
      {
      /* Only do switch if the rows are now not of that type. */
      if (a_type == TYPE_S16)
	{
        a_regPtr->rows_s16 = (S16 **)a_regPtr->rows_u16;
        a_regPtr->rows = (U16 **)NULL;            /* for the legacy software */
        a_regPtr->rows_u16 = (U16 **)NULL;
        a_regPtr->type = a_type;
        }
      }
    break;
  case TYPE_S16:
    if ((a_type != a_regPtr->type) && (a_type != TYPE_U16))
      {
      /* Requested switch cannot be done */
      rStatus = SH_CANT_SWITCH_TYPE;
      shErrStackPush("ERROR : Can only switch region of type S16 to U16");
      }
    else
      {
      /* Only do switch if the rows are now not of that type. */
      if (a_type == TYPE_U16)
	{
        a_regPtr->rows_u16 = (U16 **)a_regPtr->rows_s16;
        a_regPtr->rows = a_regPtr->rows_u16;      /* for the legacy software */
        a_regPtr->rows_s16 = (S16 **)NULL;
        a_regPtr->type = a_type;
        }
      }
    break;
  case TYPE_U32:
    if ((a_type != a_regPtr->type) && (a_type != TYPE_S32))
      {
      /* Requested switch cannot be done */
      rStatus = SH_CANT_SWITCH_TYPE;
      shErrStackPush("ERROR : Can only switch region of type U32 to S32");
      }
    else
      {
      /* Only do switch if the rows are now not of that type. */
      if (a_type == TYPE_S32)
	{
        a_regPtr->rows_s32 = (S32 **)a_regPtr->rows_u32;
        a_regPtr->rows_u32 = (U32 **)NULL;
        a_regPtr->type = a_type;
        }
      }
    break;
  case TYPE_S32:
    if ((a_type != a_regPtr->type) && (a_type != TYPE_U32))
      {
      /* Requested switch cannot be done */
      rStatus = SH_CANT_SWITCH_TYPE;
      shErrStackPush("ERROR : Can only switch region of type S32 to U32");
      }
    else
      {
      /* Only do switch if the rows are now not of that type. */
      if (a_type == TYPE_U32)
	{
        a_regPtr->rows_u32 = (U32 **)a_regPtr->rows_s32;
        a_regPtr->rows_s32 = (S32 **)NULL;
        a_regPtr->type = a_type;
        }
      }
    break;
  case TYPE_FL32:
    rStatus = SH_CANT_SWITCH_TYPE;
    shErrStackPush(
        "ERROR : Region is of type FL32 and cannot be switched to anything!!");
    break;
  default:
    /* We should never reach here */
    shFatal("In shRegTypeSwitch: The file pixel type is unknown!!");
  }

return(rStatus);
}

/* Get One pixel as a double*/
double shRegPixGetAsDbl (const REGION *reg, int r, int c)
{
/*   shAssert (r>=0 && r<reg->nrow); */
/*   shAssert (c>=0 && c<reg->ncol);   */
   return pxGetAsDbl (reg, r, c);    
} 

/*** Get One pixel with a double */
void shRegPixSetWithDbl (REGION *reg, int r, int c, double val)
{
/*   shAssert (r>=0 && r<reg->nrow); */
/*   shAssert (c>=0 && c<reg->ncol); */
   pxSetWithDbl (reg, r, c, val);    
} 


/* Every pixel in the image is set to the constant "constant" 
 * 
 */



void shRegSetWithDbl (double constant, REGION *reg)
{
   int r;
   switch (reg->type) {
    case TYPE_U8: 	
     {
       U8 *ppast, *ppout, au8   = set_u8(constant);
       for (r=0; r<reg->nrow; r++) {
         ppout  = reg->rows_u8[r];
         ppast = &(reg->rows_u8[r][reg->ncol]);
         while (ppout < ppast) *ppout++ = au8;
       }
       break;
     }      
    case TYPE_S8:\
      { 	
	S8 *ppast, *ppout, as8   = set_s8(constant);
	for (r=0; r<reg->nrow; r++) {
         ppout  = reg->rows_s8[r];
         ppast = &(reg->rows_s8[r][reg->ncol]);
         while (ppout < ppast) *ppout++ = as8;
       }
	break;      
      }
    case TYPE_U16:
     {	
       U16 *ppast, *ppout, au16  = set_u16(constant);
       for (r=0; r<reg->nrow; r++) {
         ppout  = reg->rows_u16[r];
         ppast = &(reg->rows_u16[r][reg->ncol]);
         while (ppout < ppast) *ppout++ = au16;
       }
       break; 
     }     
    case TYPE_S16:
     {	
       S16 *ppast, *ppout, as16  = set_s16(constant);
       for (r=0; r<reg->nrow; r++) {
         ppout  = reg->rows_s16[r];
         ppast = &(reg->rows_s16[r][reg->ncol]);
         while (ppout < ppast) *ppout++ = as16;
       }
       break; 
     }     
    case TYPE_U32:	
     {
       U32 *ppast, *ppout, au32  = set_u32(constant);

       for (r=0; r<reg->nrow; r++) {
         ppout  = reg->rows_u32[r];
         ppast = &(reg->rows_u32[r][reg->ncol]);
         while (ppout < ppast) *ppout++ = au32;
       }
       break; 
     }     
    case TYPE_S32:	
     {
       S32  *ppast, *ppout, as32  = set_s32(constant);
       for (r=0; r<reg->nrow; r++) { 
         ppout  = reg->rows_s32[r];
         ppast = &(reg->rows_s32[r][reg->ncol]);
         while (ppout < ppast) *ppout++ = as32;
       }
       break;
     }      
    case TYPE_FL32:
     { 
       FL32 *ppast, *ppout, afl32 = set_fl32(constant);
       for (r=0; r<reg->nrow; r++) {
         ppout  = reg->rows_fl32[r];
         ppast = &(reg->rows_fl32[r][reg->ncol]);
         while (ppout < ppast) *ppout++ = afl32;
       }
       break; 
     }     
    default:		
     utilerror ("region type is unsupported"); 
   }
   return;
}


/*
 * Subtract SUBTRAHEND from MINUEND pixel-by-pixel. Place the
 * resulting pixel in ANS.  ANS may be SUBTRAHEND or MINUEND.
 *
 * - This procedure may be used on REGIONS with any type of pixel.
 * - The caller ensures all regions must be of the same size.
 */ 

void shRegSub(const REGION *minuend, const REGION *subtrahend, REGION *ans)
{
   int r,c;
   shAssert(samesize3(minuend, subtrahend, ans));
   if ( sametype3 (minuend, subtrahend, ans)) {  
      FOR_EACH_TYPE3(sub3, minuend, subtrahend, ans);
   }else{  
     double temp;
     for (r=0; r<minuend->nrow; r++) {
       for (c=0; c<minuend->ncol; c++) {
	 temp = pxGetAsDbl(minuend, r, c) - pxGetAsDbl(subtrahend, r, c);
	 pxSetWithDbl(ans, r, c, temp);
       }
     }
   }
   return;
}
/*
 * Exclusive-OR PIX1 and PIX2 pixel-by-pixel. Place the
 * resulting pixel in ANS.  ANS may be PIX1 or PIX2.
 *
 * - This procedure may be used on REGIONS with integer types of pixel.
 * - The caller ensures all regions must be of the same size.
 */ 

void shRegXor(const REGION *pix1, const REGION *pix2, REGION *ans)
{
   shAssert(samesize3(pix1, pix2, ans));

   if (! sametype3(pix1, pix2, ans)) {
      utilerror ("regions must be of same type");
   } else {
      switch (pix1->type) {
         case TYPE_S8:
         case TYPE_U8:
         case TYPE_S16:
         case TYPE_U16:
         case TYPE_S32:
         case TYPE_U32: break;
         default:
           utilerror ("region type is unsupported (must be integral)");
           break;
      }

      FOR_EACH_TYPE3(xor3, pix1, pix2, ans);
   }
   return;
}


/* The constant "constant" is added to each pixel in "img" 
 */
void shRegAddWithDbl (double constant, const REGION *reg, REGION *ans)
{
   int r,c;
    if (sametype2 (reg, ans)) {
      switch (reg->type) {
      case TYPE_U8: 	
	{
	  int q = ROUND_NEAREST(constant);
	  U8 *ppin, *ppout, *ppast;
	  int t;
	  for (r=0; r<reg->nrow; r++) {
	    ppin  = reg->rows_u8[r];
	    ppout = ans->rows_u8[r];
	    ppast = &(ans->rows_u8[r][reg->ncol]);
	    while (ppout < ppast) { 
	      t = *ppin++ + q;
              /* it is essential to cast */
	      t = (t > (int)U8_MAX ? (int)U8_MAX : t); 
	      t = (t < (int)U8_MIN ? (int)U8_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break;
	}
      case TYPE_S8:
	{ 	
	  int q = ROUND_NEAREST(constant);
	  S8 *ppin, *ppout, *ppast;
	  int t;
	  for (r=0; r<reg->nrow; r++) {
	    ppin  = reg->rows_s8[r];
	    ppout = ans->rows_s8[r];
	    ppast = &(ans->rows_s8[r][reg->ncol]);
	    while (ppout < ppast) { 
	      t = *ppin++ + q;
	      t = (t > (int)S8_MAX ? (int)S8_MAX : t); 
	      t = (t < (int)S8_MIN ? (int)S8_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break;      
	}
      case TYPE_U16:
	{	
	  int q = ROUND_NEAREST(constant);
	  U16 *ppin, *ppout, *ppast;
	  int t;
	  for (r=0; r<reg->nrow; r++) {
	    ppin  = reg->rows_u16[r];
	    ppout = ans->rows_u16[r];
	    ppast = &(ans->rows_u16[r][reg->ncol]);
	    while (ppout < ppast) { 
	      t = *ppin++ + q;
              /* it is essential to cast */
	      t = (t > (int)U16_MAX ? (int)U16_MAX : t); 
	      t = (t < (int)U16_MIN ? (int)U16_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break; 
	}     
      case TYPE_S16:
	{	
	  int q = ROUND_NEAREST(constant);
	  S16 *ppin, *ppout, *ppast;	
	  int t;
	  for (r=0; r<reg->nrow; r++) {
	    ppin  = reg->rows_s16[r];
	    ppout = ans->rows_s16[r];
	    ppast = &(ans->rows_s16[r][reg->ncol]);
	    while (ppout < ppast) { 
	      t = *ppin++ + q;
	      t = (t > (int)S16_MAX ? (int)S16_MAX : t); 
	      t = (t < (int)S16_MIN ? (int)S16_MIN : t);
	      *ppout++ = t; 
	    }
	  }

	  break; 
	}     
      case TYPE_U32:	
	{
	  double  q = ROUND_NEAREST(constant);
	  U32 *ppin, *ppout, *ppast;
	  double t;
	  for (r=0; r<reg->nrow; r++) {
	    ppin  = reg->rows_u32[r];
	    ppout = ans->rows_u32[r];
	    ppast = &(ans->rows_u32[r][reg->ncol]);
	    while (ppout < ppast) { 
	      t = *ppin++ + q;
              /* it is essential to cast */
	      t = (t > (double)U32_MAX ? (double)U32_MAX : t); 
	      t = (t < (double)U32_MIN ? (double)U32_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break; 
	}     
      case TYPE_S32:	
	{
	  double q = ROUND_NEAREST(constant);
	  S32 *ppin, *ppout, *ppast;
	  double t;
	  for (r=0; r<reg->nrow; r++) {
	    ppin  = reg->rows_s32[r];
	    ppout = ans->rows_s32[r];
	    ppast = &(ans->rows_s32[r][reg->ncol]);
	    while (ppout < ppast) { 
	      t = *ppin++ + q;
	      t = (t > (double)S32_MAX ? (double)S32_MAX : t); 
	      t = (t < (double)S32_MIN ? (double)S32_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break;
	}      
      case TYPE_FL32:
	{	
	  FL32 *ppin, *ppout, *ppast;
	  for (r=0; r<reg->nrow; r++) {
	    ppin  = reg->rows_fl32[r];
	    ppout = ans->rows_fl32[r];
	    ppast = &(ans->rows_fl32[r][reg->ncol]);
	    while (ppout < ppast) *ppout++ = *ppin++ + constant;
	  }
	  break; 
	}     
      default:		
	utilerror ("region type is unsupported"); 
      }
    }else{
      double temp;      
      for (r=0; r<reg->nrow; r++) {
	for (c=0; c<reg->ncol; c++) {
	  temp = pxGetAsDbl(reg, r, c) + constant;
	  pxSetWithDbl(ans, r, c, temp);
	}
   }

    }
   return;
}   
/*
 * Add ADDEND1 to ADDEND2 pixel-by-pixel. Place the
 * resulting pixels in ANS.  ANS may be ADDEND1 or ADDEND2.
 *
 * - This procedure may be used on REGIONS with any type of pixel.
 * - The caller ensure than all regions must be of the same size.
 */ 


void shRegAdd(const REGION *addend1, const REGION *addend2, REGION *ans)
{
   shAssert(samesize3(addend1, addend2, ans));
   
   if ( sametype3 (addend1, addend2, ans)) {  
      FOR_EACH_TYPE3(add3, addend1, addend2, ans);
   }else{  
     double temp;
     int r,c;
     for (r=0; r<addend1->nrow; r++) {
       for (c=0; c<addend1->ncol; c++) {
	 temp = pxGetAsDbl(addend1, r, c) + pxGetAsDbl(addend2, r, c);
	 pxSetWithDbl(ans, r, c, temp);
       }
     }
   }
   return;
}

/*
 * Add the row vector ADDEN2 to the region ADDEN1 pixel-by-pixel,
 *	column-by-column
 * Place the resulting pixels in ANS.  ANS may be ADDEND1.
 *
 * - This procedure may be used on REGIONS with any type of pixel.
 * - The caller ensure than all regions must be of the same size.
 */ 


void shRegAddRow(const REGION *addend1, const REGION *addend2, REGION *ans)
{
   int r;
   shAssert(p_samesize2(addend1, ans));
   shAssert(samecols2(addend1, addend2));
   
   if ( sametype3 (addend1, addend2, ans)) {  
      switch (addend1->type) {
      case TYPE_U8: 	
	{
	  int t; /* wide type */
	  U8 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<addend1->nrow; r++) {
	    ppa1  = addend1->rows_u8[r];
	    ppa2  = addend2->rows_u8[0];
	    ppout = ans->rows_u8[r];
	    ppast = &(ans->rows_u8[r][ans->ncol]);
	    while (ppout < ppast) { 
              /* it is essential to cast */
	      t = (int)*ppa1++ + (int)*ppa2++;
	      t = (t > (int)U8_MAX ? (int)U8_MAX : t); 
	      t = (t < (int)U8_MIN ? (int)U8_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break;
	}
      case TYPE_S8:
	{ 	
	  int t; /* wide type */
	  S8 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<addend1->nrow; r++) {
	    ppa1  = addend1->rows_s8[r];
	    ppa2  = addend2->rows_s8[0];
	    ppout = ans->rows_s8[r];
	    ppast = &(ans->rows_s8[r][ans->ncol]);
	    while (ppout < ppast) { 
	      t = *ppa1++ + *ppa2++;
	      t = (t > (int)S8_MAX ? (int)S8_MAX : t); 
	      t = (t < (int)S8_MIN ? (int)S8_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break;      
	}
      case TYPE_U16:
	{	
	  int t; /* wide type */
	  U16 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<addend1->nrow; r++) {
	    ppa1  = addend1->rows_u16[r];
	    ppa2  = addend2->rows_u16[0];
	    ppout = ans->rows_u16[r];
	    ppast = &(ans->rows_u16[r][ans->ncol]);
	    while (ppout < ppast) { 
              /* it is essential to cast */
	      t = (int)*ppa1++ + (int)*ppa2++;
	      t = (t > (int)U16_MAX ? (int)U16_MAX : t); 
	      t = (t < (int)U16_MIN ? (int)U16_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break; 
	}     
      case TYPE_S16:
	{	
	  int t; /* wide type */
	  S16 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<addend1->nrow; r++) {
	    ppa1  = addend1->rows_s16[r];
	    ppa2  = addend2->rows_s16[0];
	    ppout = ans->rows_s16[r];
	    ppast = &(ans->rows_s16[r][ans->ncol]);
	    while (ppout < ppast) { 
	      t = *ppa1++ + *ppa2++;
	      t = (t > (int)S16_MAX ? (int)S16_MAX : t); 
	      t = (t < (int)S16_MIN ? (int)S16_MIN : t);
	      *ppout++ = t; 
	    }
	  }

	  break; 
	}     
      case TYPE_U32:	
	{
	  double t; /* wide type */
	  U32 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<addend1->nrow; r++) {
	    ppa1  = addend1->rows_u32[r];
	    ppa2  = addend2->rows_u32[0];
	    ppout = ans->rows_u32[r];
	    ppast = &(ans->rows_u32[r][ans->ncol]);
	    while (ppout < ppast) {
	      t = (double)*ppa1++ + (double)*ppa2++;
	      t = (t > (double)U32_MAX ? (double)U32_MAX : t); 
	      t = (t < (double)U32_MIN ? (double)U32_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break;
	}     
      case TYPE_S32:	
	{
	  double t; /* wide type */
	  S32 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<addend1->nrow; r++) {
	    ppa1  = addend1->rows_s32[r];
	    ppa2  = addend2->rows_s32[0];
	    ppout = ans->rows_s32[r];
	    ppast = &(ans->rows_s32[r][ans->ncol]);
	    while (ppout < ppast) {
	      t = (double)*ppa1++ + (double)*ppa2++;
	      t = (t > (double)S32_MAX ? (double)S32_MAX : t); 
	      t = (t < (double)S32_MIN ? (double)S32_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break;
	}      
      case TYPE_FL32:
	{	
	  FL32 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<addend1->nrow; r++) {
	    ppa1  = addend1->rows_fl32[r];
	    ppa2  = addend2->rows_fl32[0];
	    ppout = ans->rows_fl32[r];
	    ppast = &(ans->rows_fl32[r][ans->ncol]);
	    while (ppout < ppast) *ppout++ = *ppa1++ + *ppa2++;
	  }
	  break; 
	}     
      default:		
	utilerror ("region type is unsupported"); 
      }

   }else{  
     double temp;
     int r,c;
     for (r=0; r<addend1->nrow; r++) {
       for (c=0; c<addend1->ncol; c++) {
	 temp = pxGetAsDbl(addend1, r, c) + pxGetAsDbl(addend2, 0, c);
	 pxSetWithDbl(ans, r, c, temp);
       }
     }
   }

   return;
}

/*
 * Add the column vector ADDEN2 to the region ADDEN1 pixel-by-pixel,
 *	row-by-row
 * Place the resulting pixels in ANS.  ANS may be ADDEND1.
 *
 * - This procedure may be used on REGIONS with any type of pixel.
 * - The caller ensure than all regions must be of the same size.
 */ 


void shRegAddCol(const REGION *addend1, const REGION *addend2, REGION *ans)
{
   double temp;
   int r,c;
   shAssert(p_samesize2(addend1, ans));
   shAssert(samerows2(addend1, addend2));
   
   for (r=0; r<addend1->nrow; r++) {
      for (c=0; c<addend1->ncol; c++) {
	 temp = pxGetAsDbl(addend1, r, c) + pxGetAsDbl(addend2, r, 0);
	 pxSetWithDbl(ans, r, c, temp);
      }
   }
   return;
}



/*
 * Subtract the row vector ADDEN2 from the region ADDEN1 pixel-by-pixel,
 *	column-by-column
 * Place the resulting pixels in ANS.  ANS may be ADDEND1.
 *
 * - This procedure may be used on REGIONS with any type of pixel.
 * - The caller ensure than all regions must be of the same size.
 */ 


void shRegSubRow(const REGION *addend1, const REGION *addend2, REGION *ans)
{
   double temp;
   int r,c;
   shAssert(p_samesize2(addend1, ans));
   shAssert(samecols2(addend1, addend2));

  if ( sametype3 (addend1, addend2, ans)) {  
      switch (addend1->type) {
      case TYPE_U8: 	
	{
	  int t; /* wide type */
	  U8 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<addend1->nrow; r++) {
	    ppa1  = addend1->rows_u8[r];
	    ppa2  = addend2->rows_u8[0];
	    ppout = ans->rows_u8[r];
	    ppast = &(ans->rows_u8[r][ans->ncol]);
	    while (ppout < ppast) { 
              /* it is essential to cast */
	      t = (int)*ppa1++ - (int)*ppa2++;
	      t = (t > (int)U8_MAX ? (int)U8_MAX : t); 
	      t = (t < (int)U8_MIN ? (int)U8_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break;
	}
      case TYPE_S8:
	{ 	
	  int t; /* wide type */
	  S8 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<addend1->nrow; r++) {
	    ppa1  = addend1->rows_s8[r];
	    ppa2  = addend2->rows_s8[0];
	    ppout = ans->rows_s8[r];
	    ppast = &(ans->rows_s8[r][ans->ncol]);
	    while (ppout < ppast) { 
	      t = *ppa1++ - *ppa2++;
	      t = (t > (int)S8_MAX ? (int)S8_MAX : t); 
	      t = (t < (int)S8_MIN ? (int)S8_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break;      
	}
      case TYPE_U16:
	{	
	  int t; /* wide type */
	  U16 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<addend1->nrow; r++) {
	    ppa1  = addend1->rows_u16[r];
	    ppa2  = addend2->rows_u16[0];
	    ppout = ans->rows_u16[r];
	    ppast = &(ans->rows_u16[r][ans->ncol]);
	    while (ppout < ppast) { 
              /* it is essential to cast */
	      t = (int)*ppa1++ - (int)*ppa2++;
	      t = (t > (int)U16_MAX ? (int)U16_MAX : t); 
	      t = (t < (int)U16_MIN ? (int)U16_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break; 
	}     
      case TYPE_S16:
	{	
	  int t; /* wide type */
	  S16 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<addend1->nrow; r++) {
	    ppa1  = addend1->rows_s16[r];
	    ppa2  = addend2->rows_s16[0];
	    ppout = ans->rows_s16[r];
	    ppast = &(ans->rows_s16[r][ans->ncol]);
	    while (ppout < ppast) { 
	      t = *ppa1++ - *ppa2++;
	      t = (t > (int)S16_MAX ? (int)S16_MAX : t); 
	      t = (t < (int)S16_MIN ? (int)S16_MIN : t);
	      *ppout++ = t; 
	    }
	  }

	  break; 
	}     
      case TYPE_U32:	
	{
	  double t; /* wide type */
	  U32 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<addend1->nrow; r++) {
	    ppa1  = addend1->rows_u32[r];
	    ppa2  = addend2->rows_u32[0];
	    ppout = ans->rows_u32[r];
	    ppast = &(ans->rows_u32[r][ans->ncol]);
	    while (ppout < ppast) {
              /*the cast is required*/
	      t = (double)*ppa1++ - (double)*ppa2++;
	      t = (t > (double)U32_MAX ? (double)U32_MAX : t); 
	      t = (t < (double)U32_MIN ? (double)U32_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break;
	}     
      case TYPE_S32:	
	{
	  double t; /* wide type */
	  S32 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<addend1->nrow; r++) {
	    ppa1  = addend1->rows_s32[r];
	    ppa2  = addend2->rows_s32[0];
	    ppout = ans->rows_s32[r];
	    ppast = &(ans->rows_s32[r][ans->ncol]);
	    while (ppout < ppast) {
	      t = *ppa1++ - *ppa2++;
	      t = (t > (double)S32_MAX ? (double)S32_MAX : t); 
	      t = (t < (double)S32_MIN ? (double)S32_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break;
	}      
      case TYPE_FL32:
	{	
	  FL32 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<addend1->nrow; r++) {
	    ppa1  = addend1->rows_fl32[r];
	    ppa2  = addend2->rows_fl32[0];
	    ppout = ans->rows_fl32[r];
	    ppast = &(ans->rows_fl32[r][ans->ncol]);
	    while (ppout < ppast) *ppout++ = *ppa1++ - *ppa2++;
	  }
	  break; 
	}     
      default:		
	utilerror ("region type is unsupported"); 
      }

      
  }else{
    for (r=0; r<addend1->nrow; r++) {
      for (c=0; c<addend1->ncol; c++) {
	temp = pxGetAsDbl(addend1, r, c) - pxGetAsDbl(addend2, 0, c);
	pxSetWithDbl(ans, r, c, temp);
      }
    }
  }
   return;
}


/*
 * Subtract the column vector ADDEN2 from the region ADDEN1 pixel-by-pixel,
 *	row-by-row
 * Place the resulting pixels in ANS.  ANS may be ADDEND1.
 *
 * - This procedure may be used on REGIONS with any type of pixel.
 * - The caller ensure than all regions must be of the same size.
 */ 


void shRegSubCol(const REGION *addend1, const REGION *addend2, REGION *ans)
{
   double temp;
   int r,c;
   shAssert(p_samesize2(addend1, ans));
   shAssert(samerows2(addend1, addend2));

  if ( sametype3 (addend1, addend2, ans)) {  
      switch (addend1->type) {
      case TYPE_U8: 	
	{
	  int t, t2; /* wide type */
	  U8 *ppa1, *ppout, *ppast;
	  for (r=0; r<addend1->nrow; r++) {
	    ppa1  = addend1->rows_u8[r];
	    t2    = (int) addend2->rows_u8[r][0];
	    ppout = ans->rows_u8[r];
	    ppast = &(ans->rows_u8[r][ans->ncol]);
	    while (ppout < ppast) { 
              /* it is essential to cast */
	      t = (int)*ppa1++ - (int)t2;
	      t = (t > (int)U8_MAX ? (int)U8_MAX : t); 
	      t = (t < (int)U8_MIN ? (int)U8_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break;
	}
      case TYPE_S8:
	{ 	
	  int t, t2; /* wide type */
	  S8 *ppa1, *ppout, *ppast;
	  for (r=0; r<addend1->nrow; r++) {
	    ppa1  = addend1->rows_s8[r];
	    t2    = (int) addend2->rows_s8[r][0];
	    ppout = ans->rows_s8[r];
	    ppast = &(ans->rows_s8[r][ans->ncol]);
	    while (ppout < ppast) { 
	      t = *ppa1++ - t2;
	      t = (t > (int)S8_MAX ? (int)S8_MAX : t); 
	      t = (t < (int)S8_MIN ? (int)S8_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break;      
	}
      case TYPE_U16:
	{	
	  int t, t2; /* wide type */
	  U16 *ppa1, *ppout, *ppast;
	  for (r=0; r<addend1->nrow; r++) {
	    ppa1  = addend1->rows_u16[r];
	    t2    = (int) addend2->rows_u16[r][0];
	    ppout = ans->rows_u16[r];
	    ppast = &(ans->rows_u16[r][ans->ncol]);
	    while (ppout < ppast) { 
              /* it is essential to cast */
	      t = (int)*ppa1++ - (int)t2;
	      t = (t > (int)U16_MAX ? (int)U16_MAX : t); 
	      t = (t < (int)U16_MIN ? (int)U16_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break; 
	}     
      case TYPE_S16:
	{	
	  int t, t2; /* wide type */
	  S16 *ppa1, *ppout, *ppast;
	  for (r=0; r<addend1->nrow; r++) {
	    ppa1  = addend1->rows_s16[r];
	    t2    = (int) addend2->rows_s16[r][0];
	    ppout = ans->rows_s16[r];
	    ppast = &(ans->rows_s16[r][ans->ncol]);
	    while (ppout < ppast) { 
	      t = *ppa1++ - t2;
	      t = (t > (int)S16_MAX ? (int)S16_MAX : t); 
	      t = (t < (int)S16_MIN ? (int)S16_MIN : t);
	      *ppout++ = t; 
	    }
	  }

	  break; 
	}     
      case TYPE_U32:	
	{
	  double t, t2; /* wide type */
	  U32 *ppa1, *ppout, *ppast;
	  for (r=0; r<addend1->nrow; r++) {
	    ppa1  = addend1->rows_u32[r];
	    t2    = (double) addend2->rows_u32[r][0];
	    ppout = ans->rows_u32[r];
	    ppast = &(ans->rows_u32[r][ans->ncol]);
	    while (ppout < ppast) {
              /*the cast is required*/
	      t = (double)*ppa1++ - (double)t2;
	      t = (t > (double)U32_MAX ? (double)U32_MAX : t); 
	      t = (t < (double)U32_MIN ? (double)U32_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break;
	}     
      case TYPE_S32:	
	{
	  double t, t2; /* wide type */
	  S32 *ppa1, *ppout, *ppast;
	  for (r=0; r<addend1->nrow; r++) {
	    ppa1  = addend1->rows_s32[r];
	    t2    = (double) addend2->rows_s32[r][0];
	    ppout = ans->rows_s32[r];
	    ppast = &(ans->rows_s32[r][ans->ncol]);
	    while (ppout < ppast) {
	      t = *ppa1++ - t2;
	      t = (t > (double)S32_MAX ? (double)S32_MAX : t); 
	      t = (t < (double)S32_MIN ? (double)S32_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break;
	}      
      case TYPE_FL32:
	{	
	  FL32 t2, *ppa1, *ppout, *ppast;
	  for (r=0; r<addend1->nrow; r++) {
	    ppa1  = addend1->rows_fl32[r];
	    t2    = addend2->rows_fl32[r][0];
	    ppout = ans->rows_fl32[r];
	    ppast = &(ans->rows_fl32[r][ans->ncol]);
	    while (ppout < ppast) *ppout++ = *ppa1++ - t2;
	  }
	  break; 
	}     
      default:		
	utilerror ("region type is unsupported"); 
      }
   } 
   else {
     for (r=0; r<addend1->nrow; r++) {
        for (c=0; c<addend1->ncol; c++) {
  	  temp = pxGetAsDbl(addend1, r, c) - pxGetAsDbl(addend2, r, 0);
	  pxSetWithDbl(ans, r, c, temp);
        }
     }
   }
   return;
}

/*
 * Copy the pixels from REGION1 to REGION pixel-by-pixel.
 * perform type conversions if necessary.
 *
 * - This procedure may be used on REGIONS with any type of pixel.
 * - The caller gaurantees all regions must be of the same size.
 * - THe use of memmove is indiciated as things may overlap.
 */ 

void shRegPixCopy(const REGION *regIn, REGION *regOut)
{
   double temp;
   int r,c;
   shAssert(p_samesize2(regIn, regOut));
   
   if (regIn->type != regOut->type) {
     /* special case promote unsigned to float quickly */
      if (regIn->type == TYPE_U16 && regOut->type == TYPE_FL32) {
          U16  *ppin;
	  FL32 *ppout;
	  for (r=0; r<regIn->nrow; r++) {
	    ppin  = regIn->rows_u16[r];
	    ppout = regOut->rows_fl32[r];
	    for (c=0; c<regIn->ncol; c++) {
	      *ppout++ = *ppin++; 
	    }	  
	  }
      }else{
	/* general mixed type case */
	for (r=0; r<regIn->nrow; r++) {
	  for (c=0; c<regIn->ncol; c++) {
	    temp = pxGetAsDbl(regIn, r, c);
	    pxSetWithDbl(regOut, r, c, temp);
	  }
	}
      }
   }else{
      /* homogeneous types */
      void  **Outp, **Inp;
      size_t len    = shSizeOfPix(regIn->type) * regIn->ncol;
      
      Outp    =  shRegionRowVector(regOut);
      Inp     =  shRegionRowVector(regIn);
      for (r=0; r<regIn->nrow; r++) memmove (Outp[r], Inp[r], len);
   }
/*
 *	Check if the mask needs to be copied or cleared
 */
   if (regIn->mask != NULL && regOut->mask != NULL) {
      shMaskCopy (regIn->mask, regOut->mask);
   }
   if (regIn->mask == NULL && regOut->mask != NULL) {
      shMaskClear (regOut->mask);
   }
   return;
}

/*
 * Compare the pixels REGION1 to REGION pixel-by-pixel.
 * perform type conversions if necessary.
 *
 * - This procedure may be used on REGIONS with any type of pixel.
 * - The caller ensures all regions must be of the same size.
 */ 

int  shRegComp (const REGION *reg1, const REGION *reg2, int *rowFirstDiff, int *colFirstDiff)
{
   int r,c;
   shAssert(p_samesize2(reg1, reg2));
   
   for (r=0; r<reg1->nrow; r++) {
      for (c=0; c<reg1->ncol; c++) {
	 if (pxGetAsDbl(reg1, r, c) != pxGetAsDbl(reg2, r, c)) {
	    *rowFirstDiff = r;
	    *colFirstDiff = c;
	    return (1);
	 }
      }
   }
   return (0); /*Regions compare*/
}

/*
 * Flip the image by rotating CW 90 degrees
 * perform type conversions if necessary.
 *
 * - This procedure may be used on REGIONS with any type of pixel.
 * - The caller ensures all regions must be of the same size.
 * - Flipping is done by copying pixels.
 */    

REGION* shRegFlip (REGION *reg)
{   
   REGION *newReg;
   double tmp;
   int nrow, ncol, i, j;
   
   nrow = reg->nrow;
   ncol = reg->ncol;
   newReg = shRegNew(reg->name, reg->ncol, reg->nrow, reg->type);

   
	for(i=0; i< nrow; i++) {
   		for(j=0; j< ncol; j++) {
			tmp =  pxGetAsDbl(reg, i, j);
			pxSetWithDbl (newReg, j, i, tmp);
		}
   	}
	shRegRowFlip(newReg);
	shRegDel(reg);
	reg = newReg;
	return(reg);
}

/*
 * Flip the image row-wise pixel-by-pixel.
 * perform type conversions if necessary.
 *
 * - This procedure may be used on REGIONS with any type of pixel.
 * - The caller ensures all regions must be of the same size.
 * - Flipping is done by copying pixles, not by altering row vectors.
 *	- Flipping a subregion flips the corespondingbits in the parent.
 * 	- This is symmetric with flipping columns.
 */    

void shRegRowFlip (REGION *reg)
{   
   double tmp;
   int rowTop, rowBottom, c;
   
   rowTop = 0;
   rowBottom = reg->nrow-1;
   
   while (rowTop < rowBottom) {
      for (c=0; c < reg->ncol; c++) {
	 tmp =  pxGetAsDbl(reg, rowTop, c);
	 pxSetWithDbl (reg, rowTop,    c, pxGetAsDbl(reg, rowBottom, c));
	 pxSetWithDbl (reg, rowBottom, c, tmp                          );
      }
      rowTop++;
      rowBottom--;
   }
/*
 *	If region contains a mask, flip the mask's rows as well
 */
   if (reg->mask != NULL) {
      shMaskRowFlip (reg->mask);
   }
}

/*
 * Flip the image column-wise pixel-by-pixel.
 * perform type conversions if necessary.
 *
 * - This procedure may be used on REGIONS with any type of pixel.
 * - The caller ensures all regions must be of the same size.
 * - Flipping is done by copying pixels, not by altering row vectors.
 *	- Flipping a subregion flips the coresponding pixles  in the parent.
 * 	- This is symmetric with flipping columns.
 */    

void shRegColFlip (REGION *reg)
{
   double tmp;
   int colRight, colLeft, r;
   
   colLeft = 0;
   colRight = reg->ncol-1;
   
   while (colLeft < colRight) {
      for (r=0; r < reg->nrow; r++) {
	 tmp =  pxGetAsDbl(reg, r, colRight);
	 pxSetWithDbl (reg, r, colRight, pxGetAsDbl(reg, r, colLeft));
	 pxSetWithDbl (reg, r, colLeft , tmp                         );
      }
      colLeft++;
      colRight--;
   }
/*
 *	If region has a mask, flip the mask's columns as well
 */
   if (reg->mask != NULL) {
      shMaskColFlip(reg->mask);
   }
}

/*
 * - Multiply each pixel in inReg by constant. 
 * - place the result in regOut.  
 * - regIn and regOut may be the same.
 * - This procedure may be used on REGIONS with any type of pixel.
 * - The caller ensures all regions must be of the same size.
 */    

void shRegMultWithDbl (double constant, const REGION *regIn, REGION *regOut)
{
   int r, c;
   double tmp;
   shAssert(p_samesize2(regIn, regOut));
   
   for (r=0; r<regIn->nrow; r++) {
      for (c=0; c<regIn->ncol; c++) {
	 tmp = pxGetAsDbl(regIn, r, c);
	 pxSetWithDbl (regOut, r, c, tmp*constant);
      }
   }
}


/*
 * - Multiply each pixel in Mult1 by the corresponding pixel in Mutl1  and divide it by scale, 
 * - place the result in the corresponding place in Result.
 * - resul may be mult1 or mult2 may be the same.
 * - This procedure may be used on REGIONS with any type of pixel.
 * - The caller ensures all regions must be of the same size.
 */    


void shRegMult (double scale, const REGION *mult1, const REGION *mult2,  REGION *result)
{
   int r, c;
   double tmp;
   shAssert(samesize3(mult1, mult2, result));
   
   for (r=0; r<mult1->nrow; r++) {
      for (c=0; c<mult1->ncol; c++) {
	 tmp = pxGetAsDbl(mult1, r, c) * pxGetAsDbl(mult2, r, c) / scale;
	 pxSetWithDbl (result, r, c, tmp);
      }
   }
}


/*
 * - Multiply each pixel in Mult1 by the corresponding pixel in the row
	vector Mult2 and divide it by scale.
 * - place the result in the corresponding place in Result.
 * - result may be mult1.
 * - This procedure may be used on REGIONS with any type of pixel.
 * - The caller ensures that region and row have same number of columns.
 */    


void shRegMultWithRow (double scale, const REGION *mult1, const REGION *mult2,  REGION *result)
{
   int r, c;
   double tmp;
   shAssert(p_samesize2(mult1, result));
   shAssert(samecols2(mult1,mult2));
   
  if ( sametype3 (mult1, mult2, result) && (scale == 1)) {  
      switch (mult1->type) {
      case TYPE_U8: 	
	{
	  int t; /* wide type */
	  U8 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<mult1->nrow; r++) {
	    ppa1  = mult1->rows_u8[r];
	    ppa2  = mult2->rows_u8[0];
	    ppout = result->rows_u8[r];
	    ppast = &(result->rows_u8[r][result->ncol]);
	    while (ppout < ppast) { 
              /* it is essential to cast */
	      t = (int)*ppa1++ * (int)*ppa2++;
	      t = (t > (int)U8_MAX ? (int)U8_MAX : t); 
	      t = (t < (int)U8_MIN ? (int)U8_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break;
	}
      case TYPE_S8:
	{ 	
	  int t; /* wide type */
	  S8 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<mult1->nrow; r++) {
	    ppa1  = mult1->rows_s8[r];
	    ppa2  = mult2->rows_s8[0];
	    ppout = result->rows_s8[r];
	    ppast = &(result->rows_s8[r][result->ncol]);
	    while (ppout < ppast) { 
	      t = *ppa1++ * *ppa2++;
	      t = (t > (int)S8_MAX ? (int)S8_MAX : t); 
	      t = (t < (int)S8_MIN ? (int)S8_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break;      
	}
      case TYPE_U16:
	{	
	  int t; /* wide type */
	  U16 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<mult1->nrow; r++) {
	    ppa1  = mult1->rows_u16[r];
	    ppa2  = mult2->rows_u16[0];
	    ppout = result->rows_u16[r];
	    ppast = &(result->rows_u16[r][result->ncol]);
	    while (ppout < ppast) { 
              /* it is essential to cast */
	      t = (int)*ppa1++ * (int)*ppa2++;
	      t = (t > (int)U16_MAX ? (int)U16_MAX : t); 
	      t = (t < (int)U16_MIN ? (int)U16_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break; 
	}     
      case TYPE_S16:
	{	
	  int t; /* wide type */
	  S16 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<mult1->nrow; r++) {
	    ppa1  = mult1->rows_s16[r];
	    ppa2  = mult2->rows_s16[0];
	    ppout = result->rows_s16[r];
	    ppast = &(result->rows_s16[r][result->ncol]);
	    while (ppout < ppast) { 
	      t = *ppa1++ * *ppa2++;
	      t = (t > (int)S16_MAX ? (int)S16_MAX : t); 
	      t = (t < (int)S16_MIN ? (int)S16_MIN : t);
	      *ppout++ = t; 
	    }
	  }

	  break; 
	}     
      case TYPE_U32:	
	{
	  double t; /* wide type */
	  U32 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<mult1->nrow; r++) {
	    ppa1  = mult1->rows_u32[r];
	    ppa2  = mult2->rows_u32[0];
	    ppout = result->rows_u32[r];
	    ppast = &(result->rows_u32[r][result->ncol]);
	    while (ppout < ppast) {
              /*the cast is required*/
	      t = (double)*ppa1++ * (double)*ppa2++;
	      t = (t > (double)U32_MAX ? (double)U32_MAX : t); 
	      t = (t < (double)U32_MIN ? (double)U32_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break;
	}     
      case TYPE_S32:	
	{
	  double t; /* wide type */
	  S32 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<mult1->nrow; r++) {
	    ppa1  = mult1->rows_s32[r];
	    ppa2  = mult2->rows_s32[0];
	    ppout = result->rows_s32[r];
	    ppast = &(result->rows_s32[r][result->ncol]);
	    while (ppout < ppast) {
	      t = *ppa1++ * *ppa2++;
	      t = (t > (double)S32_MAX ? (double)S32_MAX : t); 
	      t = (t < (double)S32_MIN ? (double)S32_MIN : t);
	      *ppout++ = t; 
	    }
	  }
	  break;
	}      
      case TYPE_FL32:
	{	
	  FL32 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<mult1->nrow; r++) {
	    ppa1  = mult1->rows_fl32[r];
	    ppa2  = mult2->rows_fl32[0];
	    ppout = result->rows_fl32[r];
	    ppast = &(result->rows_fl32[r][result->ncol]);
	    while (ppout < ppast) *ppout++ = *ppa1++ * *ppa2++;
	  }
	  break; 
	}     
      default:		
	utilerror ("region type is unsupported"); 
      }

  }else{
    for (r=0; r<mult1->nrow; r++) {
      for (c=0; c<mult1->ncol; c++) {
	tmp = pxGetAsDbl(mult1, r, c) * pxGetAsDbl(mult2, 0, c) / scale;
	pxSetWithDbl (result, r, c, tmp);
      }
    }
  }
 }


/*
 * - Multiply each pixel in Mult1 by the corresponding pixel in the column
	vector Mult2 and divide it by scale.
 * - place the result in the corresponding place in Result.
 * - result may be mult1.
 * - This procedure may be used on REGIONS with any type of pixel.
 * - The caller ensures that region and column  have same number of rows.
 */    


void shRegMultWithCol (double scale, const REGION *mult1, const REGION *mult2,  REGION *result)
{
   int r, c;
   double tmp;
   shAssert(p_samesize2(mult1, result));
   shAssert(samerows2(mult1,mult2));
   
   for (r=0; r<mult1->nrow; r++) {
      for (c=0; c<mult1->ncol; c++) {
	 tmp = pxGetAsDbl(mult1, r, c) * pxGetAsDbl(mult2, r, 0) / scale;
	 pxSetWithDbl (result, r, c, tmp);
      }
   }
}

/*
 * - Divide each pixel in DIVIDEND by the corresponding pixel in DIVISOR, then multiply it by scale 
 * - place the result in the corresponding place in Result.
 * - RESULT may be DIVIDEND or DIVISOR.
 * - This procedure may be used on regions with any type of pixel.
 * - The caller ensures all regions must be of the same size.
 */    


void shRegDiv (double scale, const REGION *dividend, const REGION *divisor, REGION *result)
{
   int r, c;
   shAssert(samesize3(divisor, dividend, result));
   if ( sametype3 (divisor, dividend, result) && scale == 1.0) {  
      /* oops -- the macro frame work cannot deal with scale very well */
      FOR_EACH_TYPE3(div3, dividend, divisor, result);
   }else{  
     double tmp;
     for (r=0; r<divisor->nrow; r++) {
       for (c=0; c<divisor->ncol; c++) {
	 tmp = pxGetAsDbl(dividend, r, c) / pxGetAsDbl(divisor, r, c) *  scale;
	 pxSetWithDbl (result, r, c, tmp);
       }
     }
   }

}


/*
 * - Divide each pixel in DIVIDEND by the corresponding pixel in DIVISOR, then multiply it by scale 
 * - place the result in the corresponding place in Result.
 * - RESULT may be DIVIDEND or DIVISOR.
 * - If divide by zero, replace with value
 * - This procedure may be used on regions with any type of pixel.
 * - The caller ensures all regions must be of the same size.
 */    


void shRegDivZero (double divZero, double scale, const REGION *dividend, const REGION *divisor, REGION *result)
{
   int r, c;
   shAssert(samesize3(divisor, dividend, result));
     double tmp;
     for (r=0; r<divisor->nrow; r++) {
       for (c=0; c<divisor->ncol; c++) {
	 tmp =  pxGetAsDbl(divisor, r, c) ;
     if (tmp == 0) { 
        tmp = divZero; 
     } else {
	    tmp = pxGetAsDbl(dividend, r, c) / pxGetAsDbl(divisor, r, c) *  scale;
     }
	 pxSetWithDbl (result, r, c, tmp);
     }
   }

}


/*
 * - Divide each pixel in DIVIDEND by the corresponding pixel in the row
 *	vector DIVISOR, then multiply it by scale 
 * - place the result in the corresponding place in Result.
 * - RESULT may be in DIVIDEND
 * - This procedure may be used on regions with any type of pixel.
 * - The caller ensures DIVIDEND and RESULT are same size
 * - The caller ensures DIVIDEND and DIVISOR have same number of columns
 */    


void shRegDivByRow (double scale, const REGION *dividend, const REGION *divisor, REGION *result)
{
   int r, c;
   double tmp;
   shAssert(p_samesize2(dividend, result));
   shAssert(samecols2(divisor, dividend));
   


  if ( sametype3 (dividend, divisor, result) && (scale == 1)) {  
      switch (dividend->type) {
      case TYPE_U8: 	
	{
	  U8 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<dividend->nrow; r++) {
	    ppa1  = dividend->rows_u8[r];
	    ppa2  = divisor->rows_u8[0];
	    ppout = result->rows_u8[r];
	    ppast = &(result->rows_u8[r][result->ncol]);
	    while (ppout < ppast) { 
              div3(U8_MAX, U8_MIN, int, *ppa1, *ppa2,  *ppout)  
	      ppa1++ ; ppa2++; ppout++; 
	    }
	  }
	  break;
	}
      case TYPE_S8:
	{ 	
	  S8 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<dividend->nrow; r++) {
	    ppa1  = dividend->rows_s8[r];
	    ppa2  = divisor->rows_s8[0];
	    ppout = result->rows_s8[r];
	    ppast = &(result->rows_s8[r][result->ncol]);
	    while (ppout < ppast) { 
              div3(S8_MAX, S8_MIN, int, *ppa1, *ppa2,  *ppout)  
	      ppa1++ ; ppa2++; ppout++; 
	    }
	  }
	  break;      
	}
      case TYPE_U16:
	{	
	  U16 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<dividend->nrow; r++) {
	    ppa1  = dividend->rows_u16[r];
	    ppa2  = divisor->rows_u16[0];
	    ppout = result->rows_u16[r];
	    ppast = &(result->rows_u16[r][result->ncol]);
	    while (ppout < ppast) { 
              div3(U16_MAX, U16_MIN, int, *ppa1, *ppa2,  *ppout)  
	      ppa1++ ; ppa2++; ppout++; 
	    }
	  }
	  break; 
	}     
      case TYPE_S16:
	{	
	  S16 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<dividend->nrow; r++) {
	    ppa1  = dividend->rows_s16[r];
	    ppa2  = divisor->rows_s16[0];
	    ppout = result->rows_s16[r];
	    ppast = &(result->rows_s16[r][result->ncol]);
	    while (ppout < ppast) { 
              div3(S16_MAX, S16_MIN, int, *ppa1, *ppa2,  *ppout)  
	      ppa1++ ; ppa2++; ppout++; 

	    }
	  }

	  break; 
	}     
      case TYPE_U32:	
	{
	  U32 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<dividend->nrow; r++) {
	    ppa1  = dividend->rows_u32[r];
	    ppa2  = divisor->rows_u32[0];
	    ppout = result->rows_u32[r];
	    ppast = &(result->rows_u32[r][result->ncol]);
	    while (ppout < ppast) {
              div3(U32_MAX, U32_MIN, double, *ppa1, *ppa2,  *ppout)  
	      ppa1++ ; ppa2++; ppout++; 

	    }
	  }
	  break;
	}     
      case TYPE_S32:	
	{
	  S32 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<dividend->nrow; r++) {
	    ppa1  = dividend->rows_s32[r];
	    ppa2  = divisor->rows_s32[0];
	    ppout = result->rows_s32[r];
	    ppast = &(result->rows_s32[r][result->ncol]);
	    while (ppout < ppast) {
              div3(S32_MAX, S32_MIN, double, *ppa1, *ppa2,  *ppout)  
	      ppa1++ ; ppa2++; ppout++; 
	    }
	  }
	  break;
	}      
      case TYPE_FL32:
	{	
	  FL32 *ppa1, *ppa2, *ppout, *ppast;
	  for (r=0; r<dividend->nrow; r++) {
	    ppa1  = dividend->rows_fl32[r];
	    ppa2  = divisor->rows_fl32[0];
	    ppout = result->rows_fl32[r];
	    ppast = &(result->rows_fl32[r][result->ncol]);
	    while (ppout < ppast) *ppout++ = *ppa1++ / *ppa2++;
	  }
	  break; 
	}     
      default:		
	utilerror ("region type is unsupported"); 
      }

  }else{
    for (r=0; r<dividend->nrow; r++) {
      for (c=0; c<dividend->ncol; c++) {
	tmp = pxGetAsDbl(dividend, r, c) / pxGetAsDbl(divisor, 0, c) *  scale;
	pxSetWithDbl (result, r, c, tmp);
      }
    }
  }
}
/*
 * - Divide each pixel in DIVIDEND by the corresponding pixel in the column
 *	vector DIVISOR, then multiply it by scale 
 * - place the result in the corresponding place in Result.
 * - RESULT may be in DIVIDEND
 * - This procedure may be used on regions with any type of pixel.
 * - The caller ensures DIVIDEND and RESULT are same size
 * - The caller ensures DIVIDEND and DIVISOR have same number of rows
 */    


void shRegDivByCol (double scale, const REGION *dividend, const REGION *divisor, REGION *result)
{
   int r, c;
   double tmp;
   shAssert(p_samesize2(dividend, result));
   shAssert(samerows2(divisor, dividend));
   
   for (r=0; r<dividend->nrow; r++) {
      for (c=0; c<dividend->ncol; c++) {
	 tmp = pxGetAsDbl(dividend, r, c) / pxGetAsDbl(divisor, r, 0) *  scale;
	 pxSetWithDbl (result, r, c, tmp);
      }
   }
}



/*
 * Flip the mask row-wise pixel-by-pixel.
 *
 * - Flipping is done by copying pixles, not by altering row vectors.
 * 	- This is symmetric with flipping columns.
 */    

void shMaskRowFlip (MASK *mask)
{   
   int rowTop, rowBottom, c;

   shAssert(mask != (MASK *)0);
   
   rowTop = 0;
   rowBottom = mask->nrow-1;
   
   while (rowTop < rowBottom) {
      for (c=0; c < mask->ncol; c++) {
	 mask->rows[rowBottom][c] = mask->rows[rowTop][c];
      }
      rowTop++;
      rowBottom--;
   }
}

/*
 * Flip the mask column-wise pixel-by-pixel.
 *
 * - Flipping is done by copying pixels, not by altering row vectors.
 * 	- This is symmetric with flipping columns.
 */    

void shMaskColFlip (MASK *mask)
{
   int colRight, colLeft, r;

   shAssert(mask != (MASK *)0);
   
   colLeft  = 0;
   colRight = mask->ncol-1;
   
   while (colLeft < colRight) {
      for (r=0; r < mask->nrow; r++) {
	 mask->rows[r][colLeft] = mask->rows[r][colRight];
      }
      colLeft++;
      colRight--;
   }
}







