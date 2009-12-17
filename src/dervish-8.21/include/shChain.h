#ifndef _SHCHAIN_H
#define _SHCHAIN_H

/*
 * A general purpose linked list handler. 
 *
 * Vijay K. Gurbani
 * (C) 1994 All Rights Reserved 
 * Fermi National Accelerator Laboratory
 * Batavia, Illinois USA
 *
 * ----------------------------------------------------------------------------
 *
 * Some terminology:
 *   chain  - a synonym for a linked list 
 *   chain element - an element on the linked list
 *   cursor - a context device associated with a chain
 *
 * This package provides a generic (doubly) linked list handler. In the
 * spirit of Object Orientation, data abstraction and encapsulation are
 * enforced by keeping the internal data structures private and providing
 * functions (or methods) to access and/or modify these data structures.
 */

#include "shCArray.h"
#include "shCVecExpr.h"

/*
 * Some important items...
 */

#define HEAD                 0
#define TAIL              (unsigned int) ~HEAD

typedef enum { 
   BEFORE, 
   AFTER 
} CHAIN_ADD_FLAGS;

typedef enum { 
   PREVIOUS, 
   THIS, 
   NEXT 
} CHAIN_WALK_FLAGS;

typedef enum { 
   NOT_ALLOCATED, 
   ALLOCATED
} CURSOR_STATUS;

/*
 * Data Structures:
 * 1) Chain element : Each element on a chain (STAR, REGION, MASK, etc)
 */
typedef struct tagCHAIN_ELEM  {
   void   *pElement;       /* Pointer to actual element (STAR, MASK, etc) */
   TYPE   type;            /* Schema type of element */
   struct tagCHAIN_ELEM *pPrev;   
   struct tagCHAIN_ELEM *pNext;
 } CHAIN_ELEM;             /* pragma USER */

/*
 * 2) Cursor: a cursor holds state (context) information for a chain
 */
typedef struct tagCURSOR  {
   CHAIN_ELEM     *pChainElement; /* The cursor element pointer */
   CHAIN_ELEM     *pNext;         /* Next element */
   CHAIN_ELEM     *pPrev;         /* Previous element */
   short          gotFirstElem;   /* Flag set if first element visited */
   CURSOR_STATUS  status;         /* Status of the cursor */
} CURSOR;                         /* pragma SCHEMA */

/*
 * 3) Chain : The actual chain itself. 
 *    Note the field type. This field gives an indication of the type of
 *    chain itself. If set to anything other then GENERIC, it signifies 
 *    that the chain contains homogeneous elements. If set to GENERIC,
 *    the chain acts like a heterogeneous container.
 */
typedef struct tagCHAIN  {
   unsigned int nElements;   /* Number of elements on the chain */
   TYPE         type;
   CHAIN_ELEM   *pFirst;
   CHAIN_ELEM   *pLast;
   CHAIN_ELEM   *pCacheElem;    /* Pointer to the cached element */
   unsigned int cacheElemPos;   /* It's (absolute) position on the chain */
   CHAIN_ELEM **index;			/* index array for chain */

   CURSOR       **chainCursor;   /* Associated cursors */
   int max_cursors;			/* number of cursors allocated */
} CHAIN;                     /* pragma USER */
   
/*
 * Chains that act as heterogeneous containers set their type field to be
 * GENERIC. This structure only serves as a valid schema type for GENERIC
 * chains. This is so that shTypeGetFromName("GENERIC") works transparently
 * (without me having to modify the actual function itself).
 */

typedef struct tagGENERIC  {
   void *dummy;
} GENERIC;         /* pragma SCHEMA */

typedef int CURSOR_T;
#define INVALID_CURSOR -1
/*
 * Possible types of index to build
 */
#define SH_CHAIN_INDEX_BY_POS 0		/* index by position in chain */

/*
 * Public interface to the linked list (chain) routines
 */
#ifdef __cplusplus
extern "C"
{
#endif

void p_chainDel(CHAIN *pChain);
void p_chainElementDel(CHAIN *pChain);

CHAIN_ELEM *shChain_elemNew(const char *pType);
void     shChain_elemDel(CHAIN_ELEM *celem);
CHAIN    *shChainNew(const char *pType);
CHAIN    *shChainNewBlock(const char *a_type, int a_num);
void     p_shChainInit(CHAIN *a_pChain, const char *a_type);
CHAIN    *shChainCopy(const CHAIN *pSrc);
void     shChainDel(CHAIN *pChain);
void     *shChainElementGetByPos(const CHAIN *pChain, unsigned int pos);
void     *shChainElementRemByPos(CHAIN *pChain, unsigned int pos);
void     *shChainElementRemByCursor(CHAIN *pChain, const CURSOR_T crsr);
void     *shChainWalk(const CHAIN *pChain, const CURSOR_T crsr, 
                      const CHAIN_WALK_FLAGS whither);
void *shChainElementChangeByPos(CHAIN *pChain, unsigned int pos, void *newval);
void * shChainElementChangeByCursor(CHAIN *pChain, const CURSOR_T crsr,
				    void *newval);
RET_CODE shChainIndexMake(const CHAIN *pChain, int type);
void shChainIndexDel(const CHAIN *pChain);
int shChainIsIndexed(const CHAIN *chain);
unsigned int shChainSize(const CHAIN *pChain);
unsigned int shChainCursorCount(const CHAIN *pChain);
TYPE     shChainTypeGet(const CHAIN *pChain);
TYPE     shChainTypeSet(CHAIN *pChain, const char *type);
TYPE     shChainElementTypeGetByPos(const CHAIN *pChain, unsigned int pos);
TYPE     shChainTypeDefine(const CHAIN *pChain);
TYPE     shChainElementTypeGetByCursor(const CHAIN *pChain, 
                                       const CURSOR_T crsr);
RET_CODE shChainCursorDel(const CHAIN *pChain, const CURSOR_T crsr);
RET_CODE shChainSort(CHAIN *pChain, const char *pField, int increasing);
RET_CODE shChainCursorSet(const CHAIN *pChain, const CURSOR_T crsr, 
                           unsigned int pos);
RET_CODE shChainCompare(CHAIN *chain1, char *row1Name, char *col1Name,
                        CHAIN *chain2, char *row2Name, char *col2Name,
                        float minimumDelta, 
                        CHAIN *closestChain, VECTOR *vMask);
RET_CODE shChainCompare2(CHAIN *chain1, char *row1Name, char *col1Name,
			 CHAIN *chain2, char *row2Name, char *col2Name,
			 CHAIN *closestChain);

RET_CODE shChainJoin(CHAIN *pSrc, CHAIN *pTarget);
RET_CODE shChainElementAddByPos(CHAIN *pChain, void *pDdata, 
                                const char *a_dataType, unsigned int n, 
                                const CHAIN_ADD_FLAGS where);
RET_CODE shChainElementAddByCursor(CHAIN *pChain, void *pData, 
                                   const char *a_dataType, const CURSOR_T crsr,
                                   const CHAIN_ADD_FLAGS where);
RET_CODE shChainCursorCopy(const CHAIN *, const CURSOR_T, CURSOR_T *);
CURSOR_T shChainCursorNew(const CHAIN *pChain);

#ifdef __cplusplus
}
#endif

/*
 * This a global chain error indicator. Functions for the chain routines
 * set this global error indicator to the appropriate value on an error.
 */
#ifdef CHAIN_MAIN
    RET_CODE g_shChainErrno;
    void     *INVALID_CHAIN_ELEM = NULL;
#else
    extern RET_CODE g_shChainErrno;
    extern void     *INVALID_CHAIN_ELEM;
#endif

#endif
