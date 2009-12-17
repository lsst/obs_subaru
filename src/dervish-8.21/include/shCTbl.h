#ifndef	SHCTBL_H
#define	SHCTBL_H

/*++
 * FACILITY:	Dervish
 *
 * ABSTRACT:	Definitions for using Tables in memory.
 *
 * ENVIRONMENT:	ANSI C.
 *		shCTbl.h
 *
 * AUTHOR:	Tom Nicinski, Creation date: 30-Nov-1993
 *--
 ******************************************************************************/
/*                                                                            *
 * In order to make this header file somewhat compatible with the current     *
 * object schema generation (with makeio), the following restrictions are     *
 * adhered to:                                                                *
 *                                                                            *
 *   o   Object schemas are flagged by using typedefs.                        *
 *                                                                            *
 *          o   "typedef" must be in the first column                         *
 *                                                                            *
 *          o   "typedef" must be followed by 1 blank and "enum" or "struct". *
 *                                                                            *
 *          o   structs cannot be defined inside the object schema. They must *
 *              be defined as object schemas also.                            *
 *                                                                            *
 *          o   If "pragma" information is included, it must be contained in  *
 *              a comment that starts on the same line as the typedef name.   *
 *                                                                            *
 *          o   Arrays must specify their size as an explicit number or       *
 *              macro -- no arithmetic can be used.                           *
 *                                                                            *
 *          o   No unusual types can be used to declare elements within an    *
 *              object schema.                                                *
 *                                                                            *
 ******************************************************************************/

#include	<limits.h>

#include	"libfits.h"		/* For FITSHDUTYPE                    */

#include	"dervish_msg_c.h"		/* For RET_CODE                       */
#include	"shCSchema.h"		/* For LOGICAL                        */
#include	"shCAlign.h"		/* For U8, S8, U16, S16, U32, S32, ...*/
#include        "shChain.h"
#include	"shCArray.h"
#include	"shCFitsIo.h"		/* For MAXHDRLINE                     */
#include	"shCHdr.h"
#include	"shCUtils.h"		/* For sets (declSet, ...), SHCASE... */

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef	IMPORT
#undef	IMPORT
#endif

#ifdef	SHTBLIMP
#define	IMPORT
#else
#define	IMPORT	extern
#endif

/******************************************************************************/
/*
 * Define FITS constants.  Some are based on LIBFITS values.
 *
 *    RECORDSIZE		2880	   FITS logical record size (bytes).
 *
 * NOTE:  These definitions are here, rather than in the more natural place of
 *        shCFits.h, since shCFits.h depends on shCTbl.h. We don't want a circular
 *        dependency.
 */

#define	shFitsRecSize	(RECORDSIZE)	/* FITS logical record size (bytes)   */
#define	shFitsHdrLineSize	  80	/* FITS header line    size (bytes)   */

#define	shFitsHdrStrSize ((shFitsHdrLineSize) - 10 /* "KEYWORD = " */ - 2 /* "'" + "'" */)

/******************************************************************************/
/*
 * Define object schemas.
 *
 *   o	There are some practical limits in FITS, even though the standard may
 *	not say it.
 *
 *	   TDIMlim	the practical maximum number of dimensions that a TDIMn
 *			keyword can specify.  This limitation is imposed by the
 *			80 (shFitsHdrLineSize) byte header line size and the
 *			formatting of the line.
 */

#define	shBinTDIMlim (((shFitsHdrLineSize) - 13 /* "TDIMnnn = '(" + "'" */) / 2 /* "n," or "n)" */)

#if ((shBinTDIMlim) >= (shArrayDimLim))
#error	ARRAYs do not support all possible FITS Binary Table array dimensions
#endif

/*
 *   o	TBLCOL.  The top level structure (object schema) used to maintain a
 *	Table read in columnar format.
 */

typedef struct shTbl_TBLCOL
   {
                  int	 rowCnt;	/* Number of rows                     */
   HDR			 hdr;		/* Header                             */
   CHAIN		 fld;		/* Fields' structures      */
   }		     TBLCOL;		/*
					   pragma   CONSTRUCTOR
					 */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/*
 *   o	Additional object schemas to handle certain data types.
 *
 *	TBLHEAPDSC	Description of a variable length array in heap storage
 *			(in-memory). TBLHEAPDSC is meant to (at least partially)
 *			overlay the Binary Table `P' data type.  Therefore, the
 *			count field (cnt) must be a 4 byte integer.  The pointer
 *			field (ptr) must be adjacent and at least 4 bytes.
 *
 *	LOGICAL		Equivalent to the Binary Table `L' data type.  It is a
 *	(shCSchema.h)	boolean.  It   M U S T   be the same size as the `L'
 *			data type, namely, one byte.
 */

typedef struct	 shTbl_TblHeapDsc
   {
                  int	 cnt;		/* Number of heap elements            */
   unsigned       char	*ptr;		/* Address of data in heap            */
   }		       TBLHEAPDSC;	/*
					   pragma   SCHEMA
					 */

#if (UINT_MAX != U32_MAX)
#error TBLHEAPDSC cnt field   M U S T   be 4 bytes in size
#endif

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/*
 *   o	TBLFLD.  Table field descriptor.
 *
 *	The TBLFLD field descriptor can maintain information about both ASCII
 *	and Binary Table fields.  There is considerable overlap between the two
 *	FITS extensions to allow this sharing of one structure.  Tables are
 *	discussed in general, unless a feature is particular to only one of the
 *	Tables.  The distinctions between the Tables are:
 *
 *	   ASCII Table	Binary Table
 *	   -----------	------------
 *	   TBCOLn	EXTNAME
 *			EXTVER
 *			EXTLEVEL
 *			THEAP
 *			TDIMn
 *			TDISPn
 *			AUTHOR
 *			REFERENC
 *
 *	   TTYPEn	TTYPEn		ASCII Tables indicate that string com-
 *					parisions against TTYPEn should be case
 *					insensitive.  Binary Tables do not
 *					discuss this issue.
 *
 *	   TFORMn	TFORMn		ASCII Tables used FORTRAN-77 format
 *					specifiers without a repeat count.
 *					Binary Tables use their own field
 *					descriptions.
 *
 *	Since the Table Record Storage Area (RSA) contains one or more rows, all
 *	data is stored in an array, even if it is a scalar.
 *
 *	TBLCOL data is maintained by ARRAYs.  When reading about ARRAYs (see
 *	shCArray.h), keep in mind that when a FITS Table is read into a Dervish
 *	Table, an additional array dimension, namely the Table row is added to
 *	the Dervish Table. This is the slowest varying (first) index.  A Binary
 *	Table scalar field becomes a 1-dimensional array in a Dervish Table.  A
 *	Binary Table field containing an n-dimensional array becomes an (n+1)-
 *	dimensional array in the Dervish Table.
 *
 *	FITS ASCII and Binary Tables differ when it comes to arrays as data
 *	within a field.  ASCII Tables (TABLE HDUs) do not permit arrays;  a
 *	field can only hold a scalar value (or a character string).  Binary
 *	Tables (BINTABLE HDUs), on the other hand, permit a field to contain
 *	both a scalar or an array of values.
 *
 *	Since the ASCII Table is more restrictive, it is still possible to
 *	discuss them as if they allowed 1-dimensional arrays (to permit charac-
 *	ter strings to be handled). In the discussion below, for an ASCII Table,
 *	TFORMn's element count would always be one (1).  Additionally, even
 *	though ASCII Tables do not permit the TDIMn keyword, allowing its
 *	limited use (only the row index and character string index are allowed)
 *	permits TBLCOL Tables to remain generic.
 *
 *	Although a Table field form (TFORMn keyword) can indicate a scalar
 *	value (a element count of one (1) without a corresponding TDIMn
 *	specification) or a multidimension array,   A L L   field values are
 *	stored as an array in the TBLCOL format.  The ARRAY object schema
 *	describes how data is stored.  At a minimum, the Table row number is
 *	the slowest varying index, as in this C code example
 *
 *	     myField[row][d ][d ]...[d ]
 *	                   1   1      c
 *
 *	where c is the number of dimensions specified by TDIMn.
 *
 *	The notation used throughout follows C's row-major ordering of array
 *	elements, that is, the last subscript varies most rapidly.  FITS'
 *	column-major ordering of array dimensions (where the first subscript
 *	varies most rapidly) specified by TDIMn is reversed under Dervish. In the
 *	example above:
 *	                                th
 *	     d   is  TDIMn's (c - i + 1)   subscript
 *	      i
 *
 *	where i and TDIMn's subscripts are 1-indexed:
 *
 *	     TDIMn = '(s ,s , ...,s )'
 *	                1  2       c
 *
 *	The analogy to C arrays applies only to notation other limitations with
 *	respect to the notation also apply). Since the TBLCOL format is generic,
 *	array dimensions are unknown until the file is read.  Therefore, multi-
 *	dimension arrays are stored in such a manner that regular C syntax can
 *	be used to access these arrays without knowing their dimensions at com-
 *	pilation-time (thus, they're not stored the same a C arrays).  The C
 *	concept of an array name representing a pointer to the array is used to
 *	accomplish this type of addressing.  For n-dimensional arrays, n sets
 *	of pointers are used.  The first pointer is associated with the array
 *	name.  It points to the data in a 1-dimensional array or to the second
 *	set of pointers.  These pointers in turn point to the data in a 2-dim-
 *	ensional array or to the third set of pointers; and so forth.
 *
 *	Arrays are controlled by the following elements:
 *
 *	   array->dimCnt  the number of dimensions from the TDIMn keyword plus 1
 *			for the row indices (pointers).
 *
 *	   array->dim	the array dimensions.  array.dim[0] is the number of
 *			rows, namely, NAXIS2.  NOTE:  These dimensions are in
 *			row-major order, the reverse of FITS' column-major order
 *			in the TDIMn keyword value.
 *
 *	See shCArray.h for a detailed description of how an ARRAY organizes data.
 */

typedef enum	shTblFld_keywords  /* The FITS header keywords present        */
   {				/* A "set" is used with these; so, hopefully, */
   SH_TBL_HEAP = 0,		/* ... there won't be more keywords than the  */
   SH_TBL_TTYPE,			/* ... number of bits in an unsigned long int!*/
   SH_TBL_TUNIT,
   SH_TBL_TSCAL,
   SH_TBL_TZERO,
   SH_TBL_TNULLSTR,
   SH_TBL_TNULLINT,
   SH_TBL_TDISP,
   SH_TBL_TDIM,
   SH_TBL_TFORM			/* In TBLFLD prvt area                        */
   }		  TBLFLD_KEYWORDS;

/*
 * Not all fields are valid for all Table types.  The Table reading code will
 * determine which fields are supported for a particular Table type.  Some field
 * are:
 *
 *   o	TFORM describes how the field was read in (or, possibly in the future,
 *	will be written out).  It does   N O T   describe the in-memory format
 *	of the data.
 *
 *	   o	ascFldWidth, ascFldDecPt, and ascFldFmt are used to make up a
 *		FORTRAN-77 FORMAT specification used by an ASCII Table's TFORM
 *		keyword.
 *
 *		ascFldOff is the offset (0-indexed) of the field within an ASCII
 *		Table.  It is mainly used when reading in an ASCII Table and
 *		writing it out.  If ascFldOff is set to -1, then the ASCII Table
 *		writer will begin the field after the previous field.
 *		Explicit offset (0-indexed) values can used to overlap fields,
 *		but users should be careful.
 *
 *	TFORM is valid when the Table is read in from a FITS file.  But, for
 *	Tables created from scratch, SH_TBL_TFORM must be set in Tpres for it to
 *	be useful.
 *
 *   o	array->dim and array->dimCnt are always valid.  If SH_TBL_TDIM is not set,
 *	it only indicates that TDIMn was not in the header, NOT that array->dim
 *	is invalid.
 *
 *   o	TTYPE, TUNIT, and TDISP have preallocated storage.  Although this is
 *	usually wasteful, the complexities of malloc'ing space are not worth
 *	the savings in storage (consider that each string would need to have
 *	space malloc'ed -- this would permit users to free each individually
 *	to remalloc space when changing the character string value.
 *
 * Additionally, the TBLFLD structure describes other aspects of the field
 * (beyond those that came from the FITS header):
 *
 *   o	Tpres indicates which keywords were present in the FITS header, and thus
 *	are valid in the TBLFLD structure.  Tpres is a set (see shCUtils.h).
 *
 *   o	fld describes the in-memory copy of the field.
 *
 *   o	data points to the start of the data and the root of the hierarchy of
 *	array pointers.
 *
 *   o	heap storage information and data.
 *
 *   o	private data which is used temporarily for reading and writing.
 *
 *	   o	schemaCnt is the number of objects of array->data.schemaType, as
 *		opposed to the atomic data elements which comprise that type.
 *		This count is for one row in the Table.
 *
 *	   o	elemCnt is the number of primary data types that comprise the
 *		array size in one Table row.  It is similar to schemaCnt, except
 *		for data types such as strings, where schemaCnt reflects the
 *		number of strings while elemCnt reflects the total number of
 *		characters in all strings.
 *
 *	   o	dataType is the original data type read from the FITS file; it
 *		is used mainly for parsing error messages.
 *
 *	   o	signFlip is an exclusive OR mask used to quickly flip the sign
 *		bit of a datum.  It   M U S T   be a byte that is either 0 (no
 *		sign bit flip, TBLFLD_SIGNNOFLIP) or 0x80 (sign flip, TBLFLD_
 *		SIGNFLIP). If this assumption changes, then the code in shFits.c
 *		must also reflect that change.
 */

typedef struct	 shTbl_TBLFLDtform	/* TFORM keyword information          */
   {
                  int	 ascFldWidth;	/* F77 format: field width            */
                  int	 ascFldDecPt;	/* F77 format: decimal point position */
                  char	 ascFldFmt;	/* F77 format: FORMAT specifier       */
                  int	 ascFldOff;	/* Field offset (0-indexed) in row    */
   }		       TBLFLD_TFORM;	/*
					   pragma   SCHEMA
					 */

#define	TBLFLD_SIGNFLIP   0x80		/* Sign bit is not flipped            */
#define	TBLFLD_SIGNNOFLIP 0x00		/* Sign bit is     flipped            */

typedef struct	 shTbl_TBLFLDprvt	/* "Private" data                     */
   {
   unsigned       char	*datumPtr;	/* Temporary used for filling data    */
                  int	 schemaCnt;	/* # array->data.schemaType objects   */
                  int	 elemCnt;	/* TFORMn: element count              */
                  char	 dataType;	/* TFORMn: original data type (4 msgs)*/
   unsigned	  char	 signFlip;	/* Flip "signedness" of data?         */
   TBLFLD_TFORM		 TFORM;		/* TFORMn: ASCII Table information    */
   }		       TBLFLD_PRVT;	/*
					  pragma    SCHEMA
					 */

/*
 * NOTE: To comply with the behavior of makeio (at the time of writing this
 *       routine):
 *
 *	   o	The TTYPE, TUNIT, and TDISP array sizes must be hardwired as an
 *		explicit number.  They must be (shFitsHdrStrSize) + 1.
 *
 *	   o	Tpres should be declared as "declSet (shTblFld_keywords)". It is
 *		assumed declSet is a macro that expands to "unsigned long int".
 */

#if  ( (shFitsHdrStrSize)       !=      68)	/* Since makeio doesn't handle*/
#error  shFitsHdrStrSize expected to be 68	/* any expressions for array  */
#endif				/*       |	 * dimensions, ...            */			
				/*       |	                              */
typedef struct	 shTbl_TBLFLD	/*       `-.	                              */
   {				/*         |	                              */
   unsigned long  int	 Tpres;	/*         |	 * Txxx keyword present?  ### */
                  char	 TTYPE   [69];/*<--'	 *                        ### */
                  char	 TUNIT   [69];/*<--'	 *                        ### */
                  char	 TDISP   [69];/*<--'	 *                        ### */
                  char	 TNULLSTR[69];/*<--'	 *                        ### */
                  int	 TNULLINT;
   double		 TSCAL;
   double		 TZERO;
   ARRAY		*array;		/* Information about data area        */
   ARRAY_DATA		 heap;		/* Heap storage information           */
   TBLFLD_PRVT		 prvt;		/* Private data                       */

   }		       TBLFLD;		/*
					   pragma   CONSTRUCTOR
					 */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/*
 * Describe how the TFORMn keyword should be parsed/interpreted.
 *
 *   o	dataType is the data type, based on the Table type, supported in TFORMn.
 *
 *   o	ascFldWidthDef, ascFldDecPtDef, and ascFldFmtDef are used to generate
 *	an ASCII Table TFORMn keyword when writing out the FITS file. These are
 *	defaults, only useful for non-ASCII Tables.
 *
 *	If ascFldWidthDef or ascFldDecPtDef is zero (0), it indicates that it
 *	is not used in the formation of the TFORMn keyword.
 *
 * NOTE:  It is important to keep this list of mappings consistent with other
 *        code and declarations.  In particular (but not limited to):
 *
 *           shCAlign.h		shAlign data types are mapped to object schema
 *				types.
 *
 *           shAlign.c		shAlignSchemaToAlignMap () maps certain object
 *				schemas to shAlign types.
 *
 *				   o	`L' <--> LOGICAL <--> shAlignType_U8
 *
 * NOTE: The ordering of entries in the mappings are important.  They are also
 *       used to determine the TFORM data type given an object schema/alignment
 *       type. Place the object schema/alignment types that are to translate to
 *       a TFORM data type near the start of the map table. This still does not
 *       prevent the need to test for particular object schema types explicitly.
 *
 *	   o	For example, place "compound" data types, such as a heap des-
 *		criptor (TFORM data type of 'P') near the end, since their
 *		alignment type in the map table is SHALIGNTYPE_S32, but an
 *		alignment type of SHALIGNTYPE_S32 (equivalent to an object
 *		schema type of INT) should be mapped to a TFORM data type of
 *		'J' (Binary Table).  The object schema type TBLHEAPDSC needs
 *		to be tested for explicitly before going through a mapping.
 *
 *	   o	Data types marked with \* +++ *\ comments indicate a data type
 *		defined as a Dervish extension (not part of the FITS Standard).
 */

struct shTbl_SuppData			/* Supported data types               */
   {
                  char	 dataType;	/* FITS file TFORM data type          */
                  int	 dataTypeComply;/* Data type complies with FITS Std?  */
   SHALIGN_TYPE		 fldType;	/* In-memory       data type          */
                  int	 fldTNULLok;	/* .--------------- TNULLn allowed?   */
                  int	 fldTSCALok;	/* |  .------------ TSCALn allowed?   */
                  int	 fldTZEROok;	/* |  |  .--------- TZEROn allowed?   */
                  int	 ascFldWidthDef;/* |  |  |   .--------- Default field width */
                  int	 ascFldDecPtDef;/* |  |  |   |   .----- Default decimals    */
                  char	 ascFldFmtDef;	/* |  |  |   |   |   .- Default F77 FORMAT  */
   };					/* |  |  |   |   |   |             */
					/* |  |  |   |   |   |             */
IMPORT struct shTbl_SuppData	 p_shTblDataTypeBINTABLEmap[]
#ifdef	SHTBLIMP			/* V  V  V  V   V   V   V             */
   = {	{ 'H', 0, SHALIGNTYPE_S8,	   1, 1, 1,  4,  0, 'I' },	/* +++*/
	{ 'I', 1, SHALIGNTYPE_S16,	   1, 1, 1,  6,  0, 'I' },
	{ 'J', 1, SHALIGNTYPE_S32,	   1, 1, 1, 11,  0, 'I' },
	{ 'B', 1, SHALIGNTYPE_U8,	   1, 1, 1,  4,  0, 'I' },
	{ 'U', 0, SHALIGNTYPE_U16,	   1, 1, 1,  6,  0, 'I' },	/* +++*/
	{ 'V', 0, SHALIGNTYPE_U32,	   1, 1, 1, 11,  0, 'I' },	/* +++*/
	{ 'A', 1, SHALIGNTYPE_S8,	   0, 1, 1,  1,  0, 'A' },
	{ 'E', 1, SHALIGNTYPE_FL32,	   0, 1, 1, 17,  8, 'E' },
	{ 'D', 1, SHALIGNTYPE_FL64,	   0, 1, 1, 25, 17, 'D' },
	{ 'L', 1, SHALIGNTYPE_U8,	   0, 1, 1,  1,  0, 'I' },
/* ###	{ 'X', 1, SHALIGNTYPE_U8,	   0, 1, 1,  0,  0, 'I' },	   ###*/
	{ 'C', 1, SHALIGNTYPE_FL32,	   0, 1, 1, 17,  8, '\000' },
	{ 'M', 1, SHALIGNTYPE_FL64,	   0, 1, 1, 25, 17, '\000' },
	{ 'P', 1, SHALIGNTYPE_S32,	   0, 1, 1, 11,  0, '\000' },
	{ '\000' }			/* |  |  |   |   |   |	* List end    */
   }					/* |  |  |   |   |   |	              */
#endif					/* |  |  |   |   |   |	              */
    ;					/* |  |  |   |   |   |	              */
IMPORT struct shTbl_SuppData  p_shTblDataTypeTABLEmap[] /*   |	              */
#ifdef	SHTBLIMP			/* V  V  V   V   V   V	              */
   = {	{ 'A', 1, SHALIGNTYPE_S8,	   0, 0, 0,  0,  0, 'A' },
	{ 'I', 1, SHALIGNTYPE_S32,	   1, 1, 1, 11,  0, 'I' },
	{ 'F', 1, SHALIGNTYPE_FL32,	   1, 1, 1, 17,  8, 'F' },
	{ 'E', 1, SHALIGNTYPE_FL32,	   1, 1, 1, 17,  8, 'E' },
	{ 'D', 1, SHALIGNTYPE_FL64,	   1, 1, 1, 25, 17, 'D' },
	{ '\000' }			/* |  |  |   |   |   |	* List end    */
   }					/* |  |  |   |   |   |	              */
#endif					/* |  |  |   |   |   |	              */
    ;					/* |  |  |   |   |   |	              */

/******************************************************************************/

/******************************************************************************/
/*
 * API for FITS Tables.
 *
 * Public  Function		Explanation
 * ----------------------------	------------------------------------------------
 *   shTblColCreate		Create a skeleton TBLCOL Table.
 *   shTblFldAdd		Add a field to a TBLCOL Table (to end of fields)
 *   shTblFldLoc		Locate a field based on position or name.
 *   shTblFldHeapAlloc		Allocate space for  heap storage.
 *   shTblFldHeapFree		Free     space from heap storage.
 *   shTblTBLFLDlink		Allocate and link a TBLFLD to a field (ARRAY).
 *   shTblTBLFLDclr		Clear TBLFLD member (make it not present).
 *   shTblTBLFLDsetWithAscii	Set a TBLFLD member with an ASCII value.
 *   shTblTBLFLDsetWithDouble	Set a TBLFLD member with a double value.
 *   shTblTBLFLDsetWithInt	Set a TBLFLD member with a signed integer value.
 *
 * Private Function		Explanation
 * ----------------------------	------------------------------------------------
 * p_shTblFldGen		Read a Table header and fill an ARRAY list.
 * p_shTblFldDel		Delete a field from a TBLCOL (ARRAY list).
 * p_shTblFldDelByIdx		Delete a field from a TBLCOL (ARRAY list).
 * p_shTblFldParseTFORMasc	Parse TFORM keyword in FITS ASCII  Table header.
 * p_shTblFldParseTFORMbin	Parse TFORM keyword in FITS Binary Table header.
 * p_shTblFldParseTDIM		Parse TDIM  keyword in FITS        Table header.
 * p_shTblFldHeapCnt		Count number of heap elements in a field.
 * p_shTblFldSignFlip		Return info about flipping "signedness" of data.
 *   shTblcolNew		Object schema constructor for TBLCOL.
 *   shTblcolDel		Object schema destructor  for TBLCOL.
 *   shTblfldNew		Object schema constructor for TBLFLD.
 *   shTblfldDel		Object schema destructor  for TBLFLD.
 */

RET_CODE	   shTblColCreate		(int rowCnt, int fldCnt, int tblFldPres, TBLCOL **tblCol);
RET_CODE	   shTblFldAdd			(TBLCOL *tblCol, int tblFldPres, ARRAY **array, TBLFLD **tblFld, int *tblFldIdx);
RET_CODE	   shTblFldLoc			(TBLCOL *tblCol, int tblFldWanted, char *tblFldName, int tblFldNamePos,
						 SHCASESENSITIVITY tblFldNameCase, ARRAY **array, TBLFLD **tblFld, int *tblFldIdx);
RET_CODE	   shTblFldHeapAlloc		(ARRAY *array, char *schemaName, long int extraCnt, unsigned char **extra);
RET_CODE	   shTblFldHeapFree		(ARRAY *array);
RET_CODE	   shTblTBLFLDlink		(ARRAY *array, TBLFLD **tblFld);
RET_CODE	   shTblTBLFLDclr		(ARRAY *array, TBLFLD_KEYWORDS tblKeyFld,                   TBLFLD **tblFld);
RET_CODE	   shTblTBLFLDsetWithAscii	(ARRAY *array, TBLFLD_KEYWORDS tblKeyFld, char  *tblKeyVal, TBLFLD **tblFld);
RET_CODE	   shTblTBLFLDsetWithDouble	(ARRAY *array, TBLFLD_KEYWORDS tblKeyFld, double tblKeyVal, TBLFLD **tblFld);
RET_CODE	   shTblTBLFLDsetWithInt	(ARRAY *array, TBLFLD_KEYWORDS tblKeyFld, int    tblKeyVal, TBLFLD **tblFld);

RET_CODE	 p_shTblFldGen			(CHAIN *fldList, HDR_VEC *hdrVec, FITSHDUTYPE hduType, int *TFIELDS, int *NAXIS1,
						 int *NAXIS2);
RET_CODE	 p_shTblFldDelByIdx		(CHAIN *fldList, unsigned int fldIdx);
RET_CODE	 p_shTblFldParseTFORMasc	(HDR_VEC *hdrVec, TBLFLD *tblFld, int fldIdx, int *fldWidth,
						 struct shTbl_SuppData *dataTypeMap, int *dataTypeIdx);
RET_CODE	 p_shTblFldParseTFORMbin	(HDR_VEC *hdrVec, TBLFLD *tblFld, int fldIdx, int *fldWidth,
						 struct shTbl_SuppData *dataTypeMap, int *dataTypeIdx);
RET_CODE	 p_shTblFldParseTDIM		(HDR_VEC *hdrVec, TBLFLD *tblFld, int fldIdx);
RET_CODE	 p_shTblFldHeapCnt		(ARRAY *array, long int *heapNonZero, long int *heapCntAll, long int *heapCntMax);
RET_CODE	 p_shTblFldSignFlip		(TYPE schemaType, TYPE *schemaTypeFlip, double *TSCAL, double *TZERO);
TBLCOL		*  shTblcolNew			(void);
void		   shTblcolDel			(TBLCOL *tblCol);
TBLFLD		*  shTblfldNew			(void);
void		   shTblfldDel			(TBLFLD *tblFld);

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif	/* ifndef SHCTBL_H */
