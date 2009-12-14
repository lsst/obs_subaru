/*
 * TCL interface for Fitobj functions.
 */

#include "dervish.h"
#include "phUtils.h"
#include "phObjects.h"
#include "phFitobj.h"
#include "phCellFitobj.h"
#include "phExtract.h"

static char *module = "tclFitobj";

static char *tclMakeProfCat_use = "USAGE: makeProfCat <file> [-show_progress]";
#define tclMakeProfCat_hlp "Create a Profile Catalogue in a disk file <file>."

static ftclArgvInfo tclMakeProfCat_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclMakeProfCat_hlp},
   {"<file>", FTCL_ARGV_STRING, NULL, NULL, "Name of catalogue file"},
   {"-show_progress", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Print information to stderr as catalog construction proceeds"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclMakeProfCat(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
   int i;
   char *fileStr = NULL;		/* Name of catalogue file */
   int show_progress = 0;		/* Print info about construction */

   shErrStackClear();

   i = 1;
   tclMakeProfCat_opts[i++].dst = &fileStr;
   tclMakeProfCat_opts[i++].dst = &show_progress;
   shAssert(tclMakeProfCat_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp, &argc, argv, tclMakeProfCat_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * Do the work.
 */
   if(phProfCatalogMake(fileStr,show_progress) != SH_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclInitFitobj_use = "USAGE: initFitobj <file>";
#define tclInitFitobj_hlp "Initialize the fitobj routines, and read a "\
"profile catalogue from <file>."

static ftclArgvInfo tclInitFitobj_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclInitFitobj_hlp},
   {"<file>", FTCL_ARGV_STRING, NULL, NULL, "Profile catalogue to read"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclInitFitobj(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
   int i;
   char *fileStr = NULL;		/* Profile catalogue to read */

   shErrStackClear();

   i = 1;
   tclInitFitobj_opts[i++].dst = &fileStr;
   shAssert(tclInitFitobj_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp, &argc, argv, tclInitFitobj_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * Do the work.
 */
   if(phInitFitobj(fileStr) != SH_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclFiniFitobj_use =
  "USAGE: finiFitobj ";
#define tclFiniFitobj_hlp \
  "Free memory allocated by initFitobj"

static ftclArgvInfo finiFitobj_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFiniFitobj_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFiniFitobj(
	      ClientData clientDatag,
	      Tcl_Interp *interp,
	      int ac,
	      char **av
	      )
{
   shErrStackClear();

   if(get_FtclOpts(interp,&ac,av,finiFitobj_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * work
 */
   phFiniFitobj();
   
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * make models --- note that the nearest model is returned, and that no
 * interpolation is done
 *
 * Models are PSFs, DeVs, and Exponentials
 */
static char *tclMakePSFModel_use = "USAGE: makePSFModel totflux seeing -max -exact";
#define tclMakePSFModel_hlp "Create a seeing convolved de Vaucouleurs model."

static ftclArgvInfo tclMakePSFModel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclMakePSFModel_hlp},
   {"<totflux>", FTCL_ARGV_DOUBLE, NULL, NULL, "total flux in model"},
   {"<seeing>", FTCL_ARGV_STRING, NULL, NULL, "PSF"},
   {"-max", FTCL_ARGV_STRING, NULL, NULL,
			       "Name of variable to return the max intensity"},
   {"-exact", FTCL_ARGV_CONSTANT, (void *)1, NULL,
	       "Calculate model, rather then interpolating in profile tables"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclMakePSFModel(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
    int i;
    char name[HANDLE_NAMELEN];
    HANDLE hand;
    int max = 0;			/* maximum value in model */
    MODEL_PARAMS p;
    REGION *reg;
    char str[30];			/* return value of max, if desired */
    void *vptr;				/* used by shTclHandleExprEval */
    double totflux = 0.0;		/* total flux in model */
    char *seeingStr = NULL;		/* PSF */
    char *maxStr = NULL;		/* Name of variable to return max */
    int exact = 0;			/* make exact model? */

    shErrStackClear();

    i = 1;
    tclMakePSFModel_opts[i++].dst = &totflux;
    tclMakePSFModel_opts[i++].dst = &seeingStr;
    tclMakePSFModel_opts[i++].dst = &maxStr;
    tclMakePSFModel_opts[i++].dst = &exact;
    shAssert(tclMakePSFModel_opts[i].type == FTCL_ARGV_END);

    if(get_FtclOpts(interp, &argc, argv, tclMakePSFModel_opts) != TCL_OK) {

       return(TCL_ERROR);
    }
/*
 * process arguments
 */
    p.class = PSF_MODEL;
    p.exact = exact;
    p.rsize = 0;
    p.aratio = 1;
    p.orient = 0;
    if(shTclHandleExprEval(interp,seeingStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
    }
    if(hand.type != shTypeGetFromName("DGPSF")) {
	 Tcl_SetResult(interp,"makePSFModel: "
		       "second argument is not a DGPSF",TCL_STATIC);
	 return(TCL_ERROR);
    }
    p.psf = hand.ptr;
/*
 * Not too hard.
 */
    reg = phAnyModelMakeScaled(&p, totflux, &max);
/*
 * and return the desired answers
 */
   if(maxStr != NULL) {
      sprintf(str,"%d",max);
      if(Tcl_SetVar(interp,maxStr,str,TCL_LEAVE_ERR_MSG) == NULL) {
	 Tcl_SetResult(interp,"Failed to return max",TCL_VOLATILE);
	 return(TCL_ERROR);
      }
   } 

   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = reg;
   hand.type = shTypeGetFromName("REGION");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new REGION handle",TCL_STATIC);
      shRegDel(reg);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

static char *tclMakeDevModel_use = "USAGE: makeDevModel totflux size axrat orient seeing -max -exact";
#define tclMakeDevModel_hlp "Create a seeing convolved de Vaucouleurs model."

static ftclArgvInfo tclMakeDevModel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclMakeDevModel_hlp},
   {"<totflux>", FTCL_ARGV_DOUBLE, NULL, NULL, "total flux in model"},
   {"<size>", FTCL_ARGV_DOUBLE, NULL, NULL, "radial size"},
   {"<axrat>", FTCL_ARGV_DOUBLE, NULL, NULL, "axis ratio"},
   {"<orient>", FTCL_ARGV_DOUBLE, NULL, NULL, "orientation"},
   {"<seeing>", FTCL_ARGV_STRING, NULL, NULL, "PSF"},
   {"-max", FTCL_ARGV_STRING, NULL, NULL,
			       "Name of variable to return the max intensity"},
   {"-exact", FTCL_ARGV_CONSTANT, (void *)1, NULL,
	       "Calculate model, rather then interpolating in profile tables"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclMakeDevModel(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
    int i;
    char name[HANDLE_NAMELEN];
    HANDLE hand;
    int max = 0;			/* maximum value in model */
    MODEL_PARAMS p;
    REGION *reg;
    char str[30];			/* return value of max, if desired */
    void *vptr;				/* used by shTclHandleExprEval */
    double totflux = 0.0;		/* total flux in model */
    double size = 0.0;			/* radial size */
    double axrat = 0.0;			/* axis ratio */
    double orient = 0.0;		/* orientation */
    char *seeingStr = NULL;		/* PSF */
    char *maxStr = NULL;		/* Name of variable to return max */
    int exact = 0;			/* make exact model? */
    
    shErrStackClear();

    i = 1;
    tclMakeDevModel_opts[i++].dst = &totflux;
    tclMakeDevModel_opts[i++].dst = &size;
    tclMakeDevModel_opts[i++].dst = &axrat;
    tclMakeDevModel_opts[i++].dst = &orient;
    tclMakeDevModel_opts[i++].dst = &seeingStr;
    tclMakeDevModel_opts[i++].dst = &maxStr;
    tclMakeDevModel_opts[i++].dst = &exact;
    shAssert(tclMakeDevModel_opts[i].type == FTCL_ARGV_END);
    
    if(get_FtclOpts(interp, &argc, argv, tclMakeDevModel_opts) != TCL_OK) {

       return(TCL_ERROR);
    }
/*
 * process arguments
 */
    p.class = DEV_MODEL;
    p.exact = exact;
    p.rsize = size;
    p.aratio = axrat;
    p.orient = orient;
    if(shTclHandleExprEval(interp,seeingStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
    }
    if(hand.type != shTypeGetFromName("DGPSF")) {
	 Tcl_SetResult(interp,"makeDevModel: "
		       "fifth argument is not a DGPSF",TCL_STATIC);
	 return(TCL_ERROR);
    }
    p.psf = hand.ptr;
/*
 * Not too hard.
 */
    reg = phAnyModelMakeScaled(&p, totflux, &max);
/*
 * and return the desired answers
 */
   if(maxStr != NULL) {
      sprintf(str,"%d",max);
      if(Tcl_SetVar(interp,maxStr,str,TCL_LEAVE_ERR_MSG) == NULL) {
	 Tcl_SetResult(interp,"Failed to return max",TCL_VOLATILE);
	 return(TCL_ERROR);
      }
   } 

   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = reg;
   hand.type = shTypeGetFromName("REGION");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new REGION handle",TCL_STATIC);
      shRegDel(reg);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

static char *tclMakeExpModel_use = "USAGE: makeExpModel totflux size axrat orient seeing -max -exact";
#define tclMakeExpModel_hlp "Create a seeing convolved exponential disk model."

static ftclArgvInfo tclMakeExpModel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclMakeExpModel_hlp},
   {"<totflux>", FTCL_ARGV_DOUBLE, NULL, NULL, "total flux in model"},
   {"<size>", FTCL_ARGV_DOUBLE, NULL, NULL, "radial size"},
   {"<axrat>", FTCL_ARGV_DOUBLE, NULL, NULL, "axis ratio"},
   {"<orient>", FTCL_ARGV_DOUBLE, NULL, NULL, "orientation"},
   {"<seeing>", FTCL_ARGV_STRING, NULL, NULL, "PSF"},
   {"-max", FTCL_ARGV_STRING, NULL, NULL,
			       "Name of variable to return the max intensity"},
   {"-exact", FTCL_ARGV_CONSTANT, (void *)1, NULL,
	       "Calculate model, rather then interpolating in profile tables"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclMakeExpModel(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
    int i;
    char name[HANDLE_NAMELEN];
    HANDLE hand;
    int max = 0;			/* maximum value in model */
    MODEL_PARAMS p;
    REGION *reg;
    char str[30];			/* return value of max, if desired */
    void *vptr;				/* used by shTclHandleExprEval */
    double totflux = 0.0;		/* total flux in model */
    double size = 0.0;			/* radial size */
    double axrat = 0.0;			/* axis ratio */
    double orient = 0.0;		/* orientation */
    char *seeingStr = NULL;		/* PSF */
    char *maxStr = NULL;		/* Name of variable to return max */
    int exact = 0;			/* make exact model? */

    shErrStackClear();
    
    i = 1;
    tclMakeExpModel_opts[i++].dst = &totflux;
    tclMakeExpModel_opts[i++].dst = &size;
    tclMakeExpModel_opts[i++].dst = &axrat;
    tclMakeExpModel_opts[i++].dst = &orient;
    tclMakeExpModel_opts[i++].dst = &seeingStr;
    tclMakeExpModel_opts[i++].dst = &maxStr;
    tclMakeExpModel_opts[i++].dst = &exact;
    shAssert(tclMakeExpModel_opts[i].type == FTCL_ARGV_END);

    if(get_FtclOpts(interp, &argc, argv, tclMakeExpModel_opts) != TCL_OK) {

       return(TCL_ERROR);
    }
/*
 * process arguments
 */
    p.class = EXP_MODEL;
    p.exact = exact;
    p.rsize = size;
    p.aratio = axrat;
    p.orient = orient;
    if(shTclHandleExprEval(interp,seeingStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
    }
    if(hand.type != shTypeGetFromName("DGPSF")) {
	 Tcl_SetResult(interp,"makeExpModel: "
		       "fifth argument is not a DGPSF",TCL_STATIC);
	 return(TCL_ERROR);
    }
    p.psf = hand.ptr;
/*
 * Not too hard.
 */
    reg = phAnyModelMakeScaled(&p, totflux, &max);
/*
 * and return the desired answers
 */
   if(maxStr != NULL) {
      sprintf(str,"%d",max);
      if(Tcl_SetVar(interp,maxStr,str,TCL_LEAVE_ERR_MSG) == NULL) {
	 Tcl_SetResult(interp,"Failed to return max",TCL_VOLATILE);
	 return(TCL_ERROR);
      }
   } 

   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = reg;
   hand.type = shTypeGetFromName("REGION");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new REGION handle",TCL_STATIC);
      shRegDel(reg);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

static char *tclMakePsfModel_use =
  "USAGE: makePsfModel <totflux> <seeing> -max sval -exact";
#define tclMakePsfModel_hlp \
  "Make a PSF model"

static ftclArgvInfo makePsfModel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclMakePsfModel_hlp},
   {"<totflux>", FTCL_ARGV_DOUBLE, NULL, NULL, "total flux in model"},
   {"<seeing>", FTCL_ARGV_STRING, NULL, NULL, "PSF"},
   {"-max", FTCL_ARGV_STRING, NULL, NULL,
			       "Name of variable to return the max intensity"},
   {"-exact", FTCL_ARGV_CONSTANT, (void *)1, NULL,
     "Calculate model, rather then interpolating in profile tables (default)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define makePsfModel_name "makePsfModel"

static int
tclMakePsfModel(ClientData clientData,
		Tcl_Interp *interp,
		int ac,
		char **av)
{
   HANDLE hand;
   char name[HANDLE_NAMELEN];
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   double totflux = 0.0;		/* total flux in model */
   char *seeingStr = NULL;		/* PSF */
   char *maxStr = NULL;			/* Name of variable to return the
					   max intensity */
   int max = 0;
   int exact = 1;			/* Calculate model, rather then
					   interpolating in profile tables */
   MODEL_PARAMS p;			/* describe model */
   REGION *reg;				/* desired model */

   shErrStackClear();

   a_i = 1;
   makePsfModel_opts[a_i++].dst = &totflux;
   makePsfModel_opts[a_i++].dst = &seeingStr;
   makePsfModel_opts[a_i++].dst = &maxStr;
   makePsfModel_opts[a_i++].dst = &exact;
   shAssert(makePsfModel_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, makePsfModel_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     makePsfModel_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   p.class = PSF_MODEL;
   p.exact = exact;

   if(shTclHandleExprEval(interp,seeingStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("DGPSF")) {
      Tcl_AppendResult(interp,"makePsfModel: "
                       "argument \"", seeingStr, "\" is not a DGPSF",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   p.psf = hand.ptr;
/*
 * Not too hard.
 */
    reg = phAnyModelMakeScaled(&p, totflux, &max);
/*
 * and return the desired answers
 */
   if(maxStr != NULL) {
      char str[30];
      
      sprintf(str,"%d",max);
      if(Tcl_SetVar(interp,maxStr,str,TCL_LEAVE_ERR_MSG) == NULL) {
	 Tcl_SetResult(interp,"Failed to return max",TCL_VOLATILE);
	 return(TCL_ERROR);
      }
   } 

   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = reg;
   hand.type = shTypeGetFromName("REGION");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new REGION handle",TCL_STATIC);
      shRegDel(reg);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}



static char *tclCellMakeProfCat_use = "USAGE: makeCellProfCat <file> <nseeing> <fseeing> <dseeing> <psf_sigma_ratio> <psf_amp_ratio> [-show_progress]";
#define tclCellMakeProfCat_hlp "Create a compress cell Profile Catalogue in a disk file <file>."

static ftclArgvInfo tclCellMakeProfCat_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCellMakeProfCat_hlp},
   {"<file>", FTCL_ARGV_STRING, NULL, NULL, "Name of catalogue file"},
   {"<nseeing>", FTCL_ARGV_INT, NULL, NULL, "Number of seeings"},
   {"<fseeing>", FTCL_ARGV_DOUBLE, NULL, NULL, "First seeing"},
   {"<dseeing>", FTCL_ARGV_DOUBLE, NULL, NULL, "Delta seeing"},
   {"<psf_sigma_ratio>", FTCL_ARGV_DOUBLE, NULL, NULL, "sigma2/sigma1 for PSF"},
   {"<psf_amp_ratio>", FTCL_ARGV_DOUBLE, NULL, NULL, "PSF's b parameter"},
   {"-exact", FTCL_ARGV_CONSTANT, (void *)1, NULL,
		"Make exact models (slower)"},
   {"-show_progress", FTCL_ARGV_CONSTANT, (void *)1, NULL,
		"Print information to stderr as catalog construction proceeds"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclCellMakeProfCat(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
   int i;
   char *fileStr = NULL;		/* Name of catalogue file */
   int nseeing = 0;			/* Number of seeings */
   double fseeing = 0.0;		/* First seeing */
   double dseeing = 0.0;		/* Delta seeing */
   double psf_sigma_ratio = 0.0;	/* sigma2/sigma1 for PSF */
   double psf_amp_ratio = 0.0;		/* PSF's b parameter */
   int exact = 0;			/* make exact models? */
   int show_progress = 0;		/* Print info about construction */

   shErrStackClear();

   i = 1;
   tclCellMakeProfCat_opts[i++].dst = &fileStr;
   tclCellMakeProfCat_opts[i++].dst = &nseeing;
   tclCellMakeProfCat_opts[i++].dst = &fseeing;
   tclCellMakeProfCat_opts[i++].dst = &dseeing;
   tclCellMakeProfCat_opts[i++].dst = &psf_sigma_ratio;
   tclCellMakeProfCat_opts[i++].dst = &psf_amp_ratio;
   tclCellMakeProfCat_opts[i++].dst = &exact;
   tclCellMakeProfCat_opts[i++].dst = &show_progress;
   shAssert(tclCellMakeProfCat_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp, &argc, argv, tclCellMakeProfCat_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * Do the work.
 */
   if(phCellMakeProfCat(fileStr, nseeing, fseeing, dseeing, psf_sigma_ratio,
			psf_amp_ratio, exact, show_progress) != SH_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclInitCellFitobj_use = "USAGE: initCellFitobj <file>";
#define tclInitCellFitobj_hlp "Initialize the cellfitobj routines, and read "\
"a profile catalogue for a particular seeing from <file>; " \
"the seeing may be specified by either a DGPSF or a region containing a PSF"

static ftclArgvInfo tclInitCellFitobj_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclInitCellFitobj_hlp},
   {"<file>", FTCL_ARGV_STRING, NULL, NULL, "Profile catalogue to read"},
   {"<ncolor>", FTCL_ARGV_INT, NULL, NULL, "the number of filters"},
   {"-median", FTCL_ARGV_CONSTANT, (void *)1, NULL, "use median profiles"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclInitCellFitobj(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
   int i;
   char *fileStr = NULL;		/* Profile catalogue to read */
   int ncolor = 0;			/* the number of filters */
   int median = 0;			/* use median profiles */

   shErrStackClear();

   i = 1;
   tclInitCellFitobj_opts[i++].dst = &fileStr;
   tclInitCellFitobj_opts[i++].dst = &ncolor;
   tclInitCellFitobj_opts[i++].dst = &median;
   shAssert(tclInitCellFitobj_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp, &argc, argv, tclInitCellFitobj_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * Do the work.
 */
   if(phInitCellFitobj(fileStr, ncolor, median) != SH_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclFiniCellFitobj_use =
  "USAGE: finiCellFitobj ";
#define tclFiniCellFitobj_hlp \
  "Free memory allocated by initCellFitobj"

static ftclArgvInfo finiCellFitobj_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFiniCellFitobj_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFiniCellFitobj(
	      ClientData clientDatag,
	      Tcl_Interp *interp,
	      int ac,
	      char **av
	      )
{
   shErrStackClear();

   if(get_FtclOpts(interp,&ac,av,finiCellFitobj_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * work
 */
   phFiniCellFitobj();
   
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclFitCellPsfModel_use
		       = "USAGE: fitCellPsfModel object color fieldparams";
#define tclFitCellPsfModel_hlp "Fit a seeing convolved PSF model."

static ftclArgvInfo tclFitCellPsfModel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFitCellPsfModel_hlp},
   {"<objc>", FTCL_ARGV_STRING, NULL, NULL, "object to fit"},
   {"<color>", FTCL_ARGV_INT, NULL, NULL, "color of object"},
   {"<fiparams>", FTCL_ARGV_STRING, NULL, NULL, "fieldparams"},
   {"-fit_sky", FTCL_ARGV_CONSTANT, (void *)1, NULL,
		    "Fit the sky level as well as the amplitude? (default: 0)"},
   {"-nannuli", FTCL_ARGV_INT, NULL, NULL, "Number of annuli to use in fit"},
   {"-sky_noise_only", FTCL_ARGV_CONSTANT, (void *)1, NULL,
		    "Only include sky noise (default: 0)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFitCellPsfModel(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
    int i;
    FIELDPARAMS *fiparams;
    float I0 = 0;			/* desired amplitude of PSF */
    float sky = 0;			/* desired sky level */
    HANDLE hand;
    OBJC *obj;
    void *vptr;
    char *objcStr = NULL;		/* object to fit */
    int color = 0;			/* color of object */
    char *fiparamsStr = NULL;		/* fieldparams */
    int fit_sky = 0;			/* Fit sky level as well as amplitude?*/
    int nannuli = -1;			/* Number of annuli to use in fit */
    int sky_noise_only = 0;		/* only include sky noise when fitting*/

    shErrStackClear();

    i = 1;
    tclFitCellPsfModel_opts[i++].dst = &objcStr;
    tclFitCellPsfModel_opts[i++].dst = &color;
    tclFitCellPsfModel_opts[i++].dst = &fiparamsStr;
    tclFitCellPsfModel_opts[i++].dst = &fit_sky;
    tclFitCellPsfModel_opts[i++].dst = &nannuli;
    tclFitCellPsfModel_opts[i++].dst = &sky_noise_only;
    shAssert(tclFitCellPsfModel_opts[i].type == FTCL_ARGV_END);

    if(get_FtclOpts(interp, &argc, argv, tclFitCellPsfModel_opts) != TCL_OK) {

       return(TCL_ERROR);
    }
/*
 * process arguments
 */
    if(shTclHandleExprEval(interp, objcStr, &hand, &vptr) != TCL_OK) {
       return(TCL_ERROR);
    }
    if(hand.type != shTypeGetFromName("OBJC")) {
       Tcl_SetResult(interp,"fitCellPsfModel: first arg is not an OBJC",
		     TCL_STATIC);
       return(TCL_ERROR);
    }
    obj = hand.ptr;

    if(shTclHandleExprEval(interp, fiparamsStr, &hand, &vptr) != TCL_OK) {
       return(TCL_ERROR);
    }
    if(hand.type != shTypeGetFromName("FIELDPARAMS")) {
       Tcl_SetResult(interp,"fitCellPsfModel: third arg is not a FIELDPARAMS",
		     TCL_STATIC);
       return(TCL_ERROR);
    }
    fiparams = hand.ptr;
/*
 * Do it.
 */
    if(phFitCellAsPsf(obj, color, NULL, fiparams, nannuli, sky_noise_only,
		      (fit_sky ? &I0 : NULL), (fit_sky ? &sky : NULL)) < 0) {
       shTclInterpAppendWithErrStack(interp);
       return(TCL_ERROR);
    }

    if(fit_sky) {
       char buff[40];
       sprintf(buff,"%.3f %.3f ",I0, sky);
       Tcl_SetResult(interp, buff, TCL_VOLATILE);
    }
    
    return(TCL_OK);
}

/*****************************************************************************/

static char *tclFitCellDevModel_use
		       = "USAGE: fitCellDevModel object color fieldparams";
#define tclFitCellDevModel_hlp "Fit a seeing convolved de Vaucouleurs model."

static ftclArgvInfo tclFitCellDevModel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFitCellDevModel_hlp},
   {"<objc>", FTCL_ARGV_STRING, NULL, NULL, "object to fit"},
   {"<color>", FTCL_ARGV_INT, NULL, NULL, "color of object"},
   {"<fiparams>", FTCL_ARGV_STRING, NULL, NULL, "fieldparams"},
   {"-nannuli", FTCL_ARGV_INT, NULL, NULL, "Number of annuli to use in fit"},
   {"-sky_noise_only", FTCL_ARGV_CONSTANT, (void *)1, NULL,
		    "Only include sky noise (default: 0)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFitCellDevModel(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
    int i;
    FIELDPARAMS *fiparams;
    HANDLE hand;
    OBJC *obj;
    void *vptr;
    char *objcStr = NULL;		/* object to fit */
    int color = 0;			/* color of object */
    char *fiparamsStr = NULL;		/* fieldparams */
    int nannuli = -1;			/* Number of annuli to use in fit */
    int sky_noise_only = 0;		/* only include sky noise when fitting*/

    shErrStackClear();

    i = 1;
    tclFitCellDevModel_opts[i++].dst = &objcStr;
    tclFitCellDevModel_opts[i++].dst = &color;
    tclFitCellDevModel_opts[i++].dst = &fiparamsStr;
    tclFitCellDevModel_opts[i++].dst = &nannuli;
    tclFitCellDevModel_opts[i++].dst = &sky_noise_only;
    shAssert(tclFitCellDevModel_opts[i].type == FTCL_ARGV_END);
    
    if(get_FtclOpts(interp, &argc, argv, tclFitCellDevModel_opts) != TCL_OK) {
       return(TCL_ERROR);
    }
/*
 * process arguments
 */
    if(shTclHandleExprEval(interp, objcStr, &hand, &vptr) != TCL_OK) {
       return(TCL_ERROR);
    }
    if(hand.type != shTypeGetFromName("OBJC")) {
       Tcl_SetResult(interp,"fitCellDevModel: first arg is not an OBJC",
		     TCL_STATIC);
       return(TCL_ERROR);
    }
    obj = hand.ptr;

    if(shTclHandleExprEval(interp, fiparamsStr, &hand, &vptr) != TCL_OK) {
       return(TCL_ERROR);
    }
    if(hand.type != shTypeGetFromName("FIELDPARAMS")) {
       Tcl_SetResult(interp,"fitCellDevModel: third arg is not a FIELDPARAMS",
		     TCL_STATIC);
       return(TCL_ERROR);
    }
    fiparams = hand.ptr;
/*
 * Do it.
 */
    if(phFitCellAsDeV(obj, color, NULL, fiparams, nannuli, sky_noise_only) < 0){
       shTclInterpAppendWithErrStack(interp);
       return(TCL_ERROR);
    }
    
    return(TCL_OK);
}

static char *tclFitCellExpModel_use = 
				    "USAGE: fitCellExpModel color fieldparams";
#define tclFitCellExpModel_hlp "Fit a seeing convolved de Vaucouleurs model."

static ftclArgvInfo tclFitCellExpModel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFitCellExpModel_hlp},
   {"<objc>", FTCL_ARGV_STRING, NULL, NULL, "object to fit"},
   {"<color>", FTCL_ARGV_INT, NULL, NULL, "color of object"},
   {"<fiparams>", FTCL_ARGV_STRING, NULL, NULL, "fieldparams"},
   {"-nannuli", FTCL_ARGV_INT, NULL, NULL, "Number of annuli to use in fit"},
   {"-sky_noise_only", FTCL_ARGV_CONSTANT, (void *)1, NULL,
		    "Only include sky noise (default: 0)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFitCellExpModel(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
    int i;
    FIELDPARAMS *fiparams;
    HANDLE hand;
    OBJC *obj;
    void *vptr;
    char *objcStr = NULL;		/* object to fit */
    int color = 0;			/* color of object */
    char *fiparamsStr = NULL;		/* fieldparams */
    int nannuli = -1;			/* Number of annuli to use in fit */
    int sky_noise_only = 0;		/* only include sky noise when fitting*/

    shErrStackClear();

    i = 1;
    tclFitCellExpModel_opts[i++].dst = &objcStr;
    tclFitCellExpModel_opts[i++].dst = &color;
    tclFitCellExpModel_opts[i++].dst = &fiparamsStr;
    tclFitCellExpModel_opts[i++].dst = &nannuli;
    tclFitCellExpModel_opts[i++].dst = &sky_noise_only;
    shAssert(tclFitCellExpModel_opts[i].type == FTCL_ARGV_END);

    if(get_FtclOpts(interp, &argc, argv, tclFitCellExpModel_opts) != TCL_OK) {
       return(TCL_ERROR);
    }
/*
 * process arguments
 */
    if(shTclHandleExprEval(interp, objcStr, &hand, &vptr) != TCL_OK) {
       return(TCL_ERROR);
    }
    if(hand.type != shTypeGetFromName("OBJC")) {
       Tcl_SetResult(interp,"fitCellExpModel: first arg is not an OBJC",
		     TCL_STATIC);
       return(TCL_ERROR);
    }
    obj = hand.ptr;

    if(shTclHandleExprEval(interp, fiparamsStr, &hand, &vptr) != TCL_OK) {
       return(TCL_ERROR);
    }
    if(hand.type != shTypeGetFromName("FIELDPARAMS")) {
       Tcl_SetResult(interp,"fitCellExpModel: third arg is not a FIELDPARAMS",
		     TCL_STATIC);
       return(TCL_ERROR);
    }
    fiparams = hand.ptr;
/*
 * Do it.
 */
    if(phFitCellAsExp(obj, color, NULL, fiparams, nannuli, sky_noise_only) < 0){
       shTclInterpAppendWithErrStack(interp);
       return(TCL_ERROR);
    }
    
    return(TCL_OK);
}

/*****************************************************************************/
static char *tclRegIntRotate_use =
  "USAGE: RegIntRotate <region> <angle> [-bkgd val] [-sinc] [-square]";
#define tclRegIntRotate_hlp \
  "Rotate a U16 region through <angle> degrees"

static ftclArgvInfo regIntRotate_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegIntRotate_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region to rotate"},
   {"<angle>", FTCL_ARGV_DOUBLE, NULL, NULL, "Angle to rotate by (degrees)"},
   {"-bkgd", FTCL_ARGV_INT, NULL, NULL,
		   "Value to use for pixels not present in <reg> (default: 0)"},
   {"-sinc", FTCL_ARGV_CONSTANT, (void *)1, NULL,
					"Use sinc interpolation when rotating"},
   {"-square", FTCL_ARGV_CONSTANT, (void *)1, NULL,"Make output region square"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclRegIntRotate(
		ClientData clientDatag,
		Tcl_Interp *interp,
		int ac,
		char **av
		)
{
   int i;
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   REGION *reg;
   void *vptr;				/* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region to rotate */
   double angle = 0.0;			/* Angle to rotate by (degrees) */
   int bkgd = 0;			/* Value to use for missing pixels */
   int sinc = 0;			/* Use sinc interpolation? */
   int square = 0;			/* Make output region square? */

   shErrStackClear();

   i = 1;
   regIntRotate_opts[i++].dst = &regStr;
   regIntRotate_opts[i++].dst = &angle;
   regIntRotate_opts[i++].dst = &bkgd;
   regIntRotate_opts[i++].dst = &sinc;
   regIntRotate_opts[i++].dst = &square;
   shAssert(regIntRotate_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,regIntRotate_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"regIntRotate: "
                    "first argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(reg->type != TYPE_PIX) {
      Tcl_SetResult(interp,"regIntRotate: REGION is not U16",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   reg = shRegIntRotate(reg,angle,bkgd,sinc,square);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = reg;
   hand.type = shTypeGetFromName("REGION");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new REGION handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);


   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Show the contents of a model data file, either raw profiles or the
 * extracted cell arrays
 */
static char *tclProfileTableHeaderPrint_use =
  "USAGE: ProfileTableHeaderPrint file [-raw]";
#define tclProfileTableHeaderPrint_hlp \
  "Print the header from a profile table; usually the cell array version, but"\
"the raw (pixel) table with the -raw flag"

static ftclArgvInfo profileTableHeaderPrint_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclProfileTableHeaderPrint_hlp},
   {"<table>", FTCL_ARGV_STRING, NULL, NULL, "Table to read"},
   {"-raw", FTCL_ARGV_CONSTANT, (void *)1, NULL,
				     "The table's raw, not a cell array table"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclProfileTableHeaderPrint(
			   ClientData clientDatag,
			   Tcl_Interp *interp,
			   int ac,
			   char **av
			   )
{
   int i;
   char *tableStr = NULL;		/* Table to read */
   int raw = 0;				/* The table's raw, not a cell table */

   shErrStackClear();

   i = 1;
   profileTableHeaderPrint_opts[i++].dst = &tableStr;
   profileTableHeaderPrint_opts[i++].dst = &raw;
   shAssert(profileTableHeaderPrint_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,profileTableHeaderPrint_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */

/*
 * work
 */
   if(phProfileTableHeaderPrint(tableStr,!raw) != SH_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Check the header of a model data file, either raw profiles or the
 * extracted cell arrays
 */
static char *tclProfileTableHeaderCheck_use =
  "USAGE: ProfileTableHeaderCheck file [-hash val] [-raw]";
#define tclProfileTableHeaderCheck_hlp \
  "Check the header from a profile table; usually the cell array version, but"\
"the raw (pixel) table with the -raw flag. "\
"If hash is provided, it's the expected hash value"

static ftclArgvInfo profileTableHeaderCheck_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclProfileTableHeaderCheck_hlp},
   {"<table>", FTCL_ARGV_STRING, NULL, NULL, "Table to read"},
   {"-raw", FTCL_ARGV_CONSTANT, (void *)1, NULL,
				     "The table's raw, not a cell array table"},
   {"-hash", FTCL_ARGV_INT, NULL, NULL, "Expected hash value"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclProfileTableHeaderCheck(
			   ClientData clientDatag,
			   Tcl_Interp *interp,
			   int ac,
			   char **av
			   )
{
   int i;
   char *tableStr = NULL;		/* Table to read */
   int raw = 0;				/* The table's raw, not a cell table */
   int hash = 0;			/* Expected hash value */

   shErrStackClear();

   i = 1;
   profileTableHeaderCheck_opts[i++].dst = &tableStr;
   profileTableHeaderCheck_opts[i++].dst = &raw;
   profileTableHeaderCheck_opts[i++].dst = &hash;
   shAssert(profileTableHeaderCheck_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,profileTableHeaderCheck_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */

/*
 * work
 */
   if(phProfileTableHeaderCheck(tableStr, !raw, hash) != SH_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclProfileTableHashGet_use =
  "USAGE: ProfileTableHashGet file";
#define tclProfileTableHashGet_hlp \
  "Returns the hash code from the profile's header"

static ftclArgvInfo profileTableHashGet_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclProfileTableHashGet_hlp},
   {"<file>", FTCL_ARGV_STRING, NULL, NULL, "File with desired hash code"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclProfileTableHashGet(
		       ClientData clientDatag,
		       Tcl_Interp *interp,
		       int ac,
		       char **av
		       )
{
   int i;
   char buff[30];
   int hash = 0;
   char *fileStr = NULL;		/* File with desired hash code */
   
   shErrStackClear();

   i = 1;
   profileTableHashGet_opts[i++].dst = &fileStr;
   shAssert(profileTableHashGet_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,profileTableHashGet_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * work
 */
   if(phProfileTableHashGet(fileStr, &hash) != SH_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   
   sprintf(buff, "0x%x", hash);
   Tcl_SetResult(interp, buff, TCL_VOLATILE);

   return(TCL_OK);
}


/*****************************************************************************/
static char *tclFitCellChisq_use =
  "USAGE: fitCellChisq <obj> <color> <fiparams> -median ...";
#define tclFitCellChisq_hlp \
"Return a region containg a slice through the chisq surface for fitting a "\
"model to the object in <obj> using cell arrays"

static ftclArgvInfo fitCellChisq_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFitCellChisq_hlp},
   {"<obj>", FTCL_ARGV_STRING, NULL, NULL, "object to fit (OBJC or OBJECT1)"},
   {"<color>", FTCL_ARGV_INT, NULL, NULL, "color of object"},
   {"<fiparams>", FTCL_ARGV_STRING, NULL, NULL, "fieldparams"},
   {"-deV", FTCL_ARGV_CONSTANT, (void *)1, NULL,
			   "Fit a deVaucouleurs model (default: exponential)"},
   {"-exp", FTCL_ARGV_CONSTANT, (void *)1, NULL,
			   "Fit an exponential model (default)"},
   {"<aratio0>", FTCL_ARGV_DOUBLE, NULL, NULL, "Starting value of a/b"},
   {"<aratio1>", FTCL_ARGV_DOUBLE, NULL, NULL, "Ending value of a/b"},
   {"<naratio>", FTCL_ARGV_INT, NULL, NULL, "Number of desired values of a/b"},
   {"<rsize0>", FTCL_ARGV_DOUBLE, NULL, NULL, "Starting value of r_eff"},
   {"<rsize1>", FTCL_ARGV_DOUBLE, NULL, NULL, "Ending value of r_eff"},
   {"<nrsize>", FTCL_ARGV_INT, NULL, NULL,"Number of desired values of r_eff"},
   {"-sky_noise_only", FTCL_ARGV_CONSTANT, (void *)1, NULL,
      "Only include sky noise when evaluating chi^2"},
   {"-per_dof", FTCL_ARGV_CONSTANT, (void *)1, NULL,
      "Divide chi^2 by number of degrees of freedom in fit"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFitCellChisq(
		ClientData clientDatag,
		Tcl_Interp *interp,
		int ac,
		char **av
		)
{
   int i;
   REGION *creg;
   FIELDPARAMS *fiparams;
   HANDLE hand;
   char name[HANDLE_NAMELEN];
   OBJECT1 *obj;
   void *vptr;
   char *objStr = NULL;			/* object to fit */
   int color = 0;			/* color of object */
   char *fiparamsStr = NULL;		/* fieldparams */
   int deV = 0;				/* Fit a deVaucouleurs model? */
   int eexp = 0;			/* Fit an exponential model? */
   double aratio0 = 0.0;		/* Starting value of a/b */
   double aratio1 = 0.0;		/* Ending value of a/b */
   int naratio = 0;			/* Number of desired values of a/b */
   double rsize0 = 0.0;			/* Starting value of r_eff */
   double rsize1 = 0.0;			/* Ending value of r_eff */
   int nrsize = 0;			/* Number of desired values of r_eff */
   int sky_noise_only = 0;		/* cell variance == sky noise? */
   int per_dof = 0;			/* Divide chi^2 by degrees of freedom?*/
   int made_obj = 0;			/* did we create obj outselves? */

   shErrStackClear();

   i = 1;
   fitCellChisq_opts[i++].dst = &objStr;
   fitCellChisq_opts[i++].dst = &color;
   fitCellChisq_opts[i++].dst = &fiparamsStr;
   fitCellChisq_opts[i++].dst = &deV;
   fitCellChisq_opts[i++].dst = &eexp;
   fitCellChisq_opts[i++].dst = &aratio0;
   fitCellChisq_opts[i++].dst = &aratio1;
   fitCellChisq_opts[i++].dst = &naratio;
   fitCellChisq_opts[i++].dst = &rsize0;
   fitCellChisq_opts[i++].dst = &rsize1;
   fitCellChisq_opts[i++].dst = &nrsize;
   fitCellChisq_opts[i++].dst = &sky_noise_only;
   fitCellChisq_opts[i++].dst = &per_dof;
   shAssert(fitCellChisq_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,fitCellChisq_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(deV && eexp) {
      Tcl_SetResult(interp, "fitCellChisq: please choose -deV or -exp",
								   TCL_STATIC);
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp, objStr, &hand, &vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type == shTypeGetFromName("OBJECT1")) {
      obj = hand.ptr;
   } else if(hand.type == shTypeGetFromName("OBJC")) {
      const OBJC *objc = hand.ptr;

      shAssert(color < objc->ncolor);

      obj = objc->color[color];
   } else if(hand.type == shTypeGetFromName("OBJC_IO")) {
      const OBJC_IO *objcIo = hand.ptr;
      const CELL_STATS *cstats = phProfileGeometry();

      shAssert(color < objcIo->ncolor);

      made_obj = 1;
      obj = phObject1New();
      obj->rowc = objcIo->rowc[color]; obj->rowcErr = objcIo->rowcErr[color];
      obj->colc = objcIo->colc[color]; obj->colcErr = objcIo->colcErr[color];
      obj->sky = objcIo->sky[color];
      obj->skyErr = objcIo->skyErr[color];

      obj->mask = phObjmaskFromCircle(obj->rowc, obj->colc,
				 cstats->radii[objcIo->nprof[color] + 1] + 1.5);
   } else {
      Tcl_SetResult(interp,"fitCellChisq: first arg is not an OBJECT1, OBJC, or OBJC_IO",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
   
   if(shTclHandleExprEval(interp, fiparamsStr, &hand, &vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FIELDPARAMS")) {
      Tcl_SetResult(interp,"fitCellChisq: third arg is not a FIELDPARAMS",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
   fiparams = hand.ptr;
/*
 * work
 */
   creg = phFitCellChisq(obj, color, fiparams, deV,
			 aratio0, aratio1, naratio, rsize0, rsize1, nrsize,
			 sky_noise_only, per_dof);

   if(made_obj) {			/* we made it ourselves */
      phObject1Del(obj);
   }

   if(creg == NULL) {
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
   hand.ptr = creg;
   hand.type = shTypeGetFromName("REGION");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new REGION handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclResidualsFind_use =
  "USAGE: ResidualsFind <reg1> <reg2> <drow> <dcol>";
#define tclResidualsFind_hlp \
  "Return the difference <reg1> - <reg2>; <reg2>'s origin in <reg1> is taken"\
"to be (<drow>,<dcol>)"

static ftclArgvInfo residualsFind_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclResidualsFind_hlp},
   {"<reg1>", FTCL_ARGV_STRING, NULL, NULL, "First region"},
   {"<reg2>", FTCL_ARGV_STRING, NULL, NULL, "Second region"},
   {"<drow>", FTCL_ARGV_DOUBLE, NULL, NULL, "Row offset of <reg2>"},
   {"<dcol>", FTCL_ARGV_DOUBLE, NULL, NULL, "Column offset of <reg2>"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclResidualsFind(
		 ClientData clientDatag,
		 Tcl_Interp *interp,
		 int ac,
		 char **av
		 )
{
   int i;
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   REGION *reg1, *reg2, *out;
   void *vptr;				/* used by shTclHandleExprEval */
   char *reg1Str = NULL;		/* First region */
   char *reg2Str = NULL;		/* Second region */
   double drow = 0.0;			/* Row offset of <reg2> */
   double dcol = 0.0;			/* Column offset of <reg2> */

   shErrStackClear();

   i = 1;
   residualsFind_opts[i++].dst = &reg1Str;
   residualsFind_opts[i++].dst = &reg2Str;
   residualsFind_opts[i++].dst = &drow;
   residualsFind_opts[i++].dst = &dcol;
   shAssert(residualsFind_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,residualsFind_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,reg1Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"residualsFind: "
                    "first argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg1 = hand.ptr;
   if(reg1->type != TYPE_PIX) {
      Tcl_SetResult(interp,"residualsFind: "
                    "first argument is not of type U16",TCL_STATIC);
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,reg2Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"residualsFind: "
                    "second argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg2 = hand.ptr;
   if(reg2->type != TYPE_PIX) {
      Tcl_SetResult(interp,"residualsFind: "
                    "second argument is not of type U16",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   out = phResidualsFind(reg1, reg2, drow, dcol);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = out;
   hand.type = shTypeGetFromName("REGION");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new REGION handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclFitPsfFromReg_use =
  "USAGE: FitPsfFromReg <reg> <sky>";
#define tclFitPsfFromReg_hlp \
  "Return a DGPSF (with all fields set) from a star in <reg> (sky level <sky>"

static ftclArgvInfo fitPsfFromReg_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFitPsfFromReg_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region containing star to fit"},
   {"<sky>", FTCL_ARGV_DOUBLE, NULL, NULL, "Sky level in region"},
   {"-sigma", FTCL_ARGV_DOUBLE, NULL, NULL,
				  "Smooth with N(0,sigma^2) when centroiding"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFitPsfFromReg(
			   ClientData clientDatag,
			   Tcl_Interp *interp,
			   int ac,
			   char **av
			   )
{
   int i;
   HANDLE hand;
   char name[HANDLE_NAMELEN];
   DGPSF *psf;				/* PSF to return */
   const REGION *reg;
   void *vptr;				/* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region containing star to fit */
   double sky = 0.0;			/* Sky level in region */
   double sigma = 1.0;			/* smoothing sigma */

   shErrStackClear();

   i = 1;
   fitPsfFromReg_opts[i++].dst = &regStr;
   fitPsfFromReg_opts[i++].dst = &sky;
   fitPsfFromReg_opts[i++].dst = &sigma;
   shAssert(fitPsfFromReg_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,fitPsfFromReg_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"fitPsfFromReg: "
                    "first argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;
/*
 * work
 */
   *(psf = phDgpsfNew()) = *phFitPsfFromReg(reg, sigma, sky, NULL);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = psf;
   hand.type = shTypeGetFromName("DGPSF");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new DGPSF handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclFitCellColorSet_use =
  "USAGE: fitCellColorSet <c> <coeffs>";
#define tclFitCellColorSet_hlp \
  "Tell measure objects about a PSF. Debugging/stand alone code only"

static ftclArgvInfo fitCellColorSet_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFitCellColorSet_hlp},
   {"<c>", FTCL_ARGV_INT, NULL, NULL, "The desired colour"},
   {"<coeffs>", FTCL_ARGV_STRING, NULL, NULL,
				  "PSF_COEFFS (as set by e.g. fitPsfFromReg)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define fitCellColorSet_name "fitCellColorSet"

static int
tclFitCellColorSet(ClientData clientData,
		   Tcl_Interp *interp,
		   int ac,
		   char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   int c = 0;				/* The desired colour */
   char *coeffsStr = NULL;		/* the PSF_COEFFS */
   PSF_COEFFS *coeffs;

   shErrStackClear();

   a_i = 1;
   fitCellColorSet_opts[a_i++].dst = &c;
   fitCellColorSet_opts[a_i++].dst = &coeffsStr;
   shAssert(fitCellColorSet_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, fitCellColorSet_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     fitCellColorSet_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,coeffsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PSF_COEFFS")) {
      Tcl_AppendResult(interp,"fitCellColorSet: "
                       "argument \"", coeffsStr, "\" is not a PSF_COEFFS",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   coeffs = hand.ptr;
/*
 * work
 */
   phFitCellColorSet(c, coeffs);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclFittedModelMake_use =
  "USAGE: fittedModelMake <objc> <color> -exp -deV <basis> -psf -nocorrect -sigma dval -cellprof";
#define tclFittedModelMake_hlp \
  "Return a region containing the model of a galaxy, allowing for the\n\
discrepancy between the PSF and the model PSF\n\
\n\
The algorithm used parallels that used to reconstruct the cellarray\n\
models used in fitting\n\
"

static ftclArgvInfo fittedModelMake_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFittedModelMake_hlp},
   {"<basis>", FTCL_ARGV_STRING, NULL, NULL, "PSF_BASIS for this field"},
   {"<objc>", FTCL_ARGV_STRING, NULL, NULL, "Return the model for this OBJC"},
   {"<color>", FTCL_ARGV_INT, NULL, NULL, "in this band"},
   {"-deV", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Make a deVaucouleurs model"},
   {"-exp", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Make an exponential model"},
   {"-psf", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Make a PSF model"},
   {"-nocorrect", FTCL_ARGV_CONSTANT, (void *)1, NULL,
				     "Don't correct for residuals wrt KL PSF"},
   {"-sigma", FTCL_ARGV_DOUBLE, NULL, NULL, "Smoothing sigma for centroiding"},
   {"-exact", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Make exact models"},
   {"-cellprof", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Return the cellmodel"},   
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define fittedModelMake_name "fittedModelMake"

static int
tclFittedModelMake(ClientData clientData,
		   Tcl_Interp *interp,
		   int ac,
		   char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *objcStr = NULL;		/* Return the model for this OBJC */
   char *basisStr = NULL;		/* PSF_BASIS for this field */
   PSF_BASIS *basis;
   OBJC *objc;
   int color = 0;			/* in this band */
   int make_deV = 0;			/* Make a deVaucouleurs model */
   int make_exp = 0;			/* Make an exponential model */
   int make_psf = 0;			/* Make a PSF model */
   REGION *gal;				/* desired galaxy */
   int nocorrect = 0;			/* Don't correct for PSF residuals */
   double sigma = 0.0;			/* Smoothing sigma for centroiding */
   int exact = 0;			/* make exact models? */
   int cellprof = 0;			/* Return the cellmodel */

   shErrStackClear();

   a_i = 1;
   fittedModelMake_opts[a_i++].dst = &basisStr;
   fittedModelMake_opts[a_i++].dst = &objcStr;
   fittedModelMake_opts[a_i++].dst = &color;
   fittedModelMake_opts[a_i++].dst = &make_deV;
   fittedModelMake_opts[a_i++].dst = &make_exp;
   fittedModelMake_opts[a_i++].dst = &make_psf;
   fittedModelMake_opts[a_i++].dst = &nocorrect;
   fittedModelMake_opts[a_i++].dst = &sigma;
   fittedModelMake_opts[a_i++].dst = &exact;
   fittedModelMake_opts[a_i++].dst = &cellprof;
   shAssert(fittedModelMake_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, fittedModelMake_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     fittedModelMake_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,basisStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PSF_BASIS")) {
      Tcl_AppendResult(interp,"fittedModelMake: "
                       "argument \"", basisStr, "\" is not a PSF_BASIS",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   basis = hand.ptr;

   if(shTclHandleExprEval(interp,objcStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJC")) {
      Tcl_AppendResult(interp,"fittedModelMake: "
                       "argument \"", objcStr, "\" is not an OBJC",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   objc = hand.ptr;

   if(color < 0 || color >= objc->ncolor) {
      char buff[100];
      sprintf(buff, "fittedModelMake: invalid colour %d (must be 0..%d)",
	      color, objc->ncolor - 1);
      Tcl_SetResult(interp, buff, TCL_VOLATILE);
      return(TCL_ERROR);
   }

   if(make_deV + make_exp + make_psf != 1) {
      Tcl_SetResult(interp, "fittedModelMake: "
		    "please choose exactly one of -deV and -exp and -psf",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   gal = phFittedModelMake(basis, objc, color,
			   (make_deV ? DEV_MODEL :
			    make_exp ? EXP_MODEL : PSF_MODEL),
			   sigma, !nocorrect, exact, cellprof);
/*
 * Return the answer
 */
   if(gal == NULL) {
      Tcl_SetResult(interp, "fittedModelMake: model is NULL", TCL_STATIC);
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = gal;
   hand.type = shTypeGetFromName("REGION");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"gal\" to new REGION handle",
                                                                   TCL_STATIC);
      shRegDel(gal);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

static char *tclCompareProfileFiles_use =
  "USAGE: compareProfileFiles <file1> <file2> -raw -eps dval -feps dval -normalize -model0 ival -model1 ival -nomodel -fsao ival -outfileFmt sval";
#define tclCompareProfileFiles_hlp \
  ""

static ftclArgvInfo compareProfileFiles_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCompareProfileFiles_hlp},
   {"<file1>", FTCL_ARGV_STRING, NULL, NULL, "One profile file"},
   {"<file2>", FTCL_ARGV_STRING, NULL, NULL, "The other profile file"},
   {"-n0", FTCL_ARGV_INT, NULL, NULL, "First model to compare (0-indexed)"},
   {"-n1", FTCL_ARGV_INT, NULL, NULL, "Last model to compare (-ve => relative to end)"},   
   {"-raw", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Files contain raw profiles"},
   {"-eps", FTCL_ARGV_DOUBLE, NULL, NULL, "Maximum allowable difference"},
   {"-feps", FTCL_ARGV_DOUBLE, NULL, NULL, "Maximum allowed fractional difference"},
   {"-normalize", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Normalise profiles to centre == 1"},
   {"-nomodel", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Don't compare models"},
   {"-outfileFmt", FTCL_ARGV_STRING, NULL, NULL, "Format (with one %%d) for dumping objects"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define compareProfileFiles_name "compareProfileFiles"

static int
tclCompareProfileFiles(ClientData clientData,
		       Tcl_Interp *interp,
		       int ac,
		       char **av)
{
   int a_i;
   char *file1Str = NULL;		/* One profile file */
   char *file2Str = NULL;		/* The other profile file */
   int raw = 0;				/* Files contain raw profiles */
   double eps = 0.0;			/* Maximum allowable fractional difference */
   double feps = 0.0;			/* Maximum allowed fractional difference */
   int normalize = 0;			/* Normalise profiles to centre == 1 */
   int n0 = 0;				/* First model to compare (0-indexed) */
   int n1 = -1;				/* Last model to compare (-ve => relative to end) */
   int nomodel = 0;			/* Don't compare models */
   char *outfileFmtStr = NULL;		/* Format (with one %%d) for dumping objects */

   shErrStackClear();

   a_i = 1;
   compareProfileFiles_opts[a_i++].dst = &file1Str;
   compareProfileFiles_opts[a_i++].dst = &file2Str;
   compareProfileFiles_opts[a_i++].dst = &n0;
   compareProfileFiles_opts[a_i++].dst = &n1;
   compareProfileFiles_opts[a_i++].dst = &raw;
   compareProfileFiles_opts[a_i++].dst = &eps;
   compareProfileFiles_opts[a_i++].dst = &feps;
   compareProfileFiles_opts[a_i++].dst = &normalize;
   compareProfileFiles_opts[a_i++].dst = &nomodel;
   compareProfileFiles_opts[a_i++].dst = &outfileFmtStr;
   shAssert(compareProfileFiles_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, compareProfileFiles_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     compareProfileFiles_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * Process arguments
 */
   if(nomodel) {
       n0 = 1; n1 = 0;
   }
/*
 * work
 */
   {
       int ret = raw ?
	   phCompareRawProfileFiles(file1Str, file2Str, n0, n1,
				    eps, feps, normalize, outfileFmtStr) :
	   phCompareProfileFiles(file1Str, file2Str, n0, n1,
				 eps, feps, normalize);

       if (ret != 0) {
	   Tcl_SetResult(interp, "compareProfileFiles: ", TCL_STATIC);
	   shTclInterpAppendWithErrStack(interp);
	   return(TCL_ERROR);
       }
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclMakePowerLawModel_use =
  "USAGE: makePowerLawModel <I0> <beta> <rmin> -nrow ival -ncol ival -rowc dval -colc dval";
#define tclMakePowerLawModel_hlp \
  "Make a model that's a pure powerlaw outside rmin, and constant inside"

static ftclArgvInfo makePowerLawModel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclMakePowerLawModel_hlp},
   {"<I0>", FTCL_ARGV_DOUBLE, NULL, NULL, "Coefficient of r^{-beta}"},
   {"<beta>", FTCL_ARGV_DOUBLE, NULL, NULL, "Power law index"},
   {"<rmin>", FTCL_ARGV_DOUBLE, NULL, NULL, "Radius of constant core"},
   {"-nrow", FTCL_ARGV_INT, NULL, NULL, "Number of rows in region that model will be subtracted from"},
   {"-ncol", FTCL_ARGV_INT, NULL, NULL, "Number of columns in region that model will be subtracted from"},
   {"-rowc", FTCL_ARGV_DOUBLE, NULL, NULL, "row-position of object in region that model will be subtracted from"},
   {"-colc", FTCL_ARGV_DOUBLE, NULL, NULL, "column-position of object in region that model will be subtracted from"},   
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define makePowerLawModel_name "makePowerLawModel"

static int
tclMakePowerLawModel(ClientData clientData,
		     Tcl_Interp *interp,
		     int ac,
		     char **av)
{
   char name[HANDLE_NAMELEN];
    HANDLE hand;
   int a_i;
   double I0 = 0.0;			/* Coefficient of r^{-beta} */
   double beta = 0.0;			/* Power law index */
   double rmin = 0.0;			/* Radius of constant core */
   REGION *reg = NULL;			/* the desired answer */
   int nrow = 0;			/* Number of rows in region that model will be subtracted from */
   int ncol = 0;			/* Number of columns in region that model will be subtracted from */
   double rowc = 0.0;		     /* row-position of object in region that model will be subtracted from */
   double colc = 0.0;		  /* column-position of object in region that model will be subtracted from */

   shErrStackClear();

   a_i = 1;
   makePowerLawModel_opts[a_i++].dst = &I0;
   makePowerLawModel_opts[a_i++].dst = &beta;
   makePowerLawModel_opts[a_i++].dst = &rmin;
   makePowerLawModel_opts[a_i++].dst = &nrow;
   makePowerLawModel_opts[a_i++].dst = &ncol;
   makePowerLawModel_opts[a_i++].dst = &rowc;
   makePowerLawModel_opts[a_i++].dst = &colc;
   shAssert(makePowerLawModel_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, makePowerLawModel_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     makePowerLawModel_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   reg = phMakePowerLawModel(I0, beta, rmin, nrow, ncol, rowc, colc);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = reg;
   hand.type = shTypeGetFromName("REGION");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"reg\" to new REGION handle",
                                                                   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/

void phTclFitobjDeclare(Tcl_Interp *interp)
{
    shTclDeclare(interp, "makeProfCat",
		 (Tcl_CmdProc *)tclMakeProfCat,
		 (ClientData) 0,
		 (Tcl_CmdDeleteProc *) NULL,
		 module,
		 tclMakeProfCat_hlp, tclMakeProfCat_use);
    shTclDeclare(interp, "initFitobj",
		 (Tcl_CmdProc *)tclInitFitobj,
		 (ClientData) 0,
		 (Tcl_CmdDeleteProc *) NULL,
		 module,
		 tclInitFitobj_hlp, tclInitFitobj_use);
    shTclDeclare(interp, "makePSFModel",
		 (Tcl_CmdProc *)tclMakePSFModel,
		 (ClientData) 0,
		 (Tcl_CmdDeleteProc *) NULL,
		 module,
		 tclMakePSFModel_hlp, tclMakePSFModel_use);
    shTclDeclare(interp, "makeDevModel",
		 (Tcl_CmdProc *)tclMakeDevModel,
		 (ClientData) 0,
		 (Tcl_CmdDeleteProc *) NULL,
		 module,
		 tclMakeDevModel_hlp, tclMakeDevModel_use);
    shTclDeclare(interp, "makeExpModel",
		 (Tcl_CmdProc *)tclMakeExpModel,
		 (ClientData) 0,
		 (Tcl_CmdDeleteProc *) NULL,
		 module,
		 tclMakeExpModel_hlp, tclMakeExpModel_use);
   shTclDeclare(interp,"finiFitobj",
		(Tcl_CmdProc *)tclFiniFitobj, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFiniFitobj_hlp,
		tclFiniFitobj_use);

    shTclDeclare(interp, "makeCellProfCat",
		 (Tcl_CmdProc *)tclCellMakeProfCat,
		 (ClientData) 0,
		 (Tcl_CmdDeleteProc *) NULL,
		 module,
		 tclCellMakeProfCat_hlp, tclCellMakeProfCat_use);
    shTclDeclare(interp, "initCellFitobj",
		 (Tcl_CmdProc *)tclInitCellFitobj,
		 (ClientData) 0,
		 (Tcl_CmdDeleteProc *) NULL,
		 module,
		 tclInitCellFitobj_hlp, tclInitCellFitobj_use);
   shTclDeclare(interp,"finiCellFitobj",
		(Tcl_CmdProc *)tclFiniCellFitobj, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFiniCellFitobj_hlp,
		tclFiniCellFitobj_use);
    shTclDeclare(interp, "fitCellPsfModel",
		 (Tcl_CmdProc *)tclFitCellPsfModel,
		 (ClientData) 0,
		 (Tcl_CmdDeleteProc *) NULL,
		 module,
		 tclFitCellPsfModel_hlp, tclFitCellPsfModel_use);
    shTclDeclare(interp, "fitCellDevModel",
		 (Tcl_CmdProc *)tclFitCellDevModel,
		 (ClientData) 0,
		 (Tcl_CmdDeleteProc *) NULL,
		 module,
		 tclFitCellDevModel_hlp, tclFitCellDevModel_use);
    shTclDeclare(interp, "fitCellExpModel",
		 (Tcl_CmdProc *)tclFitCellExpModel,
		 (ClientData) 0,
		 (Tcl_CmdDeleteProc *) NULL,
		 module,
		 tclFitCellExpModel_hlp, tclFitCellExpModel_use);


   shTclDeclare(interp,"regIntRotate",
		(Tcl_CmdProc *)tclRegIntRotate, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegIntRotate_hlp,
		tclRegIntRotate_use);

   shTclDeclare(interp,"profileTableHeaderPrint",
		(Tcl_CmdProc *)tclProfileTableHeaderPrint, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclProfileTableHeaderPrint_hlp,
		tclProfileTableHeaderPrint_use);

   shTclDeclare(interp,"profileTableHeaderCheck",
		(Tcl_CmdProc *)tclProfileTableHeaderCheck, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclProfileTableHeaderCheck_hlp,
		tclProfileTableHeaderCheck_use);

   shTclDeclare(interp,"fitCellChisq",
		(Tcl_CmdProc *)tclFitCellChisq, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFitCellChisq_hlp,
		tclFitCellChisq_use);

   shTclDeclare(interp,"residualsFind",
		(Tcl_CmdProc *)tclResidualsFind, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclResidualsFind_hlp,
		tclResidualsFind_use);

   shTclDeclare(interp,"fitPsfFromReg",
		(Tcl_CmdProc *)tclFitPsfFromReg, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFitPsfFromReg_hlp,
		tclFitPsfFromReg_use);

   shTclDeclare(interp,"profileTableHashGet",
		(Tcl_CmdProc *)tclProfileTableHashGet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclProfileTableHashGet_hlp,
		tclProfileTableHashGet_use);

   shTclDeclare(interp,fitCellColorSet_name,
		(Tcl_CmdProc *)tclFitCellColorSet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFitCellColorSet_hlp,
		tclFitCellColorSet_use);

   shTclDeclare(interp,fittedModelMake_name,
		(Tcl_CmdProc *)tclFittedModelMake, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFittedModelMake_hlp,
		tclFittedModelMake_use);

   shTclDeclare(interp,makePsfModel_name,
		(Tcl_CmdProc *)tclMakePsfModel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclMakePsfModel_hlp,
		tclMakePsfModel_use);

   shTclDeclare(interp,compareProfileFiles_name,
		(Tcl_CmdProc *)tclCompareProfileFiles, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclCompareProfileFiles_hlp,
		tclCompareProfileFiles_use);

   shTclDeclare(interp,makePowerLawModel_name,
		(Tcl_CmdProc *)tclMakePowerLawModel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclMakePowerLawModel_hlp,
		tclMakePowerLawModel_use);
}
