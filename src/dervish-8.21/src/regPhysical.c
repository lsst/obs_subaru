/*****************************************************************************
******************************************************************************
**
** FILE:
**	regPhysical.c
**
** ABSTRACT:
**	This file contains routines that support the use of physical regions.
**
** ENTRY POINT		    SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
** shPhysRegConfig	    public	Initialize phys configuration struct
** p_shPhysReserve	    private	Reserve available phys config struct
** p_shPhysUnreserve	    private	Detach reg from phys config struct
** p_shRegPixFreeFromPhys   private	Free reg from phys pixel buffer/vector
** p_shRegPixNewFromPhys    private	Create pixel buffer/vector from phys
** p_shRegHdrCopyFromPhys   private	Create virtual header from phys header
** p_shRegReadFromPool	    private	Read into region from frame pool
**
** ENVIRONMENT:
**	ANSI C.
**
** REQUIRED PRODUCTS:
**	LIBFITS
**
** CONDITIONAL COMPILATION DIRECTIVES:
**	PHYSICAL_REGION_TEST	Support for simulating a physical region
**				with a virtual region.
**
** AUTHORS:
**	Creation date:  May 25, 1993
**
******************************************************************************
******************************************************************************
*/
#include <string.h>
#include "region.h"
#include "shCUtils.h"	    /* Needed for prvt/utils_p.h */
#include "shCFitsIo.h"	    /* Needed for header size constants */
#include "prvt/region_p.h"
#include "prvt/utils_p.h"
#include "shCGarbage.h"
#include "shCHdr.h"
#include "shCErrStack.h"
#include "shCAssert.h"
#include "libfits.h"	    /* Needed for f_ikey prototype */

/*============================================================================  
**============================================================================
**
** GLOBAL VARIABLES FOR THIS MODULE
**
**============================================================================
*/
#define MAXPHYSREGION	4

static int	    g_physRegMax = 0;
static PHYS_CONFIG  g_physConfig[MAXPHYSREGION];


/*============================================================================  
**============================================================================
**
** ROUTINE: shPhysRegConfig
**
** DESCRIPTION:
**	Initialize physical configuration structures so that images located
**	in physical memory may be manipulated (at TCL level) similar to
**	virtual regions.  This routine must be called by the user's main
**	program in order to utilize the TCL commands available for physical
**	regions.
**
** RETURN VALUES:
**	SH_SUCCESS
**	SH_NO_PHYS_REG
**
** GLOBALS REFERENCED:
**	g_physRegMax
**	g_physConfig
**
**============================================================================
*/
RET_CODE shPhysRegConfig
   (
   int	a_numregions,	    /* IN : Number of regions to configure */
   char *(*a_pixelCall)(),  /* IN: Routine called to get pixel data pointer */
   void (*a_pixelFree)(),   /* IN: Routine called when done with pixel data */
   char	*(*a_hdrCall)(),    /* IN: Routine called to get header data pointer */
   void (*a_hdrFree)(),	    /* IN: Routine called when done with header data */
   int	(*a_fillCall)()     /* IN: Routine called to fill region with a frame */
   )   
{
int	    i, index;
PHYS_CONFIG *physConfigPtr;
RET_CODE    rstat = SH_SUCCESS;

/*
** LOOP AND CONFIGURE THE SPECIFIED NUMBER OF REGIONS 
*/
for (i=0; i < a_numregions; i++)   
   {

   /*
   ** GET A NEW PHYSICAL CONFIGURATION STRUCTURE
   */
   /* Make sure we're not going to exceed the maximum number of regions */
   if (g_physRegMax >= MAXPHYSREGION)
      {
      rstat = SH_NO_PHYS_REG;
      shErrStackPush("Physical region configuration limit exceeded");
      goto exitit;
      }
   index = g_physRegMax;

   /* Get physical struct pointer and update physical regions counter */
   physConfigPtr = &g_physConfig[g_physRegMax++];

   /*
   ** INITIALIZE PHYSICAL STRUCTURE
   */
   physConfigPtr->physIndex = index;
   physConfigPtr->physRegPtr    = NULL;
   physConfigPtr->physPixAlloc = (char *(*)(int, int))a_pixelCall;
   physConfigPtr->physPixFree  = (void (*)(int, char *))a_pixelFree;
   physConfigPtr->physHdrAlloc = (char *(*)(int, int))a_hdrCall;
   physConfigPtr->physHdrFree  = (void (*)(int, char *))a_hdrFree;
   physConfigPtr->physFillCall = (int (*)(int, char *, char *))a_fillCall;
   }

exitit:
return(rstat);
}


/*============================================================================  
**============================================================================
**
** ROUTINE: p_shPhysReserve
**
** DESCRIPTION:
**	Reserve the next available physical configuration structure.
**	Note that physical configuration structure reserving may be added in
**	the future to prevent conflicts when multiple applications access the
**	same physical memory.
**
** RETURN VALUES:
**	NON-ZERO	Pointer to unused physical structure.
**	NULL		No physical structures have been configured, or
**			they are all in use.
**
** GLOBALS REFERENCED:
**	g_physRegMax
**	g_physConfig
**
**============================================================================
*/

PHYS_CONFIG *p_shPhysReserve(void)
{
int	    i;
PHYS_CONFIG *retPtr = NULL;
 
/* Look for an available configured physical structure */
for (i=0; i < g_physRegMax; i++)
   {
   /* If the structure has not been assigned a region, then it is available */
   if (g_physConfig[i].physRegPtr == NULL)
      {
      /* We found one, so set the regPtr to indicate it's reserved, but
         not assigned to any region yet */
      g_physConfig[i].physRegPtr  = (void *)-1;
      retPtr = &g_physConfig[i];
      break;
      }
   }
return(retPtr);
}


/*============================================================================  
**============================================================================
**
** ROUTINE: p_shPhysUnreserve
**
** DESCRIPTION:
**	Detach a region from the physical configuration structure.
**	Note that physical configuration structure reserving may be added in
**	the future to prevent conflicts when multiple applications access the
**	same physical memory.
**
** RETURN VALUES:
**	N/A
**
** GLOBALS REFERENCED:
**	g_physRegMax
**	g_physConfig
**
**============================================================================
*/
void p_shPhysUnreserve
   (
   PHYS_CONFIG *a_physPtr   /* MOD: Physical configuration structure */
   )
{
shAssert(a_physPtr);

/* For now all we need to do is zero the region pointer */
a_physPtr->physRegPtr = NULL;
}


/*============================================================================  
**============================================================================
**
** ROUTINE: p_shRegPixFreeFromPhys
**
** DESCRIPTION:
**	Free a region from association with a physical pixel buffer and vector.
**
** RETURN VALUES:
**	N/A
**
**============================================================================
*/
void p_shRegPixFreeFromPhys
   (
   REGION   *a_reg	/* MOD: Region pointer */
   )
{
PHYS_CONFIG	*physPtr;

shAssert(a_reg);
shAssert(a_reg->prvt);
shAssert(a_reg->prvt->phys);
 
if (a_reg->rows_u16)
   {
   physPtr = a_reg->prvt->phys;

   /* Call the user specified free routine */
   if (a_reg->prvt->phys->physPixFree)
      {
      (*(physPtr->physPixFree))(physPtr->physIndex,
                                (char *)a_reg->prvt->pixels);
      }
   a_reg->prvt->pixels = NULL;

   /* Free the vector */
   shFree((char *)a_reg->rows_u16);
   a_reg->rows_u16 = NULL;
   a_reg->rows     = NULL;
   a_reg->nrow = 0;
   a_reg->ncol = 0;
   }
}


/*============================================================================  
**============================================================================
**
** ROUTINE: p_shRegPixNewFromPhys
**
** DESCRIPTION:
**	Create a new pixel vector and fill it out with a pixel buffer
**	that's located in physical memory.
**
** RETURN VALUES:
**	SH_SUCCESS
**	SH_PHYS_FUNC_ERR
**
**============================================================================
*/
RET_CODE p_shRegPixNewFromPhys
   (
   REGION   *a_reg,	/* MOD: Region pointer */
   int	    a_nrow,	/* IN : Number of rows in image */
   int	    a_ncol	/* IN : Number of columns in image */
   )
{
int		i;
U16		**rows;
int             totBytes;
PHYS_CONFIG	*physPtr;
RET_CODE	rstat = SH_SUCCESS;

shAssert(a_reg);
shAssert(a_reg->prvt);
shAssert(a_reg->prvt->phys);
shAssert(a_reg->prvt->phys->physPixAlloc);

physPtr = a_reg->prvt->phys;

/* Get rid of existing pixel vector first (if there is one) */
p_shRegPixFreeFromPhys(a_reg);

/* The type should already be U16, but let's be safe */
*(PIXDATATYPE *)&a_reg->type = TYPE_U16;

/* Allocate pixel vector */
rows = (U16 **)shMalloc(a_nrow*sizeof(U16 *));
if (rows == NULL)
   {shFatal("p_shRegNewFromPhys: Row vector allocation failure");}
a_reg->rows_u16 = rows;
a_reg->rows     = rows;

/* Get pointer to physical pixel buffer */
totBytes = a_nrow * a_ncol * sizeof(U16);
rows[0] = (U16 *)((*(physPtr->physPixAlloc))(physPtr->physIndex,
                                             (int )totBytes));
if (rows[0] == NULL)
   {
   shErrStackPush("User specified physical buffer allocation routine failed");
   rstat = SH_PHYS_FUNC_ERR; goto err_exit;
   }
a_reg->prvt->pixels = rows[0];

/* Fill out pixel vector with pointers to physical memory */
for(i = 0; i < a_nrow; i++)
   {
   rows[i] = (U16 *)rows[0] + i*a_ncol;
   /* rows[i] = (U16 *)((char *)rows[0] + i*a_ncol*sizeof(U16)); */
   }

a_reg->nrow = a_nrow;
a_reg->ncol = a_ncol;

err_exit:
return(rstat);
}


/*============================================================================  
**============================================================================
**
** ROUTINE: p_shRegHdrCopyFromPhys
**
** DESCRIPTION:
**	Create a virtual region header and copy a header located in 
**	physical memory into it.
**
**      NOTE: If any of the header keywords are missing or contain
**      an unexpected value, then we'll always return SH_PHYS_HDR_INV.
**      We don't want to be any more specific than that since we want
**      to continue if we're doing a "regNew -physical".  See shTclRegNew
**      in tclRegion.c.  GPS - 2/19/96
**
** RETURN VALUES:
**	SH_SUCCESS
**	SH_PHYS_FUNC_ERR
**	SH_PHYS_HDR_INV
**	SH_PHYS_HDR_NOEND
**	return values from shHdrInsertLine
**
**============================================================================
*/
RET_CODE p_shRegHdrCopyFromPhys
   (
   REGION   *a_reg,	/* MOD: Region pointer */
   int	    *a_nrow,	/* OUT: Number of rows in pixel image */
   int	    *a_ncol	/* OUT: Number of columns in pixel image */
   )
{
int	    i, iline, simple;
static char hdrLine[HDRLINESIZE];
PHYS_CONFIG *physPtr;
char	    *bufptr, *ptrHdr;
char	    **l_hdr;
int         bitpix = 0;
int         naxis = 0;
int	    b_end = 0;
RET_CODE    rstat = SH_SUCCESS;

shAssert(a_reg);
shAssert(a_reg->prvt);
shAssert(a_reg->prvt->phys);
shAssert(a_reg->prvt->phys->physHdrAlloc);

physPtr = a_reg->prvt->phys;
*a_nrow = 0;
*a_ncol = 0;

/*
** GET PHYSICAL HEADER POINTER AND MAKE SURE IT CONTAINS VALID DATA
*/
/* Get rid of existing virtual header first (if there is one) */
p_shHdrFree(&a_reg->hdr);

/* Get pointer to physical header */
bufptr = (*(physPtr->physHdrAlloc))(physPtr->physIndex,
                                   (MAXHDRLINE*HDRLINESIZE));
if (!(bufptr) || (bufptr == (char *)-1))
   {
   shErrStackPush("User specified physical hdr allocation routine failed");
   rstat = SH_PHYS_FUNC_ERR; goto err_exit;
   }

/* The first line of a header should always start with "SIMPLE", so 
   check for it.  More extensive header checks will be done later
   in this routine and by p_shFitsDelHdrLines */
if (memcmp(bufptr, "SIMPLE", 6))
   {rstat = SH_PHYS_HDR_INV; goto err_exit;}

/* Save header pointer in private structure */
physPtr->physHdrPtr = bufptr;  

/*
** CREATE NEW HEADER VECTOR AND FILL IT OUT
*/
ptrHdr  = bufptr;
iline   = 0;
for (i=0; i<MAXHDRLINE; i++) 
   {
   /* Copy line from physical header.  Then null terminate line. */
   strncpy(hdrLine, ptrHdr, HDRLINECHAR);
   hdrLine[HDRLINECHAR]   = (char)0;  

   /* If header line contains all spaces, then continue to next line */
   if (strspn(hdrLine, " ") == strlen(hdrLine))
      {continue;}

   /* Insert line into header copy.  Note: shHdrInsertLine will allocate
      header vector and lines for us */
   rstat = shHdrInsertLine(&a_reg->hdr, iline++, hdrLine);
   if (rstat != SH_SUCCESS)
      {goto err_exit;}

   /* If we just copied "END" line, we are done */
   if (!(strncmp(hdrLine, "END", 3)))
      {b_end = 1; break;}

   /* Point to next line in physical header */
   ptrHdr += HDRLINECHAR;
   }  /* for (i=0; i<MAXHDRLINE; i++)... */

if (!(b_end))
   {
#ifdef PHYSICAL_REGION_TEST
   shErrStackPush("No end card in physical header or header is too big");
   rstat = SH_PHYS_HDR_NOEND; goto err_exit;
#endif /* PHYSICAL_REGION_TEST */
   }

/*
** VALIDATE NAXIS KEYWORDS IN HEADER AND SAVE NUMBER OF ROWS AND COLUMNS.
** THEN DELETE THE USUAL LINES. 
*/
l_hdr = a_reg->hdr.hdrVec;
if (! f_ikey(&naxis, l_hdr, "NAXIS"))     /* NAXIS should be defined */
   {
   rstat = SH_PHYS_HDR_INV; goto err_exit;
   }

switch (naxis)
   {
   case 0:
      rstat = SH_PHYS_HDR_INV;     /* There is no data in this file. */
      break;
   case 1:
      *a_nrow = 1;
      if (!f_ikey(a_ncol, l_hdr, "NAXIS1"))
         {rstat = SH_PHYS_HDR_INV;}  /* There should be an NAXIS1 keyword */
      break;
   case 2:
      if (!f_ikey(a_ncol, l_hdr, "NAXIS1"))
         {rstat = SH_PHYS_HDR_INV;}  /* There should be an NAXIS1 keyword */
      if (!f_ikey(a_nrow, l_hdr, "NAXIS2"))
         {rstat = SH_PHYS_HDR_INV;}  /* There should be an NAXIS2 keyword */
      break;
   default:
      rstat = SH_PHYS_HDR_INV; 
      break;
   }
if (rstat != SH_SUCCESS)
   {goto err_exit;}
if ((*a_nrow <= 0) || (*a_ncol <= 0))
   {
   rstat = SH_PHYS_HDR_INV; 
   goto err_exit;
   }

/* Save SIMPLE */
if (! f_lkey(&simple, l_hdr, "SIMPLE"))
   {
   rstat = SH_PHYS_HDR_INV; goto err_exit;
   }

/* Let's check BITPIX for fun (plus it'll make Tom happy) */
if (! f_ikey(&bitpix, l_hdr, "BITPIX"))
   {
   rstat = SH_PHYS_HDR_INV; goto err_exit;
   }
if (bitpix != 16)
   {
   rstat = SH_PHYS_HDR_INV; goto err_exit;
   }

/* Delete the usual lines from hdr (as is done when reading normal regions) */
if ((rstat = p_shFitsDelHdrLines(l_hdr, simple)) != SH_SUCCESS)
   {goto err_exit;}

/* Normal return */
return(SH_SUCCESS);

err_exit:
p_shHdrFree(&a_reg->hdr);
return(rstat);
}


/*============================================================================  
**============================================================================
**
** ROUTINE: p_shRegReadFromPool
**
** DESCRIPTION:
**	Read in a physical region from the frame pool.  In all cases, the
**	existing region is overwritten.
**
**      This routine can also be used to simply re-construct the hdr and
**	pixel vectors if the physical region was updated by some other
**	means.  To do this, the "a_read" parameter should be 0.  This will
**	prevent the physFillCall routine from being invoked.
**
** RETURN VALUES:
**	SH_SUCCESS
**	SH_PHYS_REG_REQD
**	SH_PHYS_FUNC_ERR
**      return values from p_shHdrCopyFromPhys
**      return values from p_shRegPixNewFromPhys
**
**============================================================================
*/
RET_CODE p_shRegReadFromPool
   (
   REGION   *a_reg,	/* MOD: Region pointer */
   char	    *a_frame,	/* IN : Frame name */
   int      a_read      /* IN : DON'T read from frame pool (=0) */
   )
{
PHYS_CONFIG *physPtr;
int	    nrow, ncol;
char	    fillError[132];		    /*What size???*/
RET_CODE    rstat = SH_SUCCESS;

shAssert(a_reg);
shAssert(a_reg->prvt);

physPtr = a_reg->prvt->phys;

if (a_reg->prvt->type != SHREGPHYSICAL)
   {
   shErrStackPush("Region must be physical");
   rstat = SH_PHYS_REG_REQD;
   goto exitit;
   }

/*
** CALL FILL ROUTINE TO LOAD IMAGE AND HEADER INTO PHYSICAL MEMORY
*/
shAssert(physPtr);
if (a_read != 0) 
  {
  shAssert(physPtr->physFillCall);
  if ((*(physPtr->physFillCall))(physPtr->physIndex, a_frame, fillError))
     {
     shErrStackPush("Error from user pool read routine : %s", fillError);
     rstat = SH_PHYS_FUNC_ERR;
     goto exitit;
     }
  }

/*
** COAX PHYSICAL DATA INTO REGION
*/
/* Copy header from physical region (if valid data is there) */
rstat = p_shRegHdrCopyFromPhys(a_reg, &nrow, &ncol);
if (rstat == SH_SUCCESS)
   {
   /* Header is valid, so create the pixel vector */
   rstat = p_shRegPixNewFromPhys(a_reg, nrow, ncol);
   if (rstat != SH_SUCCESS)
      {goto exitit;}
   }
else
   {goto exitit;}

exitit:
return(rstat);
}
