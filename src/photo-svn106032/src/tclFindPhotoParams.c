/*
 * TCL support for MAG type in photo.
 *
 * ENTRY POINT			SCOPE	DESCRIPTION
 * tclFindPhotoParamsDeclare	public	Declare all the verbs defined in 
 *					this module
 *
 * REQUIRED PRODUCTS:
 *	FTCL		TCL + XTCL + Fermilab extensions
 *	DERVISH		Survey Human Interface and Visualization
 *
 * Heidi Newberg
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "phUtils.h"
#include "phFindPhotoParams.h"

#define MAXLISTLENGTH 500
#define DEBUG 1
#define NODEBUG 0

static int
set_filternames(char *str, char ***names, int *ncolor);

static void
free_names(char **names, int numnames);

static int
get_floatarrayFromTclArray(Tcl_Interp *interp, char *arrayName, int N, float *vals);

static int
get_TclArrayFromFloatArray(Tcl_Interp *interp, char *arrayName, int N, float *vals);

static char *module = "phTclFindPhotoParams";    /* name of this set of code */

/***************************************************************************
 *  findPhotoParams
 */

#define RETURN(VAL) \
   if(pstamps != NULL) shFree(pstamps); \
   if(filternames != NULL) free_names(filternames,ncolors); \
   if(ref_filternames != NULL) free_names(ref_filternames,nrefcolors); \
   if(flux20_m != NULL) shFree(flux20_m); \
   if(flux20_c != NULL) shFree(flux20_c); \
   if(flux20_chi2 != NULL) shFree(flux20_chi2); \
   return(VAL)
 

/**********************************************************************
**<AUTO EXTRACT>
** TCL VERB: findPhotoParams
**
** Description:
**
** Determine the photometric calibration for each frame.
**
**</AUTO>
**********************************************************************/


static char *tclFindPhotoParams_use =
  "USAGE: findPhotoParams filterlist ref_filterlist ListOfStar1s ListOfPatches ListofExtinctions ListofFrameInfos oldCalib1bytime  starsep sys_ph_err max_Err n_max ListOfNewCalib1bytimes ListOfCalib1byFrames";

#define tclFindPhotoParams_hlp \
  "Find the photometric calibration for the patches and each input frame"

  /* these are all used to parse arguments to findPhotoParams TCL verb */
static char *fpp_filters, *fpp_ref_filters, *fpp_star1_str, *fpp_patches_str;
static char *fpp_extinction_str, *fpp_frameinfo_str, *fpp_oldcal_str;
static char *fpp_newcalib1bytime_str, *fpp_newcalib1byframe_str;
static char *fpp_colorterm_str, *fpp_colortermErr_str;
static double fpp_starsep;
static double fpp_sys_ph_err;
static double fpp_max_Err;
static int fpp_n_max;
static char *flux20_m_str = NULL;
static char *flux20_c_str = NULL;
static char *flux20_chi2_str = NULL;


static ftclArgvInfo tclFindPhotoParams_opts[] = {
   { NULL, FTCL_ARGV_HELP, NULL, NULL, tclFindPhotoParams_hlp },
   { "<filterlist>", FTCL_ARGV_STRING, NULL, &fpp_filters, 
          "filters for which to make photometric solution" },
   { "<ref_filterlist>", FTCL_ARGV_STRING, NULL, &fpp_ref_filters, 
          "list of reference filters, one for each element in 'filtlist'" },
   { "<color_terms>", FTCL_ARGV_STRING, NULL, &fpp_colorterm_str,
          "list of color terms, one for each element in 'filtlist'" },
   { "<color_term_errors>", FTCL_ARGV_STRING, NULL, &fpp_colortermErr_str,
         "list of errors for color terms, one for each element in 'filtlist'"},
   { "<star1s>", FTCL_ARGV_STRING, NULL, &fpp_star1_str, 
          "CHAIN of STAR1s detected in postage stamps" },
   { "<starcs>", FTCL_ARGV_STRING, NULL, &fpp_patches_str,
          "CHAIN of CHAINS of STARCs from MT pipeline patches" },
   { "<extinctions>", FTCL_ARGV_STRING, NULL, &fpp_extinction_str,
          "CHAIN of EXTINCTIONs from MT pipeline extinction files" },
   { "<frameinfos>", FTCL_ARGV_STRING, NULL, &fpp_frameinfo_str,
          "CHAIN of FRAMEINFOs" },
   { "<oldcal>", FTCL_ARGV_STRING, NULL, &fpp_oldcal_str,
          "old CALIB1BYTIME structure from previous run" },
   { "<starsep>", FTCL_ARGV_DOUBLE, NULL, &fpp_starsep,
          "max dist (arcsec) for matching postage stamps with standards" },
   { "<sys_ph_err>", FTCL_ARGV_DOUBLE, NULL, &fpp_sys_ph_err,
          "systematic photometric error (min. error for a patch)" },
   { "<max_Err>", FTCL_ARGV_DOUBLE, NULL, &fpp_max_Err,
          "max. photometric error for matched stars" },
   { "<n_max>", FTCL_ARGV_INT, NULL, &fpp_n_max,
         "max polyn. for flux20(t) fit " },
   { "<flux20_m>", FTCL_ARGV_STRING, NULL, &flux20_m_str,
         "slope for flux20(t)" },
   { "<flux20_c>", FTCL_ARGV_STRING, NULL, &flux20_c_str,
         "intercept for flux20(t)" },
   { "<flux20_chi2>", FTCL_ARGV_STRING, NULL, &flux20_chi2_str,
         "chi2 for the fit flux20(t)" },
   { "<newcalib1bytimes>", FTCL_ARGV_STRING, NULL, &fpp_newcalib1bytime_str,
          "CHAIN of CALIB1BYTIMEs into which output is placed" },
   { "<newcalib1byframes>", FTCL_ARGV_STRING, NULL, &fpp_newcalib1byframe_str,
          "CHAIN of CALIB1BYFRAMEs into which output is placed" },
   { NULL, FTCL_ARGV_END, NULL, NULL, NULL },
};



static int
tclFindPhotoParams(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
    int ncolors=0,nrefcolors=0; /* number of colors (& lists of pstamps) */
    CHAIN **pstamps;     /* list of STAR1s per color */
    CHAIN *patches;      /* list of STARCs */
    CHAIN *extinction;     /* list of EXTINCTIONs */
    CHAIN *frames;	  /* list of FRAMEINFOs (time, z) */
    CALIB1BYTIME *oldcal; /* last good cal; used if patches==NULL*/
    CHAIN *patchcal = NULL; /* output list of CALIB1BYTIMEs */
    CHAIN *framecal = NULL; /* output list of CALIB1BYFRAMEs */
    char filters[MAXLISTLENGTH], ref_filters[MAXLISTLENGTH];
    char **filternames=NULL, **ref_filternames=NULL;
    char result_str[300];
    char tmpstr1[20], tmpstr2[20];
    int i, flag1=0, flag2=0;
    RET_CODE result;
    float starsep, sys_ph_err, max_Err;
    int n_max;
    float colorterms[MAX_FILTERS], colortermErrs[MAX_FILTERS];
    int patchcal_flags[MAX_FILTERS];
    int framecal_flags[MAX_FILTERS];
    float *flux20_m = NULL;	
    float *flux20_c = NULL;	
    float *flux20_chi2 = NULL;	

    
    shErrStackClear();

    /* Initialize stuff */
    pstamps = NULL;
    patches = NULL;
    extinction = NULL;
    frames = NULL;
    oldcal = NULL;
    
    if (get_FtclOpts(interp, &argc, argv, tclFindPhotoParams_opts) != TCL_OK) {
        return(TCL_ERROR);
    }

    /* 
     * First arg is Tcl list of filters, 
     * which we convert to an array of strings
     */
    strncpy(filters, fpp_filters, MAXLISTLENGTH);
    
    if (set_filternames(filters, &filternames, &ncolors) != SH_SUCCESS) {
	Tcl_SetResult(interp, "can't get filternames", TCL_STATIC);
	RETURN(TCL_ERROR);
    }

    /* 
     * Second arg is Tcl list of reference filters,
     * which we also convert to an array of strings
     */
    strncpy(ref_filters, fpp_ref_filters, MAXLISTLENGTH);

    if (set_filternames(ref_filters, &ref_filternames, &nrefcolors) != 
	SH_SUCCESS) {
	Tcl_SetResult(interp, "can't get ref_filternames", TCL_STATIC);
	RETURN(TCL_ERROR);
    }

    /* 
     * Next arg is Tcl list of color terms,
     * which we convert to an array of floats
     */
    
    if ((i = sscanf(fpp_colorterm_str,"%f %f %f %f %f\n",&colorterms[0],
		    &colorterms[1],&colorterms[2],&colorterms[3],
		    &colorterms[4])) != ncolors) {

	Tcl_SetResult(interp, "can't read colorterms", TCL_STATIC);
	RETURN(TCL_ERROR);
    }

    /* 
     * Next arg is Tcl list of errors for color terms,
     * which we convert to an array of floats
     */
    
    if ((i = sscanf(fpp_colortermErr_str,"%f %f %f %f %f\n",&colortermErrs[0],
		    &colortermErrs[1],&colortermErrs[2],&colortermErrs[3],
		    &colortermErrs[4])) != ncolors) {

	Tcl_SetResult(interp, "can't read colortermErrs", TCL_STATIC);
	RETURN(TCL_ERROR);
    }



    /* Next arg is an array of chains of star1's one per color */
    if (phPtrArrayGetFromTclArrayOfHandles(interp, fpp_star1_str, filters,
					   "CHAIN", (void ***) &pstamps,
						     &ncolors) != SH_SUCCESS) {
	shErrStackPush("findPhotoParams: trouble getting pstamps");
	shFree(pstamps);
	shTclInterpAppendWithErrStack(interp);
	RETURN(TCL_ERROR);
    }  


    /* Next arg is a chain of patches, each of which is a chain
     * of starcs.
     */
    if (shTclAddrGetFromName(interp, fpp_patches_str, 
			     (void **) &patches, "CHAIN") != TCL_OK) {
	Tcl_SetResult(interp,"findPhotoParams: trouble getting patches",
		      TCL_STATIC);
	shFree(pstamps);
	RETURN(TCL_ERROR);
    }
 
  
    /* Next arg is a list of EXTINCTIONs */
    if (shTclAddrGetFromName(interp, fpp_extinction_str, 
			     (void **)&extinction, "CHAIN") != TCL_OK) {
	Tcl_SetResult(interp,"findPhotoParams: trouble getting extinction",
		      TCL_STATIC);
	RETURN(TCL_ERROR);
    }

    /* Next arg is a list of FRAMEINFOs*/
    if (shTclAddrGetFromName(interp, fpp_frameinfo_str,
			     (void **)&frames, "CHAIN") != TCL_OK) {
	Tcl_SetResult(interp, "findPhotoParams: trouble getting frames", 
		      TCL_STATIC);
	shFree(pstamps);
	RETURN(TCL_ERROR);
    }

    /* The next arg is a CALIB1BYTIME from last run */
    if (shTclAddrGetFromName(interp, fpp_oldcal_str, 
			     (void **) &oldcal, 
			     "CALIB1BYTIME") != TCL_OK) {
	Tcl_SetResult(interp, "findPhotoParams: trouble getting oldcal", 
		      TCL_STATIC);
	shFree(pstamps);
	RETURN(TCL_ERROR);
    }


    /* 
     * The next arg gives maximum allowable separation between a 
     * matched star1 and starc
     */
    starsep = (float) fpp_starsep;
    /* systematic photometric error (min. error for a patch) */
    sys_ph_err = (float) fpp_sys_ph_err;
    /* max. photometric error for matched stars */
    max_Err = (float) fpp_max_Err;
    n_max = fpp_n_max;
   
    flux20_m = shMalloc(ncolors*sizeof(float));
    if(get_floatarrayFromTclArray(interp, flux20_m_str, ncolors, flux20_m) < 1){
       shErrStackPush("findPhotoParams: can't get flux20_m");
       RETURN(TCL_ERROR);
    } 

    flux20_c = shMalloc(ncolors*sizeof(float));
    if(get_floatarrayFromTclArray(interp, flux20_c_str, ncolors, flux20_c) < 1){
       shErrStackPush("findPhotoParams: can't get flux20_c");
       RETURN(TCL_ERROR);
    } 

    flux20_chi2 = shMalloc(ncolors*sizeof(float));
    if(get_floatarrayFromTclArray(interp, flux20_chi2_str, ncolors, flux20_chi2) < 1){
       shErrStackPush("findPhotoParams: can't get flux20_chi2");
       RETURN(TCL_ERROR);
    } 

   
    /* The next arg is list of CALIB1BYTIMES for output  */
    if (shTclAddrGetFromName(interp, fpp_newcalib1bytime_str, 
			     (void **) &patchcal, "CHAIN") != TCL_OK) {
	Tcl_SetResult(interp, "findPhotoParams: trouble getting patchcal", 
		      TCL_STATIC);
	RETURN(TCL_ERROR);
    }

    /* The last arg is list of CALIB1FRAMES */
    if (shTclAddrGetFromName(interp, fpp_newcalib1byframe_str,
			     (void **) &framecal, "CHAIN") != TCL_OK) {
	Tcl_SetResult(interp, "findPhotoParams: trouble getting framecal", 
		      TCL_STATIC);
	RETURN(TCL_ERROR);
    }


    for (i = 0; i < ncolors; i++) {
	patchcal_flags[i] = patches->nElements;
        framecal_flags[i] = 1;
    }

    result = phFindPhotoParams(ncolors, filternames, ref_filternames, 
			       colorterms, colortermErrs, pstamps, patches, 
                               extinction, frames, oldcal, starsep, sys_ph_err,
			       max_Err, n_max, flux20_m, flux20_c, flux20_chi2,  
			       patchcal, framecal, patchcal_flags, framecal_flags);

    if (result != SH_SUCCESS) {
	Tcl_SetResult(interp, "phFindPhotoParams returns error", 
		      TCL_STATIC);
	RETURN(TCL_ERROR);
    }  else {
	*tmpstr1 = *tmpstr2 = '\0';
	for (i = 0; i < ncolors; i++) {
	    if (patchcal_flags[i]) {
		if (flag1) {
		    sprintf(tmpstr1,"%s %s", tmpstr1, filternames[i]);
		} else {
		    sprintf(tmpstr1,"%s", filternames[i]);		    
		    flag1 = 1;
		}
	    }
	    if (framecal_flags[i]) {
		if (flag2) {
		    sprintf(tmpstr2, "%s %s", tmpstr2, filternames[i]);
		} else {
		    sprintf(tmpstr2, "%s", filternames[i]);		    
		    flag2 = 1;
		}
	    }
	}
	
        if(get_TclArrayFromFloatArray(interp, flux20_m_str, ncolors, flux20_m) < 1){
             shErrStackPush("findPhotoParams: can't convert flux20_m");
             RETURN(TCL_ERROR);
        } 
        if(get_TclArrayFromFloatArray(interp, flux20_c_str, ncolors, flux20_c) < 1){
             shErrStackPush("findPhotoParams: can't convert flux20_c");
             RETURN(TCL_ERROR);
        } 
        if(get_TclArrayFromFloatArray(interp, flux20_chi2_str, ncolors, flux20_chi2) < 1){
             shErrStackPush("findPhotoParams: can't convert flux20_chi2");
             RETURN(TCL_ERROR);
        } 

	sprintf(result_str, "{patchcal {%s}} {framecal {%s}}", tmpstr1, tmpstr2);
	Tcl_SetResult(interp, result_str, TCL_STATIC);

	RETURN(TCL_OK);
    }
}
/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclFindPhotoParamsDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"findPhotoParams",
                (Tcl_CmdProc *)tclFindPhotoParams,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclFindPhotoParams_hlp, tclFindPhotoParams_use);
}


/**************************************************************************
 * given a string with a list of names "g r i", identify individual
 * names "g" "r" "i" and set the second arg to point to an array
 * of the individual names, each a null-terminated string.
 * place the number of names into the int argument 'ncolor' -- we'll
 * use it for a sanity check later on.
 * 
 * return SH_SUCCESS if all goes well, or SH_GENERIC ERROR if not.
 */

#define COLORMAX   20

static int
set_filternames(char *str, char ***names, int *ncolor)
{
   int i, in, num, pos[COLORMAX], len[COLORMAX];

   shDebug(10, "coming into set_filternames: str is ..%s..", str);

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
      shError("set_filternames: too many filter names, stopping after %d",
	    COLORMAX);
      num = COLORMAX;
   }
   shDebug(10, "set_filternames: count ..%d.. names", num);
   *ncolor = num;

   /* now allocate space for each one and copy the name into the
      new array */
   if ((*names = (char **) shMalloc(num*sizeof(char *))) == NULL) {
      shError("set_filternames: can't alloc for names array");
      return(SH_GENERIC_ERROR);
   }
   for (i = 0; i < num; i++) {
      if (((*names)[i] = (char *) shMalloc((len[i] + 1)*sizeof(char))) == NULL) {
	 shError("set_filternames: can't alloc for name %d", i);
	 return(SH_GENERIC_ERROR);
      }
      strncpy((*names)[i], str + pos[i], len[i]);
      (*names)[i][len[i]] = '\0';
      shDebug(10, "set_filternames: name %d is ..%s..", i, (*names)[i]);
   }

   return(SH_SUCCESS);
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
#define STRSIZE 10000
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
