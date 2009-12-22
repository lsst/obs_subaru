/* routines to fit profiles to objects */

#include <math.h>
#include <string.h>
#include <alloca.h>
#include <unistd.h>
#include "dervish.h"
#include "phObjc.h"
#include "phObjects.h"
#include "phExtract.h"
#include "phDgpsf.h"
#include "phFitobj.h"
#include "phCellFitobj.h"
#include "phSkyUtils.h"
#include "phMathUtils.h"
#include "phUtils.h"
#include "phObjectCenter.h"

#define SQRT3 1.732050808		/* sqrt(3) */

/* header of model catalogue file. */
static struct spro_cheader_T pro_cheader;
static spro_catentry *dev_entries = NULL; /* DeVaucouleurs models. */
static spro_catentry *exp_entries = NULL; /* Exponential models. */

#define NAMELEN 100
static char catalog_file[NAMELEN] = "";	/* Name of catalogue file. */

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * Initialize data structures for the fitting routines.  For now this
 * means initializing the catalogue data structures.
 */
int
phInitFitobj(char *file)
{
    FILE *cat_file;
    int nitem;

    if((cat_file = fopen(file, "rb")) == NULL) {
       shErrStackPushPerror("phInitFitobj: "
			    "Cannot open catalogue file %s for read\n",file);
       return(SH_GENERIC_ERROR);
    }
/*
 * Read in header, check it, then allocate space for spro_cheader.proc_catentry,
 * setup pointers, and read data
 */
    nitem = fread(&pro_cheader, sizeof(pro_cheader), 1, cat_file);
    shAssert(nitem == 1);

    if(strncmp(pro_cheader.proc_catver,RAWPROF_VERSION, strlen(RAWPROF_VERSION)) != 0) {
	pro_cheader.proc_catver[IDSIZE - 1] = '\0';	/* may be binary... */
	fprintf(stderr,"Raw header is corrupted or out of date. "
		"Expected\n\t%s\n"
		"Saw\n\t%s\n",RAWPROF_VERSION, pro_cheader.proc_catver);
	fclose(cat_file); cat_file = NULL;
	pro_cheader.proc_catentry = NULL;

	return(SH_GENERIC_ERROR);
    }

    nitem = p_phReadCatentries(&pro_cheader, cat_file);
    shAssert(nitem == MAXSCAT);
/*
 * Set up extra pointers.
 */
    exp_entries = pro_cheader.proc_catentry[1];
    dev_entries = pro_cheader.proc_catentry[1 +
			   pro_cheader.proc_nexpincl*pro_cheader.proc_nexpsiz];
    fclose(cat_file);

    strncpy(catalog_file,file,NAMELEN);	/* save name of current catalog file */
/*
 * Setup a numbers for dithering
 */
    if(!phRandomIsInitialised()) {
       (void)phRandomNew("100000:3", 0);       
    }
    
    return(SH_SUCCESS);
}

void
phFiniFitobj(void)
{
   exp_entries = dev_entries = NULL;

   catalog_file[0] = '\0';

   if(pro_cheader.proc_catentry != NULL) {
      shFree(pro_cheader.proc_catentry[0]);
      shFree(pro_cheader.proc_catentry);
   }

   memset(&pro_cheader, '\0', sizeof(pro_cheader));
}


/*****************************************************************************/
/*
 * Read the catentries part of a raw profile file header.
 */
int
p_phReadCatentries(spro_cheader *header, /* raw header to set */
		   FILE *fd)		/* file to read from */
{
   int i;
   int nitem;
   
   header->proc_catentry = shMalloc(MAXSCAT*sizeof(spro_catentry *));
   header->proc_catentry[0] = shMalloc(MAXSCAT*sizeof(spro_catentry));
   
   for(i = 0; i < MAXSCAT; i++) {
      header->proc_catentry[i] = header->proc_catentry[0] + i;
   }

   nitem = fread(header->proc_catentry[0], sizeof(spro_catentry), MAXSCAT, fd);

   return(nitem);
}

/*****************************************************************************/
/*
 * Given an effective radius, return a size index.
 * Assumes that the catalogue is ordered in decreasing size.
 */
static int
dev_re_to_isize(
    double re
)
{
    int i;

    for(i = 0; i < pro_cheader.proc_ndevsiz; i++) {
	if(re > dev_entries[i].scat_reff)
	    break;
    }
    if(i == pro_cheader.proc_ndevsiz)
	return i-1;
    if(i == 0)
	return i;
    /* Find closest */
    if(re - dev_entries[i].scat_reff
       < dev_entries[(i-1)].scat_reff - re)
	return i;
    return i-1;
}

/*
 * Given an axis ratio, return an inclination index.
 * Assumes that the catalogue is ordered in decreasing axis ratio.
 */
static int
dev_aratio_to_iinc(
    double aratio
)
{
    int i;
    int stride;

    stride = pro_cheader.proc_ndevsiz;
    for(i = 0; i < pro_cheader.proc_ndevincl; i++) {
	if(aratio > dev_entries[i*stride].scat_axr)
	    break;
    }
    if(i == pro_cheader.proc_ndevincl)
	return i-1;
    if(i == 0)
	return i;
    /* Find closest */
    if(aratio - dev_entries[i*stride].scat_axr
       < dev_entries[(i-1)*stride].scat_axr - aratio)
	return i;
    return i-1;
}

static int
dev_param_to_index(
    const MODEL_PARAMS *p
)
{
    int isize;
    int iinc;

    shAssert(dev_entries != NULL);

    isize = dev_re_to_isize(p->rsize);
    iinc = dev_aratio_to_iinc(p->aratio);
    return 1 + pro_cheader.proc_nexpsiz*pro_cheader.proc_nexpincl
	+ iinc*pro_cheader.proc_ndevsiz + isize;
}

/*
 * Given an effective radius, return a size index.
 * Assumes that the catalogue is ordered in decreasing axis ratio.
 */
static int
exp_re_to_isize(
    double re
)
{
    int i;

    for(i = 0; i < pro_cheader.proc_nexpsiz; i++) {
	if(re > exp_entries[i].scat_reff)
	    break;
    }
    if(i == pro_cheader.proc_nexpsiz)
	return i-1;
    if(i == 0)
	return i;
    /* Find closest */
    if(re - exp_entries[i].scat_reff
       < exp_entries[(i-1)].scat_reff - re)
	return i;
    return i-1;
}

/*
 * Given an axis ratio, return an inclination index.
 * Assumes that the catalogue is ordered in decreasing axis ratio.
 */
static int
exp_aratio_to_iinc(
    double aratio
)
{
    int i;
    int stride;

    stride = pro_cheader.proc_nexpsiz;
    for(i = 0; i < pro_cheader.proc_nexpincl; i++) {
	if(aratio > exp_entries[i*stride].scat_axr)
	    break;
    }
    if(i == pro_cheader.proc_nexpincl)
	return i-1;
    if(i == 0)
	return i;
    /* Find closest */
    if(aratio - exp_entries[i*stride].scat_axr
       < exp_entries[(i-1)*stride].scat_axr - aratio)
	return i;
    return i-1;
}

static int
exp_param_to_index(
    const MODEL_PARAMS *p
)
{
    int isize;
    int iinc;

    shAssert(exp_entries != NULL);

    isize = exp_re_to_isize(p->rsize);
    iinc = exp_aratio_to_iinc(p->aratio);
    return 1 + iinc*pro_cheader.proc_nexpsiz + isize;
}

/*
 * Read a model from the catalogue file into a region with space for
 * smoothing.  The origin of the model is therefore at [smmax, smmax].
 */

static REGION *
read_model_index(int index,
		 int expand_model)		/* fill in bottom half of region? */
{
    FILE *cat_file;
    REGION *qreg;		/* Upper right hand quadrant of image */
    int nitem;
    int xsize;
    int ysize;
    int smmax;
    int i;
    int ir;
    MODEL_PIX *rp;
    MODEL_PIX *rmp;

    if(catalog_file[0] == '\0') {
       shError("read_model: no catalog file is declared");
       return(NULL);
    }

    cat_file = fopen(catalog_file, "rb");
    shAssert(cat_file != NULL);
/*
 * Seek to appropriate spot in file.
 */
    if(fseek(cat_file, pro_cheader.proc_catentry[index]->scat_offset, 0) ==-1){
       shError("Failed to seek to model");
    }
/*
 * Allocate quadrant region.
 */
    xsize = pro_cheader.proc_catentry[index]->scat_xsize;
    ysize = pro_cheader.proc_catentry[index]->scat_ysize;
    smmax = pro_cheader.proc_smmax;

    if(expand_model) {
	qreg = shRegNew("quadrant", ysize + 2*smmax + 50, xsize + 50, TYPE_MODEL);
    } else {
	qreg = shRegNew("quadrant", ysize, xsize - smmax, TYPE_MODEL);
    }
    shRegClear(qreg);
/*
 * Read it in, leaving space to smooth.  Origin is at [smmax, smmax].
 */
    for(i = 0; i < ysize; i++) {
	if (expand_model) {
	    nitem = fread(qreg->rows_model[i + smmax],
			  sizeof(qreg->rows_model[0][0]), xsize, cat_file);
	} else {
	    if(fseek(cat_file, smmax*sizeof(qreg->rows_model[0][0]), SEEK_CUR) != 0) {
		perror("seeking to model");
		break;
	    }
	    nitem = smmax + fread(qreg->rows_model[i],
				  sizeof(qreg->rows_model[0][0]), xsize - smmax, cat_file);
	}
	if(nitem != xsize) {
	   perror("reading model");
	   break;
	}
    }
    fclose(cat_file);

    if(expand_model) {
	/*
	 * Reflect bottom about the origin.
	 */
	for(ir = 0; ir < smmax; ir++) {
	    rp = qreg->rows_model[smmax + 1 + ir];
	    rmp = qreg->rows_model[smmax - 1 - ir];
	    memcpy(rmp, rp, xsize*sizeof(qreg->rows_model[0][0]));
	}
    }
    
    return qreg;
}

static REGION *
read_model(const MODEL_PARAMS *p)
{
    const int expand_model = 1;		/* fill in bottom half of region? */
    int index;				/* index into file */

    if(catalog_file[0] == '\0') {
       shError("read_model: no catalog file is declared");
       return(NULL);
    }

    switch (p->class) {
     case PSF_MODEL:			/* always the first model */
       index = 0; break;
     case DEV_MODEL:
       index = dev_param_to_index(p); break;
     case EXP_MODEL:
       index = exp_param_to_index(p); break;
     default:
       shErrStackPush("read_model: unable to set index for class %d",p->class);
       return(NULL);
    }

    return read_model_index(index, expand_model);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * This function will return a region containing a realization of a PSF, or
 * a seeing-convolved de Vaucouleurs law or Exponential law.
 *
 * The model isn't scaled to a given amplitude, instead the total flux in the
 * model is returned, allowing you to scale if you so desire
 */
REGION *
phAnyModelMake(const MODEL_PARAMS *p, 	/* parameters of model to create */
	       float *flux,		/* total flux of model (or NULL) */
	       int *max,		/* max. pixel value before rotation,
					   including any SOFT_BIAS, or NULL */
	       float *scale)		/* amount model was scaled up, or NULL*/
{
    REGION *reg;
    REGION *scr_reg;			/* scratch region used for smoothing */
    REGION *qreg;
    int smmax;
    int ir;
    int ic;
    PIX *rp, *rmp;
    float iscale = 0;			/* storage for scale if it's NULL */
    int xsize;
    int ysize;
    int xoffset;
    int yoffset;
    int filt_size = 31;
    float sigma1, sigma2;

    if(scale == NULL) {
       scale = &iscale;
    }

    smmax = pro_cheader.proc_smmax;
    
    if(p->class == PSF_MODEL || p->exact) {
       qreg = p_phMakeExactModel(p);
    } else {
       qreg = read_model(p);
    }
    shAssert(qreg != NULL);		/* in reality this could fail, but
					   it won't be able to when we keep
					   everything in memory, so don't
					   propagate the possible error now */
    xsize = qreg->ncol;
    ysize = qreg->nrow;
/*
 * Smooth with PSF
 */
    scr_reg = shRegNew("scratch", qreg->nrow, qreg->ncol, qreg->type);
/*
 * PSF is now a circularly symmetric double gaussian. This is not a
 * separable filter, so we have to create its components and add them up
 */
    if(p->psf->sigmax1 != p->psf->sigmay1 || 
       p->psf->sigmax2 != p->psf->sigmay2) {
       shError("phAnyModelMake enforces circular PSFs; sorry");
    }
    sigma1 = (p->psf->sigmax1 + p->psf->sigmay1)/2;
    sigma2 = (p->psf->sigmax2 + p->psf->sigmay2)/2;

/*
 * small component.
 */
    if(!p->exact && BROKEN_MODEL_MAGS) {
       REGION *temp;			/* temporary accumulator for
					   psf-smoothed images */
       int i;
       float sum1, sum2;
       float *filter;			/* PSF smoothing filter */

       temp = shRegNew("temp", qreg->nrow, qreg->ncol, qreg->type);
       filter = shMalloc(filt_size*sizeof(float));

       sum1 = 0;
       for(i = 0;i != filt_size;i++) {
	  filter[i] = exp(-pow((i - filt_size/2)/sigma1,2)/2);
	  sum1 += filter[i];
       }
       
       sum1 *= sqrt(1 + p->psf->b);	/* normalise to 1/(1 + b); n.b.
					   filter's applied in x _and_ y */
       for(i = 0;i != filt_size;i++) {
	  filter[i] /= sum1;
       }
       phConvolve(temp, qreg, scr_reg, filt_size, filt_size, filter,
		  filter, 0, CONVOLVE_ANY);
/*
 * large component (if it exists)
 */
       sum2 = 0;
       for(i = 0;i != filt_size;i++) {
	  filter[i] = exp(-pow((i - filt_size/2)/sigma2,2)/2);
	  sum2 += filter[i];
       }
       if(fabs(sum2) < 1e-10) {		/* no second component to add in */
	  shRegDel(qreg);
	  qreg = temp;
       } else {
	  for(i = 0;i != filt_size;i++) {
	     filter[i] /= sum2;
	  }
	  phConvolve(qreg, qreg, scr_reg, filt_size, filt_size, filter,
		     filter, 0, CONVOLVE_ANY);
	  
	  /*
	   * the AMPLITUDE of the second Gaussian is equal to the amplitude
	   * of the first, multiplied by dgpsf->b -- but this isn't what
	   * we're doing here.  We've already scaled the model down by
	   * a factor of (sigma2/sigma1)^2.  This is wrong.
	   */
	  shRegIntConstMult(qreg, p->psf->b/(1 + p->psf->b));
	  shRegIntAdd(qreg,temp);	/* assemble full PSF-smoothed image */
	  shRegDel(temp);
       }
       
       shFree(filter);
    } else {
       if(p->psf->b == 0) {
	  phConvolveWithGaussian(qreg, qreg, scr_reg, filt_size, sigma1,
				 0, CONVOLVE_ANY);
       } else {
	  const float b = p->psf->b;
	  const float psfscale = (pow(sigma1,2) + b*pow(sigma2, 2));
	  REGION *temp = shRegNew("temp", qreg->nrow, qreg->ncol, qreg->type);
	  
	  phConvolveWithGaussian(temp, qreg, scr_reg, filt_size, sigma2,
				 0, CONVOLVE_ANY);
	  phConvolveWithGaussian(qreg, qreg, scr_reg, filt_size, sigma1,
				 0, CONVOLVE_ANY);
	  shRegIntLincom(qreg, temp,
			 0, pow(sigma1,2)/psfscale, b*pow(sigma2, 2)/psfscale, LINCOM_EXACT);
	  shRegDel(temp);
       }
    }
    shRegDel(scr_reg);
/*
 * Convert to desired pixel type.  If we're converting from float to
 * int, dither to preserve fractional values
 */
    if(TYPE_PIX == qreg->type) {
       *scale = 1;			/* no scaling */
    } else {
       REGION *tmp = shRegNew("", ysize, xsize, TYPE_PIX);

       if(tmp->type == TYPE_U16 && qreg->type == TYPE_FL32) { /* Dither */
	  int i, j;
	  float fac = (MAX_U16 - SOFT_BIAS - 100)/qreg->rows_fl32[smmax][smmax];
	  FL32 *qrow;
	  U16 *trow;
	  int tmpval;

	  *scale = fac;
	  shAssert(phRandomIsInitialised());
	  for(i = 0; i < ysize; i++) {
	     qrow = qreg->rows_fl32[i];
	     trow = tmp->rows_u16[i];
	     for(j = 0; j < xsize; j++) {
		tmpval = fac*qrow[j] + phRandomUniformdev();
		trow[j] = (tmpval > MAX_U16 - 100) ? MAX_U16 - 100 : tmpval;
	     }
	  }
       } else {
	  *scale = 1;
	  shRegIntCopy(tmp, qreg);
	  if(tmp->type == TYPE_U16) {
	     shRegIntConstAdd(tmp, -SOFT_BIAS, 0); /* ... so remove it */
	  } else if(qreg->type == TYPE_U16) { /* shRegIntCopy removed bias */
	     shRegIntConstAdd(tmp, SOFT_BIAS, 0); /* ... so put it back */
	  }
       }
       shRegDel(qreg);
       qreg = tmp;
    }
/*
 * Do we care about the maximum value?
 */
    if(max != NULL) {
       *max = qreg->ROWS[smmax][smmax] + SOFT_BIAS;
    }
/*
 * Expand into full region. The "top right" includes the central pixel,
 * which is copied to (ysize - 1, xsize - 1) in scr_reg
 */
    xsize -= 2*smmax;
    ysize -= 2*smmax;
    scr_reg = shRegNew("scratch", 2*ysize - 1, 2*xsize - 1, TYPE_PIX);

    for(ir = 1; ir < ysize; ir++) {	/* bottom left */
	rp = &qreg->ROWS[smmax + ir][smmax];
	rmp = &scr_reg->ROWS[ysize - 1 - ir][xsize - 1];
	for(ic = 1; ic < xsize; ic++)
	    rmp[-ic] = rp[ic] + SOFT_BIAS;
    }
    for(ir = 1; ir < ysize; ir++) {	/* bottom right */
	rp = &qreg->ROWS[smmax + ir][smmax];
	rmp = &scr_reg->ROWS[ysize - 1 - ir][xsize - 1];
	for(ic = 0; ic < xsize; ic++)
	    rmp[ic] = rp[ic] + SOFT_BIAS;
    }
    for(ir = 0; ir < ysize; ir++) {	/* top left */
	rp = &qreg->ROWS[smmax + ir][smmax];
	rmp = &scr_reg->ROWS[ysize - 1 + ir][xsize - 1];
	for(ic = 1; ic < xsize; ic++)
	    rmp[-ic] = rp[ic] + SOFT_BIAS;
    }
    for(ir = 0; ir < ysize; ir++) {	/* top right */
	rp = &qreg->ROWS[smmax + ir][smmax];
	rmp = &scr_reg->ROWS[ysize - 1 + ir][xsize - 1];
	for(ic = 0; ic < xsize; ic++)
	    rmp[ic] = rp[ic] + SOFT_BIAS;
    }
    shRegDel(qreg);
/*
 * Now rotate.
 */
    if(fabs(p->orient) < 1e-6) {
       reg = scr_reg;
    } else {
       reg = shRegIntRotate(scr_reg, -p->orient, SOFT_BIAS, 1, 1);
       shRegDel(scr_reg);
    }
/*
 * If needed, pad region out so that phProfileExtract() will work. Because
 * the centre may be a pixel or so off, we need to be a little larger than
 * SYNC_REG_SIZEI
 */
    if(reg->nrow < SYNC_REG_SIZEI + 2 || reg->ncol < SYNC_REG_SIZEI + 2) {
	xsize = (reg->ncol < SYNC_REG_SIZEI + 10) ?
					       SYNC_REG_SIZEI + 10 : reg->ncol;
	ysize = (reg->nrow < SYNC_REG_SIZEI + 10) ?
					       SYNC_REG_SIZEI + 10 : reg->nrow;

	/* ensure even padding */
	if((xsize-reg->ncol)&1)
	    xsize++;
	if((ysize-reg->nrow)&1)
	    ysize++;

	scr_reg = shRegNew("padded", ysize, xsize, TYPE_PIX);
	xoffset = (xsize - reg->ncol)/2;
	yoffset = (ysize - reg->nrow)/2;
	shRegIntSetVal(scr_reg, SOFT_BIAS);
	for(ir = 0; ir < reg->nrow; ir++) {
	    memcpy(&scr_reg->ROWS[yoffset+ir][xoffset], reg->ROWS[ir],
							reg->ncol*sizeof(PIX));
	}
	shRegDel(reg);
	reg = scr_reg;
    }
/*
 * Maybe save image to help with debugging
 */
   {
      static char *dump_filename = NULL; /* write data to this file?
					    For use from gdb */
      if(dump_filename != NULL) {
	 shRegWriteAsFits((REGION *)reg,
			  dump_filename, STANDARD, 2, DEF_NONE, NULL, 0);
      }
   }
/*
 * if flux isn't NULL, find the total flux in the model. This isn't exactly
 * the total flux in the requested model due to the dithering
 */
    if(flux != NULL) {
       const int nrow = reg->nrow, ncol = reg->ncol;
       PIX *row;
       float sum = 0;

       for(ir = 0;ir < nrow;ir++) {
	  row = reg->ROWS[ir];
	  for(ic = 0;ic < ncol;ic++) {
	     sum += row[ic] - SOFT_BIAS; /* subtract here to avoid roundoff */
	  }
       }
       *flux = sum;
/*
 * Compare with expected model value
 */
#if 0
       if(p->class != PSF_MODEL) {
	  float model_flux = *scale*phFluxGetFromModel(p);
	  
	  if(model_flux != 0) {
	     if(fabs(*flux/model_flux - 1) > 2e-2) {
		fprintf(stderr,"class = %d aratio = %.3f rsize = %7.3f "
			"flux = %g model_flux = %g rat = %g\n",
			p->class, p->aratio, p->rsize,
			*flux, model_flux, *flux/model_flux);
	     }
	  }
       }
#endif
    }
/*
 * Record where the center of the object is (or would be if there weren't problems
 * in the rotation code that moved it a little -- or did I fix those?)
 */
    shHdrInsertDbl(&reg->hdr, "CPEAK", reg->ncol/2, "Column pixel of object's centre");
    shHdrInsertDbl(&reg->hdr, "RPEAK", reg->nrow/2, "Row pixel of object's centre");
/*
 * Add an empty mask.
 */
    reg->mask = (MASK *) phSpanmaskNew(reg->nrow, reg->ncol);

    return reg;
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Here's a version of phAnyModelMake() that scales to a given total flux
 */
REGION *
phAnyModelMakeScaled(const MODEL_PARAMS *p, /* parameters of model to create */
		     float totflux,	/* desired total flux of model */
		     int *max)		/* maximum pixel value (neglecting
					   any rotation) */
{
   float flux;				/* flux in returned model */
   int i, j;
   int nrow, ncol;			/* == reg->n{row,col} */
   REGION *reg;				/* the model in question */
   float scale;				/* how much to scale model */

   reg = phAnyModelMake(p, &flux, max, NULL);
   nrow = reg->nrow; ncol = reg->ncol;

   scale = totflux/flux;

   for(i = 0; i < nrow; i++) {
      PIX *row = reg->ROWS[i];
      if(reg->type == TYPE_FL32 || FLOATING_PHOTO) {
	 for(j = 0; j < ncol; j++) {
	    row[j] = scale*(row[j] - SOFT_BIAS) + SOFT_BIAS;
	 }
      } else if(reg->type == TYPE_U16) {
	 int tmpval;
	 for(j = 0; j < ncol; j++) {
	    tmpval =
	      scale*(row[j] - SOFT_BIAS) + phRandomUniformdev() + SOFT_BIAS;
	    row[j] = (tmpval > MAX_U16) ? MAX_U16 : tmpval;
	 }
      } else {
	 for(j = 0; j < ncol; j++) {
	    row[j] = scale*(row[j] - SOFT_BIAS) + phRandomUniformdev() +
								     SOFT_BIAS;
	 }      
      }
   }

   if(max != NULL) {
      *max = *max*scale + 0.5;
   }

   return(reg);
}

/*****************************************************************************/
/*
 * Make a model that's I0*pow((r < rmin ? rmin : r), -beta) and return it as a region
 */
REGION *
phMakePowerLawModel(const float I0,	/* amplitude of power law */
		    const float beta,	/* power law index */
		    const float rmin,	/* minimum radius for power-law wings (constant inside) */
		    int nrow,		/* Number of rows in region that model will be subtracted from */
		    int ncol,		/* Number of columns in region that model will be subtracted from */
		    double rowc,     /* row-position of object in region that model will be subtracted from */
		    double colc	  /* column-position of object in region that model will be subtracted from */
    )
{
    const float hbeta = beta/2;		/* == beta/2 */
    float min = 0.1;			/* minimum value in model */
    const int size = exp(-log(min/I0)/beta); /* output region is logically
						(2*size + 1)x(2*size + 1); centre (size, size) (but we
						may not allocate all that memory */
    const float val0 = I0*pow(rmin, -beta); /* value of profile at rmin */
    int row0 = 0, col0 = 0;		/* coordinates of lower-left-hand corner of allocated REGION */
    const int Size = 2*size + 1;	/* logical size of entire image of object */

    if (nrow <= 0 || ncol <= 0) {
	nrow = ncol = Size;
    } else {
	int r0, c0;			/* bottom left corner of needed REGION in Size*Size model */
	int r1, c1;			/* top right corner of needed REGION in Size*Size model */

	c0 = size - (int)colc;
	c1 = size + (ncol - (int)colc);
	if (c0 < 0) c0 = 0;
	if (c1 > Size) c1 = Size;
	
	r0 = size - (int)rowc;
	r1 = size + (nrow - (int)rowc);
	if (r0 < 0) r0 = 0;
	if (r1 > Size) r1 = Size;

	ncol = c1 - c0 + 1; col0 = c0;
	nrow = r1 - r0 + 1; row0 = r0;
    }

    REGION *reg = shRegNew("phMakePowerLawModel", nrow, ncol, TYPE_FL32);
    reg->row0 = row0;  reg->col0 = col0;
    
    shRegClear(reg);

    {
	int r, c;
	for (r = 0; r < nrow; r++) {
	    for (c = 0; c < ncol; c++) {
		float dr = r + row0 - size, dc = c + col0 - size;
		const float r2 = dr*dr + dc*dc;
		const float val = (r2 <= rmin*rmin) ? val0 : I0*pow(r2, -hbeta);
		       
		reg->rows_fl32[r][c] = val;
	    }
	}
    }

    if(!FLOATING_PHOTO) {
       int i, j;
       const int nrow = reg->nrow;
       const int ncol = reg->ncol;
       REGION *out = shRegNew(reg->name, nrow, ncol, TYPE_PIX);
       out->row0 = reg->row0; out->col0 = reg->col0;
       
       for(i = 0; i < nrow; i++) {
	  float *irow = reg->rows_fl32[i];
	  PIX *orow = out->ROWS[i];
	  int tmpval;
	  for(j = 0; j < ncol; j++) {
	     tmpval = irow[j] + phRandomUniformdev() + SOFT_BIAS;
	     orow[j] = (tmpval > MAX_U16) ? MAX_U16 : tmpval;
	  }
       }
       shRegDel(reg); reg = out;
    }
/*
 * Record where the center of the object is
 */
    shHdrInsertDbl(&reg->hdr, "CPEAK", size - col0, "Column pixel of object's centre");
    shHdrInsertDbl(&reg->hdr, "RPEAK", size - row0, "Row pixel of object's centre");

    return reg;
}

/*****************************************************************************/

static void
set_coeffs_f(float dx,
	     float *coeffs,
	     float *cbell,
	     int sincsize)
{
   int i;

   if(dx == 0 || dx == 1) {
      for(i = 0; i < sincsize;i++) {
	 coeffs[i] = coeffs[-i] = 0;
      }

      if(dx == 0) {
	 coeffs[0] = 1;
      } else if(dx == 1) {
	 coeffs[-1] = 1;
      } else {
	 shFatal("set_coeffs_f: you cannot get here");
      }
   } else {
      float ftmp = (sincsize & 0x1) == 1 ? dx : -dx;
      float fsum = 0;
      int sum;

      ftmp = (ftmp < 0) ? -1 : 1;
      for(i = -sincsize + 1;i < sincsize;i++) {
	 coeffs[i] =  ftmp/(i + dx)*cbell[i];
	 fsum += coeffs[i];
	 ftmp = -ftmp;
      }
/*
 * normalise
 */
      ftmp = 1/fsum;
      sum = 0;
      for(i = -sincsize + 1;i < sincsize;i++) {
	 coeffs[i] *= ftmp;
	 sum += coeffs[i];
      }
   }
}

static void
set_coeffs(float dx,
	   int *coeffs,
	   float *cbell,
	   int sincsize,
	   int fac)
{
   int i;
   float *fcoeffs;			/* floating version of coeffs */
   int sum;

   fcoeffs = alloca((2*sincsize - 1)*sizeof(float));
   fcoeffs += sincsize - 1;
   set_coeffs_f(dx,fcoeffs,cbell,sincsize);
/*
 * Scale to fac
 */
   sum = 0;
   for(i = -sincsize + 1;i < sincsize;i++) {
      coeffs[i] = fac*fcoeffs[i] + 0.5;
      sum += coeffs[i];
   }
/*
 * fixup rounding errors
 */
   sum -= fac;
   if(sum < 0) {
      coeffs[0]++; sum++;
      for(i = 1;i < sincsize;i++) {
	 if(sum == 0) break;
	 coeffs[i]++; sum++;
	 if(sum == 0) break;
	 coeffs[-i]++; sum++;
      }
   } else if(sum > 0) {
      coeffs[0]--; sum--;
      for(i = 1;i < sincsize;i++) {
	 if(sum == 0) break;
	 coeffs[i]--; sum--;
	 if(sum == 0) break;
	 coeffs[-i]--; sum--;
      }
   }
   shAssert(sum == 0);
}

/*****************************************************************************/
/*
 * We rotate using 3 shears; in interpolating we need to scale up the image
 * by SCALE to avoid floating point arithmetic
 */
#define SINCSIZE 7
#define FAC (1<<15)			/* SHOULD BE POWER OF 2 <= 2^15 */
#if FLOATING_PHOTO
#  define HALFFAC 0			/* no need to round */
#else
#  define HALFFAC (FAC>>1)		/* == FAC/2 */
#endif

static REGION *
shear_x(const REGION *in,		/* input region */
	float shear,			/* how much to shear by */
	int bkgd,			/* background value */
	const char *name,		/* name of output region */
	int out_nrow,			/* size of output region */
	int out_ncol,
	int junk,			/* how many columns to ignore in input
					   region (at each end of each row) */
	int use_sinc)			/* use sinc interpolation? */
{
   float *cbell, s_cbell[2*SINCSIZE - 1]; /* cosbell filter */
   int *coeffs, s_coeffs[2*SINCSIZE - 1]; /* sinc coeffs*FAC */
   int col, row;			/* column and row counters */
   int dnew0;				/* offset to new0 to ensure that the
					   input image's corner fits in out */
   float dx;				/* extra shift to align centre */
   int endcol;				/* last column to process in in */
   int fracnew0, omfracnew0;		/* scaled weights for pixels */
   int i;
   const PIX *inptr;			/* pointers to rows of REGION in */
   PIX *outptr;				/* pointers to rows of REGION out */
   int in_ncol;				/* == in->ncol */
   int inew0;				/* == (int)new0 */
   float new0;				/* start of data in output row */
   REGION *out = shRegNew(name, out_nrow, out_ncol, TYPE_PIX);
#if FLOATING_PHOTO
   float prevxel, tmp;
#else
   int prevxel, tmp;
#endif
   REGION *true_out;			/* the full output REGION */

   shAssert(in != NULL && in->type == TYPE_PIX && in->nrow <= out_nrow);

   if(out_nrow > in->nrow) {		/* use a subregion */
      int row0 = (out_nrow - in->nrow)/2;
      true_out = out;
      out = shSubRegNew(name,out,in->nrow,out_ncol,row0,0,NO_FLAGS);

      for(row = 0;row < row0;row++) {
	 outptr = true_out->ROWS[row];

	 col = 0;
	 while(col < out_ncol) {
	    outptr[col++] = bkgd;
	 }
      }

      for(row = row0 + in->nrow;row < out_nrow;row++) {
	 outptr = true_out->ROWS[row];

	 col = 0;
	 while(col < out_ncol) {
	    outptr[col++] = bkgd;
	 }
      }

      out_nrow = out->nrow;		/* i.e. of the subregion */
   } else {
      true_out = out;
   }

   if(in->ncol < 2*SINCSIZE) use_sinc = 0;
/*
 * calculate cosbell filter
 */
   if(use_sinc) {
      coeffs = s_coeffs + SINCSIZE - 1;	/* so centre of filter is coeffs[0] */
      cbell = s_cbell + SINCSIZE - 1;	/* ditto for the cosbell */
      for(i = 0; i < SINCSIZE;i++) {
	 cbell[-i] = cbell[i] = 0.5*(1 + cos(i*M_PI/SINCSIZE));
      }
   } else {
      cbell = NULL; coeffs = NULL;
   }

   in_ncol = in->ncol;

   dnew0 = (shear < 0) ? 0 : out_nrow*shear + 0.9999;

   dx = shear*0.5*(out_nrow - 1) + 0.5*(out_ncol - in_ncol) - dnew0 + junk;

   for(row = 0; row < out_nrow; row++) {
      inptr = in->ROWS[row];
      outptr = out->ROWS[row];

      new0 = dnew0 - shear*row + dx;
      inew0 = (int)new0;
      new0 -= inew0;
      inew0 -= junk;

      col = 0;
      while(col < inew0) {
	 outptr[col++] = bkgd;
      }

      endcol = in_ncol;
      if(endcol > out_ncol - inew0) endcol = out_ncol - inew0;

      fracnew0 = new0*FAC;
      omfracnew0 = FAC - fracnew0;
      prevxel = bkgd;

      if(use_sinc) {
	 set_coeffs(new0,coeffs,cbell,SINCSIZE,FAC);
/*
 * do the first and last SINCSIZE columns by linear interpolation
 */
	 while(col - inew0 < SINCSIZE) {
	    tmp = inptr[col - inew0];
	    outptr[col++] = (fracnew0*prevxel + omfracnew0*tmp + HALFFAC)/FAC;
	    prevxel = tmp;
	 }

	 while(col - inew0 < endcol - SINCSIZE) {
	    tmp = HALFFAC;
	    i = -SINCSIZE + 1;
	    do {
	       tmp += inptr[col - inew0 + i]*coeffs[i];
	    } while(++i < SINCSIZE);
	    outptr[col++] = tmp/FAC;
	 }

	 prevxel = inptr[col - inew0 - 1];
	 while(col - inew0 < endcol) {
	    tmp = inptr[col - inew0];
	    outptr[col++] = (fracnew0*prevxel + omfracnew0*tmp + HALFFAC)/FAC;
	    prevxel = tmp;
	 }
	 if(fracnew0 > 0 && col < out_ncol ) {
	    outptr[col++] = (fracnew0*prevxel + omfracnew0*bkgd + HALFFAC)/FAC;
	 }
      } else {
	 while(col - inew0 < endcol) {
	    tmp = inptr[col - inew0];
	    outptr[col++] = (fracnew0*prevxel + omfracnew0*tmp + HALFFAC)/FAC;
	    prevxel = tmp;
	 }
	 if(fracnew0 > 0 && col < out_ncol ) {
	    outptr[col++] = (fracnew0*prevxel + omfracnew0*bkgd + HALFFAC)/FAC;
	 }
      }
/*
 * set rows that are to the right of the data region to the background
 */
      while(col < out_ncol) {
	 outptr[col++] = bkgd;
      }
   }

   if(true_out != out) {
      shRegDel(out);
      out = true_out;
   }

   return(out);
}

static REGION *
shear_y(const REGION *in,		/* input region */
	float shear,			/* how much to shear by */
	int bkgd,			/* background value */
	const char *name,		/* name of output region */
	int out_nrow,			/* size of output region */
	int out_ncol,
	int junk,			/* how many columns to ignore in input
					   region (at each end of each row) */
	int use_sinc)			/* use sinc interpolation? */
{
   float *cbell, s_cbell[2*SINCSIZE - 1]; /* cosbell filter */
   int *coeffs, s_coeffs[2*SINCSIZE - 1]; /* sinc coeffs*FAC */
   int col, row;			/* column and row counters */
   int dnew0;				/* offset to new0 to ensure that the
					   input image's corner fits in out */
   float dy;				/* extra shift to align centre */
   int endrow;				/* last row to process in in */
   int fracnew0, omfracnew0;		/* scaled weights for pixels */
   int i;
   PIX **in_rows, **out_rows;		/* pointers to rows of in and out */
   int in_nrow;				/* == in->ncol */
   int inew0;				/* == (int)new0 */
   float new0;				/* start of data in output row */
   REGION *out = shRegNew(name, out_nrow, out_ncol, TYPE_PIX);
#if FLOATING_PHOTO
   float prevxel, tmp;
#else
   int prevxel, tmp;
#endif

   shAssert(in != NULL && in->type == TYPE_PIX && in->ncol == out_ncol);

   if(in->nrow < 2*SINCSIZE) use_sinc = 0;
/*
 * calculate cosbell filter
 */
   if(use_sinc) {
      coeffs = s_coeffs + SINCSIZE - 1;	/* so centre of filter is coeffs[0] */
      cbell = s_cbell + SINCSIZE - 1;	/* ditto for the cosbell */
      for(i = 0; i < SINCSIZE;i++) {
	 cbell[-i] = cbell[i] = 0.5*(1 + cos(i*M_PI/SINCSIZE));
      }
   } else {
      cbell = NULL; coeffs = NULL;
   }
/*
 * time to work; first set the pixels that aren't in region in to the
 * background. shear_x does this as it processes the ends of the rows,
 * but for shear_y it's probably better to do it once-and-for-all so
 * as to avoid cache problems
 */
   shRegIntSetVal(out,bkgd);

   in_rows = in->ROWS;
   out_rows = out->ROWS;
   in_nrow = in->nrow;

   dnew0 = (shear < 0) ? 0 : out_ncol*shear + 0.9999;

   dy = shear*0.5*(out_ncol - 1) + 0.5*(out_nrow - in_nrow) - dnew0 + junk;

   for(col = 0; col < out_ncol; col++) {
      new0 = dnew0 - shear*col + dy;
      inew0 = (int)new0;
      new0 -= inew0;
      inew0 -= junk;

      endrow = in_nrow;
      if(endrow > out_nrow - inew0) endrow = out_nrow - inew0;

      fracnew0 = new0*FAC;
      omfracnew0 = FAC - fracnew0;
      prevxel = bkgd;
      row = inew0 > 0 ? inew0 : 0;

      if(use_sinc) {
	 set_coeffs(new0,coeffs,cbell,SINCSIZE,FAC);
/*
 * do the first and last SINCSIZE rows by linear interpolation
 */
	 while(row - inew0 < SINCSIZE && row - inew0 < endrow) {
	    tmp = in_rows[row - inew0][col];
	    out_rows[row++][col] =
			     (fracnew0*prevxel + omfracnew0*tmp + HALFFAC)/FAC;
	    prevxel = tmp;
	 }

	 while(row - inew0 < endrow - SINCSIZE) {
	    tmp = HALFFAC;
	    i = -SINCSIZE + 1;
	    do {
	       tmp += in_rows[row - inew0 + i][col]*coeffs[i];
	    } while(++i < SINCSIZE);
	    out_rows[row++][col] = tmp/FAC;
	 }

	 prevxel = (row - inew0 - 1 < endrow) ?
	   in_rows[row - inew0 - 1][col] : bkgd;

	 while(row - inew0 < endrow) {
	    tmp = in_rows[row - inew0][col];
	    out_rows[row++][col] =
			     (fracnew0*prevxel + omfracnew0*tmp + HALFFAC)/FAC;
	    prevxel = tmp;
	 }
	 if(fracnew0 > 0 && row < out_nrow) {
	    out_rows[row++][col] =
			    (fracnew0*prevxel + omfracnew0*bkgd + HALFFAC)/FAC;
	 }
      } else {
	 while(row - inew0 < endrow) {
	    tmp = in_rows[row - inew0][col];
	    out_rows[row++][col] =
			     (fracnew0*prevxel + omfracnew0*tmp + HALFFAC)/FAC;
	    prevxel = tmp;
	 }
	 if(fracnew0 > 0 && row < out_nrow) {
	    out_rows[row++][col] =
			    (fracnew0*prevxel + omfracnew0*bkgd + HALFFAC)/FAC;
	 }
      }
   }

   return(out);
}

/*****************************************************************************/
/*
 * This rotation is based on an algorithm by Alan Paeth (Graphics
 * Interface '86, p. 77) which uses the fact that any rotation can be
 * expressed as the product of three shears.  If we shear by a factor
 * \alpha in x, then \beta in y, then \gamma in x again, the relation
 * between the shear factors and the rotation angle, \theta is:
 * \alpha = \gamma = -tan(\theta/2)
 * \beta = sin(\theta)
 * The main benefit of this algorithm is it allows a quick way to
 * fairly accurately anti-alias.
 *
 * <AUTO>
 * The region rotation code was originally taken from pnmrotate which bears
 * the following notice:
 *
**-----------------------------------------------------------------------------
** pnmrotate.c - read a portable anymap and rotate it by some angle
**
** Copyright (C) 1989, 1991 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**-----------------------------------------------------------------------------
 *
 * It has been _extensively_ rewritten since then, so don't blame poor Jef
 * for it.
 * </AUTO>
 */
REGION *
shRegIntRotate(REGION *in,		/* region to rotate */
	       double angle,		/* angle to rotate clockwise by;
					   degrees */
	       int bgxel,		/* value for pixels not in in */
	       int use_sinc,		/* use sinc interpolation */
	       int square)		/* make output region square */
{
   int rot180 = 0;			/* rotate through an extra 180? */
   REGION* temp1xels;
   REGION* temp2xels;
   REGION* out;				/* output, rotated, region */
   int innrow, inncol;			/* == in->{nrow,ncol} */
   int outnrow, outncol;		/* size of rotated region, out */
   int tempncol, yshearjunk, x2shearjunk;
   float xshearfac, yshearfac;

   shAssert(in != NULL && in->type == TYPE_PIX && in->ROWS != NULL);

   while(angle < -180) angle += 360;
   while(angle >  180) angle -= 360;

   if(angle >= 90) {
      angle -= 180;
      rot180 = 1;
   }
   if(angle < -90) {
      angle += 180;
      rot180 = 1;
   }
   shAssert ( angle >= -90.0 && angle <= 90.0 );
   
   angle = angle*M_PI/180.0;		/* convert to radians */
   innrow = in->nrow;
   inncol = in->ncol;

   xshearfac = fabs(tan(angle/2));
   yshearfac = fabs(sin(angle));

   tempncol = innrow*xshearfac + inncol + 0.999999;
   if(tempncol%2 == 0) tempncol++;
   yshearjunk = (tempncol - inncol)*yshearfac;
   outnrow = tempncol*yshearfac + innrow - 2*yshearjunk + 0.999999;
   if(outnrow%2 == 0) outnrow++;
   x2shearjunk = (outnrow - innrow + yshearjunk)*xshearfac;
   outncol = outnrow*xshearfac + tempncol + 0.999999 - 2*x2shearjunk;
   if(outncol%2 == 0) outncol++;
/*
 * First shear X into temp1xels
 */
   if(angle > 0) {
      xshearfac = -xshearfac;
   } else {
      yshearfac = -yshearfac;
   }
   temp1xels = shear_x(in, xshearfac, bgxel,
		       "xshear1", innrow, tempncol, 0, use_sinc);
/*
 * Now inverse shear Y from temp1 into temp2.
 */
   temp2xels = shear_y(temp1xels, yshearfac, bgxel,
		       "yshear", outnrow, tempncol, yshearjunk, use_sinc);
   shRegDel(temp1xels);
/*
 * Finally, shear X from temp2 into xelrow
 */
   if(square) {
      if(outnrow < outncol) {
	 outnrow = outncol;
      } else {
	 x2shearjunk -= (outnrow - outncol)/2; /* centre rotated region */
	 outncol = outnrow;
      }
   }
   out = shear_x(temp2xels, xshearfac, bgxel,
		 "rotated", outnrow, outncol, x2shearjunk, use_sinc);
   shRegDel(temp2xels);

   if(rot180) {
      shRegRowFlip(out); shRegColFlip(out);
   }

   return out;
}

/*****************************************************************************/

static COMP_CSTATS *
CellCompress(const CELL_STATS *stats)
{
    COMP_CSTATS *cs;
    int iann;
    int icell;
    int icomp;
    int sect0;

    shAssert(stats != NULL);

    cs = shMalloc(sizeof(*cs));
    cs->ncells = stats->ncell/2 + 1;
    cs->mem = shMalloc(3*sizeof(float)*cs->ncells);
    cs->mean = cs->mem;
    cs->median = cs->mean + cs->ncells;
    cs->sig = cs->median + cs->ncells;
    /* zero is special */
    cs->mean[0] = stats->cells[0].mean;
    cs->median[0] = stats->cells[0].qt[1];
    cs->sig[0] = stats->cells[0].sig;
    icomp = 1;
    /* loop through, grabbing half of each anulli */
    for(iann = 1; iann < stats->nannuli; iann++) {
	sect0 = (iann - 1)*NSEC + 1;
	for(icell = sect0; icell < sect0 + NSEC/2; icell++) {
	    cs->mean[icomp] = stats->cells[icell].mean;
	    cs->median[icomp] = stats->cells[icell].qt[1];
	    cs->sig[icomp] = stats->cells[icell].sig;
	    ++icomp;
	}
    }
    shAssert(icomp == cs->ncells);
    return cs;
}

/*****************************************************************************/
/*
 * Make and compress a model
 */
static COMP_CSTATS *
make_model(MODEL_PARAMS *p,		/* model to make */
	   float orient,		/* at this orientation */
	   int outer,			/* outer limit of extracted profile */
	   const FRAMEPARAMS *fparams)	/* gain etc. */
{
   COMP_CSTATS *cstats;			/* compressed cell structure */
   float flux = 0;			/* total flux in model */
   int i;
   int max = 0;				/* maximum pixel value in a model */
   OBJECT *obj;				/* a found object */
   REGION *reg;				/* region containing model */
   const CELL_STATS *stats_model;	/* extracted profile */
   float scale = 0;			/* how much model was scaled up */

   p->orient = orient;
   reg = phAnyModelMake(p, &flux, &max, &scale);
   shAssert(FLOATING_PHOTO || max < MAX_U16);
/*
 * Maybe save image to help with debugging
 */
   {
      static char *dump_filename = NULL; /* write data to this file?
					    For use from gdb */
      if(dump_filename != NULL) {
	 shRegWriteAsFits((REGION *)reg,
			  dump_filename, STANDARD, 2, DEF_NONE, NULL, 0);
      }
   }
/*
 * All this is to find the center of the model.
 */
   obj = phObjectNew(1);
   {
      int ret;
      ret = phRegIntMaxPixelFind(reg, SOFT_BIAS,
				 reg->nrow/2 - 5, reg->ncol/2 - 5,	
				 reg->nrow/2 + 5, reg->ncol/2 + 5,
				 &obj->peaks->peaks[0]->rpeak,
				 &obj->peaks->peaks[0]->cpeak, NULL, NULL);
      shAssert(ret == 0);
   }
   phObjectCenterFit(obj, reg, fparams, ALWAYS_SMOOTH);
   /* Extract the cells and compress them */
   stats_model = phProfileExtract(-1, -1, reg, obj->rowc, obj->colc,
				  outer, SOFT_BIAS, 0.0, 0);
   if(stats_model == NULL) {
      shRegWriteAsFits(reg,"fail.fts",STANDARD,2,DEF_NONE,NULL,0);
   }
   shAssert(stats_model != NULL);
   
   phObjectDel(obj);
   
   phSpanmaskDel((SPANMASK *) reg->mask);
   reg->mask = NULL;
   shRegDel(reg);
   cstats = CellCompress(stats_model);
/*
 * Undo any scaling
 */
   if(scale != 1.0) {
      const float iscale = 1.0/scale;
      int ncells = cstats->ncells;
      float *cs_model = cstats->mem;
#if USE_MODEL_SIG
      float *cs_sig = cstats->sig;
#endif

      flux *= iscale;
      for(i = 0; i < ncells; i++) {
	 cs_model[i] *= iscale;
#if USE_MODEL_SIG
	 cs_sig[j] *= iscale;
#endif
      }
   }
/*
 * Now figure out the total flux in the model
 */
   cstats->totflux = flux;
   
   return(cstats);
}

/*****************************************************************************/
/*
 * Return the Fourier coefficients for an annulus, assumed to have two
 * planes of symmetry and a total of 12 sectors
 */
static float *
fourier_annulus(const float *model0,
		const float *model15)
{
   float M[NSEC/2 + 1];			/* points from model0 and model15 */
   static float C[NSEC/2];		/* Fourier coefficients */

   shAssert(NSEC/2 == 6);		/* hand coded stuff follows */
/*
 * unpack the two models, at 0 and 15 degrees, to give the model within
 * this annulus, sampled every 15 degrees
 */
   M[0] = model0[0];
   M[1] = model15[1];
   M[2] = model0[1];
   M[3] = model15[2];
   M[4] = model0[2];
   M[5] = model15[3];
   M[6] = model0[3];
/*
 * get a Fourier series for the model
 */
   C[0] = 1/12.0*(M[0] + 2*(M[1] + M[2] + M[3] + M[4] + M[5]) + M[6]);
   C[1] = 1/6.0*(M[0] + SQRT3*M[1] + M[2] - M[4] - SQRT3*M[5] - M[6]);
   C[2] = 1/6.0*(M[0] + M[1] - M[2] - 2*M[3] - M[4] + M[5] + M[6]);
   C[3] = 1/6.0*(M[0] - 2*M[2] + 2*M[4] - M[6]);
   C[4] = 1/6.0*(M[0] - M[1] - M[2] + 2*M[3] - M[4] - M[5] + M[6]);
   C[5] = 1/6.0*(M[0] - SQRT3*M[1] + M[2] - M[4] + SQRT3*M[5] - M[6]);
   
   return(C);
}

/*****************************************************************************/
/*
 * Given two COMP_CSTATSs, representing models 15degrees apart,
 * return a COMP_CSTATS whose values are the Fourier coefficients
 * of the angular dependence of those two models
 *
 * Both input COMP_CSTATSs are freed (or reused)
 */
static COMP_CSTATS *
fourier_analyse(COMP_CSTATS *cstats0,
		COMP_CSTATS *cstats15)
{
   float *C;				/* Fourier coefficients */
   const int ncells = cstats0->ncells;	/* number of cells */
   int sect0;				/* index of 0th sector in annulus */

   shAssert(cstats15->ncells >= ncells);
   
   for(sect0 = 1; sect0 < ncells; sect0 += NSEC/2) {
      C = fourier_annulus(&cstats0->mean[sect0], &cstats15->mean[sect0]);
      memcpy(&cstats0->mean[sect0], C, NSEC/2*sizeof(float));

      C = fourier_annulus(&cstats0->median[sect0], &cstats15->median[sect0]);
      memcpy(&cstats0->median[sect0], C, NSEC/2*sizeof(float));

      C = fourier_annulus(&cstats0->sig[sect0], &cstats15->sig[sect0]);
      memcpy(&cstats0->sig[sect0], C, NSEC/2*sizeof(float));
   }

   cstats0->totflux = 0.5*(cstats0->totflux + cstats15->totflux);
   
   shFree(cstats15->mem); shFree(cstats15);
   
   return(cstats0);
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * Make a seeing convolved cell catalogue from a image profile
 * catalogue.
 *
 * The structure of the file is:
 *
 *      |=========================================|
 *	| cell header (includes raw header)       |
 *      |=========================================|
 *      | Psf | exp_0 ... exp_n | deV_0 ... deV_n | Canonical_0
 *      |-----+-----------------+-----------------|
 *      | Psf | exp_0 ... exp_n | deV_0 ... deV_n | Single_0
 *      |=========================================|
 *      | Psf | exp_0 ... exp_n | deV_0 ... deV_n | Canonical_1
 *      |-----+-----------------+-----------------|
 *      | Psf | exp_0 ... exp_n | deV_0 ... deV_n | Single_1
 *      |=========================================|
 *      | Psf | exp_0 ... exp_n | deV_0 ... deV_n | Canonical_2
 *      |-----+-----------------+-----------------|
 *      | Psf | exp_0 ... exp_n | deV_0 ... deV_n | Single_2
 *      |=========================================|
 *      :                                         :
 *      :                                         :
 *
 * Where Canonical models are of the form N(s) + b*N(a*s)
 * (for fixed a and b, e.g. 3 and 1/9), and single models are N(s).
 *
 * The values of s, the inner sigma, are given by fseeing + n*dseeing
 *
 * Within each type of model (at a given seeing), the models are arranged
 * in decreasing size (fastest changing index), and decreasing axis ratio:,
 * e.g.
 *  (re,ab) = {(10,1) (5,1) (1,1)  (10,.5) (5,.5) (1,.5)  (10,0) (5,0) (1,0)}
 */

int
phCellMakeProfCat(char *outfile,	/* cell catalog file */
		  int nseeing,		/* number of seeings */
		  double fseeing,	/* width of first seeing */
		  double dseeing,	/* seeing increment */
		  float psf_sigma_ratio, /* ratio of sigma2 to sigma1 */
		  float psf_amp_ratio,	/* ratio "b" of two Gaussians */
		  int exact,		/* make exact models? */
		  int show_progress)	/* print progress report to stderr? */
{
    int iseeing;
    int imodel;
    FILE *fout;
    FRAMEPARAMS *fparams;		/* gain etc. */
    CELLPROCATHEADER header;
    long offsets_offset;		/* start of offsets table in file */
    int nitem;
    MODEL_PARAMS p;
    const CELL_STATS *stats_model;
    COMP_CSTATS *cstats0, *cstats15;	/* compressed profile at 0 and 15 deg*/
    COMP_CSTATS *cstats_f;		/* Fourier analysed cstats0, cstats15*/
    int nmade, nmodel;			/* used to keep the user informed */
    long *offsets;
    int outer;				/* outer limit of profiles */
    int m;
    int ipsf;
    const int NPSF = 2;

    shAssert(outfile != NULL);
    shAssert(nseeing >= 1);

    /* open the file */
    if((fout = fopen(outfile,"wb")) == NULL) {
       shErrStackPushPerror("phCellProfCatMake: "
			    "Cannot create catalogue file %s",outfile);
       return(SH_GENERIC_ERROR);
    }
    /* fill in header and write it out. */
    memset(&header, '\0', sizeof(header));
    strcpy(header.idstr,CELLPROF_VERSION " | ");
    strcat(header.idstr, phPhotoName());
    shAssert(strlen(header.idstr) < IDSIZE);

    header.seeing_ratio = psf_sigma_ratio;
    header.seeing_b = psf_amp_ratio;
    header.nseeing = nseeing;
    header.fseeing = fseeing;
    header.dseeing = dseeing;

    header.max_models = 2*nseeing*pro_cheader.proc_maxscat;
    header.prof_cat = pro_cheader;
    header.crc = phCrcCalcFromFile(catalog_file, 0);
    nitem = fwrite(&header, sizeof(header), 1, fout);
    shAssert(nitem == 1);

    nitem = fwrite(header.prof_cat.proc_catentry[0],
		   sizeof(spro_catentry), MAXSCAT, fout);
    shAssert(nitem == MAXSCAT);

    offsets = shMalloc(header.max_models*sizeof(*offsets));
    offsets_offset = ftell(fout);	/* start of offsets */
    fseek(fout, header.max_models*sizeof(*offsets), SEEK_CUR);

    /* determine outer radius of extract. */
    stats_model = phProfileGeometry();
    shAssert(stats_model != NULL && stats_model->geom != NULL);
    outer = stats_model->geom[stats_model->ncell-1].outer;
    m = 0;

    memset(&p, '\0', sizeof(p));
    if(exact) {
       shFatal("exact argument to phCellMakeProfCat() is untested and suspect");
    }
    p.exact = exact;
    p.psf = phDgpsfNew();
    fparams = phFrameparamsNew('0');
    fparams->sky = phBinregionNewFromConst(0.0,1,1,1,1,5);

    nmodel = 2*nseeing*pro_cheader.proc_maxscat;
    nmade = 0;
    if(show_progress) {
       fprintf(stderr, "model type     n (  n   %%):     aratio      scale sigma1 sigma2       b\n");
    }

    for(iseeing = 0; iseeing < nseeing; iseeing++) {
/*
 * For each seeing, we make two catalogues: one with a psf of
 * N(0, seeing) + N(0, seeing*header.seeing_ratio)*seeing_b
 * and one with just N(0, seeing).
 */
      p.psf->sigmax1 = p.psf->sigmay1 = fseeing + iseeing*dseeing;
      p.psf->sigmax2 = p.psf->sigmay2 = header.seeing_ratio*p.psf->sigmax1;
      for(ipsf = 0; ipsf < NPSF; ipsf++) {
	if(ipsf == 0)
	      p.psf->b = header.seeing_b;
	else
	      p.psf->b = 0.0;

	for(imodel = 0; imodel < pro_cheader.proc_maxscat; imodel++) {
	    p.class = pro_cheader.proc_catentry[imodel]->scat_class;
	    p.rsize = pro_cheader.proc_catentry[imodel]->scat_reff;
	    p.aratio = pro_cheader.proc_catentry[imodel]->scat_axr;

	    nmade++;
	    if(show_progress) {
	       fprintf(stderr,
		       "\rmodel %s %6d (%6.2f%%): %10g %10g %6.2f %6.2f %7.4f",
		       (p.class == PSF_MODEL ? "psf" :
			p.class == DEV_MODEL ? "deV" : "exp"), nmade,
		       (float)100*nmade/nmodel, p.aratio, p.rsize,
		       p.psf->sigmax1, p.psf->sigmax2, p.psf->b);
	       fflush(stderr);
	    }

	    cstats0 = make_model(&p, 0, outer, fparams);
	    cstats15 = make_model(&p, 15, outer, fparams);
/*
 * Fourier analyse that pair of profiles
 */
	    cstats_f = fourier_analyse(cstats0, cstats15);
/*
 * write profile to disk
 */
	    offsets[m] = ftell(fout);
	    m++;

	    nitem = 0;
	    nitem += fwrite(&cstats_f->ncells,sizeof(int),1, fout);
	    nitem += fwrite(&cstats_f->totflux,sizeof(float),1, fout);
	    nitem += fwrite(cstats_f->mem, sizeof(float),
						     3*cstats_f->ncells, fout);
	    shAssert(nitem == 2 + 3*cstats_f->ncells);

	    shFree(cstats_f->mem); shFree(cstats_f);
	}
      }
    }
    if(show_progress) {
       fprintf(stderr,"\n");
    }

    /* write out offsets. */
    fseek(fout, offsets_offset, SEEK_SET);
    nitem = fwrite(offsets, sizeof(*offsets), header.max_models, fout);
    shAssert(nitem == header.max_models);
    shAssert(m == header.max_models);	/* sanity check */

    phDgpsfDel(p.psf);
    phFrameparamsDel(fparams);
    shFree(offsets);
    nitem = fclose(fout);
    shAssert(nitem == 0);
    return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * return the total flux in a model, integrated to infinity
 *
 * We do not, in fact, currently make any corrections for missing flux
 * in the outer parts of the objects unless CORRECT_MODEL_FLUX is
 * true; when we call phFluxGetFromModel() we really do ignore the
 * cutoffs in the model profiles.  Note that this is not a good idea
 * in production code, as for very small models all the flux lies
 * within the seeing disk, and no correction is required.
 */
#define CORRECT_MODEL_FLUX 0

float
phTotalFluxGet(const CELL_STATS *cstats,
	       const MODEL_PARAMS *p)	/* correct to infinity if non-NULL */
{
   int clip = 0;			/* clip estimate of profMean? */
   float flux = 0;			/* the desired flux */
   int i;

   for(i = 0; i < cstats->nannuli; i++) {
      flux += cstats->area[i]*phProfileMean(cstats, i, clip, 1, NULL);
   }

#if CORRECT_MODEL_FLUX
   if(p != NULL) {
      switch (p->class) {
       case EXP_MODEL:
	 flux *= TOTFLUX_EXP/TOTFLUX_CUTOFF_EXP;
	 break;
       case DEV_MODEL:
	 flux *= TOTFLUX_DEV/TOTFLUX_CUTOFF_DEV;
	 break;
       case PSF_MODEL:
	 break;
       default:
	 shFatal("phTotalFluxGet: unknown object class: %d",p->class);
      }
   }
#endif

   return(flux);   
}

/*****************************************************************************/
/*
 * Check a cellprofile table for consistency
 */
int
phProfileTableHeaderCheck(const char *file, /* file to check */
			  int is_celltable, /* file is a cellarray */
			  int crc)	/* expected crc, if non-zero */
{
   CELLPROCATHEADER head;		/* the CELL file's header */
   FILE *fil;				/* the file's FILE */
   spro_cheader s_rawhead;		/* the header if the file's raw */
   spro_cheader *rawhead;		/* pointer to raw part of header */

   if((fil = fopen(file,"rb")) == NULL) {
      shErrStackPushPerror("phProfileTableHeaderPrint: "
			   "Cannot open catalogue file %s for read\n",file);
      return(SH_GENERIC_ERROR);
   }

   if(is_celltable) {
      if(fread(&head, sizeof(head), 1, fil) != 1) {
	 shErrStackPush("phProfileTableHeaderPrint: Cannot read header");
	 fclose(fil);
	 return(SH_GENERIC_ERROR);
      }
      fclose(fil);

      if(strncmp(head.idstr,CELLPROF_VERSION, strlen(CELLPROF_VERSION)) != 0) {
	 head.idstr[IDSIZE - 1] = '\0';	/* may be binary... */
	 fprintf(stderr,"Header is corrupted or out of date. "
		 "Expected\n\t%s\n"
		 "Saw\n\t%s\n",CELLPROF_VERSION,head.idstr);
	 return(SH_GENERIC_ERROR);
      }

      if(crc != 0 && head.crc != crc) {
	 shErrStackPush("%s has an incorrect CRC.\n"
			"Expected 0x%x, saw 0x%x",file, crc, head.crc);
	 return(SH_GENERIC_ERROR);
      }

      rawhead = &head.prof_cat;
   } else {
      if(fread(&s_rawhead, sizeof(s_rawhead), 1, fil) != 1) {
	 shErrStackPush("phProfileTableHeaderPrint: Cannot read raw header");
	 fclose(fil);
	 return(SH_GENERIC_ERROR);
      }
      fclose(fil);

      rawhead = &s_rawhead;
   }
/*
 * check that the raw header looks right
 */
   if(strncmp(rawhead->proc_catver,RAWPROF_VERSION, strlen(RAWPROF_VERSION))
									!= 0) {
      rawhead->proc_catver[IDSIZE - 1] = '\0';	/* may be binary... */
      fprintf(stderr,"Raw header is corrupted or out of date. "
	      "Expected\n\t%s\n"
	      "Saw\n\t%s\n",RAWPROF_VERSION,rawhead->proc_catver);
      return(SH_GENERIC_ERROR);
   }
/*
 * OK, it passes
 */
   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * Get a cellprofile table's hash code (actually the raw file's CRC)
 */
int
phProfileTableHashGet(const char *file,
		      int *crc)
{
   CELLPROCATHEADER head;		/* the CELL file's header */
   FILE *fil;				/* the file's FILE */

   if((fil = fopen(file,"rb")) == NULL) {
      shErrStackPushPerror("phProfileTableHashGet: "
			   "Cannot open catalogue file %s for read\n",file);
      return(SH_GENERIC_ERROR);
   }

   if(fread(&head, sizeof(head), 1, fil) != 1) {
      shErrStackPush("phProfileTableHashGet: Cannot read header");
      fclose(fil);
      return(SH_GENERIC_ERROR);
   }
   fclose(fil);

   *crc = head.crc;

   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * Print the header of a model data file, either raw profiles or the
 * extracted cell arrays 
 */
int
phProfileTableHeaderPrint(const char *file, /* the file in question */
			  int is_celltable) /* file is a cellarray */
{
   CELLPROCATHEADER head;		/* the CELL file's header */
   FILE *fil;				/* the file's FILE */
   int i;
   spro_cheader s_rawhead;		/* the header if the file's raw */
   spro_cheader *rawhead;		/* pointer to raw part of header */
   spro_catentry *dev_entries;		/* DeVaucouleurs models. */
   spro_catentry *exp_entries;		/* Exponential models. */

   if((fil = fopen(file,"rb")) == NULL) {
      shErrStackPushPerror("phProfileTableHeaderPrint: "
			   "Cannot open catalogue file %s for read\n",file);
      return(SH_GENERIC_ERROR);
   }

   if(is_celltable) {
      if(fread(&head, sizeof(head), 1, fil) != 1) {
	 shErrStackPush("phProfileTableHeaderPrint: Cannot read header");
	 fclose(fil);
	 return(SH_GENERIC_ERROR);
      }

      if(strncmp(head.idstr,CELLPROF_VERSION, strlen(CELLPROF_VERSION)) == 0) {
	 printf("Id string: %s\n",head.idstr);
      } else {
	 head.idstr[IDSIZE - 1] = '\0';	/* may be binary... */
	 fprintf(stderr,"Header is corrupted or out of date. "
		 "Expected\n\t%s\n"
		 "Saw\n\t%s\n",CELLPROF_VERSION,head.idstr);
	 return(SH_GENERIC_ERROR);
      }
      printf("%d models\n",head.max_models);

      printf("Seeing models are N(0, sigma) and "
	     "N(0, sigma) + %.3f*N(0, %g*sigma)\n",
	     head.seeing_b, head.seeing_ratio);
      printf("sigma: %g ... %g (%g)\n",head.fseeing,
	     head.fseeing + (head.nseeing - 1)*head.dseeing,
	     head.dseeing);

      rawhead = &head.prof_cat;
   } else {
      if(fread(&s_rawhead, sizeof(s_rawhead), 1, fil) != 1) {
	 shErrStackPush("phProfileTableHeaderPrint: Cannot read raw header");
	 fclose(fil);
	 return(SH_GENERIC_ERROR);
      }
      rawhead = &s_rawhead;
   }

   i = p_phReadCatentries(rawhead, fil);
   shAssert(i == MAXSCAT);
      
   fclose(fil);
/*
 * check that the raw header looks right
 */
   if(strncmp(rawhead->proc_catver,RAWPROF_VERSION, strlen(RAWPROF_VERSION))
									== 0) {
      printf("Raw profiles: %s  ", rawhead->proc_catver);
      if(is_celltable) {
	 printf("(CRC: 0x%04x)\n", head.crc);
      } else {
	 printf("(CRC: not available)\n");
      }
   } else {
      rawhead->proc_catver[IDSIZE - 1] = '\0';	/* may be binary... */
      fprintf(stderr,"Raw header is corrupted or out of date. "
	      "Expected\n\t%s\n"
	      "Saw\n\t%s\n",RAWPROF_VERSION,rawhead->proc_catver);
      return(SH_GENERIC_ERROR);
   }
/*
 * The proc_catentry tables consist of:
 *    1 PSF
 *    Exponential models
 *    DeVaucoleurs models
 * The galaxy models are arranged with the size index increasing fast,
 * and thus the axis ratio index increasing slowly
 */
   exp_entries = rawhead->proc_catentry[1];
   dev_entries = rawhead->proc_catentry[1 +
				 rawhead->proc_nexpincl*rawhead->proc_nexpsiz];

   printf("Exponential disks (surface brightness is zero at %.2f):\n",
	  rawhead->cutoff[EXP_MODEL]);
   printf("%d sizes:\n",rawhead->proc_nexpsiz);
   for(i = 0;i < rawhead->proc_nexpsiz;i++) {
      printf("%10g%c",exp_entries[i].scat_reff,((i + 1)%7==0?'\n':' '));
   }
   if(i%7 != 0) printf("\n");

   printf("%d axis ratios:\n",rawhead->proc_nexpincl);
   for(i = 0;i < rawhead->proc_nexpincl;i++) {
      printf("%10.5f%c",exp_entries[i*rawhead->proc_nexpsiz].scat_axr,
	     ((i + 1)%7==0?'\n':' '));
   }
   if(i%7 != 0) printf("\n");

   printf("\nDeVaucouleurs profiles (surface brightness is zero at %.2f):\n",
	  rawhead->cutoff[DEV_MODEL]);
   printf("%d sizes:\n",rawhead->proc_ndevsiz);
   for(i = 0;i < rawhead->proc_ndevsiz;i++) {
      printf("%10g%c",dev_entries[i].scat_reff,((i + 1)%7==0?'\n':' '));
   }
   if(i%7 != 0) printf("\n");

   printf("%d axis ratios:\n",rawhead->proc_ndevincl);
   for(i = 0;i < rawhead->proc_ndevincl;i++) {
      printf("%10.5f%c",dev_entries[i*rawhead->proc_ndevsiz].scat_axr,
	     ((i + 1)%7==0?'\n':' '));
   }
   if(i%7 != 0) printf("\n");

   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * A debugging function to return an image of the difference between two
 * images. The returned region will coincide with reg1
 */
REGION *
phResidualsFind(const REGION *reg1,	/* one region */
		const REGION *reg2,	/* the one to be subracted */
		float drow, float dcol)	/* reg2's origin is at (drow, dcol) in
					   reg1 */
{
   int c,r;
   float *cbell, s_cbell[2*SINCSIZE - 1]; /* cosbell filter */
   float *coeffs_row, s_coeffs_row[2*SINCSIZE - 1]; /* row sinc coeffs */
   float *coeffs_col, s_coeffs_col[2*SINCSIZE - 1]; /* column sinc coeffs */
   int i;
   int idrow, idcol;			/* integral shift of reg2 wrt reg1 */
   int nrow, ncol;			/* size of difference region */
   REGION *out;				/* reg1 - reg2 */
   float *optr;				/* == out->rows_fl32[r] */
   PIX *r1ptr, *r2ptr;			/* == reg{1,2}->ROWS[r] */
   REGION *sreg2;			/* reg2 shifted by (drow,dcol),
					   expanded to the size of out */
   REGION *scr;				/* scratch region for convolution */

   shAssert(reg1 != NULL && reg1->type == TYPE_PIX);
   shAssert(reg2 != NULL && reg2->type == TYPE_PIX);
/*
 * find fractional offset of reg2
 */
   if(drow >= 0) {
      idrow = (int)drow;
   } else {
      idrow = -(1 + (int)(-drow));
   }
   if(dcol >= 0) {
      idcol = (int)dcol;
   } else {
      idcol = -(1 + (int)(-dcol));
   }
   drow -= idrow; dcol -= idcol;	/* d{row,col} are now in [0,1) */
   shAssert(drow >= 0 && drow < 1 && dcol >= 0 && dcol < 1);
/*
 * offset reg2 into sreg2
 */
   coeffs_row = s_coeffs_row + SINCSIZE - 1; /* so centre of filter is [0] */
   coeffs_col = s_coeffs_col + SINCSIZE - 1; /* so centre of filter is [0] */
   cbell = s_cbell + SINCSIZE - 1;	/* ditto for the cosbell */
   for(i = 0; i < SINCSIZE;i++) {
      cbell[-i] = cbell[i] = 0.5*(1 + cos(i*M_PI/SINCSIZE));
   }

   set_coeffs_f(drow,coeffs_row,cbell,SINCSIZE);
   set_coeffs_f(dcol,coeffs_col,cbell,SINCSIZE);

   sreg2 = shRegNew("sreg2",reg2->nrow,reg2->ncol,reg2->type);
   scr = shRegNew("scr",reg2->nrow,reg2->ncol,reg2->type);

   phConvolve(sreg2, reg2, scr, 2*SINCSIZE - 1, 2*SINCSIZE - 1,
	      &coeffs_col[1 - SINCSIZE], &coeffs_row[1 - SINCSIZE],
	      0, CONVOLVE_ANY);
/*
 * figure out the offset of the area the regions have in common, and
 * the offset of this region wrt the origin on reg1
 */
   nrow = reg1->nrow; ncol = reg1->ncol;
   out = shRegNew("residuals",nrow,ncol,TYPE_FL32);
   shRegClear(out);
/*
 * Actually do the subtraction
 */
   if(idrow > 0) {
      nrow -= idrow;
   }
   if(idcol > 0) {
      ncol -= idcol;
   }
   for(r = (idrow < 0) ? -idrow : 0;r < nrow;r++) {
      optr = out->rows_fl32[r];
      r1ptr = reg1->ROWS[r];
      r2ptr = &sreg2->ROWS[r + idrow][idcol];

      for(c = (idcol < 0) ? -idcol : 0;c < ncol;c++) {
	 optr[c] = r1ptr[c] - r2ptr[c];
      }
   }

   return(out);
}

/******************************************************************************/
 /*
  * Compare two raw profile files
  *
  * N.B. This routine is only for use while debugging problems!
  */
typedef struct {
    struct spro_cheader_T header;
    int nmodel;
    REGION **models;
} ALL_RAW_MODELS;

static ALL_RAW_MODELS *
read_all_raw_models(const char *file)	/* file to read */
{
    int i;
    ALL_RAW_MODELS *all = NULL;		/* data to return */

    if(phInitFitobj((char *)file) != SH_SUCCESS) {
	return NULL;
    }
 /*
  * Allocate space for all those models
  */
    all = shMalloc(sizeof(ALL_RAW_MODELS));
    all->header = pro_cheader;

    all->nmodel = 1 + pro_cheader.proc_nexpincl*pro_cheader.proc_nexpsiz +
	pro_cheader.proc_ndevincl*pro_cheader.proc_ndevsiz;
    shAssert(all->nmodel == pro_cheader.proc_maxscat);

    all->models = shMalloc(all->nmodel*sizeof(REGION **));
    memset(all->models, '\0', all->nmodel*sizeof(REGION *));
/*
 * Time to read them 
 */
    for (i = 0; i < all->nmodel; i++) {
	const int expand_model = 0;	/* don't expand model for smoothing */
	all->models[i] = read_model_index(i, expand_model);
	shHdrInsertAscii(&all->models[i]->hdr, "MODTYPE",
			 phFormatCatentry(pro_cheader.proc_catentry[i]),
			 "Description of model");
    }

    phFiniFitobj();

    return all;
}

static void
free_all_raw_models(ALL_RAW_MODELS *all)
{
    int i;
    
    if(all == NULL) return;

    for (i = 0; i < all->nmodel; i++) {
	shRegDel(all->models[i]);
    }
    shFree(all->models);
    
    shFree(all);
}


void
phCompareRawHeaders(const spro_cheader *hdr1, const spro_cheader *hdr2)
{
    int i;

    if(strcmp(hdr1->proc_catver, hdr2->proc_catver) != 0) {
	fprintf(stderr,"ID strings for raw catalogues differ:\n\"%s\"\n\"%s\"\n",
		hdr1->proc_catver, hdr2->proc_catver);
    }
    if(hdr1->proc_bufmax != hdr2->proc_bufmax) {
	fprintf(stderr,"maximum size of raw disk image differ: %d %d\n",
		hdr1->proc_bufmax, hdr2->proc_bufmax);
    }		
    if(hdr1->proc_linemax != hdr2->proc_linemax) {
	fprintf(stderr,"maximum length of raw line differ: %d %d\n",
		hdr1->proc_linemax, hdr2->proc_linemax);
    }		
    if(hdr1->proc_maxscat != hdr2->proc_maxscat) {
	fprintf(stderr,"number of raw entries differ: %d %d\n",
		hdr1->proc_maxscat, hdr2->proc_maxscat);
    }		
    if(hdr1->proc_nexpincl != hdr2->proc_nexpincl) {
	fprintf(stderr,"number of raw exp inclinations differ: %d %d\n",
		hdr1->proc_nexpincl, hdr2->proc_nexpincl);
    }		
    if(hdr1->proc_ndevincl != hdr2->proc_ndevincl) {
	fprintf(stderr,"number of raw deV inclinations differ: %d %d\n",
		hdr1->proc_ndevincl, hdr2->proc_ndevincl);
    }		
    if(hdr1->proc_nexpsiz != hdr2->proc_nexpsiz) {
	fprintf(stderr,"number of raw exp sizes differ: %d %d\n",
		hdr1->proc_nexpsiz, hdr2->proc_nexpsiz);
    }		
    if(hdr1->proc_ndevsiz != hdr2->proc_ndevsiz) {
	fprintf(stderr,"number of raw deV sizes differ: %d %d\n",
		hdr1->proc_ndevsiz, hdr2->proc_ndevsiz);
    }		
    if(hdr1->proc_pssig1 != hdr2->proc_pssig1) {
	fprintf(stderr,"raw little gaussian sigma differ: %g %g\n",
		hdr1->proc_pssig1, hdr2->proc_pssig1);
    }		
    if(hdr1->proc_pssig2 != hdr2->proc_pssig2) {
	fprintf(stderr,"raw big gaussian sigma differ: %g %g\n",
		hdr1->proc_pssig2, hdr2->proc_pssig2);
    }		
    if(hdr1->proc_psamp1 != hdr2->proc_psamp1) {
	fprintf(stderr,"raw ampl of little gaussian differ: %g %g\n",
		hdr1->proc_psamp1, hdr2->proc_psamp1);
    }		
    if(hdr1->proc_smmax != hdr2->proc_smmax) {
	fprintf(stderr,"raw smoothing fringe differ: %d %d\n",
		hdr1->proc_smmax, hdr2->proc_smmax);
    }		

    for (i = 0; i < NCLASSTYPE; i++) {
	if(hdr1->cutoff[i] != hdr2->cutoff[i]) {
	    fprintf(stderr,"%dth cutoffs for raw models differ: %g %g\n",
		    i, hdr1->cutoff[i], hdr2->cutoff[i]);
	}
    }
}

static int
compare_images(const REGION *reg1,	/* first region to compare */
	       const REGION *reg2,	/* second region to compare */
	       const float eps, /* largest allowable fractional difference */		    
	       const float feps) /* Maximum allowed fractional difference */
	       
{
    int cmp;
    int i, j;

    shAssert(reg1->type == TYPE_FL32 && reg2->type == TYPE_FL32);

    cmp = reg1->nrow - reg2->nrow;
    if (cmp != 0) return cmp;
    cmp = reg1->ncol - reg2->ncol;
    if (cmp != 0) return cmp;

    for (i = 0; i < reg1->nrow; i++) {
	for (j = 0; j < reg1->ncol; j++) {
	    FL32 v1 = reg1->rows_fl32[i][j];
	    FL32 v2 = reg2->rows_fl32[i][j];

	    if(fabs(v1 - v2) > eps && fabs(v1 - v2) > feps*fabs(v1 + v2)/2) {
		return 1;
	    }
	}
    }

    return 0;
}

/************************************************************************************************************/

int
phCompareRawProfileFiles(const char *file1, /* One file */
			 const char *file2, /* the other file */
			 int n0,	/* First model to compare (0 indexed) */
			 int n1,	/* Last model to compare (if -ve, counted from end) */
			 const float eps, /* largest allowable fractional difference */		    
			 const float feps, /* Maximum allowed fractional difference */
			 int normalize,	/* Normalise profiles to centre == 1 */
			 const char *dump_filename) /* format for output filenames */
{
    int err = 0;			/* number of errors */
    int first = 1;			/* This is the first model to be written */
    int i;
    ALL_RAW_MODELS *model1, *model2;	/* all the models in the file */
    char *dump_filename1 = NULL, *dump_filename2 = NULL;

    if (dump_filename != NULL) {
	dump_filename1 = alloca(strlen(dump_filename));	/* not + 1 as %d is 2 chars */
	dump_filename2 = alloca(strlen(dump_filename));	/* not + 1 as %d is 2 chars */

	sprintf(dump_filename1, dump_filename, 1); (void)unlink(dump_filename1);
	sprintf(dump_filename2, dump_filename, 2); (void)unlink(dump_filename2);
    }

    model1 = read_all_raw_models(file1);
    phFiniFitobj();
    model2 = read_all_raw_models(file2);
    phFiniFitobj();
/*
 * Process those models
 */
    if (n1 < 0) {
	n1 += model1->nmodel;
    }

    if (n0 >= model1->nmodel) {
	n0 = model1->nmodel - 1;
    }
    if (n1 >= model1->nmodel) {
	n1 = model1->nmodel - 1;
    }

    for (i = n0; i <= n1; i++) {
	if(compare_images(model1->models[i], model2->models[i], eps, feps) == 0) {
	    continue;
	}

	{
	    char descrip[HDRLINESIZE+1];
	    shHdrGetAscii(&model1->models[i]->hdr, "MODTYPE", descrip);
	    fprintf(stderr,"Model %4d %s differs\n", i, descrip);
	}

	if (dump_filename != NULL) {
	    if(first) {
		shHdrInsertLogical(&model1->models[i]->hdr, "EXTEND", 1, NULL);
		shHdrInsertLogical(&model2->models[i]->hdr, "EXTEND", 1, NULL);
	    }
	    
	    shHdrInsertInt(&model1->models[i]->hdr, "INDEX", i, "Model Index");
	    if(shRegWriteAsFits(model1->models[i], dump_filename1,
				(first ? STANDARD : IMAGE),
				2, DEF_NONE, NULL, 0) != SH_SUCCESS) {
		err++; break;
	    }
	    if(shRegWriteAsFits(model2->models[i], dump_filename2,	
				(first ? STANDARD : IMAGE),
				2, DEF_NONE, NULL, 0) != SH_SUCCESS) {
		err++; break;
	    }

	    first = 0;
	}
    }
/*
 * Clean up
 */
    free_all_raw_models(model1);
    free_all_raw_models(model2);

    return err;
}
