/*++
 * FACILITY:
 *
 * ABSTRACT:	Routines to handle memory alignment at run-time.
 *
 *   Public
 *   --------------------------	------------------------------------------------
 *     shAlignSchemaToAlignMap	Map an object schema type to a shAlign type.
 *     shAlignAlignToSchemaMap	Map a shAlign type to an object schema type.
 *
 *   Private
 *   --------------------------	------------------------------------------------
 *   p_shAlignConstruct		Build the alignment table.
 *
 * ENVIRONMENT:	ANSI C.
 *		shAlign.c
 *
 * AUTHOR:	Tom Nicinski, Creation date: 17-Nov-1993
 *--
 ******************************************************************************/

#include	<stddef.h>	/* For offsetof ()                            */
#include	<string.h>

#include	"shCSchema.h"
#include	"shCUtils.h"	/* For shFatal ()                             */

#define		 shAlignIMP
#include	"shCAlign.h"
#undef		 shAlignIMP

/******************************************************************************/

SHALIGN_TYPE	   shAlignSchemaToAlignMap
   (
   TYPE			 objSchemaType	/* IN:    Object schema type          */
   )

/*++
 * ROUTINE:	   shAlignSchemaToAlignMap
 *
 *	Map an object schema type to a shAlign type.  Only a subset of object
 *	schemas are mapped, particularly, those that correspond to the primitive
 *	data types supported by shAlign.
 *
 * RETURN VALUES:
 *
 *   o	shAlign type.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	No checks are made to see whether the table mapping shAlign types to
 *	object schema names has proper object schema names.
 *--
 ******************************************************************************/

   {	/*   shAlignSchemaToAlignMap */

   /*
    * Declarations.
    */

                  int	 lcl_alignIdx;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   for (lcl_alignIdx = 0; lcl_alignIdx < SHALIGNCNT; lcl_alignIdx++)
      {
      if (objSchemaType == shAlignment.type[lcl_alignIdx].schemaType)
         {
         break;
         }
      }

   /*
    * NOTE:  Make sure any changes in the explicit mappings of object schemas
    *        (e.g., LOGICAL) to shAlign types is reflected in shCAlign.h and
    *        shCTbl.h (the shTbl_SuppData structures).
    */

   if (lcl_alignIdx >= SHALIGNCNT)
      {
       lcl_alignIdx  = (objSchemaType == shTypeGetFromName ("CHAR"))    ? SHALIGNTYPE_S8	/* Synonym for SCHAR */
                     : (objSchemaType == shTypeGetFromName ("LOGICAL")) ? SHALIGNTYPE_U8
                     : (objSchemaType == shTypeGetFromName ( "INT"))    ? shAlignment.schemaINTalign
                     : (objSchemaType == shTypeGetFromName ("UINT"))    ? shAlignment.schemaUINTalign
                                                                        : SHALIGNTYPE_unknown;
      }

   return (SHALIGN_TYPE)(lcl_alignIdx);

   }	/*   shAlignSchemaToAlignMap */

/******************************************************************************/

TYPE		   shAlignAlignToSchemaMap
   (
   SHALIGN_TYPE		 alignType	/* IN:    shAlign type                */
   )

/*++
 * ROUTINE:	   shAlignAlignToSchemaMap
 *
 *	Map a shAlign type to an object schema type.  Not all shAlign types
 *	necessarily map to object schemas.
 *
 * RETURN VALUES:
 *
 *   o	Object schema name.  If the shAlign type is unknown or does not map to
 *	an object schema, "UNKNOWN" is returned.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/*   shAlignAlignToSchemaMap */

   /*
    * Declarations.
    */

   TYPE			 lcl_objSchemaType;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   lcl_objSchemaType = ((alignType < 0) || (alignType >= SHALIGNCNT)) ? shAlignment.type[SHALIGNTYPE_unknown].schemaType
                                                                      : shAlignment.type[alignType].schemaType;

   return (lcl_objSchemaType);

   }	/*   shAlignAlignToSchemaMap */

/******************************************************************************/

void		 p_shAlignConstruct
   (
   void
   )

/*++
 * ROUTINE:	 p_shAlignConstruct
 *
 * DESCRIPTION:
 *
 *	Build the alignment table.  This table lists the memory alignment needed
 *	for the particular type (primitive C types only).
 *
 * RETURN VALUES:	None
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	This routine should be called prior to using the alignment table.
 *--
 ******************************************************************************/

   {	/* p_shAlignConstruct */

   /*
    * Declarations.
    *
    * The structures below are used to determine alignments.  The compiler/run-
    * time system for the particular system will align data properly in automa-
    * tic storage.  Since offsetof () will be used, the "kink" will be aligned
    * at address zero (0).  The "kink" is the smallest allocatable datum, namely
    * a byte.  It is followed by the type, which will be aligned on the smallest
    * possible boundary.
    *
    *   +----+----+--------------+
    *   |kink|////|  type field  |
    *   +----+----+--------------+
    *
    * NOTE:  It's assumed that the sizes of a signed type and its unsigned
    *        version are identical.  Thus, only one size/alignment entry is
    *        made.
    */

#define	shAlignStruct(alignType)\
   struct  lcl_align##alignType\
      {\
      unsigned char	 kink;		/* Throw things off w/ smallest unit */\
      alignType		 typeField;	/* Used to determine real alignment  */\
      }

   shAlignStruct (U8);
   shAlignStruct (S8);
   shAlignStruct (U16);
   shAlignStruct (S16);
   shAlignStruct (U32);
   shAlignStruct (S32);
   shAlignStruct (FL32);
   shAlignStruct (FL64);
   shAlignStruct (ptrData);		/* Pointer to data                    */

#undef	shAlignStruct

   TYPE			 lcl_schemaUNKNOWNtype;
   SCHEMA		*lcl_schema[2];

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * NOTE:  It's assumed that the "kink" is one byte in size and aligns on
    *        any boundary.  The code is written a bit more generally, but will
    *        N O T   properly compute the type alignment in case the "kink"
    *        size is more than one byte.  It may be more beneficial to hardwire
    *        a subtraction of one (1) rather than the "kink" size.
    */

   lcl_schemaUNKNOWNtype = shTypeGetFromName (shAlignSchemaName_unknown);

#define	p_shAlignDetermine(alignType)\
        shAlignment.type[SHALIGNTYPE_##alignType].size   =   sizeof (alignType);\
   if ((shAlignment.type[SHALIGNTYPE_##alignType].align  = offsetof   (struct lcl_align##alignType, typeField)\
                                                           - sizeof (((struct lcl_align##alignType *)0)->kink))\
                                                         > shAlignment.alignMax)\
      {\
        shAlignment.alignMax = shAlignment.type[SHALIGNTYPE_##alignType].align;\
      }\
        shAlignment.type[SHALIGNTYPE_##alignType].incr       =\
           (((sizeof (alignType)-1) / (shAlignment.type[SHALIGNTYPE_##alignType].align+1))+1)\
                                    * (shAlignment.type[SHALIGNTYPE_##alignType].align+1);\
        shAlignment.type[SHALIGNTYPE_##alignType].typeName   = shAlignTypeName_##alignType;\
        shAlignment.type[SHALIGNTYPE_##alignType].dervishName  = shAlignDervishName_##alignType;\
   if  (shAlignSchemaName_##alignType == ((char *)0))\
      {\
        shAlignment.type[SHALIGNTYPE_##alignType].schemaType = lcl_schemaUNKNOWNtype;\
      }\
   else\
   if ((shAlignment.type[SHALIGNTYPE_##alignType].schemaType = shTypeGetFromName (shAlignSchemaName_##alignType))\
                                                            == lcl_schemaUNKNOWNtype)\
      {\
      shFatal ("shCAlign.h object schema name \"%s\" for %s does not exist (%s line %d)",\
                shAlignSchemaName_##alignType, shAlignDervishName_##alignType, __FILE__, __LINE__);\
      }

#define	p_shAlignSchemaAlias(alignType, schemaName)\
   if ((lcl_schema[0] = ((SCHEMA *)shSchemaGet (#schemaName))) != ((SCHEMA *)0))\
      {\
        lcl_schema[1] = ((SCHEMA *)shSchemaGet (shAlignSchemaName_##alignType));\
        if (lcl_schema[0]->size == lcl_schema[1]->size)\
           {\
           shAlignment.schema##schemaName##align    = SHALIGNTYPE_##alignType;\
           }\
      }

   /*
    *   o   Initialize general information.
    */

   shAlignment.alignMax        = 0;
   shAlignment.schemaINTalign  = SHALIGNTYPE_unknown;
   shAlignment.schemaUINTalign = SHALIGNTYPE_unknown;

   /*
    *   o   Hardwire the "unknown" type.  The alignment factor for the unknown
    *       type will be the largest alignment factor from all the primitive
    *       data types understood by shAlign.
    */

   shAlignment.type[SHALIGNTYPE_unknown].size       = 0;
   shAlignment.type[SHALIGNTYPE_unknown].align      = 0;
   shAlignment.type[SHALIGNTYPE_unknown].incr       = 0;
   shAlignment.type[SHALIGNTYPE_unknown].typeName   =                    shAlignTypeName_unknown;
   shAlignment.type[SHALIGNTYPE_unknown].dervishName  =                    shAlignDervishName_unknown;
   shAlignment.type[SHALIGNTYPE_unknown].schemaType = shTypeGetFromName (shAlignSchemaName_unknown);

   /*
    *   o   Fill in all other types more generically.
    */

   p_shAlignDetermine (U8);     p_shAlignSchemaAlias (U8,   UINT);
   p_shAlignDetermine (S8);     p_shAlignSchemaAlias (S8,    INT);
   p_shAlignDetermine (U16);    p_shAlignSchemaAlias (U16,  UINT);
   p_shAlignDetermine (S16);    p_shAlignSchemaAlias (S16,   INT);
   p_shAlignDetermine (U32);    p_shAlignSchemaAlias (U32,  UINT);
   p_shAlignDetermine (S32);    p_shAlignSchemaAlias (S32,   INT);
   p_shAlignDetermine (FL32);
   p_shAlignDetermine (FL64);
   p_shAlignDetermine (ptrData);

#undef	p_shAlignDetermine
#undef	p_shAlignSchemaAlias

   }	/* p_shAlignConstruct */

/******************************************************************************/
