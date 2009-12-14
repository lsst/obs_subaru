/*
 * TCL support for makeStarlist module
 *
 * ENTRY POINT			SCOPE	DESCRIPTION
 * tclMakeStarlistDeclare	public	Declare all the verbs defined in 
 *					this module
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "phUtils.h"
#include "phStar1.h"
#include "phStar1c.h"
#include "phRandom.h"

static char *module = "phMakeStarlist";    /* name of this set of code */

#define DEBUG 1
#define NODEBUG 0
#define NOREG 0
#define YESREG 1
#define MAXLISTLENGTH 500

static int
set_filternames(char *str, char ***names, int *ncolor);
static void
free_names(char **names, int numnames);

static int nframes = 0;
static int nstars = 0;
static int amp = 1000;
static int stamp_size = 0;
static double change = 0;
static char *psfstr = NULL;
static char *starstr = NULL;
static char *fpsfstr = NULL;

static char *tclMakeTestStarlist_use =
  "USAGE: makeTestStarlist nframes stars_per_frame starlist truepsfs framepsfs";
static char *tclMakeTestStarlist_hlp =
  "Make a starlist from a set of simulated stars with psf's varying according to a set function.";

static ftclArgvInfo makeTestStarlist_argTable[] = {
   { (char *) NULL, FTCL_ARGV_HELP, NULL, NULL,
         "Make list of STAR1s from faked postage stamps with good psf fits" },
   { "<nframes>", FTCL_ARGV_INT, NULL, &nframes,
         "number of frames with postage stamps" },
   { "<stars_per_frame>", FTCL_ARGV_INT, NULL, &nstars,
         "average number of stars per frame" },
   { "<amp>", FTCL_ARGV_INT, NULL, &amp,
         "central intensity of stars" },
   { "<%change>", FTCL_ARGV_DOUBLE, NULL, &change,
         "amount by which psf width will vary over the whole set of frames" },
   { "<starlist>", FTCL_ARGV_STRING, NULL, &starstr,
         "chain to hold star1s" },
   { "<starpsfs>", FTCL_ARGV_STRING, NULL, &psfstr,
         "chain to hold true psfs corresponding to stars on starlist" },
   { "<framepsfs>", FTCL_ARGV_STRING, NULL, &fpsfstr,
         "chain to hold true psfs corresponding frame centers" },
   { "<stamp_size>", FTCL_ARGV_INT, NULL, &stamp_size,
         "chain to hold true psfs corresponding to stars on starlist" },
   { (char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL }
};


static int
tclMakeTestStarlist(
          ClientData clientData,

          Tcl_Interp *interp,
          int argc,
          char **argv
		)
{

  CHAIN *star1list=NULL, *psflist=NULL, *framepsfs=NULL;
  
   /* 
    * we have to set this by hand upon entering function; otherwise,
    * it retains value that user last set.
    */
  shErrStackClear();

  if (!phRandomIsInitialised()) {
      shError("tclMakeTestStarlist: random numbers not initialized");
      return(TCL_ERROR);
  }
  if (!phProfileIsInitialised()) {
      shError("tclMakeTestStarlist: profile extraction not initialized");
      return(TCL_ERROR);
  }

  if (get_FtclOpts(interp, &argc, argv, makeTestStarlist_argTable) != TCL_OK) {
      return(TCL_ERROR);
  }

  if (shTclAddrGetFromName(interp, starstr, (void **)&star1list, "CHAIN") 
      != TCL_OK) {
      shError("tclMakeTestStarlist: cant get star1list");
      return (TCL_ERROR);
  } 
  if (shTclAddrGetFromName(interp, psfstr, (void **)&psflist, "CHAIN") 
      != TCL_OK) {
      shError("tclMakeStarlist: cant get trupsfs");
      return (TCL_ERROR);
  } 
  if (shTclAddrGetFromName(interp, fpsfstr, (void **)&framepsfs, "CHAIN") 
      != TCL_OK) {
      shError("tclMakeStarlist: cant get framepsfs");
      return (TCL_ERROR);
  } 

  if (phMakeTestStarlist(nframes, nstars, amp, change, star1list, psflist, 
			 framepsfs, stamp_size) 
      != SH_SUCCESS) {
      Tcl_SetResult(interp, "phMakeTestStarlist returns error",TCL_STATIC);
      return(TCL_ERROR);
  } 
  return(TCL_OK);

}

/*****************************************************************/

static char *star1cStr = NULL;
static double nsigma = 0;
static int niter = 0;
static int min_starnum = 0;
static int critical_amplitude = 0;
static char *filters;

static char *tclPsfReject_use = 
   "USAGE: psfReject star1clist filterlist nsigma niter min_starnum amp_crit";

static char *tclPsfReject_hlp = 
   "Do width and symmetry rejection on a chain of star1c's, flagging bad candidates";

static ftclArgvInfo psfReject_argTable[] = {
  { (char *)NULL, FTCL_ARGV_HELP, NULL, NULL,
      "do rough width rejection on a CHAIN of star1s" },
  { "<star1clist>", FTCL_ARGV_STRING, NULL, &star1cStr,
      "input chain of STAR1Cs" },
  { "<filterlist>", FTCL_ARGV_STRING, NULL, &filters, 
      "filter list" },
  { "<nsigma>", FTCL_ARGV_DOUBLE, NULL, &nsigma,
					"Number of sigma to clip widths at" }, 
  { "<niter>", FTCL_ARGV_INT, NULL, &niter,
				     "number of times to iterate sigma clip" },
  { "<min_starnum>", FTCL_ARGV_INT, NULL, &min_starnum,
 "Minimum number of quality stars we need to do meaningful width rejection." },
  { "<critical_amplitude>", FTCL_ARGV_INT, NULL, &critical_amplitude,
      "minimum DGPSF->a value for a bad width to condemn all bands" },
  { (char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL }
};

static int 
tclPsfReject(
          ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv)
{
   HANDLE hand;
   CHAIN *star1clist = NULL;
   void *vptr;				/* used by shTclHandleExprEval */
   char string_filters[MAXLISTLENGTH];
   char **filternames=NULL;
   int ncolors = 0l;

   shErrStackClear();
   if (get_FtclOpts(interp, &argc, argv, psfReject_argTable) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * process arguments
 */
   if(shTclHandleExprEval(interp,star1cStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_SetResult(interp,"psfReject: argument is not a CHAIN",TCL_STATIC);
      return(TCL_ERROR);
   }
   star1clist = hand.ptr;

   if(star1clist->type != shTypeGetFromName("STAR1C")) {
      Tcl_SetResult(interp, "psfReject: chain isn't of STAR1Cs",TCL_STATIC);
      return(TCL_ERROR);
   } 
/* 
 * Convert Tcl list of filters to an array of strings
 */
    strncpy(string_filters, filters, MAXLISTLENGTH);
    
    if (set_filternames(string_filters, &filternames, &ncolors) != SH_SUCCESS) {
	Tcl_SetResult(interp, "can't get filternames", TCL_STATIC);
	return(TCL_ERROR);
    }
/*
 * do the work
 */
   if (phPsfReject(star1clist, filternames, nsigma, niter, min_starnum, critical_amplitude)
							       != SH_SUCCESS) {
      Tcl_SetResult(interp, "phPsfReject returns error",TCL_STATIC);
      return(TCL_ERROR);
   } else {
      if(filternames != NULL) free_names(filternames,ncolors); 
   }
   
   return (TCL_OK);
}

/*****************************************************************/

static char *st1cstr = NULL;
static char *ststr = NULL;
static char *rejectstr = NULL;
static int nfilter = 0;
static int first_frame = 0;
static int last_frame = 0;
static STAR1_FLAGS star1c_require_mask = STAR1_NOFLAGS;
static STAR1_FLAGS star1c_ignore_mask = STAR1_NOFLAGS;
static STAR1_FLAGS star1_require_mask = STAR1_NOFLAGS;
static STAR1_FLAGS star1_ignore_mask = STAR1_NOFLAGS;

static char *tclMakeStarlist_use = 
   "USAGE: makeStarlist star1clist starlist ps_rejects nfilter first_frame last_frame star1c_require_mask star1c_ignore_mask star1_require_mask star1_ignore_mask";

static char *tclMakeStarlist_hlp = 
   "create Chains of star1s for each filter from the flagged chain of star1c's";

static ftclArgvInfo makeStarlist_argTable[] = {
   { (char *)NULL, FTCL_ARGV_HELP, NULL, NULL,
        "" },
   { "<star1clist>", FTCL_ARGV_STRING, NULL, &st1cstr,
        "Input chain of Star1cs" },
   { "<starlist>", FTCL_ARGV_STRING, NULL, &ststr,
        "Chain containing accepted stars" },
   { "<ps_rejects>", FTCL_ARGV_STRING, NULL, &rejectstr,
        "Chain containing rejected stars" },
   { "<nfilter>", FTCL_ARGV_INT, NULL, &nfilter,
         "Code for filter number u=0, g=1, r=2, i=3, z=4" },
   { "<first_frame>", FTCL_ARGV_INT, NULL, &first_frame,
         "Start from this position in *star1clist" },
   { "<last_frame>", FTCL_ARGV_INT, NULL, &last_frame,
         "End at this position in *star1clist" },
   { "<star1c_require_mask>", FTCL_ARGV_INT, NULL, &star1c_require_mask,
	 "Require that STAR1C->flags have these bits set" },
   { "<star1c_ignore_mask>", FTCL_ARGV_INT, NULL, &star1c_ignore_mask,
	 "Ignore these bits when rejecting on STAR1C->flags" },
   { "<star1_require_mask>", FTCL_ARGV_INT, NULL, &star1_require_mask,
	 "Require that STAR1->flags have these bits set" },
   { "<star1_ignore_mask>", FTCL_ARGV_INT, NULL, &star1_ignore_mask,
	 "Ignore these bits when rejecting on STAR1->flags" },
   { (char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL }
};

static int 
tclMakeStarlist(
          ClientData clientData,

	  Tcl_Interp *interp,
	  int argc,
	  char **argv)
{

  CHAIN *star1clist = NULL, *starlist = NULL, *ps_rejects = NULL;
  star1c_require_mask = star1c_ignore_mask = STAR1_NOFLAGS;
  star1_require_mask = star1_ignore_mask = STAR1_NOFLAGS;

  shErrStackClear();
  if (get_FtclOpts(interp, &argc, argv, makeStarlist_argTable) != TCL_OK) {
    return(TCL_ERROR);
  }
  
  if (shTclAddrGetFromName(interp, st1cstr, (void **)&star1clist, "CHAIN") 
      != TCL_OK) {
    shError("tclMakeStarlist: can't get star1clist");
    return (TCL_ERROR);
  } 
  if (shTclAddrGetFromName(interp, ststr, (void **)&starlist, "CHAIN") 
      != TCL_OK) {
    shError("tclMakeStarlist: can't get starlist");
    return (TCL_ERROR);
  }
  if (shTclAddrGetFromName(interp, rejectstr, (void **)&ps_rejects, "CHAIN") 
      != TCL_OK) {
    shError("tclMakeStarlist: can't get ps_rejects");
    return (TCL_ERROR);
  }  

  if (phMakeStarlist(star1clist, starlist, ps_rejects, nfilter,
		     first_frame, last_frame,
		     star1c_require_mask, star1c_ignore_mask,
		     star1_require_mask, star1_ignore_mask) != SH_SUCCESS) {
    Tcl_SetResult(interp, "phMakeStarlist returns error",TCL_STATIC);
    return(TCL_ERROR);
  }
  
  return (TCL_OK);
  
}


/*****************************************************************/

static char *tclMakeStarPClist_use = 
   "USAGE: makeStarPClist star1clist starPClist rejects nfilter first_frame last_frame star1_ignore_mask";

static char *tclMakeStarPClist_hlp = 
   "create chains of STAR1PCs for each filter from the flagged chain of star1c's";

static ftclArgvInfo makeStarPClist_argTable[] = {
   { (char *)NULL, FTCL_ARGV_HELP, NULL, NULL,
        "" },
   { "<star1clist>", FTCL_ARGV_STRING, NULL, &st1cstr,
        "Input chain of Star1cs" },
   { "<starPClist>", FTCL_ARGV_STRING, NULL, &ststr,
        "Chain containing accepted stars" },
   { "<rejects>", FTCL_ARGV_STRING, NULL, &rejectstr,
        "Chain containing rejected stars" },
   { "<nfilter>", FTCL_ARGV_INT, NULL, &nfilter,
         "Code for filter number u=0, g=1, r=2, i=3, z=3" },
   { "<first_frame>", FTCL_ARGV_INT, NULL, &first_frame,
         "Start from this position in *star1clist" },
   { "<last_frame>", FTCL_ARGV_INT, NULL, &last_frame,
         "End at this position in *star1clist" },
   { "<star1_ignore_mask>", FTCL_ARGV_INT, NULL, &star1_ignore_mask,
	 "Ignore these flags in the rejection process)" },
   { (char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL }
};

static int 
tclMakeStarPClist(
          ClientData clientData,

	  Tcl_Interp *interp,
	  int argc,
	  char **argv)
{

  CHAIN *star1clist = NULL, *starPClist = NULL, *rejects = NULL;
  star1_ignore_mask = STAR1_NOFLAGS;

  shErrStackClear();
  if (get_FtclOpts(interp, &argc, argv, makeStarPClist_argTable) != TCL_OK) {
    return(TCL_ERROR);
  }
  
  if (shTclAddrGetFromName(interp, st1cstr, (void **)&star1clist, "CHAIN") 
      != TCL_OK) {
    shError("tclMakeStarPClist: can't get star1clist");
    return (TCL_ERROR);
  } 
  if (shTclAddrGetFromName(interp, ststr, (void **)&starPClist, "CHAIN") 
      != TCL_OK) {
    shError("tclMakeStarPClist: can't get starPClist");
    return (TCL_ERROR);
  }
  if (shTclAddrGetFromName(interp, rejectstr, (void **)&rejects, "CHAIN") 
      != TCL_OK) {
    shError("tclMakeStarPClist: can't get rejects");
    return (TCL_ERROR);
  }  

  if (phMakeStarPClist(star1clist, starPClist, rejects, nfilter, first_frame, last_frame, star1_ignore_mask) != SH_SUCCESS) {
    Tcl_SetResult(interp, "phMakeStarPClist returns error",TCL_STATIC);
    return(TCL_ERROR);
  }
  
  return (TCL_OK);
  
}

/*****************************************************************
 * Declare my new tcl verbs to tcl
 */
void
phTclMakeStarlistDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"makeTestStarlist",
                (Tcl_CmdProc *)tclMakeTestStarlist,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclMakeTestStarlist_hlp, tclMakeTestStarlist_use);
   shTclDeclare(interp,"psfReject",
                (Tcl_CmdProc *)tclPsfReject,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclPsfReject_hlp, tclPsfReject_use);
   shTclDeclare(interp,"makeStarlist",
                (Tcl_CmdProc *)tclMakeStarlist,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclMakeStarlist_hlp, tclMakeStarlist_use);
   shTclDeclare(interp,"makeStarPClist",
                (Tcl_CmdProc *)tclMakeStarPClist,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclMakeStarPClist_hlp, tclMakeStarPClist_use);
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

