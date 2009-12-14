/*
 * Export JEG's object profile extraction code to TCL
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "dervish.h"
#include "phUtils.h"
#include "phExtract.h"

static char *module = "phTclExtract";    /* name of this set of code */

/*****************************************************************************/
static char *tclInitProfileExtract_use =
  "USAGE: InitProfileExtract";
#define tclInitProfileExtract_hlp \
  "allocate and initialise data structures for profileExtract"

static ftclArgvInfo initProfileExtract_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclInitProfileExtract_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclInitProfileExtract(
		      ClientData clientDatag,
		      Tcl_Interp *interp,
		      int ac,
		      char **av
		      )
{
   shErrStackClear();

   if(get_FtclOpts(interp,&ac,av,initProfileExtract_opts) != TCL_OK) {
      return(TCL_ERROR);
   }

   phInitProfileExtract();

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclFiniProfileExtract_use =
  "USAGE: FiniProfileExtract";
#define tclFiniProfileExtract_hlp \
  "deallocate all data structures used by profileExtract"

static ftclArgvInfo finiProfileExtract_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFiniProfileExtract_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFiniProfileExtract(
		      ClientData clientDatag,
		      Tcl_Interp *interp,
		      int ac,
		      char **av
		      )
{
   shErrStackClear();

   if(get_FtclOpts(interp,&ac,av,finiProfileExtract_opts) != TCL_OK) {
      return(TCL_ERROR);
   }

   phFiniProfileExtract();

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Extract a profile in sectors
 */
static char *tclProfileExtract_use =
  "USAGE: profileExtract reg xc yc outer sky skysig [-id id]";
#define tclProfileExtract_hlp \
  ""

static ftclArgvInfo profileExtract_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclProfileExtract_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "handle to region"},
   {"<row_c>", FTCL_ARGV_DOUBLE, NULL, NULL, "Centre of object (row)"},
   {"<col_c>", FTCL_ARGV_DOUBLE, NULL, NULL, "Centre of object (column)"},
   {"<outer>", FTCL_ARGV_INT, NULL, NULL, "radius of largest annulus"},
   {"<sky>", FTCL_ARGV_DOUBLE, NULL, NULL, "sky level"},
   {"<sigma>", FTCL_ARGV_DOUBLE, NULL, NULL, "standard deviation of sky"},
   {"-id", FTCL_ARGV_INT, NULL, NULL, "ID for object"},
   {"-keep_profile", FTCL_ARGV_CONSTANT, (void *)1, NULL,
						  "Keep old parts of profile"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclProfileExtract(
		  ClientData clientDatag,
		  Tcl_Interp *interp,
		  int ac,
		  char **av
		  )
{
   int i;
   CELL_STATS *cells;
   HANDLE hand;
   int id;
   int keep;
   char name[HANDLE_NAMELEN];
   int orad;
   char *regName = NULL; REGION *reg;
   double rowc = 0, colc = 0;
   double sky = 0, skysig = 0;
   void *vptr;				/* used by shTclHandleExprEval */
   
   shErrStackClear();
   i = 1;
   profileExtract_opts[i++].dst = &regName;
   profileExtract_opts[i++].dst = &rowc;
   profileExtract_opts[i++].dst = &colc;
   profileExtract_opts[i++].dst = &orad;
   profileExtract_opts[i++].dst = &sky;
   profileExtract_opts[i++].dst = &skysig;
   profileExtract_opts[i++].dst = &id; id = -1;
   profileExtract_opts[i++].dst = &keep; keep = 0;
   shAssert(profileExtract_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,profileExtract_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * process those arguments
 */
   if(*regName == '\0' ||
      strcmp(regName, "null") == 0 ||  strcmp(regName, "NULL") == 0) {
      reg = NULL;
   } else {
      if(shTclHandleExprEval(interp,regName,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("REGION")) {
	 Tcl_SetResult(interp,"profileExtract: first argument is not a REGION",
		       TCL_STATIC);
	 return(TCL_ERROR);
      }
      reg = hand.ptr;
   }
/*
 * work
 */
   cells = phProfileExtract(id, -1, reg, rowc, colc, orad, sky, skysig, keep);
   if(cells == NULL) {
      if(reg == NULL) {
	 return(TCL_OK);
      } else {
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
   }
/*
 * and return result
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = cells;
   hand.type = shTypeGetFromName("CELL_STATS");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new CELL_STATS handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclProfileExtractLinear_use =
  "USAGE: profileExtractLinear <reg> <row_0> col_0> <row_1> <col_1> "
"<hwidth> <sky> <skysigma>";
#define tclProfileExtractLinear_hlp \
  "Returns a CELL_STATS for the (linear) profile perpendicular to the object "\
"stretching from point 0 to point 1. The profile's half width is <hwidth>."

static ftclArgvInfo profileExtractLinear_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclProfileExtractLinear_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "handle to region"},
   {"<row_0>", FTCL_ARGV_DOUBLE, NULL, NULL, "one end of object (row)"},
   {"<col_0>", FTCL_ARGV_DOUBLE, NULL, NULL, "one end of object (column)"},
   {"<row_1>", FTCL_ARGV_DOUBLE, NULL, NULL, "other end of object (row)"},
   {"<col_1>", FTCL_ARGV_DOUBLE, NULL, NULL, "other end of object (column)"},
   {"<hwidth>", FTCL_ARGV_INT, NULL, NULL, "half-width of strip"},
   {"<sky>", FTCL_ARGV_DOUBLE, NULL, NULL, "sky level"},
   {"<sigma>", FTCL_ARGV_DOUBLE, NULL, NULL, "standard deviation of sky"},
   {"-bin", FTCL_ARGV_INT, NULL, NULL, "width of bin"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclProfileExtractLinear(
			ClientData clientDatag,
			Tcl_Interp *interp,
			int ac,
			char **av
			)
{
   int i;
   CELL_STATS *cells;
   HANDLE hand;
   char name[HANDLE_NAMELEN];
   void *vptr;				/* used by shTclHandleExprEval */
   char *regStr = NULL;			/* handle to region */
   double row_0 = 0.0;			/* one end of object (row) */
   double col_0 = 0.0;			/* one end of object (column) */
   double row_1 = 0.0;			/* other end of object (row) */
   double col_1 = 0.0;			/* other end of object (column) */
   int hwidth = 0;			/* half-width of strip */
   double sky = 0.0;			/* sky level */
   double sigma = 0.0;			/* standard deviation of sky */
   int bin = 0;				/* width of bin */
   
   shErrStackClear();

   bin = 0;

   i = 1;
   profileExtractLinear_opts[i++].dst = &regStr;
   profileExtractLinear_opts[i++].dst = &row_0;
   profileExtractLinear_opts[i++].dst = &col_0;
   profileExtractLinear_opts[i++].dst = &row_1;
   profileExtractLinear_opts[i++].dst = &col_1;
   profileExtractLinear_opts[i++].dst = &hwidth;
   profileExtractLinear_opts[i++].dst = &sky;
   profileExtractLinear_opts[i++].dst = &sigma;
   profileExtractLinear_opts[i++].dst = &bin;
   shAssert(profileExtractLinear_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,profileExtractLinear_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * process arguments
 */
   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"profileExtractLinear: "
		    "first argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   cells = phProfileExtractLinear(-1,hand.ptr, row_0, col_0, row_1, col_1,
				   hwidth, bin, sky, sigma);
   if(cells == NULL) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * and return result
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = cells;
   hand.type = shTypeGetFromName("CELL_STATS");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new CELL_STATS handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);


   return(TCL_OK);
}

/*****************************************************************************/
/*
 * extract a cross section from an image
 */
static char *tclProfileXsectionExtract_use =
  "USAGE: profileXsectionExtract "
			       "<reg> <row_0> col_0> <row_1> <col_1> <hwidth>";
#define tclProfileXsectionExtract_hlp \
  "Extract a cross section of an image, returning it as a VECTOR"

static ftclArgvInfo profileXsectionExtract_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclProfileXsectionExtract_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "handle to region"},
   {"<row_0>", FTCL_ARGV_DOUBLE, NULL, NULL, "one end of cross-section (row)"},
   {"<col_0>", FTCL_ARGV_DOUBLE, NULL, NULL,
					   "one end of cross-section (column)"},
   {"<row_1>", FTCL_ARGV_DOUBLE, NULL, NULL,"other end of cross-section (row)"},
   {"<col_1>", FTCL_ARGV_DOUBLE, NULL, NULL,
					 "other end of cross-section (column)"},
   {"<hwidth>", FTCL_ARGV_INT, NULL, NULL, "half-width of strip"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclProfileXsectionExtract(
			  ClientData clientDatag,
			  Tcl_Interp *interp,
			  int ac,
			  char **av
			  )
{
   int i;
   VECTOR *vec;				/* the answer */
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int n;				/* number of points in cross-section */
   REGION *reg;
   float *sect;				/* the cross-section */
   void *vptr;				/* used by shTclHandleExprEval */
   char *regStr = NULL;			/* handle to region */
   double row_0 = 0.0;			/* one end of cross-section (row) */
   double col_0 = 0.0;			/* one end of cross-section (column) */
   double row_1 = 0.0;			/* other end of cross-section (row) */
   double col_1 = 0.0;			/* other end of cross-section (column)*/
   int hwidth = 0;			/* half-width of strip */

   shErrStackClear();

   i = 1;
   profileXsectionExtract_opts[i++].dst = &regStr;
   profileXsectionExtract_opts[i++].dst = &row_0;
   profileXsectionExtract_opts[i++].dst = &col_0;
   profileXsectionExtract_opts[i++].dst = &row_1;
   profileXsectionExtract_opts[i++].dst = &col_1;
   profileXsectionExtract_opts[i++].dst = &hwidth;
   shAssert(profileXsectionExtract_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,profileXsectionExtract_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * process arguments
 */
   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"profileExtractLinear: "
		    "first argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;
   if(reg->type != TYPE_PIX) {
      Tcl_SetResult(interp,"profileExtractLinear: "
		    "first argument is not a U16 REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   
/*
 * do the work
 */
   sect = phXsectionExtract(reg, row_0, col_0, row_1, col_1, hwidth, &n);

   vec = shVectorNew(n);
   for(i = 0;i < n;i++) {
      vec->vec[i] = sect[i];
   }
/*
 * return answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = vec;
   hand.type = shTypeGetFromName("VECTOR");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new VECTOR handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   
   return(TCL_OK);
}


/*****************************************************************************/
/*
 * Print a radial profile to a file
 */
static char *tclProfilePrint_use =
  "USAGE: ProfilePrint <file> <cell_stats> [-append]";
#define tclProfilePrint_hlp \
  "Write a CELL_STATS structure to a file"

static ftclArgvInfo profilePrint_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclProfilePrint_hlp},
   {"<file>", FTCL_ARGV_STRING, NULL, NULL, "file to print to"},
   {"<cells>", FTCL_ARGV_STRING, NULL, NULL, "handle to a CELL_STATS"},
   {"-append", FTCL_ARGV_CONSTANT, (void *)1, NULL, "append to file"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclProfilePrint(
		ClientData clientDatag,
		Tcl_Interp *interp,
		int ac,
		char **av
		)
{
   CELL_STATS *cstats;
   FILE *fil;
   HANDLE hand;
   int i;
   void *vptr;				/* used by shTclHandleExprEval */
   char *fileStr = NULL;		/* file to print to */
   char *cellsStr = NULL;		/* handle to a CELL_STATS */
   int append = 0;			/* append to file */

   shErrStackClear();

   i = 1;
   profilePrint_opts[i++].dst = &fileStr;
   profilePrint_opts[i++].dst = &cellsStr;
   profilePrint_opts[i++].dst = &append;
   shAssert(profilePrint_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,profilePrint_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * process args
 */
   if(shTclHandleExprEval(interp,cellsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CELL_STATS")) {
      Tcl_SetResult(interp,"profilePrint: second argument is not a CELL_STATS",
		    						   TCL_STATIC);
      return(TCL_ERROR);
   }
   cstats = hand.ptr;
/*
 * do the work
 */
   if((fil = fopen(fileStr,append ? "a" : "w")) == NULL) {
      Tcl_SetResult(interp,"Can't open file",TCL_STATIC);
      return(TCL_ERROR);
   }

   fprintf(fil,"# n <col> <row> 1/<1/r>   med mean sigma\n");
   fprintf(fil,"%d\n",cstats->ncell);
   for(i = 0;i < cstats->ncell;i++) {
      fprintf(fil,"%g  %g %g %g  %g %g %g\n",
	      cstats->geom[i].n,
	      cstats->mgeom[i].col,cstats->mgeom[i].row,cstats->mgeom[i].rmean,
	      cstats->cells[i].qt[1],cstats->cells[i].mean,
	      cstats->cells[i].sig);
   }

   fclose(fil);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Set objmask where a profile is above a threshold
 */
static char *tclObjmaskSetFromProfile_use =
  "USAGE: objmaskSetFromProfile "
  "<objmask> <val> <cstats> <thresh> [-nsec N] [-clip N]";
#define tclObjmaskSetFromProfile_hlp \
"Set <val>s bits in <objmask> wherever the profile <cells> exceeds <thresh>."\
"If -nsec is specified, only complete annuli are masked, and only if they "\
"have more than N sectors above threshold. "\
"If -clip is specified, calculate mean of sector annuli (clipping N sectors "\
"top and bottom), and compare this to threshold before masking entire annulus"


static ftclArgvInfo objmaskSetFromProfile_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjmaskSetFromProfile_hlp},
   {"<ncol>", FTCL_ARGV_INT, NULL, NULL, "number of columns in REGION"},
   {"<nrow>", FTCL_ARGV_INT, NULL, NULL, "number of rows in REGION"},
   {"<cells>", FTCL_ARGV_STRING, NULL, NULL, "handle to a CELL_STATS"},
   {"<thresh>", FTCL_ARGV_DOUBLE, NULL, NULL, "threshold to use"},
   {"-nsec", FTCL_ARGV_INT, NULL, NULL, "Number of sectors above <thesh> to mask annulus (default 0)"},
   {"-clip", FTCL_ARGV_INT, NULL, NULL, "Number of sectors to clip in mean(sector median) as annulus's value"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjmaskSetFromProfile(
		      ClientData clientDatag,
		      Tcl_Interp *interp,
		      int ac,
		      char **av
   )
{
   int i;
   CELL_STATS *cstats;
   HANDLE hand;
   OBJMASK *mask;
   char name[HANDLE_NAMELEN];
   void *vptr;				/* used by shTclHandleExprEval */
   int ncol = 0;			/* number of columns in REGION */
   int nrow = 0;			/* number of rows in REGION */
   char *cellsStr = NULL;		/* handle to a CELL_STATS */
   double thresh = 0.0;			/* threshold to use */
   int nsec = 0;			/* Number of sectors above <thesh>
					   to mask annulus */
   int clip = -1;			/* Number of sectors to clip in
					   mean(sector median) */
   
   shErrStackClear();

   i = 1;
   objmaskSetFromProfile_opts[i++].dst = &ncol;
   objmaskSetFromProfile_opts[i++].dst = &nrow;
   objmaskSetFromProfile_opts[i++].dst = &cellsStr;
   objmaskSetFromProfile_opts[i++].dst = &thresh;
   objmaskSetFromProfile_opts[i++].dst = &nsec;
   objmaskSetFromProfile_opts[i++].dst = &clip;
   shAssert(objmaskSetFromProfile_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objmaskSetFromProfile_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * process arguments
 */
   if(shTclHandleExprEval(interp,cellsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CELL_STATS")) {
      Tcl_SetResult(interp,"objmaskSetFromProfile: "
		    "third argument is not a CELL_STATS",
		    						   TCL_STATIC);
      return(TCL_ERROR);
   }
   cstats = hand.ptr;
/*
 * do the work
 */
   mask = phObjmaskSetFromProfile(ncol, nrow, cstats, thresh, nsec, clip);
/*
 * and return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = mask;
   hand.type = shTypeGetFromName("OBJMASK");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new OBJMASK handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclObjmaskSetFromProfileLinear_use =
  "USAGE: ObjmaskSetFromProfileLinear ";
#define tclObjmaskSetFromProfileLinear_hlp \
  ""

static ftclArgvInfo objmaskSetFromProfileLinear_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjmaskSetFromProfileLinear_hlp},
   {"<ncol>", FTCL_ARGV_INT, NULL, NULL, "number of columns in REGION"},
   {"<nrow>", FTCL_ARGV_INT, NULL, NULL, "number of rows in REGION"},
   {"<cstats>", FTCL_ARGV_STRING, NULL, NULL, "CELL_STATS describing profile"},
   {"<thresh>", FTCL_ARGV_DOUBLE, NULL, NULL, "desired threshold"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjmaskSetFromProfileLinear(
			    ClientData clientDatag,
			    Tcl_Interp *interp,
			    int ac,
			    char **av
			    )
{
   int i;
   HANDLE hand;
   char name[HANDLE_NAMELEN];
   OBJMASK *mask;
   void *vptr;				/* used by shTclHandleExprEval */
   int ncol = 0;			/* number of columns in REGION */
   int nrow = 0;			/* number of rows in REGION */
   char *cstatsStr = NULL;		/* CELL_STATS describing profile */
   double thresh = 0.0;			/* desired threshold */

   shErrStackClear();

   i = 1;
   objmaskSetFromProfileLinear_opts[i++].dst = &ncol;
   objmaskSetFromProfileLinear_opts[i++].dst = &nrow;
   objmaskSetFromProfileLinear_opts[i++].dst = &cstatsStr;
   objmaskSetFromProfileLinear_opts[i++].dst = &thresh;
   shAssert(objmaskSetFromProfileLinear_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objmaskSetFromProfileLinear_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,cstatsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CELL_STATS")) {
      Tcl_SetResult(interp,"objmaskSetFromProfileLinear: "
			      "third argument is not a CELL_STATS",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * do the work
 */
   mask = phObjmaskSetFromProfileLinear(ncol, nrow, hand.ptr,thresh);
/*
 * and return it
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = mask;
   hand.type = shTypeGetFromName("OBJMASK");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new OBJMASK handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);


   return(TCL_OK);
}

/*****************************************************************************/
/*
 * set mask bits along a specified ridgeline. This is mostly exported to
 * TCL for debugging
 */
static char *tclObjmaskSetLinear_use =
  "USAGE: ObjmaskSetLinear ";
#define tclObjmaskSetLinear_hlp \
  ""

static ftclArgvInfo objmaskSetLinear_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjmaskSetLinear_hlp},
   {"<mask>", FTCL_ARGV_STRING, NULL, NULL, "Objmask to set"},
   {"<ncol>", FTCL_ARGV_INT, NULL, NULL, "number of columns in REGION"},
   {"<nrow>", FTCL_ARGV_INT, NULL, NULL, "number of rows in REGION"},
   {"<r0>", FTCL_ARGV_DOUBLE, NULL, NULL, "Starting row"},
   {"<c0>", FTCL_ARGV_DOUBLE, NULL, NULL, "Starting column"},
   {"<r1>", FTCL_ARGV_DOUBLE, NULL, NULL, "Ending row"},
   {"<c1>", FTCL_ARGV_DOUBLE, NULL, NULL, "Ending column"},
   {"<hwidth_l>", FTCL_ARGV_INT, NULL, NULL,
					  "Half-width to left (or above) line"},
   {"<hwidth_r>", FTCL_ARGV_INT, NULL, NULL,
					 "Half-width to right (or below) line"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjmaskSetLinear(
		 ClientData clientDatag,
		 Tcl_Interp *interp,
		 int ac,
		 char **av
		 )
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *maskStr = NULL;		/* Objmask to set */
   int ncol = 0;			/* number of columns in REGION */
   int nrow = 0;			/* number of rows in REGION */
   double r0 = 0.0;			/* Starting row */
   double c0 = 0.0;			/* Starting column */
   double r1 = 0.0;			/* Ending row */
   double c1 = 0.0;			/* Ending column */
   int hwidth_l = 0;			/* Half-width to left (or above) line */
   int hwidth_r = 0;			/* Half-width to right (or below) line*/

   shErrStackClear();

   i = 1;
   objmaskSetLinear_opts[i++].dst = &maskStr;
   objmaskSetLinear_opts[i++].dst = &ncol;
   objmaskSetLinear_opts[i++].dst = &nrow;
   objmaskSetLinear_opts[i++].dst = &r0;
   objmaskSetLinear_opts[i++].dst = &c0;
   objmaskSetLinear_opts[i++].dst = &r1;
   objmaskSetLinear_opts[i++].dst = &c1;
   objmaskSetLinear_opts[i++].dst = &hwidth_l;
   objmaskSetLinear_opts[i++].dst = &hwidth_r;
   shAssert(objmaskSetLinear_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objmaskSetLinear_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * process arguments
 */
   if(shTclHandleExprEval(interp,maskStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJMASK")) {
      Tcl_SetResult(interp,"objmaskSetLinear: "
				"first argument is not an OBJMASK",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * do the work. Note that we switch dval{0,1} and dval{2,3} to agree with
 * phObjmaskSetLinear's argument list
 */
   phObjmaskSetLinear(hand.ptr, ncol, nrow, c0, r0, c1, r1, hwidth_l, hwidth_r);

   return(TCL_OK);
}


/**********************************************************************
**<AUTO EXTRACT>
** TCL VERB: cellprofNew
**
** Description:
**
** Return a handle to a new CELL_PROF
**
**</AUTO>
**********************************************************************/

static char *tclCellProfNew_use =
  "USAGE: cellprofNew [-help]";
#define tclCellProfNew_hlp \
  "create a handle to new CELL_PROF and allocate memory for the data." 

static ftclArgvInfo cellprofNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCellProfNew_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclCellProfNew(
	    ClientData clientDatag,
	    Tcl_Interp *interp,
	    int ac,
	    char **av
	    )
{
   CELL_PROF *cellprof;
   HANDLE hand;
   char name[HANDLE_NAMELEN];
   
   shErrStackClear();

   if(get_FtclOpts(interp,&ac,av,cellprofNew_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
 /*
 * work
 */
   cellprof = phCellProfNew(MAXCELLS);

/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = cellprof;
   hand.type = shTypeGetFromName("CELL_PROF");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new CELL_PROF handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}


/**********************************************************************
**<AUTO EXTRACT>
** TCL VERB: cellprofDel
**
** Description:
**
** Delete a CELL_PROF
**
**</AUTO>
**********************************************************************/

static char *tclCellProfDel_use =
  "USAGE: cellprofDel cellprof";
static char *tclCellProfDel_hlp =
  "Delete a CELL_PROF";

static int
tclCellProfDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *cellprof;
   char *opts = "cellprof";

   shErrStackClear();

   ftclParseSave("cellprofDel");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      cellprof = ftclGetStr("cellprof");
      if(p_shTclHandleAddrGet(interp,cellprof,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      ftclParseRestore("cellprofDel");
      Tcl_SetResult(interp,tclCellProfDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if(handle->type != shTypeGetFromName("CELL_PROF")) {
      Tcl_SetResult(interp,"handle is not a CELL_PROF",TCL_STATIC);
      return(TCL_ERROR);
   }

   phCellProfDel(handle->ptr);
   (void) p_shTclHandleDel(interp,cellprof);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}



/*****************************************************************************/
/*
 * return the radial profile array
 */
static char *tclProfileRadii_use =
  "USAGE: profileRadii";
#define tclProfileRadii_hlp \
  "Return a list of the outer radii of profileExtract's annuli"

static ftclArgvInfo profileRadii_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclProfileRadii_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclProfileRadii(
		ClientData clientDatag,
		Tcl_Interp *interp,
		int ac,
		char **av
		)
{
   char buff[100];			/* return result */
   int i;
   const CELL_STATS *prof;

   shErrStackClear();

   if(get_FtclOpts(interp,&ac,av,profileRadii_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */

/*
 * work
 */
   prof = phProfileGeometry();
/*
 * return answer
 */
   for(i = 1;i < NANN + 1;i++) {
      sprintf(buff,"%f",prof->radii[i]);
      Tcl_AppendElement(interp,buff);
   }

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclApoApertureEval_use =
  "USAGE: apoApertureEval <reg> <n> <bkgd> <rowc> <colc>";
#define tclApoApertureEval_hlp \
  "Evaluate the flux in the nth apodised aperture"

static ftclArgvInfo apoApertureEval_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclApoApertureEval_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region to extract value from"},
   {"<n>", FTCL_ARGV_INT, NULL, NULL, "Which aperture?"},
   {"<bkgd>", FTCL_ARGV_DOUBLE, NULL, NULL, "Background level of region"},
   {"<rowc>", FTCL_ARGV_DOUBLE, NULL, NULL, "row centre of desired aperture"},
   {"<colc>", FTCL_ARGV_DOUBLE, NULL, NULL,
					  "column centre of desired aperture"},
   {"-asigma", FTCL_ARGV_DOUBLE, NULL, NULL,
			  "Sigma with which to apodise aperture (default: 0)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclApoApertureEval(ClientData clientDatag,
		   Tcl_Interp *interp,
		   int ac,
		   char **av)
{
   HANDLE hand;
   int i;
   void *vptr;				/* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region to extract value from */
   REGION *reg;
   int n = 0;				/* Which aperture? */
   double bkgd = 0.0;			/* Background level of region */
   double rowc = 0.0;			/* row centre of desired aperture */
   double colc = 0.0;			/* column centre of desired aperture */
   double asigma = 0.0;			/* Sigma with which to apodise */
   float val;				/* returned value */

   shErrStackClear();

   i = 1;
   apoApertureEval_opts[i++].dst = &regStr;
   apoApertureEval_opts[i++].dst = &n;
   apoApertureEval_opts[i++].dst = &bkgd;
   apoApertureEval_opts[i++].dst = &rowc;
   apoApertureEval_opts[i++].dst = &colc;
   apoApertureEval_opts[i++].dst = &asigma;
   shAssert(apoApertureEval_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,apoApertureEval_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"apoApertureEval: "
                    "argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;
/*
 * work
 */
   if(phApoApertureEval(reg, n, asigma, bkgd, rowc, colc, &val) < 0) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   {
      char buff[30];
      sprintf(buff, "%g", val);
      Tcl_AppendElement(interp,buff);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclApoApertureMomentsEval_use =
  "USAGE: apoApertureMomentsEval <reg> <n> <bkgd> <rowc> <colc> -asigma n";
#define tclApoApertureMomentsEval_hlp \
  "Evaluate the row- and column image moments in the nth apodised aperture"

static ftclArgvInfo apoApertureMomentsEval_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclApoApertureMomentsEval_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region to extract value from"},
   {"<n>", FTCL_ARGV_INT, NULL, NULL, "Which aperture?"},
   {"<bkgd>", FTCL_ARGV_DOUBLE, NULL, NULL, "Background level of region"},
   {"<rowc>", FTCL_ARGV_DOUBLE, NULL, NULL, "row centre of desired aperture"},
   {"<colc>", FTCL_ARGV_DOUBLE, NULL, NULL,
					  "column centre of desired aperture"},
   {"-asigma", FTCL_ARGV_DOUBLE, NULL, NULL,
			  "Sigma with which to apodise aperture (default: 0)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclApoApertureMomentsEval(ClientData clientDatag,
			  Tcl_Interp *interp,
			  int ac,
			  char **av)
{
   HANDLE hand;
   int i;
   void *vptr;				/* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region to extract value from */
   REGION *reg;
   int n = 0;				/* Which aperture? */
   double bkgd = 0.0;			/* Background level of region */
   double rowc = 0.0;			/* row centre of desired aperture */
   double colc = 0.0;			/* column centre of desired aperture */
   float mr, mc;			/* <row|col> */
   double asigma = 0.0;			/* Sigma with which to apodise */

   shErrStackClear();

   i = 1;
   apoApertureMomentsEval_opts[i++].dst = &regStr;
   apoApertureMomentsEval_opts[i++].dst = &n;
   apoApertureMomentsEval_opts[i++].dst = &bkgd;
   apoApertureMomentsEval_opts[i++].dst = &rowc;
   apoApertureMomentsEval_opts[i++].dst = &colc;
   apoApertureMomentsEval_opts[i++].dst = &asigma;
   shAssert(apoApertureMomentsEval_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,apoApertureMomentsEval_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"apoApertureMomentsEval: "
                    "argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;
/*
 * work
 */
   if(phApoApertureMomentsEval(reg, n, asigma, bkgd, rowc, colc, &mr, &mc)
									 < 0) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   {
      char buff[40];
      sprintf(buff, "%g %g", mr, mc);
      Tcl_AppendElement(interp,buff);
   }

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * evaluate a naive filter (it's not actually apodized)
 */
static char *tclApoApertureEvalNaive_use =
  "USAGE: apoApertureEvalNaive ";
#define tclApoApertureEvalNaive_hlp \
  ""

static ftclArgvInfo apoApertureEvalNaive_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclApoApertureEvalNaive_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "region to measure"},
   {"<r>", FTCL_ARGV_DOUBLE, NULL, NULL, "desired radius"},
   {"<bkgd>", FTCL_ARGV_DOUBLE, NULL, NULL, "Background level"},
   {"<rowc>", FTCL_ARGV_DOUBLE, NULL, NULL, "Row centre of aperture"},
   {"<colc>", FTCL_ARGV_DOUBLE, NULL, NULL, "Column centre of aperture"},
   {"-asigma", FTCL_ARGV_DOUBLE, NULL, NULL,
			  "Sigma with which to apodise aperture (default: 0)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclApoApertureEvalNaive(ClientData clientDatag,
			Tcl_Interp *interp,
			int ac,
			char **av)
{
   HANDLE hand;
   int i;
   void *vptr;				/* used by shTclHandleExprEval */
   char *regStr = NULL;			/* region to measure */
   REGION *reg;
   double r = 0.0;			/* desired radius */
   double bkgd = 0.0;			/* Background level */
   double rowc = 0.0;			/* Row centre of aperture */
   double colc = 0.0;			/* Column centre of aperture */
   float val;				/* returned value */
   double asigma = 0.0;			/* Sigma with which to apodise aper. */

   shErrStackClear();

   i = 1;
   apoApertureEvalNaive_opts[i++].dst = &regStr;
   apoApertureEvalNaive_opts[i++].dst = &r;
   apoApertureEvalNaive_opts[i++].dst = &bkgd;
   apoApertureEvalNaive_opts[i++].dst = &rowc;
   apoApertureEvalNaive_opts[i++].dst = &colc;
   apoApertureEvalNaive_opts[i++].dst = &asigma;
   shAssert(apoApertureEvalNaive_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,apoApertureEvalNaive_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"apoApertureEvalNaive: "
                    "argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;
/*
 * work
 */
   if(phApoApertureEvalNaive(reg, r, asigma, bkgd, rowc, colc, &val) < 0) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   {
      char buff[30];
      sprintf(buff, "%g", val);
      Tcl_AppendElement(interp,buff);
   }

   return(TCL_OK);
}


/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclExtractDeclare(Tcl_Interp *interp)
{

   shTclDeclare(interp,"profileExtract",
		(Tcl_CmdProc *)tclProfileExtract, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclProfileExtract_hlp,
		tclProfileExtract_use);

   shTclDeclare(interp,"initProfileExtract",
		(Tcl_CmdProc *)tclInitProfileExtract, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclInitProfileExtract_hlp,
		tclInitProfileExtract_use);

   shTclDeclare(interp,"finiProfileExtract",
		(Tcl_CmdProc *)tclFiniProfileExtract, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFiniProfileExtract_hlp,
		tclFiniProfileExtract_use);

   shTclDeclare(interp,"profilePrint",
		(Tcl_CmdProc *)tclProfilePrint, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclProfilePrint_hlp,
		tclProfilePrint_use);

   shTclDeclare(interp,"objmaskSetFromProfile",
		(Tcl_CmdProc *)tclObjmaskSetFromProfile, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjmaskSetFromProfile_hlp,
		tclObjmaskSetFromProfile_use);

   shTclDeclare(interp,"profileExtractLinear",
		(Tcl_CmdProc *)tclProfileExtractLinear, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclProfileExtractLinear_hlp,
		tclProfileExtractLinear_use);

   shTclDeclare(interp,"profileXsectionExtract",
		(Tcl_CmdProc *)tclProfileXsectionExtract, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclProfileXsectionExtract_hlp,
		tclProfileXsectionExtract_use);

   shTclDeclare(interp,"objmaskSetFromProfileLinear",
		(Tcl_CmdProc *)tclObjmaskSetFromProfileLinear, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjmaskSetFromProfileLinear_hlp,
		tclObjmaskSetFromProfileLinear_use);

   shTclDeclare(interp,"objmaskSetLinear",
		(Tcl_CmdProc *)tclObjmaskSetLinear, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjmaskSetLinear_hlp,
		tclObjmaskSetLinear_use);

   shTclDeclare(interp,"profileRadii",
		(Tcl_CmdProc *)tclProfileRadii, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclProfileRadii_hlp,
		tclProfileRadii_use);

   shTclDeclare(interp,"cellprofNew",
		(Tcl_CmdProc *)tclCellProfNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclCellProfNew_hlp,
		tclCellProfNew_use);

   shTclDeclare(interp,"cellprofDel",
		(Tcl_CmdProc *)tclCellProfDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclCellProfDel_hlp,
		tclCellProfDel_use);

   shTclDeclare(interp,"apoApertureEval",
		(Tcl_CmdProc *)tclApoApertureEval, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclApoApertureEval_hlp,
		tclApoApertureEval_use);

   shTclDeclare(interp,"apoApertureMomentsEval",
		(Tcl_CmdProc *)tclApoApertureMomentsEval, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclApoApertureMomentsEval_hlp,
		tclApoApertureMomentsEval_use);

   shTclDeclare(interp,"apoApertureEvalNaive",
		(Tcl_CmdProc *)tclApoApertureEvalNaive, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclApoApertureEvalNaive_hlp,
		tclApoApertureEvalNaive_use);
}
