/*****************************************************************************
******************************************************************************
**
** FILE:
**	tclFitsIo.c
**
** ABSTRACT:
**	This file contains routines that support the definition of TCL verbs
**      that read/write FITS format files.
**
** ENTRY POINT		SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
** shTclDirGet          public  Gets the default directory
** shTclDirList         public  List the default directory
** shTclDirSet          public  Sets the default directory
** shTclRegReadAsFits   public  Reads a FITS file into a region
** shTclRegWriteAsFits  public  Writes a region in a FITS format file
** shTclHdrReadAsFits   public  Reads a FITS header into a HDR
** shTclHdrWriteAsFits  public  Writes a HDR in a FITS format file
**
** ENVIRONMENT:
**	ANSI C.
**
** REQUIRED PRODUCTS:
**	ftcl
**	libfits
**
** AUTHORS:
**	Creation date:  May 7, 1993
**
******************************************************************************
******************************************************************************
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ftcl.h"
#include "tclExtdInt.h"
#include "libfits.h"
#include "shCEnvscan.h"
#include "shTclParseArgv.h"
#include "shTclRegSupport.h"
#include "shTclUtils.h"
#include "shCHdr.h"
#include "shTclHdr.h"
#include "shCFitsIo.h"
#include "region.h"
#include "shCErrStack.h"
#include "shTclErrStack.h"
#include "shCSao.h"
#include "shTclVerbs.h"
#include "shTclFits.h"
#include "shCUtils.h"		/* For SHKEYENT, shKeyLocate() */
#include "shTclHandle.h"

/*============================================================================  
**============================================================================
**
** LOCAL MACROS, DEFINITIONS, ETC.
**
**============================================================================
*/

/*----------------------------------------------------------------------------
**
** LOCAL DEFINITIONS
*/
static char *TCLUSAGE_DIRLIST   = "USAGE: dirList\n";
static char *TCLUSAGE_REGTYPE =
               "REGION_FILE - regReadAsFits and regWriteAsFits\n";
static char *TCLUSAGE_POOLTYPE = "POOL_GROUP  - regReadPool\n";
static char *TCLUSAGE_PLOTTYPE = "PLOT_FILE   - pgGet and pgSave\n";
static char *TCLUSAGE_MASKTYPE = 
               "MASK_FILE   - maskReadAsFits and maskWriteAsFits";


/*----------------------------------------------------------------------------
**
** GLOBAL VARIABLES
**
** The types for the file specification defaults.  This   M U S T   match the
** quantity and order of DEFDIRENUM (in shCFitsIo.h).  Only DEF_NUM_OF_ELEMS
** should not be in this table.  NONE can only be at the end of the table.
*/

SHKEYENT TCLDIRTYPES[] ={{ "REGION_FILE", DEF_REGION_FILE },
			 { "POOL_GROUP",  DEF_POOL_GROUP  },
			 { "PLOT_FILE",   DEF_PLOT_FILE   },
			 { "MASK_FILE",   DEF_MASK_FILE   },
			 { "NONE",        DEF_NONE        },
			 { ((char *)0),   0               }}; /* End list */


/*============================================================================
**============================================================================
**
** ROUTINE: p_shPipeValidate
**
** DESCRIPTION:
**	This routine will validate the pipe name passed to it and return
**      the number associated with this pipe.  Extended tcl will return to the
**      user (who executes the 'pipe' tcl command) 2 strings, "filennn" and
**      "filemmm" where the nnn and the mmm are the values returned from the
**      opening of the two half-duplex pipes.  Therefore we need to pick these
**      numbers out of the string that we were given.
**	
** RETURN VALUES:
**      TCL_OK
**      TCL_ERROR
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
RET_CODE  p_shPipeValidate
   (
    Tcl_Interp	*a_interp,	/* IN: */
    char        *a_pipe,        /* IN: */
    int         *a_pipeNum       /* OUT: */
   )
{
RET_CODE rstatus;
char tclFile[] = "file";
int  len = strlen(tclFile);

if (strncmp(a_pipe, tclFile, len) == 0)
   {
   /* The string in pipe did begin with the value in tclFile */
   Tcl_StrToInt(&a_pipe[len], 10, a_pipeNum);
   rstatus = TCL_OK;
   }
else
   {
   /* The string in pipe did not begin with this.  It is an invalid
      pipe */
   Tcl_AppendResult(a_interp, "Invalid pipe name - ", a_pipe,
		    ", must be of form ", tclFile,
		    "nnn, where nnn is a number\n", (char *)NULL);
   rstatus = TCL_ERROR;
   }

return (rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: shTclDirGet
**
** DESCRIPTION:
**	This routine is called in response to the tcl verb 'dirGet'.  It
**	checks the syntax of the entered command and then calls
**	shDirGet.
**	
** RETURN VALUES:
**      TCL_OK
**      TCL_ERROR
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static char *g_dirGet = "dirGet";
static ftclArgvInfo g_dirGetArgTable[] = {
     {NULL, FTCL_ARGV_HELP, NULL, NULL,
      "Get the default directory\n" },
     {"<dirType>",    FTCL_ARGV_STRING, NULL, NULL, NULL},
     {"-dir",    FTCL_ARGV_STRING, NULL, NULL, NULL},
     {"-ext",    FTCL_ARGV_STRING, NULL, NULL, NULL},
     {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
   };

static int shTclDirGet
   (
   ClientData	a_clientData,	/* IN: */
   Tcl_Interp	*a_interp,	/* IN: */
   int		argc,		/* IN: */
   char		**argv		/* IN: */
   )   
{
int  rstatus;
DEFDIRENUM        dirType;
unsigned int uInt;	/* For shKeyLocate() */
char l_dir[MAXDDIRLEN], *dirValPtr = NULL, *dirTypeValPtr = NULL;
char l_ext[MAXDEXTLEN], *extValPtr = NULL;
int  dirExtFlg;         	/* Directory/extension flag (1=dir, 0=ext) */
int  flags=FTCL_ARGV_NO_LEFTOVERS;
 
l_dir[0] = (char)0;
l_ext[0] = (char)0;
 
/* Parse the command */

g_dirGetArgTable[1].dst = &dirTypeValPtr;
g_dirGetArgTable[2].dst = &dirValPtr;
g_dirGetArgTable[3].dst = &extValPtr;

rstatus = shTclParseArgv(a_interp, &argc, argv, g_dirGetArgTable, 
                         flags, g_dirGet);
if (rstatus ==  FTCL_ARGV_SUCCESS)
   {
   rstatus = TCL_OK;

   /* Check the options */
   if (extValPtr!=NULL)
      {dirExtFlg = 0;}			/* Get Extension */
   else 
      {dirExtFlg = 1;}			/* Get directory - the default */

   /* Call the C routine depending on the value of the first parameter */
   if (shKeyLocate(dirTypeValPtr, TCLDIRTYPES, &uInt, SH_CASE_INSENSITIVE) != SH_SUCCESS)
      {
      rstatus = TCL_ERROR;		/* Syntax error */
      goto syntax_err;
      }
   if ((dirType = ((DEFDIRENUM)uInt)) == DEF_NONE) /* Satisfy type checking */
      {
      rstatus = TCL_ERROR;
      goto syntax_err;
      }
   shDirGet(dirType, l_dir, l_ext);

   if (dirExtFlg)
      {Tcl_SetResult(a_interp, l_dir, TCL_VOLATILE);}
   else
      {Tcl_SetResult(a_interp, l_ext, TCL_VOLATILE);}
   }
return(rstatus);

/* If we get to here then there was a syntax error */
syntax_err:
Tcl_AppendResult(a_interp, shTclGetUsage(a_interp, g_dirGetArgTable, 
                 flags, g_dirGet), (char *)NULL);
Tcl_AppendResult(a_interp, "\n   Where :\n      ", TCLUSAGE_REGTYPE,
                 "      ", TCLUSAGE_POOLTYPE,
                 "      ", TCLUSAGE_PLOTTYPE,  
                 "      ", TCLUSAGE_MASKTYPE, (char *)NULL);

return(rstatus);
 
}

/*============================================================================
**============================================================================
**
** ROUTINE: shTclDirList
**
** DESCRIPTION:
**	This routine is called in response to the tcl verb 'dirList'.
**	
**
** RETURN VALUES:
**      TCL_OK
**      TCL_ERROR
**
** CALLS TO:
**	shDirList
**	Tcl_AppendResult
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static int shTclDirList
   (
   ClientData	a_clientData,	/* IN: */
   Tcl_Interp	*a_interp,	/* IN: */
   int		argc,		/* IN: */
   char		**argv		/* IN: */
   )   
{
char l_dir[MAXDDIRLEN];
char l_ext[MAXDEXTLEN];

shDirGet(DEF_REGION_FILE, l_dir, l_ext);
if (l_dir[0] || l_ext[0])
   {Tcl_AppendResult(a_interp, "\n", TCLUSAGE_REGTYPE, "\n", (char *)NULL);}
if (l_dir[0])
   {
   Tcl_AppendResult(a_interp, "   Default directory      = ",
                    l_dir, "\n", (char *)NULL);
   }

if (l_ext[0])
   {
   Tcl_AppendResult(a_interp, "   Default file extension = ",
                    l_ext, "\n", (char *)NULL);
   }
 
shDirGet(DEF_POOL_GROUP,  l_dir, l_ext);
if (l_dir[0] || l_ext[0])
   {Tcl_AppendResult(a_interp, "\n", TCLUSAGE_POOLTYPE, "\n", (char *)NULL);}
if (l_dir[0])
   {
   Tcl_AppendResult(a_interp, "   Default directory      = ",
                    l_dir, "\n", (char *)NULL);
   }
if (l_ext[0])
   {
   Tcl_AppendResult(a_interp, "   Default file extension = ",
                    l_ext, "\n", (char *)NULL);
   }

shDirGet(DEF_PLOT_FILE,  l_dir, l_ext);
if (l_dir[0] || l_ext[0])
   {Tcl_AppendResult(a_interp, "\n", TCLUSAGE_PLOTTYPE, "\n", (char *)NULL);}
if (l_dir[0])
   {
   Tcl_AppendResult(a_interp, "   Default directory      = ",
                    l_dir, "\n", (char *)NULL);
   }
if (l_ext[0])
   {
   Tcl_AppendResult(a_interp, "   Default file extension = ",
                    l_ext, "\n", (char *)NULL);
   }
 
shDirGet(DEF_MASK_FILE,  l_dir, l_ext);
if (l_dir[0] || l_ext[0])
   {Tcl_AppendResult(a_interp, "\n", TCLUSAGE_MASKTYPE, "\n", (char *)NULL);}
if (l_dir[0])
   {
   Tcl_AppendResult(a_interp, "   Default directory      = ",
                    l_dir, "\n", (char *)NULL);
   }
if (l_ext[0])
   {
   Tcl_AppendResult(a_interp, "   Default file extension = ",
                    l_ext, "\n", (char *)NULL);
   }

return(TCL_OK);
}

/*============================================================================
**============================================================================
**
** ROUTINE: shTclDirSet
**
** DESCRIPTION:
**	This routine is called in response to the tcl verb 'dirSet'.  It
**	checks the syntax of the entered command and then calls
**	shDirSet.
**	
**
** RETURN VALUES:
**      TCL_OK
**      TCL_ERROR
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static char *g_dirSet = "dirSet";
static ftclArgvInfo g_dirSetArgTable[] = {
     {NULL, FTCL_ARGV_HELP, NULL, NULL,
      "Set the default directory\n" },
     {"<dirType>",    FTCL_ARGV_STRING, NULL, NULL, NULL},
     {"-dir",    FTCL_ARGV_STRING, NULL, NULL, NULL},
     {"-ext",    FTCL_ARGV_STRING, NULL, NULL, NULL},
     {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
   };

static int shTclDirSet
   (
   ClientData	a_clientData,	/* IN: */
   Tcl_Interp	*a_interp,	/* IN: */
   int		argc,		/* IN: */
   char		**argv		/* IN: */
   )   
{
int      rstatus;
DEFDIRENUM        dirType;
unsigned int uInt;	/* For shKeyLocate() */
char     dirPtr[MAXDDIRLEN], *dirValPtr = NULL, *dirTypeValPtr = NULL;
char     extPtr[MAXDEXTLEN], *extValPtr = NULL;
char     *l_dir;
RET_CODE dirstat;
int  flags=FTCL_ARGV_NO_LEFTOVERS;
 
/* Parse the command */
  g_dirSetArgTable[1].dst = &dirTypeValPtr;
  g_dirSetArgTable[2].dst = &dirValPtr;
  g_dirSetArgTable[3].dst = &extValPtr;

  rstatus = shTclParseArgv(a_interp, &argc, argv, g_dirSetArgTable, 
                           FTCL_ARGV_NO_LEFTOVERS, g_dirSet);
  if (rstatus == FTCL_ARGV_SUCCESS)
   {
   rstatus = TCL_OK;

   /* Check the options */
   if (extValPtr!=NULL)
      {strcpy(extPtr, extValPtr);}	/* default extension */
   else 
      {extPtr[0] = 0;}

   if (dirValPtr!=NULL)
      {strcpy(dirPtr, dirValPtr);}	/* default directory */
   else 
      {dirPtr[0] = 0;}

   /* Translate ~ signs and env variables */
   if(dirPtr[0] != 0)
      {l_dir = shEnvscan(dirPtr);}
   else
      {l_dir = dirPtr;}

   /* There should be something there */
   if (l_dir != NULL)
      {
      if (shKeyLocate(dirTypeValPtr, TCLDIRTYPES, &uInt, SH_CASE_INSENSITIVE) != SH_SUCCESS)
         {
         if (l_dir != dirPtr)
            {shEnvfree(l_dir);}
         rstatus = TCL_ERROR;		/* Syntax error */
         goto syntax_err;
         }
      if ((dirType = ((DEFDIRENUM)uInt)) == DEF_NONE) /* Satisfy type check */
         {
         if (l_dir != dirPtr)
            {shEnvfree(l_dir);}
         rstatus = TCL_ERROR;
         goto syntax_err;
         }
      dirstat = (dirType != DEF_POOL_GROUP)
              ? shDirSet(dirType, l_dir, extPtr, 1)
              : shDirSet(dirType, l_dir, extPtr, 0);

      /* Check for errors */
      if (dirstat == SH_DIR_TOO_LONG)
         {
         Tcl_AppendResult(a_interp, "Directory path too long ", (char *)NULL);
         rstatus = TCL_ERROR;
         }
      else if (dirstat == SH_EXT_TOO_LONG)
         {
         Tcl_AppendResult(a_interp, "File extension too long ", (char *)NULL);
         rstatus = TCL_ERROR;
         }
      if (l_dir != dirPtr)
         {shEnvfree(l_dir);}
      }
   else
      {
      rstatus = TCL_ERROR;
      Tcl_AppendResult(a_interp,
                       "Error in directory name : ", dirPtr, (char *)NULL);
      }
   }
return(rstatus);

/* If we get to here then there was a syntax error */
syntax_err:
Tcl_AppendResult(a_interp, shTclGetUsage(a_interp, g_dirSetArgTable, 
                 flags, g_dirSet), (char *)NULL);
Tcl_AppendResult(a_interp, "\n   Where :\n      ", TCLUSAGE_REGTYPE,
                 "      ", TCLUSAGE_POOLTYPE, 
                 "      ", TCLUSAGE_PLOTTYPE, 
                 "      ", TCLUSAGE_MASKTYPE, (char *)NULL);
return(rstatus);
}


/*============================================================================
**============================================================================
**
** ROUTINE: shTclRegReadAsFits
**
** DESCRIPTION:
**      Read a FITS file into a region.  The specified region is always
**      destroyed and a new region obtained.
**	
**
** RETURN VALUES:
**      TCL_OK
**      TCL_ERROR
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static char *g_regReadAsFits = "regReadAsFits";
static ftclArgvInfo g_regReadAsFitsArgTable[] = {
    {NULL, FTCL_ARGV_HELP, NULL, NULL,
     "Read a FITS file into a region\n" },
    {"<region>",  FTCL_ARGV_STRING,NULL, NULL, "Region to read in to"},
    {"<FITSfile>",FTCL_ARGV_STRING,NULL, NULL,
       "File containing the data or pipe descriptor if -pipe is present"},
    {"-dirset", FTCL_ARGV_STRING,NULL, NULL,
       "Directory where file resides if not current one"},
    {"-checktype", FTCL_ARGV_CONSTANT,(void*) 1, NULL,
       "Make sure type of region and FITS file are equivalent"},
    {"-keeptype", FTCL_ARGV_CONSTANT,(void*) 1, NULL, ""},
    {"-pipe", FTCL_ARGV_CONSTANT, (void *)1, NULL,
       "If present, FITSfile is actually a pipe descriptor"},
    {"-tape", FTCL_ARGV_CONSTANT, (void *)1, NULL,
       "Read from tape device specified in <FITSFILE>"},
    {"-naxis3", FTCL_ARGV_CONSTANT, (void *)1, NULL,
       "Don't complain if NAXIS3 > 1"},
    {"-hdu", FTCL_ARGV_INT, NULL, NULL,
       "Read from HDU <hdu> (0: PDU)"},
    {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
   };

static int shTclRegReadAsFits
   (
   ClientData	a_clientData,	/* IN: */
   Tcl_Interp	*a_interp,	/* IN: */
   int		argc,		/* IN: */
   char		**argv		/* IN: */
   )   
{
REGION   *regPtr;
RET_CODE rstatus;
char     *regNamePtr=NULL;	        /* first parameter */
char     *file;				/* second parameter */
int      checktype = 0, keeptype = 0;	/* option */
DEFDIRENUM FITSdef;			/* dirSet default to use */
int      inPipe = 0;
char*    dirsetValPtr = NULL;
int      flags = FTCL_ARGV_NO_LEFTOVERS;
OpenFile *inFilePtr;
FILE     *hfile = NULL;
int      inTape = 0;                    /* tape read option */
int	 naxis3_is_ok = 0;		/* NAXIS3 > 1 is silently allowed */
int      hdu = 0;			/* which HDU to read */

inFilePtr = NULL;

/* Parse the command */
g_regReadAsFitsArgTable[1].dst = &regNamePtr;
g_regReadAsFitsArgTable[2].dst = &file;
g_regReadAsFitsArgTable[3].dst = &dirsetValPtr;
g_regReadAsFitsArgTable[4].dst = &checktype;
g_regReadAsFitsArgTable[5].dst = &keeptype;
g_regReadAsFitsArgTable[6].dst = &inPipe;
g_regReadAsFitsArgTable[7].dst = &inTape;
g_regReadAsFitsArgTable[8].dst = &naxis3_is_ok;
g_regReadAsFitsArgTable[9].dst = &hdu;

rstatus = shTclParseArgv(a_interp, &argc, argv, g_regReadAsFitsArgTable,
                         flags, g_regReadAsFits);
if (rstatus == FTCL_ARGV_SUCCESS)
   {
   /* Can't have both -tape and -pipe */
   if (inPipe && inTape)
      {
      Tcl_AppendResult(a_interp, "\n  Illegal to specify both -tape and -pipe ",
		       (char *)NULL);
      rstatus = TCL_ERROR;
      goto rtn_return;
      }
   /* We do not need to get the file specification if we are using a pipe or tape */
   if (!inPipe && !inTape)
      {
	/* Get the dirset file specification defaulting info */
	p_shTclDIRSETexpand(dirsetValPtr,a_interp, DEF_REGION_FILE, &FITSdef,
			    ((char *)0), ((char **)0), ((char **)0));
      }
   
   /* Check if this is valid region by trying to get the pointer to it */
   if ((rstatus = shTclRegAddrGetFromName(a_interp, regNamePtr,
                                          &regPtr)) == TCL_OK)
      {
      /* Validate the pipe we were given (if we were given one) */
      if (inPipe)
	 {
	 if ((inFilePtr = Tcl_GetOpenFileStruct(a_interp, file)) == NULL)
	    {goto rtn_return;}
	 hfile = inFilePtr->f;      /* Get ptr to pipes 'file' descriptor */
	 }

      /* Now read the FITS file into the region */
      if ((rstatus =
	   shRegReadAsFits(regPtr, file, checktype, keeptype,
                           FITSdef, hfile, inTape, naxis3_is_ok, hdu)) != SH_SUCCESS)
         {
         Tcl_AppendResult(a_interp, "\n  Attempting to read into region : ",
                          regNamePtr, "\n", (char *)NULL);
         rstatus = TCL_ERROR;
         }
      else
         { 
	 /* Return with region name if successful */
	 Tcl_SetResult(a_interp, regNamePtr, TCL_VOLATILE);
	 rstatus = TCL_OK;				/* Success */
	 }
      }
   }

rtn_return:
if (rstatus != TCL_OK) 
  {
  shTclInterpAppendWithErrStack(a_interp); /* Append errors from stack if any */
  }
return (rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: shTclRegWriteAsFits
**
** DESCRIPTION:
**	This routine is called in response to the tcl verb 'regWriteAsFits'.
**	It writes the specified region out to a FITS file.
**	
**
** RETURN VALUES:
**      TCL_OK
**      TCL_ERROR
**
** GLOBALS REFERENCED:
**	g_myglobal
**
**============================================================================
*/
static char *g_regWriteAsFits = "regWriteAsFits";
static ftclArgvInfo  g_regWriteAsFitsArgTable[] = {
      {NULL, FTCL_ARGV_HELP, NULL, NULL,
       "Write a region to a FITS file\n" },
      {"<region>",    FTCL_ARGV_STRING, NULL, NULL, NULL},
      {"<FITSfile>", FTCL_ARGV_STRING, NULL, NULL,
       "File to contain the header or pipe descriptor if -pipe is present"},
      {"-dirset",    FTCL_ARGV_STRING, NULL, NULL, NULL},
      {"-standard",  FTCL_ARGV_STRING, NULL, NULL, NULL},
      {"-naxis",     FTCL_ARGV_INT, NULL, NULL, NULL},
      {"-pipe",      FTCL_ARGV_CONSTANT, (void *)1, NULL,
       "If present, FITSfile is actually a pipe descriptor"},
      {"-tape",      FTCL_ARGV_CONSTANT, (void *)1, NULL,
       "Write to tape device specified in <FITSFILE>"},
      {"-image",     FTCL_ARGV_CONSTANT, (void *)1, NULL,
       "Append data as an IMAGE extension"},
      {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
    };

static int shTclRegWriteAsFits
   (
   ClientData	a_clientData,	/* IN: */
   Tcl_Interp	*a_interp,	/* IN: */
   int		argc,		/* IN: */
   char		**argv		/* IN: */
   )   
{
REGION       *regPtr;
RET_CODE     rstatus;
FITSFILETYPE fitsType;
char         *regNamePtr = NULL;
char         *l_filePtr;
char         *c_fitsType = "T";
int          naxis = 2;
DEFDIRENUM   FITSdef;			/* dirSet default to use */
char*        dirsetValPtr = NULL;
int          outPipe = 0;
int          flags = FTCL_ARGV_NO_LEFTOVERS;
OpenFile     *outFilePtr;
FILE         *hfile = NULL;
int          outTape = 0;               /* option for writing to tape */
int          imageXtension = 0;		/* option for writing IMAGE xtension */

outFilePtr = NULL;

/* Parse the command */
g_regWriteAsFitsArgTable[1].dst = &regNamePtr;
g_regWriteAsFitsArgTable[2].dst = &l_filePtr;
g_regWriteAsFitsArgTable[3].dst = &dirsetValPtr;
g_regWriteAsFitsArgTable[4].dst = &c_fitsType;
g_regWriteAsFitsArgTable[5].dst = &naxis;
g_regWriteAsFitsArgTable[6].dst = &outPipe;
g_regWriteAsFitsArgTable[7].dst = &outTape;
g_regWriteAsFitsArgTable[8].dst = &imageXtension;

rstatus = shTclParseArgv(a_interp, &argc, argv, g_regWriteAsFitsArgTable,
                         flags, g_regWriteAsFits);
if (rstatus == FTCL_ARGV_SUCCESS)
   {
   /* Get the 'standard' option - if none, the default will be returned */
   if(imageXtension) {
      fitsType = IMAGE;
   } else {
      if ((c_fitsType[0] == 'F') || (c_fitsType[0] == 'f'))
	{fitsType = NONSTANDARD;}
      else
	{fitsType = STANDARD;}
   }

   /* Can't have both -tape and -pipe */
   if (outPipe && outTape)
      {
      Tcl_AppendResult(a_interp, "\n  Illegal to specify both -tape and -pipe ",
		       (char *)NULL);
      rstatus = TCL_ERROR;
      goto rtn_return;
      }

   /* We do not need to get the file specification if we are using a pipe or tape */
   if (!outPipe && !outTape)
      {
	/* Get the dirset file specification defaulting info */
	p_shTclDIRSETexpand(dirsetValPtr,a_interp, DEF_REGION_FILE, &FITSdef,
			    ((char *)0), ((char **)0), ((char **)0));
      }

   /* Check if this is valid region by trying to get the pointer to it */
   if ((rstatus = shTclRegAddrGetFromName(a_interp, regNamePtr,
                                          &regPtr)) == TCL_OK)
      {
      /* Yes the region is legal */
      
      /* The default for naxis is 2.  However, if the region contains no data,
	 (nrow * ncol = 0), then the default for naxis should be 0. */
      if ((regPtr->nrow * regPtr->ncol) == 0)
	 {naxis = 0;}

      /* Validate the pipe we were given (if we were given one) */
      if (outPipe)
	 {
	 if ((outFilePtr = Tcl_GetOpenFileStruct(a_interp, l_filePtr)) == NULL)
	    {goto rtn_return;}
	 hfile = outFilePtr->f;      /* Get ptr to pipes 'file' descriptor */
	 }

      /* Write out the FITS header and the pixels */
      if ((rstatus = shRegWriteAsFits(regPtr, l_filePtr, fitsType, naxis,
                     FITSdef, hfile, outTape)) != SH_SUCCESS)
         {
         Tcl_AppendResult(a_interp, "\n  Attempting to write region : ",
                          regNamePtr, "\n", (char *)NULL);
         rstatus = TCL_ERROR;
         }
      else
         { 
         /* Return with region name if successful */
         Tcl_SetResult(a_interp, regNamePtr, TCL_VOLATILE);
         rstatus = TCL_OK;				/* Success */
         }
      }
   }

rtn_return:
if (rstatus != TCL_OK) 
  {
  shTclInterpAppendWithErrStack(a_interp); /* Append errors from stack if any */
  }
return (rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: shTclHdrReadAsFits
**
** DESCRIPTION:
**      Read a FITS file header into a HDR.  The specified header lines are
**      always destroyed.
**
** RETURN VALUES:
**      TCL_OK
**      TCL_ERROR
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static char *g_hdrReadAsFits = "hdrReadAsFits";
static ftclArgvInfo g_hdrReadAsFitsArgTable[] = {
    {NULL, FTCL_ARGV_HELP, NULL, NULL,
     "Read a FITS file header into a HDR\n" },
    {"<header>",  FTCL_ARGV_STRING,NULL, NULL, NULL},
    {"<FITSfile>",FTCL_ARGV_STRING,NULL, NULL,
       "File to contain the header or pipe descriptor if -pipe is present"},
    {"-dirset",FTCL_ARGV_STRING,NULL, NULL, NULL},
    {"-pipe", FTCL_ARGV_CONSTANT, (void *)1, NULL,
       "If present, FITSfile is actually a pipe descriptor"},
    {"-tape", FTCL_ARGV_CONSTANT, (void *)1, NULL,
       "Read from tape device specified in <FITSFILE>"},
    {"-hdu", FTCL_ARGV_INT, NULL, NULL,
       "Read from HDU <hdu> (0: PDU)"},
    {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
   };

static int shTclHdrReadAsFits
   (
   ClientData	a_clientData,	/* IN: */
   Tcl_Interp	*a_interp,	/* IN: */
   int		argc,		/* IN: */
   char		**argv		/* IN: */
   )   
{
HDR      *hdrPtr;
RET_CODE rstatus;
char     *hdrNamePtr=NULL;       	/* first parameter */
char     *file;				/* second parameter */
DEFDIRENUM FITSdef;			/* dirSet default to use */
int      flags = FTCL_ARGV_NO_LEFTOVERS;
char*    dirsetValPtr = NULL;
int      inPipe = 0;
OpenFile *inFilePtr;
FILE     *hfile = NULL;
int      inTape = 0;                    /* option to read from tape */
int      hdu = 0;			/* HDU to read */

inFilePtr = NULL;
 
/* Parse the command */

g_hdrReadAsFitsArgTable[1].dst = &hdrNamePtr;
g_hdrReadAsFitsArgTable[2].dst = &file;
g_hdrReadAsFitsArgTable[3].dst = &dirsetValPtr;
g_hdrReadAsFitsArgTable[4].dst = &inPipe;
g_hdrReadAsFitsArgTable[5].dst = &inTape;
g_hdrReadAsFitsArgTable[6].dst = &hdu;

rstatus = shTclParseArgv(a_interp, &argc, argv, g_hdrReadAsFitsArgTable,
                         flags, g_hdrReadAsFits);
if (rstatus == FTCL_ARGV_SUCCESS)
   {
   /* Can't have both -tape and -pipe */
   if (inPipe && inTape)
      {
      Tcl_AppendResult(a_interp, "\n  Illegal to specify both -tape and -pipe ",
		       (char *)NULL);
      rstatus = TCL_ERROR;
      goto rtn_return;
      }
   /* We do not need to get the file specification if we are using a pipe or tape */
   if (!inPipe && !inTape)
      {
      /* Get the dirset file specification defaulting info */
      p_shTclDIRSETexpand(dirsetValPtr, a_interp, DEF_REGION_FILE, &FITSdef,
			  ((char *)0), ((char **)0), ((char **)0));
      }
   
   /* Check if this is valid header by trying to get the pointer to it */
   if ((rstatus = shTclHdrAddrGetFromExpr(a_interp, hdrNamePtr,
                                          &hdrPtr)) == TCL_OK)
      {
      /* Validate the pipe we were given (if we were given one) */
      if (inPipe)
	 {
	 if ((inFilePtr = Tcl_GetOpenFileStruct(a_interp, file)) == NULL)
	    {goto rtn_return;}
	 hfile = inFilePtr->f;      /* Get ptr to pipes 'file' descriptor */
	 }

      /* Now read the FITS file into the header */
      if ((rstatus = shHdrReadAsFits(hdrPtr, file, FITSdef, hfile, inTape, hdu))
	  != SH_SUCCESS)
         {
         Tcl_AppendResult(a_interp, "\n  Attempting to read into header : ",
                          hdrNamePtr, "\n", (char *)NULL);
         rstatus = TCL_ERROR;
         }
      else
         { 
	 /* Return with header name if successful */
	 Tcl_SetResult(a_interp, hdrNamePtr, TCL_VOLATILE);
	 rstatus = TCL_OK;				/* Success */
	 }
      }
   }

rtn_return:

if (rstatus != TCL_OK) 
  {
  shTclInterpAppendWithErrStack(a_interp); /* Append errors from stack if any */
  }
return (rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: shTclHdrWriteAsFits
**
** DESCRIPTION:
**	This routine is called in response to the tcl verb 'hdrWriteAsFits'.
**	It writes the specified header out to a FITS file.
**	
**
** RETURN VALUES:
**      TCL_OK
**      TCL_ERROR
**
** CALLS TO:
**
** GLOBALS REFERENCED:
**
**============================================================================
*/

static char *g_hdrWriteAsFits = "hdrWriteAsFits";
static ftclArgvInfo  g_hdrWriteAsFitsArgTable[] = {
      {NULL, FTCL_ARGV_HELP, NULL, NULL,
       "Write a header to a FITS file\n" },
      {"<header>", FTCL_ARGV_STRING, NULL, NULL, NULL},
      {"<FITSfile>", FTCL_ARGV_STRING, NULL, NULL,
       "File to contain the header or pipe descriptor if -pipe is present"},
      {"-reg",    FTCL_ARGV_STRING, NULL, NULL, NULL},
      {"-dirset",    FTCL_ARGV_STRING, NULL, NULL, NULL},
      {"-standard",  FTCL_ARGV_STRING, NULL, NULL, NULL},
      {"-naxis",     FTCL_ARGV_INT, NULL, NULL, NULL},
      {"-pipe",      FTCL_ARGV_CONSTANT, (void *)1, NULL,
       "If present, FITSfile is actually a pipe descriptor"},
      {"-tape",      FTCL_ARGV_CONSTANT, (void *)1, NULL,
       "Write to tape device specified in <FITSFILE>"},
      {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
    };

static int shTclHdrWriteAsFits
   (
   ClientData	a_clientData,	/* IN: */
   Tcl_Interp	*a_interp,	/* IN: */
   int		argc,		/* IN: */
   char		**argv		/* IN: */
   )   
{
HDR          *hdrPtr;
REGION       *regPtr = NULL;
RET_CODE     rstatus;
FITSFILETYPE fitsType;
char         *hdrNamePtr = NULL;
char         *regNamePtr = NULL;
char         *l_filePtr;
DEFDIRENUM   FITSdef;			/* dirSet default to use */
char*        dirsetValPtr = NULL;
char         *c_fitsType = "T";
int          naxis = 2;
int          outPipe = 0;
int          flags = FTCL_ARGV_NO_LEFTOVERS;
OpenFile     *outFilePtr;
FILE         *hfile = NULL;
int          outTape = 0;              /* option to write to tape */

outFilePtr = NULL;

/* Parse the command */
g_hdrWriteAsFitsArgTable[1].dst = &hdrNamePtr;
g_hdrWriteAsFitsArgTable[2].dst = &l_filePtr;
g_hdrWriteAsFitsArgTable[3].dst = &regNamePtr;
g_hdrWriteAsFitsArgTable[4].dst = &dirsetValPtr;
g_hdrWriteAsFitsArgTable[5].dst = &c_fitsType;
g_hdrWriteAsFitsArgTable[6].dst = &naxis;
g_hdrWriteAsFitsArgTable[7].dst = &outPipe;
g_hdrWriteAsFitsArgTable[8].dst = &outTape;

rstatus = shTclParseArgv(a_interp, &argc, argv, g_hdrWriteAsFitsArgTable,
                         flags, g_hdrWriteAsFits);

if (rstatus == FTCL_ARGV_SUCCESS)
   {
   /* Get the 'standard' option - if none, the default will be returned */
   if ((c_fitsType[0] == 'F') || (c_fitsType[0] == 'f'))
      {fitsType = NONSTANDARD;}
   else
      {fitsType = STANDARD;}

   /* Can't have both -tape and -pipe */
   if (outPipe && outTape)
      {
      Tcl_AppendResult(a_interp, "\n  Illegal to specify both -tape and -pipe ",
		       (char *)NULL);
      rstatus = TCL_ERROR;
      goto rtn_return;
      }

   /* We do not need to get the file specification if we are using a pipe or tape */
   if (!outPipe && !outTape)
      {
      /* Get the dirset file specification defaulting info */
      p_shTclDIRSETexpand(dirsetValPtr,a_interp, DEF_REGION_FILE, &FITSdef,
			  ((char *)0), ((char **)0), ((char **)0));
      }

   /* Check if this is valid header by trying to get the pointer to it */
   if ((rstatus = shTclHdrAddrGetFromExpr(a_interp, hdrNamePtr,
                                          &hdrPtr)) == TCL_OK)
      {
      /* Yes the header is legal */

      /* See if we were passed a region name. */
      if(regNamePtr != NULL)
	 {
	 /* Yes we were passed a region name, see if it is legal or not. */
	 if ((rstatus = shTclRegAddrGetFromName(a_interp, regNamePtr,
						&regPtr)) != TCL_OK)
	    {
	    /* No the region is not legal */
	    goto rtn_return;
	    }

	 /* The default for naxis is 2.  However, if the region contains no
	    data (nrow * ncol = 0), then the default for naxis should be 0. */
	 if ((regPtr->nrow * regPtr->ncol) == 0)
	    {naxis = 0;}
	 }
      
      /* Validate the pipe we were given (if we were given one) */
      if (outPipe)
	 {
	 if ((outFilePtr = Tcl_GetOpenFileStruct(a_interp, l_filePtr)) == NULL)
	    {goto rtn_return;}
	 hfile = outFilePtr->f;      /* Get ptr to pipes 'file' descriptor */
	 }

      /* Write out the FITS header and the pixels */
      if ((rstatus = shHdrWriteAsFits(hdrPtr, l_filePtr, fitsType, naxis,
                     FITSdef, regPtr, hfile, outTape)) != SH_SUCCESS)
         {
         Tcl_AppendResult(a_interp, "\n  Attempting to write header : ",
                          hdrNamePtr, "\n", (char *)NULL);
         rstatus = TCL_ERROR;
         }
      else
         { 
         /* Return with header name if successful */
         Tcl_SetResult(a_interp, hdrNamePtr, TCL_VOLATILE);
         rstatus = TCL_OK;				/* Success */
         }
      }
   }

rtn_return:

if (rstatus != TCL_OK) 
  {
  shTclInterpAppendWithErrStack(a_interp); /* Append errors from stack if any */
  }
return (rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: shTclFitsIoDeclare
**
** DESCRIPTION:
**      Declare TCL verbs for this module.
**
** RETURN VALUES:
**      N/A
**
**============================================================================
*/
void shTclFitsIoDeclare(Tcl_Interp *interp)
{
int flags = FTCL_ARGV_NO_LEFTOVERS;

shTclDeclare(interp, g_dirGet, (Tcl_CmdProc *)shTclDirGet,
   (ClientData) 0, (Tcl_CmdDeleteProc *) NULL, "shFitsIo", 
   shTclGetArgInfo(interp, g_dirGetArgTable, flags, g_dirGet),
   shTclGetUsage(interp, g_dirGetArgTable, flags, g_dirGet));

shTclDeclare(interp, g_dirSet, (Tcl_CmdProc *)shTclDirSet,
   (ClientData) 0, (Tcl_CmdDeleteProc *) NULL, "shFitsIo", 
   shTclGetArgInfo(interp, g_dirSetArgTable, flags, g_dirSet),
   shTclGetUsage(interp, g_dirSetArgTable, flags, g_dirSet));

shTclDeclare(interp, "dirList", (Tcl_CmdProc *)shTclDirList,
   (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
   "shFitsIo", "List the default directories.", TCLUSAGE_DIRLIST);

shTclDeclare(interp, g_regReadAsFits, (Tcl_CmdProc *)shTclRegReadAsFits,
   (ClientData) 0, (Tcl_CmdDeleteProc *) NULL, "shFitsIo", 
   shTclGetArgInfo(interp, g_regReadAsFitsArgTable, flags, g_regReadAsFits),
   shTclGetUsage(interp, g_regReadAsFitsArgTable, flags, g_regReadAsFits));

shTclDeclare(interp, g_regWriteAsFits, (Tcl_CmdProc *)shTclRegWriteAsFits,
   (ClientData) 0, (Tcl_CmdDeleteProc *) NULL, "shFitsIo", 
   shTclGetArgInfo(interp, g_regWriteAsFitsArgTable, flags, g_regWriteAsFits),
   shTclGetUsage(interp, g_regWriteAsFitsArgTable, flags, g_regWriteAsFits));

shTclDeclare(interp, g_hdrReadAsFits, (Tcl_CmdProc *)shTclHdrReadAsFits,
   (ClientData) 0, (Tcl_CmdDeleteProc *) NULL, "shFitsIo", 
   shTclGetArgInfo(interp, g_hdrReadAsFitsArgTable, flags, g_hdrReadAsFits),
   shTclGetUsage(interp, g_hdrReadAsFitsArgTable, flags, g_hdrReadAsFits));

shTclDeclare(interp, g_hdrWriteAsFits, (Tcl_CmdProc *)shTclHdrWriteAsFits,
   (ClientData) 0, (Tcl_CmdDeleteProc *) NULL, "shFitsIo", 
   shTclGetArgInfo(interp, g_hdrWriteAsFitsArgTable, flags, g_hdrWriteAsFits),
   shTclGetUsage(interp, g_hdrWriteAsFitsArgTable, flags, g_hdrWriteAsFits));
}
