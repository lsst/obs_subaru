/*****************************************************************************
**
** FILE:
**	shHash.c
**
** ABSTRACT:
**	This file contains routines that manage hash tables
**
** ENTRY POINT		SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
** shHashAdd            public  Add an element to a hash table
** shHashGet    	public	Return an element in a hash table
** shHashPrintStat      public  Print the statistics of a hash table
**
** ENVIRONMENT:
**	ANSI C.
**
** REQUIRED PRODUCTS:
**
** NOTE: (PCG 05/96)
**    We made this hash table case insensitive .... except that we rely
**    on shSchemaGet to be the only one to call shHashGet and to uppercase
**    the incoming name.
*/
#include <string.h>
#include <stdlib.h>
#include "shCAssert.h"
#include "shCHash.h"
#include "shCErrStack.h"
#include <ctype.h>       /* This is in agreement with new  C standards */
/*
 * Hash STR returning the key K; note that NUMBUCKETS must be a power of 2
 */
#define HASHIT(K,STR,NBUCKET) \
   { \
      const char *ptr = STR; \
      for(K = 0;*ptr != '\0';) K = (K << 1) ^ *ptr++; \
      K &= (NBUCKET - 1); \
   }

/*----------------------------------------------------------------------------
**
** GLOBAL VARIABLES
*/
static int collisions = 0;		/* Number of collisions in table */

/*
 *
 * <AUTO>
 * Create a new hash table. We cannot simply use a static array as the
 * compiler is forced to generate multiply instructions to access the
 * elements, so we need to set up the pointers
 *
 * nbucket MUST be a power of two
 *
 * Returns the new hash table
 *
 * </AUTO>
 */
SHHASHTAB *
shHashTableNew(
	       unsigned int nbucket	/* number of buckets desired */
	       )
{
   int i;
   SHHASHTAB *new;
/*
 * nbucket must be a power of 2; let us check
 */
   for(i = nbucket;(i & 01) == 0;i >>= 1);
   shAssert(i == 1);			/* i.e. nbucket _is_ a power of 2 */

   /*
    * Explicit typecast added. Needed on some compilers. (P.Kunszt)
    */
   new = (SHHASHTAB *)malloc(sizeof(SHHASHTAB));
   new->bucket = (struct hash_bucket **)malloc(nbucket*sizeof(new->bucket[0]));
   new->bucket[0] = (struct hash_bucket *)malloc(nbucket*sizeof(*new->bucket[0]));
   new->nbucket = nbucket;

   for(i = 0;i < nbucket;i++) {
      new->bucket[i] = new->bucket[0] + i;
      new->bucket[i]->name[0] = '\0';
      new->bucket[i]->value = NULL;
   }

   return(new);
}

/*****************************************************************************/
/*
 * <AUTO>
 * Delete a hash table
 * </AUTO>
 */
void
shHashTableDel(
	       SHHASHTAB *table		/* the hash table in question */
	       )
{
   if(table != NULL) {
      free(table->bucket);
      free(table);
   }
}

/*============================================================================
**============================================================================
**
** ROUTINE: shHashAdd
**
** DESCRIPTION:
**	This routine adds a key to a hash table
**
** RETURN VALUES:
**	Success: SH_SUCCESS
**	Failure:
**		SH_HASH_TAB_IS_FULL	Hash table is full
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
RET_CODE
shHashAdd(
	  SHHASHTAB  *hashTab,		/* address of hash table */
	  const char *name,		/* Character string to add to table */
	  const void *value		/* pointer to information to associate
					   with this key */
	  )   
{
   unsigned int key, savedKey;
   int nbucket = hashTab->nbucket;

   char HashName[SIZE];
   int i = 0;
   while( name[i] !='\0') {
     HashName[i]=toupper(name[i]);
     i++;
   }
   HashName[i]='\0';

   HASHIT(key,HashName,nbucket);		/* find hash key */

   savedKey = key;
/*
 * Look for an empty slot at or following the hash value
 */
   while(*hashTab->bucket[key]->name != '\0') {
      if(hashTab->bucket[key]->name[0] == HashName[0] &&
	 strcmp(&hashTab->bucket[key]->name[1], &HashName[1]) == 0) {
	 hashTab->bucket[key]->value = (void *)value;
	 return(SH_SUCCESS);
      }
      
/*      key = (++key & (nbucket - 1));  */
      key = ((key+1) & (nbucket - 1));
      if(key == savedKey) {		/* The table is full */
	 return(SH_HASH_TAB_IS_FULL);
      }
   }

   hashTab->bucket[key]->value = (void *)value;
   strncpy(hashTab->bucket[key]->name,HashName,SIZE);
   hashTab->bucket[key]->name[SIZE] = '\0';
   
   if(savedKey != key) {
      collisions++;
   }

   return(SH_SUCCESS);
}

/*============================================================================
**============================================================================
**
** ROUTINE: shHashGet
**
** DESCRIPTION:
**	This routine returns the value associated with a key
**
** RETURN VALUES:
**	Success - pointer to value associated with the passed key
**	Failure - 
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
RET_CODE
shHashGet(
	  const SHHASHTAB *hashTab,	/* Hash table */
	  const char *name,		/* String to look up */
	  void **value			/* pointer to desired value */
	  )   
{
   unsigned int key, savedKey;
   int nbucket = hashTab->nbucket;
   struct hash_bucket **bucket = hashTab->bucket;


   /* We should NOT comment this out.  However performance is 
      essential in this function (at least let's not make it worse)
      so we rely on the fact that only shSchemaGet calls shHashGet and
      that shSchemaGet ALREADY has uppercased the name.
      */
   /*
   char HashName[SIZE];
   int i = 0;
   while(name[i]!='\0') { HashName[i]=toupper(name[i]); i++; }
   */
   
   HASHIT(key,name,nbucket);			/* find hash key */

   savedKey = key;
/*
 * Search the table for the desired value
 */
   do {
      if(bucket[key]->name[0] == name[0] &&
	 strcmp(&bucket[key]->name[1], &name[1]) == 0) { /* got it */
	 if((*value = bucket[key]->value) == NULL) {
	    return(SH_HASH_ENTRY_NOT_FOUND);
	 } else {
	    return(SH_SUCCESS);
	 }
      }
/*      key = (++key & (nbucket - 1));  */
     key = ((key+1) & (nbucket - 1));
   } while(key != savedKey);

   return(SH_HASH_ENTRY_NOT_FOUND);
}

/*============================================================================
**============================================================================
**
** ROUTINE: shHashPrintStat
**
** DESCRIPTION:
**	This routine prints out statistic information about the hash table
**
** RETURN VALUES:
**	Success - 
**	Failure - 
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
RET_CODE
shHashPrintStat(
		const SHHASHTAB *hashTab /* Pointer to hash table */
		)   
{
   int i;
   int n_entry;

   if(hashTab == NULL) {
      shErrStackPush("shHashPrintStat: Hash table is NULL");
      return(SH_GENERIC_ERROR);
   }
   
   n_entry = 0;
   for(i = 0;i < hashTab->nbucket;i++) {
      if(*hashTab->bucket[i]->name != '\0') {
	 n_entry++;
      }
   }

   printf("Hash table %d entries (%.1g%%) with %d conflicts\n",
	  n_entry,n_entry/(float)hashTab->nbucket,collisions);
   
   return(SH_SUCCESS);
}



