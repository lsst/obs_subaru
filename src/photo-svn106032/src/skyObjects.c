/*
 * <INTRO>
 *
 * This file provides support for cutting sky objects out of a frame
 */
#include "dervish.h"
#include "phObjc.h"
#include "phRandom.h"

/*
 * Add nobj sky objects to the end of the chain of OBJCs
 */
static void
find_skyobjects(int nobj,		/* number of objects desired */
		CHAIN *objects,		/* chain of merged objects */
		int ncolor,		/* number of filters in use */
		int ref_band_index,	/* canonical colour */
		int nrow,		/* no. of rows in corrected frame */
		int ncol,		/* no. of cols in corrected frame */
		int objsize,	        /* width/height of sky object in pixels */
		int nreject_max	        /* no. of trial sky objects that can fail. */
		)
{
   CURSOR_T crsr;
   int nreject;				/* number of rejected sky objects */
   int nsky;				/* number of sky objects found */
   char *objc_type = (char *)shTypeGetFromName("OBJC");
   OBJC *objc;
   OBJMASK *om;
   int rmin, rmax, cmin, cmax;		/* corners of object */

   shAssert(objects != NULL && objects->type == (TYPE)objc_type);

   crsr = shChainCursorNew(objects);
   for(nsky = nreject = 0; nreject < nreject_max && nsky < nobj; ) {
      rmin = ((nrow - objsize)*(0.5 + phRandom_u16()/(float)MAX_U16) + 0.5);
      cmin = ((ncol - objsize)*(0.5 + phRandom_u16()/(float)MAX_U16) + 0.5);
      rmax = rmin + objsize - 1;
      cmax = cmin + objsize - 1;

      shChainCursorSet(objects, crsr, HEAD);
      while((objc = (OBJC *)shChainWalk(objects, crsr, NEXT)) != NULL) {
	 om = objc->aimage->master_mask;
	 if((cmax >= om->cmin && cmin <= om->cmax) &&
	    (rmax >= om->rmin && rmin <= om->rmax)) {
	    break;			/* they intersect */
	 }
      }
      if(objc != NULL) {		/* prospective sky object intersects
					   at least one real object */
	 nreject++;
	 continue;
      }
/*
 * OK, we have a valid sky position. Now we have to make a valid OBJC to
 * fill it. It has to have at least one valid OBJECT1, which we take to be
 * the canonical colour so as not to worry about coordinate transformations
 */
      objc = phObjcNew(ncolor);
      objc->aimage->master_mask = phObjmaskFromRect(cmin, rmin, cmax, rmax);
      objc->type = OBJ_SKY;
      objc->rowc = (rmin + rmax)/2.0;
      objc->colc = (cmin + cmax)/2.0;
      objc->rowcErr = objc->colcErr = 0;
      objc->flags3 |= OBJECT3_HAS_CENTER;

      objc->color[ref_band_index] = phObject1New();
      objc->color[ref_band_index]->type = OBJ_SKY;
      objc->color[ref_band_index]->rowc = objc->rowc;
      objc->color[ref_band_index]->colc = objc->colc;
      objc->color[ref_band_index]->rowcErr = 0;
      objc->color[ref_band_index]->colcErr = 0;

      objc->color[ref_band_index]->peaks = phPeaksNew(1);
      objc->color[ref_band_index]->peaks->peaks[0]->rowc = objc->rowc;
      objc->color[ref_band_index]->peaks->peaks[0]->colc = objc->colc;
      objc->color[ref_band_index]->peaks->npeak++;

      shChainElementAddByPos(objects, objc, objc_type, TAIL, AFTER);

      nsky++;
   }
   shChainCursorDel(objects, crsr);

   if(nsky < nobj) {
      shError("find_skyobjects: "
	      "only found %d sky objects (rejected %d candidates)",
								 nsky, nreject);
   }
}

/*****************************************************************************/
/*
 * Now for the TCL interface
 */
#include "dervish.h"
#include "phUtils.h"

static char *module = "phTclSkyObjects"; /* name of this set of code */

/*
 * Variables used by ftcl_ParseArgv in parsing argument lists.
 *
 * You don't _have_ to use these, but doing so will minimise the number
 * of extern static variables in this file
 */
static int ival0 = 0, ival1 = 0, ival2 = 0, ival3 = 0, ival4 = 0;
static int nfail = 0;
static int skyobj_size = 0;
static char *sval0 = NULL;

static char *tclSkyObjectsFind_use =	
  "USAGE: skyObjectsFind <ncolor> <nobj> <objects> <fiber_color> <nrow> <ncol> [-nfail N -size N]";
#define tclSkyObjectsFind_hlp \
  "Create <nobj> sky objects and add them to the end of the OBJC chain "\
"<objects>. The OBJCs contain ncolor OBJECT1s, and live in an <nrow>x<ncol> "\
"sized region. Will stop trying after 10*nobj failures, unless overridden with "\
"the -nfail option. The sky object width&height can be set to an odd value "\
"with -size, and is 45 pixels by default."

static ftclArgvInfo skyObjectsFind_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclSkyObjectsFind_hlp},
   {"<nobj>", FTCL_ARGV_INT, NULL, &ival0,
					  "The desired number of sky objects"},
   {"<objects>", FTCL_ARGV_STRING, NULL, &sval0, "Chain of OBJCs"},
   {"<ncolor>", FTCL_ARGV_INT, NULL, &ival1, "Number of colours in the OBJCs"},
   {"<fiber_color>", FTCL_ARGV_INT, NULL, &ival2,
				   "Index of canonical colour (typically r')"},
   {"<nrow>", FTCL_ARGV_INT, NULL, &ival3, "Number of rows in parent region"},
   {"<ncol>", FTCL_ARGV_INT, NULL, &ival4,
					 "Number of columns in parent region"},
   {"-nfail", FTCL_ARGV_INT, NULL, &nfail,
					 "How many locations to fail at before giving up (default=10*nobj)"},
   {"-size", FTCL_ARGV_INT, NULL, &skyobj_size,
					 "Width/height of sky objects (default=45, must be odd)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclSkyObjectsFind(
		  ClientData clientDatag,
		  Tcl_Interp *interp,
		  int ac,
		  char **av
		  )
{
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */

   shErrStackClear();

   nfail = 0;			/* Flag for using backward-compatibile value. */
   skyobj_size = 45;		/* Backward-compatibility default. */
   if(get_FtclOpts(interp,&ac,av,skyObjectsFind_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,sval0,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_SetResult(interp,"skyObjectsFind: "
                    "second argument is not a CHAIN",TCL_STATIC);
      return(TCL_ERROR);
   }
   if(shChainTypeGet(hand.ptr) != shTypeGetFromName("OBJC")) {
      Tcl_SetResult(interp,"skyObjectsFind: "
                    "Chain is not of type OBJC",TCL_STATIC);
      return(TCL_ERROR);
   }
   if(skyobj_size < 1 || skyobj_size % 2 == 0) {
      Tcl_SetResult(interp,"skyObjectsFind: "
                    "size argument must be a positive and odd.",TCL_STATIC);
      return(TCL_ERROR);
   }

   if (nfail <= 0) nfail = ival0*10; /* By default, try up to 10*nobj locations */
/*
 * work
 */
   find_skyobjects(ival0, hand.ptr, ival1, ival2, ival3, ival4, 
		   skyobj_size, nfail);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phSkyObjectsDeclare(Tcl_Interp *interp)
{

   shTclDeclare(interp,"skyObjectsFind",
		(Tcl_CmdProc *)tclSkyObjectsFind, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclSkyObjectsFind_hlp,
		tclSkyObjectsFind_use);
}
