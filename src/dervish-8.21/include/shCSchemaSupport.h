
#ifndef _SHCSCHEMA_SUPPORT_H
#define _SHCSCHEMA_SUPPORT_H


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
#include "shCSchemaTrans.h"
#include "shCTbl.h"


#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ENUM_LEN 64
#define MAX_NAME_LEN 100



typedef enum 
 {
  NOT_A_CONTAINER,
  LIST_TYPE, 
  ARRAY_TYPE,
  CHAIN_TYPE
 } CONTAINER_TYPE; /*
                   pragma SCHEMA

                   */

 

RET_CODE  shSpptGeneralSyntaxCheck(
            SCHEMATRANS *xptr,                   /* translation table */
            SCHEMA* schema ,                 /* check against this schema */
            unsigned int to_table                /* direction of converison */
            );



int       p_shSpptEntryFind(
               SCHEMATRANS* xptr,                /* translation table */
               char* str                                    /* string to search */
             );
         

char*     p_shSpptArrayCheck(
            char* str,                  /* checking this string */
            int* num,                  /* in: dimension information. 
                                          Elem number must be at least 
                                          MAX_INDIRECTION or more */
            int* array_size,           /* out: array_size, start at 0 */
            int* dimen,                /* out: number of dimensions */
            char* token                /* int: checking again this token */   
         );


SCHEMA_ELEM* p_shSpptTransTblWalk(
                SCHEMATRANS* xptr, 
                const SCHEMA_ELEM* elemptr,
                SCHEMA_ELEM** tmp_elemptr, 
                int* entry_fnd
         );


char*  p_shSpptTransTblTraverse (
    	SCHEMATRANS* xptr,           	/* xtable */
    	SCHEMA_ELEM * current_elem,   	/* in: current, top level schema elem */
    	SCHEMA_ELEM ** last,          	/* out: schema elem at the bottom */
    	char*   field,                	/* field instance address to start */
    	int*    entry_start,          	/* in: entry gives the start up entry number */
                                      	/* out: entry gives the bottom continuation 
						entry number */
    	char**   upperPtr               /* pointer to memory of next to last level*/              
        );

int 	p_shSpptGetMaxStrlenOfEnum(char* type, int*);

int 	p_shSpptHeapDimenGet(
            SCHEMA* schema, 
            char* inst, char* str, 
            int* num, 
            unsigned get_total_length
        );

int 	p_shSpptHeapFldInit(SCHEMA* rootSchema,
      		char* inst, 
      		SCHEMA_ELEM * elemptr,
      		char** addr, 
      		SCHEMATRANS* xptr, 
      		int entry,
      		int heap_size_in_bytes);

int p_shSpptCastAndConvert(
   		char* src, 
                DSTTYPE srcDataType,
   		char* dst, 
   		DSTTYPE dstDataType, 
   		int array_size, 
   		double scale,        	/* scale factor, applied before zero*/
   		double zero         	/* zero translation, applied after scale */
);

int    p_shSpptArrayInfoExtract(char* str, int *num);
char*  p_shSpptHeapBlockGet(char* inst,
        	int nstar,
        	int existing_dimen,
        	int dst_dimen, 
        	int total_dimen,
        	int *num,
        	int dst_array_index,
        	int desired_block);
      
#if !defined(NOTCL) 
RET_CODE shSpptTclObjectInit(
            Tcl_Interp*  a_interp, 
            SCHEMATRANS* xtbl_ptr,
            void** instPtr,
            int    objCnt,
            SCHEMA* schemaPtr,
            unsigned short handleRetain
);
#endif

RET_CODE shSpptObjectStateInit(
            SCHEMATRANS* xtbl_ptr,
            void** objPtrs,
            int    objCnt,
            SCHEMA* schemaPtr
);


char* 
shSpptObjGetFromContainer(
          CONTAINER_TYPE type,
          void* container, 
          void** walkPtr,          /* for use with list */
          char* schemaName,
          int index
);

int p_shSpptStringCmp(char* s1, char* s2, unsigned short check_array);

TYPE        shSpptDSTTypeToSchemaType(DSTTYPE);
DSTTYPE     shSpptSchemaTypeToDSTType(TYPE);

#define MAX_ADDRESS_SIZE  500
#define MAX_RUNTIME_ENTRIES 500 


#ifdef __cplusplus
}
#endif 

#endif   /* ifndef SHCSCHEMA_SUPPORT_H */
      






