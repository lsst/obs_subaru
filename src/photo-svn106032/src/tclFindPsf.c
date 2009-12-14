/*
 * TCL suppot for makeStarlist module
 *
 * ENTRY POINT			SCOPE	DESCRIPTION
 * tclFindPsfDeclare	public	Declare all the verbs defined in 
 *					this module
 *
 * REQUIRED PRODUCTS:
 *	FTCL		TCL + XTCL + Fermilab extensions
 *	DERVISH		Survey Human Interface and Visualization
 *
 * Nan Ellman
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "phUtils.h"
#include "phStar1.h"
#include "phFindPsf.h" 


#define MAXLISTLENGTH 500

static int
parse_names(const char *str, char ***array_ptr);

static void
free_names(char **names, int numnames);

static int
get_intarrayFromTclArray(Tcl_Interp *interp, char *arrayName, int N, int *vals);	 

static int
get_floatarrayFromTclArray(Tcl_Interp *interp, char *arrayName, int N, float *vals);

static int
get_TclArrayFromFloatArray(Tcl_Interp *interp, char *arrayName, int N, float *vals);

static char *module = "phFindPsf";    /* name of this set of code */

/*****************************************************************************/


#define RETURN2(VAL) \
   shFree(badstar); \
   shFree(oddness); \
   shFree(maxchi2); \
   return(VAL)

#define RETURN3(VAL) \
   if(frameinfolists != NULL) shFree(frameinfolists); \
   if(filternames != NULL) free_names(filternames,nfilters); \
   return(VAL) 

#define RETURN4(VAL) \
   if(frameinfolists != NULL) shFree(frameinfolists); \
   if(filternames != NULL) free_names(filternames,nfilters); \
   return(VAL)

#define RETURN5(VAL) \
   if(winglists != NULL) shFree(winglists); \
   if(wingmasks != NULL) shFree(wingmasks); \
   if(frameinfolists != NULL) shFree(frameinfolists); \
   if(filternames != NULL) free_names(filternames,nfilters); \
   return(VAL)


static char *starstr = NULL;
static char *wingstr = NULL;
static char *wingmaskstr = NULL;
static char *cstr = NULL;
static char *fistr = NULL;
static char *filterlist = NULL;
static char *psfKLstr = NULL;
static int w_opt = 1;
static int ifilter = 0;
static double nsigma_sigma = -1.0;	  
static double nsigma_QU = -1.0;	  
static double nsigma_RMS_rad = -1.0;	  
static int niter = -1;
static int min_starnum = -1;
static double err_min = 0.0;
static double err_max = 1.0;
static double ncellclip = 100.0;
static int frame = -1; 
static int first_star = -1; 
static int last_star = -1; 
static int nbadcell = 100; 
static double maxdev = 100.0;
static int nrow = 1361;
static int ncol = 2048;
static double gain = 1.0;
static double dark_var = 1.0;
static double sky = 0.0;
static double skyerr = 0.0;
static double q_min = 0.0;
static double q_max = 1.0;
static char *starliststr = NULL;
static int first = 0;
static int last = 0;
static char *badstarstr = NULL;
static int type = 1;
static int scaling_type = 1;
static char *cprofstr = NULL;
static char *oddnessstr = NULL;
static char *maxchi2str = NULL;




static char *tclMeasurePsf_use =
  "USAGE: measurePsf psfBasis winglists wingmasks calib1byframelist frameinfolists \
          gain dark_var filterlist filterindex sky skyerr err_min err_max frame \
          nrowperframe ncolperframe wing_options";
static char *tclMeasurePsf_hlp =
  "Determine the best psf for for each frame and add to calib1byframelist";

static ftclArgvInfo MeasurePsf_argTable[] = {
   { (char *) NULL, FTCL_ARGV_HELP, NULL, NULL,
         "Determine the best psf for each frame" },
   { "<psfBasis>", FTCL_ARGV_STRING, NULL, &psfKLstr,
         "PSF_BASIS for this frame" },
   { "<winglists>", FTCL_ARGV_STRING, NULL, &wingstr,
         "tcl array of winglists, 1 per color (or \"\" or \"NULL\")" },
   { "<wingmasks>", FTCL_ARGV_STRING, NULL, &wingmaskstr,
         "Chain to hold masks corresponding to wing stars" },
   { "<calib1byframelist>", FTCL_ARGV_STRING, NULL, &cstr,
         "list of calib1byframe structures, 1 per frame" },
   { "<frameinfolists>", FTCL_ARGV_STRING, NULL, &fistr,
         "tcl array of frameinfolists, 1 per color" },
   { "<gain>", FTCL_ARGV_DOUBLE, NULL, &gain,
         "gain for this color" },
   { "<dark_var>", FTCL_ARGV_DOUBLE, NULL, &dark_var,
         "dark variance for this color" },
   { "<filterlist>", FTCL_ARGV_STRING, NULL, &filterlist,
         "list of filters" },
   { "<filterindex>", FTCL_ARGV_INT, NULL, &ifilter,
         "index for the current filter" },
   { "<sky>", FTCL_ARGV_DOUBLE, NULL, &sky,
     "mean sky for this frame" },
   { "<skyerr>", FTCL_ARGV_DOUBLE, NULL, &skyerr,
     "mean skyerr for this frame" },
   { "<err_min>", FTCL_ARGV_DOUBLE, NULL, &err_min,
         "minimal error in composite profile (softening)" },
   { "<err_max>", FTCL_ARGV_DOUBLE, NULL, &err_max,
         "maximal error in composite profile (softening)" },
   { "<frame>", FTCL_ARGV_INT, NULL, &frame,
         "frame to analyze" },
   { "<nrow>", FTCL_ARGV_INT, NULL, &nrow,
         "number of pixel rows per frame" },
   { "<ncol>", FTCL_ARGV_INT, NULL, &ncol,
         "number of pixel cols per frame" },
   { "<wing_options>", FTCL_ARGV_INT, NULL, &w_opt,
         "flag telling what to do w/o wing stars" },
   { (char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL }
};



static int
tclMeasurePsf(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
		)
{
   const CHAIN **winglists=NULL; 
   const CHAIN **wingmasks=NULL; 
   PSF_BASIS *psfKLbasis = NULL;    
   CHAIN **frameinfolists=NULL; 
   CHAIN *cframes=NULL;
   char filters[MAXLISTLENGTH];
   char **filternames = NULL;
   int nfilters = 0;


  shErrStackClear();
  if (get_FtclOpts(interp, &argc, argv, MeasurePsf_argTable) != TCL_OK) {
      return(TCL_ERROR);
  }
  strncpy(filters,filterlist,MAXLISTLENGTH);
  nfilters = parse_names(filterlist, &filternames);

  if (shTclAddrGetFromName(interp, psfKLstr, (void **)&psfKLbasis, "PSF_BASIS") 
								   != TCL_OK) {
      shError("tclMeasurePsf: can't get psfKLbasis");
      shTclInterpAppendWithErrStack(interp);
      RETURN5 (TCL_ERROR);
  }

   if(*wingstr == '\0' || strcmp(wingstr, "NULL") == 0) {
      winglists = NULL;
   } else {
      if (phPtrArrayGetFromTclArrayOfHandles(interp, wingstr, filters, 
					     "CHAIN", (void ***)&winglists, 
					     &nfilters) != SH_SUCCESS) { 
	 shErrStackPush("MeasurePsf: can't get winglists");
	 shTclInterpAppendWithErrStack(interp);
	 RETURN5 (TCL_ERROR);
      }
   }

  if (phPtrArrayGetFromTclArrayOfHandles(interp, wingmaskstr, filters, 
					 "CHAIN", (void ***)&wingmasks, 
						    &nfilters) != SH_SUCCESS) {
      shErrStackPush("MeasurePsf: can't get wingmasks");
      RETURN5 (TCL_ERROR);
  } 

  if (shTclAddrGetFromName(interp, cstr, (void **)&cframes, "CHAIN") 
								   != TCL_OK) {
      shError("tclMeasurePsf: can't get calib1byframes");
      shTclInterpAppendWithErrStack(interp);
      RETURN5 (TCL_ERROR);
  }

  if (phPtrArrayGetFromTclArrayOfHandles(interp, fistr, filters, 
					 "CHAIN", (void ***)&frameinfolists, 
					 &nfilters) != SH_SUCCESS) {
     shErrStackPush("MeasurePsf: can't get frameinfolists");
     shTclInterpAppendWithErrStack(interp);
     RETURN5 (TCL_ERROR);
  } 


  /* do the work */   
  phMeasurePsf(psfKLbasis,winglists,wingmasks,cframes,frameinfolists,gain,dark_var,
               nfilters,ifilter,sky,skyerr, err_min,err_max,frame,nrow,ncol,w_opt);


   RETURN5 (TCL_OK);
 
}



static char *tclCheckCompiler_use =
  "USAGE: CheckCompiler";
static char *tclCheckCompiler_hlp = 
  "A little thingy to check compiler";

static int
tclCheckCompiler(
          ClientData clientData,
          int argc,
          char **argv
		)
{
    shErrStackClear();
    phCheckCompiler(10);

  return (TCL_OK);
 
}


static char *tclKLpsf2dgpsf_use =
  "USAGE: set psf [KLpsf2dgpsf psfBasis row col sky skyerr gain dark_var]";
static char *tclKLpsf2dgpsf_hlp = 
  "Determine local ap.corr. and fit2-Gaussian psf for a given PSF_BASIS at position (row,col)";

static ftclArgvInfo KLpsf2dgpsf_argTable[] = {
   { (char *) NULL, FTCL_ARGV_HELP, NULL, NULL,
         "Determine the best psf for each frame" },
   { "<psfBasis>", FTCL_ARGV_STRING, NULL, &psfKLstr,
         "PSF_BASIS for this frame" },
   { "<row>", FTCL_ARGV_INT, NULL, &nrow,
         "row position on frame" },
   { "<col>", FTCL_ARGV_INT, NULL, &ncol,
         "col position on frame" },
   { "<sky>", FTCL_ARGV_DOUBLE, NULL, &sky,
     "mean sky for this frame" },
   { "<skyerr>", FTCL_ARGV_DOUBLE, NULL, &skyerr,
     "mean skyerr for this frame" },
   { "<gain>", FTCL_ARGV_DOUBLE, NULL, &gain,
         "gain for this color" },
   { "<dark_var>", FTCL_ARGV_DOUBLE, NULL, &dark_var,
         "dark variance for this color" },
   { (char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL }
};



static int
tclKLpsf2dgpsf(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
		)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   PSF_BASIS *psfKLbasis = NULL; 
   DGPSF *dgpsf = NULL;   


   shErrStackClear();
   if (get_FtclOpts(interp, &argc, argv, KLpsf2dgpsf_argTable) != TCL_OK) {
      return(TCL_ERROR);
   }
   if (shTclAddrGetFromName(interp, psfKLstr, (void **)&psfKLbasis, "PSF_BASIS") 
								   != TCL_OK) {
      shError("tclKLpsf2dgpsf: can't get psfKLbasis");
      shTclInterpAppendWithErrStack(interp);
      return (TCL_ERROR);
   }

   /* do the work */   
   dgpsf = phKLpsf2dgpsf(psfKLbasis, nrow, ncol, sky, skyerr, gain, dark_var);

   if (dgpsf == NULL) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = dgpsf;
   hand.type = shTypeGetFromName("DGPSF");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"dgpsf\" to new DGPSF handle",
                                                                   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   
  return (TCL_OK);
}


static char *tclSelectPsfStars_use =
  "USAGE: selectPsfStars star1list calib1byframelist frameinfolists filterlist filterindex \
                         frame first_star last_star ncellclip nbadcell maxdev nsigma_sigma nsigma_QU nsigma_RMS_rad \
                         niter min_starnum";
static char *tclSelectPsfStars_hlp =
  "Select stars to be used in PSF determination";

static ftclArgvInfo SelectPsfStars_argTable[] = {
   { (char *) NULL, FTCL_ARGV_HELP, NULL, NULL,
         "Select psf stars for each frame" },
   { "<star1list>", FTCL_ARGV_STRING, NULL, &starstr,
         "list of STAR1s" },
   { "<calib1byframelist>", FTCL_ARGV_STRING, NULL, &cstr,
         "list of calib1byframe structures, 1 per frame" },
   { "<frameinfolists>", FTCL_ARGV_STRING, NULL, &fistr,
         "tcl array of frameinfolists, 1 per color" },
   { "<filterlist>", FTCL_ARGV_STRING, NULL, &filterlist,
         "list of filters" },
   { "<filterindex>", FTCL_ARGV_INT, NULL, &ifilter,
         "index for the current filter" },
   { "<frame>", FTCL_ARGV_INT, NULL, &frame,
         "frame" },
   { "<first_star>", FTCL_ARGV_INT, NULL, &first_star,
         "first star to analyze" },
   { "<last_star>", FTCL_ARGV_INT, NULL, &last_star,
         "last star to analyze" },
   { "<ncellclip>", FTCL_ARGV_DOUBLE, NULL, &ncellclip,
         "# of sigma for a cell to be bad" },
   { "<nbadcell>", FTCL_ARGV_INT, NULL, &nbadcell,
         "max. # of bad cells for a star to be good" },
   { "<maxdev>", FTCL_ARGV_DOUBLE, NULL, &maxdev,
         "max. dev. of a cell to condemn all" },
   { "<nsigma_sigma>", FTCL_ARGV_DOUBLE, NULL, &nsigma_sigma,
         "how many sigma to clip sigma at" },
   { "<nsigma_QU>", FTCL_ARGV_DOUBLE, NULL, &nsigma_QU,
         "how many sigma to clip QU at" },
   { "<nsigma_RMS_rad>", FTCL_ARGV_DOUBLE, NULL, &nsigma_RMS_rad,
         "how many sigma to clip RMS radius at" },
   { "<niter>", FTCL_ARGV_INT, NULL, &niter,
         "no. of times to iterate clipping" },
   { "<min_starnum>", FTCL_ARGV_INT, NULL, &min_starnum,
         "min # of stars for PSF fitting" },
   { (char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL }
};



static int
tclSelectPsfStars(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
		)
{
   CHAIN **frameinfolists=NULL; 
   CHAIN *cframes=NULL;
   CHAIN *starlist=NULL; 
   char filters[MAXLISTLENGTH];
   char **filternames = NULL;
   int nfilters = 0;


  shErrStackClear();
  if (get_FtclOpts(interp, &argc, argv, SelectPsfStars_argTable) != TCL_OK) {
      return(TCL_ERROR);
  }
  strncpy(filters,filterlist,MAXLISTLENGTH);
  nfilters = parse_names(filterlist, &filternames);

# if 0
  if (phPtrArrayGetFromTclArrayOfHandles(interp, starstr, filters, 
					 "CHAIN", (void ***)&starlists, 
					 &nfilters) != SH_SUCCESS) {
     shErrStackPush("SelectPsfStars: can't get starlists");
     shTclInterpAppendWithErrStack(interp);
     RETURN4 (TCL_ERROR);
  } 
#endif


  if (shTclAddrGetFromName(interp, cstr, (void **)&cframes, "CHAIN") 
								   != TCL_OK) {
      shError("tclSelectPsfStars: can't get calib1byframes");
      shTclInterpAppendWithErrStack(interp);
      RETURN4 (TCL_ERROR);
  }

  if (shTclAddrGetFromName(interp, starstr, (void **)&starlist, "CHAIN") 
								   != TCL_OK) {
      shError("tclSelectPsfStars: can't get starlist");
      shTclInterpAppendWithErrStack(interp);
      RETURN4 (TCL_ERROR);
  }



  if (phPtrArrayGetFromTclArrayOfHandles(interp, fistr, filters, 
					 "CHAIN", (void ***)&frameinfolists, 
					 &nfilters) != SH_SUCCESS) {
     shErrStackPush("SelectPsfStars: can't get frameinfolists");
     shTclInterpAppendWithErrStack(interp);
     RETURN4 (TCL_ERROR);
  } 


   /* do the work */   
   phSelectPsfStars(starlist, cframes, frameinfolists, nfilters, ifilter,
		    frame, first_star, last_star, ncellclip, nbadcell, maxdev,
		    nsigma_sigma, nsigma_QU, nsigma_RMS_rad, niter,
		    min_starnum);

   RETURN4(TCL_OK); 
}



static char *tclQAforPsfStars_use =
  "USAGE: QAforPsfStars star1list psfBasis calib1byframelist frameinfolists \
                        filterlist filterindex frame nrow ncol gain dark_var \
                        sky sky_err first_star last_star";

static char *tclQAforPsfStars_hlp = 
  "QA for stars used in PSF determination";

static ftclArgvInfo QAforPsfStars_argTable[] = {
   { (char *) NULL, FTCL_ARGV_HELP, NULL, NULL,
         "Select psf stars for each frame" },
   { "<star1list>", FTCL_ARGV_STRING, NULL, &starstr,
         "list of STAR1s" },
   { "<psfBasis>", FTCL_ARGV_STRING, NULL, &psfKLstr,
         "PSF_BASIS for this frame" },
   { "<calib1byframelist>", FTCL_ARGV_STRING, NULL, &cstr,
         "list of calib1byframe structures, 1 per frame" },
   { "<frameinfolists>", FTCL_ARGV_STRING, NULL, &fistr,
         "tcl array of frameinfolists, 1 per color" },
   { "<filterlist>", FTCL_ARGV_STRING, NULL, &filterlist,
         "list of filters" },
   { "<filterindex>", FTCL_ARGV_INT, NULL, &ifilter,
         "index for the current filter" },
   { "<frame>", FTCL_ARGV_INT, NULL, &frame,
         "frame" },
   { "<nrow>", FTCL_ARGV_INT, NULL, &nrow,
         "number of pixel rows per frame" },
   { "<ncol>", FTCL_ARGV_INT, NULL, &ncol,
         "number of pixel cols per frame" },
   { "<gain>", FTCL_ARGV_DOUBLE, NULL, &gain,
         "gain for this color" },
   { "<dark_var>", FTCL_ARGV_DOUBLE, NULL, &dark_var,
         "dark variance for this color" },
   { "<sky>", FTCL_ARGV_DOUBLE, NULL, &sky,
     "mean sky for this frame" },
   { "<skyerr>", FTCL_ARGV_DOUBLE, NULL, &skyerr,
     "mean skyerr for this frame" },
   { "<first_star>", FTCL_ARGV_INT, NULL, &first_star,
         "first star to analyze" },
   { "<last_star>", FTCL_ARGV_INT, NULL, &last_star,
         "last star to analyze" },
   { "<q_min>", FTCL_ARGV_DOUBLE, NULL, &q_min,
     "lower parameter for clipping (0-1)" },
   { "<q_max>", FTCL_ARGV_DOUBLE, NULL, &q_max,
     "upper parameter for clipping (0-1)" },
   { (char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL }
};



static int
tclQAforPsfStars(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
		)
{
   CHAIN *starlist=NULL; 
   PSF_BASIS *psfKLbasis = NULL;    
   CHAIN **frameinfolists=NULL; 
   CHAIN *cframes=NULL;
   char filters[MAXLISTLENGTH];
   char **filternames = NULL;
   int nfilters = 0;


  shErrStackClear();
  if (get_FtclOpts(interp, &argc, argv, QAforPsfStars_argTable) != TCL_OK) {
      return(TCL_ERROR);
  }
  strncpy(filters,filterlist,MAXLISTLENGTH);
  nfilters = parse_names(filterlist, &filternames);
 

  if (shTclAddrGetFromName(interp, psfKLstr, (void **)&psfKLbasis, "PSF_BASIS") 
								   != TCL_OK) {
      shError("tclQAforPsfStars: can't get psfKLbasis");
      shTclInterpAppendWithErrStack(interp);
      RETURN4 (TCL_ERROR);
  }

  if (shTclAddrGetFromName(interp, cstr, (void **)&cframes, "CHAIN") 
								   != TCL_OK) {
      shError("tclQAforPsfStars: can't get calib1byframes");
      shTclInterpAppendWithErrStack(interp);
      RETURN4 (TCL_ERROR);
  }

  if (shTclAddrGetFromName(interp, starstr, (void **)&starlist, "CHAIN") 
								   != TCL_OK) {
      shError("tclQAforPsfStars: can't get starlist");
      shTclInterpAppendWithErrStack(interp);
      RETURN4 (TCL_ERROR);
  }

  if (phPtrArrayGetFromTclArrayOfHandles(interp, fistr, filters, 
					 "CHAIN", (void ***)&frameinfolists, 
					 &nfilters) != SH_SUCCESS) {
     shErrStackPush("tclQAforPsfStars: can't get frameinfolists");
     shTclInterpAppendWithErrStack(interp);
     RETURN4 (TCL_ERROR);
  } 


   /* do the work */   
   phQAforPsfStars(starlist, psfKLbasis, cframes, frameinfolists, nfilters, ifilter, 
                   frame, nrow, ncol, gain, dark_var, sky, skyerr, first_star, 
                   last_star, q_min, q_max);

   RETURN4(TCL_OK); 
}






 
static char *tclGetCprof_use =
  "USAGE: getCprof starlist first last badstar type scaling_type cprof_bar oddness maxchi2";

static char *tclGetCprof_hlp = "Determine an average CELL_PROF for a list of STAR1s";

static ftclArgvInfo getCprof_argTable[] = {
   { (char *) NULL, FTCL_ARGV_HELP, NULL, NULL,
         "Determine an average CELL_PROF" },
   { "<starlist>", FTCL_ARGV_STRING, NULL, &starliststr,
         "chain of star1s" },
   { "<first>", FTCL_ARGV_INT, NULL, &first,
         "first star to include in starlist" },
   { "<last>", FTCL_ARGV_INT, NULL, &last,
         "last star to include in starlist" },
   { "<badstar>", FTCL_ARGV_STRING, NULL, &badstarstr,
         "only stars with badstar=0 will be included (index: 0 to last-first)" },
   { "<type>", FTCL_ARGV_INT, NULL, &type,
         "type of average profile (1:mean, 2-4: median, 5: weighted average)" },
   { "<scaling_type>", FTCL_ARGV_INT, NULL, &scaling_type,
         "type of normalization(1: pixel 0, 2: pixels 0-6, 3: aperture flux)" },
   { "<cprof_bar>", FTCL_ARGV_STRING, NULL, &cprofstr,
         "CELL_PROF to hold average profile" },
   { "<oddness>", FTCL_ARGV_STRING, NULL, &oddnessstr,
         "measures deviation from the average profile" },
   { "<maxchi2>", FTCL_ARGV_STRING, NULL, &maxchi2str,
         "maximum (cell) deviation from the average profile" },
   { (char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL }
};

static int
tclGetCprof(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
		)
{
   const CHAIN *starlist=NULL; 
   CELL_PROF *cprof_bar = NULL;
   int *badstar = NULL;			 
   float *oddness = NULL;	
   float *maxchi2 = NULL;	
   int Nel; /* number of elements in badstar, oddnes and maxchi2 */
 
  shErrStackClear();
  if (get_FtclOpts(interp, &argc, argv, getCprof_argTable) != TCL_OK) {
      RETURN2 (TCL_ERROR);
  }

  if (shTclAddrGetFromName(interp, starliststr, (void **)&starlist, "CHAIN") 
      != TCL_OK) {
    shError("tclFindPsf: can't get starlist");
    RETURN2 (TCL_ERROR);
  } 

  Nel = last - first + 1;
  badstar = shMalloc(Nel*sizeof(int));
  if(get_intarrayFromTclArray(interp, badstarstr, Nel, badstar) < 1){
      shErrStackPush("tclGetCprof: can't get badstar array");
      RETURN2 (TCL_ERROR);
  }  

  oddness = shMalloc(Nel*sizeof(float));
  if(get_floatarrayFromTclArray(interp, oddnessstr, Nel, oddness) < 1){
      shErrStackPush("tclGetCprof: can't get oddness array");
      RETURN2 (TCL_ERROR);
  } 

  maxchi2 = shMalloc(Nel*sizeof(float));
  if(get_floatarrayFromTclArray(interp, maxchi2str, Nel, maxchi2) < 1){
      shErrStackPush("tclGetCprof: can't get maxchi2 array");
      RETURN2 (TCL_ERROR);
  } 
 
  if (shTclAddrGetFromName(interp, cprofstr, (void *)&cprof_bar, "CELL_PROF") 
	!= TCL_OK) {
	shError("tclGetCprof: can't get cprof_bar");
	RETURN2 (TCL_ERROR);
  } 
  memset(cprof_bar->data,'\0',85*sizeof(float));
  memset(cprof_bar->sig,'\0',85*sizeof(float));
/*
 * do the work
 */
  if (phGetCprof(starlist, first, last, badstar, type, scaling_type, cprof_bar, oddness, maxchi2) != SH_SUCCESS) {
      Tcl_SetResult(interp, "phGetCprof returns error",TCL_STATIC);
      RETURN2 (TCL_ERROR);
  }

  if(get_TclArrayFromFloatArray(interp, oddnessstr, Nel, oddness) < 1){
      shErrStackPush("tclGetCprof: can't convert oddness array");
      RETURN2 (TCL_ERROR);
  } 
  if(get_TclArrayFromFloatArray(interp, maxchi2str, Nel, maxchi2) < 1){
      shErrStackPush("tclGetCprof: can't convert maxchi2 array");
      RETURN2 (TCL_ERROR);
  } 


 
  RETURN2 (TCL_OK);
}



/*****************************************************************************/
/*
 * given a string with a set of filter names, separated by spaces:
 *           "u g r i z"
 *
 * parse the string, and create a new, null-terminated array for 
 * each of the filter names.  Set the second argument to point
 * to the array of names, and return the number
 */

#define COLORMAX    20         /* max number of filter names allowed */

static int
parse_names(const char *str,		/* string with names separated by
					   spaces */
	    char ***array_ptr)		/* O: set this to point to base of
					   array of names */
{
   int i, in, num, pos[COLORMAX], len[COLORMAX];

   /* count the number of names */
   num = 0;
   in = 0;
   for (i = 0; i < strlen(str); i++) {
      if (isalnum(str[i])) {
         if (in == 0) {
            in = 1;
            if (num < COLORMAX) {
               pos[num] = i;
            }
            num++;
         }
      }
      else {
         if (in == 1) {
            in = 0;
            if (num < COLORMAX) {
               len[num - 1] = i - pos[num - 1];
            }
         }
      }
   }
   if (in == 1) {
      if (num < COLORMAX) {
         len[num - 1] = i - pos[num - 1];
      }
   }
   if (num > COLORMAX) {
      shError("parse_names: too many filter names, stopping after %d",
            COLORMAX);
      num = COLORMAX;
   }

   /* now allocate space for each one and copy the name into the
      new array */
   *array_ptr = shMalloc(num*sizeof(char *));
   for (i = 0; i < num; i++) {
      (*array_ptr)[i] = shMalloc((len[i] + 1)*sizeof(char));
      strncpy((*array_ptr)[i], str + pos[i], len[i]);
      (*array_ptr)[i][len[i]] = '\0';
   }

   return(num);
}

/**********************************************************************
 * free up the filternames arrays
 */

static void
free_names(char **names, int numnames)
{
   int i;

   if (names == NULL) 
      return;

   for (i = 0; i < numnames; i++) {
      if (names[i] != NULL) 
         shFree(names[i]);
   }
   shFree(names);
}

/***************************************************************************** 
 * Given the name of a TCL array and number of elements, generate an array of 
 * ints corresponding to its elements, and return the number of elements. 
 */
int
get_intarrayFromTclArray(Tcl_Interp *interp,
			 char *arrayName,    /* name of tcl array */
			 int N,              /* number of elements in tcl array */
			 int *vals)	     /* array to fill */
{
   int i;
   char *ptr;
#define STRSIZE 10000
   char av[STRSIZE + 1];

   for (i = 0; i < N; i++) {
       sprintf(av, "%d", i);
       shAssert(strlen(av) < STRSIZE);   
       ptr = Tcl_GetVar2(interp, arrayName, av, 0);
       vals[i] = atoi(ptr);
   }
    
   return(i);
}

/*****************************************************************************
 * Given the name of a TCL array and number of elements, generate an array of 
 * floats corresponding to its elements, and return the number of elements.
 */
int
get_floatarrayFromTclArray(Tcl_Interp *interp,
			 char *arrayName,    /* name of tcl array */
			 int N,              /* number of elements in tcl array */
			 float *vals)	     /* array to fill */
{
   int i;
   char *ptr;
   char av[STRSIZE + 1];

   for (i = 0; i < N; i++) {
       sprintf(av, "%d", i);
       shAssert(strlen(av) < STRSIZE);   
       ptr = Tcl_GetVar2(interp, arrayName, av, 0);
       vals[i] = atof(ptr);
   }
 
   return(i);
}



/*****************************************************************************
 * Given an array of floats, name of a TCL array and number of elements, 
 * copy floats to TCL array, and return the number of elements.
 */
int
get_TclArrayFromFloatArray(Tcl_Interp *interp,
			 char *arrayName,    /* name of tcl array */
			 int N,              /* number of elements in tcl array */
			 float *vals)	     /* array to fill */
{
   int i;
   char *ptr;
   char av[STRSIZE + 1];

   for (i = 0; i < N; i++) {
       sprintf(av, "%d", i);
       shAssert(strlen(av) < STRSIZE);   
       ptr = Tcl_GetVar2(interp, arrayName, av, 0);
       sprintf(ptr,"%f",vals[i]);
   }
 
   return(i);
}

/*****************************************************************************/

static char *tclSetBadstarFlagFromChain_use =
  "USAGE: setBadstarFlagFromChain <star1clist> <frameinfolist> <filternames> -nsigma_sigma dval -nsigma_QU dval -nsigma_RMS_rad dval -ncellclip dval -nbadcell ival -maxdev dval";
#define tclSetBadstarFlagFromChain_hlp \
  "Set the STAR1.badstar values in all objects on a chain, using parameters\n"\
  "determined by selectPsfStars.  The cellclipping parameters are ignored"

static ftclArgvInfo setBadstarFlagFromChain_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclSetBadstarFlagFromChain_hlp},
   {"<star1clist>", FTCL_ARGV_STRING, NULL, NULL, "Chain of STAR1Cs"},
   {"<frameinfolist>", FTCL_ARGV_STRING, NULL, NULL,
			      "Name of TCL array of FRAME_INFORMATION chains"},
   {"<filters>", FTCL_ARGV_STRING, NULL, NULL,
			      "Names of elements of frameinfolist"},
   {"-nsigma_sigma", FTCL_ARGV_DOUBLE, NULL, NULL,
				     "Number of s.d. to clip sigma[12] and b"},
   {"-nsigma_QU", FTCL_ARGV_DOUBLE, NULL, NULL, "Number of s.d. to clip U/Q"},
   {"-nsigma_RMS_rad", FTCL_ARGV_DOUBLE, NULL, NULL,
					  "Number of s.d. to clip RMS radius"},
   {"-ncellclip", FTCL_ARGV_DOUBLE, NULL, NULL,
					"Number of s.d. for a cell to be bad"},
   {"-nbadcell", FTCL_ARGV_INT, NULL, NULL,
				     "Max number of bad cells in a good star"},
   {"-maxdev", FTCL_ARGV_DOUBLE, NULL, NULL, 
			       "Maximum deviation of one cell to condemn all"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define setBadstarFlagFromChain_name "setBadstarFlagFromChain"

static int
tclSetBadstarFlagFromChain(ClientData clientData,
			   Tcl_Interp *interp,
			   int ac,
			   char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *star1clistStr = NULL;		/* Chain of STAR1Cs */
   CHAIN *star1clist;
   char *frameinfolistStr = NULL;	/* TCL arr of FRAME_INFORMATION chain*/
   CHAIN **frameinfolist=NULL;		/* Corresponding C array */
   char *filtersStr = NULL;		/* Element names for frameinfolist */
   double nsigma_sigma = 100;		/* No. of s.d. to clip sigma[12]/b */
   double nsigma_QU = 100.0;		/* No. of s.d. to clip U/Q */
   double nsigma_RMS_rad = 100.0;	/* No. of s.d. to clip RMS radius */
   double ncellclip = 100.0;		/* No. of s.d. for a cell to be bad */
   int nbadcell = 1000;			/* Max no of bad cells in a good star*/
   double maxdev = 100.0;		/* Max dev of one cell to condemn all*/
   char filters[MAXLISTLENGTH];		/* static copy of filtersStr */
   char **filternames = NULL;		/* split version of filters  */
   int nfilters;			/* number of filter names provided */

   shErrStackClear();

   a_i = 1;
   setBadstarFlagFromChain_opts[a_i++].dst = &star1clistStr;
   setBadstarFlagFromChain_opts[a_i++].dst = &frameinfolistStr;
   setBadstarFlagFromChain_opts[a_i++].dst = &filtersStr;
   setBadstarFlagFromChain_opts[a_i++].dst = &nsigma_sigma;
   setBadstarFlagFromChain_opts[a_i++].dst = &nsigma_QU;
   setBadstarFlagFromChain_opts[a_i++].dst = &nsigma_RMS_rad;
   setBadstarFlagFromChain_opts[a_i++].dst = &ncellclip;
   setBadstarFlagFromChain_opts[a_i++].dst = &nbadcell;
   setBadstarFlagFromChain_opts[a_i++].dst = &maxdev;
   shAssert(setBadstarFlagFromChain_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, setBadstarFlagFromChain_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     setBadstarFlagFromChain_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * Deal with arguments.  Memory will leak in case of error, but who cares?
 */
   strncpy(filters, filtersStr, MAXLISTLENGTH);
   nfilters = parse_names(filters, &filternames);

   if(shTclHandleExprEval(interp,star1clistStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_AppendResult(interp,"setBadstarFlagFromChain: "
                       "argument \"", star1clistStr, "\" is not a CHAIN",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   star1clist = hand.ptr;

   if(star1clist->type != shTypeGetFromName("STAR1C")) {
      Tcl_AppendResult(interp,
                       "CHAIN \"",star1clistStr, "\" doesn't have type STAR1C",
								 (char *)NULL);
      return(TCL_ERROR);
   }

   if(phPtrArrayGetFromTclArrayOfHandles(interp, frameinfolistStr, filters, 
					 "CHAIN", (void ***)&frameinfolist,
					 &nfilters) != SH_SUCCESS) { 
      shErrStackPush("setBadstarFlagFromChain: can't get frameinfolist");
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   if(frameinfolist[0]->type != shTypeGetFromName("FRAME_INFORMATION")) {
      Tcl_AppendResult(interp, "CHAIN \"", frameinfolistStr, "<0>"
		       "\" doesn't have type FRAME_INFORMATION", (char *)NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phSetBadstarFlagFromChain(star1clist, (const CHAIN *const *)frameinfolist,
			     ncellclip, nbadcell, maxdev,	
			     nsigma_sigma, nsigma_QU, nsigma_RMS_rad);
/*
 * Cleanup
 */
   shFree(frameinfolist);
   free_names(filternames,nfilters);

   return(TCL_OK);
}


/*****************************************************************
 * Declare my new tcl verbs to tcl
 */
void
phTclFindPsfDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"CheckCompiler",
                (Tcl_CmdProc *)tclCheckCompiler,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclCheckCompiler_hlp, tclCheckCompiler_use);

   shTclDeclare(interp,"KLpsf2dgpsf",
                (Tcl_CmdProc *)tclKLpsf2dgpsf,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclKLpsf2dgpsf_hlp, tclKLpsf2dgpsf_use);

   shTclDeclare(interp,"measurePsf",
                (Tcl_CmdProc *)tclMeasurePsf,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclMeasurePsf_hlp, tclMeasurePsf_use);

   shTclDeclare(interp,"selectPsfStars",
                (Tcl_CmdProc *)tclSelectPsfStars,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclSelectPsfStars_hlp, tclSelectPsfStars_use);

   shTclDeclare(interp,"QAforPsfStars",
                (Tcl_CmdProc *)tclQAforPsfStars,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclQAforPsfStars_hlp, tclQAforPsfStars_use);

   shTclDeclare(interp,"getCprof",
                (Tcl_CmdProc *)tclGetCprof,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclGetCprof_hlp, tclGetCprof_use);

   shTclDeclare(interp,setBadstarFlagFromChain_name,
		(Tcl_CmdProc *)tclSetBadstarFlagFromChain, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclSetBadstarFlagFromChain_hlp,
		tclSetBadstarFlagFromChain_use);
}
 

