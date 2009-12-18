/*
 * Functions to find and subtract sky
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include "dervish.h"
#include "phConsts.h"
#include "phUtils.h"
#include "phSkyUtils.h"
#include "phSpanUtil.h"

static void interpolate_2n( int *arr, int n, int lb_dx, int y0, int y1);
static void subtract_row(PIX *out, PIX *in, int *smooth,
			 int n, int nbit, unsigned int mask, RANDOM *rand );

/*****************************************************************************/
/*
 * con/destructors for BINREGIONs
 */

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Support a datatype for binned REGIONs
 */
BINREGION *
phBinregionNew(void)
{
   BINREGION *new = shMalloc(sizeof(BINREGION));

   new->reg = new->parent = NULL;
   new->bin_row = new->bin_col = 0;
   new->row0 = new->col0 = 0;
   new->rsize = new->csize = 0;
   new->shift = 0;

   return(new);
}

/*
 * <AUTO EXTRACT>
 *
 * Create a BINREGION that's set to a given constant
 */
BINREGION *
phBinregionNewFromConst(float val, int nrow, int ncol,
			int bin_row, int bin_col, int shift)
{
   int i,j;
   const int is_float = (shift == MAX_U16) ? 1 : 0; /* should the region be a float? */
   BINREGION *new = phBinregionNew();

   if (is_float) {
       new->reg = shRegNew("BINREGION from const",nrow,ncol,TYPE_FL32);
   } else {
       new->reg = shRegNew("BINREGION from const",nrow,ncol,TYPE_S32);
   }
   new->parent = NULL;
   new->bin_row = bin_row; new->bin_col = bin_col;
   new->shift = shift;
   new->row0 = new->col0 = 0;
   new->rsize = bin_row*nrow;
   new->csize = bin_col*ncol;

   if (is_float) {
       for(i = 0;i < nrow;i++) {
	   float *rows = new->reg->rows_fl32[i];
	   for(j = 0;j < ncol;j++) {
	       rows[j] = val;
	   }
       }

   } else {
       const int ival = val*(1 << new->shift) + 0.5; /* scaled value as an int */
       for(i = 0;i < nrow;i++) {
	   S32 *rows = new->reg->rows_s32[i];
	   for(j = 0;j < ncol;j++) {
	       rows[j] = ival;
	   }
       }
   }

   return(new);
}

/*
 * <AUTO EXTRACT>
 *
 * Create a BINREGION that's equal to another BINREGION
 */
BINREGION *
phBinregionNewFromBinregion(const BINREGION *old)
{
   BINREGION *new = phBinregionNew();

   shAssert(old != NULL && old->reg != NULL);

   *new = *old;
   new->reg = shRegNew("BINREGION from binregion",
		       old->reg->nrow, old->reg->ncol, old->reg->type);
   shRegIntCopy(new->reg, old->reg);

   return(new);
}

/*
 * <AUTO EXTRACT>
 *
 * Delete a BINREGION
 */
void
phBinregionDel(BINREGION *breg)
{
   if(breg == NULL) return;
     
   if(breg->reg != NULL) shRegDel(breg->reg);

   shFree(breg);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * Given a region <in> and a smoothed, rebinned version thereof <smooth>,
 * interpolate into <smooth>, and subtract the result from <in> giving <out>
 *
 * <smooth> is assumed to have been scaled up by a factor 2^nbit, and to be
 * a 32-bit region
 *
 * The original filter was of size <filtsize_c>*<filtsize_r>, (<filtsize_r>
 * is is the number of columns to be used in the row direction (it's the
 * number of columns), and <filtsize_c> is the dimension of the filter along
 * the columns (it's the number of rows) and <smooth> is sampled every
 * (<filtsize_c>/2, <filtsize_r>/2). The values of the first and last rows
 * and columns are ignored; they are populated by linear interpolation. The
 * last row/column may correspond to values outside the original image if
 * the filter and original region are incommensurate.
 *
 * In other words, <smooth> is consistent with having been produced by a call
 * to
 *	phMedianSmooth(region, filtsize_c, filtsize_r, ###, nbit, ...)
 *
 * Filters both of whose dimensions are powers of 2 are somewhat more efficient
 *
 * <in> and <out> may be identical
 *
 * Always returns SH_SUCCESS
 */
void
phSkySubtract(
	      REGION *out,		/* subtracted region */
	      const REGION *in,		/* input region */
	      const BINREGION *smooth,	/* region to subtract; generally
					   smaller than <in> or <out> */
	      RANDOM *rand		/* random numbers; used in dithering */
	      )
{
   if(smooth->reg->nrow == 1 && smooth->reg->ncol == 1) {
      shAssert(smooth != NULL && smooth->reg != NULL);
      shAssert(smooth->reg->type == TYPE_S32);
      shAssert(smooth->shift >= 0);

      if(out != in) {
	 shRegIntCopy(out, in);
      }
      shRegIntConstAdd(out,
		     -smooth->reg->rows_s32[0][0]/(float)(1<<smooth->shift), 1);
   } else {
      phInterpolateAndSubtract(out,in,smooth,rand);
   }
}

/*****************************************************************************/
/*
 * <EXTRACT AUTO>
 *
 * The region smooth is assumed to be of size nr*nc. We assume that the top
 * and right rows and columns are present, and that the spacing of values in
 * smooth is uniform.
 *
 * If the dimensions of the smoothing filter that generated smooth weren't
 * commensurate with the region, these top and right rows will correspond to
 * to cells partially outside the image --- whether you fill these by
 * extrapolation or by dealing with these partial regions is up to you, but
 * this routine assumes that all pixels in the interpolated image can be
 * evaluated by interpolation (i.e. no extrapolation)
 *
 * Fill the entire image. It's made up of cells whose corner intensities are
 * given by smooth, and for each cell we could proceed as follows:
 *	1/ interpolate up a column to create the left side of the cell
 *	2/ interpolate up a column to create the right side of the cell
 *	3/ use these two cell boundaries to interpolate the interior
 * In fact it's better to use cells whose sides are powers of 2; in this
 * case the scenario is:
 *	1/ interpolate up a column to create the left side of the cell; the
 * endpoint for this interpolation is 2^n above the bottom of the cell
 *	2a/ extrapolate beyond the right side of the cell to a point 2^m pixels
 * beyond the start of the cell. Do this at the bottom and 2^n pixels up
 *	2b/ interpolate up the column at 2^m
 *	3/ use these two columns to interpolate within the cell itself
 *
 *  2^n v01...................vtmp...v11
 *     	:       	             :
 *      :       	             :
 *   dc |_____________________       :
 *      |---------------------|      :
 *      |---------------------|      :
 *      |---------------------|      :
 *      |---------------------|      :
 *      |---------------------|      :
 *      |---------------------|      :
 *      |---------------------|      :
 *      v00-------------------|------v10
 *                            dr     2^m
 * The names (v11, tmp, etc) refer to variables used in the interpolating
 * In the code, n is called dc2 and m, dr2
 */

void
phInterpolateAndSubtract(
			 REGION *out,	/* subtracted region */
			 const REGION *in, /* input region */
			 const BINREGION *smooth, /* region to subtract;
						     generally smaller than
						     <in> or <out> */
			 RANDOM *rand	/* random numbers; used in dithering */
			 )
{
   int dr2, dc2;			/* 1<<dr2 is the smallest power of
					   2 >= dr; similarly for dc2 */
   float facc, facr;			/* interpolating factors */
   int i;
   int *col0, *col1;
   int nc, nr;				/* number of cols and rows in smooth*/
   int *scr[3];				/* scratch space for interpolation */
   int sr,sc;				/* row and col counters in smooth */
   int v00, v01, v10, v11;		/* values at corners of cells */
   float vtmp;				/* temporary for finding v11 etc. */
   int dc, dr;				/* size of filter; see header */
   int nbit;				/* number of bits by which
					   smooth is scaled */
   S32 **rows;				/* == smooth->reg->rows_s32 */

   dc = smooth->bin_col;
   dr = smooth->bin_row;
   nc = smooth->reg->ncol;
   nr = smooth->reg->nrow;
   nbit = smooth->shift;

   shAssert(in != NULL && in->type == TYPE_PIX);
   shAssert(out != NULL && out->type == TYPE_PIX);
   shAssert(in->nrow == out->nrow && in->ncol == out->ncol);
   shAssert(smooth != NULL && smooth->reg != NULL);
   shAssert(smooth->reg->type == TYPE_S32);
   shAssert(nc*dc >= in->ncol);		/* the no-extrapolation constraints */
   shAssert(nr*dr >= in->nrow);
   shAssert(nbit >= 0);

   rows = smooth->reg->rows_s32;

   dc2 = 0;
   for(i = dc;i > 0;i >>= 1) dc2++;
   if((1 << (dc2 - 1)) == dc) {
      dc2--;
   }

   dr2 = 0;
   for(i = dr;i > 0;i >>= 1) dr2++;
   if((1 << (dr2 - 1)) == dr) dr2--;

   facc = (float)(1 << dc2)/dc;
   facr = (float)(1 << dr2)/dr;

   scr[0] = shMalloc((2*(1 << dc2) + dr)*sizeof(int));
   scr[1] = scr[0] + (1 << dc2);
   scr[2] = scr[1] + (1 << dc2);

   for(sr = 0;sr < nr - 1;sr++) {
      int c0,r0;			/* starting row and column */
      int dcp, drp;			/* usually dc and dr, but smaller at
					   edges to deal with partial cells */

      r0 = sr*dr;
      if(in->nrow - r0 > dc) {
	 dcp = dc;
      } else {
	 dcp = in->nrow - r0;
	 if(dcp <= 0) break;
      }

      col0 = scr[0];
/*
 * The case where the smoothing filter is a power of 2 in both directions
 * can be made significantly faster than the non-power-of-2 case, so let's
 * do so. We could treat the case of one dimension being a power of two
 * specially too, but let's not bother.
 */
      if((1 << dc2) == dc && (1 << dr2) == dr) {
/*
 * find values on first column
 */
	 v00 = rows[sr][0];
	 v01 = rows[sr + 1][0];
	 interpolate_2n(col0,dc,dc2,v00,v01);
	 
	 c0 = 0;
	 v10 = v00; v11 = v01;
	 for(sc = 0;sc < nc - 1;sc++, c0 += dc) {
	    if(in->ncol - c0 > dr) {
	       drp = dr;
	    } else {
	       drp = in->ncol - c0;
	       if(drp <= 0) break;
	    }
	    
	    col0 = scr[sc%2];
	    col1 = scr[(sc + 1)%2];
/*
 * now set up the last column of cell
 */
	    v00 = v10;
	    v01 = v11;
	    v10 = rows[sr][sc + 1];
	    v11 = rows[sr + 1][sc + 1];
	    
	    interpolate_2n(col1,dc,dc2,v10,v11);
	    
	    for(i = 0;i < dcp;i++) {
	       interpolate_2n(scr[2],dr,dr2,col0[i],col1[i]);
	       subtract_row(&out->ROWS[r0 + i][c0], &in->ROWS[r0 + i][c0],
			    scr[2], drp, nbit, (1 << nbit) - 1, rand);
	    }
	 }
      } else {
/*
 * find values on first column, remembering to extrapolate up to 2^n
 */
	 col0 = scr[0];
	 col1 = scr[1];
	 c0 = 0;
	 v00 = rows[sr][0];
	 vtmp = v00 + facc*(rows[sr + 1][0] - v00);
	 for(sc = 0;sc < nc - 1;sc++, c0 += dc) {
	    if(in->ncol - c0 > dr) {
	       drp = dr;
	    } else {
	       drp = in->ncol - c0;
	       if(drp == 0) break;
	    }
/*
 * now set up the column 2^m; first extrapolate upwards to 2^n at dr,
 * then extrapolate sideways to 2^m
 */
	    v00 = rows[sr][sc];
	    v01 = vtmp;
	    
	    v10 = rows[sr][sc + 1];
	    vtmp = v10 + facc*(rows[sr + 1][sc + 1] - v10);
	    v11 = v01 + facr*(vtmp - v01);
	    v10 = v00 + facr*(v10 - v00);
	    
	    interpolate_2n(col0,dc,dc2,v00,v01);
	    interpolate_2n(col1,dc,dc2,v10,v11);
	    
	    for(i = 0;i < dcp;i++) {
	       interpolate_2n(scr[2],dr,dr2,col0[i],col1[i]);
	       subtract_row(&out->ROWS[r0 + i][c0], &in->ROWS[r0 + i][c0],
			    scr[2], drp, nbit, (1 << nbit) - 1, rand);
	    }
	 }
      }
   }

   shFree(scr[0]);
}

/*****************************************************************************/
/*
 * Subtract row <smoothed> from <in>, scaling by 2^<nbit> and dithering,
 * and putting the result in <out>
 *
 * N.b. Using Duff's device to handle the n%3 != 0 case leads to slower code;
 * it must confuse the optimiser.
 */
static void
subtract_row(PIX *out,
	     PIX *in,
	     int *smoothed,
	     int n,
	     int nbit,
	     unsigned int mask,
	     RANDOM *rand
	     )
{
   int i;
   unsigned long r;			/* a random number */
   int val;				/* the output value (in PIX's range) */
   DECLARE_PHRANDOM(rand);

   shAssert(nbit <= 10);

   i = 0;
#if 0					/* output smoothed for debugging. N.b. no control of under/overflow */
   while(i <= n - 3) {
      r = PHRANDOM;
      out[i] = (unsigned)(-smoothed[i] + (r & mask));
      i++;

      r >>= nbit;
      out[i] = (unsigned)(-smoothed[i] + (r & mask));
      i++;

      r >>= nbit;
      out[i] = (unsigned)(-smoothed[i] + (r & mask));
      i++;
   }

   while(i < n) {
      r = PHRANDOM;
      out[i] = (unsigned)(-smoothed[i] + (r & mask));
      i++;
   }
#else
   while(i <= n - 3) {
      r = PHRANDOM;
      val = (((long)in[i] << nbit) - smoothed[i] + (long)(r & mask)) >> nbit;
      out[i++] = val < 0 ? 0 : (val > MAX_U16 ? MAX_U16 : val);

      r >>= nbit;
      val = (((long)in[i] << nbit) - smoothed[i] + (long)(r & mask)) >> nbit;
      out[i++] = val < 0 ? 0 : (val > MAX_U16 ? MAX_U16 : val);

      r >>= nbit;
      val = (((long)in[i] << nbit) - smoothed[i] + (long)(r & mask)) >> nbit;
      out[i++] = val < 0 ? 0 : (val > MAX_U16 ? MAX_U16 : val);
   }

   while(i < n) {
      r = PHRANDOM;
      val = (((long)in[i] << nbit) - smoothed[i] + (long)(r & mask)) >> nbit;
      out[i++] = val < 0 ? 0 : (val > MAX_U16 ? MAX_U16 : val);
   }
#endif

   END_PHRANDOM(rand);
}

/*****************************************************************************/
/*
 * Interpolate using Bresenham's line-drawing algorithm, suitable modified
 * to avoid generating more than one point with a given x-value, even if
 * the line being drawn is steeper than 45degrees.
 *
 * The end points are taken to be at (0,y0) and (n,y1), and n
 * values are inserted into arr[0] ... arr[n - 1]. Note that this interval
 * _doesn't_ extend all the way to the last point, so this routine can
 * be called repeatedly to interpolate many line segments
 *
 * We require n > 0
 *
 * Note that if dx is a power of 2 it's _much_ faster to call interpolate_2n
 *
 * The function interpolate_slow is also provided (although #if'd to 0) as
 * it's algorithms are the same, but it hasn't been optimised and should
 * be easier to unserstand.
 */
#if 0
static void
interpolate(
	    int *arr,			/* output array */
	    int n,			/* number of points desired */
	    unsigned int dx,		/* spacing between y0 and y1 */
	    int y0,			/* value at beginning */
	    int y1			/* value at end */
	  )
{
   int a,px,py;
   int mirror;				/* is line mirrored in y axis? */
   int dlg,dsml;
   int dy;
   int y;
	    
   shAssert(n > 0);

   dy = y1 - y0;

   if(dy == 0) {
      px = 0;
      do {
	 arr[px] = y0;
	 px++;
      } while(px != n);
      return;
   }

   py = y0;
   if(dy > 0) {				/* line slopes up */
      mirror = 0;
   } else {
      dy = -dy;
      mirror = 1;
   }

   if(dy >= dx) {
      a = (unsigned)dy/2;
      dlg = dy;
      dsml = dx;
/*
 * The two halves of this if block are identical except for the sign of py
 * (and thus y)
 */
      if(mirror) {
	 px = 0;
	 do {
	    y = py;
	    if(a >= dsml) {
	       unsigned int nstep = a/dsml + 1;
	       a -= nstep*dsml;
	       if(py < y0) {
		  y -= nstep/2;
	       }
	       py -= nstep;
	    }
	    a += dlg - dsml;
	    
	    arr[px] = y;
	    
	    py--;
	    px++;
	 } while(px != n);
      } else {
	 px = 0;
	 do {
	    y = py;
	    if(a >= dsml) {
	       unsigned int nstep = a/dsml + 1;
	       a -= nstep*dsml;
	       if(py > y0) {
		  y += nstep/2;
	       }
	       py += nstep;
	    }
	    a += dlg - dsml;
	    
	    arr[px] = y;
	    
	    py++;
	    px++;
	 } while(px != n);
      }
   } else {
      a = dx/2;
      dlg = dx;
      dsml = dy;

/*
 * The two halves of this if block are identical except for the sign of py
 */
      if(mirror) {
	 px = 0;
	 do {
	    arr[px] = py;
	    
	    a -= dsml;
	    if(a < 0) {
	       a += dlg;
	       py--;
	    }
	    px++;
	 } while(px != n);
      } else {
	 px = 0;
	 do {
	    arr[px] = py;
	    
	    a -= dsml;
	    if(a < 0) {
	       a += dlg;
	       py++;
	    }
	    px++;
	 } while(px != n);
      }
   }
}
#endif

/*
 * This is identical to interpolate(), except that dx is specified as the
 * log to the base 2 of dx
 */

static void
interpolate_2n(
	    int *arr,			/* output array */
	    int n,			/* number of points desired */
	    int lb_dx,			/* log_2(spacing between y0 and y1) */
	    int y0,			/* value at beginning */
	    int y1			/* value at end */
	  )
{
   int a,px,py;
   int mirror;				/* is line mirrored in y axis? */
   int dlg,dsml;
   unsigned int dx;
   int dy;
   int y;
	    
   shAssert(n > 0);

   dx = 1 << lb_dx;
   dy = y1 - y0;

   if(dy == 0) {
      px = 0;
      do {
	 arr[px] = y0;
	 px++;
      } while(px != n);
      return;
   }

   py = y0;
   if(dy > 0) {				/* line slopes up */
      mirror = 0;
   } else {
      dy = -dy;
      mirror = 1;
   }

   if(dy >= dx) {
      a = (unsigned)dy/2;
      dlg = dy;
      dsml = dx;
/*
 * The two halves of this if block are identical except for the sign of py
 * (and thus y)
 */
      if(mirror) {
	 px = 0;
	 do {
	    y = py;
	    if(a >= dsml) {
	       unsigned int nstep = ((unsigned int)a >> lb_dx) + 1;
	       a -= (int)(nstep << lb_dx);
	       if(py < y0) {
		  y -= nstep/2;
	       }
	       py -= nstep;
	    }
	    a += dlg - dsml;
	    
	    arr[px] = y;
	    
	    py--;
	    px++;
	 } while(px != n);
      } else {
	 px = 0;
	 do {
	    y = py;
	    if(a >= dsml) {
	       unsigned int nstep = ((unsigned int)a >> lb_dx) + 1;
	       a -= (int)(nstep << lb_dx);
	       if(py > y0) {
		  y += nstep/2;
	       }
	       py += nstep;
	    }
	    a += dlg - dsml;
	    
	    arr[px] = y;
	    
	    py++;
	    px++;
	 } while(px != n);
      }
   } else {
      a = dx/2;
      dlg = dx;
      dsml = dy;

/*
 * The two halves of this if block are identical except for the sign of py
 */
      if(mirror) {
	 px = 0;
	 do {
	    arr[px] = py;
	    
	    a -= dsml;
	    if(a < 0) {
	       a += dlg;
	       py--;
	    }
	    px++;
	 } while(px != n);
      } else {
	 px = 0;
	 do {
	    arr[px] = py;
	    
	    a -= dsml;
	    if(a < 0) {
	       a += dlg;
	       py++;
	    }
	    px++;
	 } while(px != n);
      }
   }
}

#if 0
/*
 * This is the ancestor of the above interpolate functions. As its name
 * suggests it hasn't been optimised, but if you want to understand what's
 * going on it might be a good place to start
 */
static void
interpolate_slow(
		 int *arr,		/* output array */
		 int n,			/* number of points desired */
		 int dx,		/* number of points between y0 and y1*/
		 int y0,
		 int y1
	  )
{
   register int a,px,py;
   int mirror;				/* is line mirrored in y axis? */
   int dlg,dsml;
   int dy;
   
   dy = y1 - y0;

   if(dy == 0) {
      for(px = 0;px < n;px++) {		/* fill the array with constant */
	 arr[px] = y0;
      }
      return;
   }

   if(dy > 0) {				/* line slopes up */
      py = y0;
      mirror = 0;
   } else {
      dy = -dy;
      py = -y0;
      mirror = 1;
   }

   if(dy >= dx) {
      a = dy/2;
      dlg = dy;
      dsml = dx;
      
      for(px = 0;px < n;px++) {
	 int y;
	 
	 y = py;
	 if(a >= dsml) {
	    unsigned int nstep = a/dsml + 1;
	    a -= nstep*dsml;
	    if(py > y0) {
	       y += nstep/2;
	    }
	    py += nstep;
	 }
	 a += dlg - dsml;
	 
	 arr[px] = (mirror ? -y : y);
	 
	 py++;
      }
   } else {
      a = dx/2;
      dlg = dx;
      dsml = dy;
      
      for(px = 0;px < n;px++) {
	 arr[px] = (mirror ? -py : py);
	 
	 a -= dsml;
	 if(a < 0) {
	    a += dlg;
	    py++;
	 }
      }
   }
}
#endif

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Return the sky value at a point given a BINREGION by bilinear-interpolation.
 *
 * A BINREGION with only one point is acceptable (its sole value is returned),
 * otherwise the BINREGION must be at least 2x2
 */
float
phBinregionInterpolate(
		       const BINREGION *sky, /* the binned data */
		       int row, int col	/* where we want the value */
		       )
{
   float r, c;				/* fractional pixel position of
					   (row, col) in sky */
   const int is_float = (sky == NULL) || (sky->reg == NULL) || (sky->reg->type == TYPE_FL32);
   int ir, ic;				/* == (int)r, (int)c */
   float v00, v01, v10, v11, v0, v1, val; /* interpolated points */

   shAssert(sky != NULL && sky->reg != NULL);
   shAssert(is_float || sky->reg->type == TYPE_S32);

   row -= sky->row0; col -= sky->col0;

   if(sky->reg->nrow == 1 && sky->reg->ncol == 1) {
       if (is_float) {
	   return sky->reg->rows_fl32[0][0];
       } else {
	   return sky->reg->rows_s32[0][0]*pow(2.0,-sky->shift);
       }
   }
   shAssert(sky->reg->nrow >= 2 && sky->reg->nrow >= 2);
   
   row -= sky->reg->row0; col -= sky->reg->col0;

   r = (float)row/sky->bin_row; ir = (int)r;
   c = (float)col/sky->bin_col; ic = (int)c;
   ir = (ir < 0) ? 0 : ((ir >= sky->reg->nrow - 1) ? sky->reg->nrow - 2 : ir);
   ic = (ic < 0) ? 0 : ((ic >= sky->reg->ncol - 1) ? sky->reg->ncol - 2 : ic);

   v00 = is_float ? sky->reg->rows_fl32[ir][ic]   : sky->reg->rows_s32[ir][ic];
   v01 = is_float ? sky->reg->rows_fl32[ir][ic+1] : sky->reg->rows_s32[ir][ic+1];
   v0 = (1 - (c-ic))*v00 + (c-ic)*v01 + (is_float ? 0.0 : 0.5);

   v10 = is_float ? sky->reg->rows_fl32[ir+1][ic]   : sky->reg->rows_s32[ir+1][ic];
   v11 = is_float ? sky->reg->rows_fl32[ir+1][ic+1] : sky->reg->rows_s32[ir+1][ic+1];
   v1 = (1 - (c-ic))*v10 + (c-ic)*v11 + (is_float ? 0.0 : 0.5);
   
   val = (1 - (r - ir))*v0 + (r - ir)*v1;

   return is_float ? val : val*pow(2.0, -sky->shift);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Rebin an image (currently only PIX is supported), returning a BINREGION
 *
 * The cases 1x2, 2x1, 2x2, and 4x4 are specially coded, all other cases
 * are somewhat slower, and are only carried out if <slow> is true.
 *
 * If rshift is negative, the output pixel will be the average of the
 * input pixels; otherwise the output will be the sum of the pixels right
 * shifted by rshift bits. If you need to specify <slow>, your choices of
 * rshift are restricted to 0 or -1.
 *
 * Any pixels in the binned image for which _all_ the needed pixels are
 * not present in the input image are set to bkgd.
 *
 * If specified, addcon is added to the image before rshift is applied.
 *
 * The binned image is truncated to 16bits after rshift is applied.
 * 
 * row0 and col0 are the coordinates of the pixel in the parent region
 * that defines the bottom left of the superpixel that becomes the (0,0)
 * pixel in the binned image. If row0/col0 are positive, this may lead
 * to some pixels in the output image that don't correspond to any in
 * the input; if row0/col0 are negative the region to be binned is still
 * taken to be the size of the input region (and some of the input region's
 * pixels are therefore ignored).
 *
 * This is MUCH faster than using shRegBin (even with slow true); also it
 * optionally averages over the pixels rather than simply summing them,
 * allows for adding a constant or rescaling, and handles offsets in the
 * parent region.
 *
 * Return: binned region, or NULL. If bout is non-NULL, it will be the returned
 * binned region.
 */
BINREGION *
phRegIntBin(BINREGION *bout,		/* The output region; may be NULL */
	    const REGION *in,		/* The input region */
	    const int bin_row,		/* the number of rows to average */
	    const int bin_col,		/* the number of columns to average */
	    int rshift,			/* amount to shift answer */
	    const int bkgd,		/* value for missing parts of image */
	    const int addcon,		/* constant to add to image */
	    const int row0, const int col0, /* origin of binned portion */
	    const int slow)		/* allow slow algorithm? */
{
   int brow0, bcol0;			/* origin in binned image */
   unsigned int i;
   int in_ncol,in_nrow;			/* unpacked from in */
   PIX **in_rptr;			/* unpacked from in */
   REGION *out;				/* the binned output region */
   int out_ncol,out_nrow;		/* size of out */
   PIX **out_rptr;			/* unpacked from out */
   PIX *iptr0, *iptr1, *iptr2, *iptr3;	/* pointers to rows of in */
   PIX *optr,*oend;			/* pointer to row of out, and one
					   beyond end */
   S32 val;				/* value of rebinned pixel */

   shAssert(in != NULL);
   shAssert(in->type == TYPE_PIX);
   shAssert(bin_col > 0 && bin_row > 0);
   in_ncol = in->ncol;			/* unpack these from in */
   in_nrow = in->nrow;
   in_rptr = in->ROWS;
/*
 * if row0|col0 is negative, we will be unable to set the bottom|left
 * part of the binned region as there's no data in the input region. We
 * shall still take the area to be binned to be that of the input region,
 * which means that we'll _ignore_ a top|right strip of width abs(row0|col0)
 *
 * What this means in practice is that we can and must reset row0 and col0
 *
 * We have to be a little careful about the size of bout->reg. It is most
 * useful if one BINREGION can be used when binning a given sized input
 * region, whatever the values of (row0, col0). This means that bout->reg
 * should be sized to be as large as would be needed for row0 == col0 == 0,
 * although this may correspond to pixels off the edge of in. We deal with
 * this properly in phBinregionUnBin()
 */
   if(row0 >= 0) {
      brow0 = 0;
   } else {
      brow0 = (-row0 + bin_row - 1)/bin_row;
      *(int *)&row0 = row0 + bin_row*brow0;
      in_nrow -= bin_row*brow0;
   }

   if(col0 >= 0) {
      bcol0 = 0;
   } else {
      bcol0 = (-col0 + bin_col - 1)/bin_col;
      *(int *)&col0 = col0 + bin_col*bcol0;
      in_ncol -= bin_col*bcol0;
   }
   shAssert(row0 >= 0 && col0 >= 0);

   out_ncol = (in_ncol - col0)/bin_col;
   out_nrow = (in_nrow - row0)/bin_row;
   if(out_nrow <= 0 || out_ncol <= 0) {
      shErrStackPush("phRegIntBin: region to bin lies outside region");
      return(NULL);
   }

   if(bout == NULL) {
      bout = phBinregionNew();
   }
   if(bout->reg == NULL) {
      char name[100];
      sprintf(name, "binned %dx%d", bin_row, bin_col);
      bout->reg = shRegNew(name, in->nrow/bin_row, in->ncol/bin_col, TYPE_PIX);
   }

   if(bout->reg->nrow < brow0 + out_nrow ||
					  bout->reg->ncol < bcol0 + out_ncol) {
      shErrStackPush("phRegIntBin: provided binned region is too small");
      return(NULL);
   }
   
   bout->bin_row = bin_row; bout->bin_col = bin_col;

   shAssert(rshift != MAX_U16)		/* we don't support floating BINREGIONs here */
   bout->shift = rshift;
   bout->row0 = row0;
   bout->col0 = col0;
   bout->rsize = in->nrow;
   bout->csize = in->ncol;

   out = bout->reg;
   shAssert(out->type == TYPE_PIX);
   out_rptr = out->ROWS;
/*
 * set to bkgd any part of the output region that isn't going to be set
 */
   if(brow0 > 0) {
      for(i = 0;i < out->ncol; i++) {
	 out_rptr[0][i] = bkgd;
      }
      for(i = 1;i < brow0;i++) {
	 memcpy(out_rptr[i], out_rptr[0], out->ncol*sizeof(PIX));
      }
   }
   if(out->nrow > out_nrow) {
      for(i = 0;i < out->ncol; i++) {
	 out_rptr[out_nrow][i] = bkgd;
      }
      for(i = out_nrow + 1;i < out->nrow;i++) {
	 memcpy(out_rptr[i], out_rptr[out_nrow], out->ncol*sizeof(PIX));
      }
   }
   if(bcol0 > 0) {
      for(i = 0;i < bcol0; i++) {
	 out_rptr[0][i] = bkgd;
      }
      for(i = 1;i < out->nrow;i++) {
	 memcpy(out_rptr[i], out_rptr[0], bcol0*sizeof(PIX));
      }
   }
   if(out->ncol > out_ncol) {
      int n = out->ncol - out_ncol;

      for(i = 0;i < n; i++) {
	 out_rptr[0][out_ncol + i] = bkgd;
      }
      for(i = 1;i < out->nrow;i++) {
	 memcpy(&out_rptr[i][out_ncol], &out_rptr[0][out_ncol], n*sizeof(PIX));
      }
   }
/*
 * All inputs are verified, now deal with important special cases specially
 */
   if(bin_col == 1) {
      if(bin_row == 2) {
	 if(rshift == 0) {
	    for(i = 0;i < out_nrow;i++) {
	       optr = &out_rptr[brow0 + i][bcol0];
	       iptr0 = &in_rptr[row0 + 2*i][col0];
	       iptr1 = &in_rptr[row0 + 2*i + 1][col0];
	       oend = optr + out_ncol;
	       while(optr < oend) {
		  val = iptr0[0] + iptr1[0] + addcon;
		  *optr++ = (val >= 0) ? val & 0xffff : 0;
		  iptr0++;
		  iptr1++;
	       }
	    }
	 } else {
	    if(rshift < 0) rshift = 1;

	    for(i = 0;i < out_nrow;i++) {
	       optr = &out_rptr[brow0 + i][bcol0];
	       iptr0 = &in_rptr[row0 + 2*i][col0];
	       iptr1 = &in_rptr[row0 + 2*i + 1][col0];
	       oend = optr + out_ncol;
	       while(optr < oend) {
		  val = ((iptr0[0] + iptr1[0]) >> rshift) + addcon;
		  *optr++ = (val >= 0) ? val & 0xffff : 0;
		  iptr0++;
		  iptr1++;
	       }
	    }
	 }
	 return(bout);
      }
   } else if(bin_col == 2) {
      if(bin_row == 1) {
	 if(rshift == 0) {
	    for(i = 0;i < out_nrow;i++) {
	       optr = &out_rptr[brow0 + i][bcol0];
	       iptr0 = &in_rptr[row0 + i][col0];
	       oend = optr + out_ncol;
	       while(optr < oend) {
		  val = iptr0[0] + iptr0[1] + addcon;
		  *optr++ = (val >= 0) ? val & 0xffff : 0;
		  iptr0 += 2;
	       }
	    }
	 } else {
	    if(rshift < 0) rshift = 1;

	    for(i = 0;i < out_nrow;i++) {
	       optr = &out_rptr[brow0 + i][bcol0];
	       iptr0 = &in_rptr[row0 + i][col0];
	       oend = optr + out_ncol;
	       while(optr < oend) {
		  val = ((iptr0[0] + iptr0[1]) >> rshift) + addcon;
		  *optr++ = (val >= 0) ? val & 0xffff : 0;
		  iptr0 += 2;
	       }
	    }
	 }
	 return(bout);
      } else if(bin_row == 2) {
	 if(rshift == 0) {
	    for(i = 0;i < out_nrow;i++) {
	       optr = &out_rptr[brow0 + i][bcol0];
	       iptr0 = &in_rptr[row0 + 2*i][col0];
	       iptr1 = &in_rptr[row0 + 2*i + 1][col0];
	       oend = optr + out_ncol;
	       while(optr < oend) {
		  val = iptr0[0] + iptr0[1] + iptr1[0] + iptr1[1] + addcon;
		  *optr++ = (val >= 0) ? val & 0xffff : 0;
		  iptr0 += 2;
		  iptr1 += 2;
	       }
	    }
	 } else {
	    if(rshift < 0) rshift = 2;
	    for(i = 0;i < out_nrow;i++) {
	       optr = &out_rptr[brow0 + i][bcol0];
	       iptr0 = &in_rptr[row0 + 2*i][col0];
	       iptr1 = &in_rptr[row0 + 2*i + 1][col0];
	       oend = optr + out_ncol;
	       while(optr < oend) {
		  val =
		    ((iptr0[0] + iptr0[1] + iptr1[0] + iptr1[1]) >> rshift) +
									addcon;
		  *optr++ = (val >= 0) ? val & 0xffff : 0;
		  iptr0 += 2;
		  iptr1 += 2;
	       }
	    }
	 }
	 return(bout);
      }
   } else if(bin_col == 4) {
      if(bin_row == 4) {
	 if(rshift == 0) {
	    for(i = 0;i < out_nrow;i++) {
	       optr = &out_rptr[brow0 + i][bcol0];
	       iptr0 = &in_rptr[row0 + 4*i][col0];
	       iptr1 = &in_rptr[row0 + 4*i + 1][col0];
	       iptr2 = &in_rptr[row0 + 4*i + 2][col0];
	       iptr3 = &in_rptr[row0 + 4*i + 3][col0];
	       oend = optr + out_ncol;
	       while(optr < oend) {
		  val = (iptr0[0] + iptr0[1] + iptr0[2] + iptr0[3] +
			 iptr1[0] + iptr1[1] + iptr1[2] + iptr1[3] +
			 iptr2[0] + iptr2[1] + iptr2[2] + iptr2[3] +
			 iptr3[0] + iptr3[1] + iptr3[2] + iptr3[3]) + addcon;
		  *optr++ = (val >= 0) ? val & 0xffff : 0;
		  iptr0 += 4;
		  iptr1 += 4;
		  iptr2 += 4;
		  iptr3 += 4;
	       }
	    }
	 } else {
	    if(rshift < 0) rshift = 4;
	    
	    for(i = 0;i < out_nrow;i++) {
	       optr = &out_rptr[brow0 + i][bcol0];
	       iptr0 = &in_rptr[row0 + 4*i][col0];
	       iptr1 = &in_rptr[row0 + 4*i + 1][col0];
	       iptr2 = &in_rptr[row0 + 4*i + 2][col0];
	       iptr3 = &in_rptr[row0 + 4*i + 3][col0];
	       oend = optr + out_ncol;
	       while(optr < oend) {
		  val = ((iptr0[0] + iptr0[1] + iptr0[2] + iptr0[3] +
			  iptr1[0] + iptr1[1] + iptr1[2] + iptr1[3] +
			  iptr2[0] + iptr2[1] + iptr2[2] + iptr2[3] +
			  iptr3[0] + iptr3[1] + iptr3[2] + iptr3[3]) >>
							      rshift) + addcon;
		  *optr++ = (val >= 0) ? val & 0xffff : 0;
		  iptr0 += 4;
		  iptr1 += 4;
		  iptr2 += 4;
		  iptr3 += 4;
	       }
	    }
	 }
	 return(bout);
      }
   }
/*
 * general case. This will be slower, so see if they asked for a slow algorithm
 */
   if(slow) {
      PIX **in_rrptr;			/* == &in->ROWS[...] */
      int j;
      PIX *iend0;			/* point one beyond a rebinned pixel
					   in the in REGION */
      int start_col;			/* index of start of rebinned pixel in
					   REGION in */
      const int i_norm = (1 << 12)/(bin_row*bin_col); /* used to normalise */

      if(rshift == 0) {
	 for(i = 0;i < out_nrow;i++) {
	    in_rrptr = &in_rptr[row0 + bin_row*i];
	    optr = &out_rptr[brow0 + i][bcol0];
	    oend = optr + out_ncol;
	    start_col = col0;
	    while(optr < oend) {
	       val = 0;
	       j = 0;
	       do {
		  iptr0 = in_rrptr[j] + start_col;
		  iend0 = iptr0 + bin_col;
		  do {
		     val += *iptr0++;
		  } while(iptr0 < iend0);
		  j++;
	       } while(j < bin_row);
	       start_col += bin_col;
	       *optr++ = (val + addcon) & 0xffff;
	    }
	 }
      } else {
	 if(rshift > 0) {
	    shErrStackPush("phRegIntBin: for binning when -slow is needed, "
			   "rshift must be <= 0");
	 }
	 for(i = 0;i < out_nrow;i++) {
	    in_rrptr = &in_rptr[row0 + bin_row*i];
	    optr = &out_rptr[brow0 + i][bcol0];
	    oend = optr + out_ncol;
	    start_col = col0;
	    while(optr < oend) {
	       val = 0;
	       j = 0;
	       do {
		  iptr0 = in_rrptr[j] + start_col;
		  iend0 = iptr0 + bin_col;
		  do {
		     val += *iptr0++;
		  } while(iptr0 < iend0);
		  j++;
	       } while(j < bin_row);
	       start_col += bin_col;
	       *optr++ = (((val*i_norm) >> 12) + addcon) & 0xffff;
	    }
	 }
      }

      return(bout);
   } else {
      shErrStackPush("phRegIntBin: binning by %dx%d would be slow, and"
		     "slow is false",bin_col,bin_row);
      return(NULL);
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Expand a BINREGION. The constant addcon is added to the image _before_
 * any desired shifting has been carried out
 */
REGION *
phBinregionUnBin(const BINREGION *in,	/* the region to expand */
		 RANDOM *rand,		/* random numbers to dither with */
		 const int bkgd,	/* value for missing parts of image */
		 const int addcon)	/* constant to add to image */
{
   PIX **brows;				/* == in->reg->ROWS */
   int bin_row, bin_col;		/* == in->bin_{row,col} */
   int c,r;
   int i,j;
   unsigned int mask;			/* mask to select desired random bits*/
   int nrow, ncol;			/* == in->reg->n{row,col} */
   REGION *reg;				/* output REGION */
   int reg_nrow, reg_ncol;		/* == reg->n{row,col} */
   int row0, col0;			/* origin on in->reg in reg */
   PIX **rows;				/* == in->ROWS */
   int shift;				/* == in->shift */
   int val;				/* the value of a pixel */

   shAssert(in != NULL && in->reg != NULL);
   shAssert(in->reg->type == TYPE_PIX);

   brows = in->reg->ROWS;
   nrow = in->reg->nrow;
   ncol = in->reg->ncol;
   bin_row = in->bin_row;
   bin_col = in->bin_col;
   shift = in->shift;
   mask = (1 << (shift + 1)) - 1;

   reg = shRegNew("binregionUnBin", in->rsize, in->csize, in->reg->type);
   rows = reg->ROWS;
   reg_nrow = reg->nrow; reg_ncol = reg->ncol;
/*
 * allow for the origin of the binned image within the unbinned one. This
 * is the inverse of the fiddling near the top of phRegIntBin()
 */
   if((row0 = in->row0) < 0) {
      for(c = 0; c < reg_ncol; c++) {
	 rows[reg_nrow + row0][c] = bkgd;
      }
      for(r = reg_nrow + row0 + 1;r < reg_nrow; r++) {
	 memcpy(rows[r], rows[reg_nrow + row0], reg_ncol*sizeof(PIX));
      }

      row0 = 0;
   } else if(row0 > 0) {
      for(c = 0; c < reg_ncol; c++) {
	 rows[0][c] = bkgd;
      }
      for(r = 1;r < row0; r++) {
	 memcpy(rows[r], rows[0], reg_ncol*sizeof(PIX));
      }
   }

   if((col0 = in->col0) < 0) {
      for(c = reg_ncol + col0; c < reg_ncol; c++) {
	 rows[0][c] = bkgd;
      }
      for(r = 1; r < reg_nrow; r++) {
	 memcpy(&rows[r][reg_ncol + col0], &rows[0][reg_ncol + col0],
							    -col0*sizeof(PIX));
      }

      col0 = 0;
   } else if(col0 > 0) {
      for(c = 0; c < col0; c++) {
	 rows[0][c] = bkgd;
      }
      for(r = 1; r < reg_nrow; r++) {
	 memcpy(rows[r], rows[0], col0*sizeof(PIX));
      }
   }
   shAssert(nrow <= in->reg->nrow && ncol <= in->reg->ncol);
/*
 * the binned region may have been made too big to be expanded into the
 * unbinned one; ignore the extra binned pixels
 */
   if(row0 + bin_row*nrow > in->rsize) {
      nrow = (in->rsize - row0)/bin_row;
   }
   if(col0 + bin_col*ncol > in->csize) {
      ncol = (in->csize - col0)/bin_col;
   }
/*
 * and expand the binned data
 */
   if(shift == 0) {
      for(r = 0;r < nrow;r++) {
	 for(c = 0;c < ncol;c++) {
	    val = brows[r][c] + addcon;
	    for(i = 0;i < bin_row;i++) {
	       for(j = 0;j < bin_col;j++) {
		  rows[row0 + bin_row*r + i][col0 + bin_col*c + j] = val;
	       }
	    }
	 }
      }
   } else {
      DECLARE_PHRANDOM(rand);
      for(r = 0;r < nrow;r++) {
	 for(c = 0;c < ncol;c++) {
	    val = brows[r][c] + addcon;
	    for(i = 0;i < bin_row;i++) {
	       for(j = 0;j < bin_col;j++) {
		  rows[row0 + bin_row*r + i][col0 + bin_col*c + j] =
					    (val + (PHRANDOM & mask)) >> shift;
	       }
	    }
	 }
      }

      END_PHRANDOM(rand);
   }
/*
 * set any extra pixels at top/right of region to bkgd
 */
   if(bin_row*nrow < reg_nrow) {
      for(r = bin_row*nrow; r < reg_nrow; r++) {
	 for(c = 0; c < reg_ncol; c++) {
	    rows[r][c] = bkgd;
	 }
      }
   }
   if(bin_col*ncol < reg_ncol) {
      int c0 = bin_col*ncol;
      for(r = 0; r < reg_nrow; r++) {
	 for(c = c0; c < reg_ncol; c++) {
	    rows[r][c] = bkgd;
	 }
      }
   }

   return(reg);
}

