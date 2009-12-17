/*****************************************************************************
******************************************************************************
**<AUTO>
** FILE:
**	tclRegion.c
**
**<HTML>
**	These procedures allow regions to be created, deleted, and parented.
**	Subregions may be created from regions. Overall, these modules present 
**	the <A HREF=dervish.region.html#region>basic features of regions</A> to the 
**	TCL user.
**</HTML>
**</AUTO>
**
** ABSTRACT:
**	This file contains routines that support the definition of TCL verbs
**	that manipulate regions.
**
** ENTRY POINT		SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
** shTclRegNew		local	Create a new region
** shTclSubRegNew	local	Create a new subregion
** shTclRegNewFromReg	local   Create a new region based on an existing region
** shTclRegDel		local	Delete a region
** shTclRegReadFromPool	local	Read region data from the pool
** shTclRegDump		local	Dump region data
** shTclSubRegReparent	local	Promote a subregion to a region
** shTclRegionDeclare	public	Declare TCL verbs for this module
**
** ENVIRONMENT:
**	ANSI C.
**
** REQUIRED PRODUCTS:
**	FTCL
**	LIBFITS
**
** CONDITIONAL COMPILATION DIRECTIVES:
**	PHYSICAL_REGION_TEST	TCL command for simulating a physical region
**				with a virtual region.
**
** CREATION DATE:
**	May 7, 1993
**
******************************************************************************
******************************************************************************
*/
#include <string.h>
#include "ftcl.h"
#include "region.h"
#include "shCGarbage.h"
#include "shCUtils.h"		/* Needed for prvt/utils_p.h ??? */
#include "prvt/region_p.h"
#include "prvt/utils_p.h"
#include "shCErrStack.h"
#include "shTclErrStack.h"
#include "shCSao.h"
#include "shTclVerbs.h"
#include "shTclRegSupport.h"
#include "shTclUtils.h"
#include "shTclParseArgv.h"
#include "shCFitsIo.h"		/* Needed for MAXDDIRLEN and MAXDEXTLEN */
#include "shCHdr.h"
#include "shCAssert.h"
#include "libfits.h"		/* Needed for f_ikey prototype */
#ifdef PHYSICAL_REGION_TEST
#include "shCRegPhysical.h"
#endif /* PHYSICAL_REGION_TEST */

/*============================================================================  
**============================================================================
**
** LOCAL DEFINITIONS
**
**============================================================================
*/

/*
** MAXIMUM ASCII NAME FOR REGION (DON'T CONFUSE WITH MAXTCLREGNAMELEN)
*/
#define L_MAXREGNAMELEN	70


/*============================================================================  
**============================================================================
**
** ROUTINE: shTclRegNew
**
**<AUTO>
** TCL VERB:
**	regNew
**
**<HTML>
** <P>This procedure creates a region of the size indicated by NROWS and NCOLS.
** If the size is not specified a region with 0 rows and 0 columns is created.
** If the -physical switch is specified, The region is allocated from the
** pools of physical space. (Physical regions are meaningful only in the data
** acquisition systems.) Pixels are not cleared, and have unpredictable
** contents.
**
** <P>If the -mask switch is present, a mask is created with
** along with the pixels.  The mask is cleared?? has random contents?
**
** <P>The -type switch specified the type of the pixels in
** the region. If omitted, the type of the pixels is U16. Available types are
** U8 S8 S16 U16 S32 U32 and FL32.
**
** <P>The -name switch may be used to load the name field in the region
** structure. This name should not be confused with the handle to the region
** returned by this command.  If not specified, the name of the region's handle
** is placed in the name field.
**</HTML>
** TCL SYNTAX:
**   regNew [-physical] [-type U16] [-mask] [-name] [nrows 0 ncols 0]
**
**	nrows		Integer giving number of rows.
**	ncols		Integer giving number of rows.
**	-physical	Specifying region is to be created from Physical Pool.
**	-type		Specifies type of underlying pixels.
**	-name		Specifies name for a region.
**
** RETURNS:
**	TCL_OK		Successful completion. Handle to region is returned.
**	TCL_ERROR	Error occurred.  The Interp result explains the error.
**
**</AUTO>
**============================================================================
*/
static char *TCLUSAGE_REGNEW	      =
   "USAGE: regNew [-physical] [-type image_type] [-mask] [-name name] [<nrows> <ncols>]";

static int shTclRegNew
   (
   ClientData	a_clientdata,	/* IN : TCL parameter - not used */
   Tcl_Interp	*a_interp,	/* OUT: TCL Interpreter structure */
   int		argc,		/* IN : TCL argument count */
   char		**argv		/* IN : TCL argument */
   )   
{
REGION	    *regPtr;
MASK	    *maskPtr;
PIXDATATYPE regType;
char	    *regTypeASCII, *nameASCII;
char	    *dumPtr;
char	    tclRegionName[MAXTCLREGNAMELEN];
char	    passedNameASCII[L_MAXREGNAMELEN+1];
RET_CODE    rfunc;
int	    i;
int	    nrow = 0;
int	    ncol = 0;
PHYS_CONFIG *physConfigPtr = NULL;
int	    b_syntax   = 0;
int	    b_physical = 0;
int	    b_mask     = 0;
int	    b_type     = 0;
int	    rowColCount= 0;

regTypeASCII = "U16";	    /* Image data type - default is unsigned 16-bit */
nameASCII = tclRegionName;  /* ASCII name for region - default is TCL region
                               name */

/*
** PARSE ARGUMENTS
*/
for (i=1; i<argc; i++)
   {
   /* Handle switches */
   if (argv[i][0] == '-')
      {
      /* Pixel mask */
      if (!strcmp("-mask", argv[i]))
         {b_mask = 1;}

      /* Image data type */
      else if (!strcmp("-type", argv[i]))
         {
         i++;
         if ((i >= argc) || (argv[i][0] == '-'))
            {
            shErrStackPush("Missing/invalid argument for 'type' switch");
            b_syntax = 1; goto err_exit;
            }
         b_type = 1;
         regTypeASCII = argv[i];
         }

      /* Physical or virtual region */
      else if (!strcmp("-physical", argv[i]))
         {b_physical = 1;}

      /* ASCII name for region */
      else if (!strcmp("-name", argv[i]))
         {
         i++;
         if ((i >= argc) || (argv[i][0] == '-'))
            {
            shErrStackPush("Missing/invalid argument for 'name' switch");
            b_syntax = 1; goto err_exit;
            }
         strncpy(passedNameASCII, argv[i], L_MAXREGNAMELEN);
         passedNameASCII[L_MAXREGNAMELEN] = (char)0;
         nameASCII = passedNameASCII;
         }

      /* Invalid switch */
      else
         {
         shErrStackPush("Unknown switch '%s'", &argv[i][1]);
         b_syntax = 1; goto err_exit;
         }
      }

   /* Handle row and column entries (or other garbage) */
   else
      {
      if (rowColCount == 0)
         {
         /* Number of rows and columns in the image */
         nrow = (int)strtol(argv[i], &dumPtr, 0);
         if (dumPtr == argv[i]) 
            {
            shErrStackPush("Expecting number of rows, got: '%s'", argv[i]);
            b_syntax = 1; goto err_exit;
            }
         rowColCount++;
         }
      else
         {
         ncol = (int)strtol(argv[i], &dumPtr, 0);
         if (dumPtr == argv[i]) 
            {
            shErrStackPush("Expecting number of columns, got: '%s'", argv[i]);
            b_syntax = 1; goto err_exit;
            }
         rowColCount++;
         }
      }
   }

/* Make sure both row and column were entered, or none at all */
if ((rowColCount == 1) || (rowColCount > 2))
    {
    shErrStackPush("Syntax error");
    b_syntax = 1; goto err_exit;
    }

/* Convert the region type to an enumeration */
if (shTclRegTypeGetAsEnum(a_interp, regTypeASCII, &regType) != TCL_OK)
    {b_syntax = 1; goto err_exit;}

/*
** EXTRA PARSING CHECKS FOR PHYSICAL REGIONS
*/
if (b_physical)
   {
   if ((nrow !=0) || (ncol !=0))
      {
      shErrStackPush(
          "Number of rows and columns can't be specified for physical regions");
      goto err_exit;
      }
   if (b_type)
      {
      shErrStackPush("\"type\" option not allowed for physical regions");
      goto err_exit;
      }
   if (b_mask)
      {
      shErrStackPush("\"mask\" option not allowed for physical regions");
      goto err_exit;
      }
   }

/*
** CREATE REGION
*/
/* Note that for physical regions nrow, ncol, and b_mask are zero.
   We would not make it to here if that were not the case. */
/* Now check to make sure a physical configuration structure is available */
if (b_physical)
   {
   physConfigPtr = p_shPhysReserve();
   if (physConfigPtr == NULL)
      {
      shErrStackPush("All physical regions are in use or none are configured");
      goto err_exit;
      }
   }

/* Get new TCL region name */
if (shTclRegNameGet(a_interp, tclRegionName) != TCL_OK)
   {goto err_exit;}

/* Create new region (using TCL region name if no name was specified) */
regPtr = shRegNew(nameASCII, nrow, ncol, regType);
if (regPtr == NULL)
   {
   shTclRegNameDel(a_interp, tclRegionName);
   goto err_exit;
   }

/* Associate region with TCL region name */
if (shTclRegNameSetWithAddr(a_interp, regPtr, tclRegionName) != TCL_OK)
   {
   shRegDel(regPtr);
   shTclRegNameDel(a_interp, tclRegionName);
   goto err_exit;
   }

/* Add mask if specified */
if (b_mask)
   {
   char *maskNamePtr;

   /*
    * Distinguish a mask name from the region name it's attached to. For 
    * example, if a region is named "reg1", then let's make the mask's 
    * name to be "reg1-mask"
    */
   maskNamePtr = (char *) shMalloc(strlen(nameASCII) + strlen("-mask") + 1);
   sprintf(maskNamePtr, "%s-mask", nameASCII);

   maskPtr = shMaskNew(maskNamePtr, nrow, ncol);
   shFree(maskNamePtr);   /* shMaskNew() allocates memory for maskNamePtr, */
                          /* so we can free it now */
   if (maskPtr == NULL)
      {
      shRegDel(regPtr);
      shTclRegNameDel(a_interp, tclRegionName);
      goto err_exit;
      }
   regPtr->mask = maskPtr;

   }

/*
** CREATE HEADER AND PIXEL BUFFER FOR PHYSICAL REGION (IF NECESSARY)
*/
/* For a physical region associate the region and physical structure, then
   designate the region as being physical */
if (b_physical)
   {
   physConfigPtr->physRegPtr = (void *)regPtr;
   regPtr->prvt->phys = physConfigPtr;
   regPtr->prvt->type = SHREGPHYSICAL;

   /* Copy header from physical region (if valid data is there) */
   rfunc = p_shRegHdrCopyFromPhys(regPtr, &nrow, &ncol);
   if (rfunc == SH_SUCCESS)
      {
      /* Header is valid, so create the pixel vector */
      if (p_shRegPixNewFromPhys(regPtr, nrow, ncol) != SH_SUCCESS)
         {goto err_exit;}
      }

   /* Continue to success only if there was no header */
   else if (rfunc != SH_PHYS_HDR_INV)
      {goto err_exit;}
   }

/*
** EXIT HANDLING
*/
/* Successful return with TCL region name */
Tcl_SetResult(a_interp, tclRegionName, TCL_VOLATILE);
return(TCL_OK);			/* We made it ! */

err_exit:
shTclInterpAppendWithErrStack(a_interp); /* Append errors from stack - if any */
if (b_syntax)
   {Tcl_AppendResult(a_interp, TCLUSAGE_REGNEW, (char *)NULL);}
if ((b_physical) && (physConfigPtr))
   {p_shPhysUnreserve(physConfigPtr);}
return(TCL_ERROR);
}


/*============================================================================  
**============================================================================
**
** ROUTINE: shTclSubRegNew
**
**<AUTO>
** TCL VERB: subRegNew
**
**<HTML>
** <P>Generate a sub-region from the given region. The
** <A HREF=region.feature.html>basic features  of regions</A> give the
** rules for the behavior of this procedure.
**</HTML>
**
** TCL SYNTAX:
**  subRegNew <parent> nrow ncol srow scol [-read]
**
**	parent		handle to parent region
**	nrow		number of rows in sub-region
**	ncol		number of columns in sub region
**	srow		smallest row of parent in sub region
**	scol		smallest col of parent in sub region.
**	-read		switch specifying the subregion as readonly
**
** RETURNS:
**	TCL_OK		Successful completion.
**	TCL_ERROR	Error occurred.  The Interp result explains the error.
**
**</AUTO>
**============================================================================
*/
static char *TCLUSAGE_SUBREGNEW	      = 
   "USAGE: subRegNew <parent> <nrow> <ncol> <srow> <scol> [-read] [-mask] [-copy_header_deep] [-enlarge_exact]";

static int shTclSubRegNew
   (
   ClientData	a_clientdata,	/* IN : TCL parameter - not used */
   Tcl_Interp	*a_interp,	/* OUT: TCL Interpreter structure */
   int		argc,		/* IN : TCL argument count */
   char		**argv		/* IN : TCL argument */
   )   
{
int	    nrow, ncol, srow, scol;
REGION	    *regPtr, *subPtr;
char	    *regName;
char        *dumPtr;
char	    subName[MAXTCLREGNAMELEN];
int	    b_syntax   = 0;
REGION_FLAGS flags     = NO_FLAGS;
REGION_FLAGS sav_flags;
int optind;

/*
** PARSE ARGUMENTS
*/
if ((argc >= 6))
   {
   /* Region Name - translate to address */
   regName = argv[1];
   if (shTclRegAddrGetFromName(a_interp, regName, &regPtr) != TCL_OK)
      {goto err_exit;}

   /* Number of rows and columns in subregion image */
   nrow = (int)strtol(argv[2], &dumPtr, 0);
   if (dumPtr == argv[2]) {b_syntax = 1; goto err_exit;}
   ncol = (int)strtol(argv[3], &dumPtr, 0);
   if (dumPtr == argv[3]) {b_syntax = 1; goto err_exit;}

   /* Row and column of parent image where subregion starts */
   srow = (int)strtol(argv[4], &dumPtr, 0);
   if (dumPtr == argv[4]) {b_syntax = 1; goto err_exit;}
   scol = (int)strtol(argv[5], &dumPtr, 0);
   if (dumPtr == argv[5]) {b_syntax = 1; goto err_exit;}

   /* "Handle options corresponginf to those allowed in REGION_FLAGS*/
   optind = 6;			/* optial args start at six */
   while (optind < argc)  {
      sav_flags = flags;
      if (!strcmp(argv[optind], "-read"))
         {flags = (REGION_FLAGS )(flags | READ_ONLY);}
      if (!strcmp(argv[optind], "-mask"))
         {flags = (REGION_FLAGS )(flags | COPY_MASK);}
      if (!strcmp(argv[optind], "-enlarge_exact"))
         {flags = (REGION_FLAGS )(flags | ENLARGE_EXACT);}
      if (!strcmp(argv[optind], "-copy_header_deep"))
         {flags = (REGION_FLAGS )(flags | COPY_HEADER_DEEP);}
      if (flags == sav_flags)
         {b_syntax = 1; goto err_exit;}
      optind ++;
      }
   shAssert(regPtr != NULL);
   }
else
   {b_syntax = 1; goto err_exit;}

/*
** CREATE SUBREGION
*/

/* Get new name for subregion, then create subregion */
if (shTclRegNameGet(a_interp, subName) != TCL_OK) 
   {goto err_exit;}
subPtr = shSubRegNew(subName, regPtr, nrow, ncol, srow, scol, flags);
if (subPtr == NULL)
   {
   shTclRegNameDel(a_interp, subName);
   goto err_exit;
   }

/* Associate subregion with TCL region name */
if (shTclRegNameSetWithAddr(a_interp, subPtr, subName) != TCL_OK)
   {
   shRegDel(subPtr);
   shTclRegNameDel(a_interp, subName);
   goto err_exit;
   }

/* Return with TCL region name */
Tcl_SetResult(a_interp, subName, TCL_VOLATILE);

/*
** EXIT HANDLING
*/
/* Normal successful return */
return(TCL_OK);

err_exit:
shTclInterpAppendWithErrStack(a_interp); /* Append errors from stack - if any */
if (b_syntax)
   {
   shErrStackPush("Syntax error");
   Tcl_AppendResult(a_interp, TCLUSAGE_SUBREGNEW, (char *)NULL);
   }
return(TCL_ERROR);
}


/*============================================================================  
**============================================================================
**
** ROUTINE: shTclRegNewFromReg
**
**<AUTO>
** TCL VERB: regNewFromReg
**
**<HTML>
** <P>Generate a new region whose properties are derived from the input
** region.  The user may specify a new type for the region using the
** -type switch. This procedure can be used to promote a sub region to
** region(i.e. allow it to own its own pixels).
**
** <P> By default, the new region has a mask if its parent does. The mask
** is Cleared? Set to a copy of its parent? if the -mask switch is
** present, the region will have a (cleared?) mask even if the parent does
** not.
**
** <P>If the -name switch is present, it is used as the name of the region.
** Be sure to not confuse this name with the name of the handle pointing to the
** region. If -name is omitted, the handle to the region is used instead.
**</HTML>
** TCL SYNTAX:
**  regNewFromReg <region> [-type image_type] [-hdr] [-mask] [-name name]
**
**	region		handle to parent region
**	-hdr		specifying that the new region have a hdr
**	-mask		specifying that the new region have a mask
**	-name		specifying the name of the new region
**	-type		specifying the image type of the new region
**			(default = source region type)
** RETURNS:
**	TCL_OK		Successful completion.
**	TCL_ERROR	Error occurred.  The Interp result explains the error.
**
**</AUTO>
**============================================================================
*/
static char *TCLUSAGE_REGNEWFROMREG   = 
   "USAGE: regNewFromReg <source> [-type image_type] [-mask] [-hdr] [-name name]";

static int shTclRegNewFromReg
   (
   ClientData	a_clientdata,	/* IN : TCL parameter - not used */
   Tcl_Interp	*a_interp,	/* OUT: TCL Interpreter structure */
   int		argc,		/* IN : TCL argument count */
   char		**argv		/* IN : TCL argument */
   )   
{
int	    i;
PIXDATATYPE regType;
char	    tclRegionName[MAXTCLREGNAMELEN];
char	    passedNameASCII[L_MAXREGNAMELEN+1];
REGION	    *sourcePtr;
char	    *nameASCII;
char	    *regTypeASCII = NULL;
char	    *sourceName= NULL;
REGION	    *newPtr    = NULL;
REGION_FLAGS flags     = NO_FLAGS;
int	    b_syntax   = 0;
RET_CODE    rfunc      = SH_SUCCESS;

nameASCII = tclRegionName;  /* ASCII name for region - default is TCL region
                               name */

/*
** PARSE ARGUMENTS
*/
for (i=1; i<argc; i++)
   {
   /* Handle switches */
   if (argv[i][0] == '-')
      {
      /* Pixel mask */
      if (!strcmp("-mask", argv[i]))
         {flags = (REGION_FLAGS )(flags | COPY_MASK);}

      /* Header flag */
      else if (!strcmp("-hdr", argv[i]))
         {flags = (REGION_FLAGS )(flags | COPY_HEADER_DEEP);}

      /* Image data type */
      else if (!strcmp("-type", argv[i]))
         {
         i++;
         if ((i >= argc) || (argv[i][0] == '-'))
            {
            shErrStackPush("Missing/invalid argument for 'type' switch");
            b_syntax = 1; goto err_exit;
            }
         regTypeASCII = argv[i];
         }

      /* ASCII name for region */
      else if (!strcmp("-name", argv[i]))
         {
         i++;
         if ((i >= argc) || (argv[i][0] == '-'))
            {
            shErrStackPush("Missing/invalid argument for 'name' switch");
            b_syntax = 1; goto err_exit;
            }
         strncpy(passedNameASCII, argv[i], L_MAXREGNAMELEN);
         passedNameASCII[L_MAXREGNAMELEN] = (char)0;
         nameASCII = passedNameASCII;
         }

      /* Invalid switch */
      else
         {
         shErrStackPush("Unknown switch '%s'", &argv[i][1]);
         b_syntax = 1; goto err_exit;
         }
      }

   /* Handle region entries (or other garbage) */
   else
      {
      /* If we've been here already, must be syntax error */
      if (sourceName)
         {b_syntax = 1; goto err_exit;}

      /* Assume Region Name - translate to address */
      sourceName = argv[i];
      if (shTclRegAddrGetFromName(a_interp, sourceName, &sourcePtr) != TCL_OK)
         {goto err_exit;}
      }
   } /* end for */

/* Make sure required source region name was entered */
if (sourceName == NULL)
   {
   shErrStackPush("Source region not specified");
   b_syntax = 1; goto err_exit;
   }

/* Determine what the new region type will be */
if (regTypeASCII == NULL)
   {
   /* User didn't specify - so get the region type of the source region */
   regType = sourcePtr->type;
   }
else
   {
   /* Convert the region type specified by user to an enumeration */
   if (shTclRegTypeGetAsEnum(a_interp, regTypeASCII, &regType) != TCL_OK)
      {b_syntax = 1; goto err_exit;}
   }

/* Does the orignal region have a mask, if so the new region should too. */
if (sourcePtr->mask != NULL)
   {flags = (REGION_FLAGS )(flags | COPY_MASK);}

/*
** MAKE A COPY OF THE SOURCE REGION
*/
/* Get new TCL region name */
if (shTclRegNameGet(a_interp, tclRegionName) != TCL_OK)
   {goto err_exit;}

/* Copy region (using TCL region name if no name was specified) */
newPtr = shRegNewFromReg(sourcePtr, nameASCII, regType, flags, &rfunc);

/* Associate region with TCL region name */
if (shTclRegNameSetWithAddr(a_interp, newPtr, tclRegionName) != TCL_OK)
   {
   shRegDel(newPtr);
   shTclRegNameDel(a_interp, tclRegionName);
   goto err_exit;
   }

/* Return with the TCL region name */
Tcl_SetResult(a_interp, tclRegionName, TCL_VOLATILE);

/*
** EXIT HANDLING
*/
/* Normal successful return */
return(TCL_OK);

err_exit:
shTclInterpAppendWithErrStack(a_interp); /* Append errors from stack - if any */
if (b_syntax)
   {Tcl_AppendResult(a_interp, TCLUSAGE_REGNEWFROMREG, (char *)NULL);}
return(TCL_ERROR);
}


/*============================================================================  
**============================================================================
**
** ROUTINE: shTclRegDel
**
**<AUTO>
** TCL VERB: regDel
**
**<HTML>
** <P>Delete the region specified by the REGION parameter. The handle
** is freed, the region is deleted if there are not other handles to it.
** It is an error to delete a physical region. It is an error to delete a
** region with sub-regions attached to it.
**</HTML>
** TCL SYNTAX:
**  regDel <region>
**
**	region		handle to parent region
**
** RETURNS:
**	TCL_OK		Successful completion.
**	TCL_ERROR	Error occurred.  The Interp result explains the error.
**
**</AUTO>
**============================================================================
*/
static char *TCLUSAGE_REGDEL	      = 
   "USAGE: regDel <regName>";

static int shTclRegDel
   (
   ClientData	a_clientdata,	/* IN : TCL parameter - not used */
   Tcl_Interp	*a_interp,	/* OUT: TCL Interpreter structure */
   int		argc,		/* IN : TCL argument count */
   char		**argv		/* IN : TCL argument */
   )   
{
char	    *regName;
REGION	    *regPtr    = NULL;
int	    b_syntax   = 0;

/*
** PARSE ARGUMENTS
*/
if (argc == 2)
   {
   regName = argv[1];
   if (shTclRegAddrGetFromName(a_interp, regName, &regPtr) != TCL_OK)
      {goto err_exit;}
   }
else
   {b_syntax = 1; goto err_exit;}

/*
** DELETE PHYSICAL REGION SPECIFICS
*/
if (regPtr->prvt->type == SHREGPHYSICAL)
   {
   if(regPtr->prvt->nchild != 0)
      {
      shErrStackPush("Attempt to delete a physical region with children");
      goto err_exit;
      }

   p_shHdrFree(&regPtr->hdr);		/* Free header */
   p_shHdrFreeForVec(&regPtr->hdr);	/* Free header vector */
   p_shRegPixFreeFromPhys(regPtr);	/* Free pixel vector */

   /* Disassociate the region and physical structure */
   p_shPhysUnreserve(regPtr->prvt->phys);
   regPtr->prvt->phys = NULL;
   regPtr->prvt->type = SHREGINVALID;    /* Invalidate region */

   /* We can continue on from here since the region has been invalidated
      and the header/pixel vectors freed.  The subsequent call to shRegDel 
      should free the region itself.  Note that we do NOT rely on shRegDel 
      to free the header since p_shHdrFree must be called while the region
      is still considered to be physical (this is a minor kludge - fix it
      later). */
   }

/* Disassociate tcl name from region */
if (shTclRegNameDel(a_interp, regName) != TCL_OK)
   {goto err_exit;}

/*
** DELETE REGION
*/
if (shRegDel(regPtr) != SH_SUCCESS)	/* Nuke it */
   {goto err_exit;}

/*
** EXIT HANDLING
*/
/* Normal successful return */
return(TCL_OK);

err_exit:
shTclInterpAppendWithErrStack(a_interp); /* Append errors from stack - if any */
if (b_syntax)
   {Tcl_AppendResult(a_interp, TCLUSAGE_REGDEL, (char *)NULL);}
return(TCL_ERROR);
}


/*============================================================================  
**============================================================================
**
** ROUTINE: shTclRegReadFromPool
**
**<AUTO EXTRACT>
** TCL VERB:
**	regReadFromPool
**
** DESCRIPTION:
**	Read in a physical region from the frame pool.  In all cases, the
**	existing region is overwritten.
**
**</AUTO>
** RETURN VALUES:
**	TCL_OK	    Successful completion.
**	TCL_ERROR   Error occurred.  The Interp result string will contain
**		    an error string.
**
**============================================================================
*/
static char g_regReadFromPool[] = "regReadFromPool";
static ftclArgvInfo shTclRegReadFromPoolArgTable[] = {
       {NULL, FTCL_ARGV_HELP, NULL, NULL, 
        "Read region data from the pool\n"},
       {"<reg>", FTCL_ARGV_STRING,   NULL, NULL, 
        "Region to read into"},
       {"<frame>", FTCL_ARGV_STRING,   NULL, NULL, 
        "Frame number"},
       {"-group", FTCL_ARGV_STRING,   NULL, NULL, 
        "e.g. DEF_POOL_GROUP"},
       {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
    };

static int shTclRegReadFromPool
   (
   ClientData	a_clientdata,	/* IN : TCL parameter - not used */
   Tcl_Interp	*a_interp,	/* OUT: TCL Interpreter structure */
   int		argc,		/* IN : TCL argument count */
   char		**argv		/* IN : TCL argument */
   )   
{
char	    tclRegionName[MAXTCLREGNAMELEN],*tclRegionNamePtr=NULL;
char	    framename[MAXDDIRLEN], *frameNamePtr = NULL, *groupPtr = "NONE";   
char	    dummy[MAXDEXTLEN];
char	    *tmpPtr;
REGION	    *regPtr    = NULL;
int         rstatus;

shTclRegReadFromPoolArgTable[1].dst = &tclRegionNamePtr;
shTclRegReadFromPoolArgTable[2].dst = &frameNamePtr;
shTclRegReadFromPoolArgTable[3].dst = &groupPtr;

/*
** PARSE ARGUMENTS
*/
framename[0] = (char)0;

if ((rstatus = shTclParseArgv(a_interp, &argc, argv,
			      shTclRegReadFromPoolArgTable,
			      FTCL_ARGV_NO_LEFTOVERS, g_regReadFromPool))
    == FTCL_ARGV_SUCCESS)
   {
   /* Region */
   strcpy(tclRegionName, tclRegionNamePtr);
   if (shTclRegAddrGetFromName(a_interp, tclRegionName, &regPtr) != TCL_OK)
      {goto err_exit;}

   /* Group (get default group if not specified) */
   tmpPtr = groupPtr;
   if (strlen(tmpPtr) > MAXDDIRLEN)
      {
      shErrStackPush("Group name is too long");
      goto err_exit;
      }
   if (!(strncmp(tmpPtr, "NONE", 4)))
      {shDirGet(DEF_POOL_GROUP, framename, dummy);}
   else
      {strcpy(framename, tmpPtr);}

   /* Frame number (in ASCII) */
   tmpPtr = frameNamePtr;

   /* Consolidate the group and frame number */
   if ((strlen(tmpPtr) + strlen(framename) + 1) > MAXDDIRLEN)
      {
      shErrStackPush("Combined group and frame name is too long");
      goto err_exit;
      }
   strcat(framename, " ");
   strcat(framename, tmpPtr);
   }
else
   {return(rstatus);}

/*
** READ FROM THE FRAME POOL
*/
if (p_shRegReadFromPool(regPtr, framename, 1) != SH_SUCCESS)
   {
   shErrStackPush("Attempting to read from : %s", framename);
   goto err_exit;
   }

/* Return with region name if successful */
Tcl_SetResult(a_interp, tclRegionName, TCL_VOLATILE);

/*
** EXIT HANDLING
*/
/* Normal successful return */

return(TCL_OK);

err_exit:

shTclInterpAppendWithErrStack(a_interp); /* Append errors from stack - if any */
return(TCL_ERROR);
}


/*============================================================================ 
**============================================================================
**
** ROUTINE: shTclRegPhysUpdate
**
**<AUTO EXTRACT>
** TCL VERB:
**	regPhysUpdate
**
** DESCRIPTION:
**      This routine is used to simply re-construct the hdr and
**      pixel vectors if the physical region was updated by some 
**	means other than through shTclRegReadFromPool.
**
**</AUTO>
** RETURN VALUES:
**	TCL_OK	    Successful completion.
**	TCL_ERROR   Error occurred.  The Interp result string will contain
**		    an error string.
**
**============================================================================
*/
static char g_regPhysUpdate[] = "regPhysUpdate";
static ftclArgvInfo g_regPhysUpdateTbl[] = {
       {NULL, FTCL_ARGV_HELP, NULL, NULL, 
        "Update handles when a physical region has changed\n"},
       {"<reg>", FTCL_ARGV_STRING,   NULL, NULL, 
        "Handle to physical region that has changed"},
       {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
    };

static int shTclRegPhysUpdate
   (
   ClientData	a_clientdata,	/* IN : TCL parameter - not used */
   Tcl_Interp	*a_interp,	/* OUT: TCL Interpreter structure */
   int		argc,		/* IN : TCL argument count */
   char		**argv		/* IN : TCL argument */
   )   
{
char	    tclRegionName[MAXTCLREGNAMELEN],*tclRegionNamePtr=NULL;
char	    *dummy	= NULL;
REGION	    *regPtr	= NULL;
int         rstatus;

g_regPhysUpdateTbl[1].dst = &tclRegionNamePtr;

/*
** PARSE ARGUMENTS
*/
if ((rstatus = shTclParseArgv(a_interp, &argc, argv, g_regPhysUpdateTbl,
			      FTCL_ARGV_NO_LEFTOVERS, g_regPhysUpdate))
    == FTCL_ARGV_SUCCESS)
   {
   /* Region */
   strcpy(tclRegionName, tclRegionNamePtr);
   if (shTclRegAddrGetFromName(a_interp, tclRegionName, &regPtr) != TCL_OK)
      {goto err_exit;}
   }
else
   {return(rstatus);}

/*
** UPDATE THE PHYSICAL REGION HANDLE
*/
if (p_shRegReadFromPool(regPtr, dummy, 0) != SH_SUCCESS)
   {
   shErrStackPush("Trouble updating physical region handle");
   goto err_exit;
   }

/* Return with region name if successful */
Tcl_SetResult(a_interp, tclRegionName, TCL_VOLATILE);

/*
** EXIT HANDLING
*/
/* Normal successful return */

return(TCL_OK);

err_exit:

shTclInterpAppendWithErrStack(a_interp); /* Append errors from stack - if any*/
return(TCL_ERROR);
}


#ifdef PHYSICAL_REGION_TEST
/*============================================================================ 
**============================================================================
**
** ROUTINE: testxxx
**
** DESCRIPTION:
**	These routines are used as part of the physical region test using 
**	virtual regions.
**
**============================================================================
*/

/*
** GLOBALS USED FOR PHYSICAL REGION TEST
*/
char	    *g_testHdrPtr;
U16	    *g_testPixPtr;

static char *testPixCall(int a_index, int a_bytes)
{
return((char *)g_testPixPtr);
}

static void testPixFree(int a_index, char *a_bufptr)
{
shFree(g_testPixPtr);
g_testPixPtr = NULL;
}

static char *testHdrCall(int a_index, int a_bytes)
{
return((char *)g_testHdrPtr);
}

static void testHdrFree(int a_index, char *a_bufptr)
{
shFree(g_testHdrPtr);
g_testHdrPtr = NULL;
}

static char *testFillCall()
{
return((char *)1);
}

/*============================================================================  
**============================================================================
**
** ROUTINE: shTclPhysTest
**
** DESCRIPTION:
**	Simulate a physical region by configuring a physical region,
**	and copying data from a virtual location to a known location. 
**
** RETURN VALUES:
**	TCL_OK	    Successful completion.  The Interp result string will
**		    contain the new region name.
**	TCL_ERROR   Error occurred.  The Interp result string will contain
**		    an error string.
**
**============================================================================
*/
static int shTclPhysTest
   (
   ClientData	a_clientdata,	/* IN : TCL parameter - not used */
   Tcl_Interp	*a_interp,	/* OUT: TCL Interpreter structure */
   int		argc,		/* IN : TCL argument count */
   char		**argv		/* IN : TCL argument */
   )   
{
int	    i, nrow, ncol, rowBytes;
char	    *regName;
char	    *hdrPtr;
char	    *pixPtr;
char	    naxis1[30], naxis2[30];
REGION	    *regPtr;
int	    b_syntax = 0;

/*
** PARSE ARGUMENTS
*/
if (argc==2)
   {
   /* Region Name - translate to address */
   regName = argv[1];
   if (shTclRegAddrGetFromName(a_interp, regName, &regPtr) != TCL_OK)
      {goto err_exit;}
   }
else
   {b_syntax = 1; goto err_exit;}

/*
** COPY THE SOURCE REGION TO MALLOCED BUFFERS
*/
if (regPtr->hdr.hdrVec == NULL)
   {shErrStackPush("Source region doesn't have header"); goto err_exit;}
if (regPtr->type != TYPE_U16)
   {shErrStackPush("Source region must be unsigned 16-bit"); goto err_exit;}

/* Malloc header buffer and copy source header (must be contiguous) */
if ((g_testHdrPtr = (char *)shMalloc(HDRLINECHAR*MAXHDRLINE)) == NULL)
   {shErrStackPush("Malloc failure"); goto err_exit;}
hdrPtr = g_testHdrPtr;

/* Replace the 5 required header lines */
sprintf(naxis1, "NAXIS1  = %d", regPtr->ncol);
sprintf(naxis2, "NAXIS2  = %d", regPtr->nrow);
sprintf(hdrPtr, "%-80s%-80s%-80s%-80s%-80s", "SIMPLE  = T", "BITPIX  = 16",
        "NAXIS   = 2", naxis1, naxis2);
hdrPtr += 80*5;

/* Copy the rest of the header lines */
for (i=0; i<(MAXHDRLINE-5); i++)
   {
   if (regPtr->hdr.hdrVec[i] == NULL)
      {break;}
   memmove(hdrPtr, regPtr->hdr.hdrVec[i], HDRLINECHAR);
   if (!(strncmp(hdrPtr, "END", 3)))
      {break;}
   hdrPtr += HDRLINECHAR;
   }

nrow = regPtr->nrow;
ncol = regPtr->ncol;

/* Malloc pixel buffer and copy source pixels */
if ((g_testPixPtr = (U16 *)shMalloc(nrow*ncol*sizeof(U16))) == NULL)
   {shErrStackPush("shMalloc failure"); goto err_exit;}
pixPtr = (char *)g_testPixPtr;
rowBytes = ncol*(sizeof(U16));
for (i=0; i<nrow; i++)
   {
   if (regPtr->rows_u16[i] == NULL)
      {break;}
   memmove(pixPtr, regPtr->rows_u16[i], rowBytes);
   pixPtr += rowBytes;
   }

/*
** CONFIGURE A PHYSICAL REGION WITH OUR OWN ROUTINES
*/
if (shPhysRegConfig(1, testPixCall, testPixFree, testHdrCall, testHdrFree, 
    testFillCall) != SH_SUCCESS)
   {goto err_exit;}

/*
** EXIT HANDLING
*/
/* Normal successful return */

return(TCL_OK);

err_exit:

shTclInterpAppendWithErrStack(a_interp); /* Append errors from stack - if any */
if (b_syntax)
   {Tcl_AppendResult(a_interp, "\n", "physTest <source>", (char *)NULL);}
return(TCL_ERROR);
}
#endif /* PHYSICAL_REGION_TEST */


/*============================================================================  
**============================================================================
**
** ROUTINE: shTclRegionDeclare
**
** DESCRIPTION:
**	Declare TCL verbs for this module.
**
** RETURN VALUES:
**	N/A
**
**============================================================================
*/
void shTclRegionDeclare(Tcl_Interp *interp)
{

int flags = FTCL_ARGV_NO_LEFTOVERS;

shTclDeclare(interp, "regNew", (Tcl_CmdProc *)shTclRegNew,
   (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
   "shRegion", "Create a new region", TCLUSAGE_REGNEW);

shTclDeclare(interp, "regDel", (Tcl_CmdProc *)shTclRegDel,
   (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
   "shRegion", "Delete a region", TCLUSAGE_REGDEL);

shTclDeclare(interp, "subRegNew", (Tcl_CmdProc *)shTclSubRegNew,
   (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
   "shRegion", "Create a sub-region of an existing region", 
   TCLUSAGE_SUBREGNEW);

shTclDeclare(interp, "regNewFromReg", (Tcl_CmdProc *)shTclRegNewFromReg,
   (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
   "shRegion", "Create a new region based on an existing region",
   TCLUSAGE_REGNEWFROMREG);

shTclDeclare(interp, g_regReadFromPool, (Tcl_CmdProc *)shTclRegReadFromPool,
   (ClientData) 0, (Tcl_CmdDeleteProc *) NULL, "shRegion",
   shTclGetArgInfo(interp, shTclRegReadFromPoolArgTable, flags, g_regReadFromPool),
   shTclGetUsage(interp, shTclRegReadFromPoolArgTable, flags, g_regReadFromPool));

shTclDeclare(interp, "regPhysUpdate", (Tcl_CmdProc *)shTclRegPhysUpdate,
   (ClientData) 0, (Tcl_CmdDeleteProc *) NULL, "shRegion",
   shTclGetArgInfo(interp, g_regPhysUpdateTbl, flags, g_regPhysUpdate),
   shTclGetUsage(interp, g_regPhysUpdateTbl, flags, g_regPhysUpdate));

#ifdef PHYSICAL_REGION_TEST
shTclDeclare(interp, "physTest", (Tcl_CmdProc *)shTclPhysTest,
   (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
   "shRegion", "Create physical region using virtual source region",
   "USAGE: physTest <source>");
#endif /* PHYSICAL_REGION_TEST */
}
