/*
 * shGarbage.c - Implementation of the Dervish Garbage collector
 *
 * ----------------------------------------------------------------------------
 * 
 * FILE:
 *   shGarbage.c
 *
 * ABSTRACT:
 *   This file contains routines to implement garbage collection. For an over-
 *   all algorithmic overview, please refer to dervish/include/shCGarbage.h
 *
 * Public APIs are as follows:
 *
 * ENTRY                   DESCRIPTION
 * ----------------------------------------------------------------------------
 * shMalloc                Allocate memory+bookkeeping header & insert in AMP
 * shFree                  Deallocate memory, ie. put in FMP
 * shCalloc                shMalloc pace for an array with the number of elements 
 *                             specified and initializes the space to zeros.
 * shRealloc               Reallocate a previously shMalloc()'ed memory block
 * shMemRefCntrIncr        Increment a memory block's reference counter
 * shMemRefCntrDecr        Decrement a memory block's reference counter
 * shIsShMallocPtr         Ascertain if a memory block was shMalloc()'ed
 * shMemSerialCB          Register a callback function to be called later
 * shMemFreeBlocks         Insert blocks from AMP to FMP
 * shMemTraverse           Traverse the AMP, calling a user supplied function
 * shMemStatsPrint         Print (to stdout) interesting memory statistics
 * shMemSerialNumberGet    Get the current serial number in use
 * shMemTotalBytesMalloced Get the total amount of memory allocated so far
 * shMemActualBytesMalloced Get the total amount of memory used so far
 * shMemBytesInUse         Get the total amount of memory in the AMP
 * shMemBytesInPool        Get the total amount of memory in the FMP
 * shMemNumFrees           How many times did we call shFree()?
 * shMemNumMallocs         And how many times did we call shMalloc()?
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <alloca.h>
#include "shCAssert.h"
#include "shCGarbage.h"
#include "shCErrStack.h"
#include "shC.h"

/*
 * A few static global variables. They make life a whole lot easier.
 * g_Fmp - Free memory pool (aka buckets)
 * g_Amp - Allocated memory pool pointer
 * g_mallocList - Memory granted by malloc(), but not yet allocated to g_Amp
 * g_Serial_number  - Current serial number
 * g_Total_bytes    - Total actual bytes malloc'ed so far
 * g_Actual_bytes   - Total bytes malloc'ed so far (user bytes + header),
 *                    and passed into the AMP/FMP system
 * g_Amp_user_bytes - Total bytes in the AMP
 * g_Fmp_user_bytes - Total bytes in the FMP (sum of all buckets)
 * g_Num_mallocs    - Total number of times shMalloc()/shRealloc() called
 * g_Num_frees      - Total number of times shFree() has been called
 * g_Malloc_system  - Number of times system malloc(2) was called by shMalloc()
 * g_Malloc_Fmp     - Number of times shMalloc() request satisfied from FMP
 * g_Malloc_Minsize - Smallest block of memory to request from O/S
 */
static FREEMEMPOOL g_Fmp[NBUCKETS] = {
       {MIN_BUCKET_SIZE,  NULL},
       {128,     NULL},   {256,     NULL},
       {512,     NULL},   {1024,    NULL},    
       {2048,    NULL},   {4096,    NULL},
       {8192,    NULL},   {16384,   NULL},
       {MAX_BUCKET_SIZE,  NULL},    /* Catchall */
};


static SH_MEMORY        *g_Amp              = NULL;
static SH_MEMORY        *g_mallocList       = NULL;
static unsigned long  g_Serial_number    = 0L;
static unsigned long  g_Serial_threshold = 0L;
static unsigned long  g_Serial_free_threshold = 0L;
static unsigned long  g_Total_bytes      = 0L;
static unsigned long  g_Actual_bytes     = 0L;
static unsigned long  g_Amp_user_bytes   = 0L;
static unsigned long  g_Fmp_user_bytes   = 0L;
static unsigned long  g_Num_mallocs      = 0L;
static unsigned long  g_Num_frees        = 0L;
static unsigned long  g_Malloc_system    = 0L;
static unsigned long  g_Malloc_Fmp       = 0L;
static size_t g_Malloc_Minsize = 0;
/*
 * A few callback functions:
 *   shMemInform : Called by Dervish memory handling routines when the Serial
 *                 number reaches a certain user specified threshold
 *   shMemFreeInform : Called by Dervish memory handling routines when a
 *                 user specified memory block is freed
 *   shMemEmpty  : Called by Dervish memory handling routines when the system
 *                 malloc(2) returns NULL. If the handler returns, it _must_
 *                 return the requested number of bytes
 *   MemInconsistency_callback : called when a problem in the heap is detected.
 *
 * These functions are appropriately defaulted.
 */
static void
shMalloc_callback(unsigned long thresh, const SH_MEMORY *memBlock)
{
   volatile int i;
   shAssert(memBlock != NULL);		/* suppress compiler warnings */
   i = thresh;				/* something for a debugger to
					   set a break at */
}

static void
shFree_callback(unsigned long thresh, const SH_MEMORY *memBlock)
{
   volatile int i;
   shAssert(memBlock != NULL);		/* suppress compiler warnings */
   i = thresh;				/* something for a debugger to
					   set a break at */
}

static void
MemInconsistency_callback(unsigned long thresh, const SH_MEMORY *memBlock)
{
   volatile int i;
   shAssert(memBlock != NULL);		/* suppress compiler warnings */
   i = thresh;				/* something for a debugger to
					   set a break at */
   shFatal("DERVISH detected error in memory management");
}

static void *
defaultMemEmptyFunc(size_t nbyte)
{
   shAssert(nbyte > 0);			/* suppress compiler warnings */
   shFatal("DERVISH internal error(1): out of system memory");

   return(NULL);
}

static void (*shMemInform)(unsigned long, const SH_MEMORY *) =
							     shMalloc_callback;
static void (*shMemFreeInform)(unsigned long, const SH_MEMORY *) =
							       shFree_callback;
static void (*shMemInconsistency)(unsigned long, const SH_MEMORY *) =
						     MemInconsistency_callback;
static void *(*shMemEmpty)(size_t) = defaultMemEmptyFunc;

/*
 * Static functions used locally by the memory manager
 */
static size_t   round_up(size_t);
static int      getMemPoolIndex(const size_t size);
static SH_MEMORY  *search_Fmp(const size_t);
static void     insList(SH_MEMORY **, SH_MEMORY *);
static SH_MEMORY  *remList(SH_MEMORY **, SH_MEMORY *);
static int bad_memblock(const SH_MEMORY *m);

void  p_shMemRefCntrIncr(void *addr)  { shMemRefCntrIncr(addr); }
void  p_shMemRefCntrDecr(void *addr)  { shMemRefCntrDecr(addr); }
long  p_shMemRefCntrDel(long id)      { return id; }
long  p_shMemAddrIntern(void *ptr)    { shAssert(ptr != NULL); return 1L; }
long  p_shMemIdFind(long addr)        { return addr; }
int
p_shIsInternedId(long id)		/* NOTUSED */
{ return 0L; }
void
p_shMemTreeApply(void (*func)(const IPTR, const IPTR, void *)) /* NOTUSED */
{ return; }
/*
 * Some functions that allow access to private variables
 */
unsigned long shMemSerialNumberGet(void)    { return g_Serial_number; }
unsigned long shMemTotalBytesMalloced(void) { return g_Total_bytes; }
unsigned long shMemActualBytesMalloced(void) { return g_Actual_bytes; }
unsigned long shMemBytesInUse(void)    { return g_Amp_user_bytes; }
unsigned long shMemBytesInPool(void)   { return g_Fmp_user_bytes; }
unsigned long shMemNumMallocs(void)    { return g_Num_mallocs; }
unsigned long shMemNumFrees(void)      { return g_Num_frees; }
unsigned long shMemAMPSize(void)       { return g_Num_mallocs - g_Num_frees; }

/*
 * <AUTO EXTRACT>
 *
 * Set the minimum size in bytes for a malloc() request, the balance of
 * any memory requested goes onto the internal free list g_mallocList
 *
 * Returns the old value; as a special case is size of ~0UL, the value
 * is _not_ changed.
 */
size_t
shMemBlocksizeSet(size_t size)
{
   size_t old = g_Malloc_Minsize;

   if(size != ~0UL) {
      if(size == 0 && g_Malloc_Minsize != 0) {
	 shError("shMemBlocksizeSet: "
		 "you may not set g_Malloc_Minsize back to zero");
      } else {
	 g_Malloc_Minsize = ALIGN(size);
      }
   }
   
   return(old);
}

/*
 * ROUTINE: shMalloc()
 *
 * CALL:
 *   void *shMalloc(const size_t size)
 *   size - Number of bytes to allocate
 *
 * DESCRIPTION:
 *   shMalloc() is a dervish wrapper around the system malloc(2). It facilitates
 *   garbage collection. It's basic algorithm is as follows:
 *
 *   Is there enough space on FMP to satisfy a malloc request?
 *      YES : Take the appropriate block off the FMP and insert it on
 *            the AMP, passing a pointer to the user
 *      NO  : Allocate appropriately sized (and aligned) memory from the
 *            OS (including bytes needed for the bookeeping header), 
 *            initialize the header, put the memory block on the AMP and 
 *            pass a pointer to the user. The pointer thus passed excludes
 *            the bookeeping header.
 *   We actually allocate big blocks from the OS (at least g_Malloc_Minsize)
 *   and dish these out as needed. This allows us to defragment the heap
 *   if so desired. Alternatively, leave g_Malloc_Minsize as 0, and
 *   defragment by freeing all unused memory
 *
 * When memory is actually allocated from the OS, some extra bytes are 
 * for bookeeping purposes. The pointer returned to the user excludes these
 * bookeeping bytes.
 *
 * RETURNS:
 *   On success: pointer to a suitably aligned memory area
 *   There is no failure return from this function.
 */
void *
#if defined(CHECK_LEAKS)
p_check_shMalloc(size_t size, char *file, int line)
#else
shMalloc(size_t size)
#endif
{
   int from_free_list = 0;		/* did our memory come from the pool?*/
   int i;
   size_t malloc_size;			/* the size we actually allocate */
   size_t offset;			/* offset of pMem_block in block
					   from g_mallocList */
   SH_MEMORY *pMem_block;		/* block of memory from somewhere */
   size_t r_size;			/* size rounded up suitably */

   if(size == 0) return NULL;
/*
 * rounding up will fail if high bit's set; so check that it isn't.
 *
 * In other words, this assertion will fail if you try to allocate
 * an enormous (or negative) amount of memory. This may be an end-user
 * bug not a dervish one (e.g. PR 671)
 */
   if((size & ((size_t)1 << (8*sizeof(size) - 1))) != 0) {
      shFatal("Attempt to allocate %ud == %d bytes\n", size, (long)size);
   }

   g_Serial_number++;

   r_size = round_up(size);
   pMem_block  = search_Fmp(r_size);

   if (pMem_block != NULL) {
      from_free_list = 1;
   } else {
       const size_t minsize = g_Malloc_Minsize;
       SH_MEMORY *ptr;			/* used to search g_mallocList */

       malloc_size = ALIGN(r_size + sizeof(SH_MEMORY));
/*
 * see if it's available from g_mallocList. We have to keep at least
 * sizeof(SH_MEMORY) bytes back as they are used for book-keeping,
 * and because we need to keep the head pointers separate, as they
 * came from malloc()
 *
 * If we cannot get anything, try defragmenting the free pool and
 * trying again
 */
       for(i = 0; i < 2; i++) {
	  for(ptr = g_mallocList; ptr != NULL; ptr = ptr->pNext) {
	     if(ptr->user_bytes >= malloc_size + sizeof(SH_MEMORY)) {
		offset = ptr->user_bytes - malloc_size;
		pMem_block = (SH_MEMORY *)((char *)ptr + offset);
		ptr->user_bytes -= malloc_size;
		g_Actual_bytes += malloc_size;
		
		break;
	     }
	  }
	  if(pMem_block != NULL) {	/* got it */
	     break;
	  }
	  if(i == 0) {			/* defragment the pool */
	     if(g_Malloc_Minsize != 0 &&
		g_Fmp_user_bytes > 2*malloc_size) { /* there's a chance */
		(void)shMemDefragment(0);
	     }
	     
	     pMem_block = search_Fmp(r_size); /* try the pool again */

	     if(pMem_block != NULL) {	/* got it! */
		from_free_list = 1;
		break;
	     }
	  }
       }
       
       if(pMem_block == NULL) {	/* no; get it from OS */
	  size_t nalloc = (malloc_size >= minsize) ? malloc_size : minsize;

	  if(minsize == 0) {
	     pMem_block = malloc(malloc_size);
	     g_Total_bytes += malloc_size;
	  } else {
	     nalloc += sizeof(SH_MEMORY); /* add space for g_mallocList's
					     header */
	     if((ptr = malloc(nalloc)) != NULL) {
		ptr->user_bytes = ptr->actual_bytes = nalloc;

		g_Total_bytes += nalloc;
		
		offset = ptr->user_bytes - malloc_size;
		pMem_block = (SH_MEMORY *)((char *)ptr + offset);
		ptr->user_bytes -= malloc_size;
		ptr->poison = SH_MAGIC;	/* Mark memory as being malloced */
		ptr->pPrev = NULL;
		ptr->serial_number = 0;
		
		ptr->pNext = g_mallocList;
		g_mallocList = ptr;
	     }
	  }

	  g_Actual_bytes += malloc_size;
       }
       if (pMem_block == NULL) {
	  pMem_block = (*shMemEmpty)(malloc_size);
       }

       pMem_block->pNext = pMem_block->pPrev = NULL;
       pMem_block->actual_bytes = malloc_size - sizeof(SH_MEMORY);
       pMem_block->poison = SH_MAGIC;	/* Mark memory as being malloced
					   using shMalloc(). shFree() will
					   check this before free'ing */
   }

   if(from_free_list) {
      g_Malloc_Fmp++;			/* block came from free list */
   } else {
      g_Malloc_system++;		/* block came from "system" */
   }
   
   pMem_block->reference = 0;
   pMem_block->user_bytes = size;
   pMem_block->pUser_memory = (char *)pMem_block + sizeof(SH_MEMORY);

#if !defined(NDEBUG)
   if(bad_memblock(pMem_block)) {
      shError("shMalloc: Detected corrupted memory block "
	      "on free list ("PTR_FMT"): %s %d",
	      pMem_block->pUser_memory, __FILE__, __LINE__);
      (*shMemInconsistency)(g_Serial_number, pMem_block);
      shError("shMalloc: proceeding to use corrupted memory block; good luck");
   }
#endif

   pMem_block->serial_number = g_Serial_number;
#if defined(CHECK_LEAKS)
   pMem_block->file = file;
   pMem_block->line = line;
#endif

   /*
    * Insert the memory malloced on the head of the AMP.
    */
   if (g_Amp == NULL)
         g_Amp = pMem_block;
   else  {
         pMem_block->pNext = g_Amp;
         g_Amp->pPrev = pMem_block;
         g_Amp = pMem_block;
   }

   g_Amp_user_bytes += size;
    
   g_Num_mallocs++;

   if (g_Serial_threshold == ~0L || g_Serial_number == g_Serial_threshold)
       (*shMemInform)(g_Serial_threshold, pMem_block);

   return pMem_block->pUser_memory;
}

/*
 * ROUTINE: shFree()
 *
 * CALL:
 *   (void) shFree(void *ptr);
 *   ptr - Pointer to memory previously allocated with a call to shMalloc()
 *
 * DESCRIPTION:
 *   shFree() "frees" a memory location identified by ptr. It makes sure
 *   that the block being freed was indeed allocated using shMalloc(). It
 *   also makes sure that user does not call shFree() twice on the same
 *   address. shFree() also makes sure that there are no outstanding 
 *   references to the memory block being freed. If there are any references,
 *   instead of freeing that block, the reference counter associated with
 *   that memory block is decremented.
 *   shFree() does not actually free the memory block. Instead, it does some
 *   housekeeping work on the memory header, zeroes the contents of the
 *   memory block (NOT the header, just the block), and puts that block on 
 *   the FMP for later reuse.
 *
 * RETURNS:
 *   Nothing.
 */
void
#if defined(CHECK_LEAKS)
p_check_shFree(void *ptr, char *file, int line)
#else
shFree(void *ptr)
#endif
{
   SH_MEMORY *mp;
   int    indx;

   if (ptr == NULL)
       return;

   mp = (SH_MEMORY *)ptr - 1;

   if(bad_memblock(mp)) {
      shError("shFree: Attempt to free memory corrupted or "
	      "not allocated by shMalloc ("PTR_FMT"): %s %d", ptr, __FILE__, __LINE__);
      (*shMemInconsistency)(g_Serial_number, mp);
      return;
   }

   if (g_Serial_free_threshold == ~0L ||
				mp->serial_number == g_Serial_free_threshold) {
      (*shMemFreeInform)(g_Serial_number, mp);
   }

   if (mp->pUser_memory == NULL) {
      shError("shFree: Attempt to free memory previously freed "
	      "(id %d, addr "PTR_FMT"): %s %d",mp->serial_number,
	      ptr, mp->file, mp->line);
      (*shMemInconsistency)(mp->serial_number, ptr);
      return;
   }

   if (mp->reference > 0)  {
       (mp->reference)--;
       return;
   }

   indx = getMemPoolIndex(round_up(mp->user_bytes));

   /*
    * Take this block out of the AMP
    */
   {
      SH_MEMORY *prev = mp->pPrev; /* unalias */
      SH_MEMORY *next = mp->pNext; /* unalias */

      if (prev == NULL && next == NULL) {
	 g_Amp = NULL;
      } else if (prev == NULL)  {
	 g_Amp = next;
	 g_Amp->pPrev = NULL;
      } else if (next == NULL) {
	 prev->pNext = NULL;
      }
      else  {
	 prev->pNext = next;
	 next->pPrev = prev;
      }
   }

   g_Amp_user_bytes -= mp->user_bytes;

   /*
    * Set mp->pUser_memory to
    * NULL. shFree() always checks mp->pUser_memory, and if it points to
    * NULL, it is trapped as as freeing a memory block already freed.
    */
   mp->pUser_memory = mp->pPrev = mp->pNext = NULL;

#if defined(CHECK_LEAKS)
   mp->file = file;
   mp->line = line;
#endif
   /*
    * Now insert it in the appropriate FMP bucket
    */
   insList(&(g_Fmp[indx].pMemory_blocks), mp);

   g_Num_frees++;
   return;
}

/*
 * ROUTINE: shCalloc()
 *
 * CALL:
 *   void *shCalloc(size_t num_of_elts,size_t elt_size);
 *   num_of_elts - number of elements to allocate
 *   elt_size    - size of each element
 *
 * DESCRIPTION:
 *   shCalloc() is a dervish wrapper around the shMalloc(2) that mimic
 *   calloc inside the dervish memory system.
 *   It allocates space for an array with the number of ele-
 *   ments specified by the num_of_elts parameter, where each element is of the
 *   size specified by the elt_size parameter.  The space is initialized to
 *   zeros.
 *
 * RETURNS:
 *   On success: pointer to a suitably aligned memory area
 *   There is no failure return from this function.
 */
void *
#if defined(CHECK_LEAKS)
p_check_shCalloc(size_t num_of_elts,size_t elt_size, char *file, int line)
#else
shCalloc(size_t num_of_elts,size_t elt_size)
#endif
{
  size_t size;
  void* ptr;

  size = num_of_elts * elt_size;

  /* We can't call shMalloc directly when CHECK_LEAKS is defined because
     we want shMalloc to record the file and line of the shCalloc's CALLER
     */
#if defined(CHECK_LEAKS)
  ptr = p_check_shMalloc(size, __FILE__, __LINE__); 
#else
  ptr = shMalloc(size);
#endif
  
  memset ( ptr, 0, size);

  return ptr;
}

/* ROUTINE: shRealloc()
 *
 * CALL:
 *   (void *) shRealloc(void *ptr, size_t size);
 *   ptr  - Pointer to memory address
 *   size - New size
 *
 * DESCRIPTION:
 *   shRealloc() takes a pointer to memory region previously allocated by 
 *   shMalloc() and changes its size while preserving its contents. 
 *   A pointer to the new memory region is returned. If the request cannot
 *   be satisfied, a null pointer is returned and the old region is not
 *   disturbed.
 *
 *   shRealloc() conforms to the ANSI C standard for realloc(). This is
 *   outlined in Harbison and Steele, "C: A Reference Manual", Third Edition,
 *   pages 334-335.
 *
 * RETURNS:
 *   Pointer to the (possibly new) memory region,
 *   or NULL if 0 bytes are requested
 */
void *
#if defined(CHECK_LEAKS)
p_check_shRealloc(void *ptr, size_t size, char *file, int line)
#else
shRealloc(void *ptr, size_t size)
#endif
{
   SH_MEMORY *pOldMem;
   void   *pNewMem;
   size_t  count;
#if !defined(CHECK_LEAKS)
   char *file = __FILE__;
   int line = __LINE__;
#endif
   
   if (ptr == NULL)
       return p_check_shMalloc(size, __FILE__, __LINE__);

   if (ptr != NULL && size == 0)  {
       p_check_shFree(ptr, __FILE__, __LINE__);
       return NULL;
   }

   if (shIsShMallocPtr(ptr) == 0)
     shError("shRealloc: Attempt to reallocate memory corrupted or "
	     "not allocated by shMalloc ("PTR_FMT"): %s %d",ptr,file,line);

   pOldMem = (SH_MEMORY *)ptr - 1;
   if (pOldMem->pUser_memory == NULL)
       shError("shRealloc: Attempt to reallocate memory already freed "
	       "(id %d, addr "PTR_FMT"): %s, %d",pOldMem->serial_number,ptr,
	       file,line);
   /*
    * There are two reasons why we don't use the system realloc():
    * 1) system realloc() does not guarantee that the re-allocated address
    *    will be the same as before. This will cause some consentration
    *    here, since the memory manager holds the allocated blocks in a
    *    list, so if the address changes, the list should be updated.
    *    Note that this is the _right_ way to do it, but I am running out
    *    time at this point. Maybe in the future...
    * 2) it's much simpler to do this in the way outlined below.
    */

   pNewMem = p_check_shMalloc(size, __FILE__, __LINE__);

   count   = (size < pOldMem->user_bytes) ? size : pOldMem->user_bytes;

   (void *) memcpy(pNewMem, ptr, count);

   p_check_shFree(ptr, __FILE__, __LINE__);

   return pNewMem;
}

/*
 * ROUTINE: shMemRefCntrIncr()
 *
 * CALL:
 *   (void) shMemRefCntrIncr(void *addr)
 *   addr - Pointer to the address we want to increment the reference counter
 *          for
 * 
 * DESCRIPTION:
 *   shMemRefCntrIncr() increments the reference counter associated with a
 *   given memory address. Of course, it makes sure that the address passed
 *   in is indeed a "good" address, i.e. it has been allocated by shMalloc()
 *
 * RETURNS: 
 *   Nothing
 */
void shMemRefCntrIncr(void *addr)
{
   SH_MEMORY *mp;

   mp = (addr == NULL) ? NULL : (SH_MEMORY *)addr - 1;

   if (g_Serial_threshold == ~0L || mp->serial_number == g_Serial_threshold) { /*  */
       (*shMemInform)(mp->serial_number, mp);
   }

   if(bad_memblock(mp)) {
      shError("shMemRefCntrIncr: found memory region corrupted or "
	      "not allocated by shMalloc ("PTR_FMT")", addr);
      (*shMemInconsistency)(g_Serial_number, addr);
      return;
   }
   
   mp->reference++;

   return;
}

/*
 * ROUTINE: shMemRefCntrDecr()
 *
 * CALL:
 *   (void) shMemRefCntrDecr(void *addr)
 *   addr - Pointer to the address we want to decrement the reference counter
 *          for
 * 
 * DESCRIPTION:
 *   shMemRefCntrDecr() decrements the reference counter associated with a
 *   given memory address. It basically calls shFree() and let it do all the
 *   work.
 *
 * RETURNS: 
 *   Nothing
 */
void shMemRefCntrDecr(void *addr)
{
   shFree(addr);

   return;
}

/*
 * ROUTINE: p_shMemRefCntrGet()
 *
 * CALL:
 *   (unsigned int) p_shMemRefCntrGet(void *addr)
 *
 * DESCRIPTION:
 *   p_shMemRefCntrGet() retreives the reference counter associated with a 
 *   given memory address. 
 * 
 * CAVEATS:
 *   This is NOT a user-callable API. This API is to be used for Dervish 
 *   developers only!
 *
 * RETURNS: 
 *   n : where n >=0 and represents the reference counter associated with addr.
 */
unsigned int p_shMemRefCntrGet(void *addr)
{
   SH_MEMORY *mp;

   mp = (addr == NULL) ? NULL : (SH_MEMORY *)addr - 1;

   if(bad_memblock(mp)) {
      shError("p_shMemRefCntrGet: found memory block "
	      "corrupted or not allocated by shMalloc ("PTR_FMT")", addr);
      (*shMemInconsistency)(g_Serial_number, addr);
      return 0;
   }

   return mp->reference;
}

/*
 * ROUTINE: shIsShMallocPtr()
 *
 * CALL: 
 *   (int) shIsShMallocPtr(void *ptr)
 *   ptr - Address of memory area of interest
 *
 * DESCRIPTION:
 *   This function determines if a certain address has been allocated using
 *   shMalloc() or not? We basically seek four bytes _before_ the address
 *   passed and search for a constant defined by SH_MAGIC. shMalloc()
 *   initializes four bytes immedeately preceeding the address returned
 *   to the user with SH_MAGIC.
 *   The challenge is how do we determine if a given address is actually on
 *   the heap or not. For instance, there is nothing to stop the user from
 *   saying something like :
 *              shIsShMallocPtr(0x1) 
 *   and crashing the system. Note that the system can crash if address 
 *   passed to shIsShMallocPtr() is unusually small or at the beginning of
 *   the stack frame. We have to be carefull that the pointer is aligned.
 *
 *   Note that valid addresses have the last 2 bits 0 (on 32 bit machines,
 *   and, as far as I know, all current 64 bit machines too. RHL.
 *
 * RETURNS:
 *   1 : if ptr was allocated using shMalloc()
 *   0 : otherwise
 */

/*
 * Is P an aligned address?
 */
#define ALIGNED(P) ((void *)((long)(P) & ~03) == (P))

int shIsShMallocPtr(void *ptr)
{

   if(ptr == NULL) {
      return 0;
   } 
   else
     if(!ALIGNED(ptr)) {
       return(0);
     } 
     else {				/* see if we know about it */
       SH_MEMORY* pMemBlock;
       pMemBlock = (SH_MEMORY*) ( (char*)ptr - sizeof(SH_MEMORY) );
       return ( pMemBlock->poison == SH_MAGIC ? 1 : 0 );
    }
}

/*
 * Make pElem the new head of the linked list
 */
static void
insList(SH_MEMORY **pHead,		/* pointer to head of linked list */
	SH_MEMORY *pElem)		/* element to add */
{
   pElem->pNext = *pHead;
   *pHead = pElem;

   g_Fmp_user_bytes += pElem->actual_bytes;

   return;
}

/*
 *   Remove and return the requested element from a linked list pointed
 * to by *pHead
 *
 * RETURNS:
 *   Pointer to the desired block's bookeeping header, or NULL
 */
static SH_MEMORY *
remList(SH_MEMORY **pHead,		/* pointer to head of list */
	SH_MEMORY *p)			/* element specification; if NULL
					   return the head, otherwise
					   the element _following_ p */
{
   SH_MEMORY  *m;

   if (*pHead == NULL)
       return NULL;

   if (p == NULL)  {
      m = *pHead;
      *pHead = m->pNext;
   } else  {
      m = p->pNext;
      p->pNext = m->pNext;
   }
   m->pNext = NULL;

   g_Fmp_user_bytes -= m->actual_bytes;

   return m;   
}

/*
 * ROUTINE: round_up()
 * 
 * CALL:
 *   (static size_t) round_up(size_t size)
 *    size - Size of the actual malloc request
 *
 * DESCRIPTION:
 *   round_up() rounds the user's actual malloc request to the next power of
 *   2 except in two situations:
 *     a) If the user's actual malloc request is less than or equal to 
 *        MIN_BUCKET_SIZE, the request is rounded up to MIN_BUCKET_SIZE
 *     b) If the user's actual malloc request is greater than or equal to
 *        MAX_BUCKET_SIZE, a constant offset is added to that request, 
 *        and the result is returned
 *
 * RETURNS:
 *   A rounded up value which is greater then or equal to the actual malloc
 *   request.
 */
static size_t
round_up(size_t size)
{
   unsigned long return_size;
   unsigned long lsize;			/* copy of size */

   if (size <= MIN_BUCKET_SIZE) {
       return(MIN_BUCKET_SIZE);
   } else if (size >= MAX_BUCKET_SIZE) {
       return(size + 256);
   }

   return_size = 1;
   lsize = size;
      
   while((lsize >>= 1) != 0) {
      return_size <<= 1;
   }
   if(size != return_size) return_size <<= 1;
     
   return return_size;
}

/*
 * ROUTINE: getMemPoolIndex()
 * 
 * CALL:
 *   (static int) getMemPoolIndex(const size_t size)
 *    size - Hash key to the bucket we're interested in
 *
 * DESCRIPTION:
 *   getMemPoolIndex() gets the index of the appropriate bucket to check
 *   for in the FMP. The correct bucket index depends on the parameter
 *   size.
 *
 * RETURNS:
 *   n : where 0 <= n < MAX_BUCKETS
 */
static
int getMemPoolIndex(const size_t size)
{
   int i;

   if (size >= MAX_BUCKET_SIZE)
       return NBUCKETS - 1;

   /*
    * Brute force search; it's a small table
    */
   for (i = 0; i < NBUCKETS; i++)
        if (g_Fmp[i].size == size)
            break;

   shAssert(i != NBUCKETS);

   return i;
}

/*
 * ROUTINE: search_Fmp()
 * 
 * CALL:
 *   (static SH_MEMORY *) search_Fmp(const size_t size)
 *    size - size of a block we're interested in
 *
 * DESCRIPTION:
 *   search_Fmp() searches the FMP to find a free block of size bytes long.
 *   If a suitable block is found, it is removed from the appropriate FMP
 *   bucket. Note that since the last bucket in the FMP is a catchall
 *   bucket, it will have free memory blocks >= MAX_BUCKET_SIZE bytes.
 *
 *   We _used_ to use a first-fit algorithm for the last bucket, but it
 *   causes memory usage to grow and grow if users ask for chunks in
 *   2 slightly different sizes.  So we have to make an exhaustive search
 *   through the last bucket for the best-fitting block (larger than or
 *   equal to the requested size).
 *
 * RETURNS:
 *   A pointer to a free block : on success
 *   NULL                      : otherwise
 */
static 
SH_MEMORY *search_Fmp(const size_t size)
{
   int indx;

   indx = getMemPoolIndex(size);

   if (indx < NBUCKETS - 1)
       return remList(&(g_Fmp[indx].pMemory_blocks), NULL);
   else  {
       int first = 1;
       size_t left_over;
       size_t min_left_over = 0;	/* actually it's initialised when
					   first == 1 */
       SH_MEMORY *p, *t, *prev_p;

       p = g_Fmp[indx].pMemory_blocks;
       t = NULL;

       /* 
        * prev_p will point to the block _before_ the one we want.
        * However, if the one we want is the head of the linked list,
        * then it will still be == NULL.
        */
       prev_p = NULL;

       /*
        * we look through the linked list of blocks for the one
        * which is closest to "size" bytes -- but it must be at least "size"
        * bytes.  
        *
        * If we find no such block, we return NULL.
        * If we do find such a block, we want to call remList with a pointer
        *      to the block _before_ the good one; NOT with a pointer to 
        *      the block itself (since it's a singly-linked list, that's the
        *      only way we can remove the good block from the list).
        *      If the good block is the very first one in the list, then
        *      we actually do want to call "remList" with second arg = NULL.
        *
        * Note that the variable 'first' has 2 functions:
        *
        *   1. It allows us to know if the current candidate block is the
        *      very first one with >= "size" bytes.  If it were possible to
        *      initialize 'min_left_over' to some negative value, we could 
        *      use that to signal that no good block had yet been found ...
        *      but we can't set 'min_left_over' to negative numbers.  So we
        *      have to rely on 'first'.
        *
        *   2. If, after we come out of the loop over the free blocks,
        *      first == 1, then we didn't find any candidate blocks.
        *      On the other hand, if first == 0, then we did find at least
        *      one.  We can't determine if a block was found by just 
        *      asking if prev_p == NULL, because if the only good block
        *      was the very first one, then prev_p _will_ be equal to NULL.
        */

       for (; p != NULL; p = p->pNext)  {
            if (p->actual_bytes >= size) {
                left_over = p->actual_bytes - size;
                if ((first == 1) || (left_over < min_left_over)) {
                   first = 0;
                   prev_p = t;
		   if(left_over == 0) {
		      break;
		   }
                   min_left_over = left_over; 
                }
            }
            t = p;
       }
       if (first == 1)
           return NULL;
       else
           return remList(&(g_Fmp[indx].pMemory_blocks), prev_p);
   }
}

/*****************************************************************************/
/*
 * ROUTINE: shMemEmptyCB
 * 
 * CALL:
 *   (void) shMemEmptyCB(void *(*funcp)(size_t nbyte))
 *    funcp   Pointer to function to be called when dervish fails to
 *            allocate memory. The argument, nbytes, is the number of
 *            bytes requested
 *            if NULL, reinstate default handler
 *
 * DESCRIPTION:
 *   shMemEmptyCB() is a function that registers a callback-function which
 *   is called when shMalloc fails
 *
 * RETURNS:
 *   Nothing
 */
void
shMemEmptyCB(void *(*funcp)(size_t nbyte))
{
   if(funcp == NULL) {
      funcp = defaultMemEmptyFunc;
   }
   shMemEmpty = funcp;
}

/*
 * ROUTINE: shMemInconsistencyCB()
 * 
 * CALL:
 *   (void) shMemInconsistencyCB(void (*funcp)(unsigned long thresh,
 *                                             const SH_MEMORY *m))
 *    funcp   - Pointer to function to be called an inconsistency is detected.
 *              if NULL, reinstate default handler
 *
 * DESCRIPTION:
 *   shMemInconsistencyCB() is a function that registers a callback-function
 *   which is called when Dervish detects a problem in the memory that it owns
 *
 *   The callback-function has two arguments, the current value of
 *   g_Serial_number and the bad Memory block
 *
 *   N.b. The default handler calls shFatal().
 *
 * RETURNS:
 *   Nothing
 */
void
shMemInconsistencyCB(void (*funcp)(unsigned long, const SH_MEMORY *))
{
   if(funcp == NULL) {
      funcp = shMemInconsistency;
   }
   shMemInconsistency = funcp;
}

/*
 * ROUTINE: shMemSerialCB()
 * 
 * CALL:
 *   (void) shMemSerialCB(const unsigned long trigger,
 *			  void (*funcp)(unsigned long thresh, const SH_MEMORY *m))
 *    trigger - Value of g_Serial_number on which funcp will be called
 *    funcp   - Pointer to function to be called when g_Serial_number
 *              equals trigger
 *              If NULL, reinstate default handler
 * The argument, m, is the block that has just been allocated.
 *
 * DESCRIPTION:
 *   shMemSerialCB() is a function that registers a callback-function which
 *   is called when g_Serial_number reaches a certain threshold (trigger).
 *   The callback-function has two arguments, the threshold triggered and
 *   the Memory block that has just been allocated
 *
 * If trigger is all-bits-on (i.e. ~0); the callback will _always_ be called
 *
 * RETURNS:
 *   Nothing
 */
void shMemSerialCB(const unsigned long trigger,
		   void (*funcp)(unsigned long, const SH_MEMORY *))
{
   if(funcp == NULL) {
      funcp = shMalloc_callback;
   }
   g_Serial_threshold = trigger;
   shMemInform = funcp;
}

/*
 * ROUTINE: shMemSerialFreeCB()
 * 
 * CALL:
 *   (void) shMemSerialFreeCB(const unsigned long trigger,
 *			  void (*funcp)(unsigned long thresh, const SH_MEMORY *m))
 *    trigger - Value of g_Serial_number on which funcp will be called
 *    funcp   - Pointer to function to be called when g_Serial_number
 *              equals trigger
 *              If NULL, reinstate default handler
 * The argument, m, is the block that has just been allocated.
 *
 * DESCRIPTION:
 *   shMemSerialFreeCB() is a function that registers a callback-function which
 *   is called when a memory block with a certain serial number is freed;
 *   The callback-function has two arguments, the desired serail number and
 *   the Memory block that is to be freed
 *
 * If trigger is all-bits-on (i.e. ~0); the callback will _always_ be called
 *
 * RETURNS:
 *   Nothing
 */
void shMemSerialFreeCB(const unsigned long trigger,
		       void (*funcp)(unsigned long, const SH_MEMORY *))
{
   if(funcp == NULL) {
      funcp = shFree_callback;
   }
   g_Serial_free_threshold = trigger;
   shMemFreeInform = funcp;
}

/*
 * ROUTINE: shMemFreeBlocks()
 * 
 * CALL: 
 *   (void) shMemFreeBlocks(const unsigned low_bound, const unsigned hi_bound,
 *                          void (*funcp)(void *))
 *   low_bound - Lower bound serial number for the blocks to free
 *   hi_bound  - Upper bound serial number for the blocks to free
 *   funcp     - Pointer to a user-desired function to be called. May be NULL,
 *               in which case shFree() is called to "free" the blocks.
 *
 * DESCRIPTION:
 *   shFreeMemBlocks() "frees" memory blocks bounded by low_bound and hi_bound.
 *   low_bound can be equal to hi_bound, in which case only one memory block is
 *   freed. If funcp is NULL, then shFreeMemBlocks() calls shFree() to free the
 *   memory blocks. In this case, the blocks are taken out of the AMP and 
 *   inserted into their appropriate FMP buckets.
 *   If funcp != NULL, then the memory block is passed as a parameter to 
 *   the function pointed to by funcp.
 *
 * RETURNS:
 *   0 : On success
 *   1 : if AMP is empty
 *   2 : if low_bound >= hi_bound
 */
int shMemFreeBlocks(const unsigned long low_bound,
                    const unsigned long hi_bound,
                    void (*funcp)(void *))
{
   register SH_MEMORY *m, *t;
   void (*pFreeFunc)(void *);
   
   m = g_Amp;
   if (funcp)
       pFreeFunc = funcp;
   else
       pFreeFunc = shFree;

   if (m == NULL)
       return 1;

   if (low_bound == hi_bound)  {
       /*
        * Free only one memory block 
        */
       for (; m != NULL; m = m->pNext)
            if (m->serial_number == low_bound)  {
                (*pFreeFunc)(m->pUser_memory);
                break;
	    }
       /*
        * Notice that we return 0 (success) even if the user specified an 
        * invalid memory block, possibly a block number > g_Serial_number. 
        * Although this is not really a success, it is not important enough 
        * to garnish it's own return code. It does not great harm.
        */
        return 0;

   } else if (low_bound > hi_bound)
        return 2;

   /*
    * User specified a range of blocks to free...so let's do it
    */
   for (; m != NULL; )
        if (m->serial_number >= low_bound && m->serial_number <= hi_bound)  {
            /*
             * Save the value of the next memory element since shFree() 
             * will actually take a memory element out of the AMP. Thus,
             * after it returns, we can no longer safely reference m.
             * Additionally, we cannot be sure that the user-defined
             * function (if funcp != NULL) will not do weird things to the
             * memory block pointed to by m. It does not hurt to be
             * cautious.
             */
            t = m->pNext;
            (*pFreeFunc)(m->pUser_memory);
            m = t;
	} else
            m = m->pNext;

   return 0;
}

/*
 * ROUTINE: shMemTraverse()
 *
 * CALL:
 *   (short) shMemTraverse(void (*funcp)(SH_MEMORY *const))
 *   funcp - Pointer to user callable function. That function accepts one
 *           parameter, a pointer to a constant Memory structure. The
 *           functions SHOULD NOT attempt to modify it's argument.
 *
 * DESCRIPTION:
 *   shMemTraverse() walks the AMP and calls a user function for each
 *   element in the AMP.
 *
 * RETURNS: 
 *   1 : on success
 *   0 : otherwise (maybe the AMP is empty)
 */
short shMemTraverse(void (*funcp)(SH_MEMORY *const))
{
   register SH_MEMORY *m;

   m = g_Amp;

   if (m == NULL)
       return 0;

   for (; m != NULL; m = m->pNext)
        funcp(m);

   return 1;
}

/*****************************************************************************/
/*
 * a function to sort pointers. Note that returning (a - b) is illegal C
 */
static int
compar(const void *pa, const void *pb)
{
   const void *a = *(void **)pa;
   const void *b = *(void **)pb;

   if(a < b) {
      return(-1);
   } else if(a == b) {
      return(0);
   } else {
      return(1);
   }
}

/*
 * <AUTO EXTRACT>
 *
 * Defragment the free memory list.
 *
 * You have two options; if free_to_os is true, simply go through the
 * free lists freeing all allocated blocks back to the O/S, so as to give it
 * a chance to defragment for us. To use this option you must _not_
 * have called shMemBlocksizeSet or chaos will ensue.
 *
 * Otherwise, go through the FMP looking for adjacent blocks which
 * are then merged together, and if possible returned to g_mallocList
 * This will only work if large blocks have been allocated from the
 * O/S; i.e. if you called shMemBlocksizeSet with a largish argument
 * (a few Mby?).
 *
 * Return 0 if all is well, or -1 in case of trouble
 */
int
shMemDefragment(int free_to_os)		/* return free memory to O/S? */
{
   SH_MEMORY *base;			/* base of a merged free block */
   SH_MEMORY **blocks;			/* all freed blocks */
   int i, j;
   int n = 0;				/* number of blocks freed */
   SH_MEMORY *ptr, *next;		/* used to traverse free list */
   size_t size;				/* size of a block */

   if(free_to_os) {
      if(g_Malloc_Minsize != 0) {
	 shErrStackPush("shMemDefragment: "
		       "You can only use free_to_os if g_Malloc_Minsize == 0");
	 return(-1);
      }
      
      for(i = 0; i < NBUCKETS; i++) {
	 ptr = g_Fmp[i].pMemory_blocks;
	 while(ptr != NULL) {
	    next = ptr->pNext;
	    free(ptr); n++;
	    ptr = next;
	 }
	 g_Fmp[i].pMemory_blocks = NULL;
      }
      g_Actual_bytes -= g_Fmp_user_bytes + n*sizeof(SH_MEMORY);
      g_Fmp_user_bytes = 0;

      return(0);
   }
/*
 * We really do have to defragment ourselves. Start by counting blocks
 */
   for(i = 0; i < NBUCKETS; i++) {
      for(ptr = g_Fmp[i].pMemory_blocks; ptr != NULL; ptr = ptr->pNext) {
	 n++;
      }
   }
   if(n == 0) {
      return(0);
   }
/*
 * move them all onto a single list
 */
   blocks = alloca(n*sizeof(SH_MEMORY *));

   for(i = j = 0; i < NBUCKETS; i++) {
      for(ptr = g_Fmp[i].pMemory_blocks; ptr != NULL; ptr = ptr->pNext) {
	 blocks[j++] = ptr;
	 ptr->actual_bytes += sizeof(SH_MEMORY); /* i.e. _total_ size */
      }
      g_Fmp[i].pMemory_blocks = NULL;
   }
   g_Fmp_user_bytes = 0;
/*
 * merge contiguous blocks of memory. We'll end up with j blocks at the
 * beginning of the list which contain all the memory
 */
   qsort(blocks, n, sizeof(SH_MEMORY *), compar);

   i = j = 0;
   base = blocks[j++]; i++;
   size = base->actual_bytes;
   for(; i < n; i++) {
      if((char *)blocks[i] == (char *)base + size) { /* merge the blocks */
	 size += blocks[i]->actual_bytes;
	 blocks[i]->actual_bytes = 0;
      } else {				/* start a new merged block */
	 base->actual_bytes = size;
	 base = blocks[j++] = blocks[i];
	 size = base->actual_bytes;
      }
   }
   base->actual_bytes = size;
   n = j;
/*
 * try to put those blocks back onto g_mallocList
 */
   for(i = 0; i < n; i++) {
      base = blocks[i];
      for(ptr = g_mallocList; ptr != NULL; ptr = ptr->pNext) {
	 if((char *)ptr + ptr->user_bytes == (char *)base) {
	    ptr->user_bytes += base->actual_bytes;
	    g_Actual_bytes -= base->actual_bytes;
	    base->actual_bytes = 0;
	    break;
	 }
      }
   }
/*
 * put any leftovers back into the regular g_Fmp buckets. Because
 * we may have merged blocks together, there is no guarantee that
 * the allocated block is the size of one of the buckets, and if
 * it isn't it should go in the next _smallest_ bucket, as shMalloc
 * will assume that the full bucket size is available.
 */
   for(i = 0; i < n; i++) {
      base = blocks[i];
      if(base->actual_bytes == 0) continue;

      base->actual_bytes -= sizeof(SH_MEMORY);
      if((j = round_up(base->actual_bytes)) != base->actual_bytes &&
	 base->actual_bytes < MAX_BUCKET_SIZE) {
	 j /= 2;
      }
      j = getMemPoolIndex(j);
      base->pPrev = base->pNext = NULL;

      insList(&(g_Fmp[j].pMemory_blocks), base);
   }

   return(0);
}

/*****************************************************************************/
/*
 * ROUTINE: shMemStatsPrint()
 *
 * CALL:
 *   (void) shMemStatsPrint(void)
 *
 * DESCRIPTION:
 *   shMemStatsPrint() prints some interesting statistics about the memory
 *   being utilized by the program.
 *
 * RETURNS: 
 *   Nothing
 */
void shMemStatsPrint(void)
{
   float flt1, flt2;

   fprintf(stdout, "\n");
   fprintf(stdout,
          "Number of memory allocation requests:     %ld\n", g_Num_mallocs);
   fprintf(stdout,
          "Number of memory de-allocation requests:  %ld\n", g_Num_frees);
   fprintf(stdout,
          "Total bytes currently in use:             %ld\n", g_Amp_user_bytes);
   fprintf(stdout,
          "Total bytes in Free Memory Pool:          %ld\n", g_Fmp_user_bytes);
   fprintf(stdout,
          "Total bytes allocated by malloc():        %ld\n", g_Total_bytes);

   if (g_Num_mallocs == 0)
       flt1 = flt2 = 0.0;
   else  {
       flt1 = ((float)g_Malloc_Fmp/(float)g_Num_mallocs) * 100.00;
       flt2 = ((float)g_Malloc_system/(float)g_Num_mallocs) * 100.00;
   }

   fprintf(stdout,
          "Percentage of memory allocation requests\n"
          "  satisfied from Free Memory Pool:        %-6.2f%%\n", flt1);
   fprintf(stdout,
          "Percentage of memory allocation requests\n"
          "  satisfied from the Operating System:    %-6.2f%%\n", flt2);
   fprintf(stdout, "\n");

   return;
}

/*****************************************************************************/
/*
 * Even when we have CHECK_LEAKS defined, some files may not have been
 * recompiled; provide the raw functions shMalloc etc. for them
 */
#if defined(CHECK_LEAKS)
#undef shMalloc
#undef shRealloc
#undef shFree

void *
shMalloc(size_t size)
{
   return(p_check_shMalloc(size,"(unknown)",0));
}

void *
shRealloc(void *ptr, size_t size)
{
   return(p_check_shRealloc(ptr,size,"(unknown)",0));
}

void
shFree(void *ptr)
{
   p_check_shFree(ptr,"(unknown)",0);
}
#endif

/*****************************************************************************/
/*
 * Routines to check the consistency of the allocated and/or free memory arena
 *
 * N.b. If the block wasn't allocated by dervish, it will appear corrupted
 */
static int
bad_memblock(const SH_MEMORY *m)
{
   if(m == NULL) {
      shError("p_shMemCheck: NULL memory block");
      return(1);
   }
   
   if(!ALIGNED(m)) {
      shError("p_shMemCheck: non-aligned memory block");
      return(1);
   }
   
   if(m->poison != SH_MAGIC) {
      shError("p_shMemCheck: memory block %d is corrupted",
	      m->serial_number);
      return(1);
   }

   if(m->pPrev != NULL && m->pPrev->poison != SH_MAGIC) {
      shError("p_shMemCheck: memory block %d's prev pointer, "
	      "(or the previous block), is corrupted", m->serial_number);
      return(1);
   }

   if(m->pNext != NULL && m->pNext->poison != SH_MAGIC) {
      shError("p_shMemCheck: memory block %d's next pointer, "
	      "(or the next block), is corrupted", m->serial_number);
      return(1);
   }

   if(m->user_bytes > m->actual_bytes) {
      shError("p_shMemCheck: memory block %d has too few bytes allocated, "
	      "%d v. %d", m->serial_number, m->actual_bytes, m->user_bytes);
      return(1);
   }

   return(0);
}

/*
 * <AUTO EXTRACT>
 *
 * Calling this routine checks all the memory known to dervish; this may
 * take some time. Note that there is NO GUARANTEE that a trashed heap
 * will not lead to SEGVs in this code --- but of course this will only
 * happen if you have a bug anyway.
 *
 * The number of bad blocks detected is returned
 *
 * It's possible to call this from within a memory callback function
 * to achieve any desired granularity of memory checking, e.g.
 *
 * static void
 * malloc_check(unsigned long thresh, const SH_MEMORY *mem)
 * {
 *    static int abort_on_error = 1;
 *    static int check_allocated = 1;
 *    static int check_free = 1;
 *    static int frequency = 10;
 * 
 *    shAssert(mem != NULL);
 * 
 *    if(frequency > 0) {
 *       p_shMemCheck(check_allocated, check_free, abort_on_error);
 * 
 *       shMemSerialCB(thresh + frequency, malloc_check);
 *    }
 * }
 *
 * After which, calling
 *    shMemSerialCB(1, malloc_check);
 * will enable checking of the heap
 */
int
p_shMemCheck(int check_allocated,	/* check allocated blocks? */
		int check_free,		/* check free blocks? */
		int abort_on_error)	/* abort on first error? */
{
   int i;
   const SH_MEMORY *m;
   int nbad = 0;			/* number of bad blocks */
   const SH_MEMORY *p;

   if(check_allocated) {
      for(m = g_Amp; m != NULL; m = m->pNext) {
	 if(bad_memblock(m)) {
	    nbad++;
	    shError("Problem in allocated memory detected");
	    if(abort_on_error) {
	       shFatal("aborting");
	    }
	 }
      }
   }
   
   if(check_free) {
      for (i = 0; i < NBUCKETS; i++) {
	 for(p = g_Fmp[i].pMemory_blocks; p != NULL; p = p->pNext) {
	    if(bad_memblock(p)) {
	       nbad++;
	       shError("Problem in free memory detected (%dby buckets)",
								g_Fmp[i].size);
	       if(abort_on_error) {
		  shFatal("aborting");
	       }
	    }
	 }
      }

      for(p = g_mallocList; p != NULL; p = p->pNext) {
	 if(bad_memblock(p)) {
	    nbad++;
	    shError("Problem in non-allocated free memory detected");
	    if(abort_on_error) {
	       shFatal("aborting");
	    }
	 }
      }
   }
   
   return(nbad);
}


/*****************************************************************************/
/*
 * The two functions below are basically debugging aids. They print (on
 * stdout) the AMP and the indexed FMP.
 */
#ifdef DEBUG_MEM

void pr_Amp(void)
{
   register SH_MEMORY *m;

   m = g_Amp;

   if (m == NULL)  {
       fprintf(stdout, "   AMP is empty\n");
       return;
   }


   fprintf(stdout, "   AMP----->\n");
   do {
          fprintf(stdout, "      This : (%p)  Next : (%p)  Prev : (%p)\n",
                  m, m->pNext, m->pPrev);
          fprintf(stdout, "         Serial : %ld  Ref    : %d\n", m->serial_number,
                  m->reference);
          fprintf(stdout, "         User   : %ld  Actual : %ld\n", m->user_bytes,
                  m->actual_bytes);
          fprintf(stdout, "         Poison : %x   Memory : %p\n", m->poison,
                  m->pUser_memory);
          m = m->pNext;
   } while (m != NULL);

   return;
}

void pr_Fmp(const int indx)
{
   register SH_MEMORY *p;

   p = g_Fmp[indx].pMemory_blocks;

   if (p == NULL)  {
       fprintf(stdout, "   FMP[%d] is empty\n", indx);
       return;
   }

   fprintf(stdout, "   FMP[%d]----->\n", indx);
   for (; p != NULL; p = p->pNext)  {
        SH_MEMORY *m;

        fprintf(stdout, "      Data : %p  Next : %p\n", p->pData, p->pNext);
        m = (SH_MEMORY *)p->pData;
        fprintf(stdout, "      This : (%p)  Next : (%p)  Prev : (%p)\n",
                m, m->pNext, m->pPrev);
        fprintf(stdout, "         Serial : %ld  Ref    : %d\n", m->serial_number,
                m->reference);
        fprintf(stdout, "         User   : %ld  Actual : %ld\n", m->user_bytes,
                m->actual_bytes);
        fprintf(stdout, "         Poison : %x   Memory : %p\n", m->poison,
                m->pUser_memory);
   }

   return;
}
#endif
