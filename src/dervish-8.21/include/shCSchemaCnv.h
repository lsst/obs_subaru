
#ifndef _SHCSCHEMACNV_H
#define _SHCSCHEMACNV_H


/* 
 *  Facility:           dervish
 *  Synopsis:           declarations for conversions between TBLCOL to schemas
 *  Environment:        ANSI C
 *  Author:             Wei Peng
 *  Creation Date:      Feb 1, 1994
 */

#include "shTclHandle.h"
#include "dervish_msg_c.h"
#include "shCSchema.h"
#include "shCTbl.h"
#include "shCSchemaTrans.h"
#include "shCSchemaSupport.h"

#ifdef __cplusplus
extern "C" {
#endif

RET_CODE shCnvTblToSchema(
   TBLCOL* 	tblCol,
   SCHEMATRANS* xtbl_ptr,
   void*        object,
   char* 	schemaName,
   char** 	addr,   
   int    	size,   
   int 		row_index,        	
   int 		stop_index,
   CONTAINER_TYPE  containerType,
   unsigned short objectReuse   /* true if we reuse the old objects */
);

RET_CODE
shCnvSchemaToTbl(
   TBLCOL** tblcolPtrPtr,    		/* out: tblcol instance to convert to */
   void*   object,    		
   SCHEMATRANS* xtbl_ptr,       /* in: translation table structure */
   char*   schemaName,          /* schema name that's in array */
   CONTAINER_TYPE  containerType,
   unsigned want_auto_convert	/* TRUE if one wants auto_convert */
);

/************************************************************************/
/* A test structure is constructed here to test this conversion
 * package. No API is provided for this structure. Anything that
 * relies on the existence of this structure (other than test functions)
 * will be catastrophic.
 */

#include "region.h"

typedef struct _test {
     int     ia;
     int     ib[10];
     double  dbl;
     float   fa;
     float   fb[20];
     char    cb[20];
     int     **heap[3][2];    /* complex heap tester */
     REGION*  ra;
     REGION** rb;
} TRANSTEST;               /* pragma SCHEMA */


#ifdef __cplusplus
}
#endif 

#endif   /* ifndef SHCSCHEMACNV_H */
      






