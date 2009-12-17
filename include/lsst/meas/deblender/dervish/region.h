#ifndef	REGION_H
#define REGION_H

#include "dervish_msg_c.h"
#include "shCAlign.h"
#include "shCHdr.h"

#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

/*****************************************************************************
******************************************************************************
**
** FILE:
**      region.h
**
** ABSTRACT:
**      This file contains all necessary definitions for the public
**	bit of the region structure.
**
** ENVIRONMENT:
**      ANSI C.
**
** AUTHOR:
**      Creation date: May 07, 1993
**      
**
*/
#include <limits.h>

#define DEFSUBREGSIZE 1000 /* #of region pointers */
#define DEFSUBMASKSIZE 1000 /* #of mask pointers */

/*****************************************************************************/
/*
 * Types needed by region code 
 *
 * Possible pixel data types. If you ask for u16, the old interface
 * via region->rows is still available
 */
typedef enum {
   TYPE_U8   = (1 << SHALIGNTYPE_U8),	/* unsigned char */
   TYPE_S8   = (1 << SHALIGNTYPE_S8),	/* signed char */
   TYPE_U16  = (1 << SHALIGNTYPE_U16),	/* unsigned short */
   TYPE_S16  = (1 << SHALIGNTYPE_S16),	/* signed short */
   TYPE_U32  = (1 << SHALIGNTYPE_U32),	/* unsigned long */
   TYPE_S32  = (1 << SHALIGNTYPE_S32),	/* signed long */
   TYPE_FL32 = (1 << SHALIGNTYPE_FL32),	/* single precision floating point */
   TYPE_FL64 = (1 << SHALIGNTYPE_FL64)	/* double precision floating point */
} PIXDATATYPE;

/*
 * flags defining what sort of subregion is desired
 */
typedef enum {
   NO_FLAGS = 0,			/* no flags */
   READ_ONLY = 01, READ_WRITE = 0,	/* is region R only or R/W (default)*/
   COPY_MASK = 02,			/* make mask a submask of parent? */
   ENLARGE_EXACT = 04,       /* only allow exact enlargements to be made */
   COPY_HEADER_DEEP = 010		/* must make a copy of header */
  } REGION_FLAGS;

/*****************************************************************************/
/*
 * and finally the actual MASKs and REGIONs
 */
typedef struct mask{
   char *name;             /* identifying name */
   int nrow;               /* number of rows in mask */
   int ncol;               /* number of columns in mask */
   unsigned char **rows;   /* pointer to pointers to rows */
   int    row0,col0;       /* location of LLH corner of child in parent */
   struct mask_p *prvt;    /* information private to the pipeline */
} MASK;					/*
					  pragma NOCONSTRUCTOR
					  pragma USER
					 */

typedef struct region{
   char		  *name;	/* a unique identifier */
   int		  nrow;		/* number of rows in image */
   int		  ncol;		/* number of columns in image */
   PIXDATATYPE    type;		/* pixel data type */
   U16	          **rows;	/* pointer to pointers to rows */
   U8             **rows_u8;
   S8             **rows_s8;
   U16 	          **rows_u16;
   S16 	          **rows_s16;
   U32            **rows_u32;
   S32		  **rows_s32;
   FL32	          **rows_fl32;
   MASK		  *mask;	/* associated bad pixel mask */
   int		  row0,col0;	/* location of LLH corner of child in parent */
   HDR		  hdr;		/* header */
   struct region_p *prvt;	/* information private to the pipeline */
} REGION;				/*
					  pragma NOCONSTRUCTOR
					  pragma USER
					  pragma LOCK
					 */

typedef struct {
  unsigned int  modCntr;	/* number of times region has been marked dirty*/
  unsigned int  crc;		/* CRC of image in region the LAST time it was taken*/
  REGION       *parent;		/* Address of region's parent, if a subregion
				Address of region itself if NOT a subregion*/ 
  unsigned char isSubReg;	/* 0 iff not a subregion, else non-zero*/
  unsigned char isPhysical;	/* 0 iff not a physical region, else non-zero*/
  unsigned char pxAreContiguous; /* 0 iff pixels are NOT allocated contiguously*/
  unsigned char hasHeader;	/* 0 iff region has no header*/
  unsigned int  headerModCntr;	/* Number of changes made to the header*/ 
  unsigned char hasMask;	/* 0 iff region has no mask*/
  unsigned int  nSubReg;  /* Number of subregions for this region 0 if a sub-region*/
  REGION *      subRegs[DEFSUBREGSIZE];   /* Array of nSubRegions subRegions*/
  int		physicalRegNum; /* Physical region index number (if phys reg) */
} REGINFO;				/*
					  pragma SCHEMA
					  pragma LOCK
					 */

typedef struct {
  unsigned int  crc;		/* CRC of image in mask the LAST time it was taken*/
  MASK          *parent;	/* Address of mask's parent, if a submask
				   Address of mask itself if NOT a submask*/ 
  unsigned char isSubMask;	/* 0 if not a submask, else non-zero*/
  unsigned char pxAreContiguous; /* 0 if pixels are NOT allocated contiguously*/
  unsigned int  nSubMask;     /* Num of submasks for this mask 0 if a sub-mask*/
  MASK *        subMasks[DEFSUBMASKSIZE];    /* Array of nSubMasks subMasks*/
} MASKINFO;				/*
					  pragma SCHEMA
					  pragma LOCK
					 */
/*****************************************************************************/
/*
 * Prototypes
 */
MASK *shMaskNew(
	      const char *name,		/* name of MASK; non-null (but "" OK)*/
	      int nrow,			/* number of rows */
	      int ncol			/* number of columns */
	      );
MASK *shSubMaskNew(
		 const char *name,	/* name of MASK; non-null (but "" OK)*/
		 const MASK *mask,	/* parent MASK */
		 int nrow,		/* number of rows */
		 int ncol,		/* number of columns */
		 int row0,		/* starting row number in parent */
		 int col0,		/* and starting column number */
		 REGION_FLAGS flags	/* flags as defined above */
		 );
REGION *shRegNew(
		  const char *name,	/* non-NULL name of REGION ("" is OK)*/
		  int nrow,		/* number of rows */
		  int ncol,		/* number of columns */
		  PIXDATATYPE type	/* type of data desired */
		  );
REGION *shSubRegNew(
		     const char *name,	/* non-NULL name of REGION ("" is OK)*/
		     const REGION *mask, /* parent REGION */
		     int nrow,		/* number of rows */
		     int ncol,		/* number of columns */
		     int row0,		/* starting row number in parent */
		     int col0,		/* and starting column number */
		     REGION_FLAGS flags /* flags as defined above */
		     );
/*
 * Fill out the MASKINFO structure and return its address.
 */

RET_CODE shMaskInfoGet (const MASK *mask, MASKINFO **maskInfo);

/*
 * Create a new region based on an existing region
 */
REGION *shRegNewFromReg(
  const REGION  *a_reg,     /* IN : Source region pointer */
  const char    *a_name,    /* IN : ASCII name for new region */
  PIXDATATYPE   a_type,     /* IN : Pixel data type for new region */
  REGION_FLAGS  a_flags,    /* IN : Flags for copied header and or mask */
  RET_CODE      *a_rstat    /* OUT: Return status */
  );

/*
 * return a pointer to the parent of the given region
 */
REGION *parentRegion(const REGION *region);

/*
 * Clear a mask or region
 */
void shMaskClear(MASK *mask);
void shRegClear(REGION *region);

/*
 * Return 1 if the given region/mask may be modified, or 0 if READ_ONLY
 * (or SDSS_ERROR is the given pointer is NULL)
 */
int isRWRegion(const REGION *region);
int isRWMask(const MASK *mask);

/*
 * deletion of regions and masks
 */
RET_CODE shRegDel(REGION *region);
RET_CODE shMaskDel(MASK *mask);

/*
 * Fill out the REGINFO structure and return its address.
 */

RET_CODE shRegInfoGet (const REGION *reg, REGINFO **regInfo);
/*
 * Operations on masks
 */
int shMaskAndEqualsValue(MASK *mask, const int val);

/*
 * Public routines that are called by other routines, not the user.
 */
void p_shRegCntrIncrement(REGION *a_reg);
void p_shRegPtrZero(REGION *reg);
void p_shRegVectorFree(REGION *reg);
void p_shRegRowsFree(REGION *reg);
int  p_shRegVectorGet(REGION *reg, int nrow, PIXDATATYPE type);
int  p_shRegRowsGet(REGION *reg, int nrow, int ncol, PIXDATATYPE type);
void p_shMaskVectorFree(MASK *mask);
void p_shMaskRowsFree(MASK *mask);
void p_shMaskVectorGet(MASK *mask, int nrow);
void p_shMaskRowsGet(MASK *mask, int nrow, int ncol);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif

