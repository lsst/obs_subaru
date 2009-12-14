#include <stdio.h>
#include <string.h>
#include <math.h>
#include <alloca.h>
#include "dervish.h"
#include "phSignals.h"
#include "phConsts.h"
#include "phRandom.h"
#include "phCorrectFrames.h"
#include "phCorrectFrames_p.h"    /* Private definitions  */
#include "phMaskbittype.h"
#include "phSpanUtil.h"
#include "phObjects.h"
#include "phMathUtils.h"		/* here so as not to re#def M_SQRT */

#define NO_FIX_BIAS 1			/* if true, disable fix_bias code */
#define MAXBIASWIDTH   1000		/* largest bias area on chip must have
					   fewer than this many columns */
/*
 * The type of a defect is a bitpattern encoding which pixels should be used
 * for interpolation. Each good column is written as 1, and each bad as 0.
 *
 * The mask extends 2 pixels to each side of the defect, so a 1 column defect
 * with all neighbours good is 11011 (033) and a double defect in the centre is
 * 110011 (063). A set of 3 bad pixels at the right boundary is written as
 * 11000 (030); left hand boundary pixels are written as if they were at
 * the right (so 3 bad pixels at the left of the chip are coded as 00011, but
 * saved as 11000 so as not to lose the leading 0s).
 */
typedef enum {
   BAD_LEFT = 0,			/* defect is at left boundary */
   BAD_NEAR_LEFT,			/* defect is near left boundary */
   BAD_WIDE_LEFT,			/* defect is wide at left boundary */
   BAD_MIDDLE,				/* defect is in middle of frame */
   BAD_WIDE_NEAR_LEFT,			/* defect is near left, and wide */
   BAD_WIDE,				/* defect is in middle, and wide */
   BAD_WIDE_NEAR_RIGHT,			/* defect is near right, and wide */
   BAD_NEAR_RIGHT,			/* defect is near right boundary */
   BAD_WIDE_RIGHT,			/* defect is wide at right boundary */
   BAD_RIGHT				/* defect is at right boundary */
} BAD_POS;

typedef struct {
   int x1;				/* first column of bad region */
   int x2;				/* last column of bad region */
   BAD_POS pos;				/* position of defect */
   DFTYPE dftype;			/* type if intrinsic */
   DFACTION dfaction;			/* what to do */
   unsigned int type;			/* type of defect */
} BADCOLUMN;

static float *contractReg(REGION *data,int row1, int row2, int col1, int col2,\
                          int dir, int type);
static float *get_interp_coeff(float psf_width);
static float get_corr(float *C, float Im2, float Im1, float Ip1, float Ip2, float x);
static void classify_badcol(BADCOLUMN *badcol, int nbadcol, int nc);
static void do_badcol(const BADCOLUMN *badcol, int nbadcol, PIX *out,
		      int ncol, int min);
#if defined(INTERPOLATE_VERTICAL)
static void interpolate_vertical(REGION *reg, int r1, int  r2, int c, int min);
#endif

#define WIDE_DEFECT 11			/* minimum width of a BAD_WIDE defect*/

/*****************************************************************************/
/*
 * Book-keeping for the cached linearity correction tables
 */
static REGION *linearity_reg = NULL;	/* space for linearity tables */

static struct {
   LINEARITY_TYPE type;			/* type of linearity function */
   int ncoeff;				/* number of linearity coeffs */
   float coeffs[N_LINEAR_COEFFS];	/* linearity coefficients */
   U16 *table;				/* the actual table (pointer to
					   a row of linearity_reg) */
} linearity_tables[NCOLOR + 1][2];	/* indexed by camRow */

/*****************************************************************************/
/*
 * init and fini functions for Correct Frames
 */
void
phInitCorrectFrames(void)
{
   int a, i;
/*
 * setup linearity corrections
 */
   if(linearity_reg == NULL) {
      linearity_reg =
	shRegNew("linearity tables", 2*NCOLOR, MAX_U16 + 1, TYPE_U16);
   
      for(i = 0; i <= NCOLOR; i++) {
	 for(a = 0; a < 2; a++) {
	    linearity_tables[i][a].type = LINEAR_NONE;
	    linearity_tables[i][a].ncoeff = 0;
	    linearity_tables[i][a].table = ((i == 0) ? NULL :
				       linearity_reg->rows_u16[2*(i - 1) + a]);
	 }
      }
   }
}

void
phFiniCorrectFrames(void)
{
   int a, i;
/*
 * cleanup linearity corrections
 */
   for(i = 0; i <= NCOLOR; i++) {
      for(a = 0; a < 2; a++) {
	 linearity_tables[i][a].type = LINEAR_NONE;
	 linearity_tables[i][a].ncoeff = 0;
	 linearity_tables[i][a].table = NULL; 
      }
   }
   
   shRegDel(linearity_reg);
   linearity_reg = NULL;
}

/*****************************************************************************/
/*
 * Evaluate the sum of the pixels below the OBJMASKs in the data region,
 * then set the OBJMASK.sum field according to the op argument
 *
 * If flat_ptr != NULL, flat field data first.
 */
static void
find_counts_in_trails(int op,		/* Desired operation:
					   -1: subtract, 0: set, 1: add*/
		      const REGION *data, /* Region to be counted */
		      const CHAIN *om_chain, /* chain of OBJMASKS specifying
					       where to count */
		      int start_second_amp, /* column where 2nd amp begins */
		      const U16 *flat_ptr, /* flat field, or NULL */
		      const U32 *cbias,	/* corrected bias, or NULL */
		      const int *linear_thresholds, /* linearity thresholds */
		      const U16 **linear_tables) /* linearity tables */
{
   int amp;				/* which amp are we using? */
   int i, j;
   int nsat;				/* number of saturated objects */
   int nspan;				/* == om->nspan */
   OBJMASK *om;				/* an element of om_chain */
   PIX *row;
   int row0, col0;			/* == data->{row,col}0 */
   float sum_DN;			/* sum of DN under trail */
   int val;				/* bias subtracted pixel value */
   int x;				/* counter from x1..x2 */
   int x1, x2, y;			/* == om->s[].x1 etc. */

   shAssert(op == -1 || op == 0 || op == 1);
   shAssert(data != NULL && data->type == TYPE_PIX);
   row0 = data->row0; col0 = data->col0;

   if(flat_ptr == NULL) {
      shAssert(cbias == NULL &&
	       linear_thresholds == NULL && linear_tables == NULL);
   } else {
      shAssert(cbias != NULL &&
	       linear_thresholds != NULL && linear_tables != NULL);
   }

   nsat = shChainSize(om_chain);
   for(i = 0; i < nsat; i++) {
      om = shChainElementGetByPos(om_chain, i);
      nspan = om->nspan;
      sum_DN = 0;
      for(j = 0; j < nspan; j++) {
	 y = om->s[j].y - row0;
	 x1 = om->s[j].x1 - col0;
	 x2 = om->s[j].x2 - col0;
	 
	 row = data->ROWS[y];
	 if(flat_ptr == NULL) {		/* no need to flat field */
	    for(x = x1; x <= x2; x++) {
	       sum_DN += row[x];
	    }
	 } else {			/* We have to flatten the data */
	    for(x = x1; x <= x2; x++) {
	       amp = (x < start_second_amp) ? 0 : 1;
	       
	       val = row[x] - cbias[x];
	       if(val > linear_thresholds[amp]) { /* linearise? */
		  val = linear_tables[amp][val]; /*  linearise. */
	       }
	       
	       sum_DN += val*flat_ptr[x]/FSHIFT + SOFT_BIAS;
	    }
	 }
      }
      
      switch (op) {
       case -1:
	 om->sum -= sum_DN; break;
       case  0:
	 om->sum  = sum_DN; break;
       case  1:
	 om->sum += sum_DN; break;
      }
   }
}

/*****************************************************************************/
/*
 * Find the saturated pixels in the image
 */
static void
find_saturated_pixels(REGION *raw_data,	/* raw image to be corrected */
		      REGION *out_data,	/* corrected data */
		      const REGION *flat_vec, /* inverted flat field vector */
		      const CCDPARS *ccdpars, /* CCD properties */
		      FRAMEPARAMS *fparams, /* gain etc. */
		      U32 *cbias,	/* bias after correcting for drift */
		      const int *linear_thresholds, /* linearity thresholds */
		      const U16 **linear_tables, /* linearity tables */
		      BADCOLUMN *badcol_intrinsic, /* posn etc. of bad cols */
		      int nbadcol_intrinsic, /* number of intrinsic bad-column
						defects */
		      int col_0, int col_n, /* raw data is col_0 <= c < col_n*/
		      int fix_bias)	/* fix pre-Sept-1998 problems
					   in the bias level */
{
   CURSOR_T curs;			/* cursor for CHAINs */
   OBJMASK *extra = NULL;		/* extra saturated pixels we find */
   const U16 *flat_ptr = flat_vec->rows_u16[0];
   int fullWell;			/* saturation level for this half */
   int i, j;
   int ilev;				/* level to use in obj->sv[] */
   int isat;				/* counter in satObjmask */
   unsigned short levs[2];		/* levels for phObjectsFind */
   const int ncol = out_data->ncol;
   const int nrow = out_data->nrow;
   int nextra;				/* number of extra pixels */
   int nspan;				/* number of spans in om */
   OBJECT *obj;				/* a detected saturated `object' */
   OBJMASK *om;				/* the OBJMASK in obj */
   PIX *next, *prev;			/* pointers to next and previous rows*/
   int row0, col0;			/* unpacked from raw_data->{row,col}0*/
   int *saturation_level;		/* == &fparams->fullWell[] */
   SPANMASK *sm;			/* == out_data->mask */
   int start_second_amp;		/* first column readout with 2nd amp */
   CHAIN *satur_objs;			/* OBJECTs for all saturated pixels */
   int x;
   int extra_bad_satur_columns;		/* treat neighbouring columns
					   as saturated too? */
/*
 * Check that the combination of flat field, bias, and value of
 * fullWell[01] cannot lead to pixels overflowing during flat fielding
 * (don't check bad columns, of course). If there's a problem, lower
 * the saturation level appropriately
 */
   sm = (SPANMASK *)(out_data->mask);
   shAssert(sm->cookie == SPAN_COOKIE);

   extra_bad_satur_columns = fparams->extra_bad_satur_columns;
   start_second_amp = (ccdpars->namps == 1) ?
				 col_n + 1 : ccdpars->sData1 - ccdpars->sData0;
   
   isat = 0;
   j = -1;				/* counter in badcol_intrinsic */
   for(i = 0; i < col_n - col_0; i++) {
      if(i == isat) {
	 j++;
	 if(j == nbadcol_intrinsic) {
	    isat = col_n - col_0;	/* i.e. last pass through this loop */
	 } else {
	    isat = badcol_intrinsic[j].x1;
	    i = badcol_intrinsic[j].x2;
	    continue;
	 }
      }

      if(i < start_second_amp) {
	 saturation_level = &fparams->fullWell[0];
      } else {
	 saturation_level = &fparams->fullWell[1];
      }
	    
      if((int)((*saturation_level - cbias[i])*flat_ptr[i]/FSHIFT +
							SOFT_BIAS) > MAX_U16) {
	 *saturation_level = cbias[i] +
				      (MAX_U16 - SOFT_BIAS)*FSHIFT/flat_ptr[i];
      }
      shAssert((int)((*saturation_level - cbias[i])*flat_ptr[i]/FSHIFT +
							SOFT_BIAS) <= MAX_U16);
   }
#if !NO_FIX_BIAS
   if(fix_bias) {
      fparams->fullWell[0] -= 100;	/* we might add this many DN */
      fparams->fullWell[1] -= 100;	/* we might add this many DN */
   }
#else
   shAssert(fix_bias == fix_bias);	/* avoid not-used compiler warning */
#endif
/*
 * Find the saturated pixel objects first. This is complicated by the
 * fact that the saturation level is different for the two sides of
 * two-amplifier chips.  We accordingly run the object finder with both
 * thresholds, and then choose appropriate the level in the OBJECT.
 *
 * Saturated objects that span the divide between the amplifiers are
 * found with the lower of the two levels.
 *
 * Then go through the chain adding pixels just above and just below the
 * saturation trails to the list of bad pixels, as a certain amount of
 * charge spills into them.
 *
 * We actually do this by adding the extra bad pixels to an OBJMASK called
 * extra, and then merging at the end.
 *
 * In the survey electronics, as of June 1998, there is the additional
 * problem that the columns adjacent to bleeding columns are themselves
 * bad, although at a level of only a few thousand.  It is believed that
 * this is only a problem for single-amplifier chips, and that these
 * chips also have an extra low-level problem in the _next_ column to
 * the right.
 *
 * For all 1-amp chips we grow each row of the saturated mask by one
 * to both the left and the right.  If extra_bad_satur_columns is true,
 * we also add an extra bad column to the right (making a total of 2)
 */
   if(fparams->fullWell[0] < fparams->fullWell[1]) {
      levs[0] = fparams->fullWell[0]; levs[1] = fparams->fullWell[1];
   } else {
      levs[0] = fparams->fullWell[1]; levs[1] = fparams->fullWell[0];
   }

   satur_objs = phObjectsFind(raw_data, 0, 0, 0, 0, 2, levs, fparams, -1, 0);
      
   extra = phObjmaskNew(5);
   
   curs = shChainCursorNew(satur_objs);
   while(shChainWalk(satur_objs,curs,NEXT) != NULL) {
      obj = shChainElementRemByCursor(satur_objs, curs);
      
      if(obj->sv[0]->cmax < ccdpars->sData1) { /* left side */
	 ilev = (levs[0] == fparams->fullWell[0]) ? 0 : 1;
      } else if(obj->sv[0]->cmin > ccdpars->sData1) { /* right side */
	 ilev = (levs[0] == fparams->fullWell[1]) ? 0 : 1;
      } else {
	 ilev = 0;			/* lower threshold */
      }

      if(obj->nlevel < ilev + 1) {	/* not detected at that threshold */
	 phObjectDel(obj);
	 continue;
      }

      om = obj->sv[ilev]; obj->sv[ilev] = NULL;
      fullWell = levs[ilev];
      
      nspan = om->nspan;
      shAssert(nspan > 0);
/*
 * Is this a single column? We require that it extend from top to bottom, and
 * not have more than 0.5% of extra pixels (remember, this has to work for
 * things like postage stamps, so a percentage is appropriate)
 *
 * If so, it may be already labelled as a HOTCOL, so don't label it SATUR too
 */
      if(nspan == raw_data->nrow && om->npix < 1.005*raw_data->nrow) {
	 for(i = 0; i < nbadcol_intrinsic; i++) {
	    if(badcol_intrinsic[i].x1 == om->cmin) {
	       break;
	    }
	 }
	 
	 if(i < nbadcol_intrinsic) {	/* yes; an intrinsic bad column */
	    if(badcol_intrinsic[i].dftype == HOTCOL) {
	       if(om->npix == raw_data->nrow) { /* no extra pixels */
		  obj->sv[ilev] = om;		/* put it back in OBJECT */
		  phObjectDel(obj);
		  
		  continue;		/* don't label as saturated */
	       }
	    }
/*
 * we should remove the hotcolumn from the saturated object and proceed,
 * but for now simply allow the whole saturated object to be labelled
 * SATUR
 */
	    ;
	 }
      }
      
      row0 = raw_data->row0; col0 = raw_data->col0;
      
      nextra = 0;
      
      for(i = 0;i < nspan;i++) {
	 const int y = om->s[i].y - row0;
	 int x1 = om->s[i].x1 - col0, x2 = om->s[i].x2 - col0;
/*
 * Overflow of saturated serial registers; JEG claims that this is only
 * a problem for 1-amp chips. Actually he's wrong (e.g. idR-000259-g3-0514.fit)
 * so always assume that the serial overflowed, and that there
 * are extra "slosh" pixels on both sides of the trail
 */
	 if(extra_bad_satur_columns &&
	    (ccdpars->namps == 1 || ccdpars->namps == 2)) {
	    if(x1 > 0) x1--;
	    if(x2 < ncol - 1) x2++;
	 }
/*
 * CTE problems in serial registers; charge is trailed away from amps
 */
	 if(x1 >= start_second_amp) {
	    if(x1 > 0) x1--;
	 }
	 if(x2 <= start_second_amp) {
	    if(x2 < ncol - 1) x2++;
	 }
/*
 * Overflow of saturated charge up and down the rows
 */
	 next = (y == nrow - 1) ? NULL : raw_data->ROWS[y + 1];
	 prev = (y == 0) ? NULL : raw_data->ROWS[y - 1];
	 for(x = x1;x <= x2;x++) {
	    if(prev != NULL && prev[x] < fullWell) {
	       if(nextra == extra->size) {
		  phObjmaskRealloc(extra, nextra*1.4);
	       }
	       extra->s[nextra].y = row0 + y - 1;
	       extra->s[nextra].x1 = extra->s[nextra].x2 = col0 + x;
	       nextra++;
	    }
	    
	    if(next != NULL && next[x] < fullWell) {
	       if(nextra == extra->size) {
		  phObjmaskRealloc(extra, nextra*1.4);
	       }
	       extra->s[nextra].y = row0 + y + 1;
	       extra->s[nextra].x1 = extra->s[nextra].x2 = col0 + x;
	       nextra++;
	    }
	 }
      }
      
      extra->nspan = nextra;
      phCanonizeObjmask(extra,1);
      phObjmaskMerge(om,extra,0,0);
      extra->nspan = 0;
      
      phObjmaskAddToSpanmask(om, sm, S_MASK_SATUR);
      
      phObjectDel(obj);
   }
   
   shChainCursorDel(satur_objs, curs);
   shChainDel(satur_objs);
   phObjmaskDel(extra);
/*
 * Go through the image and find how many DN lie under the saturation trails
 */
   find_counts_in_trails(0, raw_data, sm->masks[S_MASK_SATUR],start_second_amp,
			 flat_ptr, cbias, linear_thresholds, linear_tables);
}

/*****************************************************************************/
/*
 * Check and maybe calculate the linearity correction tables
 */
static void
setup_linearity(const CCDPARS *ccdpars,
		int linear_threshold[2],
		U16 *linear_tables[2])
{
   int a;				/* counter for amplifiers */
   const int camRow = ccdpars->iRow;
   float coeffs[N_LINEAR_COEFFS];	/* unpacked coeffs from ccdpars */
   int i;
   int valid;				/* are current tables valid? */
   float linear_c;			/* scale factor for linearity */
   float log10_thresh;			/* == log10(linear_threshold[]) */
   const int namplifier = ccdpars->namps;
   int ncoeff;				/* number of coefficients used to
					   characterise nonlinearity */
   U16 *table;				/* table of corrected pixel values */

   switch (ccdpars->linearity_type) {
    case LINEAR_NONE:
      linear_threshold[0] = linear_threshold[1] = MAX_U16 + 1;
      return;
    case LINEAR_THRESHLOG:
      ncoeff = 2;
      break;
    default:
      shError("setup_linearity: Unsupported type of linearity correction %d; "
	      "assuming LINEAR_NONE", ccdpars->linearity_type);
      linear_threshold[0] = linear_threshold[1] = MAX_U16 + 1;
      return;
   }
   shAssert(ncoeff < N_LINEAR_COEFFS);
/*
 * Process each amplifier seperately
 */
   for(a = 0; a < namplifier; a++) {
/*
 * unpack coefficients from ccdpars, and tables from linearity_tables[]
 */
      for(i = 0; i < ncoeff; i++) {
	 coeffs[i] =
	   ((a==0) ? ccdpars->linear_coeffs0[i] : ccdpars->linear_coeffs1[i]);
      }

      linear_tables[a] = linearity_tables[camRow][a].table;
      shAssert(linear_tables[a] != NULL);
/*
 * See if they are different from the ones used to build the tables
 */
      valid = 1;			/* assume the best */
      if(linearity_tables[camRow][a].type != ccdpars->linearity_type) {
	 valid = 0;
      } else if(ncoeff != linearity_tables[camRow][a].ncoeff) {
	 valid = 0;			/* current table is invalid */
      } else {
	 for(i = 0; i < ncoeff; i++) {
	    if(linearity_tables[camRow][a].coeffs[i] != coeffs[i]) {
	       valid = 0;
	       break;
	    }
	 }
      }
/*
 * per-field book-keeping
 */
      linearity_tables[camRow][a].type = ccdpars->linearity_type;
      linearity_tables[camRow][a].ncoeff = ncoeff;
      for(i = 0; i < N_LINEAR_COEFFS; i++) {
	 linearity_tables[camRow][a].coeffs[i] = (i < ncoeff) ? coeffs[i] : 0;
      }
      linear_threshold[a] = coeffs[0];

      if(valid) {
	 continue;			/* nothing more to do */
      }
/*
 * We must build a new table
 */
      switch(linearity_tables[camRow][a].type) {
       case LINEAR_THRESHLOG:
	 shAssert(linear_threshold[a] > 0 && linear_threshold[a] <= MAX_U16);

	 linear_c = coeffs[1];
	 log10_thresh = log10(linear_threshold[a]);
	 table = linear_tables[a];
	 
	 for(i = 0; i < linear_threshold[a]; i++) {
	    table[i] = i;
	 }
	 for(; i <= MAX_U16; i++) {
	    table[i] = i*(1 + linear_c*(log10(i) - log10_thresh)) + 0.5;
	 }
	 
	 break;
       default:
	 shFatal("You can't get here (type %d)",
		 linearity_tables[camRow][a].type);
	 break;				/* NOTREACHED */
      }
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * This is the science module correctFrames which
 * performs the removal of the CCD signature from
 * the raw data 
 *
 * We can either process complete frames or partial frames. In the former
 * case the first thing that we do is to trim the frame (using shSubRegNew)
 * to reduce it the latter.
 *
 * We assume that a partial frame has no overclock data, either of its own
 * or in any of its ancestors.
 *
 * In the main body of the function:
 *   raw_data may be a subregion with (row0,col0) set
 *   bias_vec and flat_vec are the full width of a frame, without overclock
 *   out_data, out_data->mask, are the same size as raw_data,
 *   and have row0 and col0 equal to 0
 */
int
phCorrectFrames(REGION *raw_data,	/* raw image to be corrected */
		const REGION *bias_vec,	/* bias vector for entire frame */
		const REGION *flat_vec,	/* inverted flat field vector */
                const CCDPARS *ccdpars,	/* CCD properties */
		FRAMEPARAMS *fparams,	/* gain etc. */
		int bias_scale,		/* scale applied to bias_vec */
                int bias0,		/* left overclock drift */
                int bias1,		/* right overclock drift */
                int stamp_flag,		/* is this a postage stamp? */
		REGION *out_data,	/* corrected data */
		int minval,		/* minimum acceptable pixel value */
		int fix_bias)		/* fix pre-Sept-1998 problems
					   in the bias level */
{
   int amp;				/* loop counter for amplifiers */
#if !NO_FIX_BIAS
   const REGION *const bias_vec_nt = bias_vec; /* save non-trimmed bias */
#endif
   U32 *cbias;				/* bias after correcting for drift */
   int corr;				/* per-row correction to apply to bias,
					   if fix_bias != 0 */
   int *frac_bias;			/* (bias - int(bias))*flat vector */
   int startcol, endcol;		/* start/ending column to flatten */
   const int fuzz = 0;			/* extra columns to NOTCHECK */
   int i, j;
   U16 *linear_table, *linear_tables[2]; /* linearity tables */
   int linear_thresh, linear_thresholds[2]; /* threshold for
					      linearity correction */
   const unsigned long mask = FSHIFT - 1; /* (r&mask)/FSHIFT is in [0,1) */
   int ncol, nrow;			/* size of out_data */
   BADCOLUMN badcol_intrinsic[NBAD1COLMAX]; /* positions etc. of bad columns */
   BADCOLUMN badcol[2*NBAD1COLMAX];	/* positions etc. of bad and saturated
					   columns */
   int bias_drift[2];			/* == bias[01] */
   unsigned long r;			/* a random number in [0,2^32-1) */
   int row0, col0, coln;		/* raw data is col0 <= c < coln  */
   CURSOR_T curs;			/* cursor for CHAINs */
   int isat;				/* counter in satObjmask */
   int data_ncol, data_col0;	        /* number and starting col of datasec*/
   int namplifier;			/* number of amplifiers */
   int nbadcol_intrinsic;		/* number of intrinsic bad-column
					   defects */
   int nbadcol;				/* number of bad-column defects
					   and saturated columns */
#if !NO_FIX_BIAS
   const REGION *const raw_data_nt = raw_data; /* save non-trimmed data*/
#endif
   RANDOM *rand = phRandomGet();	/* XXX should be passed in */
   int samp_2;				/* first column of amplifier 2 */
   float inverse_bias_scale;		/* convert bias values to DN */
   CHAIN *notchecked;			/* == sm->masks[S_MASK_NOTCHECKED] */
   PIX oscan_bias[50];			/* debiased overscan/bias region */
   int sbias[2], nbias[2];		/* start and size of bias */
   int sdata[2], ndata[2];		/* start and size of data */
   int soscn[2], noscn[2];		/* start and size of overscan */
   REGION *trimmed, *trimmed_bias;      /* raw_data/bias trimmed of overclock*/
   U16 *flat_ptr;			/* unpack data pointers to eliminate */
   PIX *raw_ptr;			/*                 aliasing problems */
   PIX *out_ptr;
   int sat_x1, sat_x2;			/* first and last pixels in saturated
					   span */
   SPANMASK *sm;
   OBJMASK *satObjmask;			/* mask for saturated pixels */
   OBJMASK *interpMaskRect;		/* mask for interpolated pixels */
   char *objmask_type = (char *)shTypeGetFromName("OBJMASK");
   int val;				/* value of a flat-fielded pixel */

   CATCH("phCorrectFrames", SIGABRT, SH_GENERIC_ERROR);

#if NO_FIX_BIAS
   shAssert(fix_bias == 0);
#endif
   shAssert(raw_data != NULL);
   shAssert(ccdpars != NULL);
   shAssert(fparams != NULL);
   shAssert(fparams->fullWell[0] > 0 && fparams->fullWell[1] > 0);

   minval += SOFT_BIAS;
/*
 * Did the PSP not handle U16 underflow correctly?
 */
#define  FIX_PSP_BIAS_DRIFT 1
#if FIX_PSP_BIAS_DRIFT
   /* don't do the fix for binned apache data 
    *  since this caused problems. PR 6120
    */
   if (ccdpars->colBinning != 4) {
     if(bias0 > (1 << 14)) { bias0 -= (1 << 15); }
     if(bias1 > (1 << 14)) { bias1 -= (1 << 15); }
   }
#endif
/*
 * figure out which pixels are being used for overscan, bias, and data
 */
   bias_drift[0] = bias0; bias_drift[1] = bias1;
   soscn[0] = ccdpars->sPrescan0; noscn[0] = ccdpars->nPrescan0;
   sbias[0] = ccdpars->sPreBias0; nbias[0] = ccdpars->nPreBias0;

   if(ccdpars->amp1 < 0) {
      soscn[1] = noscn[1] = 0;
      sbias[1] = nbias[1] = 0;
   } else {
#if 0
      soscn[1] = ccdpars->sPrescan1; noscn[1] = ccdpars->nPrescan1;
      sbias[1] = ccdpars->sPreBias1; nbias[1] = ccdpars->nPreBias1;
#else  /* interchange overclock and bias for right amplifier */
      sbias[1] = ccdpars->sPrescan1; nbias[1] = ccdpars->nPrescan1;
      soscn[1] = ccdpars->sPreBias1; noscn[1] = ccdpars->nPreBias1;
#endif
   }

   sdata[0] = 0; sdata[1] = ccdpars->sData1 - ccdpars->sData0;
   if (ccdpars->namps > 1) {
      shAssert(ccdpars->namps == 2);
      ndata[0] = ccdpars->nData0;
      ndata[1] = ccdpars->nData1;
   } else {
      ndata[0] = ccdpars->nData0;
      ndata[1] = 0;
   }
   data_col0 = ccdpars->sData0;
   data_ncol = ndata[0] + ndata[1];

   namplifier = ccdpars->namps;
   for(i = 0; i < namplifier; i++) {
      shAssert(noscn[i] + nbias[i] < sizeof(oscan_bias)/sizeof(oscan_bias[0]));
   }
/*
 * set up linearity corrections
 */
   setup_linearity(ccdpars, linear_thresholds, linear_tables);
/*
 * correct for horizontal trails 
 */  
   if(!stamp_flag) {
      float *overscanL;     /* vertical structure in left overscan region */
      float *overscanR;     /* vertical structure in right overscan region */
      float *interpC;       /* coefficients for linear interpolation */
      float *aux;           /* temp. holder for the overscan structure */
      float medL;           /* median left overscan value */
      float medR;           /* median right overscan value */
      int nrow = raw_data->nrow; /* number of rows in raw region */
      float psf_width;      /* psf width */
      float corr_m2, corr_m1, corr_p1, corr_p2;   /* aux */
      int corr;				/* horizontal trail correction */
      int overscanLstart, overscanLend; /* extent of the left overscan */
      int overscanRstart, overscanRend; /* extent of the right overscan */
      int col0 = -10000, ncol = 10000, amp; /* aux */
      float xaux;

      /* interpolation coefficients for this width (#...#...#...# case) */
      if (fparams->psf != NULL && fparams->psf->width > 0) {
         psf_width = fparams->psf->width;
      } else {
         psf_width = 1.6;		/* XXX */
      }
      interpC = get_interp_coeff(psf_width);

      /* overscan extent */
      overscanLstart = soscn[0];
      overscanLend = soscn[0]+noscn[0]-1;
      if(ccdpars->amp1 < 0) {
         overscanRstart = ccdpars->sPostscan0;
         overscanRend = ccdpars->sPostscan0 + ccdpars->nPostscan0 - 1;
      } else {
         overscanRstart = ccdpars->sPrescan1;
         overscanRend = ccdpars->sPrescan1 + ccdpars->nPrescan1 - 1;
      }
      /*
       * get the overscan structure
       */
      aux = alloca(nrow * sizeof(float));
      if(overscanLstart > overscanLend) { /* no left overscan */
	 overscanL = shMalloc(nrow*sizeof(float));
	 memset(overscanL, '\0', nrow*sizeof(float));
	 shAssert(overscanL[0] == 0.0);	/* all bits 0 == 0.0 */
	 medL = 0;
      } else {
	 overscanL = contractReg(raw_data,
				 0,nrow-1,overscanLstart,overscanLend,1,1);
	 memcpy(aux,overscanL,nrow*sizeof(float));
	 phFloatArrayStats(aux,nrow,0,&medL,NULL,NULL,NULL);
      }

      if(overscanRstart > overscanRend) {
	 overscanR = shMalloc(nrow*sizeof(float));
	 memset(overscanR, '\0', nrow*sizeof(float));
	 shAssert(overscanR[0] == 0.0);	/* all bits 0 == 0.0 */
	 medR = 0;
      } else {
	 overscanR = contractReg(raw_data,
				 0,nrow-1,overscanRstart,overscanRend,1,1);
	 memcpy(aux,overscanR,nrow*sizeof(float));
	 phFloatArrayStats(aux,nrow,0,&medR,NULL,NULL,NULL);
      }
      /*
       * correct raw image
       */
      for(j = 2; j < nrow-2; j++) {
         raw_ptr = raw_data->ROWS[j];
         /* store the overscan excess (0.5 for float-to-integer debiasing) */
         raw_ptr[data_col0 - 1] = FLT2PIX(overscanL[j] - medL);
         raw_ptr[data_col0 + data_ncol] = FLT2PIX(overscanR[j] - medR);
         if(ccdpars->namps == 1) {
            corr_m2 = FLT2PIX(overscanL[j-2] - medL);
            corr_m1 = FLT2PIX(overscanL[j-1] - medL);
            corr_p1 = FLT2PIX(overscanR[j] - medR);
            corr_p2 = FLT2PIX(overscanR[j+1] - medR);
            if(corr_m1 >= 1 && corr_p1 >= 1) {
               col0 = data_col0;
               ncol = data_ncol;
               for(i = col0; i < col0 + ncol; i++) {
                  xaux = (1.0*i-col0)/(ncol-1);
                  corr = get_corr(interpC,
				  corr_m2,corr_m1,corr_p1,corr_p2,xaux);
                  raw_ptr[i] -= corr;
               }
            }
         } else {
            for(amp = 0; amp < namplifier; amp++) {
               if(amp == 0) {
                  corr_m2 = FLT2PIX(overscanL[j-2] - medL);
                  corr_m1 = FLT2PIX(overscanL[j-1] - medL);
                  corr_p1 = FLT2PIX(overscanL[j] - medL);
                  corr_p2 = FLT2PIX(overscanL[j+1] - medL);
                  if(corr_m1 >= 1 && corr_p1 >= 1) {
                     col0 = ccdpars->sData0;
                     ncol = ccdpars->nData0;
		  }
               } else {
                  corr_m2 = FLT2PIX(overscanR[j+1] - medR);
                  corr_m1 = FLT2PIX(overscanR[j] - medR);
                  corr_p1 = FLT2PIX(overscanR[j-1] - medR);
                  corr_p2 = FLT2PIX(overscanR[j-2] - medR);
                  if(corr_m1 >= 1 && corr_p1 >= 1) {
                     col0 = ccdpars->sData0+ccdpars->nData0;
                     ncol = ccdpars->nData1;
                  }
               }
               if(corr_m1 >= 1 && corr_p1 >= 1) {
                  for(i = col0; i < col0 + ncol; i++) {
                     xaux = (1.0*i-col0)/(ncol-1);
                     corr = get_corr(interpC,
				     corr_m2,corr_m1,corr_p1,corr_p2,xaux);
                     raw_ptr[i] -= corr;
                  }
               }
	    }
         }
      }
      shFree(overscanL);
      shFree(overscanR);
      shFree(interpC);
   }
/*
 * trim the data
 */
   if(stamp_flag) {
      trimmed = NULL;
   } else {
      if((trimmed = shSubRegNew("trimmed",raw_data,raw_data->nrow,
				data_ncol,0,data_col0,NO_FLAGS)) == NULL) {
	 shErrStackPush("phCorrectFrames: cannot trim region %s\n",
			raw_data->name);
	 return(SH_GENERIC_ERROR);
      }
      trimmed->col0 = 0;
      raw_data = trimmed;
   }
   /* The bias needs to be trimmed for both full frames and stamps */
   if((trimmed_bias = shSubRegNew("trimmed_bias",bias_vec,1,
				  data_ncol,0,data_col0,NO_FLAGS)) == NULL) {
       shErrStackPush("phCorrectFrames: cannot trim region %s\n",
		      raw_data->name);
       return(SH_GENERIC_ERROR);
   }
   trimmed_bias->col0 = 0;
   bias_vec = trimmed_bias;

/*
 * Assume here that the bias and flat REGIONs start with the first non-bias
 * column of the chip, and end with the last non-bias column of the chip.
 */
   shAssert(raw_data->type == TYPE_PIX);
   shAssert(out_data->type == TYPE_PIX);
   shAssert(bias_vec->type == TYPE_U16);
   shAssert(flat_vec->type == TYPE_U16);

   shAssert(out_data->mask != NULL);

   shAssert(raw_data->ncol == out_data->ncol);
   shAssert(out_data->row0 == raw_data->row0 && 
	    out_data->col0 == raw_data->col0);

   if (namplifier > 1) {
       shAssert((ccdpars->nData0 + ccdpars->nData1) == bias_vec->ncol);
   } else {
       shAssert((ccdpars->nData0) == bias_vec->ncol);
   }
						
   shAssert(flat_vec->ncol == bias_vec->ncol);

   shAssert(bias_scale != 0);
/*
 * Set some image dimensions.  
 */
   nrow = out_data->nrow;
   ncol = out_data->ncol;
   shAssert(ncol >= 3);			/* so we can interpolate bad columns */
   row0 = raw_data->row0;
   col0 = raw_data->col0;
   coln = col0 + ncol;
   samp_2 = ccdpars->nData0;
   if(samp_2 >= coln) {			/* we never get to amp2 */
      samp_2 = coln;
   }
/*
 * Clear all masks.
 */
   sm = (SPANMASK *)(out_data->mask);
   shAssert(sm->cookie == SPAN_COOKIE);

   for(j = 0; j < NMASK_TYPES; j++) {
       if(sm->masks[j] != NULL) {
	  shChainDel(sm->masks[j]);
       }
       sm->masks[j] = shChainNew(objmask_type);
   }
   notchecked = sm->masks[S_MASK_NOTCHECKED];
/*
 * set the cbias vectors, correcting for the left and right 
 * hand bias drifts. If the values of the drifts, bias[01], exceed
 * MAX_U16 we estimate the bias drift ourselves from the overclock
 * region, as we also have to correct for problems in the electronics
 *
 * Once we've corrected by the cbias vectors we've got the DN-over-bias
 * numbers that we need to correct for nonlinearity, but we've lost any
 * fractional part of the bias drift.  To allow for this, we also keep
 * the fractional part scaled up by FSHIFT
 */
   if(bias0 < MAX_U16) {
   } else {
      shAssert(bias1 > MAX_U16);
      bias0 = bias1 = 0;      
   }

   inverse_bias_scale = 1.0/bias_scale;
      
   cbias = shMalloc(ncol*sizeof(U32));
   frac_bias = shMalloc(ncol*sizeof(int));
   flat_ptr = flat_vec->rows_u16[0];

   for(amp = 0; amp < namplifier; amp++) {
      int i0 = sdata[amp] - col0, i1 = sdata[amp] + ndata[amp] - col0;
      if(i0 < 0) i0 = 0;
      if(i1 > coln - col0) i1 = coln - col0;
      
      for(i = i0; i < i1; i++) {
	 int bval = bias_vec->rows_u16[0][i + col0] + bias_drift[amp];
	 cbias[i] = FLT2PIX(inverse_bias_scale*bval);
	 bval -= bias_scale*cbias[i];	/* == (fractional part)*bias_scale */
	 frac_bias[i] = inverse_bias_scale*bval*FSHIFT;
      }
   }
/*
 * Read the defect structure and prepare to fix them. We are interested in
 * columns in the output structures, hence we subtract col0.
 */
   {
      CHAIN *chain = ccdpars->CCDDefect; /* the chain of defects */
      CCDDEFECT *defect;		/* a defect */
      int old_i;			/* previous value of i */

      shAssert(chain != NULL);
      
      curs = shChainCursorNew(chain);
      nbadcol_intrinsic = 0;

      old_i = -100;
      while((defect = (CCDDEFECT *)shChainWalk(chain,curs,NEXT)) != NULL) {
	 if(defect->dfaction == ADDCOL) {
	    ;				/* fixed by bias */
	 } else if(defect->dfaction == BADCOL ||
		   defect->dfaction == FILCOL) {
	    if(nbadcol_intrinsic >= NBAD1COLMAX - 1) {
	       shFatal("phCorrectFrame: "
		       "I can only handle %d bad columns per chip\n",
		       NBAD1COLMAX);
	    }

	    i = defect->dfcol0;
	    if(i < 0 && i >= ncol) {
	       shFatal("phCorrectFrames: Defect in illegal column (%d)",i);
	    }
	    i -= col0;

	    if(i < 0 || i >= ncol) {
	       continue;		/* outside postage stamp */
	    }

	    if(i != old_i + 1) {
	       if(nbadcol_intrinsic > 0) {
		  badcol_intrinsic[nbadcol_intrinsic - 1].x2 = old_i;
	       }
	       badcol_intrinsic[nbadcol_intrinsic].x1 = i;
	       badcol_intrinsic[nbadcol_intrinsic].dftype = defect->dftype;
	       badcol_intrinsic[nbadcol_intrinsic].dfaction = defect->dfaction;
	       
	       nbadcol_intrinsic++;
	    }
	    
	    old_i = i + defect->dfncol - 1;
	 } else {
	    shFatal("phCorrectFrame: Found but can't process a "
		    "type %d defect\n",defect->dfaction);
	 }
      }
      if(nbadcol_intrinsic > 0) {
	 if (old_i >= ncol) {
	    old_i = ncol - 1;
	 }
	 badcol_intrinsic[nbadcol_intrinsic - 1].x2 = old_i;
      }
      
      shChainCursorDel(chain,curs);
   }
/*
 * For the moment, stick with old single column defects; multi-column defects
 * can be specified as multiple single-column ones
 *
 * The only exception is that multi-column defects adjacent to the left- or
 * right- side of the chip are permitted (this includes the case of bad
 * serial registers or amplifiers)
 *
 * We add a little fuzz as the object finder will be run after
 * smoothing with the PSF and we don't want to smooth the interpolated
 * data into the acceptable region
 */
#if 0
   if(nbadcol_intrinsic > 0 && badcol_intrinsic[0].x1 == 0 && /* left side */
      badcol_intrinsic[0].dfaction != FILCOL) {
      shChainJoin(notchecked,
		  phSpanmaskFromRect(0, 0,
				     badcol_intrinsic[0].x2 + fuzz, nrow - 1));
      
      for(i = 1;i < nbadcol_intrinsic; i++) { /* move others down */
	 badcol_intrinsic[i - 1] = badcol_intrinsic[i];
      }
      nbadcol_intrinsic--;
   }
#else
   if(nbadcol_intrinsic > 0 && badcol_intrinsic[0].x1 == 0) { /* left side */
      badcol_intrinsic[0].dfaction = FILCOL;
   }
#endif
   
#if 0
   if(nbadcol_intrinsic > 0 &&
      badcol_intrinsic[nbadcol_intrinsic - 1].x2 == ncol - 1 && /* right */
      badcol_intrinsic[nbadcol_intrinsic - 1].dfaction != FILCOL) {
      int x1 = badcol_intrinsic[nbadcol_intrinsic - 1].x1;
      shChainJoin(notchecked,
		  phSpanmaskFromRect(x1 - fuzz, 0, ncol - 1, nrow - 1));
      nbadcol_intrinsic--;
   }
#else
   if(nbadcol_intrinsic > 0 &&
      badcol_intrinsic[nbadcol_intrinsic - 1].x2 == ncol - 1) { /* right */
      badcol_intrinsic[nbadcol_intrinsic - 1].dfaction = FILCOL;
   }
#endif
/*
 * Classify bad columns, and assert that the defects were properly sorted.
 */
   classify_badcol(badcol_intrinsic, nbadcol_intrinsic, ncol);

#if !defined(NDEBUG)
   for(i = 0;i < nbadcol_intrinsic;i++) {
      if(i > 0 && badcol_intrinsic[i].dfaction != FILCOL) {
	 shAssert(badcol_intrinsic[i].x1 > badcol_intrinsic[i - 1].x1);
	 shAssert(badcol_intrinsic[i].x1 > badcol_intrinsic[i - 1].x2);
      }
#if 0
      printf("%d: %-6s 0%o %d  %d--%d\n",i,
	     (badcol_intrinsic[i].pos==BAD_LEFT ||
	      badcol_intrinsic[i].pos==BAD_WIDE_LEFT) ? "LEFT" :
	     (badcol_intrinsic[i].pos==BAD_RIGHT ||
	      badcol_intrinsic[i].pos==BAD_WIDE_RIGHT) ? "RIGHT" :
	     (badcol_intrinsic[i].pos==BAD_WIDE) ? "WIDE" : "MIDDLE",
	     badcol_intrinsic[i].type, badcol_intrinsic[i].dfaction,
	     badcol_intrinsic[i].x1, badcol_intrinsic[i].x2);
#endif
   }
#endif
/*
 * If there are bad columns adjacent to the edges of the chip, add
 * them to the NOTCHECKED region.
 */
   if(nbadcol_intrinsic > 0 &&
      (badcol_intrinsic[0].x1 == 0 ||
       badcol_intrinsic[nbadcol_intrinsic-1].x2 >= ncol - 1)) {
      SPANMASK *sm = (SPANMASK *)out_data->mask;
      CHAIN *notchecked = shChainNew("OBJMASK");
      shAssert(sm != NULL);
      shAssert(sm->cookie == SPAN_COOKIE);
      /* left */
      if(badcol_intrinsic[0].x1 == 0 &&
	 badcol_intrinsic[0].dfaction != FILCOL) {
	 const int x = badcol_intrinsic[0].x2 + fuzz;
	 if(x >= 0) {
	    shChainJoin(notchecked, phSpanmaskFromRect(0, 0, x, nrow - 1));
	 }
      }
      /* right */
      if(badcol_intrinsic[nbadcol_intrinsic - 1].x2 >= ncol - 1 &&
	 badcol_intrinsic[nbadcol_intrinsic - 1].dfaction != FILCOL) {
	 const int x = badcol_intrinsic[nbadcol_intrinsic-1].x1 - fuzz;
	 if(x <= ncol - 1) {
	    shChainJoin(notchecked, phSpanmaskFromRect(x, 0, ncol - 1, nrow - 1));
	 }
      }
      if(notchecked->nElements == 0) {
	 shChainDel(notchecked);
      } else {
	 sm->masks[S_MASK_NOTCHECKED] =
	   phObjmaskChainMerge(sm->masks[S_MASK_NOTCHECKED], notchecked);
      }
   }
/* 
 * flat_ptr has to be defined relative to col0 (as e.g. cbias) for both 
 * function find_saturated_pixels and for the flat-fielding code below
 */
   flat_ptr = flat_vec->rows_u16[0] + col0;

   find_saturated_pixels(raw_data, out_data, flat_vec, ccdpars, fparams,
			 cbias,
			 linear_thresholds, (const U16 **)linear_tables,
			 badcol_intrinsic, nbadcol_intrinsic,
			 col0, coln, fix_bias);
   satObjmask = phMergeObjmaskChain(sm->masks[S_MASK_SATUR]);
#if 0
/*
 * Write out saturation trail diagnostics
 */
   {
      const int nsat = shChainSize(sm->masks[S_MASK_SATUR]);
      FILE *fil;			/* file descriptor */
      char buff[100];			/* filename */
      int mval;				/* mirrored pixel value */

      sprintf(buff, "satur-%d-%c.dat", 0, fparams->filter);
      fil = fopen(buff, "w");
      shAssert(fil != NULL);

      for(isat = 0; isat < nsat; isat++) {
	 OBJMASK *om = shChainElementGetByPos(sm->masks[S_MASK_SATUR], isat);
	 
	 fprintf(fil, "Saturated OBJMASK %d %d\n", isat, om->npix);
	 for(i = 0; i < om->nspan; i++) {
	    raw_ptr = raw_data->rows_u16[om->s[i].y];
	    for(j = om->s[i].x1; j <= om->s[i].x2; j++) {
	       val = raw_ptr[j];
	       mval = raw_ptr[ncol - j - 1];
	       if(j < samp_2) {
		  val -= inverse_bias_scale*(bias_vec->rows_u16[0][j] + bias0);
		  mval -= inverse_bias_scale*(bias_vec->rows_u16[0][ncol-j-1] +
									bias1);
	       } else {
		  val -= inverse_bias_scale*(bias_vec->rows_u16[0][j] + bias1);
		  mval -= inverse_bias_scale*(bias_vec->rows_u16[0][ncol-j-1] +
									bias0);
	       }
	       fprintf(fil, "%d %d  %d %d\n", om->s[i].y, j, val, mval);
	    }
	 }
      }
      fclose(fil);
   }
#endif
/*
 * Begin data processing. Both i and j are indices into raw_data (or out_data)
 * rather than the full frame.
 *
 * As noted above, the usual manner in which people correct CCD data
 * looks like this:
 *
 *        output = (raw - bias)/flat
 *
 * but we doing things differently.  First, our flatfield vector is an
 * _inverse_ of the usual flat, so we multiply by it:
 *
 *        output = (raw - bias)*flat
 *
 * Secondly, since we do all arithmetic in scaled integer units, to avoid
 * slow-as-molasses floating-point calculations, we've defined our
 * "flat" to be FSHIFT times larger than it ought to be; to return to the
 * natural units of DN we have to divide by FSHIFT:
 *
 *        output = ((raw - cbias)*flat - frac(cbias) + r*FSHIFT/2)/FSHIFT
 *
 * where frac(cbias) is the fractional part of the bias-drift corrected
 * bias cbias, and r is a random number in the range (0, 0.5)
 */
   isat = 0;
   DECLARE_PHRANDOM(rand);
   for(j = 0; j < nrow; j++) {
      raw_ptr = raw_data->ROWS[j];
      out_ptr = out_data->ROWS[j];

      for(amp = 0; amp < namplifier; amp++) {
#if 1
	 shAssert(ndata[amp] != 0);
#else
	 if(ndata[amp] == 0) {
	    shAssert(nbias[amp] == 0);
	    continue;
	 }
#endif
/*
 * If desired, estimate the effects of row-by-row changes in the bias
 *
 * We do this by monitoring the median level of the overclock and bias
 * (extended register) pixels. Note that the extended register was
 * pixels which were read _before_ the data region, so the value from
 * the previous row should be used.
 */
#if NO_FIX_BIAS
	 corr = 0;			/* if NO_FIX_BIAS, the compiler
					   knows that corr == 0 and can
					   optimise it out of the loop */
	 shAssert(sbias[0] == sbias[0] &&
		  sbias[1] == sbias[1]); /* use them to pacify compiler*/
#else
	 if(fix_bias) {
	    int b;			/* loop over bias columns */
	    const PIX *bptr;		/* bias pointer in bias region */
	    int clip = 0;		/* clip quartile? */
	    const PIX *dptr;		/* data pointer in bias region */
	    float med;			/* median of bias region */
	    int nosb_col;		/* number of columns to process in
					   overscan/bias region */

	    bptr = &bias_vec_nt->ROWS[0][sbias[amp]];
	    dptr = &raw_data_nt->ROWS[j][sbias[amp]];
	    for(b = 0; b < nbias[amp]; b++) {
	       oscan_bias[b] = FLT2PIX(SOFT_BIAS + dptr[b] -
				       bptr[b]*inverse_bias_scale);
	    }
	    nosb_col = nbias[amp];
	    
	    if(j > 0) {
	       bptr = &bias_vec_nt->ROWS[0][soscn[amp]];
	       dptr = &raw_data_nt->ROWS[j - 1][soscn[amp]];
	       for(b = 0; b < noscn[amp]; b++) {
		  oscan_bias[b + nosb_col] =
		    SOFT_BIAS + dptr[b] - bptr[b]*inverse_bias_scale;
	       }
	       nosb_col += noscn[amp];
	    }
	    
	    med = phQuartilesGetFromArray(oscan_bias, TYPE_PIX, nosb_col,
					  clip, NULL, NULL, NULL) - SOFT_BIAS;
	    corr = med*FSHIFT;
	 }
#endif
	 /*
	  * This short loop is the actual flat fielding code
	  */
	 startcol = sdata[amp] - col0;
	 endcol = sdata[amp] + ndata[amp] - col0;
	 if(startcol < 0) startcol = 0;
	 if(endcol > coln - col0) endcol = coln - col0;
	 
	 linear_thresh = linear_thresholds[amp];
	 linear_table = linear_tables[amp];

	 for(i = startcol; i < endcol; i++) {
	    r = PHRANDOM;
	    val = raw_ptr[i] - cbias[i];
	    if(val > linear_thresh) {	/* linearise? */
	       val = linear_table[val];	/* linearise. */
	    }
	    val = val*flat_ptr[i] - (frac_bias[i] + corr) + (r & mask);
	    
	    out_ptr[i] = (unsigned)(val + FSHIFT*SOFT_BIAS)/FSHIFT;
	 }
      }
/*
 * Deal with "electronics ghosts", the effect whereby a bright pixel
 * (r, c) appears as a faint ghost, with positive or negative amplitude,
 * at position (r, 2048 - c)
 */
      if(!stamp_flag && fabs(fparams->electronic_ghost_amplitude) > 1e-6) {
	 const float ampl = fparams->electronic_ghost_amplitude;
	 
	 shAssert(ncol == coln - col0);
	 for(i = 0; i < ncol; i++) {
	    out_ptr[i] -= ampl*(out_ptr[ncol - i - 1] - SOFT_BIAS);
	 }
      }
/*
 * Deal with defects and saturation trails
 *
 * We know where the intrinsic bad columns are, but what about the saturated
 * ones? If there are saturation trails in the current row, generate a new
 * set of BADCOLUMNs from the intrinsics and the saturation mask, and then
 * use this set to interpolate. 
 *
 * If there are intrinsic bad columns adjacent to (or included in) the
 * saturated ones, process them as saturated
 *
 * Note that when we are dealing with postage stamps, the saturated pixels
 * are in the coordinate system of the full frame, whereas the intrinsic
 * bad columns have already been converted to the coordinate system of the
 * postage stamp. The classify_badcol() routine needs the latter.
 */
      for(;isat < satObjmask->nspan && 
           satObjmask->s[isat].y - row0 < j;isat++) {
	 continue;
      }

      if(isat == satObjmask->nspan || (satObjmask->s[isat].y)-row0 != j) {
	 /* no saturation to worry about */
	 do_badcol(badcol_intrinsic, nbadcol_intrinsic, out_ptr, ncol, minval);
	 continue;
      }

      sat_x1 = satObjmask->s[isat].x1 - col0;
      sat_x2 = satObjmask->s[isat].x2 - col0;
      
      nbadcol = 0;
      for(i = 0;i < nbadcol_intrinsic;i++) {
	 int bad_x1 = badcol_intrinsic[i].x1;
	 int bad_x2 = badcol_intrinsic[i].x2;

	 if(badcol_intrinsic[i].dfaction == FILCOL) {
	    continue;
	 }

	 while(bad_x1 > sat_x2 + 1) {
	    if(isat < satObjmask->nspan) {
	       badcol[nbadcol].x1 = sat_x1;
	       badcol[nbadcol].x2 = sat_x2;
	       nbadcol++;

	       isat++;
	    }

	    if(isat == satObjmask->nspan || (satObjmask->s[isat].y)-row0 != j) {
	       sat_x1 = ncol + 10;	/* i.e. no more to worry about */
	       sat_x2 = ncol + 11;
	    } else {
	       sat_x1 = satObjmask->s[isat].x1 - col0;
	       sat_x2 = satObjmask->s[isat].x2 - col0;
	    }
	 }

	 if(bad_x2 >= sat_x1 - 1) {	/* bad columns intersection with
					   saturated */
	    if(bad_x2 > sat_x2) {	/* extend saturated to right */
	       sat_x2 = bad_x2;
	       satObjmask->s[isat].x2 = bad_x2 + col0;
	       while(isat + 1 < satObjmask->nspan &&
		     satObjmask->s[isat + 1].y-row0 == j &&
		     sat_x2 >= satObjmask->s[isat + 1].x1 - col0 - 1) {
		  isat++;
		  satObjmask->s[isat].x1 = sat_x1 + col0;
		  sat_x2 = satObjmask->s[isat].x2 - col0;
	       }
	    } else if(bad_x1 < sat_x1) { /* extend saturated to left */
	       sat_x1 = bad_x1;
	       satObjmask->s[isat].x1 = bad_x1 + col0;
	    }
	 } else {
	    badcol[nbadcol++] = badcol_intrinsic[i];
	 }
      }
/*
 * Convert remaining saturated spans to BADCOLUMNs
 */
      for(;isat < satObjmask->nspan && satObjmask->s[isat].y-row0 == j;isat++) {
	 badcol[nbadcol].x1 = satObjmask->s[isat].x1 - col0;
	 badcol[nbadcol].x2 = satObjmask->s[isat].x2 - col0;
	 nbadcol++;
      }
/*
 * classify all of our BADCOLUMNs, then interpolate over them
 */
      classify_badcol(badcol, nbadcol, ncol);

      do_badcol(badcol, nbadcol, out_ptr, ncol, minval);
   }
   END_PHRANDOM(rand);
   
   phObjmaskDel(satObjmask);
#if defined(INTERPOLATE_VERTICAL)
/*
 * do a further interpolation _up_ the columns to remove some of the
 * artifacts; first do the bad columns, then the saturated columns
 */
   for(i = 0;i < nbadcol_intrinsic;i++) {
      const int x1 = badcol_intrinsic[i].x1;
      const int x2 = badcol_intrinsic[i].x2;

      if(badcol_intrinsic[i].dfaction == FILCOL) {
	 continue;
      }

      for(j = x1; j <= x2; j++) {
	 interpolate_vertical(out_data, 2, nrow - 3, j, minval);
      }
   }

   {
      int nspan;			/* == trans->nspan */
      const int nsat = sm->masks[S_MASK_SATUR]->nElements;
      SPAN *s;				/* == trans->s */
      OBJMASK *trans;
      int x, y1, y2;			/* trans->s, renamed as transposed */

      for(i = 0;i < nsat; i++) {
	 satObjmask = shChainElementGetByPos(sm->masks[S_MASK_SATUR], i);
	 trans = (OBJMASK *)phObjmaskTranspose(satObjmask);
	 nspan = trans->nspan;
	 s = trans->s;
	 for(j = 0; j < nspan; j++) {
	    x = s[j].y; y1 = s[j].x1; y2 = s[j].x2;
	    if(y1 < 2) { y1 = 2; }
	    if(y2 >= nrow - 2) { y2 = nrow - 3; }
	    interpolate_vertical(out_data, y1, y2, x, minval);
	 }
	     
	 phObjmaskDel(trans);
      }
   }
#endif
/*
 * Create mask for bad columns
 */
   for(i = 0;i < nbadcol_intrinsic;i++) {
       interpMaskRect = phObjmaskNew(nrow);
       for(j = 0; j < nrow; j++) {
	   interpMaskRect->s[j].y = j;
	   interpMaskRect->s[j].x1 = badcol_intrinsic[i].x1;
	   interpMaskRect->s[j].x2 = badcol_intrinsic[i].x2;
       }
       interpMaskRect->nspan = nrow;
       interpMaskRect->rmin = 0;
       interpMaskRect->rmax = nrow - 1;
       interpMaskRect->cmin = badcol_intrinsic[i].x1;
       interpMaskRect->cmax = badcol_intrinsic[i].x2;
       interpMaskRect->npix = nrow*(badcol_intrinsic[i].x2 -
				    badcol_intrinsic[i].x1 + 1);
       phObjmaskAddToSpanmask(interpMaskRect, sm, S_MASK_INTERP);
   }
/*
 * Fix FILCOL defects
 */
   for(i = 0;i < nbadcol_intrinsic;i++) {
      float sky;			/* sky value from PSP */
      REGION *sreg;
      const int x1 = badcol_intrinsic[i].x1;
      const int x2 = badcol_intrinsic[i].x2;
      
      if(badcol_intrinsic[i].dfaction != FILCOL) {
	 continue;
      }
      
      sreg = shSubRegNew("", out_data, nrow, x2 - x1 + 1, 0, x1, NO_FLAGS);
      shAssert(sreg != NULL);
      
      shRegIntSetVal(sreg, SOFT_BIAS);
      shAssert(phRandomIsInitialised());
      sky = phBinregionInterpolate(fparams->sky, 0, x1);
      phRegIntGaussianAdd(sreg, phRandomGet(), sky, sqrt(sky/phGain(fparams, nrow/2, (x1 + x2)/2)));
      
      shRegDel(sreg);
      
      shChainJoin(notchecked, phSpanmaskFromRect(x1, 0, x2, nrow - 1));
   }
   
   phCanonizeObjmaskChain(notchecked, 0, 1);
/*
 * Label saturated pixels as INTERP
 */
   sm->masks[S_MASK_INTERP] =
                 phObjmaskChainMerge(sm->masks[S_MASK_INTERP],
			      phObjmaskChainCopy(sm->masks[S_MASK_SATUR],0,0));
/*
 * Subtract the correct frame from the counts in the saturation trails
 */
   find_counts_in_trails(-1, out_data, sm->masks[S_MASK_SATUR],
			 0, NULL, NULL, NULL, NULL);
/*
 * Clean yp
 */
   shFree(cbias);
   shFree(frac_bias);
   shRegDel(trimmed);
   shRegDel(trimmed_bias);

   END_CATCH(SIGABRT, SH_GENERIC_ERROR);

   return(SH_SUCCESS);
}


/*****************************************************************************/
/*
 * Produce an array by averaging data in a subregion along either row or 
 * column direction. Subregion within an input region, data, is defined by 
 * row1, row2, col1, and col2 (inclusive). Average can be either median, for 
 * type=1, or mean for type=2. For dir=1 average along rows, i.e. for col1 <= 
 * col <= col2) and thus produce a array parallel to the column direction of 
 * length (row2-row1+1). For dir=2 average along columns and produce a array 
 * parallel to the row direction of length (row2-row1+1). 
 *
 * Return pointer to the array 
 */
static float *
contractReg(REGION *data,	        /* region with the data */
	    int row1,			/* starting row */
	    int row2,			/* ending row */
	    int col1,			/* starting col */
	    int col2,			/* ending col */
	    int dir,			/* 1: average along rows, 
					   2: along cols */
	    int type)			/* 1: median, 2: mean */
{
   float *vector; /* vector with average values */
   PIX *aux;      /* auxiliary vector */
   int i, j;      /* counters */
   int N, Naux;   /* vector sizes */
   float median;  /* median */
   float mean;    /* mean */
   PIX *row_ptr;  /* row pointer */

   /* some sanity checks */
   shAssert(row1 <= row2);   
   shAssert(col1 <= col2);   
   shAssert(row1 >= 0);
   shAssert(row2 < data->nrow);
   shAssert(col1 >= 0);
   shAssert(col2 < data->ncol);

   /* determine the lengths of vectors */
   if(dir == 1) {
      N = row2 - row1 + 1;
      Naux = col2 - col1 + 1;    
   } else {
      N = col2 - col1 + 1;
      Naux = row2 - row1 + 1;
   }
   vector = shMalloc(N * sizeof(float));
   aux = shMalloc(Naux * sizeof(PIX));

   /* do the work */
   if(dir == 1) {
      for(j = row1; j <= row2; j++) {
         row_ptr = data->ROWS[j];
         for(i = col1; i <= col2; i++) {
            aux[i-col1] = row_ptr[i];
         }
         median = phQuartilesGetFromArray(aux, TYPE_PIX, Naux, 0, NULL,
				         &mean, NULL);
         vector[j-row1] = (type == 1) ? median : mean;
      }
   } else {
      for(i = col1; i <= col2; i++) {
         for(j = row1; j <= row2; j++) {
            row_ptr = data->ROWS[j];
            aux[j-row1] = row_ptr[i];
         }
         median = phQuartilesGetFromArray(aux, TYPE_PIX, Naux, 0, NULL,
				         &mean, NULL);
         vector[i-col1] = (type == 1) ? median : mean;
      }
   }

   shFree(aux);
   return vector;
}


/*****************************************************************************/
/* Calculate linear interpolation coefficients, C, such that
 * I(xji) = sum_k{C(i,k) * I(j+k)} for k = -2, -1, 0, 1, where 
 * xji = x(j-1) + i*0.25*[x(j)-x(j-1)], with i=1,2,3
 * Coefficients C(i,k) are calculated by make_interp and in general depend on 
 * sigma. This function utilizes analytic approximation to C(sigma) 
 * determined by fitting a quadratic parabola to a grid of C(sigma) for 
 * case "#...#...#...#", with  0.8 < sigma < 2.0 (1.3-3.2 arcsec).
 * It is empirically determined that the best interpolation for horizontal
 * trails is obtained with sigma = 1.25 * psf_width, with psf_width given in 
 * arcsec.
 * Because of symmetry there are only 6 independent coefficients:
 *  C[0]  = C1,-2 = C3,1
 *  C[1]  = C1,-1 = C3,0
 *  C[2]  = C1,0  = C3,-1
 *  C[3]  = C1,1  = C3,-2
 *  C[4]  = C2,-2 = C2,1
 *  C[5]  = C2,-1 = C2,0
 */
static float *
get_interp_coeff(float psf_width)
{
   float sig, sig2; /* sigma (for make_interp) */
   float sum;
   float *C; /* coefficients */

   C = shMalloc(6 * sizeof(float)); 

   /* empirical relationship for horizontal trails 
    * one would expect 1.2/(4*0.4)*1.41=1.06 where 1.2 is the HT/PSF width ratio, 
    * 4 is the number of interpolation segments per pixel, 0.4 arcsec is the pixel 
    * size, and 1.41 comes from the autocorrelation width
    * that is, the factor 1.25 seems to be 1.18 times larger than expected, possibly
    * because only the PSF core matters, while the PSF width is determined from neff  
    */
   sig = 1.25 * psf_width;
   sig2 = sig * sig;

   C[0] =  0.4011 - 0.5059 * sig + 0.1247 * sig2;
   C[1] =  0.4615 + 0.4739 * sig - 0.1303 * sig2;
   C[2] = -0.1393 + 0.3450 * sig - 0.0669 * sig2;
   C[3] =  0.2766 - 0.3126 * sig + 0.0725 * sig2;
   C[4] =  0.5989 - 0.7085 * sig + 0.1763 * sig2;
   C[5] = -0.0988 + 0.7086 * sig - 0.1762 * sig2;

   /* normalize to 1 to account for fitting errors */
   sum =  C[0] + C[1] + C[2] + C[3];
   C[0] /= sum; C[1] /= sum; C[2] /= sum; C[3] /= sum;
   sum = 2*(C[4] + C[5]);
   C[4] /= sum; C[5] /= sum;

   return C;
}

/*****************************************************************************/
/* Interpolate I(x), 0 < x < 1, by using coefficients, C, such that
 * 
 * I(0.25) = C[0]*Im2 + C[1]*Im1 + C[2]*Ip1 + C[3]*Ip2
 * I(0.50) = C[4]*Im2 + C[5]*Im1 + C[5]*Ip1 + C[4]*Ip2
 * I(0.75) = C[3]*Im2 + C[2]*Im1 + C[1]*Ip1 + C[0]*Ip2
 * 
 * For intermediate values of x use linear interpolation 
 */ 
static float 
get_corr(float *C, float Im2, float Im1, float Ip1, float Ip2, float x)
{
   float x1, x2, y1, y2, corr;
   
   /* end values */
   if (x <= 0) {return Im1;}
   if (x >= 1) {return Ip1;}  

   /* determine the interval */
   if (x < 0.25) {
       x1 = 0.0;
       x2 = 0.25;
       y1 = Im1;
       y2 = C[0]*Im2 + C[1]*Im1 + C[2]*Ip1 + C[3]*Ip2;
   } else if (x < 0.50) {
       x1 = 0.25;
       x2 = 0.50;
       y1 = C[0]*Im2 + C[1]*Im1 + C[2]*Ip1 + C[3]*Ip2;
       y2 = C[4]*Im2 + C[5]*Im1 + C[5]*Ip1 + C[4]*Ip2;
   } else if (x < 0.75) {
       x1 = 0.50;
       x2 = 0.75;
       y1 = C[4]*Im2 + C[5]*Im1 + C[5]*Ip1 + C[4]*Ip2;
       y2 = C[3]*Im2 + C[2]*Im1 + C[1]*Ip1 + C[0]*Ip2;    
   } else {
       x1 = 0.75;
       x2 = 1.00;
       y1 = C[3]*Im2 + C[2]*Im1 + C[1]*Ip1 + C[0]*Ip2;    
       y2 = Ip1;
   }

   /* evaluate correction */
   corr = y1 + (y2-y1)/(x2-x1)*(x-x1);

   return corr;
}

/*****************************************************************************/
/*
 * Classify an array of BADCOLUMNs
 */
static void
classify_badcol(BADCOLUMN *badcol, int nbadcol, int ncol)
{
   int i;
   int nbad;
   
   for(i = 0;i < nbadcol;i++) {
      nbad = badcol[i].x2 - badcol[i].x1 + 1;
      shAssert(nbad >= 1);
      
      if(badcol[i].x1 == 0) {
	 if(nbad >= WIDE_DEFECT) {
	    badcol[i].pos = BAD_WIDE_LEFT;
	    badcol[i].type = 03;
	 } else {
	    badcol[i].pos = BAD_LEFT;
	    badcol[i].type = 03 << nbad;
	 }
      } else if(badcol[i].x1 == 1) {	/* only second column is usable */
	 if(nbad >= WIDE_DEFECT) {
	    badcol[i].pos = BAD_WIDE_NEAR_LEFT;
	    badcol[i].type = (01 << 2) | 03;
	 } else {
	    badcol[i].pos = BAD_NEAR_LEFT;
	    badcol[i].type = (01 << (nbad + 2)) | 03;
	 }
      } else if(badcol[i].x2 == ncol - 2) { /* use only penultimate column */
	 if(nbad >= WIDE_DEFECT) {
	    badcol[i].pos = BAD_WIDE_NEAR_RIGHT;
	    badcol[i].type = (03 << 2) | 02;
	 } else {
	    badcol[i].pos = BAD_NEAR_RIGHT;
	    badcol[i].type = (03 << (nbad + 2)) | 02;
	 }
      } else if(badcol[i].x2 == ncol - 1) {
	 if(nbad >= WIDE_DEFECT) {
	    badcol[i].pos = BAD_WIDE_RIGHT;
	    badcol[i].type = 03;
	 } else {
	    badcol[i].pos = BAD_RIGHT;
	    badcol[i].type = 03 << nbad;
	 }
      } else if(nbad >= WIDE_DEFECT) {
	 badcol[i].pos = BAD_WIDE;
	 badcol[i].type = (03 << 2) | 03;
      } else {
	 badcol[i].pos = BAD_MIDDLE;
	 badcol[i].type = (03 << (nbad + 2)) | 03;
      }
/*
 * look for bad columns in regions that we'll get `good' values from.
 *
 * We know that no two BADCOLUMNS are adjacent.
 */
      if(i > 0) {
	 int nshift;			/* number of bits to shift to
					   get to left edge of defect pattern */
	 shAssert(badcol[i-1].x2 < badcol[i].x1);
	 switch (badcol[i].pos) {
	  case BAD_WIDE:		/* no bits */
	  case BAD_WIDE_NEAR_LEFT:	/*       are used to encode */
	  case BAD_WIDE_NEAR_RIGHT:	/*            the bad section of data */
	    nshift = 0; break;
	  default:
	    nshift = nbad; break;
	 }

	 if(badcol[i].pos == BAD_RIGHT || badcol[i].pos == BAD_WIDE_RIGHT) {
	    ;				/* who cares about pixels to the left? */
	 } else {
	    if(badcol[i - 1].x2 == badcol[i].x1 - 2) {
	       badcol[i].type &= ~(02 << (nshift + 2));
	    }
	 }
      }
      if(i < nbadcol - 1) {
	 if(badcol[i].pos == BAD_LEFT || badcol[i].pos == BAD_WIDE_LEFT) {
	    ;				/* who cares about pixels to the right? */
	 } else {
	    if(badcol[i].x2 == badcol[i + 1].x1 - 2) {
	       badcol[i].type &= ~01;
	    }
	 }
      }
   }
}

/*****************************************************************************/
/*
 * Interpolate over the defects in a given line of data. In the comments,
 * a bad pixel is written as . and a good one as #, so the simple case of
 * one bad pixel is ##.## (a type of 033)
 */
static void
do_badcol(const BADCOLUMN *badcol,
	  int nbadcol,
	  PIX *out,			/* data to fix */
	  int ncol,			/* number of elements in out */
	  int min)			/* minimum acceptable value */
{
   int bad_x1, bad_x2;
   int i,j;
   float out1_2, out1_1, out2_1, out2_2; /* out[bad_x1-2], ..., out[bad_x2+2]*/
   int val;				/* unpack a pixel value */

   for(i = 0;i < nbadcol;i++) {
      bad_x1 = badcol[i].x1;
      bad_x2 = badcol[i].x2;

      switch (badcol[i].pos) {
       case BAD_LEFT:
	 shAssert(bad_x1 >= 0 && bad_x2 + 2 < ncol);
	 out2_1 = out[bad_x2 + 1];
	 out2_2 = out[bad_x2 + 2];

	 switch (badcol[i].type) {
          case 06:              /* .##, <noise^2> = 0 */
            val = FLT2PIX(1.4288*out2_1 - 0.4288*out2_2);
            out[bad_x2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 014:             /* ..##, <noise^2> = 0 */
            val = FLT2PIX(1.0933*out2_1 - 0.0933*out2_2);
            out[bad_x1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(1.4288*out2_1 - 0.4288*out2_2);
            out[bad_x2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 030:             /* ...##, <noise^2> = 0 */
            val = FLT2PIX(0.6968*out2_1 + 0.3032*out2_2);
            out[bad_x1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(1.0933*out2_1 - 0.0933*out2_2);
            out[bad_x2 - 1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(1.4288*out2_1 - 0.4288*out2_2);
            out[bad_x2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 060:             /* ....##, <noise^2> = 0 */
            val = FLT2PIX(0.5370*out2_1 + 0.4630*out2_2);
            out[bad_x1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6968*out2_1 + 0.3032*out2_2);
            out[bad_x1 + 1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(1.0933*out2_1 - 0.0933*out2_2);
            out[bad_x2 - 1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(1.4288*out2_1 - 0.4288*out2_2);
            out[bad_x2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 0140:            /* .....##, <noise^2> = 0 */
            val = FLT2PIX(0.5041*out2_1 + 0.4959*out2_2);
            out[bad_x1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5370*out2_1 + 0.4630*out2_2);
            out[bad_x1 + 1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6968*out2_1 + 0.3032*out2_2);
            out[bad_x2 - 2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(1.0933*out2_1 - 0.0933*out2_2);
            out[bad_x2 - 1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(1.4288*out2_1 - 0.4288*out2_2);
            out[bad_x2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 0300:            /* ......##, <noise^2> = 0 */
            val = FLT2PIX(0.5003*out2_1 + 0.4997*out2_2);
            out[bad_x1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5041*out2_1 + 0.4959*out2_2);
            out[bad_x1 + 1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5370*out2_1 + 0.4630*out2_2);
            out[bad_x1 + 2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6968*out2_1 + 0.3032*out2_2);
            out[bad_x2 - 2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(1.0933*out2_1 - 0.0933*out2_2);
            out[bad_x2 - 1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(1.4288*out2_1 - 0.4288*out2_2);
            out[bad_x2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 0600:            /* .......##, <noise^2> = 0 */
            val = FLT2PIX(0.5000*out2_1 + 0.5000*out2_2);
            out[bad_x1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5003*out2_1 + 0.4997*out2_2);
            out[bad_x1 + 1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5041*out2_1 + 0.4959*out2_2);
            out[bad_x1 + 2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5370*out2_1 + 0.4630*out2_2);
            out[bad_x2 - 3] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6968*out2_1 + 0.3032*out2_2);
            out[bad_x2 - 2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(1.0933*out2_1 - 0.0933*out2_2);
            out[bad_x2 - 1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(1.4288*out2_1 - 0.4288*out2_2);
            out[bad_x2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 01400:           /* ........##, <noise^2> = 0 */
            val = FLT2PIX(0.5000*out2_1 + 0.5000*out2_2);
            out[bad_x1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5000*out2_1 + 0.5000*out2_2);
            out[bad_x1 + 1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5003*out2_1 + 0.4997*out2_2);
            out[bad_x1 + 2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5041*out2_1 + 0.4959*out2_2);
            out[bad_x1 + 3] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5370*out2_1 + 0.4630*out2_2);
            out[bad_x2 - 3] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6968*out2_1 + 0.3032*out2_2);
            out[bad_x2 - 2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(1.0933*out2_1 - 0.0933*out2_2);
            out[bad_x2 - 1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(1.4288*out2_1 - 0.4288*out2_2);
            out[bad_x2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 03000:           /* .........##, <noise^2> = 0 */
            val = FLT2PIX(0.5000*out2_1 + 0.5000*out2_2);
            out[bad_x1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5000*out2_1 + 0.5000*out2_2);
            out[bad_x1 + 1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5000*out2_1 + 0.5000*out2_2);
            out[bad_x1 + 2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5003*out2_1 + 0.4997*out2_2);
            out[bad_x1 + 3] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5041*out2_1 + 0.4959*out2_2);
            out[bad_x2 - 4] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5370*out2_1 + 0.4630*out2_2);
            out[bad_x2 - 3] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6968*out2_1 + 0.3032*out2_2);
            out[bad_x2 - 2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(1.0933*out2_1 - 0.0933*out2_2);
            out[bad_x2 - 1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(1.4288*out2_1 - 0.4288*out2_2);
            out[bad_x2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 06000:           /* ..........##, <noise^2> = 0 */
            val = FLT2PIX(0.5000*out2_1 + 0.5000*out2_2);
            out[bad_x1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5000*out2_1 + 0.5000*out2_2);
            out[bad_x1 + 1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5000*out2_1 + 0.5000*out2_2);
            out[bad_x1 + 2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5000*out2_1 + 0.5000*out2_2);
            out[bad_x1 + 3] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5003*out2_1 + 0.4997*out2_2);
            out[bad_x1 + 4] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5041*out2_1 + 0.4959*out2_2);
            out[bad_x2 - 4] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5370*out2_1 + 0.4630*out2_2);
            out[bad_x2 - 3] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6968*out2_1 + 0.3032*out2_2);
            out[bad_x2 - 2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(1.0933*out2_1 - 0.0933*out2_2);
            out[bad_x2 - 1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(1.4288*out2_1 - 0.4288*out2_2);
            out[bad_x2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
	  default:
	    shFatal("Unsupported defect type: LEFT 0%o",badcol[i].type);
	    break;			/* NOTREACHED */
	 }
	 break;
       case BAD_WIDE_LEFT:
	 shAssert(bad_x1 >= 0);
	 if(bad_x2 + 2 >= ncol) {	/* left defect extends near
					   right edge of data! */
	    if(bad_x2 == ncol - 2) {	/* one column remains */
	       val = FLT2PIX(out[ncol - 1]);
	    } else {
	       val = FLT2PIX(MAX_U16);		/* there is no information */
	    }
	    for(j = bad_x1;j <= bad_x2;j++) {
	       out[j] = val;
	    }
	    break;
	 }
	 out2_1 = out[bad_x2 + 1];
	 out2_2 = out[bad_x2 + 2];

	 switch (badcol[i].type) {
          case 02:            /* ?#., <noise^2> = 0 */
	    val = FLT2PIX(1.0000*out2_1);
            val = (val < min) ? out2_1 : (val > MAX_U16 ? MAX_U16 : val);

	    for(j = bad_x1;j <= bad_x2;j++) {
	       out[j] = val;
	    }
	    break;
          case 03:            /* ?##, <noise^2> = 0 */
	    val = FLT2PIX(0.5000*out2_1 + 0.5000*out2_2);
            val = (val < min) ? out2_1 : (val > MAX_U16 ? MAX_U16 : val);

	    for(j = bad_x1;j < bad_x2 - 5;j++) {
	       out[j] = val;
	    }

            val = FLT2PIX(0.5003*out2_1 + 0.4997*out2_2);
            out[bad_x2 - 5] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5041*out2_1 + 0.4959*out2_2);
            out[bad_x2 - 4] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5370*out2_1 + 0.4630*out2_2);
            out[bad_x2 - 3] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6968*out2_1 + 0.3032*out2_2);
            out[bad_x2 - 2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(1.0933*out2_1 - 0.0933*out2_2);
            out[bad_x2 - 1] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(1.4288*out2_1 - 0.4288*out2_2);
            out[bad_x2] = (val < min) ? out2_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
	  default:
	    shFatal("Unsupported defect type: WIDE_LEFT 0%o",badcol[i].type);
	    break;			/* NOTREACHED */
	 }
	 
	 break;
       case BAD_RIGHT:
	 shAssert(bad_x1 >= 2 && bad_x2 < ncol);

	 out1_2 = out[bad_x1 - 2];
	 out1_1 = out[bad_x1 - 1];
	 
	 switch (badcol[i].type) {
          case 06:              /* ##., <noise^2> = 0 */
            val = FLT2PIX(-0.4288*out1_2 + 1.4288*out1_1);
            out[bad_x2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 014:             /* ##.., <noise^2> = 0 */
            val = FLT2PIX(-0.4288*out1_2 + 1.4288*out1_1);
            out[bad_x1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.0933*out1_2 + 1.0933*out1_1);
            out[bad_x2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 030:             /* ##..., <noise^2> = 0 */
            val = FLT2PIX(-0.4288*out1_2 + 1.4288*out1_1);
            out[bad_x1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.0933*out1_2 + 1.0933*out1_1);
            out[bad_x2 - 1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3032*out1_2 + 0.6968*out1_1);
            out[bad_x2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 060:             /* ##...., <noise^2> = 0 */
            val = FLT2PIX(-0.4288*out1_2 + 1.4288*out1_1);
            out[bad_x1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.0933*out1_2 + 1.0933*out1_1);
            out[bad_x1 + 1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3032*out1_2 + 0.6968*out1_1);
            out[bad_x2 - 1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4630*out1_2 + 0.5370*out1_1);
            out[bad_x2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 0140:            /* ##....., <noise^2> = 0 */
            val = FLT2PIX(-0.4288*out1_2 + 1.4288*out1_1);
            out[bad_x1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.0933*out1_2 + 1.0933*out1_1);
            out[bad_x1 + 1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3032*out1_2 + 0.6968*out1_1);
            out[bad_x2 - 2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4630*out1_2 + 0.5370*out1_1);
            out[bad_x2 - 1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4959*out1_2 + 0.5041*out1_1);
            out[bad_x2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 0300:            /* ##......, <noise^2> = 0 */
            val = FLT2PIX(-0.4288*out1_2 + 1.4288*out1_1);
            out[bad_x1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.0933*out1_2 + 1.0933*out1_1);
            out[bad_x1 + 1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3032*out1_2 + 0.6968*out1_1);
            out[bad_x1 + 2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4630*out1_2 + 0.5370*out1_1);
            out[bad_x2 - 2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4959*out1_2 + 0.5041*out1_1);
            out[bad_x2 - 1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4997*out1_2 + 0.5003*out1_1);
            out[bad_x2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 0600:            /* ##......., <noise^2> = 0 */
            val = FLT2PIX(-0.4288*out1_2 + 1.4288*out1_1);
            out[bad_x1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.0933*out1_2 + 1.0933*out1_1);
            out[bad_x1 + 1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3032*out1_2 + 0.6968*out1_1);
            out[bad_x1 + 2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4630*out1_2 + 0.5370*out1_1);
            out[bad_x2 - 3] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4959*out1_2 + 0.5041*out1_1);
            out[bad_x2 - 2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4997*out1_2 + 0.5003*out1_1);
            out[bad_x2 - 1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5000*out1_2 + 0.5000*out1_1);
            out[bad_x2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 01400:           /* ##........, <noise^2> = 0 */
            val = FLT2PIX(-0.4288*out1_2 + 1.4288*out1_1);
            out[bad_x1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.0933*out1_2 + 1.0933*out1_1);
            out[bad_x1 + 1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3032*out1_2 + 0.6968*out1_1);
            out[bad_x1 + 2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4630*out1_2 + 0.5370*out1_1);
            out[bad_x1 + 3] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4959*out1_2 + 0.5041*out1_1);
            out[bad_x2 - 3] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4997*out1_2 + 0.5003*out1_1);
            out[bad_x2 - 2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5000*out1_2 + 0.5000*out1_1);
            out[bad_x2 - 1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5000*out1_2 + 0.5000*out1_1);
            out[bad_x2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 03000:           /* ##........., <noise^2> = 0 */
            val = FLT2PIX(-0.4288*out1_2 + 1.4288*out1_1);
            out[bad_x1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.0933*out1_2 + 1.0933*out1_1);
            out[bad_x1 + 1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3032*out1_2 + 0.6968*out1_1);
            out[bad_x1 + 2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4630*out1_2 + 0.5370*out1_1);
            out[bad_x1 + 3] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4959*out1_2 + 0.5041*out1_1);
            out[bad_x2 - 4] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4997*out1_2 + 0.5003*out1_1);
            out[bad_x2 - 3] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5000*out1_2 + 0.5000*out1_1);
            out[bad_x2 - 2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5000*out1_2 + 0.5000*out1_1);
            out[bad_x2 - 1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5000*out1_2 + 0.5000*out1_1);
            out[bad_x2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 06000:           /* ##.........., <noise^2> = 0 */
            val = FLT2PIX(-0.4288*out1_2 + 1.4288*out1_1);
            out[bad_x1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.0933*out1_2 + 1.0933*out1_1);
            out[bad_x1 + 1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3032*out1_2 + 0.6968*out1_1);
            out[bad_x1 + 2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4630*out1_2 + 0.5370*out1_1);
            out[bad_x1 + 3] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4959*out1_2 + 0.5041*out1_1);
            out[bad_x1 + 4] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4997*out1_2 + 0.5003*out1_1);
            out[bad_x2 - 4] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5000*out1_2 + 0.5000*out1_1);
            out[bad_x2 - 3] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5000*out1_2 + 0.5000*out1_1);
            out[bad_x2 - 2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5000*out1_2 + 0.5000*out1_1);
            out[bad_x2 - 1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5000*out1_2 + 0.5000*out1_1);
            out[bad_x2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
	  default:
	    shFatal("Unsupported defect type: RIGHT 0%o",badcol[i].type);
	    break;			/* NOTREACHED */
	 }
	 break;
       case BAD_WIDE_RIGHT:
	 shAssert(bad_x2 < ncol);

	 if(bad_x1 < 2) {		/* right defect extends near
					   left edge of data! */
	    if(bad_x1 == 1) {		/* one column remains */
	       val = FLT2PIX(out[0]);
	    } else {
	       val = FLT2PIX(MAX_U16);		/* there is no information */
	    }
	    for(j = bad_x1;j <= bad_x2;j++) {
	       out[j] = val;
	    }
	    break;
	 }
	 
	 out1_2 = out[bad_x1 - 2];
	 out1_1 = out[bad_x1 - 1];

	 switch (badcol[i].type) {
	  case 03:			/* ##?, S/N = infty */
            val = FLT2PIX(-0.4288*out1_2 + 1.4288*out1_1);
            out[bad_x1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.0933*out1_2 + 1.0933*out1_1);
            out[bad_x1 + 1] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3032*out1_2 + 0.6968*out1_1);
            out[bad_x1 + 2] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4630*out1_2 + 0.5370*out1_1);
            out[bad_x1 + 3] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4959*out1_2 + 0.5041*out1_1);
            out[bad_x1 + 4] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4997*out1_2 + 0.5003*out1_1);
            out[bad_x1 + 5] = (val < min) ? out1_1 :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5000*out1_2 + 0.5000*out1_1);
            val = (val < min) ? out1_1 : (val > MAX_U16 ? MAX_U16 : val);

	    for(j = bad_x1 + 6;j <= bad_x2;j++) {
	       out[j] = val;
	    }
	    break;
	  default:
	    shFatal("Unsupported defect type: WIDE_RIGHT 0%o",badcol[i].type);
	    break;			/* NOTREACHED */
	 }
	 break;
       case BAD_MIDDLE:
       case BAD_NEAR_LEFT:
       case BAD_NEAR_RIGHT:
	 if(badcol[i].pos == BAD_MIDDLE) {
	    shAssert(bad_x1 >= 2 && bad_x2 + 2 < ncol);
	    out1_2 = out[bad_x1 - 2];
	    out2_2 = out[bad_x2 + 2];
	 } else if(badcol[i].pos == BAD_NEAR_LEFT) {
	    shAssert(bad_x1 >= 1 && bad_x2 + 2 < ncol);
	    out1_2 = -1;		/* NOTUSED */
	    out2_2 = out[bad_x2 + 2];
	 } else if(badcol[i].pos == BAD_NEAR_RIGHT) {
	    shAssert(bad_x1 >= 2 && bad_x2 + 1 < ncol);
	    out1_2 = out[bad_x1 - 2];
	    out2_2 = -1;		/* NOTUSED */
	 } else {
	    shFatal("Unknown defect classification %d (%s:%d)",badcol[i].pos,
		    __FILE__,__LINE__);
	    out1_2 = out2_2 = -1;	/* NOTUSED */
	 }
	 out1_1 = out[bad_x1 - 1];
	 out2_1 = out[bad_x2 + 1];
	 
	 switch (badcol[i].type) {
          case 012:             /* #.#., <noise^2> = 0, sigma = 1 */
            val = FLT2PIX(0.5000*out1_1 + 0.5000*out2_1);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)):
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 013:		/* #.##, <noise^2> = 0 */
            val = FLT2PIX(0.4875*out1_1 + 0.8959*out2_1 - 0.3834*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 022:             /* #..#., <noise^2> = 0, sigma = 1 */
            val = FLT2PIX(0.7297*out1_1 + 0.2703*out2_1);
            out[bad_x1] = (val < 0) ? 0 : (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2703*out1_1 + 0.7297*out2_1);
            out[bad_x2] = (val < 0) ? 0 : (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 023:		/* #..##, <noise^2> = 0 */
            val = FLT2PIX(0.7538*out1_1 + 0.5680*out2_1 - 0.3218*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3095*out1_1 + 1.2132*out2_1 - 0.5227*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 032:		/* ##.#., <noise^2> = 0 */
            val = FLT2PIX(-0.3834*out1_2 + 0.8959*out1_1 + 0.4875*out2_1);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 033:		/* ##.##, <noise^2> = 0 */
	                        /* These coefficients are also available as
				   INTERP_1_C1 and INTERP_1_C2 */
            val = FLT2PIX(-0.2737*out1_2 + 0.7737*out1_1 + 0.7737*out2_1 -
								0.2737*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
	  case 042:			/* #...#., <noise^2> = 0, sigma = 1 */
            val = FLT2PIX(0.8430*out1_1 + 0.1570*out2_1);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
					       (val > MAX_U16 ? MAX_U16 : val);
	    
            val = FLT2PIX(0.5000*out1_1 + 0.5000*out2_1);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
					       (val > MAX_U16 ? MAX_U16 : val);
	    
            val = FLT2PIX(0.1570*out1_1 + 0.8430*out2_1);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
					       (val > MAX_U16 ? MAX_U16 : val);
	    
            break;
          case 043:             /* #...##, <noise^2> = 0 */
            val = FLT2PIX(0.8525*out1_1 + 0.2390*out2_1 - 0.0915*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5356*out1_1 + 0.8057*out2_1 - 0.3413*out2_2);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2120*out1_1 + 1.3150*out2_1 - 0.5270*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;	    
          case 062:		/* ##..#., <noise^2> = 0 */
            val = FLT2PIX(-0.5227*out1_2 + 1.2132*out1_1 + 0.3095*out2_1);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.3218*out1_2 + 0.5680*out1_1 + 0.7538*out2_1);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 063:		/* ##..##, <noise^2> = 0 */
            val = FLT2PIX(-0.4793*out1_2 + 1.1904*out1_1 +
						0.5212*out2_1 - 0.2323*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.2323*out1_2 + 0.5212*out1_1 +
						1.1904*out2_1 - 0.4793*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 0102:            /* #....#., <noise^2> = 0, sigma = 1 */
            val = FLT2PIX(0.8810*out1_1 + 0.1190*out2_1);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6315*out1_1 + 0.3685*out2_1);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3685*out1_1 + 0.6315*out2_1);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1190*out1_1 + 0.8810*out2_1);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 0103:            /* #....##, <noise^2> = 0 */
            val = FLT2PIX(0.8779*out1_1 + 0.0945*out2_1 + 0.0276*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6327*out1_1 + 0.3779*out2_1 - 0.0106*out2_2);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4006*out1_1 + 0.8914*out2_1 - 0.2920*out2_2);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1757*out1_1 + 1.3403*out2_1 - 0.5160*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 0142:		/* ##...#., <noise^2> = 0 */
            val = FLT2PIX(-0.5270*out1_2 + 1.3150*out1_1 + 0.2120*out2_1);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.3413*out1_2 + 0.8057*out1_1 + 0.5356*out2_1);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.0915*out1_2 + 0.2390*out1_1 + 0.8525*out2_1);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 0143:		/* ##...##, <noise^2> = 0 */
            val = FLT2PIX(-0.5230*out1_2 + 1.3163*out1_1 +
						0.2536*out2_1 - 0.0469*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.3144*out1_2 + 0.8144*out1_1 +
						0.8144*out2_1 - 0.3144*out2_2);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.0469*out1_2 + 0.2536*out1_1 +
						1.3163*out2_1 - 0.5230*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 0202:            /* #.....#., <noise^2> = 0, sigma = 1 */
            val = FLT2PIX(0.8885*out1_1 + 0.1115*out2_1);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
					       (val > MAX_U16 ? MAX_U16 : val);
	    
            val = FLT2PIX(0.6748*out1_1 + 0.3252*out2_1);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
					       (val > MAX_U16 ? MAX_U16 : val);
	    
            val = FLT2PIX(0.5000*out1_1 + 0.5000*out2_1);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
					       (val > MAX_U16 ? MAX_U16 : val);
	    
            val = FLT2PIX(0.3252*out1_1 + 0.6748*out2_1);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
					       (val > MAX_U16 ? MAX_U16 : val);
	    
            val = FLT2PIX(0.1115*out1_1 + 0.8885*out2_1);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
					       (val > MAX_U16 ? MAX_U16 : val);
	    
            break;
          case 0203:            /* #.....##, <noise^2> = 0 */
            val = FLT2PIX(0.8824*out1_1 + 0.0626*out2_1 + 0.0549*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6601*out1_1 + 0.2068*out2_1 + 0.1331*out2_2);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4938*out1_1 + 0.4498*out2_1 + 0.0564*out2_2);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3551*out1_1 + 0.9157*out2_1 - 0.2708*out2_2);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1682*out1_1 + 1.3447*out2_1 - 0.5129*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 0302:		/* ##....#., <noise^2> = 0 */
            val = FLT2PIX(-0.5160*out1_2 + 1.3403*out1_1 + 0.1757*out2_1);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.2920*out1_2 + 0.8914*out1_1 + 0.4006*out2_1);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.0106*out1_2 + 0.3779*out1_1 + 0.6327*out2_1);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0276*out1_2 + 0.0945*out1_1 + 0.8779*out2_1);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 0303:		/* ##....##, <noise^2> = 0 */
            val = FLT2PIX(-0.5197*out1_2 + 1.3370*out1_1 +
						0.1231*out2_1 + 0.0596*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.2924*out1_2 + 0.8910*out1_1 +
						0.3940*out2_1 + 0.0074*out2_2);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0074*out1_2 + 0.3940*out1_1 +
						0.8910*out2_1 - 0.2924*out2_2);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0596*out1_2 + 0.1231*out1_1 +
						1.3370*out2_1 - 0.5197*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 0402:            /* #......#., <noise^2> = 0, sigma = 1 */
            val = FLT2PIX(0.8893*out1_1 + 0.1107*out2_1);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6830*out1_1 + 0.3170*out2_1);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5435*out1_1 + 0.4565*out2_1);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4565*out1_1 + 0.5435*out2_1);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3170*out1_1 + 0.6830*out2_1);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1107*out1_1 + 0.8893*out2_1);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 0403:            /* #......##, <noise^2> = 0 */
            val = FLT2PIX(0.8829*out1_1 + 0.0588*out2_1 + 0.0583*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6649*out1_1 + 0.1716*out2_1 + 0.1635*out2_2);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5212*out1_1 + 0.2765*out2_1 + 0.2024*out2_2);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4477*out1_1 + 0.4730*out2_1 + 0.0793*out2_2);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3465*out1_1 + 0.9201*out2_1 - 0.2666*out2_2);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1673*out1_1 + 1.3452*out2_1 - 0.5125*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 0602:		/* ##.....#., <noise^2> = 0 */
            val = FLT2PIX(-0.5129*out1_2 + 1.3447*out1_1 + 0.1682*out2_1);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.2708*out1_2 + 0.9157*out1_1 + 0.3551*out2_1);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0564*out1_2 + 0.4498*out1_1 + 0.4938*out2_1);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1331*out1_2 + 0.2068*out1_1 + 0.6601*out2_1);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0549*out1_2 + 0.0626*out1_1 + 0.8824*out2_1);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 0603:		/* ##.....##, <noise^2> = 0 */
            val = FLT2PIX(-0.5179*out1_2 + 1.3397*out1_1 +
						0.0928*out2_1 + 0.0854*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.2796*out1_2 + 0.9069*out1_1 +
						0.2231*out2_1 + 0.1495*out2_2);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0533*out1_2 + 0.4467*out1_1 +
						0.4467*out2_1 + 0.0533*out2_2);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1495*out1_2 + 0.2231*out1_1 +
						0.9069*out2_1 - 0.2796*out2_2);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0854*out1_2 + 0.0928*out1_1 +
						1.3397*out2_1 - 0.5179*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
	  case 01002:		/* #.......#., <noise^2> = 0, sigma = 1 */
	    val = FLT2PIX(0.8894*out1_1 + 0.1106*out2_1);
	    out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
					 (val > MAX_U16 ? MAX_U16 : val);

	    val = FLT2PIX(0.6839*out1_1 + 0.3161*out2_1);
	    out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
					 (val > MAX_U16 ? MAX_U16 : val);

	    val = FLT2PIX(0.5517*out1_1 + 0.4483*out2_1);
	    out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
					 (val > MAX_U16 ? MAX_U16 : val);

	    val = FLT2PIX(0.5000*out1_1 + 0.5000*out2_1);
	    out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
					 (val > MAX_U16 ? MAX_U16 : val);

	    val = FLT2PIX(0.4483*out1_1 + 0.5517*out2_1);
	    out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
					 (val > MAX_U16 ? MAX_U16 : val);

	    val = FLT2PIX(0.3161*out1_1 + 0.6839*out2_1);
	    out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
					 (val > MAX_U16 ? MAX_U16 : val);

	    val = FLT2PIX(0.1106*out1_1 + 0.8894*out2_1);
	    out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
					 (val > MAX_U16 ? MAX_U16 : val);

	    break;
          case 01003:           /* #.......##, <noise^2> = 0 */
            val = FLT2PIX(0.8829*out1_1 + 0.0585*out2_1 + 0.0585*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6654*out1_1 + 0.1676*out2_1 + 0.1670*out2_2);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5260*out1_1 + 0.2411*out2_1 + 0.2329*out2_2);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4751*out1_1 + 0.2995*out2_1 + 0.2254*out2_2);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4390*out1_1 + 0.4773*out2_1 + 0.0836*out2_2);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3456*out1_1 + 0.9205*out2_1 - 0.2661*out2_2);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1673*out1_1 + 1.3452*out2_1 - 0.5125*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 01402:           /* ##......#., <noise^2> = 0 */
            val = FLT2PIX(-0.5125*out1_2 + 1.3452*out1_1 + 0.1673*out2_1);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.2666*out1_2 + 0.9201*out1_1 + 0.3465*out2_1);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0793*out1_2 + 0.4730*out1_1 + 0.4477*out2_1);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2024*out1_2 + 0.2765*out1_1 + 0.5212*out2_1);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1635*out1_2 + 0.1716*out1_1 + 0.6649*out2_1);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0583*out1_2 + 0.0588*out1_1 + 0.8829*out2_1);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 01403:		/* ##......##, <noise^2> = 0 */
            val = FLT2PIX(-0.5177*out1_2 + 1.3400*out1_1 +
						0.0891*out2_1 + 0.0886*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.2771*out1_2 + 0.9095*out1_1 + 0.1878*out2_1 + 0.1797*out2_2);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0677*out1_2 + 0.4614*out1_1 +
						0.2725*out2_1 + 0.1984*out2_2);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1984*out1_2 + 0.2725*out1_1 +
						0.4614*out2_1 + 0.0677*out2_2);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1797*out1_2 + 0.1878*out1_1 +
						0.9095*out2_1 - 0.2771*out2_2);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0886*out1_2 + 0.0891*out1_1 +
						1.3400*out2_1 - 0.5177*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 02002:           /* #........#., <noise^2> = 0, sigma = 1 */
            val = FLT2PIX(0.8894*out1_1 + 0.1106*out2_1);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6839*out1_1 + 0.3161*out2_1);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5526*out1_1 + 0.4474*out2_1);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5082*out1_1 + 0.4918*out2_1);
            out[bad_x1 + 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4918*out1_1 + 0.5082*out2_1);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4474*out1_1 + 0.5526*out2_1);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3161*out1_1 + 0.6839*out2_1);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1106*out1_1 + 0.8894*out2_1);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 02003:           /* #........##, <noise^2> = 0 */
            val = FLT2PIX(0.8829*out1_1 + 0.0585*out2_1 + 0.0585*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6654*out1_1 + 0.1673*out2_1 + 0.1673*out2_2);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5265*out1_1 + 0.2370*out2_1 + 0.2365*out2_2);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4799*out1_1 + 0.2641*out2_1 + 0.2560*out2_2);
            out[bad_x1 + 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4664*out1_1 + 0.3038*out2_1 + 0.2298*out2_2);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4381*out1_1 + 0.4778*out2_1 + 0.0841*out2_2);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3455*out1_1 + 0.9206*out2_1 - 0.2661*out2_2);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1673*out1_1 + 1.3452*out2_1 - 0.5125*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 03002:           /* ##.......#., <noise^2> = 0 */
            val = FLT2PIX(-0.5125*out1_2 + 1.3452*out1_1 + 0.1673*out2_1);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.2661*out1_2 + 0.9205*out1_1 + 0.3456*out2_1);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0836*out1_2 + 0.4773*out1_1 + 0.4390*out2_1);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2254*out1_2 + 0.2995*out1_1 + 0.4751*out2_1);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2329*out1_2 + 0.2411*out1_1 + 0.5260*out2_1);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1670*out1_2 + 0.1676*out1_1 + 0.6654*out2_1);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0585*out1_2 + 0.0585*out1_1 + 0.8829*out2_1);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 03003:		/* ##.......##, <noise^2> = 0 */
            val = FLT2PIX(-0.5177*out1_2 + 1.3400*out1_1 +
						0.0889*out2_1 + 0.0888*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.2768*out1_2 + 0.9098*out1_1 +
						0.1838*out2_1 + 0.1832*out2_2);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0703*out1_2 + 0.4639*out1_1 +
						0.2370*out2_1 + 0.2288*out2_2);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2130*out1_2 + 0.2870*out1_1 +
						0.2870*out2_1 + 0.2130*out2_2);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2288*out1_2 + 0.2370*out1_1 +
						0.4639*out2_1 + 0.0703*out2_2);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1832*out1_2 + 0.1838*out1_1 +
						0.9098*out2_1 - 0.2768*out2_2);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0888*out1_2 + 0.0889*out1_1 +
						1.3400*out2_1 - 0.5177*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 04002:           /* #.........#., <noise^2> = 0 */
            val = FLT2PIX(0.8894*out1_1 + 0.1106*out2_1);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6839*out1_1 + 0.3161*out2_1);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5527*out1_1 + 0.4473*out2_1);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5091*out1_1 + 0.4909*out2_1);
            out[bad_x1 + 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5000*out1_1 + 0.5000*out2_1);
            out[bad_x2 - 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4909*out1_1 + 0.5091*out2_1);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4473*out1_1 + 0.5527*out2_1);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3161*out1_1 + 0.6839*out2_1);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1106*out1_1 + 0.8894*out2_1);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 04003:           /* #.........##, <noise^2> = 0 */
            val = FLT2PIX(0.8829*out1_1 + 0.0585*out2_1 + 0.0585*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6654*out1_1 + 0.1673*out2_1 + 0.1673*out2_2);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5265*out1_1 + 0.2368*out2_1 + 0.2367*out2_2);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4804*out1_1 + 0.2601*out2_1 + 0.2595*out2_2);
            out[bad_x1 + 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4712*out1_1 + 0.2685*out2_1 + 0.2603*out2_2);
            out[bad_x2 - 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4654*out1_1 + 0.3043*out2_1 + 0.2302*out2_2);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4380*out1_1 + 0.4778*out2_1 + 0.0842*out2_2);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3455*out1_1 + 0.9206*out2_1 - 0.2661*out2_2);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1673*out1_1 + 1.3452*out2_1 - 0.5125*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 06002:           /* ##........#., <noise^2> = 0 */
            val = FLT2PIX(-0.5125*out1_2 + 1.3452*out1_1 + 0.1673*out2_1);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.2661*out1_2 + 0.9206*out1_1 + 0.3455*out2_1);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0841*out1_2 + 0.4778*out1_1 + 0.4381*out2_1);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2298*out1_2 + 0.3038*out1_1 + 0.4664*out2_1);
            out[bad_x1 + 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2560*out1_2 + 0.2641*out1_1 + 0.4799*out2_1);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2365*out1_2 + 0.2370*out1_1 + 0.5265*out2_1);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1673*out1_2 + 0.1673*out1_1 + 0.6654*out2_1);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0585*out1_2 + 0.0585*out1_1 + 0.8829*out2_1);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 06003:		/* ##........##, <noise^2> = 0 */
            val = FLT2PIX(-0.5177*out1_2 + 1.3400*out1_1 +
						0.0888*out2_1 + 0.0888*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.2768*out1_2 + 0.9098*out1_1 +
						0.1835*out2_1 + 0.1835*out2_2);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0705*out1_2 + 0.4642*out1_1 +
						0.2329*out2_1 + 0.2324*out2_2);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2155*out1_2 + 0.2896*out1_1 +
						0.2515*out2_1 + 0.2434*out2_2);
            out[bad_x1 + 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2434*out1_2 + 0.2515*out1_1 +
						0.2896*out2_1 + 0.2155*out2_2);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2324*out1_2 + 0.2329*out1_1 +
						0.4642*out2_1 + 0.0705*out2_2);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1835*out1_2 + 0.1835*out1_1 +
						0.9098*out2_1 - 0.2768*out2_2);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0888*out1_2 + 0.0888*out1_1 +
						1.3400*out2_1 - 0.5177*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 010002:          /* #..........#., <noise^2> = 0, sigma = 1 */
            val = FLT2PIX(0.8894*out1_1 + 0.1106*out2_1);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6839*out1_1 + 0.3161*out2_1);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5527*out1_1 + 0.4473*out2_1);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5092*out1_1 + 0.4908*out2_1);
            out[bad_x1 + 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5009*out1_1 + 0.4991*out2_1);
            out[bad_x1 + 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4991*out1_1 + 0.5009*out2_1);
            out[bad_x2 - 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4908*out1_1 + 0.5092*out2_1);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4473*out1_1 + 0.5527*out2_1);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3161*out1_1 + 0.6839*out2_1);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1106*out1_1 + 0.8894*out2_1);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)): 
                                         (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 010003:          /* #..........##, <noise^2> = 0 */
            val = FLT2PIX(0.8829*out1_1 + 0.0585*out2_1 + 0.0585*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6654*out1_1 + 0.1673*out2_1 + 0.1673*out2_2);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5265*out1_1 + 0.2367*out2_1 + 0.2367*out2_2);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4804*out1_1 + 0.2598*out2_1 + 0.2598*out2_2);
            out[bad_x1 + 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4717*out1_1 + 0.2644*out2_1 + 0.2639*out2_2);
            out[bad_x1 + 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4703*out1_1 + 0.2690*out2_1 + 0.2608*out2_2);
            out[bad_x2 - 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4654*out1_1 + 0.3043*out2_1 + 0.2303*out2_2);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4380*out1_1 + 0.4778*out2_1 + 0.0842*out2_2);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3455*out1_1 + 0.9206*out2_1 - 0.2661*out2_2);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1673*out1_1 + 1.3452*out2_1 - 0.5125*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 014002:          /* ##.........#., <noise^2> = 0 */
            val = FLT2PIX(-0.5125*out1_2 + 1.3452*out1_1 + 0.1673*out2_1);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.2661*out1_2 + 0.9206*out1_1 + 0.3455*out2_1);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0842*out1_2 + 0.4778*out1_1 + 0.4380*out2_1);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2302*out1_2 + 0.3043*out1_1 + 0.4654*out2_1);
            out[bad_x1 + 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2603*out1_2 + 0.2685*out1_1 + 0.4712*out2_1);
            out[bad_x2 - 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2595*out1_2 + 0.2601*out1_1 + 0.4804*out2_1);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2367*out1_2 + 0.2368*out1_1 + 0.5265*out2_1);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1673*out1_2 + 0.1673*out1_1 + 0.6654*out2_1);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0585*out1_2 + 0.0585*out1_1 + 0.8829*out2_1);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
	  case 014003:          /* ##.........##, <noise^2> = 0 */
            val = FLT2PIX(-0.5177*out1_2 + 1.3400*out1_1 +
						0.0888*out2_1 + 0.0888*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.2768*out1_2 + 0.9098*out1_1 +
						0.1835*out2_1 + 0.1835*out2_2);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0705*out1_2 + 0.4642*out1_1 +
						0.2326*out2_1 + 0.2326*out2_2);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2158*out1_2 + 0.2899*out1_1 +
						0.2474*out2_1 + 0.2469*out2_2);
            out[bad_x1 + 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2459*out1_2 + 0.2541*out1_1 +
						0.2541*out2_1 + 0.2459*out2_2);
            out[bad_x2 - 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2469*out1_2 + 0.2474*out1_1 +
						0.2899*out2_1 + 0.2158*out2_2);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2326*out1_2 + 0.2326*out1_1 +
						0.4642*out2_1 + 0.0705*out2_2);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1835*out1_2 + 0.1835*out1_1 +
						0.9098*out2_1 - 0.2768*out2_2);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0888*out1_2 + 0.0888*out1_1 +
						1.3400*out2_1 - 0.5177*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 020003:          /* #...........##, <noise^2> = 0 */
            val = FLT2PIX(0.8829*out1_1 + 0.0585*out2_1 + 0.0585*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6654*out1_1 + 0.1673*out2_1 + 0.1673*out2_2);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5265*out1_1 + 0.2367*out2_1 + 0.2367*out2_2);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4804*out1_1 + 0.2598*out2_1 + 0.2598*out2_2);
            out[bad_x1 + 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4718*out1_1 + 0.2641*out2_1 + 0.2641*out2_2);
            out[bad_x1 + 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4708*out1_1 + 0.2649*out2_1 + 0.2644*out2_2);
            out[bad_x2 - 5] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4702*out1_1 + 0.2690*out2_1 + 0.2608*out2_2);
            out[bad_x2 - 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4654*out1_1 + 0.3044*out2_1 + 0.2303*out2_2);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4380*out1_1 + 0.4778*out2_1 + 0.0842*out2_2);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3455*out1_1 + 0.9206*out2_1 - 0.2661*out2_2);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1673*out1_1 + 1.3452*out2_1 - 0.5125*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 030002:          /* ##..........#., <noise^2> = 0 */
            val = FLT2PIX(-0.5125*out1_2 + 1.3452*out1_1 + 0.1673*out2_1);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.2661*out1_2 + 0.9206*out1_1 + 0.3455*out2_1);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0842*out1_2 + 0.4778*out1_1 + 0.4380*out2_1);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2303*out1_2 + 0.3043*out1_1 + 0.4654*out2_1);
            out[bad_x1 + 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2608*out1_2 + 0.2690*out1_1 + 0.4703*out2_1);
            out[bad_x1 + 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2639*out1_2 + 0.2644*out1_1 + 0.4717*out2_1);
            out[bad_x2 - 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2598*out1_2 + 0.2598*out1_1 + 0.4804*out2_1);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2367*out1_2 + 0.2367*out1_1 + 0.5265*out2_1);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1673*out1_2 + 0.1673*out1_1 + 0.6654*out2_1);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0585*out1_2 + 0.0585*out1_1 + 0.8829*out2_1);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 030003:          /* ##..........##, <noise^2> = 0 */
            val = FLT2PIX(-0.5177*out1_2 + 1.3400*out1_1 +
						0.0888*out2_1 + 0.0888*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.2768*out1_2 + 0.9098*out1_1 +
						0.1835*out2_1 + 0.1835*out2_2);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0705*out1_2 + 0.4642*out1_1 +
						0.2326*out2_1 + 0.2326*out2_2);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2158*out1_2 + 0.2899*out1_1 +
						0.2472*out2_1 + 0.2471*out2_2);
            out[bad_x1 + 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2462*out1_2 + 0.2544*out1_1 +
						0.2500*out2_1 + 0.2495*out2_2);
            out[bad_x1 + 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2495*out1_2 + 0.2500*out1_1 +
						0.2544*out2_1 + 0.2462*out2_2);
            out[bad_x2 - 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2471*out1_2 + 0.2472*out1_1 +
						0.2899*out2_1 + 0.2158*out2_2);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2326*out1_2 + 0.2326*out1_1 +
						0.4642*out2_1 + 0.0705*out2_2);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1835*out1_2 + 0.1835*out1_1 +
						0.9098*out2_1 - 0.2768*out2_2);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0888*out1_2 + 0.0888*out1_1 +
						1.3400*out2_1 - 0.5177*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
          case 040003:          /* #............##, <noise^2> = 0 */
            val = FLT2PIX(0.8829*out1_1 + 0.0585*out2_1 + 0.0585*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6654*out1_1 + 0.1673*out2_1 + 0.1673*out2_2);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5265*out1_1 + 0.2367*out2_1 + 0.2367*out2_2);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4804*out1_1 + 0.2598*out2_1 + 0.2598*out2_2);
            out[bad_x1 + 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4718*out1_1 + 0.2641*out2_1 + 0.2641*out2_2);
            out[bad_x1 + 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4708*out1_1 + 0.2646*out2_1 + 0.2646*out2_2);
            out[bad_x1 + 5] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4707*out1_1 + 0.2649*out2_1 + 0.2644*out2_2);
            out[bad_x2 - 5] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4702*out1_1 + 0.2690*out2_1 + 0.2608*out2_2);
            out[bad_x2 - 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4654*out1_1 + 0.3044*out2_1 + 0.2303*out2_2);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4380*out1_1 + 0.4778*out2_1 + 0.0842*out2_2);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3455*out1_1 + 0.9206*out2_1 - 0.2661*out2_2);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1673*out1_1 + 1.3452*out2_1 - 0.5125*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
	  case 060002:          /* ##...........#., <noise^2> = 0 */
            val = FLT2PIX(-0.5125*out1_2 + 1.3452*out1_1 + 0.1673*out2_1);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.2661*out1_2 + 0.9206*out1_1 + 0.3455*out2_1);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0842*out1_2 + 0.4778*out1_1 + 0.4380*out2_1);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2303*out1_2 + 0.3044*out1_1 + 0.4654*out2_1);
            out[bad_x1 + 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2608*out1_2 + 0.2690*out1_1 + 0.4702*out2_1);
            out[bad_x1 + 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2644*out1_2 + 0.2649*out1_1 + 0.4708*out2_1);
            out[bad_x2 - 5] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2641*out1_2 + 0.2641*out1_1 + 0.4718*out2_1);
            out[bad_x2 - 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2598*out1_2 + 0.2598*out1_1 + 0.4804*out2_1);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2367*out1_2 + 0.2367*out1_1 + 0.5265*out2_1);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1673*out1_2 + 0.1673*out1_1 + 0.6654*out2_1);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0585*out1_2 + 0.0585*out1_1 + 0.8829*out2_1);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
	  default:
	    shFatal("Unsupported defect type: MIDDLE 0%o",badcol[i].type);
	    break;			/* NOTREACHED */
	 }
	 break;
       case BAD_WIDE:
       case BAD_WIDE_NEAR_LEFT:
       case BAD_WIDE_NEAR_RIGHT:
	 if(badcol[i].pos == BAD_WIDE_NEAR_LEFT) {
	    shAssert(bad_x1 >= 1);

	    if(bad_x2 + 2 >= ncol) {	/* left defect extends near
					   right edge of data! */
	       if(bad_x2 == ncol - 2) {	/* one column remains */
		  val = FLT2PIX(out[ncol - 1]);
	       } else {
		  val = FLT2PIX(MAX_U16);		/* there is no information */
	       }
	       for(j = bad_x1;j <= bad_x2;j++) {
		  out[j] = val;
	       }
	       break;
	    }
	    out1_2 = -1;		/* NOTUSED */
	    out2_2 = out[bad_x2 + 2];
	 } else if(badcol[i].pos == BAD_WIDE) {
	    shAssert(bad_x1 >= 2 && bad_x2 + 2 < ncol);
	    out1_2 = out[bad_x1 - 2];
	    out2_2 = out[bad_x2 + 2];
	 } else if(badcol[i].pos == BAD_WIDE_NEAR_RIGHT) {
	    shAssert(bad_x2 + 1 < ncol);

	    if(bad_x1 < 2) {		/* right defect extends near
					   left edge of data! */
	       if(bad_x1 == 1) {	/* one column remains */
		  val = FLT2PIX(out[0]);
	       } else {
		  val = FLT2PIX(MAX_U16);	/* there is no information */
	       }
	       for(j = bad_x1;j <= bad_x2;j++) {
		  out[j] = val;
	       }
	       break;
	    }
	    out1_2 = out[bad_x1 - 2];
	    out2_2 = -1;		/* NOTUSED */
	 } else {
	    shFatal("Unknown defect classification %d (%s:%d)",badcol[i].pos,
		    __FILE__,__LINE__);
	    out1_2 = out2_2 = -1;	/* NOTUSED */
	 }

	 out1_1 = out[bad_x1 - 1];
	 out2_1 = out[bad_x2 + 1];

	 switch (badcol[i].type) {

          case 06:			/* #?#., <noise^2> = 0 */
            val = FLT2PIX(0.8894*out1_1 + 0.1106*out2_1);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6839*out1_1 + 0.3161*out2_1);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5527*out1_1 + 0.4473*out2_1);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5092*out1_1 + 0.4908*out2_1);
            out[bad_x1 + 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5010*out1_1 + 0.4990*out2_1);
            out[bad_x1 + 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5001*out1_1 + 0.4999*out2_1);
            out[bad_x1 + 5] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);


            val = FLT2PIX(0.5000*out1_1 + 0.5000*out2_1);
	    val = val > 0 ? (val > MAX_U16 ? MAX_U16 : val) : 0 + 0.5;

	    for(j = bad_x1 + 6;j < bad_x2 - 5;j++) {
	       out[j] = val;
	    }

            val = FLT2PIX(0.4999*out1_1 + 0.5001*out2_1);
            out[bad_x2 - 5] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4990*out1_1 + 0.5010*out2_1);
            out[bad_x2 - 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4908*out1_1 + 0.5092*out2_1);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4473*out1_1 + 0.5527*out2_1);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3161*out1_1 + 0.6839*out2_1);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1106*out1_1 + 0.8894*out2_1);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
	  case 07:           /* #?##, <noise^2> = 0 */
            val = FLT2PIX(0.8829*out1_1 + 0.0585*out2_1 + 0.0585*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.6654*out1_1 + 0.1673*out2_1 + 0.1673*out2_2);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.5265*out1_1 + 0.2367*out2_1 + 0.2367*out2_2);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4804*out1_1 + 0.2598*out2_1 + 0.2598*out2_2);
            out[bad_x1 + 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4718*out1_1 + 0.2641*out2_1 + 0.2641*out2_2);
            out[bad_x1 + 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4708*out1_1 + 0.2646*out2_1 + 0.2646*out2_2);
            out[bad_x1 + 5] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

	    val = FLT2PIX(0.4707*out[bad_x1 - 1] +
			      0.2646*out[bad_x2 + 1] + 0.2646*out[bad_x2 + 2]);
	    val = val > 0 ? (val > MAX_U16 ? MAX_U16 : val) : 0 + 0.5;

	    for(j = bad_x1 + 6;j < bad_x2 - 5;j++) {
	       out[j] = val;
	    }

            val = FLT2PIX(0.4707*out1_1 + 0.2649*out2_1 + 0.2644*out2_2);
            out[bad_x2 - 5] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4702*out1_1 + 0.2690*out2_1 + 0.2608*out2_2);
            out[bad_x2 - 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4654*out1_1 + 0.3044*out2_1 + 0.2303*out2_2);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.4380*out1_1 + 0.4778*out2_1 + 0.0842*out2_2);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.3455*out1_1 + 0.9206*out2_1 - 0.2661*out2_2);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1673*out1_1 + 1.3452*out2_1 - 0.5125*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);
	    
	    break;
          case 016:         /* ##?#., <noise^2> = 0 */
            val = FLT2PIX(-0.5125*out1_2 + 1.3452*out1_1 + 0.1673*out2_1);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.2661*out1_2 + 0.9206*out1_1 + 0.3455*out2_1);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0842*out1_2 + 0.4778*out1_1 + 0.4380*out2_1);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2303*out1_2 + 0.3044*out1_1 + 0.4654*out2_1);
            out[bad_x1 + 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2608*out1_2 + 0.2690*out1_1 + 0.4702*out2_1);
            out[bad_x1 + 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2644*out1_2 + 0.2649*out1_1 + 0.4707*out2_1);
            out[bad_x1 + 5] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2646*out1_2 + 0.2646*out1_1 + 0.4707*out2_1);
            val = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);
	    
	    for(j = bad_x1 + 6;j < bad_x2 - 5;j++) {
	       out[j] = val;
	    }

            val = FLT2PIX(0.2646*out1_2 + 0.2646*out1_1 + 0.4708*out2_1);
            out[bad_x2 - 5] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2641*out1_2 + 0.2641*out1_1 + 0.4718*out2_1);
            out[bad_x2 - 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2598*out1_2 + 0.2598*out1_1 + 0.4804*out2_1);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2367*out1_2 + 0.2367*out1_1 + 0.5265*out2_1);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1673*out1_2 + 0.1673*out1_1 + 0.6654*out2_1);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0585*out1_2 + 0.0585*out1_1 + 0.8829*out2_1);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
	  case 017:			/* ##?##, S/N = infty */
            val = FLT2PIX(-0.5177*out1_2 + 1.3400*out1_1 +
						0.0888*out2_1 + 0.0888*out2_2);
            out[bad_x1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(-0.2768*out1_2 + 0.9098*out1_1 +
						0.1835*out2_1 + 0.1835*out2_2);
            out[bad_x1 + 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0705*out1_2 + 0.4642*out1_1 +
						0.2326*out2_1 + 0.2326*out2_2);
            out[bad_x1 + 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2158*out1_2 + 0.2899*out1_1 +
						0.2472*out2_1 + 0.2472*out2_2);
            out[bad_x1 + 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2462*out1_2 + 0.2544*out1_1 +
						0.2497*out2_1 + 0.2497*out2_2);
            out[bad_x1 + 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2497*out1_2 + 0.2503*out1_1 +
						0.2500*out2_1 + 0.2500*out2_2);
            out[bad_x1 + 5] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2500*out1_2 + 0.2500*out1_1 +
						0.2500*out2_1 + 0.2500*out2_2);
            val = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);
	    
	    for(j = bad_x1 + 6;j < bad_x2 - 5;j++) {
	       out[j] = val;
	    }

            val = FLT2PIX(0.2500*out1_2 + 0.2500*out1_1 +
						0.2503*out2_1 + 0.2497*out2_2);
            out[bad_x2 - 5] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2497*out1_2 + 0.2497*out1_1 +
						0.2544*out2_1 + 0.2462*out2_2);
            out[bad_x2 - 4] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2472*out1_2 + 0.2472*out1_1 +
						0.2899*out2_1 + 0.2158*out2_2);
            out[bad_x2 - 3] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.2326*out1_2 + 0.2326*out1_1 +
						0.4642*out2_1 + 0.0705*out2_2);
            out[bad_x2 - 2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.1835*out1_2 + 0.1835*out1_1 +
						0.9098*out2_1 - 0.2768*out2_2);
            out[bad_x2 - 1] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            val = FLT2PIX(0.0888*out1_2 + 0.0888*out1_1 +
						1.3400*out2_1 - 0.5177*out2_2);
            out[bad_x2] = (val < min) ? FLT2PIX(0.5*(out1_1 + out2_1)) :
					       (val > MAX_U16 ? MAX_U16 : val);

            break;
	  default:
	    shFatal("Unsupported defect type: WIDE 0%o",badcol[i].type);
	    break;			/* NOTREACHED */
	 }
	 break;
      }
   }
}

/*****************************************************************************/
/*
 * Interpolate _up_ a column to reduce the artifacts due to the initial
 * row-by-row interpolation
 */
#if defined(INTERPOLATE_VERTICAL)
static void
interpolate_vertical(REGION *reg,	/* data to correct */
		     int r1, int  r2,	/* rows to fix, inclusive */
		     int c,		/* desired column */
		     int min)		/* minimum acceptable value */
{
   const float alpha = 0.75;		/* amount of original value to keep */
   PIX *col = alloca(reg->nrow*sizeof(PIX)); /* temporary space for a column */
   int i;
   PIX *const *const reg_rows = reg->ROWS; /* == reg->ROWS */
   float val;				/* an interpolated value */

   shAssert(c >= 0 && c <= reg->ncol - 1);
   shAssert(r1 >= 2 && r2 <= reg->nrow - 3);
/*
 * unpack data into col for cache efficiency, and to allow us not to have to
 * keep temporaries while we fix things in place
 */
   for(i = r1 - 2; i < r2 + 2; i++) {
      col[i] = reg_rows[i][c];
   }
/*
 * do the work
 */
   for(i = r1; i < r2; i++) {
      val = alpha*col[i] +
	FLT2PIX((1 - alpha)*(INTERP_1_C2*(col[i - 2] + col[i + 2]) +
			     INTERP_1_C1*(col[i - 1] + col[i + 1])));

      reg_rows[i][c] = (val < min) ? FLT2PIX(0.5*(col[i - 1] + col[i + 1])) :
					       (val > MAX_U16 ? MAX_U16 : val);
   }
}
#endif

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Return the interpolated value for a pixel, ignoring pixels given
 * by badmask. Interolation can either be vertical or horizontal
 *
 * Note that this is a pretty expensive routine, so use only after
 * suitable thought.
 */
int
phPixelInterpolate(int rowc, int colc,	/* the pixel in question */
		   const REGION *reg,	/* in this region */
		   const CHAIN *badmask, /* pixels to ignore */
		   int horizontal,	/* interpolate horizontally */
		   int minval)		/* minimum acceptable value */
{
   BADCOLUMN badcol;			/* describe a bad column */
   PIX *data;				/* temp array to interpolate in */
   int i;
   int i0, i1;				/* data corresponds to range of
					   {row,col} == [i0,i1] */
   int ndata;				/* dimension of data */
   static int ndatamax = 40;		/* largest allowable defect. XXX */
   int nrow, ncol;			/* == reg->n{row,col} */
   PIX *val;				/* pointer to pixel (rowc, colc) */
   int z1, z2;				/* range of bad {row,columns} */

   shAssert(badmask != NULL && badmask->type == shTypeGetFromName("OBJMASK"));
   shAssert(reg != NULL && reg->type == TYPE_PIX);
   nrow = reg->nrow;
   ncol = reg->ncol;

   if(horizontal) {
      for(z1 = colc - 1; z1 >= 0; z1--) {
	 if(!phPixIntersectMask(badmask, z1, rowc)) {
	    break;
	 }
      }
      z1++;
      
      for(z2 = colc + 1; z2 < ncol; z2++) {
	 if(!phPixIntersectMask(badmask, z2, rowc)) {
	    break;
	 }
      }
      z2--;
      
      i0 = (z1 > 2) ? z1 - 2 : 0;	/* origin of available required data */
      i1 = (z2 < ncol - 2) ? z2 + 2 : ncol - 1;	/* end of "      "   "    "  */

      if(i0 < 2 || i1 >= ncol - 2) {	/* interpolation will fail */
	 return(-1);			/* failure */
      }

      ndata = (i1 - i0 + 1);
      if(ndata > ndatamax) {
	 return(-1);			/* failure */
      }
      
      data = alloca(ndata*sizeof(PIX));
      for(i = i0; i <= i1; i++) {
	 data[i - i0] = reg->ROWS[rowc][i];
      }
      val = &data[colc - i0];
   } else {
      for(z1 = rowc - 1; z1 >= 0; z1--) {
	 if(!phPixIntersectMask(badmask, colc, z1)) {
	    break;
	 }
      }
      z1++;
      
      for(z2 = rowc + 1; z2 < nrow; z2++) {
	 if(!phPixIntersectMask(badmask, colc, z2)) {
	    break;
	 }
      }
      z2--;
      
      i0 = (z1 > 2) ? z1 - 2 : 0;	/* origin of available required data */
      i1 = (z2 < nrow - 2) ? z2 + 2 : nrow - 1;	/* end of "      "   "    "  */

      if(i0 < 2 || i1 >= ncol - 2) {	/* interpolation will fail */
	 return(-1);			/* failure */
      }

      ndata = (i1 - i0 + 1);
      if(ndata > ndatamax) {
	 return(-1);			/* failure */
      }
      
      data = alloca(ndata*sizeof(PIX));
      for(i = i0; i <= i1; i++) {
	 data[i - i0] = reg->ROWS[i][colc];
      }
      val = &data[rowc - i0];
   }
      
   badcol.x1 = z1 - i0;
   badcol.x2 = z2 - i0;
   classify_badcol(&badcol, 1, ndata);
   do_badcol(&badcol, 1, data, ndata, minval);

   return(*val);
}
