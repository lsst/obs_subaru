#ifndef	SHCALIGN_H
#define	SHCALIGN_H

/*++
 * FACILITY:	Dervish
 *
 * ABSTRACT:	Memory alignment at run-time.
 *
 * ENVIRONMENT:	ANSI C.
 *		shCAlign.h
 *
 * AUTHOR:	Tom Nicinski, Creation date: 17-Nov-1993
 *--
 ******************************************************************************/

#include	<limits.h>

#include	"shCSchema.h"

#ifdef	IMPORT
#undef	IMPORT
#endif
#ifdef	IMPORTinit
#undef	IMPORTinit
#endif

#ifdef	shAlignIMP
#define	IMPORT
#define	IMPORTinit(varType, varName, initVal)\
		       varType varName = initVal
#else
#define	IMPORT	extern
#define	IMPORTinit(varType, varName, initVal)\
		extern varType varName
#endif

#ifdef	__cplusplus
extern "C"
   {
#endif

/******************************************************************************/
/*
 * Define the types that alignment information will be provided for.
 *
 *   o	The alignment types correspond to the data types 
 */

typedef enum	 shAlignType
   {
   SHALIGNTYPE_unknown  = 0,
   SHALIGNTYPE_U8,
   SHALIGNTYPE_S8,
   SHALIGNTYPE_U16,
   SHALIGNTYPE_S16,
   SHALIGNTYPE_U32,
   SHALIGNTYPE_S32,
   SHALIGNTYPE_FL32,
   SHALIGNTYPE_FL64,
   SHALIGNCNT,				/* M U S T   be after last primitive  */

   /*
    * Extend the primitive data types to certain known data types.
    */

   SHALIGNTYPE_ptrData = SHALIGNCNT,	/* Don't allow "unused" slot in enum  */
   SHALIGNCNTALL			/* M U S T   B E   L A S T            */
   }		 SHALIGN_TYPE;		/*
					   pragma   SCHEMA
					 */

/*
 *   o	Define standard typedefs used throughout Dervish.
 *
 *	   o	The shAlign primitive data types are cross-referenced to the
 *		Dervish object schema primitive data types through the object
 *		schema names.
 *
 *	   o	Define their minimum and maximum values (as appropriate).
 *
 *	   o	Determine whether they match the Dervish data type size standard.
 *
 *	   o	When changing the equivalent Dervish object schema data type in
 *		the table below, make sure the change is compatable with
 *		shAlign.c.  For example, shAlignSchemaToAlignMap () allows
 *		for synonym object schemas by testing for them explicitly
 *		(for example, "SCHAR" and "CHAR" both map to shAlignTYPE_U8).
 *									   ###
 *   NOTE:	The SGI C++ compiler (CC) does not permit tokenization	   ###
 *		stringization operators if the -cckr switch is used.	   ###
 *		When this restriction is lifted, restore the macros to	   ###
 *		use them:						   ###
 *									   ###
 *	#define	shAlignTypedef(typeName, dervishName, schemaName)\
 *	typedef	typeName                         dervishName;\
 *	IMPORTinit (char, *shAlignTypeName_   ## dervishName, #typeName);\
 *	IMPORTinit (char, *shAlignDervishName_  ## dervishName, #dervishName);\
 *	IMPORTinit (char, *shAlignSchemaName_ ## dervishName,  schemaName)
 *
 *	shAlignTypedef (unsigned       char,	unknown, "UNKNOWN"); \* typeName used to appease ACC's "token-less macro arg" err *\
 *	shAlignTypedef (unsigned       char,	U8,      "UCHAR");
 *	shAlignTypedef (  signed       char,	S8,      "SCHAR");
 *	shAlignTypedef (unsigned short int,	U16,     "USHORT");
 *	shAlignTypedef (         short int,	S16,      "SHORT");
 *	shAlignTypedef (unsigned       int,	U32,     "UINT");
 *	shAlignTypedef (               int,	S32,      "INT");
 *	shAlignTypedef (float,			FL32,    "FLOAT");
 *	shAlignTypedef (double,			FL64,    "DOUBLE");
 *	shAlignTypedef (               char *,	ptrData, "PTR");
 *
 *	#undef	shAlignTypedef
 *
 */

#define	shAT(typeName, dervishName, schemaName, typeText, dervishText, typeVar, dervishVar, schemaVar)\
typedef	typeName                         dervishName;\
IMPORTinit (char, *typeVar,   typeText);\
IMPORTinit (char, *dervishVar,  dervishText);\
IMPORTinit (char, *schemaVar, schemaName)

shAT (unsigned       char,   unknown, "UNKNOWN", "unsigned char",      "unknown", shAlignTypeName_unknown, shAlignDervishName_unknown, shAlignSchemaName_unknown);
shAT (unsigned       char,   U8,      "UCHAR",   "unsigned char",      "U8",      shAlignTypeName_U8,      shAlignDervishName_U8,      shAlignSchemaName_U8);
shAT (  signed       char,   S8,      "SCHAR",     "signed char",      "S8",      shAlignTypeName_S8,      shAlignDervishName_S8,      shAlignSchemaName_S8);
shAT (unsigned short int,    U16,     "USHORT",  "unsigned short int", "U16",     shAlignTypeName_U16,     shAlignDervishName_U16,     shAlignSchemaName_U16);
shAT (         short int,    S16,      "SHORT",           "short int", "S16",     shAlignTypeName_S16,     shAlignDervishName_S16,     shAlignSchemaName_S16);
shAT (unsigned       int,    U32,     "UINT",    "unsigned int",       "U32",     shAlignTypeName_U32,     shAlignDervishName_U32,     shAlignSchemaName_U32);
shAT (               int,    S32,      "INT",             "int",       "S32",     shAlignTypeName_S32,     shAlignDervishName_S32,     shAlignSchemaName_S32);
shAT (float,                 FL32,    "FLOAT",   "float",              "FL32",    shAlignTypeName_FL32,    shAlignDervishName_FL32,    shAlignSchemaName_FL32);
shAT (double,                FL64,    "DOUBLE",  "double",             "FL64",    shAlignTypeName_FL64,    shAlignDervishName_FL64,    shAlignSchemaName_FL64);
shAT (               char *, ptrData, "PTR",              "char *",    "ptrData", shAlignTypeName_ptrData, shAlignDervishName_ptrData, shAlignSchemaName_ptrData);

#undef	shAT

#define	U8_MIN	0
#define	U8_MAX	UCHAR_MAX
#define	S8_MIN	SCHAR_MIN
#define	S8_MAX	SCHAR_MAX
#define	U16_MIN	0
#define	U16_MAX	USHRT_MAX
#define	S16_MIN	SHRT_MIN
#define	S16_MAX	SHRT_MAX
#define	U32_MIN	0
#define	U32_MAX	UINT_MAX
#define	S32_MIN	INT_MIN
#define	S32_MAX	INT_MAX
#define FL32_MAX 1.7976931348623157E+308 /*largest possible magnitude */
#define FL32_MIN 2.2250738585072014E-308 /*smallest absolute magnitue */


#if (( U8_MIN !=           0) || ( U8_MAX !=        255U))
#error "unsigned char" expected to be 1 byte
#endif
#if (( S8_MIN !=        -128) || ( S8_MAX !=        127))
#error "signed char" expected to be 1 byte
#endif
#if ((U16_MIN !=           0) || (U16_MAX !=      65535U))
#error "unsigned short int" expected to be 2 bytes
#endif
#if ((S16_MIN !=      -32768) || (S16_MAX !=      32767))
#error "signed short int" expected to be 2 bytes
#endif
#if ((U32_MIN !=           0) || (U32_MAX != 4294967295U))
#error "unsigned int" expected to be 4 bytes
#endif
#if ((S32_MIN != ((-2147483647)-1)) || (S32_MAX != 2147483647))
#error "signed int" expected to be 4 bytes
#endif

/*
 * Declare the alignment table.
 *
 *   alignMax		The maximum alignment factor encounted for all alignment
 *			types.
 *
 *   schemaU/INTalign	The shAlign type corresponding to the object schema
 *			types of INT and UINT.  This is necessary, since all
 *			the shAlign types have size information in their names,
 *			while INT and UINT are not.
 */

IMPORT	struct
   {
   unsigned       int	 alignMax;	/* Maximum alignment of all types     */
   unsigned       int	 schemaINTalign;  /* Align type of  INT object schema */
   unsigned       int	 schemaUINTalign; /* Align type of UINT object schema */
   struct
      {
      unsigned long  int	 size;		/* Size of object in bytes    */
      unsigned       int	 align;		/* Alignment factor           */
      unsigned long  int	 incr;		/* Addr increment to next obj */
                     char	*typeName;	/* C     type name (text)     */
                     char	*dervishName;	/* Dervish type name (text)     */
      TYPE			 schemaType;	/* Dervish object schema type   */
      }			 type[SHALIGNCNTALL];
   }		 shAlignment;

/******************************************************************************/
/*
 * API for alignment routines.
 *
 * Public  Function		Explanation
 * ----------------------------	------------------------------------------------
 *   shAlignDown		Align address down to nearest boundary.
 *   shAlignUp			Align address up   to nearest boundary.
 *
 *   shAlignSchemaToAlignMap	Map an object schema name to a shAlign type.
 *   shAlignAlignToSchemaMap	Map a shAlign type to an object schema name.
 *
 * Private Function		Explanation
 * ----------------------------	------------------------------------------------
 * p_shAlignDown		Align address down to nearest boundary.
 * p_shAlignUp			Align address up   to nearest boundary.
 * p_shAlignConstruct		Constructor function.
 */

#define		   shAlignDown(addr, typeEnum) (p_shAlignDown ((addr), shAlignment[typeEnum].align))
#define		   shAlignUp(  addr, typeEnum) (p_shAlignUp   ((addr), shAlignment[typeEnum].align))

SHALIGN_TYPE	   shAlignSchemaToAlignMap (TYPE objSchemaType);
TYPE		   shAlignAlignToSchemaMap (SHALIGN_TYPE alignType);

#define		 p_shAlignDown(addr, align)    (((unsigned long  int)(addr)) & ~(align))
#define		 p_shAlignUp(  addr, align)    (p_shAlignDown (((unsigned char *)(addr)) + (align), (align)))

void    p_shAlignConstruct (void);

/******************************************************************************/

#ifdef	__cplusplus
   }
#endif

#endif	/* ifndef SHALIGN_H */
