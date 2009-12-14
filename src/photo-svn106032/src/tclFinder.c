/******************************************************************************
 * find isophotes of objects in a region above a given threshold
 */
#include <string.h>
#include "dervish.h"
#include "phUtils.h"
#include "phObjects.h"
#include "phMeasureObj.h"

static char *module = "tclRegFinder";

/*****************************************************************************/
/*
 * A utility function to do a deep deletion of a chain of OBJECTs
 */
static void
objectChainDel(CHAIN *chain)
{
   OBJECT *obj;
   CURSOR_T curs;
   
   curs = shChainCursorNew(chain);
   while((obj = shChainWalk(chain,curs,NEXT)) != NULL) {
      phObjectDel(obj);
   }
   shChainCursorDel(chain,curs);
   shChainDel(chain);
}

/*****************************************************************************/

static char *tclRegFinder_use = "USAGE: regFinder region levels fparams [-row0 #] [-col0 # ] [-row1 #] [-col1 #] -level ival";
#define tclRegFinder_hlp "Find objects by isophotes in part of REGION. "\
"fparams is a FRAMEPARAMS giving the sky level, gain, etc. If "\
"row0 and col0 are provided they are the bottom left corner of the searched "\
"area; the size is (nrow x ncol). Default is the whole REGION."\
"Returns a chain of objects."

static ftclArgvInfo regFinder_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegFinder_hlp},
   {"<region>", FTCL_ARGV_STRING, NULL, NULL, "Region to search"},
   {"<levels>", FTCL_ARGV_STRING, NULL, NULL, "vector of isophotal levels"},
   {"[fparams]", FTCL_ARGV_STRING, NULL, NULL, "FRAMEPARAMS (or \"\")"},
   {"-row0", FTCL_ARGV_INT, NULL, NULL, "Starting row to search"},
   {"-col0", FTCL_ARGV_INT, NULL, NULL, "Starting column to search"},
   {"-row1", FTCL_ARGV_INT, NULL, NULL,
			      "Last row to search (relative to nrow if <= 0)"},
   {"-col1", FTCL_ARGV_INT, NULL, NULL,
			      "Last col to search (relative to nrow if <= 0)"},
   {"-npixel_min_level", FTCL_ARGV_INT, NULL, NULL,
    "The level in the object that the npixel_min criterion applies to"},
   {"-npixel_min", FTCL_ARGV_INT, NULL, NULL,
				      "Minimum number of pixels in an object"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};


static int
tclRegFinder(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
    int a_i;
    FRAMEPARAMS *fparams;
    HANDLE hand;
    void *vptr;				/* used by shTclHandleExprEval */
    HANDLE *inputHandle = NULL;
    HANDLE *levelsHandle = NULL;
    HANDLE objHandle;
    REGION *input;
    VECTOR  *aLevels;
    char objName[HANDLE_NAMELEN];
    unsigned short *plevs;
    CHAIN *objChain;
    int i;
    char *regionStr = NULL;		/* Region to search */
    char *levelsStr = NULL;		/* vector of isophotal levels */
    char *fparamsStr = NULL;		/* FRAMEPARAMS */
    int row0 = 0;			/* Starting row to search */
    int col0 = 0;			/* Starting column to search */
    int row1 = 0;			/* last row to search */
    int col1 = 0;			/* last column to search */
    int npixel_min = 0;			/* Min number of pixels in an object
					   (<= 0 => no limit) */
    int npixel_min_level = 0;		/* The level in the object that the npixel_min criterion applies to */

    shErrStackClear();

    row0 = 0; col0 = 0;

    a_i = 1;
    regFinder_opts[a_i++].dst = &regionStr;
    regFinder_opts[a_i++].dst = &levelsStr;
    regFinder_opts[a_i++].dst = &fparamsStr;
    regFinder_opts[a_i++].dst = &row0;
    regFinder_opts[a_i++].dst = &col0;
    regFinder_opts[a_i++].dst = &row1;
    regFinder_opts[a_i++].dst = &col1;
    regFinder_opts[a_i++].dst = &npixel_min_level;
    regFinder_opts[a_i++].dst = &npixel_min;
    shAssert(regFinder_opts[a_i].type == FTCL_ARGV_END);

    if(get_FtclOpts(interp, &argc, argv, regFinder_opts) != TCL_OK) {
       return(TCL_ERROR);
    }
/*
 * Process the arguments
 */
    if(p_shTclHandleAddrGet(interp, regionStr, &inputHandle) != TCL_OK) {
       return(TCL_ERROR);
    }
    if(inputHandle->type != shTypeGetFromName("REGION")) {
       Tcl_SetResult(interp,"tclRegFinder: arg is not a REGION", TCL_STATIC);
       return(TCL_ERROR);
    }
    input = inputHandle->ptr;

    if (p_shTclHandleAddrGet(interp, levelsStr, &levelsHandle) != TCL_OK) {
	return(TCL_ERROR);
    }
    if(levelsHandle->type != shTypeGetFromName("VECTOR")) {
	Tcl_SetResult(interp,"regFinder: arg is not a VECTOR", TCL_STATIC);
	return (TCL_ERROR);
    }
    aLevels = levelsHandle->ptr;

    if(fparamsStr == NULL || fparamsStr[0] == '\0') {
       fparams = NULL;
    } else {
       if(shTclHandleExprEval(interp,fparamsStr,&hand,&vptr) != TCL_OK) {
	  return(TCL_ERROR);
       }
       if(hand.type != shTypeGetFromName("FRAMEPARAMS")) {
	  Tcl_SetResult(interp,"regFinder: "
			"argument is not a FRAMEPARAMS",TCL_STATIC);
	  return(TCL_ERROR);
       }
       fparams = hand.ptr;
    }
/*
 * Unpack array of of isophotal levels.
 */
    plevs = shMalloc(aLevels->dimen*sizeof(*plevs));
    for(i = 0; i < aLevels->dimen; i++) {
	plevs[i] = (unsigned short) aLevels->vec[i];
    }
/*
 * Hurrah! Do the work
 */
    objChain = phObjectsFind(input, row0, col0, row1, col1,
			     aLevels->dimen, plevs, fparams, npixel_min_level, npixel_min);
    shFree(plevs);
/*
 * Now pack up the results.
 */
   if(p_shTclHandleNew(interp, objName) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      objectChainDel(objChain);
      
      return(TCL_ERROR);
   }
   objHandle.ptr = objChain;
   objHandle.type = shTypeGetFromName("CHAIN");

   if (p_shTclHandleAddrBind(interp, objHandle, objName) != TCL_OK) {
      Tcl_SetResult(interp, "can't bind to new OBJECT handle", TCL_STATIC);
      objectChainDel(objChain);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, objName, TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete an object
 */
static char *tclObjectDel_use =
  "USAGE: ObjectDel obj";
#define tclObjectDel_hlp \
  "Delete and object"

static ftclArgvInfo objectDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjectDel_hlp},
   {"<obj>", FTCL_ARGV_STRING, NULL, NULL, "The object to delete"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjectDel(
	     ClientData clientDatag,
	     Tcl_Interp *interp,
	     int ac,
	     char **av
	     )
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *objStr = NULL;			/* The object to delete */

   shErrStackClear();

   i = 1;
   objectDel_opts[i++].dst = &objStr;
   shAssert(objectDel_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objectDel_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * process arguments
 */
   if(shTclHandleExprEval(interp,objStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJECT")) {
      Tcl_SetResult(interp,"objectDel: argument is not a OBJECT",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * and do the work
 */
   phObjectDel(hand.ptr);
   if(p_shTclHandleDel(interp, objStr) != TCL_OK)  {
      Tcl_SetResult(interp,"objectDel: cannot delete handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclSpanmaskSetFromObjectChain_use =
  "USAGE: SpanmaskSetFromObjectChain <chain> <mask> <value> <level>";
#define tclSpanmaskSetFromObjectChain_hlp \
  "add to the <mask> of type <VALUE> wherever the <level> in OBJECTs on"\
"the <chain> were detected"

static ftclArgvInfo spanmaskSetFromObjectChain_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclSpanmaskSetFromObjectChain_hlp},
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL, "Chain of OBJECTs"},
   {"<mask>", FTCL_ARGV_STRING, NULL, NULL, "Mask to set"},
   {"<value>", FTCL_ARGV_INT, NULL, NULL, "type of mask to OR with"},
   {"<level>", FTCL_ARGV_INT, NULL, NULL, "the index of the desired isophote"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclSpanmaskSetFromObjectChain(
			  ClientData clientDatag,
			  Tcl_Interp *interp,
			  int ac,
			  char **av
			  )
{
   int i;
   CHAIN *chain;			/* the chain of OBJECTs */
   HANDLE hand;
   SPANMASK *mask;				/* mask to set */
   OBJECT *obj;				/* first OBJECT on chain */
   void *vptr = NULL;			/* used by shTclHandleExprEval */
   char *chainStr = NULL;		/* Chain of OBJECTs */
   char *maskStr = NULL;		/* Mask to set */
   int value = 0;			/* type of mask to OR with */
   int level = 0;			/* the index of the desired isophote */

   shErrStackClear();


   i = 1;
   spanmaskSetFromObjectChain_opts[i++].dst = &chainStr;
   spanmaskSetFromObjectChain_opts[i++].dst = &maskStr;
   spanmaskSetFromObjectChain_opts[i++].dst = &value;
   spanmaskSetFromObjectChain_opts[i++].dst = &level;
   shAssert(spanmaskSetFromObjectChain_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,spanmaskSetFromObjectChain_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * Check and process arguments
 */
   if(shTclHandleExprEval(interp,chainStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_SetResult(interp,"spanmaskSetFromObjectChain: "
		    "first argument is not a CHAIN",TCL_STATIC);
      return(TCL_ERROR);
   }
   chain = hand.ptr;
   if(chain->type != shTypeGetFromName("OBJECT")) {
      Tcl_SetResult(interp,"spanmaskSetFromObjectChain: "
		   "CHAIN is not of type \"OBJECT\"",TCL_STATIC);
      return(TCL_ERROR);
   }

   obj = shChainElementGetByPos(chain,0);
#if 0
   if(level < 0 || ((obj != NULL) && (level > obj->nlevel))) {
      Tcl_SetResult(interp,"spanmaskSetFromObjectChain: "
		   "Specified level is not in first OBJECT",TCL_STATIC);
      return(TCL_ERROR);
   }
#else
   if(level < 0) {
      Tcl_SetResult(interp,"spanmaskSetFromObjectChain: "
		   "Specified level is not in first OBJECT",TCL_STATIC);
      return(TCL_ERROR);
   } else if(obj != NULL && level > obj->nlevel) {
      return(TCL_OK);			/* nothing to do */
   }
#endif

   if(shTclHandleExprEval(interp,maskStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("SPANMASK")
      && hand.type != shTypeGetFromName("MASK")) {
      Tcl_SetResult(interp,"spanmaskSetFromObjectChain: "
		    "second argument is not a MASK",TCL_STATIC);
      return(TCL_ERROR);
   }
   mask = hand.ptr;
/*
 * Do the work
 */
   phSpanmaskSetFromObjectChain(chain,mask,(S_MASKTYPE)value,level);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclSpanmaskSetFromObject1Chain_use =
  "USAGE: SpanmaskSetFromObject1Chain <chain> <mask> <value>";
#define tclSpanmaskSetFromObject1Chain_hlp \
  "add to the <mask> of type <VALUE> wherever the <level> in OBJECTs on"\
"the <chain> were detected"

static ftclArgvInfo spanmaskSetFromObject1Chain_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclSpanmaskSetFromObject1Chain_hlp},
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL, "Chain of OBJECT1s"},
   {"<mask>", FTCL_ARGV_STRING, NULL, NULL, "Mask to set"},
   {"<value>", FTCL_ARGV_INT, NULL, NULL, "type of mask to OR with"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclSpanmaskSetFromObject1Chain(
			  ClientData clientDatag,
			  Tcl_Interp *interp,
			  int ac,
			  char **av
			  )
{
   int i;
   CHAIN *chain;			/* the chain of OBJECTs */
   HANDLE hand;
   SPANMASK *mask;				/* mask to set */
   void *vptr = NULL;			/* used by shTclHandleExprEval */
   char *chainStr = NULL;		/* Chain of OBJECT1s */
   char *maskStr = NULL;		/* Mask to set */
   int value = 0;			/* type of mask to OR with */

   shErrStackClear();


   i = 1;
   spanmaskSetFromObject1Chain_opts[i++].dst = &chainStr;
   spanmaskSetFromObject1Chain_opts[i++].dst = &maskStr;
   spanmaskSetFromObject1Chain_opts[i++].dst = &value;
   shAssert(spanmaskSetFromObject1Chain_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,spanmaskSetFromObject1Chain_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * Check and process arguments
 */
   if(shTclHandleExprEval(interp,chainStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_SetResult(interp,"spanmaskSetFromObject1Chain: "
		    "first argument is not a CHAIN",TCL_STATIC);
      return(TCL_ERROR);
   }
   chain = hand.ptr;
   if(chain->type != shTypeGetFromName("OBJECT1")) {
      Tcl_SetResult(interp,"spanmaskSetFromObject1Chain: "
		   "CHAIN is not of type \"OBJECT1\"",TCL_STATIC);
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,maskStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("SPANMASK")
      && hand.type != shTypeGetFromName("MASK")) {
      Tcl_SetResult(interp,"spanmaskSetFromObject1Chain: "
		    "second argument is not a MASK",TCL_STATIC);
      return(TCL_ERROR);
   }
   mask = hand.ptr;
/*
 * Do the work
 */
   phSpanmaskSetFromObject1Chain(chain,mask,(S_MASKTYPE)value);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclObjectChainDel_use =
  "USAGE: ObjectChainDel <chain>";
#define tclObjectChainDel_hlp \
  "Delete a CHAIN of OBJECTs, including all of the OBJECTs"

static ftclArgvInfo objectChainDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjectChainDel_hlp},
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL, "Chain of OBJECTs"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjectChainDel(
		  ClientData clientDatag,
		  Tcl_Interp *interp,
		  int ac,
		  char **av
		  )
{
   int i;
   CHAIN *chain;			/* the chain of OBJECTs */
   HANDLE hand;
   void *vptr = NULL;			/* used by shTclHandleExprEval */
   char *chainStr = NULL;		/* Chain of OBJECTs */

   shErrStackClear();


   i = 1;
   objectChainDel_opts[i++].dst = &chainStr;
   shAssert(objectChainDel_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objectChainDel_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * Deal with arguments
 */
   if(shTclHandleExprEval(interp,chainStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_SetResult(interp,"maskSetFromObjectChain: "
		    "first argument is not a CHAIN",TCL_STATIC);
      return(TCL_ERROR);
   }
   chain = hand.ptr;
   if(chain->type != shTypeGetFromName("OBJECT")) {
      Tcl_SetResult(interp,"maskSetFromObjectChain: "
		    "CHAIN is not of type OBJECT",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * Work
 */
   objectChainDel(chain);
   p_shTclHandleDel(interp,chainStr);

   return(TCL_OK);
}


/*****************************************************************************/

static char *tclObjectToObject1ChainConvert_use =
  "USAGE: ObjectToObject1ChainConvert <chain> <level> <region> <sky>";
#define tclObjectToObject1ChainConvert_hlp \
  "Convert a CHAIN of OBJECTs to a chain of OBJECT1s, which is returned"

static ftclArgvInfo objectToObject1ChainConvert_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjectToObject1ChainConvert_hlp},
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL, "CHAIN of OBJECTs"},
   {"<level>", FTCL_ARGV_INT, NULL, NULL, "Index of desired level"},
   {"<region>", FTCL_ARGV_STRING, NULL, NULL, "Parent region"},
   {"<sky>", FTCL_ARGV_STRING, NULL, NULL, "Sky level region"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjectToObject1ChainConvert(
			       ClientData clientDatag,
			       Tcl_Interp *interp,
			       int ac,
			       char **av
			       )
{
   int i;
   char name[HANDLE_NAMELEN];
   BINREGION *binreg;			/* binned sky region */
   CHAIN *chain;			/* input OBJECT chain */
   CHAIN *chain1;			/* returned OBJECT1 chain */
   HANDLE hand;
   OBJECT *obj;				/* first OBJECT on chain */
   REGION *reg;				/* data REGION */
   void *vptr = NULL;			/* used by shTclHandleExprEval */
   char *chainStr = NULL;		/* CHAIN of OBJECTs */
   int level = 0;			/* Index of desired level */
   char *regionStr = NULL;		/* Parent region */
   char *skyStr = NULL;			/* Sky level region */
   
   shErrStackClear();


   i = 1;
   objectToObject1ChainConvert_opts[i++].dst = &chainStr;
   objectToObject1ChainConvert_opts[i++].dst = &level;
   objectToObject1ChainConvert_opts[i++].dst = &regionStr;
   objectToObject1ChainConvert_opts[i++].dst = &skyStr;
   shAssert(objectToObject1ChainConvert_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objectToObject1ChainConvert_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * Check arguments
 */
   if(shTclHandleExprEval(interp,chainStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_SetResult(interp,"objectToObject1ChainConvert: "
		    "first argument is not a CHAIN",TCL_STATIC);
      return(TCL_ERROR);
   }
   chain = hand.ptr;
   if(chain->type != shTypeGetFromName("OBJECT")) {
      Tcl_SetResult(interp,"objectToObject1ChainConvert: "
		   "CHAIN is not of type \"OBJECT\"",TCL_STATIC);
      return(TCL_ERROR);
   }

   /* 
    * If the given CHAIN of OBJECTs is empty, all we have to
    * do is return an empty CHAIN of OBJECT1s.  So, check for
    * this, and only do the real work if we have to.
    */
   if(shChainSize(chain) > 0) {
      obj = shChainElementGetByPos(chain,0);
      if(level < 0 || level > obj->nlevel) {
         Tcl_SetResult(interp,"objectToObject1ChainConvert: "
   		   "Specified level is not in first OBJECT",TCL_STATIC);
         return(TCL_ERROR);
      }

      if(shTclHandleExprEval(interp,regionStr,&hand,&vptr) != TCL_OK) {
         return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("REGION")) {
         Tcl_SetResult(interp,"objectToObject1ChainConvert: "
		    "third argument is not a REGION",TCL_STATIC);
         return(TCL_ERROR);
      }
      reg = hand.ptr;
   
      if(shTclHandleExprEval(interp,skyStr,&hand,&vptr) != TCL_OK) {
         return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("BINREGION")) {
         Tcl_SetResult(interp,"objectToObject1ChainConvert: "
		    "fourth argument is not a BINREGION",TCL_STATIC);
         return(TCL_ERROR);
      }
      binreg = hand.ptr;
   /*
    * Do the work
    */
      chain1 = phObjectToObject1ChainConvert(chain,level,reg,binreg);
   }
   else {
      /* 
       * in this case, input CHAIN of OBJECTs is empty.  So, just
       * create an empty CHAIN of OBJECT1s in response.
       */
      chain1 = shChainNew("OBJECT1");
   }

   /*
    * and return the answer
    */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = chain1;
   hand.type = shTypeGetFromName("CHAIN");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      OBJECT1 *obj1;
      CURSOR_T curs;
      Tcl_SetResult(interp,"can't bind to new OBJECT1 handle",TCL_STATIC);

      curs = shChainCursorNew(chain1);
      while((obj1 = shChainWalk(chain1,curs,NEXT)) != NULL) {
	 phObject1Del(obj1);
      }
      shChainDel(chain1);
      
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Grow an object based on an extracted radial profile
 */
static char *tclObjectGrowFromProfile_use =
  "USAGE: objectGrowFromProfile <obj> <level> <reg> <fparams> <thresh> "\
"<set_sectors> <clip> <rat> -npix N";
#define tclObjectGrowFromProfile_hlp \
  "Given an OBJECT <obj> and a level within that OBJECT, grow it out " \
"based upon its radial profile. It lives in REGION <reg> (FRAMEPARAMS <fparams>) "\
"and should be grown until it's profile is less than <thresh> above sky. "\
"If <set_sectors> is positive, if more than that many sectors in an annulus "\
"are above threshold, the whole annulus is taken to be part of the grown "\
"object. <clip> tells how hard to clip the median in seeing if the annulus "\
"is above threshold (if > 0). <rat> is the axis ratio above which features "\
"are taken to be linear; if -npix is specified, it's the minimum number of "\
"objects in a linear feature. Returns a handle to an new, grown, OBJECT"

static ftclArgvInfo objectGrowFromProfile_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjectGrowFromProfile_hlp},
   {"<obj>", FTCL_ARGV_STRING, NULL, NULL, "the original OBJECT"},
   {"<level>", FTCL_ARGV_INT, NULL, NULL, "the desired level in the OBJECT"},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "obj was found in this REGION"},
   {"<fparams>", FTCL_ARGV_STRING, NULL, NULL, "FRAMEPARAMS for sky level etc"},
   {"<thresh>", FTCL_ARGV_DOUBLE, NULL, NULL, "threshold above sky to grow to"},
   {"<set_sectors>", FTCL_ARGV_INT, NULL, NULL,
		   "no. of sectors above threshold to mask the entire annulus"},
   {"<clip>", FTCL_ARGV_INT, NULL, NULL, "how to clip sector statistics"},
   {"<rat>", FTCL_ARGV_DOUBLE, NULL, NULL,
				     "critical axis ratio for linear features"},
   {"-npix", FTCL_ARGV_INT, NULL, NULL,
	    "Minimum number of pixels for an object to be linear (default: 5)"},
   {"-radmax", FTCL_ARGV_INT, NULL, NULL,
			 "Minimum radius for extracted profile (default: 100)"},
   
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjectGrowFromProfile(
	      ClientData clientDatag,
	      Tcl_Interp *interp,
	      int ac,
	      char **av
	      )
{
   int i;
   FRAMEPARAMS *fparams;
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   OBJECT *obj, *newobj;
   REGION *reg;
   void *vptr;				/* used by shTclHandleExprEval */
   char *objStr = NULL;			/* the original OBJECT */
   int level = 0;			/* the desired level in the OBJECT */
   char *regStr = NULL;			/* obj was found in this REGION */
   char *fparamsStr = NULL;		/* FRAMEPARAMS for sky level etc */
   double thresh = 0.0;			/* threshold above sky to grow to */
   int set_sectors = 0;			/* no. of sectors above threshold to
					   mask the entire annulus */
   int clip = 0;			/* how to clip sector statistics */
   double rat = 0.0;			/* critical axis ratio for linear
					   features */
   int npix = 5;			/* Minimum number of pixels for an
					   object to be linear */
   int radmax = 100;			/* Min radius for extracted profile */

   shErrStackClear();

   i = 1;
   objectGrowFromProfile_opts[i++].dst = &objStr;
   objectGrowFromProfile_opts[i++].dst = &level;
   objectGrowFromProfile_opts[i++].dst = &regStr;
   objectGrowFromProfile_opts[i++].dst = &fparamsStr;
   objectGrowFromProfile_opts[i++].dst = &thresh;
   objectGrowFromProfile_opts[i++].dst = &set_sectors;
   objectGrowFromProfile_opts[i++].dst = &clip;
   objectGrowFromProfile_opts[i++].dst = &rat;
   objectGrowFromProfile_opts[i++].dst = &npix;
   objectGrowFromProfile_opts[i++].dst = &radmax;
   shAssert(objectGrowFromProfile_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objectGrowFromProfile_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * process arguments
 */
   if(shTclHandleExprEval(interp,objStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJECT")) {
      Tcl_SetResult(interp,"objectGrowFromProfile: "
                    "first argument is not a OBJECT",TCL_STATIC);
      return(TCL_ERROR);
   }
   obj = hand.ptr;

   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"objectGrowFromProfile: "
                    "third argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(shTclHandleExprEval(interp,fparamsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FRAMEPARAMS")) {
      Tcl_SetResult(interp,"objectGrowFromProfile: "
                    "fourth argument is not a FRAMEPARAMS",TCL_STATIC);
      return(TCL_ERROR);
   }
   fparams = hand.ptr;
/*
 * do the work
 */
   newobj = phObjectGrowFromProfile(obj,level,reg,fparams,thresh,
				    set_sectors,clip,rat,npix,radmax);
/*
 * and return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = newobj;
   hand.type = shTypeGetFromName("OBJECT");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new OBJECT handle",TCL_STATIC);
      phObjectDel(newobj);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Grow an object by N pixels
 */
static char *tclObjectGrow_use =
  "USAGE: ObjectGrow <object> <level> <npix>";
#define tclObjectGrow_hlp \
  "Grow an <lev> level in an OBJECT out by <n> pixels in all directions. "\
"Return the grown object"

static ftclArgvInfo objectGrow_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjectGrow_hlp},
   {"<obj>", FTCL_ARGV_STRING, NULL, NULL, "The object to grow"},
   {"<lev>", FTCL_ARGV_INT, NULL, NULL, "The level in <obj>"},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region obj lives in"},
   {"<npix>", FTCL_ARGV_INT, NULL, NULL, "How many pixels to grow it"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjectGrow(
	      ClientData clientDatag,
	      Tcl_Interp *interp,
	      int ac,
	      char **av
	      )
{
   int i;
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   OBJECT *obj;
   void *vptr;				/* used by shTclHandleExprEval */
   char *objStr = NULL;			/* The object to grow */
   int lev = 0;				/* The level in <obj> */
   char *regStr = NULL;			/* Region obj lives in */
   int npix = 0;			/* How many pixels to grow it */

   shErrStackClear();

   i = 1;
   objectGrow_opts[i++].dst = &objStr;
   objectGrow_opts[i++].dst = &lev;
   objectGrow_opts[i++].dst = &regStr;
   objectGrow_opts[i++].dst = &npix;
   shAssert(objectGrow_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objectGrow_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * process the arguments
 */
   if(shTclHandleExprEval(interp,objStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJECT")) {
      Tcl_SetResult(interp,"objectGrow: "
                    "first argument is not a OBJECT",TCL_STATIC);
      return(TCL_ERROR);
   }
   obj = hand.ptr;

   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"objectGrow: "
                    "third argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * do the work
 */
   obj = phObjectGrow(obj,lev,hand.ptr,npix);
/*
 * and return it to tcl
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = obj;
   hand.type = shTypeGetFromName("OBJECT");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new OBJECT handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Grow and merge a chain of OBJECTs
 */
static char *tclObjectChainGrow_use =
  "USAGE: ObjectChainGrow ";
#define tclObjectChainGrow_hlp \
  ""

static ftclArgvInfo objectChainGrow_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjectChainGrow_hlp},
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL, "The chain of OBJECTs to grow"},
   {"<lev>", FTCL_ARGV_INT, NULL, NULL, "The level in objects"},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region objects live in"},
   {"<npix>", FTCL_ARGV_INT, NULL, NULL, "How many pixels to grow them"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjectChainGrow(
		   ClientData clientDatag,
		   Tcl_Interp *interp,
		   int ac,
		   char **av
		   )
{
   int i;
   CHAIN *chain;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *chainStr = NULL;		/* The chain of OBJECTs to grow */
   int lev = 0;				/* The level in objects */
   char *regStr = NULL;			/* Region objects live in */
   int npix = 0;			/* How many pixels to grow them */

   shErrStackClear();

   i = 1;
   objectChainGrow_opts[i++].dst = &chainStr;
   objectChainGrow_opts[i++].dst = &lev;
   objectChainGrow_opts[i++].dst = &regStr;
   objectChainGrow_opts[i++].dst = &npix;
   shAssert(objectChainGrow_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objectChainGrow_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * process the arguments
 */
   if(shTclHandleExprEval(interp,chainStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_SetResult(interp,"objectGrow: "
                    "first argument is not a CHAIN",TCL_STATIC);
      return(TCL_ERROR);
   }
   chain = hand.ptr;
   if(chain->type != shTypeGetFromName("OBJECT")) {
      Tcl_SetResult(interp,"objectGrow: "
                    "first argument is not an OBJECT CHAIN",TCL_STATIC);
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"objectGrow: "
                    "third argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * do the work
 */
   phObjectChainGrow(chain,lev,hand.ptr,npix);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * find the limits of a (bright) linear feature, given a partially filled
 * out OBJECT1
 */
static char *tclLinearFeatureFind_use =
  "USAGE: LinearFeatureFind <reg> <obj1> <hwidth> <bin> <len> <thresh>";
#define tclLinearFeatureFind_hlp \
  "Find a linear feature associated with the specified <obj1>. "\
"Sectors of the object of length <len> are considered at a time, "\
"as we step along the object <len>/2 at a time. At each step, we extract "\
"a profile of half-width <hwidth> and bin size <bin> perpendicular to "\
"the object's major axis, and use this to determine the object's ridgeline. "\
"We keep going until we come to the edge of the region, or the value "\
"of the extracted profile drops below <thresh>"

static ftclArgvInfo linearFeatureFind_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclLinearFeatureFind_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "the region containing the object"},
   {"<obj>", FTCL_ARGV_STRING, NULL, NULL, "the OBJECT in question"},
   {"<majaxis>", FTCL_ARGV_DOUBLE, NULL, NULL, "p.a. of object's major axis"},
   {"<hwidth>", FTCL_ARGV_INT, NULL, NULL, "half width of swathe"},
   {"<bin>", FTCL_ARGV_INT, NULL, NULL, "bin width for swathe"},
   {"<len>", FTCL_ARGV_INT, NULL, NULL, "length of sectors"},
   {"<thresh>", FTCL_ARGV_DOUBLE, NULL, NULL, "threshold for object"},
   {"-follow", FTCL_ARGV_INT, NULL, NULL, "follow ridgeline?"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclLinearFeatureFind(
		     ClientData clientDatag,
		     Tcl_Interp *interp,
		     int ac,
		     char **av
		     )
{
   int i;
   HANDLE hand;
   REGION *reg;
   void *vptr;				/* used by shTclHandleExprEval */
   char *regStr = NULL;			/* the region containing the object */
   char *objStr = NULL;			/* the OBJECT in question */
   double majaxis = 0.0;		/* p.a. of object's major axis */
   int hwidth = 0;			/* half width of swathe */
   int bin = 0;				/* bin width for swathe */
   int len = 0;				/* length of sectors */
   double thresh = 0.0;			/* threshold for object */
   int follow = 1;			/* follow ridgeline? */

   shErrStackClear();

   i = 1;
   linearFeatureFind_opts[i++].dst = &regStr;
   linearFeatureFind_opts[i++].dst = &objStr;
   linearFeatureFind_opts[i++].dst = &majaxis;
   linearFeatureFind_opts[i++].dst = &hwidth;
   linearFeatureFind_opts[i++].dst = &bin;
   linearFeatureFind_opts[i++].dst = &len;
   linearFeatureFind_opts[i++].dst = &thresh;
   linearFeatureFind_opts[i++].dst = &follow;
   shAssert(linearFeatureFind_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,linearFeatureFind_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * process args
 */
   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"linearFeatureFind: argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(shTclHandleExprEval(interp,objStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJECT1")) {
      Tcl_SetResult(interp,"linearFeatureFind: argument is not a OBJECT1",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phLinearFeatureFind(reg, hand.ptr, majaxis, follow,hwidth, bin, len, thresh);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclObjectPeaksFind_use =
  "USAGE: ObjectPeaksFind <reg> -object <objmask> -delta <delta>";
#define tclObjectPeaksFind_hlp \
  "Return a PEAKS with all the peaks in an object"

static ftclArgvInfo objectPeaksFind_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjectPeaksFind_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region containing object"},
   {"-object", FTCL_ARGV_STRING, NULL, NULL,
			      "if provided, an OBJMASK specifying the object"},
   {"-delta", FTCL_ARGV_INT, NULL, NULL,
		      "Amount peak pixel must exceed neighbours (default: 1)"},
   {"-npeak_max", FTCL_ARGV_INT, NULL, NULL,
			  "Maximum number of peaks to find (-ve => no limit)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
   
};

static int
tclObjectPeaksFind(
		   ClientData clientDatag,
		   Tcl_Interp *interp,
		   int ac,
		   char **av
		   )
{
   int i;
   HANDLE hand;
   char name[HANDLE_NAMELEN];
   OBJMASK *obj;
   PEAKS *peaks;
   REGION *reg;
   void *vptr;				/* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region containing object */
   char *objectStr = NULL;		/* an OBJMASK specifying the object */
   int delta = 1;			/* Amount peak pixel must exceed
					   neighbours */
   int npeak_max = -1;			/* Maximum number of peaks to find
					   (-1 => no limit) */

   shErrStackClear();

   i = 1;
   objectPeaksFind_opts[i++].dst = &regStr;
   objectPeaksFind_opts[i++].dst = &objectStr;
   objectPeaksFind_opts[i++].dst = &delta;
   objectPeaksFind_opts[i++].dst = &npeak_max;
   shAssert(objectPeaksFind_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objectPeaksFind_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"objectPeaksFind: "
                    "first argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(objectStr != NULL) {
      if(shTclHandleExprEval(interp,objectStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("OBJMASK")) {
	 Tcl_SetResult(interp,"objectPeaksFind: "
		       "-obj argument is not a OBJMASK",TCL_STATIC);
	 return(TCL_ERROR);
      }
      obj = hand.ptr;
   } else {
      obj =
	phObjmaskFromRect(reg->col0, reg->row0,
			  reg->col0 + reg->ncol - 1, reg->row0 + reg->nrow -1);
   }
/*
 * work
 */
   peaks = phPeaksNew(1);
   phObjectPeaksFind(reg, obj, delta, peaks, npeak_max);
/*
 * return answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = peaks;
   hand.type = shTypeGetFromName("PEAKS");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new PEAKS handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
/*
 * clean up
 */
   if(objectStr == NULL) {
      phObjmaskDel(obj);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclObjectsFindInObjmask_use =
  "USAGE: objectsFindInObjmask <reg> <objmask> <nlevel> <levels>";
#define tclObjectsFindInObjmask_hlp \
  "Like regFinder, but only search within an <objmask>"

static ftclArgvInfo objectsFindInObjmask_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjectsFindInObjmask_hlp},
   {"<region>", FTCL_ARGV_STRING, NULL, NULL, "Region to search"},
   {"<objmask>", FTCL_ARGV_STRING, NULL, NULL,"Objmask within which to search"},
   {"<levels>", FTCL_ARGV_STRING, NULL, NULL, "vector of isophotal levels"},
   {"-npixel_min_level", FTCL_ARGV_INT, NULL, NULL,
    "The level in the object that the npixel_min criterion applies to"},
   {"-npixel_min", FTCL_ARGV_INT, NULL, NULL,
				      "Minimum number of pixels in an object"},
   {"-npeak_max", FTCL_ARGV_INT, NULL, NULL,
			   "Maximum number of peaks to find (-ve => no limit)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjectsFindInObjmask(
			ClientData clientDatag,
			Tcl_Interp *interp,
			int ac,
			char **av
			)
{
   int a_i;
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   VECTOR *levels;			/* input vector */
   CHAIN *objs;				/* returned OBJECT list */
   OBJMASK *om;
   unsigned short *plevs;		/* unpacked from levels */
   REGION *reg;
   void *vptr;				/* used by shTclHandleExprEval */
   char *regionStr = NULL;		/* Region to search */
   char *objmaskStr = NULL;		/* Objmask within which to search */
   char *levelsStr = NULL;		/* vector of isophotal levels */
   int npeak_max = -1;			/* Maximum number of peaks to find
					   (-ve => no limit) */
   int npixel_min_level = 0;		/* The level in the object that the npixel_min criterion applies to */
   int npixel_min = 0;			/* Min number of pixels in an object
					   (<= 0 => no limit) */

   shErrStackClear();

   a_i = 1;
   objectsFindInObjmask_opts[a_i++].dst = &regionStr;
   objectsFindInObjmask_opts[a_i++].dst = &objmaskStr;
   objectsFindInObjmask_opts[a_i++].dst = &levelsStr;
   objectsFindInObjmask_opts[a_i++].dst = &npixel_min_level;
   objectsFindInObjmask_opts[a_i++].dst = &npixel_min;
   objectsFindInObjmask_opts[a_i++].dst = &npeak_max;
   shAssert(objectsFindInObjmask_opts[a_i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objectsFindInObjmask_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regionStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"objectsFindInObjmask: "
                    "first argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;
   
   if(shTclHandleExprEval(interp,objmaskStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJMASK")) {
      Tcl_SetResult(interp,"objectsFindInObjmask: "
                    "second argument is not a OBJMASK",TCL_STATIC);
      return(TCL_ERROR);
   }
   om = hand.ptr;

   if(shTclHandleExprEval(interp,levelsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("VECTOR")) {
      Tcl_SetResult(interp,"objectsFindInObjmask: "
                    "third argument is not a VECTOR",TCL_STATIC);
      return(TCL_ERROR);
   }
   levels = hand.ptr;
/*
 * Unpack array of of isophotal levels.
 */
   plevs = shMalloc(levels->dimen*sizeof(*plevs));
   for(i = 0; i < levels->dimen; i++) {
      plevs[i] = (unsigned short)levels->vec[i];
   }
/*
 * Hurrah! Do the work
 */
   objs = phObjectsFindInObjmask(reg, om, levels->dimen, plevs,
				 npixel_min_level, npixel_min, npeak_max);
   
   shFree(plevs);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = objs;
   hand.type = shTypeGetFromName("CHAIN");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"objectsFindInObjmask: "
		    "can't bind to new CHAIN handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclPeaksCull_use =
  "USAGE: peaksCull reg peaks objmask threshold delta";
#define tclPeaksCull_hlp \
  "Cull a list of peaks in a PEAKS"

static ftclArgvInfo peaksCull_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPeaksCull_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "the region containing the peaks"},
   {"<peaks>", FTCL_ARGV_STRING, NULL, NULL,
					"a PEAKS containing the peaks to cull"},
   {"<objmask>", FTCL_ARGV_STRING, NULL, NULL,
				    "an OBJMASK defining the object (or NULL)"},
   {"-threshold", FTCL_ARGV_INT, NULL, NULL,
			   "threshold for acceptable peak values (default: 0)"},
   {"<delta>", FTCL_ARGV_INT, NULL, NULL,
					 "amount peak must exceed saddlepoint"},
   {"-gain", FTCL_ARGV_DOUBLE, NULL, NULL, "Gain of chip"},
   {"-sky_var", FTCL_ARGV_DOUBLE, NULL, NULL,
				      "sky variance (including dark_variance)"},
   {"-neff", FTCL_ARGV_DOUBLE, NULL, NULL,
			      "effective area of any smoothing filter applied"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclPeaksCull(
	     ClientData clientDatag,
	     Tcl_Interp *interp,
	     int ac,
	     char **av
	     )
{
   int i;
   HANDLE hand;
   OBJMASK *om;
   PEAKS *peaks;
   REGION *reg;
   void *vptr;				/* used by shTclHandleExprEval */
   char *regStr = NULL;			/* the region containing the peaks */
   char *peaksStr = NULL;		/* the peaks to cull */
   char *objmaskStr = NULL;		/* an OBJMASK defining the object  */
   int threshold = 0;			/* threshold for acceptable peak vals */
   int delta = 0;			/* amount peak must exceed saddlepoint*/
   double gain = 1.0;			/* Gain of chip */
   double sky_var = 0.0;		/* sky variance (including
					   dark_variance) */
   double neff = 1.0;			/* effective area of any smoothing
					   filter applied */

   shErrStackClear();

   i = 1;
   peaksCull_opts[i++].dst = &regStr;
   peaksCull_opts[i++].dst = &peaksStr;
   peaksCull_opts[i++].dst = &objmaskStr;
   peaksCull_opts[i++].dst = &threshold;
   peaksCull_opts[i++].dst = &delta;
   peaksCull_opts[i++].dst = &gain;
   peaksCull_opts[i++].dst = &sky_var;
   peaksCull_opts[i++].dst = &neff;
   shAssert(peaksCull_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,peaksCull_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"peaksCull: "
                    "first argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(shTclHandleExprEval(interp,peaksStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PEAKS")) {
      Tcl_SetResult(interp,"peaksCull: "
                    "second argument is not a PEAKS",TCL_STATIC);
      return(TCL_ERROR);
   }
   peaks = hand.ptr;

   if(strcmp(objmaskStr,"NULL") == 0 || strcmp(objmaskStr,"null") == 0) {
      om = NULL;
   } else {
      if(shTclHandleExprEval(interp,objmaskStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("OBJMASK")) {
	 Tcl_SetResult(interp,"peaksCull: "
		       "third argument is not a OBJMASK",TCL_STATIC);
	 return(TCL_ERROR);
      }
      om = hand.ptr;
   }
/*
 * work
 */
   if(delta < 0) {			/* i.e. |delta| is nsigma */
      if(gain == 0 || neff == 0) {	/* need real values of gain, neff */
	 shErrStackPush("peaksCull: gain/neff (%g/%g) must be non-zero "
		       "if delta < 0", gain, neff);
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
   }
   phPeaksCull(reg, peaks, om, threshold, delta, gain, sky_var, neff);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclObjectChainPeaksCull_use =
  "USAGE: objectChainPeaksCull <reg> <objects> <delta> -threshold n -fparams fp -neff n";
#define tclObjectChainPeaksCull_hlp \
  "Go through a chain of OBJECTs culling insignificant peaks"

static ftclArgvInfo objectChainPeaksCull_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjectChainPeaksCull_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region containing objects"},
   {"<objects>", FTCL_ARGV_STRING, NULL, NULL, "CHAIN of OBJECTs"},
   {"<delta>", FTCL_ARGV_DOUBLE, NULL, NULL,
					"Amount peaks must exceed saddlepoint"},
   {"-threshold", FTCL_ARGV_INT, NULL, NULL,
			 "Threshold peaks must exceed to survive (default: 0)"},
   {"-fparams", FTCL_ARGV_STRING, NULL, NULL,
		    "FRAMEPARAMS giving gain etc. (required iff delta < 0)"},   
   {"-neff", FTCL_ARGV_DOUBLE, NULL, NULL,
		   "N_{effective} of smoothing applied to region (default: 1)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjectChainPeaksCull(
			ClientData clientDatag,
			Tcl_Interp *interp,
			int ac,
			char **av
			)
{
   int i;
   FRAMEPARAMS *fparams;
   HANDLE hand;
   CHAIN *objs;
   REGION *reg;
   void *vptr;				/* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region containing objects */
   char *objectsStr = NULL;		/* CHAIN of OBJECTs */
   double delta = 0.0;			/* Amount peaks must exceed
					   saddlepoint */
   int threshold = 0;			/* Threshold peaks must exceed to
					   survive */
   char *fparamsStr = NULL;		/* FRAMEPARAMS giving gain etc. */
   double neff = 1.0;			/* N_{effective} of smoothing applied
					   to region */

   shErrStackClear();

   i = 1;
   objectChainPeaksCull_opts[i++].dst = &regStr;
   objectChainPeaksCull_opts[i++].dst = &objectsStr;
   objectChainPeaksCull_opts[i++].dst = &delta;
   objectChainPeaksCull_opts[i++].dst = &threshold;
   objectChainPeaksCull_opts[i++].dst = &fparamsStr;
   objectChainPeaksCull_opts[i++].dst = &neff;
   shAssert(objectChainPeaksCull_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objectChainPeaksCull_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"objectChainPeaksCull: "
                    "first argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(shTclHandleExprEval(interp,objectsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_SetResult(interp,"objectChainPeaksCull: "
                    "second argument is not a CHAIN",TCL_STATIC);
      return(TCL_ERROR);
   }
   objs = hand.ptr;

   if(fparamsStr == NULL) {
      fparams = NULL;
   } else {
      if(shTclHandleExprEval(interp,fparamsStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("FRAMEPARAMS")) {
	 Tcl_SetResult(interp,"objectChainPeaksCull: "
		       "argument is not a FRAMEPARAMS",TCL_STATIC);
	 return(TCL_ERROR);
      }
      fparams = hand.ptr;
   }
/*
 * work
 */
   phObjectChainPeaksCull(reg, objs, threshold, delta, fparams, neff);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclPeaksNew_use =
  "USAGE: PeaksNew n";
#define tclPeaksNew_hlp \
  "Create a new PEAKS with room for <n> PEAKs"

static ftclArgvInfo peaksNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPeaksNew_hlp},
   {"<n>", FTCL_ARGV_INT, NULL, NULL, "Number of PEAKs to allocate"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclPeaksNew(
	    ClientData clientDatag,
	    Tcl_Interp *interp,
	    int ac,
	    char **av
	    )
{
   int i;
   HANDLE hand;
   char name[HANDLE_NAMELEN];
   PEAKS *peaks;
   int n = 0;				/* Number of PEAKs to allocate */

   shErrStackClear();

   i = 1;
   peaksNew_opts[i++].dst = &n;
   shAssert(peaksNew_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,peaksNew_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * work
 */
   peaks = phPeaksNew(n);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = peaks;
   hand.type = shTypeGetFromName("PEAKS");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new PEAKS handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);


   return(TCL_OK);
}

/*****************************************************************************/
static char *tclPeaksDel_use =
  "USAGE: PeaksDel <peaks>";
#define tclPeaksDel_hlp \
  "Delete a PEAKS"

static ftclArgvInfo peaksDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPeaksDel_hlp},
   {"<peaks>", FTCL_ARGV_STRING, NULL, NULL, "The PEAKS to delete"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclPeaksDel(
	    ClientData clientDatag,
	    Tcl_Interp *interp,
	    int ac,
	    char **av
	    )
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *peaksStr = NULL;		/* The PEAKS to delete */

   shErrStackClear();

   i = 1;
   peaksDel_opts[i++].dst = &peaksStr;
   shAssert(peaksDel_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,peaksDel_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,peaksStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PEAKS")) {
      Tcl_SetResult(interp,"peaksDel: "
                    "argument is not a PEAKS",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phPeaksDel(hand.ptr);
   p_shTclHandleDel(interp,peaksStr);
   
   return(TCL_OK);
}

/*****************************************************************************/
static char *tclPeakCenterFit_use =
  "USAGE: PeakCenterFit <reg> <peak>";
#define tclPeakCenterFit_hlp \
  "Given a PEAK and its parent REGION (and e.g. the gain) improve the position"

static ftclArgvInfo peakCenterFit_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPeakCenterFit_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region containing peak"},
   {"<peak>", FTCL_ARGV_STRING, NULL, NULL, "The peak in question"},
   {"-objmask", FTCL_ARGV_STRING, NULL, NULL, "OBJMASK of pixels in object"},
   {"-bkgd", FTCL_ARGV_INT, NULL, NULL,
				    "Background level NOT removed from frame"},
   {"-maxbin", FTCL_ARGV_INT, NULL, NULL,
			     "Maximum allowed rebinning while finding centre"},
   {"-dark_variance", FTCL_ARGV_DOUBLE, NULL, NULL, "Dark variance"},
   {"-sky", FTCL_ARGV_DOUBLE, NULL, NULL, "Sky level for frame"},
   {"-gain", FTCL_ARGV_DOUBLE, NULL, NULL, "Gain of amplifiers"},
   {"-no_smooth", FTCL_ARGV_CONSTANT, (void *)1, NULL,
				      "Don't smooth region while centroiding"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclPeakCenterFit(ClientData clientDatag,
		 Tcl_Interp *interp,
		 int ac,
		 char **av)
{
   int i;
   FRAMEPARAMS *fparams;
   HANDLE hand;
   PEAK *peak;
   REGION *reg;
   void *vptr;				/* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region containing peak */
   char *peakStr = NULL;		/* The peak in question */
   char *objmaskStr = NULL;		/* pixels in object */
   OBJMASK *objmask;
   int bkgd = 0;			/* Background level NOT removed
					   from frame */
   int maxbin = 1;			/* Maximum allowed rebinning while
					   finding centre */
   double dark_variance = 0.0;		/* Dark variance */
   double sky = 0.0;			/* Sky level for frame */
   double gain = 1.0;			/* Gain of amplifiers */
   int no_smooth = 0;			/* Don't smooth region */
   CENTROID_FLAGS cflags = (CENTROID_FLAGS)0; /* control centroiding */

   shErrStackClear();

   i = 1;
   peakCenterFit_opts[i++].dst = &regStr;
   peakCenterFit_opts[i++].dst = &peakStr;
   peakCenterFit_opts[i++].dst = &objmaskStr;
   peakCenterFit_opts[i++].dst = &bkgd;
   peakCenterFit_opts[i++].dst = &maxbin;
   peakCenterFit_opts[i++].dst = &dark_variance;
   peakCenterFit_opts[i++].dst = &sky;
   peakCenterFit_opts[i++].dst = &gain;
   peakCenterFit_opts[i++].dst = &no_smooth;
   shAssert(peakCenterFit_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,peakCenterFit_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"peakCenterFit: "
                    "first argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(shTclHandleExprEval(interp,peakStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PEAK")) {
      Tcl_SetResult(interp,"peakCenterFit: "
                    "second argument is not a PEAK",TCL_STATIC);
      return(TCL_ERROR);
   }
   peak = hand.ptr;

   if(objmaskStr == NULL) {
      objmask = phObjmaskFromRect(reg->col0, reg->row0,
			 reg->col0 + reg->ncol - 1, reg->row0 + reg->nrow - 1);
   } else {
      if(shTclHandleExprEval(interp,objmaskStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("OBJMASK")) {
	 Tcl_AppendResult(interp,"peakCenterFit: "
			  "argument \"", objmaskStr, "\" is not an OBJMASK",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
      objmask = hand.ptr;
   }

   if(no_smooth) {
      cflags |= NO_SMOOTH;
   }
/*
 * make a FRAMEPARAMS
 */
   fparams = phFrameparamsNew('0');
   fparams->bkgd = bkgd;
   fparams->dark_variance = phBinregionNewFromConst(dark_variance, reg->nrow, reg->nrow, reg->ncol, reg->ncol, MAX_U16);
   fparams->sky = phBinregionNewFromConst(sky, 1, 1, 1, 1, 0);
   fparams->gain = phBinregionNewFromConst(gain, reg->nrow, reg->nrow, reg->ncol, reg->ncol, MAX_U16);
/*
 * work
 */
   phPeakCenterFit(peak, reg, objmask, fparams, maxbin, cflags);
/*
 * clean up
 */
   if(objmaskStr == NULL) {
      phObjmaskDel(objmask);
   }
   phFrameparamsDel(fparams);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclObjectChainFlags2Set_use =
  "USAGE: objectChainFlags2Set <objects> <level> <flags2>";
#define tclObjectChainFlags2Set_hlp \
  "OR a value into the flags2 field of each object in a chain, if a given "\
"level is present"

static ftclArgvInfo objectChainFlags2Set_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjectChainFlags2Set_hlp},
   {"<objects>", FTCL_ARGV_STRING, NULL, NULL, "CHAIN of OBJECTs"},
   {"<level>", FTCL_ARGV_INT, NULL, NULL,
				      "Level that must be present in OBJECTs"},
   {"<flags2>", FTCL_ARGV_INT, NULL, NULL, "Values to | into obj->flags2"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define objectChainFlags2Set_name "objectChainFlags2Set"

static int
tclObjectChainFlags2Set(ClientData clientData,
			Tcl_Interp *interp,
			int ac,
			char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *objectsStr = NULL;		/* CHAIN of OBJECTs */
   CHAIN *objects;
   int level = 0;			/* Level that must be present */
   int flags2 = 0;			/* Values to | into obj->flags2 */

   shErrStackClear();

   i = 1;
   objectChainFlags2Set_opts[i++].dst = &objectsStr;
   objectChainFlags2Set_opts[i++].dst = &level;
   objectChainFlags2Set_opts[i++].dst = &flags2;
   shAssert(objectChainFlags2Set_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, objectChainFlags2Set_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     objectChainFlags2Set_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,objectsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_AppendResult(interp,"objectChainFlags2Set: "
                       "argument \"", objectsStr, "\" is not a CHAIN",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   objects = hand.ptr;

   if(objects->type != shTypeGetFromName("OBJECT")) {
      Tcl_AppendResult(interp,
		       "CHAIN \"", objectsStr, "\" doesn't have type OBJECT",
								 (char *)NULL);
      return(TCL_ERROR);
   }

   if(level < 0) {
      Tcl_SetResult(interp, "Please specify a level >= 0", TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phObjectChainFlags2Set(objects, level, flags2);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclObjectChainFlags3Set_use =
  "USAGE: objectChainFlags3Set <objects> <level> <flags3>";
#define tclObjectChainFlags3Set_hlp \
  "OR a value into the flags3 field of each object in a chain, if a given "\
"level is present"

static ftclArgvInfo objectChainFlags3Set_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjectChainFlags3Set_hlp},
   {"<objects>", FTCL_ARGV_STRING, NULL, NULL, "CHAIN of OBJECTs"},
   {"<level>", FTCL_ARGV_INT, NULL, NULL,
				      "Level that must be present in OBJECTs"},
   {"<flags3>", FTCL_ARGV_INT, NULL, NULL, "Values to | into obj->flags3"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define objectChainFlags3Set_name "objectChainFlags3Set"

static int
tclObjectChainFlags3Set(ClientData clientData,
			Tcl_Interp *interp,
			int ac,
			char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *objectsStr = NULL;		/* CHAIN of OBJECTs */
   CHAIN *objects;
   int level = 0;			/* Level that must be present */
   int flags3 = 0;			/* Values to | into obj->flags3 */

   shErrStackClear();

   i = 1;
   objectChainFlags3Set_opts[i++].dst = &objectsStr;
   objectChainFlags3Set_opts[i++].dst = &level;
   objectChainFlags3Set_opts[i++].dst = &flags3;
   shAssert(objectChainFlags3Set_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, objectChainFlags3Set_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     objectChainFlags3Set_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,objectsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_AppendResult(interp,"objectChainFlags3Set: "
                       "argument \"", objectsStr, "\" is not a CHAIN",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   objects = hand.ptr;

   if(objects->type != shTypeGetFromName("OBJECT")) {
      Tcl_AppendResult(interp,
		       "CHAIN \"", objectsStr, "\" doesn't have type OBJECT",
								 (char *)NULL);
      return(TCL_ERROR);
   }

   if(level < 0) {
      Tcl_SetResult(interp, "Please specify a level >= 0", TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phObjectChainFlags3Set(objects, level, flags3);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclObject1ChainFlagsSet_use =
  "USAGE: object1ChainFlagsSet <obj1chain> <obj1_flags>";
#define tclObject1ChainFlagsSet_hlp \
  "OR a set of flags into each element of an OBJECT1 chain"

static ftclArgvInfo object1ChainFlagsSet_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObject1ChainFlagsSet_hlp},
   {"<obj1chain>", FTCL_ARGV_STRING, NULL, NULL, "Chain of OBJECT1s"},
   {"<obj1_flags>", FTCL_ARGV_INT, NULL, NULL, "Flags to | into obj1->flags"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define object1ChainFlagsSet_name "object1ChainFlagsSet"

static int
tclObject1ChainFlagsSet(ClientData clientData,
			Tcl_Interp *interp,
			int ac,
			char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *obj1chainStr = NULL;		/* Chain of OBJECT1s */
   CHAIN *obj1chain;
   int obj1_flags = 0;			/* Flags to | into obj1->flags */

   shErrStackClear();

   i = 1;
   object1ChainFlagsSet_opts[i++].dst = &obj1chainStr;
   object1ChainFlagsSet_opts[i++].dst = &obj1_flags;
   shAssert(object1ChainFlagsSet_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, object1ChainFlagsSet_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     object1ChainFlagsSet_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,obj1chainStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_SetResult(interp,"object1ChainFlagsSet: "
                    "argument \"obj1chain\" is not a CHAIN",TCL_STATIC);
      return(TCL_ERROR);
   }
   obj1chain = hand.ptr;

   if(obj1chain->type != shTypeGetFromName("OBJECT1")) {
      Tcl_SetResult(interp,"object1ChainSetAsBright: "
		    "chain is not of type OBJECT1", TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phObject1ChainFlagsSet(obj1chain, obj1_flags);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclObjectChainUnbin_use =
  "USAGE: objectChainUnbin <objects> <rbin> <cbin> <row0> <col0> -nrow n -ncol m";
#define tclObjectChainUnbin_hlp \
  "Given a chain of objects found in a binned region, convert them to live in"\
"the unbinned region"

static ftclArgvInfo objectChainUnbin_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjectChainUnbin_hlp},
   {"<objects>", FTCL_ARGV_STRING, NULL, NULL, "Chain of objects"},
   {"<rbin>", FTCL_ARGV_INT, NULL, NULL, "How much rows are binned by"},
   {"<cbin>", FTCL_ARGV_INT, NULL, NULL, "How much cols are binned by"},
   {"<row0>", FTCL_ARGV_INT, NULL, NULL, "Row origin of binned region"},
   {"<col0>", FTCL_ARGV_INT, NULL, NULL, "Column origin of binned region"},
   {"-nrow", FTCL_ARGV_INT, NULL, NULL, "Number of rows in unbinned region"},
   {"-ncol", FTCL_ARGV_INT, NULL, NULL,
				    "Number of columns in unbinned region"},   
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjectChainUnbin(
	       ClientData clientDatag,
	       Tcl_Interp *interp,
	       int ac,
	       char **av
	       )
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *objectsStr;
   CHAIN *objects;
   int rbin;
   int cbin;
   int row0;
   int col0;
   int nrow = 0;
   int ncol = 0;
   shErrStackClear();

   i = 1;
   objectChainUnbin_opts[i++].dst = &objectsStr;
   objectChainUnbin_opts[i++].dst = &rbin;
   objectChainUnbin_opts[i++].dst = &cbin;
   objectChainUnbin_opts[i++].dst = &row0;
   objectChainUnbin_opts[i++].dst = &col0;
   objectChainUnbin_opts[i++].dst = &nrow;
   objectChainUnbin_opts[i++].dst = &ncol;
   shAssert(objectChainUnbin_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objectChainUnbin_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,objectsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_SetResult(interp,"objectChainUnbin: "
                    "first argument is not a CHAIN",TCL_STATIC);
      return(TCL_ERROR);
   }
   objects = hand.ptr;
   if(objects->type != shTypeGetFromName("OBJECT")) {
      Tcl_SetResult(interp,"objectChainUnbin: "
		   "CHAIN is not of type \"OBJECT\"",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phObjectChainUnbin(objects, rbin, cbin, row0, col0, nrow, ncol);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclInitObjectFinder_use =
  "USAGE: initObjectFinder";
#define tclInitObjectFinder_hlp \
  "Initialise the object/peak finder"

static ftclArgvInfo initObjectFinder_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclInitObjectFinder_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define initObjectFinder_name "initObjectFinder"

static int
tclInitObjectFinder(ClientData clientData,
	      Tcl_Interp *interp,
	      int ac,
	      char **av)
{
   int a_i;

   shErrStackClear();

   a_i = 1;
   shAssert(initObjectFinder_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, initObjectFinder_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     initObjectFinder_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phInitObjectFinder();

   return(TCL_OK);
}

static char *tclFiniObjectFinder_use =
  "USAGE: finiObjectFinder";
#define tclFiniObjectFinder_hlp \
  "Cleanup after the object/peak finder"

static ftclArgvInfo finiObjectFinder_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFiniObjectFinder_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define finiObjectFinder_name "finiObjectFinder"

static int
tclFiniObjectFinder(ClientData clientData,
	      Tcl_Interp *interp,
	      int ac,
	      char **av)
{
   int a_i;

   shErrStackClear();

   a_i = 1;
   shAssert(finiObjectFinder_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, finiObjectFinder_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     finiObjectFinder_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phFiniObjectFinder();

   return(TCL_OK);
}

/************************************************************************************************************/
static char *tclRegTrimToThreshold_use =
  "USAGE: regTrimToThreshold <in> <threshold> -npixel_min ival -ngrow ival -maskType ival";
#define tclRegTrimToThreshold_hlp \
  "Trim a region to a given threshold, returning a new region (not subregion)"

static ftclArgvInfo regTrimToThreshold_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegTrimToThreshold_hlp},
   {"<in>", FTCL_ARGV_STRING, NULL, NULL, "The input region"},
   {"<threshold>", FTCL_ARGV_DOUBLE, NULL, NULL, "The desired threshold"},
   {"-npixel_min", FTCL_ARGV_INT, NULL, NULL, "Minimum number of pixels in object"},
   {"-ngrow", FTCL_ARGV_INT, NULL, NULL, "Number of pixels to grow detections"},
   {"-maskType", FTCL_ARGV_INT, NULL, NULL, "Name of bitplane to set in output region's mask"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define regTrimToThreshold_name "regTrimToThreshold"

static int
tclRegTrimToThreshold(ClientData clientData,
		      Tcl_Interp *interp,
		      int ac,
		      char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *inStr = NULL;			/* The input region */
   REGION *in;
   double threshold = 0.0;		/* The desired threshold */
   int npixel_min = 0;			/* Minimum number of pixels in object */
   int ngrow = 0;			/* Number of pixels to grow detections */
   int maskType = -1;			/* Name of bitplane to set in output region's mask */
   REGION *out;				/* the trimmed region */

   shErrStackClear();

   a_i = 1;
   regTrimToThreshold_opts[a_i++].dst = &inStr;
   regTrimToThreshold_opts[a_i++].dst = &threshold;
   regTrimToThreshold_opts[a_i++].dst = &npixel_min;
   regTrimToThreshold_opts[a_i++].dst = &ngrow;
   regTrimToThreshold_opts[a_i++].dst = &maskType;
   shAssert(regTrimToThreshold_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, regTrimToThreshold_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     regTrimToThreshold_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,inStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_AppendResult(interp,"regTrimToThreshold: "
                       "argument \"", inStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   in = hand.ptr;
/*
 * work
 */
   out = phRegionTrimToThreshold(in, threshold, npixel_min, ngrow, maskType);
/*
 * Return the answer
 */
   if(out == NULL) {
      Tcl_SetResult(interp, "regTrimToThreshold: "
		    "Insufficient pixels were above threshold", TCL_STATIC);
      return(TCL_ERROR);
   }

   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = out;
   hand.type = shTypeGetFromName("REGION");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"out\" to new REGION handle",
                                                                   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   
   return(TCL_OK);
}

/************************************************************************************************************/

static char *tclObjectChainTrim_use =
  "USAGE: objectChainTrim <objChain> <level> <npixMin>";
#define tclObjectChainTrim_hlp \
  ""

static ftclArgvInfo objectChainTrim_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjectChainTrim_hlp},
   {"<objChain>", FTCL_ARGV_STRING, NULL, NULL, "CHAIN of OBJECTS"},
   {"<level>", FTCL_ARGV_INT, NULL, NULL, "Desired level in OBJECTs"},
   {"<npixMin>", FTCL_ARGV_INT, NULL, NULL, "Minimum number of pixels permitted in an OBJECT"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define objectChainTrim_name "objectChainTrim"

static int
tclObjectChainTrim(ClientData clientData,
	     Tcl_Interp *interp,
	     int ac,
	     char **av)
{
    HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *objChainStr = NULL;		/* CHAIN of OBJECTS */
   CHAIN *objChain;
   int level = 0;			/* Desired level in OBJECTs */
   int npixMin = 0;			/* Minimum number of pixels permitted in an OBJECT */

   shErrStackClear();

   a_i = 1;
   objectChainTrim_opts[a_i++].dst = &objChainStr;
   objectChainTrim_opts[a_i++].dst = &level;
   objectChainTrim_opts[a_i++].dst = &npixMin;
   shAssert(objectChainTrim_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, objectChainTrim_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     objectChainTrim_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,objChainStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_AppendResult(interp,"objectChainTrim: "
                       "argument \"", objChainStr, "\" is not a CHAIN",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   objChain = hand.ptr;

   if(objChain->type != shTypeGetFromName("OBJECT")) {
      Tcl_AppendResult(interp,
                       "CHAIN \"", objChainStr, "\" doesn't have type OBJECT",
								 (char *)NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phObjectChainTrim(objChain, level, npixMin);

   return(TCL_OK);
}

/*****************************************************************************/

void phTclRegFinderDeclare(Tcl_Interp *interp)
{
    shTclDeclare(interp, "regFinder",
		 (Tcl_CmdProc *)tclRegFinder,
		 (ClientData) 0,
		 (Tcl_CmdDeleteProc *) NULL,
		 module,
		 tclRegFinder_hlp, tclRegFinder_use);

   shTclDeclare(interp,"objectToObject1ChainConvert",
		(Tcl_CmdProc *)tclObjectToObject1ChainConvert, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjectToObject1ChainConvert_hlp,
		tclObjectToObject1ChainConvert_use);

   shTclDeclare(interp,"spanmaskSetFromObjectChain",
		(Tcl_CmdProc *)tclSpanmaskSetFromObjectChain, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclSpanmaskSetFromObjectChain_hlp,
		tclSpanmaskSetFromObjectChain_use);

   shTclDeclare(interp,"spanmaskSetFromObject1Chain",
		(Tcl_CmdProc *)tclSpanmaskSetFromObject1Chain, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclSpanmaskSetFromObject1Chain_hlp,
		tclSpanmaskSetFromObject1Chain_use);

   shTclDeclare(interp,"objectChainDel",
		(Tcl_CmdProc *)tclObjectChainDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjectChainDel_hlp,
		tclObjectChainDel_use);

   shTclDeclare(interp,"linearFeatureFind",
		(Tcl_CmdProc *)tclLinearFeatureFind, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclLinearFeatureFind_hlp,
		tclLinearFeatureFind_use);

   shTclDeclare(interp,"objectGrowFromProfile",
		(Tcl_CmdProc *)tclObjectGrowFromProfile, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjectGrowFromProfile_hlp,
		tclObjectGrowFromProfile_use);

   shTclDeclare(interp,"objectDel",
		(Tcl_CmdProc *)tclObjectDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjectDel_hlp,
		tclObjectDel_use);

   shTclDeclare(interp,"objectGrow",
		(Tcl_CmdProc *)tclObjectGrow, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjectGrow_hlp,
		tclObjectGrow_use);

   shTclDeclare(interp,"objectChainGrow",
		(Tcl_CmdProc *)tclObjectChainGrow, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjectChainGrow_hlp,
		tclObjectChainGrow_use);

   shTclDeclare(interp,"objectPeaksFind",
		(Tcl_CmdProc *)tclObjectPeaksFind, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjectPeaksFind_hlp,
		tclObjectPeaksFind_use);

   shTclDeclare(interp,"objectsFindInObjmask",
		(Tcl_CmdProc *)tclObjectsFindInObjmask, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjectsFindInObjmask_hlp,
		tclObjectsFindInObjmask_use);

    shTclDeclare(interp,"peaksCull",
		(Tcl_CmdProc *)tclPeaksCull, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPeaksCull_hlp,
		tclPeaksCull_use);

   shTclDeclare(interp,"objectChainPeaksCull",
		(Tcl_CmdProc *)tclObjectChainPeaksCull, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjectChainPeaksCull_hlp,
		tclObjectChainPeaksCull_use);

   shTclDeclare(interp,"peaksNew",
		(Tcl_CmdProc *)tclPeaksNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPeaksNew_hlp,
		tclPeaksNew_use);

   shTclDeclare(interp,"peaksDel",
		(Tcl_CmdProc *)tclPeaksDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPeaksDel_hlp,
		tclPeaksDel_use);

   shTclDeclare(interp,"peakCenterFit",
		(Tcl_CmdProc *)tclPeakCenterFit, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPeakCenterFit_hlp,
		tclPeakCenterFit_use);

   shTclDeclare(interp,"objectChainUnbin",
		(Tcl_CmdProc *)tclObjectChainUnbin, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjectChainUnbin_hlp,
		tclObjectChainUnbin_use);

   shTclDeclare(interp,object1ChainFlagsSet_name,
		(Tcl_CmdProc *)tclObject1ChainFlagsSet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObject1ChainFlagsSet_hlp,
		tclObject1ChainFlagsSet_use);

   shTclDeclare(interp,objectChainFlags2Set_name,
		(Tcl_CmdProc *)tclObjectChainFlags2Set, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjectChainFlags2Set_hlp,
		tclObjectChainFlags2Set_use);

   shTclDeclare(interp,objectChainFlags3Set_name,
		(Tcl_CmdProc *)tclObjectChainFlags3Set, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjectChainFlags3Set_hlp,
		tclObjectChainFlags3Set_use);

   shTclDeclare(interp,initObjectFinder_name,
		(Tcl_CmdProc *)tclInitObjectFinder, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclInitObjectFinder_hlp,
		tclInitObjectFinder_use);

   shTclDeclare(interp,finiObjectFinder_name,
		(Tcl_CmdProc *)tclFiniObjectFinder, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFiniObjectFinder_hlp,
		tclFiniObjectFinder_use);

   shTclDeclare(interp,regTrimToThreshold_name,
		(Tcl_CmdProc *)tclRegTrimToThreshold, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegTrimToThreshold_hlp,
		tclRegTrimToThreshold_use);

   shTclDeclare(interp,objectChainTrim_name,
		(Tcl_CmdProc *)tclObjectChainTrim, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjectChainTrim_hlp,
		tclObjectChainTrim_use);
}
