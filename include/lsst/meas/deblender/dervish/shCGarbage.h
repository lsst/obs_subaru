#ifndef _SHCGARBAGE_H
#define _SHCGARBAGE_H

/*
 * Dervish Garbage Collection (or Memory Handling) Routines.
 *
 * Vijay K. Gurbani 
 * (C) 1994 All Rights Reserved
 * Fermi National Accelerator Laboratory
 * Batavia, Illinois USA
 * ----------------------------------------------------------------------------
 * This is a specialized library of memory handling routines. Overview of the
 * algorithm is as follows:
 *
 *  When a request is made to allocate memory, a "Free memory pool" (FMP) is 
 *  searched first for a best fit. If an appropriate block is found, it
 *  is taken off the FMP and given to the user. If an appropriately
 *  sized block is not found, more memory is requested from the system,
 *  initialized as needed, and given to the user. Any block given to the
 *  user is also put on an "Allocated Memory Pool" (AMP). 
 *  When a user frees a memory block, it is first searched for on the AMP. 
 *  If it is found, it is taken off the AMP, zeroed out, and stored back on
 *  the FMP. If it is not found on the AMP, chaos reigns. 
 *
 * So, pictorically, one can imagine the FMP and AMP to look as follows:
 *
 * FMP[0] --> Free block --> Free block --> Free block --> 0
 * FMP[1] --> Free block --> Free block --> 0
 *   :
 * FMP[n] --> Free block --> 0
 *
 * AMP -->  Block --> Block --> Block --> Block ... Block --> 0
 *       0 <-- 1    <-- 2     <-- 3     <-- 4     <-- n
 *
 * AMP is a doubly linked list that holds blocks of varying sizes.
 */

#include "shCUtils.h"  /* For some strange reason, all memory API are defined */
                    /* here. So, include this for backward compatibility   */

/*
 * The following constant marks a memory block as being allocated using
 * Dervish routines. Memory blocks allocated  using straight malloc(2)s,
 * will not be branded by SH_MAGIC
 */ 
#define SH_MAGIC   0xdeadbeef

/*
 * The Dervish memory structure. Order of the elements _matter_. Do not 
 * change the order! 
 * Note that due to the nature of the algorithm being used, the next
 * and previous pointers have to be stored in this structure only.
 * The "Allocated Memory Pool" (AMP) consists of a doubly linked list
 * of these structures.
 */
typedef struct _Memory  {
   struct _Memory *pNext;
   struct _Memory *pPrev;
   unsigned long  serial_number; 
   unsigned int   reference;     /* Reference counter */
   size_t         user_bytes;    /* User-requested bytes */
   size_t         actual_bytes;  /* Actually allocated bytes */
#if defined(CHECK_LEAKS)
   char *file;			 /* file memory allocated in */
   int line;			 /* line number in file */
#endif
   unsigned int   poison;        /* Mark block with SH_MAGIC */
   void           *pUser_memory; /* Pointer to the memory given to user */
} SH_MEMORY;

/*
 * The "Free Memory Pool." 
 * This is a table of MAX_BUCKETS memory "buckets."
 * Free'd memory blocks are into this pool. 
 * Each bucket holds free'd memory blocks that fall within a given range.
 * The range is defined as increasing powers of 2, starting from 
 * MIN_BUCKET_SIZE to MAX_BUCKET_SIZE
 * Thus the first bucket holds all free memory blocks ranging in size from
 * 1:2^6; second bucket holds all free memory blocks ranginging in size from 
 * (2^6)+1:2^7 ... and so on. The last bucket is a "catchall" in that 
 * any memory request greater then or equal to MAX_BUCKET_SIZE is inserted
 * to the last bucket
 *
 * All memory allocation requests are rounded up to the next highest power
 * of 2; except if they exceed MAX_BUCKET_SIZE, then a different heuristic
 * is used to round them.
 */
#define NBUCKETS          10
#define MAX_BUCKET_SIZE   32768 
#define MIN_BUCKET_SIZE   64

typedef struct {
   unsigned int size;			/* "Bucket" size */
   SH_MEMORY      *pMemory_blocks;
} FREEMEMPOOL;

/*
 * Union needed for correctly aligning memory 
 */
typedef union {
   char             filler_1;
   char            *filler_2;
   short            filler_3;
   long             filler_4;
   int              filler_5;
   int             *filler_6;
   double           filler_7;
   float            filler_8;
   void            *filler_9;
   int            (*filler_10)(void);
} ALIGN;

#if defined ALIGN
#  undef ALIGN
#endif
#define MAXALIGN (sizeof(ALIGN))
#define ALIGN(x) ((((x)+MAXALIGN-1)/MAXALIGN)*MAXALIGN)

/*
 * Public API to Dervish Memory Handling.
 * 
 */
#ifdef __cplusplus
extern "C"
{
#endif

void *shMalloc(size_t size);
void *shRealloc(void *ptr, size_t size);
void *shCalloc(size_t num_of_elts,size_t elt_size);
void shFree(void *ptr);
int shMemDefragment(int free_to_os);
int shIsShMallocPtr(void *ptr);
/*
 * Enable checking for memory leaks by replacing the normal calls to
 * shMalloc by calls to p_check_shMalloc (etc.) that include the file
 * and line where the function's called
 */
#if defined(CHECK_LEAKS)
void *p_check_shMalloc(size_t size, char *file, int line);
void *p_check_shRealloc(void *ptr, size_t size, char *file, int line);
void *p_check_shCalloc(size_t num_of_elts,size_t elt_size, char *file, int line);
void p_check_shFree(void *ptr, char *file, int line);

#define shMalloc(S) p_check_shMalloc(S,__FILE__,__LINE__)
#define shCalloc(N,S) p_check_shCalloc(N,S,__FILE__,__LINE__)
#define shRealloc(P,S) p_check_shRealloc(P,S,__FILE__,__LINE__)
#define shFree(P) p_check_shFree(P,__FILE__,__LINE__)
#endif

long p_shMemIdFind(IPTR addr);
void p_shMemRefCntrIncr(void *addr);
void p_shMemRefCntrDecr(void *addr);
unsigned int p_shMemRefCntrGet(void *);
long p_shMemRefCntrDel(long id);
long p_shMemAddrIntern(void *ptr);
void p_shMemTreeApply(void (*func)(const IPTR k, const IPTR v, void *d));
int p_shIsInternedId(long id);

size_t shMemBlocksizeSet(size_t bsize);
int           shMemFreeBlocks(const unsigned long, const unsigned long,
                              void (*)(void *));
void shMemEmptyCB(void *(*funcp)(size_t nbyte));
void shMemInconsistencyCB(void (*funcp)(unsigned long, const SH_MEMORY *));
void shMemSerialCB(const unsigned long,
		   void (*funcp)(unsigned long thr, const SH_MEMORY *));
void shMemSerialFreeCB(const unsigned long trigger,
		       void (*funcp)(unsigned long, const SH_MEMORY *));
void          shMemRefCntrIncr(void *addr);
void          shMemRefCntrDecr(void *addr);
void          shMemStatsPrint(void);
short         shMemTraverse(void (*)(SH_MEMORY *const));
unsigned long shMemSerialNumberGet(void);
unsigned long shMemTotalBytesMalloced(void);
unsigned long shMemActualBytesMalloced(void);
unsigned long shMemBytesInUse(void);
unsigned long shMemBytesInPool(void);
unsigned long shMemNumMallocs(void);
unsigned long shMemNumFrees(void);
unsigned long shMemAMPSize(void);

/*
 * check memory arena
 */
int
p_shMemCheck(int check_allocated,	/* check allocated blocks? */
	     int check_free,		/* check free blocks? */
	     int abort_on_error);	/* abort on first error? */

#ifdef __cplusplus
}
#endif

#endif
