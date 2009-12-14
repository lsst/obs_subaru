/*
 * TCL support for star1c modules
 *
 * ENTRY POINT			SCOPE	DESCRIPTION
 * tclStar1cDeclare	        public	Declare all the verbs defined in 
 *					this module
 *
 * REQUIRED PRODUCTS:
 *	FTCL		TCL + XTCL + Fermilab extensions
 *	DERVISH		Survey Human Interface and Visualization
 *
 *   Authors: Rob Simcoe
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "phUtils.h"
#include "phStar1.h"
#include "phStar1c.h"
#include "phRandom.h"
#include "phConsts.h"

static char *module = "phStar1c";    /* name of this set of code */

/*****************************************************************************/
static char *star1cstr = NULL;
static char *masksstr = NULL;
static char *psstr = NULL;
static int id0 = 0;
static int frame = -1;
static char *flatstr = NULL;
static char *biasstr = NULL;
static char *ccdparstr = NULL;
static char *calibstr = NULL;
static int stamp_size = 0;
static int wing = 0;
static int aperture = 0;
static int psf_def_rad = 0;
static double cr_min_s = 6.0;
static int cr_min_e = 150;
static double cr_cond3 = 3.0; 
static double cr_cond32 = 0.6; 
static int bscale = -1;
static int reg_flag = 0;
static int ifilter = 0;
static int nfilters = 0;
static int threshold = 0;
static double sigma_multi = 5.0; 
static double sigma_badsky = 2.2; 
static double second_peak = 0.044;
static char *fparams_s;
static int sym_flag = 0;

static char *tclMakeStar1clist_use =
  "USAGE: makeStar1clist star1clist pstamps ifilter nfilters id0 frame flat bias ccdpars bscale calib stampsize sigma_multi sigma_badsky second_peak wing aperture psf_def_rad cr_min_s cr_min_e cr_cond3 cr_cond32 fparams threshold -symdiag -region";
static char *tclMakeStar1clist_hlp =
  "Make a list of star1c's from list of raw postage stamps";

static ftclArgvInfo makeStar1clist_argTable[] = {
   { (char *) NULL, FTCL_ARGV_HELP, NULL, NULL,
         "Make list of STAR1Cs from input postage stampe with good psf fits" },
   { "<star1clist>", FTCL_ARGV_STRING, NULL, &star1cstr,
         "Chain to hold Star1c's" },
   { "<masks>", FTCL_ARGV_STRING, NULL, &masksstr,
         "Chain to hold masks corresponding to Star1c's" },
   { "<pstamps>", FTCL_ARGV_STRING, NULL, &psstr,
         "chain containing raw postage stamps" },
   { "<ifilter>", FTCL_ARGV_INT, NULL, &ifilter,
         "filter number (u=0, g=1, r=2, i=3, z=4)" },
   { "<nfilters>", FTCL_ARGV_INT, NULL, &nfilters,
         "Number of colors to be run through the pipeline" },
   { "<id0>", FTCL_ARGV_INT, NULL, &id0,
         "starting ID number for the postage stamps" },
   { "<frame>", FTCL_ARGV_INT, NULL, &frame,
         "frame containing the postage stamps" },
   { "<flat>", FTCL_ARGV_STRING, NULL, &flatstr,
         "flat vector for frame" },
   { "<bias>", FTCL_ARGV_STRING, NULL, &biasstr,
         "bias vector" },
   { "<ccdpars>", FTCL_ARGV_STRING, NULL, &ccdparstr,
         "ccdpars for fram" },
   { "<bscale>", FTCL_ARGV_INT, NULL, &bscale,
         "bias scale factor" },
   { "<calib>", FTCL_ARGV_STRING, NULL, &calibstr,
         "ccdpars for fram" },
   { "<stamp_size>", FTCL_ARGV_INT, NULL, &stamp_size,
         "size of input postage stamps" },
   { "<sigma_multi>", FTCL_ARGV_DOUBLE, NULL, &sigma_multi,
         "number of sky sigma to trigger multiple flag" },
   { "<sigma_badsky>", FTCL_ARGV_DOUBLE, NULL, &sigma_badsky,
         "number of sigma for bad sky" },
   { "<second_peak>", FTCL_ARGV_DOUBLE, NULL, &second_peak,
         "max. allowed strength for 2nd peak" },
   { "<wing>", FTCL_ARGV_INT, NULL, &wing,
         "1 if reducing wing stars (for bypassing an assertion)" },
   { "<aperture>", FTCL_ARGV_INT, NULL, &aperture,
         "index of profile aperture for photometry" },
   { "<psf_def_rad>", FTCL_ARGV_INT, NULL, &psf_def_rad,
         "radius out to which data used to create PSF" },
   { "<cr_min_s>", FTCL_ARGV_DOUBLE, NULL, &cr_min_s,
         "CRs must be > this many sky-sig above sky" },
   { "<cr_min_e>", FTCL_ARGV_INT, NULL, &cr_min_e,
         "CRs must have > this many electrons in peak pixel" },
   { "<cr_cond3>", FTCL_ARGV_DOUBLE, NULL, &cr_cond3,
         "used in condition 3 for CR; see CR.c code" },
   { "<cr_cond32>", FTCL_ARGV_DOUBLE, NULL, &cr_cond32,
         "used in condition 3 for CR; see CR.c code" },
   { "<fparams>", FTCL_ARGV_STRING, NULL, &fparams_s,
         "FRAMEPARAMS with gain, software bias, etc." },
   { "<threshold>", FTCL_ARGV_INT, NULL, &threshold,
         "threshold to use for object finder in symmetry testing" },
   { "-keep_symmetry", FTCL_ARGV_CONSTANT, (void *)1, &sym_flag,
	 "keep diagnostic information about star asymmetry?" },
   { "-region", FTCL_ARGV_CONSTANT, (void *)1, &reg_flag,
	 "Store postage stamp in star1" },
   { (char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL }
};

static int
tclMakeStar1clist(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
		)
{
  HANDLE hand;
  void *vptr;				/* used by shTclHandleExprEval */
  FRAMEPARAMS *fparams;
  CHAIN *star1clist=NULL, *masks=NULL, *pstamps=NULL;
  REGION *flat=NULL, *bias=NULL;
  CCDPARS *ccdpar=NULL; 
  CALIB1 *calib=NULL;

  reg_flag = 0;
  sym_flag = 0;

  shErrStackClear();
  if (get_FtclOpts(interp, &argc, argv, makeStar1clist_argTable) != TCL_OK) {
      return(TCL_ERROR);
  }

  if (shTclAddrGetFromName(interp, star1cstr, (void **)&star1clist, "CHAIN") 
      != TCL_OK) {
      shError("tclMakeStar1clist: cant get star1clist");
      return (TCL_ERROR);
  } 
  if (shTclAddrGetFromName(interp, masksstr, (void **)&masks, "CHAIN") 
      != TCL_OK) {
      shError("tclMakeStar1clist: cant get masks");
      return (TCL_ERROR);
  } 
  if (shTclAddrGetFromName(interp, psstr, (void **)&pstamps, "CHAIN") 
      != TCL_OK) {
      shError("tclMakeStar1clist: can't get postage stamps");
      return (TCL_ERROR);
  } 
  if (shTclRegAddrGetFromName(interp, flatstr, &flat) != TCL_OK) {
      shError("tclMakeStar1clist: can't get flat vector");
      return (TCL_ERROR);
  }
  if (shTclRegAddrGetFromName(interp, biasstr, &bias) != TCL_OK) {
      shError("tclMakeStar1clist: can't get bias vector");  
      return (TCL_ERROR);
  }
  if (shTclAddrGetFromName(interp, ccdparstr, (void **)&ccdpar, "CCDPARS") 
      != TCL_OK) {
      shError("tclMakeStar1clist: can't get ccdpars");
      return (TCL_ERROR);
  } 
  if (shTclAddrGetFromName(interp, calibstr, (void **)&calib, "CALIB1") 
      != TCL_OK) {
      shError("tclMakeStar1clist: can't get calib1");
      return (TCL_ERROR);
  }
  if(shTclHandleExprEval(interp,fparams_s,&hand,&vptr) != TCL_OK) {
     return(TCL_ERROR);
  }
  if(hand.type != shTypeGetFromName("FRAMEPARAMS")) {
     Tcl_SetResult(interp,"makeStar1clist: "
		   "argument is not a FRAMEPARAMS",TCL_STATIC);
     return(TCL_ERROR);
  }
  fparams = hand.ptr;
  
  if (phMakeStar1clist(star1clist,masks,pstamps,ifilter,nfilters,id0,frame,flat,bias,
		       ccdpar,bscale,calib,stamp_size,aperture,psf_def_rad,cr_min_s,  
		       cr_min_e,cr_cond3,cr_cond32,fparams,threshold,sigma_multi,
                       sigma_badsky,second_peak,wing,sym_flag,reg_flag) != SH_SUCCESS) {
      Tcl_SetResult(interp, "phMakeStar1clist returns error",TCL_STATIC);
      return(TCL_ERROR);
  } 

  return(TCL_OK);

}
/***************************************************************************/
/*
 * return a handle to a new STAR1C
 */

static int ncolors = 0;

static char *tclStar1cNew_use =
  "USAGE: star1cNew";
static char *tclStar1cNew_hlp =
  "create a new STAR1C";

static ftclArgvInfo star1cNew_argTable[] = {
   { "<ncolors>", FTCL_ARGV_INT, NULL, &ncolors,
         "Number of colors for which we should allocate space for a *STAR1" },
   { (char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL }
};


static int
tclStar1cNew(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE handle;
   char name[HANDLE_NAMELEN];

   shErrStackClear();
   if (get_FtclOpts(interp, &argc, argv, star1cNew_argTable) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * ok, get a handle for our new STAR1C
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   handle.ptr = phStar1cNew(ncolors);
   handle.type = shTypeGetFromName("STAR1C");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new star1c handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete a STAR1C
 */

static char *tclStar1cDel_use =
  "USAGE: star1cDel star1c";
static char *tclStar1cDel_hlp =
  "Delete a STAR1C";

static int
tclStar1cDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *star1c;
   char *opts = "star1c";

   shErrStackClear();

   ftclParseSave("star1cDel");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      star1c = ftclGetStr("star1c");
      if(p_shTclHandleAddrGet(interp,star1c,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclStar1cDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phStar1cDel(handle->ptr);
   (void) p_shTclHandleDel(interp,star1c);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}

/*****************************************************************
 * Declare my new tcl verbs to tcl
 */
void
phTclStar1cDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"makeStar1clist",
                (Tcl_CmdProc *)tclMakeStar1clist,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclMakeStar1clist_hlp, tclMakeStar1clist_use);
   shTclDeclare(interp,"star1cNew",
                (Tcl_CmdProc *)tclStar1cNew,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclStar1cNew_hlp, tclStar1cNew_use);
   shTclDeclare(interp,"star1cDel",
                (Tcl_CmdProc *)tclStar1cDel,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclStar1cDel_hlp, tclStar1cDel_use);
}
