
#ifndef _SHCSCHEMA_TRANS_H
#define _SHCSCHEMA_TRANS_H


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


#ifdef __cplusplus
extern "C" {
#endif

#define MAX_INDIRECTION 5


/********* some support enum definitions *****************************/
/*** type of conversion ***/

typedef enum 
  { 
       CONVERSION_END,           /* end of converstion table */
       CONVERSION_UNKNOWN,
    /*    CONVERSION_BY_POS,        */
       CONVERSION_BY_TYPE ,      /* conversion by field name */
       CONVERSION_IGNORE_TYPE,    /*  ignore this entry's dst field */
     /*   CONVERSION_IGNORE_POS,   no longer supported */
       CONVERSION_CONTINUE       /* a continuation entry */
} CONVTYPE;                /*
				    pragma SCHEMA
                                 */


#define CONVERSION_IGNORE CONVERSION_IGNORE_TYPE

/*** type of field at object side , historically used DST for destination */

typedef enum 
  {
       DST_UNKNOWN,                /* unknown */
       DST_STR,                    /* string type */
       DST_CHAR,                   /* elementary types */     
       DST_SHORT,
       DST_INT,
       DST_LONG,
       DST_FLOAT,
       DST_DOUBLE,
       DST_SCHAR,                  /* signed stuff begins here */
       DST_UCHAR,                  /* unsigned stuff begins here */
       DST_USHORT,
       DST_UINT,
       DST_ULONG,
       DST_ENUM,
       DST_HEAP,
       DST_STRUCT
 } DSTTYPE;                     /*
				    pragma SCHEMA
                                 */

/*** entry status  ***/

typedef enum
 {
      SCHTR_VISITED = -1,      /* used in EntryFind() */
      SCHTR_FRESH = 0,         /* initial state */
      SCHTR_GOOD,              /* syntax checked */
      SCHTR_VGOOD       
} SCHTR_STATUS;                /*
				  pragma SCHEMA
                               */

/*********************************************************************/
typedef struct _xtable_entry 
 {
      CONVTYPE type;                          /* conversion type */
      char*          src;                     /* source string */
      char*          dst;                     /* destination string */
      char*          dsttype;                 /* dst type string */
      char*          heaptype;                /* heap base type */
      char*          heaplen;                /* number of blocks  */
      DSTTYPE        dstDataType;             /* destinnation type */
      char*          proc;                    /* malloc command for dst*/
      char*          size;                    /* size string */
      int   num[MAX_INDIRECTION];             /* number of blocks to malloc*/
      double         srcTodst;                /* optional ratio of data */
    /*  SCHTR_STATUS    status;              0 = fresh otherwise visited */

} SCHEMATRANS_ENTRY;                       /*
				               pragma SCHEMA
                                            */

#define SCHTRS_HASH_TBLLEN           20


typedef struct _xtabl_hash        /* hash table for SCHEMATRANS */
{
      int    totalNum;
      int    curNum;
      int *  entries;
} SCHTR_HASH;                       /*
				               pragma SCHEMA
                                            */

/********** now the table **************************************/

typedef struct _xtable
{
      int             totalNum;	   /* total number of entries, empty or not*/
      int             entryNum;    /* current number of good entries */
SCHEMATRANS_ENTRY*    entryPtr;	   /* pointed to entryNum entries */
SCHTR_STATUS     *    status;      /* status of entries */
                         /* 0 = fresh otherwise visited */
SCHTR_HASH  hash[SCHTRS_HASH_TBLLEN];

} SCHEMATRANS;                              /*
				               pragma SCHEMA
                                            */


/******************* declaration of c routines ***********************/

#define MAX_SCHEMATRANS_ENTRY_SIZE    100       /* reasonably big */

          

SCHEMATRANS*   	shSchemaTransNew(void);

void      	shSchemaTransDel(SCHEMATRANS* xptr);
int          shSchemaTransGetHashValue(char* str);
         
RET_CODE   shSchemaTransEntryDel(
              SCHEMATRANS* ptr,      /* translation table */
              unsigned int entryNumber,    /* entry number to delete */
              unsigned short del_related   /* if TRUE,delete related entries */
            );

RET_CODE  shSchemaTransEntryAdd(
              SCHEMATRANS* ptr,                /* translation table */
              CONVTYPE convtype,              /* conversion  type */
              char* src,                  /* string at fits side */
              char* dst,                  /* string at schema side */
              char* dsttype,              /* schema field type */
	      char* heaptype,             /* heap type */
              char* heaplen,		  /* heap length */
              char* proc,                 /* Tcl procedure to call */
              char* size,                 /* optional addtional size info */
              double r,                    /* units conversion ratio */
              int   pos                  /* absolute position to add the entry to */
                                         /* if -1, add to the end */
            );

void      shSchemaTransClearEntries(SCHEMATRANS* ptr);
RET_CODE  shSchemaTransEntryImport(
              SCHEMATRANS* src,               /* source translation table */
              SCHEMATRANS* dst,               /* destination translation table */
              int src_start,             /* source side: start entry */
              int src_end,               /* source side: non-inclusive end entry  */
              int dst_pos                /* dest side: position to add to 
                                            -1 if add to the end */
           );


void shSchemaTransStatusInit(SCHEMATRANS* ptr);

          /* print the contents of the table on stdout */

void      shSchemaTransEntryShow(SCHEMATRANS* ptr, int start, int end);

RET_CODE  shSchemaTransWriteToFile(SCHEMATRANS* ptr, char* fileName, int start, int end);
          /* initialize the table at specific entry */

void      shSchemaTransInit(SCHEMATRANS* ptr, unsigned int entry_to_init);

          /* restore the visit status of all entries */

void           shSchemaTransStatusInit(SCHEMATRANS *ptr);
void           shSchemaTransStatusSet(SCHEMATRANS *ptr, SCHTR_STATUS s, int entry);
SCHTR_STATUS   shSchemaTransStatusGet(SCHEMATRANS *ptr, int entry);

DSTTYPE shSchemaTransDataTypeGet(char* str, int* size);
CONVTYPE convTypeFromName(char* name);

extern RET_CODE shSchemaTransCreateFromFile(
           SCHEMATRANS* xtbl,
           char* fName);



#ifdef __cplusplus
}
#endif 

#endif   /* ifndef SHSCHEMA_TRANS_H */
      






