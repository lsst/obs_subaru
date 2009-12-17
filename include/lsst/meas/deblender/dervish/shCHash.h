#ifndef _SHCHASH_H
#define _SHCHASH_H

/*****************************************************************************
******************************************************************************
**
** FILE:
**      shCHash.h
**
** ABSTRACT:
**      This file contains all necessary definitions, macros, etc., for
**      the routines that handle hash tables
**
** ENVIRONMENT:
**      ANSI C.
**
** AUTHOR:
**      Creation date: Jan 20, 1995
**
******************************************************************************
******************************************************************************
*/
#include "shCSchema.h"
#include "dervish_msg_c.h"

struct hash_bucket {			/* we need a name for this type
					   in shHashGet(), so it cannot be
					   an anon struct in SHHASHTAB */
   char name[SIZE + 1];
   void *value;
};


typedef struct {
   int nbucket;				/* number of buckets
					   must be a power of 2 */
   struct hash_bucket **bucket;
} SHHASHTAB;

/*----------------------------------------------------------------------------
**
** FUNCTION PROTOTYPES
*/
#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

SHHASHTAB *shHashTableNew(unsigned int nbucket);
void shHashTableDel(SHHASHTAB *table);
RET_CODE shHashAdd(SHHASHTAB *a_hashTab,
		   const char *a_name,
		   const void *a_value);
RET_CODE shHashGet(const SHHASHTAB *a_hashTab,
		   const char *a_name,
		   void **a_value);
RET_CODE shHashPrintStat(const SHHASHTAB *hashTab);
  
#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif
