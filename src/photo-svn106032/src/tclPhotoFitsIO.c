/*
 * Export the line-orientated fits binary table stuff to TCL
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "phUtils.h"
#include "phPhotoFitsIO.h"

static char *module = "phTclPhotoFitsIO"; /* name of this set of code */

/*****************************************************************************/
/*
 * We would like to use schemaTransEntryAdd to add entries to SCHEMATRANSs,
 * but it's too picky, requiring fields that we don't use. So add an equivalent
 * but simpler TCL verb, fitsBinSchemaTransEntryAdd
 */
static char *tclFitsBinSchemaTransEntryAdd_use =
  "USAGE: FitsBinSchemaTransEntryAdd <schematrans> <fitsName> <CName> "
							      "[fitsDataType]";
#define tclFitsBinSchemaTransEntryAdd_hlp \
  "Add an entry to a SCHEMATRANS; similar to schemaTransEntryAdd, but don't "\
"require fields that we don't use"

static ftclArgvInfo fitsBinSchemaTransEntryAdd_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFitsBinSchemaTransEntryAdd_hlp},
   {"<schematrans>", FTCL_ARGV_STRING, NULL, NULL, "SCHEMATRANS for <type>"},
   {"<fitsName>", FTCL_ARGV_STRING, NULL, NULL, "name of FITS column"},
   {"<CName>", FTCL_ARGV_STRING, NULL, NULL, "name of element of C struct"},
   {"-fitsDataType", FTCL_ARGV_STRING, NULL, NULL, "type in fits file"},
   {"-heapType", FTCL_ARGV_STRING, NULL, NULL, "type of heap data"},
   {"-dimen", FTCL_ARGV_STRING, NULL, NULL, "dimension of array"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFitsBinSchemaTransEntryAdd(
			      ClientData clientDatag,
			      Tcl_Interp *interp,
			      int ac,
			      char **av
			      )
{
   int i;
   HANDLE strans;			/* handle to schematrans */
   void *vptr;				/* used by shTclHandleExprEval */
   char *schematransStr = NULL;		/* SCHEMATRANS for <type> */
   char *fitsNameStr = NULL;		/* name of FITS column */
   char *CNameStr = NULL;		/* name of element of C struct */
   char *fitsDataTypeStr = NULL;	/* type in fits file */
   char *dimenStr = NULL;		/* dimension of array */
   char *heapTypeStr = "heap";		/* type of heap data; overrides
					   type in the SCHEMA */

   shErrStackClear();

   fitsDataTypeStr = ""; dimenStr = NULL;
   i = 1;
   fitsBinSchemaTransEntryAdd_opts[i++].dst = &schematransStr;
   fitsBinSchemaTransEntryAdd_opts[i++].dst = &fitsNameStr;
   fitsBinSchemaTransEntryAdd_opts[i++].dst = &CNameStr;
   fitsBinSchemaTransEntryAdd_opts[i++].dst = &fitsDataTypeStr;
   fitsBinSchemaTransEntryAdd_opts[i++].dst = &heapTypeStr;
   fitsBinSchemaTransEntryAdd_opts[i++].dst = &dimenStr;
   shAssert(fitsBinSchemaTransEntryAdd_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,fitsBinSchemaTransEntryAdd_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * Deal with arguments
 */
   if(shTclHandleExprEval(interp,schematransStr,&strans,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(strans.type != shTypeGetFromName("SCHEMATRANS")) {
      Tcl_SetResult(interp,"fitsBinSchemaTransEntryAdd: first argument is not "
		    				   "a SCHEMATRANS",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * We want to simply declare that something goes on the heap, but 
 * shSchemaTransEntryAdd forces me to declare irrelevant information;
 * what's more it returns SH_GENERIC_ERROR so I can't detect that this
 * is all that it's whingeing about. So placate it.
 */
   if(strcmp(fitsDataTypeStr,"heap") == 0) {
      if(shSchemaTransEntryAdd(strans.ptr,CONVERSION_BY_TYPE,fitsNameStr,
			       CNameStr, fitsDataTypeStr, heapTypeStr,
			       "-1", NULL, dimenStr,0.0,-1) != SH_SUCCESS) {
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
   } else {
      if(shSchemaTransEntryAdd(strans.ptr,CONVERSION_BY_TYPE,fitsNameStr,
			       CNameStr,fitsDataTypeStr,NULL,NULL,NULL,
					      dimenStr,0.0,-1) != SH_SUCCESS) {
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclFitsBinDeclareSchemaTrans_use =
  "USAGE: FitsBinDeclareSchemaTrans <type> <schematrans>";
#define tclFitsBinDeclareSchemaTrans_hlp \
  "Associate the <schematrans> with C-type <type>. The schematrans should " \
"_not_ be deleted; that's done for you when you call fitsBinForgetSchemaTrans"


static ftclArgvInfo fitsBinDeclareSchemaTrans_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFitsBinDeclareSchemaTrans_hlp},
   {"<schematrans>", FTCL_ARGV_STRING, NULL, NULL, "SCHEMATRANS for <type>"},
   {"<type>", FTCL_ARGV_STRING, NULL, NULL, "type we are declaring"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFitsBinDeclareSchemaTrans(
			     ClientData clientDatag,
			     Tcl_Interp *interp,
			     int ac,
			     char **av
			     )
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *schematransStr = NULL;		/* SCHEMATRANS for <type> */
   char *typeStr = NULL;		/* type we are declaring */

   shErrStackClear();

   i = 1;
   fitsBinDeclareSchemaTrans_opts[i++].dst = &schematransStr;
   fitsBinDeclareSchemaTrans_opts[i++].dst = &typeStr;
   shAssert(fitsBinDeclareSchemaTrans_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,fitsBinDeclareSchemaTrans_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * process results of argument parsing
 */
   if(shTclHandleExprEval(interp,schematransStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("SCHEMATRANS")) {
      Tcl_SetResult(interp,"fitsBinDeclareSchemaTrans: "
		    	    "second argument is not a SCHEMATRANS",TCL_STATIC);
      return(TCL_ERROR);
   }

   if(phFitsBinDeclareSchemaTrans(hand.ptr,typeStr) < 0) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclFitsBinForgetSchemaTrans_use =
  "USAGE: fitsBinForgetSchemaTrans <type>";
#define tclFitsBinForgetSchemaTrans_hlp \
  "Forget the SCHEMATRANS information associated with <type>; if <type> is "\
"the string \"NULL\", all types are forgotten. The declared SCHEMATRANS are "\
"deleted for you"

static ftclArgvInfo fitsBinForgetSchemaTrans_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFitsBinForgetSchemaTrans_hlp},
   {"<type>", FTCL_ARGV_STRING, NULL, NULL, "type we are declaring"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFitsBinForgetSchemaTrans(
			     ClientData clientDatag,
			     Tcl_Interp *interp,
			     int ac,
			     char **av
			     )
{
   int i;
   char *typeStr = NULL;		/* type we are declaring */
   shErrStackClear();

   i = 1;
   fitsBinForgetSchemaTrans_opts[i++].dst = &typeStr;
   shAssert(fitsBinForgetSchemaTrans_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,fitsBinForgetSchemaTrans_opts) != TCL_OK) {

      return(TCL_ERROR);
   }

   if(*typeStr == '\0' ||
      strcmp(typeStr,"null") == 0 || strcmp(typeStr,"NULL") == 0) {
      typeStr = NULL;
   }
   phFitsBinForgetSchemaTrans(typeStr);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclFitsBinTblOpen_use =
  "USAGE: FitsBinTblOpen <file> <flag> -hdr hdr";
#define tclFitsBinTblOpen_hlp \
"Open a fits binary table, and return a handle to it. If the optional -hdr\n\
is specified, copy cards from it into the binary table (write only)"

static ftclArgvInfo fitsBinTblOpen_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFitsBinTblOpen_hlp},
   {"<file>", FTCL_ARGV_STRING, NULL, NULL, "File to open"},
   {"<mode>", FTCL_ARGV_STRING, NULL, NULL, "How to open <file> (r, w, a)"},
   {"-hdr", FTCL_ARGV_STRING, NULL, NULL, "Handle to HDR structure"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFitsBinTblOpen(
	       ClientData clientDatag,
	       Tcl_Interp *interp,
	       int ac,
	       char **av
	       )
{
   int i;
   PHFITSFILE *fil;			/* binary table file descriptor */
   int flg;				/* how to open file */
   char handle[HANDLE_NAMELEN];
   HANDLE hand;
   HDR *hdr;				/* additional header cards */
   void *vptr;				/* used by shTclHandleExprEval */
   char *fileStr = NULL;		/* File to open */
   char *modeStr = NULL;		/* How to open <file> (r or w) */
   char *hdrStr = NULL;			/* Handle to HDR structure */

   shErrStackClear();

   i = 1;
   fitsBinTblOpen_opts[i++].dst = &fileStr;
   fitsBinTblOpen_opts[i++].dst = &modeStr;
   fitsBinTblOpen_opts[i++].dst = &hdrStr;
   shAssert(fitsBinTblOpen_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,fitsBinTblOpen_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * process args
 */
   if(hdrStr == NULL) {
      hdr = NULL;
   } else {
      if(shTclHandleExprEval(interp,hdrStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("HDR")) {
	 Tcl_SetResult(interp,"tclFitsBinTblOpen: third argument is not a HDR",
		       						   TCL_STATIC);
	 return(TCL_ERROR);
      }
      hdr = hand.ptr;
   }
/*
 * Convert the mode to an integer...
 */
   if(strcmp(modeStr,"r") == 0) {
      flg = 0;
   } else if(strcmp(modeStr,"w") == 0) {
      flg = 1;
   } else if(strcmp(modeStr,"a") == 0) {
      flg = 2;
      if(hdr != NULL) {
	 shErrStackPush("tclFitsBinTblOpen: "
			"You may not specify a primary header when appending");
	 return(TCL_ERROR);	 
      }
   } else {
      shErrStackPush("tclFitsBinTblOpen: "
		     "Illegal mode; please specify \"a\", \"r\", or \"w\"");
      return(TCL_ERROR);
   }
/*
 * ...and do the work
 */
   if((fil = phFitsBinTblOpen(fileStr, flg, hdr)) == NULL) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * Bind to handle and return it
 */
   if(p_shTclHandleNew(interp,handle) != TCL_OK) {
      phFitsBinTblClose(fil);
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = fil;
   hand.type = shTypeGetFromName("PHFITSFILE");
   
   if(p_shTclHandleAddrBind(interp,hand,handle) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new PHFITSFILE handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, handle, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclFitsBinTblClose_use =
  "USAGE: FitsBinTblClose <file-handle>";
#define tclFitsBinTblClose_hlp \
  "Close a file of FITS binary tables opened with fitsBinTblOpen"

static ftclArgvInfo fitsBinTblClose_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFitsBinTblClose_hlp},
   {"<file-handle>", FTCL_ARGV_STRING, NULL, NULL, "handle to open file"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFitsBinTblClose(
		   ClientData clientDatag,
		   Tcl_Interp *interp,
		   int ac,
		   char **av
		   )
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *file_handleStr = NULL;		/* handle to open file */
   
   shErrStackClear();

   i = 1;
   fitsBinTblClose_opts[i++].dst = &file_handleStr;
   shAssert(fitsBinTblClose_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,fitsBinTblClose_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * Process the arguments
 */
   if(shTclHandleExprEval(interp,file_handleStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PHFITSFILE")) {
      Tcl_SetResult(interp,"fitsBinTblClose: argument is not a PHFITSFILE",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * Do the work; first close the file...
 */
   if(phFitsBinTblClose(hand.ptr) != 0) {
      shTclInterpAppendWithErrStack(interp);
      p_shTclHandleDel(interp,file_handleStr);
      return(TCL_ERROR);
   }
/*
 * ...then destroy the handle
 */
   p_shTclHandleDel(interp,file_handleStr);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclFitsBinTblEnd_use =
  "USAGE: FitsBinTblEnd <file-handle>";
#define tclFitsBinTblEnd_hlp \
  "End a FITS binary table; the file remains open"

static ftclArgvInfo fitsBinTblEnd_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFitsBinTblEnd_hlp},
   {"<file-handle>", FTCL_ARGV_STRING, NULL, NULL, "handle to open file"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFitsBinTblEnd(
		   ClientData clientDatag,
		   Tcl_Interp *interp,
		   int ac,
		   char **av
		   )
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *file_handleStr = NULL;		/* handle to open file */
   
   shErrStackClear();

   i = 1;
   fitsBinTblEnd_opts[i++].dst = &file_handleStr;
   shAssert(fitsBinTblEnd_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,fitsBinTblEnd_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * Process the arguments
 */
   if(shTclHandleExprEval(interp,file_handleStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PHFITSFILE")) {
      Tcl_SetResult(interp,"fitsBinTblEnd: argument is not a PHFITSFILE",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * End that table (but don't close the file)
 */
   if(phFitsBinTblEnd(hand.ptr) != 0) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclFitsBinTblHdrRead_use =
  "USAGE: FitsBinTblHdrRead <file-handle> <type> -hdr hdr";
#define tclFitsBinTblHdrRead_hlp \
  "Read a header for a FITS binary table of objects of type <type>. "\
"If <type> is \"\" or NULL, skip the table entirely; if -hdr is specified, "\
"fill it out from the table's header. The number of rows in the table is returned."

static ftclArgvInfo fitsBinTblHdrRead_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFitsBinTblHdrRead_hlp},
   {"<file-handle>", FTCL_ARGV_STRING, NULL, NULL, "handle to open file"},
   {"<type>", FTCL_ARGV_STRING, NULL, NULL, "name of type to be read"},
   {"-hdr", FTCL_ARGV_STRING, NULL, NULL, "Handle to HDR structure"},
   {"-quiet", FTCL_ARGV_CONSTANT, (void *)1, NULL,
						"Suppress non-fatal messages"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFitsBinTblHdrRead(
		      ClientData clientDatag,
		      Tcl_Interp *interp,
		      int ac,
		      char **av
		      )
{
   int i;
   char buf[30];
   PHFITSFILE *fil;			/* binary table file descriptor */
   HANDLE hand;
   int nrow;				/* number of rows in table */
   HDR *hdr;
   void *vptr;				/* used by shTclHandleExprEval */
   char *file_handleStr = NULL;		/* handle to open file */
   char *typeStr = NULL;		/* name of type to be read */
   char *hdrStr = NULL;			/* Handle to HDR structure */
   int quiet = 0;			/* suppress some messages */
   
   shErrStackClear();

   hdrStr = NULL;
   i = 1;
   fitsBinTblHdrRead_opts[i++].dst = &file_handleStr;
   fitsBinTblHdrRead_opts[i++].dst = &typeStr;
   fitsBinTblHdrRead_opts[i++].dst = &hdrStr;
   fitsBinTblHdrRead_opts[i++].dst = &quiet;
   shAssert(fitsBinTblHdrRead_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,fitsBinTblHdrRead_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * process the arguments
 */
   if(shTclHandleExprEval(interp,file_handleStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PHFITSFILE")) {
      Tcl_SetResult(interp,"fitsBinHdrRead: argument is not a PHFITSFILE",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
   fil = hand.ptr;

   if(typeStr == NULL || *typeStr == '\0' ||
      strcmp(typeStr,"null") == 0 || strcmp(typeStr,"NULL") == 0) {
      typeStr = NULL;
   }

   if(hdrStr == NULL) {
      hdr = NULL;
   } else {
      if(shTclHandleExprEval(interp,hdrStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("HDR")) {
	 Tcl_SetResult(interp,"tclFitsBinHdrRead: third argument is not a HDR",
		       						   TCL_STATIC);
	 return(TCL_ERROR);
      }
      hdr = hand.ptr;
   }
/*
 * do the work
 */
   if(phFitsBinTblHdrRead(fil,typeStr,hdr,&nrow,quiet) != 0) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * and return the answer
 */
   sprintf(buf,"%d",nrow);
   Tcl_SetResult(interp,buf,TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclFitsBinTblHdrWrite_use =
  "USAGE: FitsBinTblHdrWrite <file-handle> <type> -hdr hdr";
#define tclFitsBinTblHdrWrite_hlp \
  "Write a header for a FITS binary table of objects of type <type>. If -hdr "\
"is specified it will be written to the table"

static ftclArgvInfo fitsBinTblHdrWrite_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFitsBinTblHdrWrite_hlp},
   {"<file-handle>", FTCL_ARGV_STRING, NULL, NULL, "handle to open file"},
   {"<type>", FTCL_ARGV_STRING, NULL, NULL, "name of type to be written"},
   {"-hdr", FTCL_ARGV_STRING, NULL, NULL, "Handle to HDR structure"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFitsBinTblHdrWrite(
		      ClientData clientDatag,
		      Tcl_Interp *interp,
		      int ac,
		      char **av
		      )
{
   int i;
   PHFITSFILE *fil;			/* binary table file descriptor */
   HANDLE hand;
   HDR *hdr;
   void *vptr;				/* used by shTclHandleExprEval */
   char *file_handleStr = NULL;		/* handle to open file */
   char *typeStr = NULL;		/* name of type to be written */
   char *hdrStr = NULL;			/* Handle to HDR structure */

   shErrStackClear();

   i = 1;
   fitsBinTblHdrWrite_opts[i++].dst = &file_handleStr;
   fitsBinTblHdrWrite_opts[i++].dst = &typeStr;
   fitsBinTblHdrWrite_opts[i++].dst = &hdrStr;
   shAssert(fitsBinTblHdrWrite_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,fitsBinTblHdrWrite_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * process the arguments
 */
   if(shTclHandleExprEval(interp,file_handleStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PHFITSFILE")) {
      Tcl_SetResult(interp,"fitsBinTblHdrWrite: argument is not a PHFITSFILE",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
   fil = hand.ptr;

   if(hdrStr == NULL) {
      hdr = NULL;
   } else {
      if(shTclHandleExprEval(interp,hdrStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("HDR")) {
	 Tcl_SetResult(interp,"tclFitsBinHdrRead: third argument is not a HDR",
		       						   TCL_STATIC);
	 return(TCL_ERROR);
      }
      hdr = hand.ptr;
   }
/*
 * do the work
 */
   if(phFitsBinTblHdrWrite(fil,typeStr,hdr) != 0) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclFitsBinTblRowSeek_use =
  "USAGE: FitsBinTblRowSeek <fd> <n> <whence>";
#define tclFitsBinTblRowSeek_hlp \
  "Seek n rows in table <fd> (must be open for read). <whence> may be 0 "\
"(n is relative to start of table), 1 (n is relative to current position) "\
"or 2 (n is relative to end of table)"

static ftclArgvInfo fitsBinTblRowSeek_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFitsBinTblRowSeek_hlp},
   {"<file>", FTCL_ARGV_STRING, NULL, NULL, "Handle to open file"},
   {"<n>", FTCL_ARGV_INT, NULL, NULL, "How many rows to seek"},
   {"<whence>", FTCL_ARGV_INT, NULL, NULL,
					  "Where to interpret <n> relative to"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFitsBinTblRowSeek(
		     ClientData clientDatag,
		     Tcl_Interp *interp,
		     int ac,
		     char **av
		     )
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *fileStr = NULL;		/* Handle to open file */
   int n = 0;				/* How many rows to seek */
   int whence = 0;			/* Where to interpret <n> relative to*/

   shErrStackClear();

   i = 1;
   fitsBinTblRowSeek_opts[i++].dst = &fileStr;
   fitsBinTblRowSeek_opts[i++].dst = &n;
   fitsBinTblRowSeek_opts[i++].dst = &whence;
   shAssert(fitsBinTblRowSeek_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,fitsBinTblRowSeek_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,fileStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PHFITSFILE")) {
      Tcl_SetResult(interp,"fitsBinTblRowSeek: "
                    "argument is not a PHFITSFILE",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   if((n = phFitsBinTblRowSeek(hand.ptr, n, whence)) < 0) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   {
      char val[20];
      sprintf(val, "%d", n);
      Tcl_AppendResult(interp, val, (char *)NULL);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclFitsBinTblRowRead_use =
  "USAGE: FitsBinTblRowRead <file-handle> <data-handle>";
#define tclFitsBinTblRowRead_hlp \
  "Read a row from a FITS binary table <file-handle> into object <data-handle>"

static ftclArgvInfo fitsBinTblRowRead_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFitsBinTblRowRead_hlp},
   {"<file-handle>", FTCL_ARGV_STRING, NULL, NULL, "handle to open file"},
   {"<data-handle>", FTCL_ARGV_STRING, NULL, NULL, "handle to data"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFitsBinTblRowRead(
		     ClientData clientDatag,
		     Tcl_Interp *interp,
		     int ac,
		     char **av
		     )
{
   int i;
   HANDLE data;				/* handle to data */
   HANDLE file;				/* handle to file */
   PHFITSFILE *fil;
   void *vptr;				/* used by shTclHandleExprEval */
   char *file_handleStr = NULL;		/* handle to open file */
   char *data_handleStr = NULL;		/* handle to data */
   
   shErrStackClear();

   i = 1;
   fitsBinTblRowRead_opts[i++].dst = &file_handleStr;
   fitsBinTblRowRead_opts[i++].dst = &data_handleStr;
   shAssert(fitsBinTblRowRead_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,fitsBinTblRowRead_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * Process arguments
 */
   if(shTclHandleExprEval(interp,file_handleStr,&file,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(file.type != shTypeGetFromName("PHFITSFILE")) {
      Tcl_SetResult(interp,"fitsBinRowRead: "
		    	      "first argument is not a PHFITSFILE",TCL_STATIC);
      return(TCL_ERROR);
   }
   fil = file.ptr;

   if(shTclHandleExprEval(interp,data_handleStr,&data,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * Check that the right data type is being read
 */
   if(fil->type != data.type) {
      Tcl_AppendResult(interp,"fitsBinRowRead: Attempt to read a ",
		       shNameGetFromType(data.type), " from a table of type ",
		       shNameGetFromType(fil->type),NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   if(phFitsBinTblRowRead(fil,data.ptr) != 0) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclFitsBinTblRowUnread_use =
  "USAGE: FitsBinTblRowUnread <file-handle> <data-handle>";
#define tclFitsBinTblRowUnread_hlp \
  "Unread a row from a FITS binary table <file-handle> into object <data-handle>"

static ftclArgvInfo fitsBinTblRowUnread_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFitsBinTblRowUnread_hlp},
   {"<file-handle>", FTCL_ARGV_STRING, NULL, NULL, "handle to open file"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFitsBinTblRowUnread(
		     ClientData clientDatag,
		     Tcl_Interp *interp,
		     int ac,
		     char **av
		     )
{
   int i;
   HANDLE file;				/* handle to file */
   void *vptr;				/* used by shTclHandleExprEval */
   char *file_handleStr = NULL;		/* handle to open file */
   
   shErrStackClear();

   i = 1;
   fitsBinTblRowUnread_opts[i++].dst = &file_handleStr;
   shAssert(fitsBinTblRowUnread_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,fitsBinTblRowUnread_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * Process arguments
 */
   if(shTclHandleExprEval(interp,file_handleStr,&file,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(file.type != shTypeGetFromName("PHFITSFILE")) {
      Tcl_SetResult(interp,"fitsBinRowUnread: "
		    	      "first argument is not a PHFITSFILE",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   if(phFitsBinTblRowUnread(file.ptr)!= 0) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclFitsBinTblRowWrite_use =
  "USAGE: FitsBinTblRowWrite <file-handle> <data-handle>";
#define tclFitsBinTblRowWrite_hlp \
  "Write a row of a FITS binary table <file-handle> from data in <data-handle>"

static ftclArgvInfo fitsBinTblRowWrite_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFitsBinTblRowWrite_hlp},
   {"<file-handle>", FTCL_ARGV_STRING, NULL, NULL, "handle to open file"},
   {"<data-handle>", FTCL_ARGV_STRING, NULL, NULL, "handle to data"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFitsBinTblRowWrite(
		     ClientData clientDatag,
		     Tcl_Interp *interp,
		     int ac,
		     char **av
		     )
{
   int i;
   HANDLE data;				/* handle to data */
   HANDLE file;				/* handle to file */
   PHFITSFILE *fil;
   static TYPE phfitsfile_type = UNKNOWN;
   void *vptr;				/* used by shTclHandleExprEval */
   char *file_handleStr = NULL;		/* handle to open file */
   char *data_handleStr = NULL;		/* handle to data */
   
   if(phfitsfile_type == UNKNOWN) {
      phfitsfile_type = shTypeGetFromName("PHFITSFILE");
   }
     
   shErrStackClear();

   i = 1;
   fitsBinTblRowWrite_opts[i++].dst = &file_handleStr;
   fitsBinTblRowWrite_opts[i++].dst = &data_handleStr;
   shAssert(fitsBinTblRowWrite_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,fitsBinTblRowWrite_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * Process arguments
 */
   if(shTclHandleExprEval(interp,file_handleStr,&file,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(file.type != shTypeGetFromName("PHFITSFILE")) {
      Tcl_SetResult(interp,"fitsBinRowWrite: "
		    	      "first argument is not a PHFITSFILE",TCL_STATIC);
      return(TCL_ERROR);
   }
   fil = file.ptr;

   if(shTclHandleExprEval(interp,data_handleStr,&data,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * Check that the right data type is being written
 */
   if(fil->type != data.type) {
      Tcl_AppendResult(interp,"fitsBinRowWrite: Attempt to write a ",
		       shNameGetFromType(data.type), " to a table of type ",
		       shNameGetFromType(fil->type),NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   if(phFitsBinTblRowWrite(fil, data.ptr)!= 0) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclFitsBinTblChainWrite_use =
  "USAGE: FitsBinTblChainWrite <file-handle> <chain-handle>";
#define tclFitsBinTblChainWrite_hlp \
  "Write a FITS binary table <file-handle> from data in <chain-handle>." \
"The table must already have been opened (with fitsBinTblOpen)"

static ftclArgvInfo fitsBinTblChainWrite_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFitsBinTblChainWrite_hlp},
   {"<file-handle>", FTCL_ARGV_STRING, NULL, NULL, "handle to open file"},
   {"<chain-handle>", FTCL_ARGV_STRING, NULL, NULL, "handle to data"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFitsBinTblChainWrite(
			ClientData clientDatag,
			Tcl_Interp *interp,
			int ac,
			char **av
			)
{
   int i;
   HANDLE chain;			/* handle to chain of data */
   HANDLE file;				/* handle to file */
   void *vptr;				/* used by shTclHandleExprEval */
   char *file_handleStr = NULL;		/* handle to open file */
   char *chain_handleStr = NULL;	/* handle to data */
   
   shErrStackClear();

   i = 1;
   fitsBinTblChainWrite_opts[i++].dst = &file_handleStr;
   fitsBinTblChainWrite_opts[i++].dst = &chain_handleStr;
   shAssert(fitsBinTblChainWrite_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,fitsBinTblChainWrite_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * Process arguments
 */
   if(shTclHandleExprEval(interp,file_handleStr,&file,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(file.type != shTypeGetFromName("PHFITSFILE")) {
      Tcl_SetResult(interp,"fitsBinChainWrite: "
		    	      "first argument is not a PHFITSFILE",TCL_STATIC);
      return(TCL_ERROR);
   }
   
   if(shTclHandleExprEval(interp,chain_handleStr,&chain,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(chain.type != shTypeGetFromName("CHAIN")) {
      Tcl_SetResult(interp,"fitsBinChainWrite: "
		    	      "second argument is not a CHAIN",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   if(phFitsBinTblChainWrite(file.ptr, chain.ptr) < 0) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}


/*****************************************************************************/
static char *tclFitsBinTblMachinePrint_use =
  "USAGE: FitsBinTblMachinePrint type -exec";
#define tclFitsBinTblMachinePrint_hlp \
  "Print a `machine' used to generate FITS binary tables for objects with\n"\
"the specified <type>. With the -exec flag, trace the operation of the\n"\
"machine. When file headers are written, the machines are optimised; if\n"\
"you provide a PHFITSFILE, its optimised machine is used. You must write\n"\
"a header with the -file file descriptor, e.g.\n"\
"\tset fd [fitsBinTblOpen foo.fts w]; fitsBinTblHdrWrite $fd OBJC\n"\
"\tfitsBinTblMachinePrint OBJC -file $fd -exec\n"

static ftclArgvInfo fitsBinTblMachinePrint_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFitsBinTblMachinePrint_hlp},
   {"<type>", FTCL_ARGV_STRING, NULL, NULL, "name of type to be printed"},
   {"-exec", FTCL_ARGV_CONSTANT, (void *)1, NULL, "if true, trace execution"},
   {"-file", FTCL_ARGV_STRING, NULL, NULL, "if present, use optimised machine for this file"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFitsBinTblMachinePrint(
			  ClientData clientDatag,
			  Tcl_Interp *interp,
			  int ac,
			  char **av
			  )
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *typeStr = NULL;		/* name of type to be printed */
   int exec = 0;			/* if true, trace execution */
   char *fileStr = NULL;		/* if present, use optimised machine
					   for this file */

   shErrStackClear();

   i = 1;
   fitsBinTblMachinePrint_opts[i++].dst = &typeStr;
   fitsBinTblMachinePrint_opts[i++].dst = &exec;
   fitsBinTblMachinePrint_opts[i++].dst = &fileStr;
   shAssert(fitsBinTblMachinePrint_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,fitsBinTblMachinePrint_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * process arguments
 */
   if(fileStr == NULL) {
      hand.ptr = NULL;
      if(exec) {
	 Tcl_AppendResult(interp, "You must specify -file with -exec",
								 (char *)NULL);
	 return(TCL_ERROR);
      }
   } else {
      if(shTclHandleExprEval(interp,fileStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("PHFITSFILE")) {
	 Tcl_SetResult(interp,"fitsBinTblMachinePrint: "
		       "optional argument is not a PHFITSFILE",TCL_STATIC);
	 return(TCL_ERROR);
      }
   }
/*
 * do the work
 */
   if(phFitsBinTblMachineTrace(typeStr, exec, hand.ptr) < 0) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   
   return(TCL_OK);
}

static char *tclFitsBinSchemaTransIsDefined_use =
  "USAGE: fitsBinSchemaTransIsDefined <type>";
#define tclFitsBinSchemaTransIsDefined_hlp \
  ""

static ftclArgvInfo fitsBinSchemaTransIsDefined_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFitsBinSchemaTransIsDefined_hlp},
   {"<type>", FTCL_ARGV_STRING, NULL, NULL, "Type to query for"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define fitsBinSchemaTransIsDefined_name "fitsBinSchemaTransIsDefined"

static int
tclFitsBinSchemaTransIsDefined(ClientData clientData,
			       Tcl_Interp *interp,
			       int ac,
			       char **av)
{
   char buf[30];
   int a_i;
   char *typeStr = NULL;		/* Type to query for */

   shErrStackClear();

   a_i = 1;
   fitsBinSchemaTransIsDefined_opts[a_i++].dst = &typeStr;
   shAssert(fitsBinSchemaTransIsDefined_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, fitsBinSchemaTransIsDefined_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     fitsBinSchemaTransIsDefined_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   {
       const int isDefined = phFitsBinSchemaTransIsDefined(typeStr);
       sprintf(buf,"%d", isDefined);
   }
/*
 * and return the answer
 */
   Tcl_SetResult(interp,buf,TCL_VOLATILE);

   return(TCL_OK);
}



/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phPhotoFitsBinDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"fitsBinSchemaTransEntryAdd",
		(Tcl_CmdProc *)tclFitsBinSchemaTransEntryAdd, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFitsBinSchemaTransEntryAdd_hlp,
		tclFitsBinSchemaTransEntryAdd_use);
   
   shTclDeclare(interp,"fitsBinDeclareSchemaTrans",
		(Tcl_CmdProc *)tclFitsBinDeclareSchemaTrans, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFitsBinDeclareSchemaTrans_hlp,
		tclFitsBinDeclareSchemaTrans_use);

   shTclDeclare(interp,"fitsBinForgetSchemaTrans",
		(Tcl_CmdProc *)tclFitsBinForgetSchemaTrans, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFitsBinForgetSchemaTrans_hlp,
		tclFitsBinForgetSchemaTrans_use);

   shTclDeclare(interp,"fitsBinTblOpen",
		(Tcl_CmdProc *)tclFitsBinTblOpen, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFitsBinTblOpen_hlp,
		tclFitsBinTblOpen_use);

   shTclDeclare(interp,"fitsBinTblClose",
		(Tcl_CmdProc *)tclFitsBinTblClose, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFitsBinTblClose_hlp,
		tclFitsBinTblClose_use);

   shTclDeclare(interp,"fitsBinTblEnd",
		(Tcl_CmdProc *)tclFitsBinTblEnd, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFitsBinTblEnd_hlp,
		tclFitsBinTblEnd_use);

   shTclDeclare(interp,"fitsBinTblHdrRead",
		(Tcl_CmdProc *)tclFitsBinTblHdrRead, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFitsBinTblHdrRead_hlp,
		tclFitsBinTblHdrRead_use);

   shTclDeclare(interp,"fitsBinTblHdrWrite",
		(Tcl_CmdProc *)tclFitsBinTblHdrWrite, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFitsBinTblHdrWrite_hlp,
		tclFitsBinTblHdrWrite_use);

   shTclDeclare(interp,"fitsBinTblRowRead",
		(Tcl_CmdProc *)tclFitsBinTblRowRead, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFitsBinTblRowRead_hlp,
		tclFitsBinTblRowRead_use);

   shTclDeclare(interp,"fitsBinTblRowUnread",
		(Tcl_CmdProc *)tclFitsBinTblRowUnread, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFitsBinTblRowUnread_hlp,
		tclFitsBinTblRowUnread_use);

   shTclDeclare(interp,"fitsBinTblRowWrite",
		(Tcl_CmdProc *)tclFitsBinTblRowWrite, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFitsBinTblRowWrite_hlp,
		tclFitsBinTblRowWrite_use);

   shTclDeclare(interp,"fitsBinTblMachinePrint",
		(Tcl_CmdProc *)tclFitsBinTblMachinePrint, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFitsBinTblMachinePrint_hlp,
		tclFitsBinTblMachinePrint_use);

   shTclDeclare(interp,"fitsBinTblChainWrite",
		(Tcl_CmdProc *)tclFitsBinTblChainWrite, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFitsBinTblChainWrite_hlp,
		tclFitsBinTblChainWrite_use);

   shTclDeclare(interp,"fitsBinTblRowSeek",
		(Tcl_CmdProc *)tclFitsBinTblRowSeek, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFitsBinTblRowSeek_hlp,
		tclFitsBinTblRowSeek_use);

   shTclDeclare(interp,fitsBinSchemaTransIsDefined_name,
		(Tcl_CmdProc *)tclFitsBinSchemaTransIsDefined, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFitsBinSchemaTransIsDefined_hlp,
		tclFitsBinSchemaTransIsDefined_use);
}
