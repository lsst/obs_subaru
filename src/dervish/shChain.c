/*
 * shChain.c - Implementation of generic chain (linked list) routines.
 *
 * Vijay K. Gurbani
 * (C) 1994 All Rights Reserved
 * Fermi National Accelerator Laboratory
 * Batavia, Illinois USA
 *
 * ----------------------------------------------------------------------------
 *
 * FILE :
 *   shChain.c
 *
 * ABSTRACT :
 *   This file contains generic linked list routines. These are doubly
 *   linked lists. A user will typically pass a pointer to the element
 *   (s)he wants to maintain on the linked list. It is assumed that the
 *   user has allocated memory for that element and has initialized it
 *   as needed. 
 *
 *   There are two classes of operations this package provides:
 *   o  creating and deleting linked lists 
 *   o  maintaining (add/remove/get) elements on the linked lists.
 *      We provide two methods for this purpose:
 *      o maintaining elements using indexes
 *      o maintaining elements using a cursor (A cursor can be thought
 *        as a context sensitive device that contains state information
 *        for the linked list being maintained. All maintenence is done
 *        via the cursor)
 *   
 *   Linked lists are refered to as "Chains" throught the code. This is done
 *   so as to 
 *      1) avoid emotional baggage associated with the words "lists" 
 *         and "links"
 *      2) distinguish our linked lists from the TCL primitive called "list".
 *
 * ENTRY                         SCOPE   DESCRIPTION
 * ----------------------------------------------------------------------------
 * shChain_elemNew               public  Create a new CHAIN_ELEM
 * shChain_elemDel               public  Delete a CHAIN_ELEM
 * shChainNew                    public  Create an empty chain
 * shChainDel                    public  Delete a chain
 * shChainDestroy                public  Delete a chain and objects on a chain 
 * shChainElementAddByPos        public  Add an element by position on a chain
 * shChainElementTypeGetByPos    public  Get a chain elements TYPE
 * shChainElementGetByPos        public  Get the nth chain element 
 * shChainElementRemByPos        public  Remove the nth chain element
 * shChainSize                   public  Get the chain size
 * shChainTypeGet                public  Get the _chain's_ TYPE
 * shChainTypeSet                public  Forces a chain's TYPE to a new one
 * shChainJoin                   public  Join two chains
 * shChainCopy                   public  Copy a chain
 * shChainTypeDefine             public  Define a chain's TYPE
 * shChainCursorNew              public  Get a new cursor to a chain
 * shChainCursorDel              public  Delete existing cursor for the chain
 * shChainCursorCount            public  Get the number of cursors in use
 * shChainCursorSet              public  Initialize a chain cursor 
 * shChainElementAddByCursor     public  Add a new element using the cursor
 * shChainElementRemByCursor     public  Remove the element using the cursor
 * shChainElementTypeGetByCursor public  Get the TYPE of the element under the
 *                                          cursor
 * shChainWalk                   public  Traverse a chain, modifying it's
 *                                          state, if needed
 * shChainSort                   public  Sorts a chain based on a field
 * shChainCompare                public  Compare two chains based on "closest"
 *                                          members.
 * shChainCursorCopy             public  Copy a cursor
 * getElementN                   private Low level routine to get Nth element
 * removeElement                 private Low level routine to remove  element 
 *                                          from the chain
 * insertElement                 private Low level routine to insert element 
 *                                          onto the chain
 * 
 */

#include <unistd.h>
#include <string.h>
#include <math.h>

#include "shCSchema.h"
#include "shCUtils.h"
#include "shCAssert.h"
#include "shCErrStack.h"
#include "shCGarbage.h"
#include "prvt/shGarbage_p.h"

#define CHAIN_MAIN
#include "shChain.h"
#undef CHAIN_MAIN

#if !defined(NOTCL)
#include "shCHg.h"
#endif
#include "shCVecExpr.h"
#include "shCVecChain.h"

/*
 * Local function prototypes (visible only to this source file)
 */
static CHAIN_ELEM *getElementN(const CHAIN *, unsigned int);
static void insertElement(CHAIN *, CHAIN_ELEM *, CHAIN_ELEM *, 
                          const CHAIN_ADD_FLAGS);
static void removeElement(CHAIN *, CHAIN_ELEM *);
static void initPointers(void);

#ifdef DEBUG
   static void GetMemory(void *, size_t, char *, int);
   static void FreeMemory(void *, char *, int);
#endif
/*
 * routines to create/destroy CURSORs themselves, not CURSOR_Ts
 */
static CURSOR *
p_shChainCursorNew(void)
{
   CURSOR *c = shMalloc(sizeof(CURSOR));

   c->status        = NOT_ALLOCATED;
   c->pChainElement = NULL;
   c->pPrev         = NULL;
   c->pNext         = NULL;
   c->gotFirstElem  = 0;

   return(c);
}

static void
p_shChainCursorDel(CURSOR *c)
{
   shFree(c);
}

/*
 * ROUTINE: p_chainElementDel()
 * 
 * DESCRIPTION:
 *   free the memory associated with each element on the chain
 * 
 * CALL:
 *   (void) p_chainElementDel(CHAIN *pChain)
 *   pChain - pointer to the chain whose elements are to be freed.
 *
 * RETURNS:
 */
void
p_chainElementDel(CHAIN *pChain)
{
   CHAIN_ELEM *pElement,
              *pTmpElement;

   pElement = pChain->pFirst;
   while (pElement != NULL)  {
       pTmpElement = pElement->pNext;  /* Save the next address */

#ifdef DEBUG
       FreeMemory(pElement, __FILE__, __LINE__);
#endif
       shFree(pElement);
       pElement = pTmpElement;
   }
}
/*
 * ROUTINE: p_chainDel()
 * 
 * DESCRIPTION:
 *   free the memory associated with the chain itself
 * 
 * CALL:
 *   (void) p_chainDel(CHAIN *pChain)
 *   pChain - pointer to the chain which is to be freed.
 *
 * RETURNS:
 */
void
p_chainDel(CHAIN *pChain)
{
   int i;
   
#ifdef DEBUG
   FreeMemory(pChain, __FILE__, __LINE__);
#endif
/*
 * first the index
 */
   shChainIndexDel(pChain);
/*
 * now the cursors
 */
   for (i = 0; i < pChain->max_cursors; i++)  {
      p_shChainCursorDel(pChain->chainCursor[i]);
   }
   shFree(pChain->chainCursor);

   shFree(pChain);
}

/*
 * ROUTINE: shChain_elemNew()
 * 
 * DESCRIPTION:
 *   shChain_elemNew() allocates memory for a new chain element and 
 *   initializes the memory.
 * 
 * CALL:
 *   (CHAIN_ELEM *) shChain_elemNew(const char *a_type)
 *   a_type - one of the valid schema types (including a GENERIC which
 *          signifies a heterogeneous container)
 *
 * RETURNS:
 *    On success - a pointer to the newly created chain element
 *    On failure - no successful return 
 */
CHAIN_ELEM *
shChain_elemNew(const char *a_type)
{
   CHAIN_ELEM *celem;

   celem = (CHAIN_ELEM *)shMalloc(sizeof(CHAIN_ELEM));
   if (celem == NULL)
      shFatal("shChain_elemNew: Cannot allocate storage for a new CHAIN_ELEM");

   if (a_type == NULL) 
       shFatal("shChain_elemNew: cannot create a chain for a NULL type");

   celem->pElement = NULL;
   celem->type     = shTypeGetFromName((char *)a_type);
   celem->pPrev    = NULL;
   celem->pNext    = NULL;

   if (INVALID_CHAIN_ELEM == NULL)
       initPointers();

   return(celem);
}

/*
 * ROUTINE: shChain_elemDel()
 *
 * DESCRIPTION:
 *   shChain_elemDel() deletes the specified chain. The memory for the 
 *   CHAIN_ELEM itself will be deleted. However, the structure
 *   pointed to by its "pElement" field will be unaffected. It is the user's
 *   responsibility to get a pointer to that structure before calling
 *   this function
 *
 * CALL:
 *   (void) shChain_elemDel(CHAIN_ELEM *pChain_elem)
 *   pChain_elem - Pointer to the chain_element we want to delete
 *
 * RETURNS
 *   Nothing
 */
void
shChain_elemDel(CHAIN_ELEM *celem)
{
   if (celem == NULL) {
      return;
   }

   shFree(celem);
   return;   
}

/*
 * ROUTINE: shChainNew()
 *
 * DESCRIPTION:
 *   shChainNew() allocates memory for a new chain and initializes the
 *   memory.
 *
 * CALL:
 *   (CHAIN *) shChainNew(const char *a_type)
 *   a_type - one of the valid schema types (including a GENERIC which
 *          signifies a heterogeneous container)
 *
 * RETURNS:
 *    On success - a pointer to the newly created chain
 *    On failure - no successful return 
 */
CHAIN *
shChainNew(const char *a_type)
{
   CHAIN *pChain;

   pChain = (CHAIN *)shMalloc(sizeof(CHAIN));
   pChain->max_cursors = 0;
   pChain->chainCursor = NULL;

#ifdef DEBUG
   GetMemory(pChain, sizeof(CHAIN), __FILE__, __LINE__);
#endif

   if (pChain == NULL)
       shFatal("shChainNew: Cannot allocate storage for a new CHAIN");

   if (a_type == NULL) 
       shFatal("shChain_elemNew: cannot create a chain for a NULL type");

   /* Initialize the chain */
   p_shChainInit(pChain, a_type);

   return pChain;   
}

void p_shChainInit(CHAIN *a_pChain, const char *a_type)
{
   int   i;

   shAssert(a_pChain != NULL);

   a_pChain->nElements = 0;
   a_pChain->type      = shTypeGetFromName((char *)a_type);
   a_pChain->pFirst = a_pChain->pLast = NULL;
   a_pChain->pCacheElem = NULL;
   a_pChain->cacheElemPos = 0;
   a_pChain->index = NULL;		/* let us hope that this isn't
					   a memory leak; as this is the
					   init function we cannot check
					   that it's NULL */

   for (i = 0; i < a_pChain->max_cursors; i++)  {
        a_pChain->chainCursor[i]->pChainElement = NULL;
        a_pChain->chainCursor[i]->status        = NOT_ALLOCATED;
        a_pChain->chainCursor[i]->pPrev         = NULL;
        a_pChain->chainCursor[i]->pNext         = NULL;
        a_pChain->chainCursor[i]->gotFirstElem  = 0;
   }

   if (INVALID_CHAIN_ELEM == NULL)
       initPointers();
   return;
}
static void initPointers(void)
{
   static void *filler[1];

   INVALID_CHAIN_ELEM = &filler[0];
   return;
}

/*
 * ROUTINE: shChainNewBlock()
 *
 * DESCRIPTION:
 *   shChainNewBlock() creates a new chain of the specified type
 *   with the specified number of chain elements.  The actual objects 
 *   represented by the elements are allocated as well.  As with all
 *   chains, it's up to the user to free the "objects" when chain
 *   elements are deleted.
 *
 * RETURNS:
 *   On success : A pointer to the new chain
 *   On malloc failure - no successful return 
 *   On other failure : NULL
 *
 * g_shChainErrno is set to
 *   SH_SUCCESS       - on success
 *   SH_BAD_SCHEMA    - If an unknown schema is encountered
 */
CHAIN *
shChainNewBlock(const char *a_type, int a_num)
{
   SCHEMA *schema;
   CHAIN  *pChain;
   void   *object;
   TYPE   lType;
   int    i;
   CHAIN_ELEM *pElem = NULL;   
   CHAIN_ELEM *pElemLast = NULL;

   /* 
    * Get the schema and create the chain structure
    */
   g_shChainErrno = SH_SUCCESS;
   if ((schema = (SCHEMA *)shSchemaGet((char *)a_type)) == NULL) {
     g_shChainErrno = SH_BAD_SCHEMA;
     return NULL;
   }
   pChain = shChainNew(a_type);
   lType = pChain->type;

   /* 
    * Loop will create each object and chain element, then add to chain.
    * If a constructor exists for the schema, it will be used to create 
    * the object, otherwise shMalloc is called with the schema size.
    */
   for (i=0; i<a_num; i++) {
     if (schema->construct != NULL) {
       object = (void *)(*schema->construct)();
       shAssert(object != NULL); 
     } else {
       object = (void *)shMalloc(schema->size); 
       shAssert(object != NULL); 
       memset (object, '\0', (size_t)(schema->size));     /* Clear it out */
     }

     pElem = (CHAIN_ELEM *) shMalloc(sizeof(CHAIN_ELEM)); /* Create chain elem*/
     shAssert(pElem != NULL); 
     pElem->pElement = object;
     pElem->type     = lType;
     pElem->pPrev    = pElem->pNext = NULL;
     /*
      * After putting first element on chain by hand, use insertElement 
      * for subsequent elements
      */
     if (pChain->pFirst == NULL && pChain->pLast == NULL)  {
       pChain->pFirst = pChain->pLast = pChain->pCacheElem = pElem;
       pChain->nElements = 1;
       pChain->cacheElemPos = 0;
     } else {
       insertElement(pChain, pElemLast, pElem, AFTER);
     }
     pElemLast = pElem;
   }
   return pChain;
 }

/*
 * ROUTINE: shChainDel()
 *
 * DESCRIPTION:
 *   shChainDel() deletes the specified chain. The memory for the CHAIN
 *   itself and all the CHAIN_ELEMs will be deleted. However, each element
 *   (pElement) on a CHAIN_ELEM will be unaffected. It is the user's
 *   responsibility to get a pointer to these elements before calling
 *   this function
 *
 * CALL:
 *   (void) shChainDel(CHAIN *pChain)
 *   pChain - Pointer to the chain we want to delete
 *
 * RETURNS
 *   Nothing
 */
void
shChainDel(CHAIN *pChain)
{
   shAssert(pChain != NULL);

   /* First delete the elements of the chain */
   p_chainElementDel(pChain);

   /* Now free the chain */
   p_chainDel(pChain);

   return;
}

/*
 * ROUTINE: shChainDestroy()
 * 
 * DESCRIPTION:
 *   shChainDestroy() deletes the specified chain and all objects on the chain
 *   as well. The memory for the CHAIN itself and all the CHAIN_ELEMs will be
 *   deleted. Also deleted is each element (pElement) on a CHAIN_ELEM.
 *
 *   Note that you can get into trouble calling this function on a chain
 *   if there are other places where the addresses of the objects on the
 *   chain are stored (e.g. if the object appears on two chains).
 *
 *   This will, in fact, only bite for types that call shMalloc more than
 *   once in their constructor (e.g. REGION). The problem is no worse than
 *   any other way of getting the same pointer into multiple handles, so
 *   worry too much about this warning.
 *
 * CALL:
 *   (int) shChainDestroy(CHAIN *pChain, void (*pDelFunc)(void *))
 *   pChain   - Pointer to the chain to be deleted
 *   pDelFunc - Pointer to a function which deletes each element on the chain.
 *              This function is passed the address of the element.
 *
 * RETURNS:
 *   0 : On success
 *   1 : Otherwise - for instance, if a chain's type is GENERIC, this function
 *       cannot delete objects on the chain.
 */
int
shChainDestroy(CHAIN *pChain, void (*pDelFunc)(void *))
{
   CHAIN_ELEM  *pElement,
               *pTmpElement;
   void        *pObject;     /* To be deleted */

   shAssert(pChain != NULL);

   if (pChain->type == shTypeGetFromName("GENERIC"))
       return 0;

   pElement = pChain->pFirst;
   while (pElement != NULL)  {
       pObject = pElement->pElement;
    
       if(p_shMemRefCntrGet(pObject) > 1) {
	  shMemRefCntrDecr(pObject);
       } else {
	  pDelFunc(pObject);
       }

       pTmpElement = pElement->pNext;
       shFree(pElement);
       pElement = pTmpElement;
   }

   shChainIndexDel(pChain);
   shFree(pChain);

   return 0;
}


/*
 * ROUTINE: insertElement()
 *
 * DESCRIPTION:
 *   insertElement() does the grungy work of actually inserting an element
 *   on the chain. The new element is added in reference to an existing
 *   element on the chain. The "referentiality" is controlled by the parameter
 *   where, which can be one of BEFORE or AFTER and signifies that the new
 *   element should be added BEFORE an existing one, or AFTER and existing one.
 *
 * CALL:
 *   (static void) insertElement(CHAIN *pChain, CHAIN_ELEM *pExistingElement,
 *                 CHAIN_ELEM *pNewElement, const CHAIN_ADD_FLAGS where)
 *   pChain        - Pointer to the chain
 *   pExistingElem - pointer to the existing element
 *   pNewElem      - pointer to the new elemet to be added
 *   where         - one of BEFORE or AFTER
 *
 * RETURNS:
 *   Nothing.
 */
static void
insertElement(CHAIN *pChain, CHAIN_ELEM *pExistingElement, 
              CHAIN_ELEM *pNewElement,
              const CHAIN_ADD_FLAGS where)
{
   if (where == (CHAIN_ADD_FLAGS) BEFORE)  {
       if (pExistingElement->pPrev == NULL)  {
           /*
            * We are inserting new element at the beginning of the chain
            */
           pExistingElement->pPrev = pNewElement;
           pNewElement->pNext = pExistingElement;
           pNewElement->pPrev = NULL;
           pChain->pFirst = pNewElement;
       } else  {
          /*
           * We are inserting new element somewhere in between
           */
          CHAIN_ELEM *pTmp;

          pTmp = pExistingElement->pPrev;
          pTmp->pNext = pNewElement;
          pExistingElement->pPrev = pNewElement;
          pNewElement->pNext = pExistingElement;
          pNewElement->pPrev = pTmp;
       }
   } else  {
       if (pExistingElement->pNext == NULL)  {
           /*
            * We are inserting new element a the end of the chain
            */
           pExistingElement->pNext = pNewElement;
           pNewElement->pPrev = pExistingElement;
           pNewElement->pNext = NULL;
           pChain->pLast = pNewElement;
       } else  {
          /*
           * We're inserting the element somewhere in between
           */
          CHAIN_ELEM *pTmp;
 
          pTmp = pExistingElement->pNext;
          pExistingElement->pNext = pNewElement;
          pTmp->pPrev = pNewElement;
          pNewElement->pNext = pTmp;
          pNewElement->pPrev = pExistingElement;
       }
   }

   pChain->nElements++;
   return;
}

/*
 * ROUTINE: removeElement()
 *
 * DESCRIPTION:
 *   removeElement() removes the specified CHAIN_ELEM from the CHAIN. It
 *   then frees the memory associated with the CHAIN_ELEM. The caller
 *   must make sure that they have saved the address of the object being
 *   held by the CHAIN_ELEM to be deleted.
 *   It also does the housekeeping work of decrementing the number of
 *   elements being held on the chain.
 *
 * CALL:
 *   (void) removeElement(CHAIN *pChain, CHAIN_ELEM *pElem)
 *   pChain - Pointer to the chain
 *   pElem  - Pointer to the element to be deleted
 *
 * RETURNS: 
 *   Nothing.
 */
static void
removeElement(CHAIN *pChain, CHAIN_ELEM *pElem)
{
   /*
    * There are four cases to consider:
    *   0) pElem is the ONLY element on the chain
    *   1) pElem is at the tail of the chain (pElem->pNext == NULL)
    *   2) pElem is at the head of the chain (pElem->pPrev == NULL)
    *   3) pElem is somewhere in the middle of two elements
    */
   if (pElem->pNext == NULL && pElem->pPrev == NULL)  {
       pChain->pFirst = pChain->pLast = NULL;
   } else if (pElem->pNext == NULL)  {
       pChain->pLast = pElem->pPrev;
       pElem->pPrev->pNext = NULL;
   } else if (pElem->pPrev == NULL)  {
       pChain->pFirst = pElem->pNext;
       pElem->pNext->pPrev = NULL;
   } else  {
       pElem->pPrev->pNext = pElem->pNext;
       pElem->pNext->pPrev = pElem->pPrev;
   }
/*
 * Invalidate cache if we are freeing it. If possible, simply move pointer
 * up to the next element which will fall into the newly vacated position
 */
   if(pChain->pCacheElem == pElem) {
      if((pChain->pCacheElem = pElem->pNext) == NULL) {
	 pChain->cacheElemPos = 0;	/* no cached element */
      }
   }
#ifdef DEBUG
   FreeMemory(pElem, __FILE__, __LINE__);
#endif
   shFree(pElem);

   pChain->nElements--;

   return;
}

/*
 * ROUTINE: shChainElementAddByPos()
 *
 * DESCRIPTION:
 *   shChainElementAddByPos() adds an element on to the chain. The element
 *   is added according to two parameters n and where. n signifies the 
 *   relative position the new element should be added into the chain; i.e. 
 *   the new element is added before or after the nth element. The parameter
 *   where should be one of BEFORE or AFTER and it signifies if the new element
 *   should be added BEFORE or AFTER the relative element.
 *
 * CALL:
 *   (RET_CODE) shChainElementAddByPos(CHAIN *pChain, void *pData, 
 *              const char *a_dataType, unsigned int n,
 *              const CHAIN_ADD_FLAGS where)
 *   pChain     - Pointer to the chain
 *   pData      - Pointer to the element to be inserted on the chain
 *   a_dataType - The schema data type for pData
 *   n          - Relative position in the chain for the new element
 *   where      - one of BEFORE or AFTER; i.e should the new element
 *                be added BEFORE or AFTER the nth element.
 * RETURNS:
 *    SH_SUCCESS       - on a successful insert
 *    SH_TYPE_MISMATCH - if dataType does not match the chain's type
 * 
 * g_shChainErrno is set to
 *    SH_SUCCESS - on success
 *    SH_TYPE_MISMATCH - if dataType does not match the chain's type
 *    SH_CHAIN_INVALID_ELEM - if n > sizeof chain
 */
RET_CODE
shChainElementAddByPos(CHAIN *pChain, void *pData, const char *a_dataType, 
                       unsigned int n, const CHAIN_ADD_FLAGS where)
{
   CHAIN_ELEM *pNewElement,   /* New element to be added to the chain */
              *pRefElement;   /* Reference element (i.e. the nth element */
   TYPE       chainType,
              dataType;
   static TYPE genericType = UNKNOWN_SCHEMA;	/* cache GENERIC type */
   
   shAssert(pChain != NULL);

   if(pChain->index != NULL) {		/* it'll be invalidated */
      shFree(pChain->index); pChain->index = NULL;
   }

   if(genericType == UNKNOWN_SCHEMA) {
      genericType = shTypeGetFromName("GENERIC");
   }

   g_shChainErrno = SH_SUCCESS;
   dataType = shTypeGetFromName((char *)a_dataType);

   /*
    * First make sure that the element we are adding is of the same type as
    * the chain. If the chain is of type GENERIC, anything can be added to
    * it. If chain is of specific type, only elements of that type can be
    * added to the chain
    */
   chainType = pChain->type;
   if (chainType != genericType)
       if (dataType != chainType)  {
           g_shChainErrno = SH_TYPE_MISMATCH;
           return g_shChainErrno;
       }
      
   pNewElement = (CHAIN_ELEM *) shMalloc(sizeof(CHAIN_ELEM));

#ifdef DEBUG
   GetMemory(pNewElement, sizeof(CHAIN_ELEM), __FILE__, __LINE__); 
#endif

   shAssert(pNewElement != NULL); 

   pNewElement->pElement = pData;
   pNewElement->type     = dataType;
   pNewElement->pPrev    = pNewElement->pNext = NULL;

   if (pChain->pFirst == NULL && pChain->pLast == NULL)  {
       /*
        * First element on the chain. Parameters n and where do not matter here
        */
       pChain->pFirst = pChain->pLast = pChain->pCacheElem = pNewElement;
       pChain->nElements = 1;
       pChain->cacheElemPos = 0;

       return SH_SUCCESS;
   }

   pRefElement = getElementN(pChain, n);  /* Get the nth element */
   if (pRefElement == NULL)
       g_shChainErrno = SH_CHAIN_INVALID_ELEM;
   else  {
       insertElement(pChain, pRefElement, pNewElement, where);

       if (where == (CHAIN_ADD_FLAGS) BEFORE)
	     pChain->cacheElemPos++;
             /*
	      * Note that pRefElement is also the cached element 
	      * (pChain->pCacheElem). getElementN() sees to that. Now, if we 
	      * are adding a new element before the cached element, we have 
	      * in sense bumped the cached element from position n to position 
	      * n+1. Hence the above addition to pChain->cacheElemPos. If we 
	      * add a new element after the cached element, then 
	      * pChain->pCacheElem and pChain->cacheElemPos are already synced.
	      */
   }

   return g_shChainErrno;
}                       

/*
 * ROUTINE: getElementN()
 *
 * DESCRIPTION
 *   getElementN() returns a pointer to the nth CHAIN_ELEM element.
 *   getElementN() also caches the nth element. On subsequent call, it first
 *   looks at its cache to see if the new element is near (+1 or -1) the 
 *   cached element. If so, it will satisfy the request immidiately.
 *   The cache is always updated to reflect the most recently accessed 
 *   element.
 *
 * If the chain is indexed (see shChainIndexMake), use the index to
 * find the desired element
 *
 * CALL:
 *   (static CHAIN_ELEM) *getElementN(const CHAIN *pChain, 
 *                                    unsigned int pos)
 *   pChain  - pointer to the chain
 *   pos     - position of desired element
 *
 * RETURNS:
 *   On success : A pointer to a chain element
 *   On failure : NULL
 */
static 
CHAIN_ELEM *getElementN(const CHAIN *pChain, unsigned int pos)
{
   CHAIN_ELEM *pElement;
   unsigned   int tmpPos;

   pElement = NULL;
   tmpPos = pos;

   /*
    * First we'll handle the simple cases:
    *  0) element we are looking for does not exist
    *  1) element we are looking for is at the head of the chain
    *  2) element we are looking for is at the tail of the chain
    */

   if (pos != TAIL && pos >= pChain->nElements) 
       return (CHAIN_ELEM *) NULL;

   if (pos == HEAD)  {
       pElement = pChain->pFirst;
       /*
        * With due apologies to Edsgar Dijsktra, sometimes goto is just what
        * the doctor ordered!
        */
       goto done;
   }

   if (pos == TAIL || pos == pChain->nElements - 1)  {
       pElement = pChain->pLast;
       tmpPos = pChain->nElements - 1;
       goto done;
   }
  /*
   * if the chain is indexed, use that
   */
   if(pChain->index != NULL) {
      pElement = pChain->index[pos];
      tmpPos = pos;
      goto done;
   }
  /*
   * Ok. Now we have exhausted all easy cases. Before traversing through the
   * chain, let's take a look at the cached element. If the new element falls
   * within +1 or -1 of the cached element, no sense in going through the
   * entire chain...
   *		Additionally, look to see if we're already at the cache element 
   *		(JTA speedup), if so jump to it.
   */

   if (pChain->cacheElemPos != 0) {
          if (pos == pChain->cacheElemPos + 1)  {
              if (pChain->pCacheElem->pNext != NULL)  {
                  pElement = pChain->pCacheElem->pNext;
                  tmpPos = pChain->cacheElemPos + 1;
                  goto done;
              }
          } else if (pos == pChain->cacheElemPos - 1)  {
              if (pChain->pCacheElem->pPrev != NULL)  {
                  pElement = pChain->pCacheElem->pPrev;
                  tmpPos = pChain->cacheElemPos - 1;
                  goto done;
              }
          } else if (pos == pChain->cacheElemPos )  {
              pElement = pChain->pCacheElem;
              tmpPos = pChain->cacheElemPos;
              goto done;
          }

	}

   /*
    * None of the easy cases or the cache matched. Looks like we'll have
    * traverse the chain by brute force...
    */
    pElement = (CHAIN_ELEM *) pChain->pFirst;
    tmpPos = 0;
    while (tmpPos < pos)  {
      pElement = pElement->pNext;
      tmpPos++;
    }

done:
   /*
    * Cache the most recently accessed element 
    */
   ((CHAIN *)pChain)->pCacheElem = pElement;
   ((CHAIN *)pChain)->cacheElemPos = tmpPos;
   return pElement;
}

/*
 * ROUTINE: shChainElementTypeGetByPos()
 *
 * DESCRIPTION:
 *   This function gets the type of the nth element on the chain.
 *
 * CALL:
 *   TYPE shChainElementTypeGetByPos(CHAIN *pChain, unsigned int pos)
 *   pChain - Pointer to the chain
 *   pos    - Index of the element we want off the chain
 *
 * RETURNS:
 *   The type of the nth element
 *
 *   g_shChainErrno is set to 
 *     SH_CHAIN_EMPTY        - if chain is empty
 *     SH_CHAIN_INVALID_ELEM - if pos > sizeof chain
 *     SH_SUCCESS            - otherwise
 */
TYPE
shChainElementTypeGetByPos(const CHAIN *pChain, unsigned int pos)
{
   CHAIN_ELEM *pElem;
   TYPE        type;

   type = shTypeGetFromName("UNKNOWN");
   g_shChainErrno = SH_SUCCESS;

   shAssert(pChain != NULL);

   if (pChain->nElements == 0)  {
       g_shChainErrno = SH_CHAIN_EMPTY;
       return type;
   }

   pElem = getElementN(pChain, pos);
   if (pElem == NULL)
       g_shChainErrno = SH_CHAIN_INVALID_ELEM;
   else
       type = pElem->type;       
   
   return type;
}

/*
 * ROUTINE: shChainElementGetByPos()
 *
 * DESCRIPTION:
 *   This function gets the nth element on the chain.
 *
 * CALL:
 *   TYPE shChainElementGetByPos(CHAIN *pChain, unsigned int pos)
 *   pChain - Pointer to the chain
 *   pos    - Index of the element we want off the chain
 *
 * RETURNS:
 *   Pointer to the address of the nth element on the chain. Note, it
 *   returns the address of _what_ the nth CHAIN_ELEM is holding, not
 *   the address of the nth CHAIN_ELEM!
 *   This function returns NULL if the chain is empty, or if pos > sizeof chain
 *
 * g_shChainErrno is set to
 *   SH_SUCCESS            - on success
 *   SH_CHAIN_EMPTY        - if chain is empty
 *   SH_CHAIN_INVALID_ELEM - if pos > sizeof chain
 */
void *
shChainElementGetByPos(const CHAIN *pChain, unsigned int pos)
{
   CHAIN_ELEM *pElem;

   g_shChainErrno = SH_SUCCESS;

   shAssert(pChain != NULL);

   if (pChain->nElements == 0)  {
       g_shChainErrno = SH_CHAIN_EMPTY;
       return NULL;
   }


   pElem = getElementN(pChain, pos);
   if (pElem == NULL)  {
       g_shChainErrno = SH_CHAIN_INVALID_ELEM;
       return NULL;
   }
   
   return pElem->pElement;
}

/*
 * ROUTINE: shChainElementRemByPos()
 *
 * DESCRIPTION:
 *   This function removes the nth element from the chain. It's important
 *   to note that the CHAIN_ELEM holding the nth element is deleted. This
 *   function, then returns a pointer to _what_ the nth CHAIN_ELEM was
 *   holding
 *
 * CALL:
 *   TYPE shChainElementRemByPos(CHAIN *pChain, unsigned int pos)
 *   pChain - Pointer to the chain
 *   pos    - Index of the element we want off the chain
 *
 * RETURNS:
 *   Pointer to the address of the nth element on the chain. Note, it
 *   returns the address of _what_ the nth CHAIN_ELEM is holding, not
 *   the address of the nth CHAIN_ELEM!
 *   If the chain is empty, this function returns NULL.
 *   If pos > sizeof chain, this function returns NULL as well.
 *
 * g_shChainErrno is set to
 *   SH_CHAIN_EMPTY        - if chain is empty
 *   SH_CHAIN_INVALID_ELEM - if pos > sizeof chain
 *   SH_SUCCESS            - otherwise
 */
void *
shChainElementRemByPos(CHAIN *pChain, unsigned int pos)
{
   CHAIN_ELEM *pElem;
   void       *pData;

   g_shChainErrno = SH_SUCCESS;

   shAssert(pChain != NULL);

   if (pChain->nElements == 0)  {
       g_shChainErrno = SH_CHAIN_EMPTY;
       return NULL;
   }

   if(pChain->index != NULL) {		/* it'll be invalidated */
      shFree(pChain->index); pChain->index = NULL;
   }

   pElem = getElementN(pChain, pos);
   if (pElem == NULL)  {
       g_shChainErrno = SH_CHAIN_INVALID_ELEM;
       pData = NULL;
   } else  {
      pData = pElem->pElement;
      removeElement(pChain, pElem);
   }

   return pData;
}

/*
 * ROUTINE: shChainSize()
 *
 * DESCRIPTION:
 *   Get the size of a chain
 *
 * CALL: 
 *   (unsigned int) shChainSize(const CHAIN *pChain);
 *   pChain - Pointer to the chain
 *
 * RETURNS: 
 *   N : where N >= 0
 */
unsigned int
shChainSize(const CHAIN *pChain)
{
   shAssert(pChain != NULL);

   g_shChainErrno = SH_SUCCESS;

   return pChain->nElements;
}

/*
 * ROUTINE: shChainTypeGet()
 *
 * DESCRIPTION:
 *   Get the type of a chain
 *
 * CALL: 
 *   (TYPE) shChainTypeGet(const CHAIN *pChain);
 *   pChain - Pointer to the chain
 *
 * RETURNS: 
 *   The TYPE of the given chain
 */
TYPE
shChainTypeGet(const CHAIN *pChain)
{
   shAssert(pChain != NULL);

   g_shChainErrno = SH_SUCCESS;

   return pChain->type;
}

/*
 * ROUTINE: shChainTypeSet()
 * 
 * DESCRIPTION:
 *   shChainTypeSet() sets the type field to a new one. It's the users
 *   responsibility to ensure that all elements on the chain are of the new
 *   type. This routine will simply change the type to the new one as
 *   indicated.
 *
 * CALL:
 *   (TYPE) shChainTypeSet(CHAIN *pChain, const char *a_newType)
 *   pChain    - Pointer to the chain
 *   a_newType - The new type of the chain
 *
 * RETURNS:  
 *   The old TYPE
 *
 */
TYPE
shChainTypeSet(CHAIN *pChain, const char *a_newType)
{
   TYPE oldType,
        newType;

   shAssert(pChain != NULL);

   g_shChainErrno = SH_SUCCESS;
   newType = shTypeGetFromName((char *)a_newType);

   oldType = pChain->type;
   pChain->type = newType;

   return oldType;
}

/*
 * ROUTINE: shChainJoin()
 *
 * DESCRIPTION:
 *   shChainJoin() appends a target chain to the source chain. Both chains
 *   should be of the same type, unless the source chain is of type GENERIC,
 *   in which case the target chain can be of any type.
 *
 * CALL:
 *   (RET_CODE)shChainJoin(CHAIN *pSrc, CHAIN *pTarget)
 *   pSrc    - Pointer to the source chain
 *   pTarget - Pointer to the target chain
 *
 * RETURNS: 
 *   SH_SUCCESS : on a successful join
 *   SH_TYPE_MISMATCH : if types of the chains do not match
 *
 * g_shChainErrno is set to 
 *   SH_TYPE_MISMATCH - if types of chain do not match
 *   SH_SUCCESS       - otherwise
 */
RET_CODE
shChainJoin(CHAIN *pSrc, CHAIN *pTarget)
{

   shAssert(pSrc != NULL);

   g_shChainErrno = SH_SUCCESS;

   if (pTarget == NULL) return SH_SUCCESS;
   if (pTarget->nElements == 0) {
      shChainDel(pTarget);
      return SH_SUCCESS;
   }

   if (pSrc->type != shTypeGetFromName("GENERIC")) {
       if (pSrc->type != pTarget->type)  {
           g_shChainErrno = SH_TYPE_MISMATCH;
           return g_shChainErrno;
       }
   }

   if(pSrc->index != NULL) {		/* it'll be invalidated */
      shFree(pSrc->index); pSrc->index = NULL;
   }
   
   if (pSrc->nElements == 0)  {
       pSrc->pFirst    = pTarget->pFirst;
       pSrc->pLast     = pTarget->pLast;
       pSrc->nElements = pTarget->nElements;
   } else  {
       /*
        * Now we're assured that both chains have at least 1 element on them;
        * adjust the pointers
        */
       pSrc->pLast->pNext = pTarget->pFirst;
       pTarget->pFirst->pPrev = pSrc->pLast;
       pSrc->pLast = pTarget->pLast;
       pSrc->nElements += pTarget->nElements;
   }

   pTarget->pLast = pTarget->pFirst = NULL;
   pTarget->nElements = 0;
   shChainDel(pTarget);

   return SH_SUCCESS;
}

/*
 * ROUTINE: shChainCopy()
 *
 * DESCRIPTION:
 *   shChainCopy() copies a given chain. Note that the new chain DOES NOT
 *   inherit any cursor information from the old one, and that this is a
 *  copy of the _chain_, not of the contents of the chain
 *
 * CALL:
 *   (CHAIN *)shChainCopy(const CHAIN *pSrc)
 *   pSrc - Pointer to the source chain
 *
 * RETURNS:
 *   Pointer to the newly copied chain, always
 *
 * g_shChainErrno is set to
 *   SH_SUCCESS     - always
 */
CHAIN *
shChainCopy(const CHAIN *pSrc)
{
   CHAIN      *pNewChain;
   CHAIN_ELEM *pElem;
   short      flag;

   shAssert(pSrc != NULL);

   g_shChainErrno = SH_SUCCESS;

   if (pSrc->nElements == 0)  {
      return(shChainNew((char *)pSrc->type));
   }

   flag      = 0;
   pNewChain = shChainNew((char *)pSrc->type);
   pElem     = pSrc->pFirst;

   for (; pElem != NULL; )  {
        if (shChainElementAddByPos(pNewChain, pElem->pElement, 
			     (char *)pElem->type, TAIL, AFTER) != SH_SUCCESS)  
        {
            flag = 1;
            break;
        }
        pElem = pElem->pNext;
   }

   if (flag)  {
       shChainDel(pNewChain);
       pNewChain = NULL;
   }

   return pNewChain;
}

/*
 * ROUTINE: shChainTypeDefine()
 *
 * DESCRIPTION:
 *    This function defines a chain type according to the following rules:
 *    1) if chain is empty, return the original type
 *    2) if chain type is GENERIC, but the chain contains homogeneous
 *       elements, return the type of the elements on the chain
 *    3) if chain type is GENERIC, and the chain contains heterogeneous
 *       elements, the chain type is left unchanged
 *
 * CALL:
 *   (TYPE) shChainTypeDefin(const CHAIN *pChain)
 *   pChain - Pointer to the chain
 *
 * RETURNS:
 *   TYPE of the chain
 */
TYPE
shChainTypeDefine(const CHAIN *pChain)
{
   CHAIN_ELEM *pElem;
   TYPE       elemType;
   short      homogeneous;

   shAssert(pChain != NULL);

   g_shChainErrno = SH_SUCCESS;

   if (pChain->nElements == 0)
       return pChain->type;

   if (pChain->type != shTypeGetFromName("GENERIC"))
       return pChain->type;

   pElem = pChain->pFirst;
   elemType = pElem->type;

   pElem = pElem->pNext;
   homogeneous = 1;

   for (; pElem != NULL; )  {
        if (elemType != pElem->type)
            homogeneous = 0;
        pElem = pElem->pNext;
   }

   if (homogeneous)
       return elemType;
   else
       return pChain->type;
}

/*
 * ROUTINE: shChainCursorNew()
 *
 * DESCRIPTION:
 *   Create a new cursor for the given chain. Note that this routine
 *   will exit using shFatal() if all cursors are being used. A new
 *   cursor will have it's status field be initialized to ALLOCATED,
 *   and it's pChainElementField be initialized to "one element before
 *   the start of the chain".
 *
 * CALL:
 *   (CURSOR_T) shChainCursorNew(const CHAIN *pChain)
 *   pChain - Pointer to the chain
 *
 * Note that this function doesn't modify the chain; because the cursor
 * information is stored in the chain we have to cast away the logical
 * constness of the input CHAIN
 *
 * RETURNS:
 *   Index to the next available cursor for the given chain. Always succeeds
 */
CURSOR_T
shChainCursorNew(const CHAIN *pChain)
{
   CURSOR_T crsr;
   CHAIN *chain = (CHAIN *)pChain;
   int i;
   
   shAssert(chain != NULL);

   for (crsr = 0; crsr < chain->max_cursors; crsr++) {
      if (chain->chainCursor[crsr]->status == NOT_ALLOCATED)  {
	 break;
      }
   }
   if(crsr == chain->max_cursors) {	/* need more cursors */
      chain->max_cursors = 2*chain->max_cursors + 1;
      chain->chainCursor = shRealloc(chain->chainCursor,
				     chain->max_cursors*sizeof(CURSOR *));
      for(i = crsr; i < chain->max_cursors; i++) {
	 chain->chainCursor[i] = p_shChainCursorNew();
      }
   }

   chain->chainCursor[crsr]->status        = ALLOCATED;
   chain->chainCursor[crsr]->pChainElement = pChain->pFirst;
   chain->chainCursor[crsr]->pPrev         = NULL;
   chain->chainCursor[crsr]->pNext         = NULL;
   chain->chainCursor[crsr]->gotFirstElem  = 0;

   return crsr;
}

/*
 * ROUTINE: shChainCursorDel()
 *
 * DESCRIPTION:
 *   Delete the specified cursor for the given chain.
 *
 * CALL:
 *   (RET_CODE) shChainCursorDel(const CHAIN *pChain, const CURSOR_T crsr)
 *   pChain - Pointer to the chain
 *   crsr   - Cursor index
 *
 * RETURNS:
 *   On success : SH_SUCCESS
 *   On failure : SH_CHAIN_INVALID_CURSOR - if cursor is invalid
 */
RET_CODE
shChainCursorDel(const CHAIN *pChain, const CURSOR_T crsr)
{
   int i;
   shAssert(pChain != NULL);

   g_shChainErrno = SH_SUCCESS;

   if ( (crsr >= pChain->max_cursors) || (crsr < 0) )  {
       g_shChainErrno = SH_CHAIN_INVALID_CURSOR;
       return g_shChainErrno;
   }

   if (pChain->chainCursor[crsr]->status == NOT_ALLOCATED)  {
       g_shChainErrno = SH_CHAIN_INVALID_CURSOR;
       return g_shChainErrno;
   }       
   
   ((CHAIN *)pChain)->chainCursor[crsr]->status        = NOT_ALLOCATED;
   ((CHAIN *)pChain)->chainCursor[crsr]->pChainElement = NULL;
   ((CHAIN *)pChain)->chainCursor[crsr]->pPrev         = NULL;
   ((CHAIN *)pChain)->chainCursor[crsr]->pNext         = NULL;
   ((CHAIN *)pChain)->chainCursor[crsr]->gotFirstElem  = 0;

   for(i = 0; i < pChain->max_cursors; i++) {
      if(pChain->chainCursor[i]->status == ALLOCATED) {
	 break;
      }
   }
   if(i == pChain->max_cursors) {
      for(i = 0; i < pChain->max_cursors; i++) {
	 p_shChainCursorDel(pChain->chainCursor[i]);
      }
      shFree(pChain->chainCursor);
      ((CHAIN *)pChain)->chainCursor = NULL;
      ((CHAIN *)pChain)->max_cursors = 0;
   }

   return g_shChainErrno;
}

/*
 * ROUTINE: shChainCursorCount()
 *
 * DESCRIPTION:
 *   This function returns a count of the number of cursors being used
 *   for a given chain.
 *
 * CALL:
 *   (void) shChainCursorCount(const CHAIN *pChain)
 *   pChain - Pointer to the chain
 *
 * RETURNS:
 *   N : where N >= 0 and represents the number of cursors being used
 */
unsigned int
shChainCursorCount(const CHAIN *pChain)
{
   unsigned int count, i;

   shAssert(pChain != NULL);

   g_shChainErrno = SH_SUCCESS;

   count = 0;
   for (i = 0; i < pChain->max_cursors; i++)
        if (pChain->chainCursor[i]->status == ALLOCATED)
            count++;

   return count;
}

/*
 * ROUTINE: shChainCursorSet()
 *
 * DESCRIPTION:
 *   This function "sets" the specified cursor on the chain. By "set" we
 *   mean it makes the specified cursor point to the required element.
 *
 * CALL:
 *   (RET_CODE) shChainCursorSet(CHAIN *pChain, const CURSOR_T crsr, 
 *                           unsigned int pos)
 *   pChain - Pointer to the chain
 *   crsr   - Index to the needed cursor
 *   pos    - Position of the element in the chain to set the cursor to 
 *
 * Note that this function doesn't modify the chain; because the cursor
 * information is stored in the chain we have to cast away the logical
 * constness of the input CHAIN
 *
 * RETURNS:
 *   g_shChainErrno, which is set to one of the following:
 *
 *      SH_SUCCESS              - on success
 *      SH_CHAIN_EMPTY          - if chain is empty
 *      SH_CHAIN_INVALID_CURSOR - if the cursor is invalid
 */
RET_CODE
shChainCursorSet(const CHAIN *pChain, const CURSOR_T crsr, unsigned int pos)
{
   CHAIN_ELEM *pElem;

   shAssert(pChain != NULL);

   g_shChainErrno = SH_SUCCESS;

   if (pChain->nElements == 0)  {
       g_shChainErrno = SH_CHAIN_EMPTY;
       return g_shChainErrno;
   }

   if ( (crsr >= pChain->max_cursors) || (crsr < 0) )  {
       g_shChainErrno = SH_CHAIN_INVALID_CURSOR;
       return g_shChainErrno;
   }

   if (pChain->chainCursor[crsr]->status == NOT_ALLOCATED)  {
       g_shChainErrno = SH_CHAIN_INVALID_CURSOR;
       return g_shChainErrno;
   }

   pElem = getElementN(pChain, pos);
   if (pElem == NULL)  {
       g_shChainErrno = SH_CHAIN_INVALID_ELEM;
       return g_shChainErrno;
   }
   
   ((CHAIN *)pChain)->chainCursor[crsr]->pChainElement = pElem;
   ((CHAIN *)pChain)->chainCursor[crsr]->pPrev         = pElem->pPrev;
   ((CHAIN *)pChain)->chainCursor[crsr]->pNext         = pElem->pNext;
   ((CHAIN *)pChain)->chainCursor[crsr]->gotFirstElem  = 0;

   return g_shChainErrno;
}

/*
 * ROUTINE: shChainCursorCopy()
 *
 * DESCRIPTION:
 *   This function copies the specified cursor on the chain. A new cursor is
 *   created and all information from the old one is copied to the new one.
 *
 * CALL:
 *   (RET_CODE) shChainCursorCopy(CHAIN *pChain, const CURSOR_T crsr, 
 *                                CURSOR_T *newCrsr)
 *   pChain  - Pointer to the chain
 *   crsr    - The cursor to be copied
 *   newCrsr - Pointer to memory address to store the newly copied cursor
 *
 * Note that this function doesn't modify the chain; because the cursor
 * information is stored in the chain we have to cast away the logical
 * constness of the input CHAIN
 *
 * RETURNS:
 *   g_shChainErrno, which is set to one of the following:
 *      SH_SUCCESS              - on success
 *      SH_CHAIN_INVALID_CURSOR - if the crsr is invalid
 *      SH_GENERIC_ERROR        - if could not get a new cursor
 *                                (error in shChainCursorNew -- result placed on errStack)
 */
RET_CODE
shChainCursorCopy(const CHAIN *pChain, const CURSOR_T crsr, CURSOR_T *newCrsr)
{
   CHAIN *chain = (CHAIN *)pChain;
   
   shAssert(pChain != NULL);

   g_shChainErrno = SH_SUCCESS;

   if ( (crsr >= pChain->max_cursors) || (crsr < 0) ) 
       g_shChainErrno = SH_CHAIN_INVALID_CURSOR;

   if (pChain->chainCursor[crsr]->status == NOT_ALLOCATED)
       g_shChainErrno = SH_CHAIN_INVALID_CURSOR;

   if (g_shChainErrno == SH_SUCCESS)  {
       *newCrsr = shChainCursorNew(pChain);

       chain->chainCursor[*newCrsr]->pChainElement = 
	     pChain->chainCursor[crsr]->pChainElement;
       chain->chainCursor[*newCrsr]->pPrev = pChain->chainCursor[crsr]->pPrev;
       chain->chainCursor[*newCrsr]->pNext = pChain->chainCursor[crsr]->pNext;
       chain->chainCursor[*newCrsr]->gotFirstElem = 
	     pChain->chainCursor[crsr]->gotFirstElem;
   }

   return g_shChainErrno;
}

/*
 * ROUTINE: shChainElementAddByCursor()
 *
 * DESCRIPTION:
 *   This function adds a new element to the chain relative to the cursor.
 *   The relativity is defined by the parameter where, which can be one of
 *   BEFORE or AFTER. The cursor is advanced to point to the newly added
 *   element.
 *
 * CALL:
 *   (RET_CODE) shChainElementAddByCursor(CHAIN *pChain, const void *pData,
 *                                    const char *a_dataType, 
 *                                    const CURSOR_T crsr,
 *                                    const ADD_FLAGS where)
 *   pChain     - Pointer to the chain
 *   pData      - Pointer to the data to add on the chain
 *   a_dataType - Type of the data to be added
 *   crsr       - Index to the cursor
 *   where      - Where, in reference to the cursor, should pData be added
 *
 * RETURNS:
 *   SH_SUCCESS              - on success
 *   SH_TYPE_MISMATCH        - if dataType does not match the chain's type
 *   SH_CHAIN_INVALID_CURSOR - if cursor is invalid
 *
 * g_shChainErrno is set to
 *   SH_SUCCESS              - on success
 *   SH_TYPE_MISMATCH        - if dataType does not match the chain's type
 *   SH_CHAIN_INVALID_CURSOR - if cursor is invalid
 */
RET_CODE
shChainElementAddByCursor(CHAIN *pChain, void *pData,
                          const char *a_dataType, const CURSOR_T crsr,
                          const CHAIN_ADD_FLAGS where)
{
   CHAIN_ELEM *pNewElem;
   TYPE       chainType,
              dataType;

   shAssert(pChain != NULL);

   g_shChainErrno = SH_SUCCESS;
   dataType = shTypeGetFromName((char *)a_dataType);

   if ( (crsr >= pChain->max_cursors) || (crsr < 0) )  {
       g_shChainErrno = SH_CHAIN_INVALID_CURSOR;  
       return g_shChainErrno;
   }

   if (pChain->chainCursor[crsr]->status == NOT_ALLOCATED)  {
       g_shChainErrno = SH_CHAIN_INVALID_CURSOR;  
       return g_shChainErrno;
   }

   if(pChain->index != NULL) {		/* it'll be invalidated */
      shFree(pChain->index); pChain->index = NULL;
   }

   /*
    * First make sure that the element we are adding is of the same type as
    * the chain. If the chain is of type GENERIC, anything can be added to
    * it. If chain is of specific type, only elements of that type can be
    * added to the chain
    */
   chainType = pChain->type;
   if (chainType != shTypeGetFromName("GENERIC"))
       if (dataType != chainType)  {
           g_shChainErrno = SH_TYPE_MISMATCH;
           return g_shChainErrno;
       }

   pNewElem = (CHAIN_ELEM *)shMalloc(sizeof(CHAIN_ELEM));

#ifdef DEBUG
   GetMemory(pNewElem, sizeof(CHAIN_ELEM), __FILE__, __LINE__); 
#endif

   pNewElem->pElement = pData;
   pNewElem->type     = dataType;
   pNewElem->pPrev    = pNewElem->pNext = NULL;

   if (pChain->pFirst == NULL && pChain->pLast == NULL)  {
       /*
        * First element on the chain, parameter where does not matter.
        */
       pChain->pFirst = pChain->pLast = pChain->pCacheElem = pNewElem;
       pChain->chainCursor[crsr]->pChainElement = pNewElem;
       pChain->chainCursor[crsr]->pPrev = pNewElem->pPrev;
       pChain->chainCursor[crsr]->pNext = pNewElem->pNext;
       pChain->nElements = 1;
       pChain->cacheElemPos = 0;
                                  
       return SH_SUCCESS;
   }

   insertElement(pChain, pChain->chainCursor[crsr]->pChainElement, pNewElem,
                 where);

   pChain->chainCursor[crsr]->pChainElement = pNewElem;
   pChain->chainCursor[crsr]->pPrev = pNewElem->pPrev;
   pChain->chainCursor[crsr]->pNext = pNewElem->pNext;

   return SH_SUCCESS;
}

/*
 * ROUTINE: shChainElementTypeGetByCursor()
 *
 * DESCRIPTION:
 *   This routine gets the type of the element under the cursor. The 
 *   location of the cursor is not effected.
 *
 * CALL:
 *   (RET_CODE) shChainElementTypeGetByCursor(const CHAIN *pChain, 
                                              const CURSOR_T crsr)
 *   pChain   - Pointer to the chain
 *   crsr     - Index to the cursor
 *
 * RETURNS:
 *   type : where type is the TYPE of element under the cursor.
 *          Note that if the element under the cursor has been removed,
 *          or an invalid cursor is passed, an UNKNOWN type is returned.
 * 
 * g_shChainErrno is set to
 *   SH_SUCCESS - on success
 *   SH_CHAIN_INVALID_CURSOR - if the cursor is invalid
 */
TYPE
shChainElementTypeGetByCursor(const CHAIN *pChain, const CURSOR_T crsr)
{
   shAssert(pChain != NULL);
   
   if ( (crsr >= pChain->max_cursors) || (crsr < 0) ) {
       g_shChainErrno = SH_CHAIN_INVALID_CURSOR;
       return shTypeGetFromName("UNKNOWN"); /* note that we only call
					       shTypeGetFromName if needed */
   }

   if (pChain->chainCursor[crsr]->status != ALLOCATED)  {
       g_shChainErrno = SH_CHAIN_INVALID_CURSOR;
       return shTypeGetFromName("UNKNOWN");
   }

   if (pChain->chainCursor[crsr]->pChainElement == NULL)  {
       g_shChainErrno = SH_CHAIN_INVALID_CURSOR;
       return shTypeGetFromName("UNKNOWN");
   }

   g_shChainErrno = SH_SUCCESS;
   return pChain->chainCursor[crsr]->pChainElement->type;
}

/*
 * ROUTINE: shChainElementRemByCursor()
 *
 * DESCRIPTION:
 *   This function removes the element under the cursor. It DOES NOT
 *   delete what the chain element is pointing to, rather, it simply 
 *   deletes the chain element.
 *
 * CALL:
 *   TYPE shChainElementRemByCursor(CHAIN *pChain, const CURSOR_T crsr)
 *   pChain - Pointer to the chain
 *   crsr    - Index of the cursor
 *
 * RETURNS:
 *   Pointer to the address of the element under the cursor. Note, it
 *   returns the address of _what_ the cursor is holding, not the address
 *   of the _cursor_ itself!
 *   A NULL is returned on all error cases and g_shChainErrno is set to
 *   the appropriate error message
 *
 * g_shChainErrno is set to
 *   SH_SUCCESS         - on success
 *   SH_CHAIN_INVALID_CURSOR - on an invalid cursor
 */
void *
shChainElementRemByCursor(CHAIN *pChain, const CURSOR_T crsr)
{
   void *pData;
   CHAIN_ELEM *pPrev,    /* Left  of cursor element */
              *pNext;    /* Right of cursor element */
   CURSOR_T   tmpCrsr;

   shAssert(pChain != NULL);

   g_shChainErrno = SH_SUCCESS;

   if (pChain->nElements == 0)  {
       g_shChainErrno = SH_CHAIN_EMPTY;
       return NULL;
   }

   if(pChain->index != NULL) {		/* it'll be invalidated */
      shFree(pChain->index); pChain->index = NULL;
   }

   if ( (crsr >= pChain->max_cursors) || (crsr < 0) )   {
       g_shChainErrno = SH_CHAIN_INVALID_CURSOR;
       return NULL;
   }

   if (pChain->chainCursor[crsr]->status != ALLOCATED)  {
       g_shChainErrno = SH_CHAIN_INVALID_CURSOR;
       return NULL;
   }

   if (pChain->chainCursor[crsr]->pChainElement == NULL)  {
       g_shChainErrno = SH_CHAIN_INVALID_CURSOR;
       return NULL;
   }

   /*
    * Four steps to deletion:
    * 1) Save the pointer to the data the cursor element is holding
    * 2) Save the addresses of it's neighbors
    * 3) For all cursors that are pointing to the element to be deleted, set 
    *    them to NULL, assign saved neighbor's addresses
    * 4) Finally, take the cursor element out of the chain
    *
    * The reason we want to save the cursor element's neighbors is that a
    * user might want to delete the cursor element while traversing the chain.
    * So, if the cursor element is deleted, how can we walk the chain? Well,
    * the answer is we save the neighbors addresses and use those while
    * walking the chain
    */
   pData = pChain->chainCursor[crsr]->pChainElement->pElement;

   pNext =  pChain->chainCursor[crsr]->pChainElement->pNext;
   pPrev =  pChain->chainCursor[crsr]->pChainElement->pPrev;

   /* 
    * Update the cursor before deleting element. A chain might have more then
    * one cursor pointing to the element to be deleted. If so, update those
    * cursors as well.  
    */
   for (tmpCrsr = 0; tmpCrsr < pChain->max_cursors; tmpCrsr++)  {

        if (tmpCrsr == crsr)
            continue;

        if (pChain->chainCursor[tmpCrsr]->status == ALLOCATED)
            if (pChain->chainCursor[tmpCrsr]->pChainElement == 
                pChain->chainCursor[crsr]->pChainElement)
            {
               pChain->chainCursor[tmpCrsr]->pChainElement =INVALID_CHAIN_ELEM;
               pChain->chainCursor[tmpCrsr]->pPrev = pPrev;
               pChain->chainCursor[tmpCrsr]->pNext = pNext;
               pChain->chainCursor[tmpCrsr]->gotFirstElem = 1;
	    }
   }

   removeElement(pChain, pChain->chainCursor[crsr]->pChainElement);

   pChain->chainCursor[crsr]->pChainElement = INVALID_CHAIN_ELEM;
   pChain->chainCursor[crsr]->pPrev = pPrev;
   pChain->chainCursor[crsr]->pNext = pNext;
   pChain->chainCursor[crsr]->gotFirstElem = 1;

   return pData;
} 

/*
 * ROUTINE: shChainWalk()
 * 
 * DESCRIPTION:
 *   shChainWalk() traverses a chain depending on the parameter 'whither'.
 *   This parameter can be one of THIS, PREVIOUS, or NEXT. Traversal is
 *   done using a cursor to preserve context. In case 'whither' is NEXT
 *   or PREVIOUS, the cursor is updated accordingly.
 *
 * CALL: 
 *   (void *) shChainWalk(CHAIN *pChain, const CURSOR_T crsr, 
 *                        const CHAIN_WALK_FLAGS whither)
 *   pChain  - Pointer to the chain
 *   crsr    - Chain cursor index
 *   whither - Direction of traversal: one of PREVIOUS, NEXT of THIS.
 *
 * Note that this function doesn't modify the chain; because the cursor
 * information is stored in the chain we have to cast away the logical
 * constness of the input CHAIN
 *
 * RETURNS: 
 *   address of the next (or previous, or the current) element. On reaching
 *   the end (or begining of chain, if walking from the end), a NULL is
 *   returned. A NULL return does not neccessarily signify a bad return 
 *   value. Users calling this function should make sure that they also
 *   check the value of g_shChainErrno. 
 *
 * g_shChainErrno is set to:
 *   SH_CHAIN_EMPTY          - if the chain is empty
 *   SH_CHAIN_INVALID_CURSOR - if the cursor is not valid
 *   SH_CHAIN_INVALID_ELEM   - if the cursor is pointing to a deleted element
 */
void *
shChainWalk(const CHAIN *pChain, const CURSOR_T crsr,
	    const CHAIN_WALK_FLAGS whither)
{
   void *pData = NULL;			/* desired data */
   CHAIN_ELEM *pChainElem;
   const CURSOR *curs;			/* == &pChain->chainCursor[crsr],
					   unaliased for efficiency */

   shAssert(pChain != NULL);

   g_shChainErrno = SH_SUCCESS;

   if (pChain->nElements == 0)  {
       g_shChainErrno = SH_CHAIN_EMPTY;
       return NULL;
   }

   if ( (crsr >= pChain->max_cursors) || (crsr <0) )  {
       g_shChainErrno = SH_CHAIN_INVALID_CURSOR;
       return NULL;
   }

   curs = pChain->chainCursor[crsr];
   if (curs->status == NOT_ALLOCATED)  {
       g_shChainErrno = SH_CHAIN_INVALID_CURSOR;
       return NULL;
   }

   /*
    * Okay, now comes the real work. While traversing the chain, user can
    * specify 'whither' to be one of THIS, PREVIOUS, NEXT. The appropriate
    * element, with respect to the cursor, will be returned. Note that if
    * this is the first time the cursor is being used to walk the chain, 
    * simply return the element under the cursor. 
    */
   pChainElem = curs->pChainElement;

   if (curs->gotFirstElem == 0)  {
       /*
        * First time we're traversing the chain, return cursor element.
        */
       ((CURSOR *)curs)->gotFirstElem = 1;
       pData = pChainElem->pElement;
   } else  {
       switch (whither)  {
           case THIS : 
                if (pChainElem == (CHAIN_ELEM *) INVALID_CHAIN_ELEM)  {
                    pData = NULL;
                    g_shChainErrno = SH_CHAIN_INVALID_ELEM;
		} else if (pChainElem == NULL)
                    pData = NULL;
                else
                   pData = pChainElem->pElement;

                break;

           case NEXT:
	        if (pChainElem == (CHAIN_ELEM *) INVALID_CHAIN_ELEM)  {
                    /*
                     * Element under the cursor was deleted. However, the
                     * addresses of it's neighbors were saved. So, let's use
                     * those now.
                     */
		    ((CURSOR *)curs)->pChainElement = curs->pNext;
                    if (curs->pChainElement != NULL)
                     pData = curs->pChainElement->pElement;
                    else
                     pData = NULL;
 
                    ((CURSOR *)curs)->pNext = NULL;
                    ((CURSOR *)curs)->pPrev = NULL;
	        } else if (pChainElem == NULL)  {
                    /*
                     * Reached the end of chain. Set the return value to NULL,
                     * and make the chain ready for traversal again, if the
                     * user so desires. Making a chain ready for traversal
                     * consists of setting the cursor field "gotFirstElem" to
                     * 0.
                     */
                    pData = NULL;
                    ((CURSOR *)curs)->pChainElement = pChain->pFirst;
                    ((CURSOR *)curs)->gotFirstElem  = 0;
                                                               
                } else  {     /* pChainElem != NULL */
                    pChainElem = pChainElem->pNext;
                    if (pChainElem != NULL)  {
                        pData = pChainElem->pElement;
                        ((CURSOR *)curs)->pChainElement = pChainElem;
		    } else  {
                        pData = NULL;
                        ((CURSOR *)curs)->pChainElement = pChain->pFirst;
                        ((CURSOR *)curs)->gotFirstElem  = 0;
                    }
		}

                break;

           case PREVIOUS:
                if (pChainElem == (CHAIN_ELEM *) INVALID_CHAIN_ELEM)  {
                    /*
                     * Element under the cursor was deleted. However, the
                     * address of its neighbors was saved. So let's use that
                     */
                    ((CURSOR *)curs)->pChainElement = curs->pPrev;

                    if (curs->pChainElement != NULL)
                     pData = curs->pChainElement->pElement;
                    else
                     pData = NULL;

                    ((CURSOR *)curs)->pNext = NULL;
                    ((CURSOR *)curs)->pPrev = NULL;
                } else if (pChainElem == NULL)  {
                    /*
                     * Reached the begining of chain. Set the return value to 
                     * NULL and make the chain ready for traversal again, if 
                     * the user so desires. Making a chain ready for traversal
                     * consists of setting the cursor field "gotFirstElem" to
                     * 0
                     */
                    pData = NULL;
                    ((CURSOR *)curs)->pChainElement = pChain->pLast;
                    ((CURSOR *)curs)->gotFirstElem  = 0;
                } else  {   /* pChainElem != NULL */
                    pChainElem = pChainElem->pPrev;
                    if (pChainElem != NULL)  {
                        pData = pChainElem->pElement;
                        ((CURSOR *)curs)->pChainElement = pChainElem;
  		    } else  {
                        pData = NULL;
                        ((CURSOR *)curs)->pChainElement = pChain->pLast;
                        ((CURSOR *)curs)->gotFirstElem = 0;
		    }
                }
 
                break;

	   default :
                shFatal("shChainWalk(): unknown case for parameter 'whither'");
                break;
       }
   }

   return pData;
}

/*****************************************************************************/
/*
 * AUTO EXTRACT
 *
 * Change the data associated with an element of a chain,
 * returning the old value
 *
 * g_shChainErrno is set to
 *   SH_SUCCESS            - on success
 *   SH_CHAIN_EMPTY        - if chain is empty
 *   SH_CHAIN_INVALID_ELEM - if pos > sizeof chain
 */
void *
shChainElementChangeByPos(CHAIN *pChain,
			  unsigned int pos,
			  void *newval)
{
   CHAIN_ELEM *pElem;
   void *oldval;			/* old value of chain's element */

   g_shChainErrno = SH_SUCCESS;

   shAssert(pChain != NULL);

   if (pChain->nElements == 0)  {
       g_shChainErrno = SH_CHAIN_EMPTY;
       return NULL;
   }

   pElem = getElementN(pChain, pos);
   if (pElem == NULL)  {
       g_shChainErrno = SH_CHAIN_INVALID_ELEM;
       return NULL;
   }
   
   oldval = pElem->pElement;
   pElem->pElement = newval;

   return(oldval);
}

/*****************************************************************************/
/*
 * AUTO EXTRACT
 *
 * Change the data associated with a chain cursor, returning the old value
 *
 * g_shChainErrno is set to:
 *   SH_CHAIN_EMPTY          - if the chain is empty
 *   SH_CHAIN_INVALID_CURSOR - if the cursor is not valid
 *   SH_CHAIN_INVALID_ELEM   - if the cursor is pointing to a deleted element
 */
void *
shChainElementChangeByCursor(CHAIN *pChain,
			     const CURSOR_T crsr,
			     void *newval)
{
   const CURSOR *curs;			/* == &pChain->chainCursor[crsr],
					   unaliased for efficiency */
   void *oldval;			/* old value of chain's element */

   g_shChainErrno = SH_SUCCESS;

   shAssert(pChain != NULL);

   if (pChain->nElements == 0)  {
       g_shChainErrno = SH_CHAIN_EMPTY;
       return NULL;
   }

   if ( (crsr >= pChain->max_cursors) || (crsr <0) )  {
       g_shChainErrno = SH_CHAIN_INVALID_CURSOR;
       return NULL;
   }

   curs = pChain->chainCursor[crsr];
   if (curs->status == NOT_ALLOCATED)  {
       g_shChainErrno = SH_CHAIN_INVALID_CURSOR;
       return NULL;
   }

   oldval = curs->pChainElement->pElement;
   curs->pChainElement->pElement = newval;

   return(oldval);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Build an index for a chain
 */
RET_CODE
shChainIndexMake(const CHAIN *pChain, int type)
{
   CHAIN *chain = (CHAIN *)pChain;	/* cast away const; we are not modifying
					   anything on the chain, so this is
					   logically OK */
   int i;
   CHAIN_ELEM **ind;				/* == chain->index, unaliased */
   CHAIN_ELEM *elem;			/* used to traverse chain */

   shAssert(chain != NULL);

   if(type != SH_CHAIN_INDEX_BY_POS) {
      shErrStackPush("shChainIndexMake: index type %d is not supported", type);
      return(SH_GENERIC_ERROR);
   }

   if(chain->index == NULL) {
      chain->index = shMalloc(chain->nElements*sizeof(CHAIN_ELEM *));
   }
   ind = chain->index;

   for(i = 0, elem = chain->pFirst; elem != NULL; elem = elem->pNext, i++) {
      ind[i] = elem;
   }
   shAssert(i == chain->nElements);

   return(SH_SUCCESS);
}

/*
 * <AUTO EXTRACT>
 *
 * Delete a chain's index. Note that this is done automatically when the
 * chain is deleted or modified.
 */
void
shChainIndexDel(const CHAIN *pChain)
{
   CHAIN *chain = (CHAIN *)pChain;	/* cast away const; we are not modifying
					   anything on the chain, so this is
					   logically OK */

   if(chain == NULL) {
      return;
   }

   shFree(chain->index);
   chain->index = NULL;
}

/*
 * <AUTO EXTRACT>
 *
 * Is chain indexed? Return 0 or 1
 */
int
shChainIsIndexed(const CHAIN *chain)
{
   return(chain == NULL || chain->index == NULL ? 0 : 1);
}


/*
 * ROUTINE: shChainSort()
 * 
 * DESCRIPTION:
 *   shChainSort() sorts a chain. The field the sort is to be performed on is
 *   is specified as a parameter. As a side effect, note that any cursors set
 *   on the chain before calling this function will be deleted, since this
 *   function does manipulate the pointers to sort the elements. Thus 
 *   cursors set to elements will probably be invalid.
 *
 *   Note that the sort used is a bubble sort --- one of the worst algorithms
 *   known to man.
 *
 * CALL:
 *   (RET_CODE) shChainSort(CHAIN *pChain, const char *pField, 
 *                          const int increasing)
 *   pChain     - Pointer to the chain
 *   pField     - The field to sort on
 *   increasing - Direction of the sort
 *
 * RETURNS:
 *   SH_SUCCESS       - on success
 *   SH_TYPE_MISMATCH - if the chain is of type GENERIC
 *   SH_FLD_SRCH_ERR  - If the field specified does not exist in the schema
 *   SH_BAD_SCHEMA    - If an unknown schema is encountered
 *
 * g_shChainErrno is set to
 *   SH_SUCCESS       - on success
 *   SH_TYPE_MISMATCH - if the chain is of type GENERIC
 *   SH_FLD_SRCH_ERR  - If the field specified does not exist in the schema
 *   SH_BAD_SCHEMA    - If an unknown schema is encountered
 */
RET_CODE
shChainSort(CHAIN *pChain, const char *pField, int increasing)
{
   void         *pData,
                **itemArray,
                *element;
   TYPE         type;
   CURSOR_T     crsr;
   CHAIN_ELEM   *pElement, *pTmpElement;
   double        value, *af;
   int          i, iNumElems, nswitch = 0;
   const SCHEMA_ELEM *pSchema;

   shAssert(pChain != NULL);

   g_shChainErrno = SH_SUCCESS;

   iNumElems = pChain->nElements;

   if (iNumElems == 0)  {
       return g_shChainErrno; /* which is set to SH_SUCCESS, trivial case */
   } else if (iNumElems == 1)
       return g_shChainErrno;  /* which is set to SH_SUCCESS */

   if (pChain->type == shTypeGetFromName("GENERIC"))  {
       /*
        * Has to be a homogeneous chain 
        */
       g_shChainErrno = SH_TYPE_MISMATCH;
       return g_shChainErrno;
   }

   pData = shChainElementGetByPos(pChain, HEAD);
   type  = shChainElementTypeGetByPos(pChain, HEAD);
     
   if ((pSchema = shSchemaElemGet(shNameGetFromType(type), (char *)pField)) 
        == NULL)  
   {
       g_shChainErrno = SH_FLD_SRCH_ERR;
       return g_shChainErrno;
   }

   crsr = shChainCursorNew(pChain);
           
   af = (double*)shMalloc(iNumElems * sizeof(double));
   itemArray = (void**)shMalloc(iNumElems * sizeof(void *));

   i = 0;
   while ((pData = shChainWalk(pChain, crsr, NEXT)) != NULL)  {
           type = shChainElementTypeGetByCursor(pChain, crsr);
           element = shElemGet(pData, (SCHEMA_ELEM *)pSchema, &type);
           sscanf(shPtrSprint(element, type), "%lf", &value);
           af[i] = value * (2 * increasing - 1);
           itemArray[i++] = pData;
   }

   if (g_shChainErrno != SH_SUCCESS)  {
       /*
        * If pData == NULL in the above test, some error occurred in 
        * shChainWalk(). In that case, return g_shChainErrno
        */
       shFree(af);
       shFree(itemArray);
       return g_shChainErrno;
   }

   /*
    * Do a bubble sort on af. If we swap the values of af, we'll also
    * swap the values of itemArray. In this manner, itemArray will be
    * sorted correctly; and since itemArray contains pointers to the
    * elements on the list, we have a sorted list!
    */
   do  {
       nswitch = 0;
       for (i = 0; i < iNumElems - 1; i++)  { 
            if (af[i] > af[i+1])  {
                /*
                 * Swap the values in the af 
                 */
                value = af[i];
                af[i] = af[i+1];
                af[i+1] = value;
                /*
                 * Now swap the values in the list as well
                 */
                pData = itemArray[i];
                itemArray[i] = itemArray[i+1];
                itemArray[i+1] = pData;
                
                nswitch++;
            }
       }

   } while (nswitch != 0);

   /*
    * Now rebuild the chain. Note that we will delete all the elements
    * on the chain, but NOT the chain itself. We can delete all the
    * elements, since pointers are saved in itemArray, and since this is
    * a generic chain, each element type is the same as chain type. We will
    * then recreate the chain, only this time it will be sorted. One
    * side effect is that all the cursor information will be lost. But that
    * is to be expected: if a cursor is a "context-sensitive" device, then
    * it stands to reason that when chain is re-ordered as a result of a sort,
    * the context has been disrupted. So, the context of a cursor no longer
    * makes much sense.
    */
   pElement = pChain->pFirst;
   while (pElement != NULL)  {
       pTmpElement = pElement->pNext;

#ifdef DEBUG
       FreeMemory(pElement, __FILE__, __LINE__);
#endif
       shFree(pElement);
       pElement = pTmpElement;
   }
   pChain->pFirst = pChain->pLast = pChain->pCacheElem = NULL;
   pChain->nElements = pChain->cacheElemPos = 0;
   for (i = 0; i < pChain->max_cursors; i++)  {
        pChain->chainCursor[i]->pChainElement = NULL;
        pChain->chainCursor[i]->pNext         = NULL;
        pChain->chainCursor[i]->pPrev         = NULL;
        pChain->chainCursor[i]->status        = NOT_ALLOCATED;
   }

   for (i = 0; i < iNumElems; i++)
        if (shChainElementAddByPos(pChain, itemArray[i], 
                   shNameGetFromType(pChain->type), TAIL, AFTER) != SH_SUCCESS)
            break;
   
       
   shFree(af);
   shFree(itemArray);

   return g_shChainErrno;  /* which is set in shChainElementAddByPos() */
}

/*
 * ROUTINE: shChainCompare()
 * 
 * DESCRIPTION:
 *   shChainCompare() compares chain1 to chain2.  The closestChain corresponds
 *   to chain1 and points to the element of chain2 that is closes to that 
 *   element in chain1.  Also, set vMask value to 1 if the separation is 
 *   less than minimumDelta.
 *
 *   After this call, step through chain1 and closestChain at the same time.
 *   Element i of closestChain is the element of chain2 that is closest
 *   to element i of chain1, and vMask->vec[i-1]=1 if the separation
 *   is less than minimumDelta.
 *
 *   WARNING WARNING WARNING:  Note that the elements in chains are numbered
 *   starting at 1 and the elements in vMask are numbered starting at 0.
 *
 * CALL:
 *   (RET_CODE) shChainCompare(CHAIN *chain1, char *row1Name, char *col1Name,
 *                             CHAIN *chain2, char *row2Name, char *col2Name,
 *                             float minimumDelta, 
 *                             CHAIN *closestChain, VECTOR *vMask)
 *
 *   chain1     - the first chain
 *   row1Name   - name in the schema for "row", chain 1
 *   col1Name   - name in the schame for "col", chain 1
 *   chain2, row2Name, col2Name - same as above, for chain 2
 *   minimumDelta - the threshold for setting vMask
 *   closestChain - the answer; closest element on chain 2 for each element
 *                  of chain 1.
 *   vMask - mask, set to 1 if the separation is less than minimumDelta.
 *
 * RETURNS:
 *   SH_SUCCESS       - on success
 *   SH_GENERIC_ERROR - if there is some kind of trouble
 *
 */
RET_CODE
shChainCompare(CHAIN *chain1, char *row1Name, char *col1Name,
	       CHAIN *chain2, char *row2Name, char *col2Name,
	       float minimumDelta, 
	       CHAIN *closestChain, VECTOR *vMask) {
  int nItem1, nItem2;
  void *item1, *item2;
  void *element1, *element2;
  const SCHEMA *schema1, *schema2;
  TYPE type1, type2, type1First, type2First;
  int iS1Row, iS1Col, iS2Row, iS2Col, iSchema;
  int iItem1, iItem2;
  float delta;
  float row1, col1, row2, col2;
  float minimum = 0.0;
  void *closest_item2 = NULL;
  
  nItem1 = shChainSize(chain1);
  nItem2 = shChainSize(chain2);
  if ( (nItem1 <=0) || (nItem2 <= 0) ) {
    shErrStackPush("shChainCompare:  no elements on one of the chains");
    return SH_GENERIC_ERROR;
  }

  
/* Get schema for the two chains.  Bail out if row and col aren't defined */
  item1 = shChainElementGetByPos(chain1, HEAD);
  type1 = shChainElementTypeGetByPos(chain1, HEAD);
  type1First = type1;
  if ((schema1 = (SCHEMA *)shSchemaGet(shNameGetFromType(type1))) == NULL) {
    shErrStackPush("shChainCompare:  can not get schema for chain1 type %s",
		   shNameGetFromType(type1));
    return SH_GENERIC_ERROR;
  }
  iS1Row = -1;
  iS1Col = -1;
  for (iSchema = 0; iSchema < schema1->nelem; iSchema++) {
    if (strcmp(schema1->elems[iSchema].name, row1Name) == 0) iS1Row = iSchema;
    if (strcmp(schema1->elems[iSchema].name, col1Name) == 0) iS1Col = iSchema;
  }
  if ( (iS1Row==-1) || (iS1Col==-1) ) {
    shErrStackPush("%s and/or %s not in schema for %s",
		   row1Name, col1Name, shNameGetFromType(type1));
    return SH_GENERIC_ERROR;
  }
  
  
  item2 = shChainElementGetByPos(chain2, HEAD);
  type2 = shChainElementTypeGetByPos(chain2, HEAD);
  type2First = type2;
    if ((schema2 = (SCHEMA *)shSchemaGet(shNameGetFromType(type2))) == NULL) {
    shErrStackPush("shChainCompare:  can not get schema for chain2 type %s",
		   shNameGetFromType(type2));
    return SH_GENERIC_ERROR;
  }
  iS2Row = -1;
  iS2Col = -1;
  for (iSchema = 0; iSchema < schema2->nelem; iSchema++) {
    if (strcmp(schema2->elems[iSchema].name, row2Name) == 0) iS2Row = iSchema;
    if (strcmp(schema2->elems[iSchema].name, col2Name) == 0) iS2Col = iSchema;
  }
  if ( (iS2Row==-1) || (iS2Col==-1) ) {
    shErrStackPush("%s and/or %s not in schema for %s",
		   row2Name, col2Name, shNameGetFromType(type2));
    return SH_GENERIC_ERROR;
  }


/* Iterate over chain1.  Put the closest member of chain2 on closest */

  for (iItem1=0; iItem1 < nItem1; iItem1++) {
    vMask->vec[iItem1] = 0.0;
    item1 = shChainElementGetByPos(chain1, iItem1);
    type1 = type1First;
    element1 = shElemGet(item1, &schema1->elems[iS1Row], &type1);
    sscanf(shPtrSprint(element1, type1), "%f", &row1);
    type1 = type1First;
    element1 = shElemGet(item1, &schema1->elems[iS1Col], &type1);
    sscanf(shPtrSprint(element1, type1), "%f", &col1);
    for (iItem2=0; iItem2 < nItem2; iItem2++) {
      item2 = shChainElementGetByPos(chain2, iItem2);
      type2 = type2First;
      element2 = shElemGet(item2, &schema2->elems[iS2Row], &type2);
      sscanf(shPtrSprint(element2, type2), "%f", &row2);
      type2 = type2First;
      element2 = shElemGet(item2, &schema2->elems[iS2Col], &type2);
      sscanf(shPtrSprint(element2, type2), "%f", &col2);
      delta = pow(row1-row2,2) + pow(col1-col2,2);
      if (delta<minimum || iItem2==0) {
	minimum = delta;
	if (minimum <= minimumDelta) {
	  vMask->vec[iItem1] = 1.0;
	}
	closest_item2 = item2;
      }
    }
    shChainElementAddByPos(closestChain, closest_item2, 
                           shNameGetFromType(type2First), TAIL, AFTER);
  }
  return SH_SUCCESS;
}

/*
 * ROUTINE: shChainCompare2()
 * 
 * DESCRIPTION:
 *
 * This is a "new and improved" version of shChainCompare 
 *
 *   shChainCompare2() compares chain1 to chain2.  The closestChain corresponds
 *   to chain1 and points to the element of chain2 that is closes to that 
 *   element in chain1.  
 *
 *   After this call, step through chain1 and closestChain at the same time.
 *   Element i of closestChain is the element of chain2 that is closest
 *   to element i of chain1.
 *
 *
 * RETURNS:
 *   SH_SUCCESS       - on success
 *   SH_GENERIC_ERROR - if there is some kind of trouble
 *
 */
RET_CODE
shChainCompare2(CHAIN *chain1, char *row1Name, char *col1Name,
		CHAIN *chain2, char *row2Name, char *col2Name,
		CHAIN *closestChain) {
  int nItem1, nItem2;
  int iItem1, iItem2;
  int iItem2Closest;
  float delta;
  float minimum = 0.0;
  void *closest_item2 = NULL;
  VECTOR *row1=NULL, *col1=NULL, *row2=NULL, *col2=NULL;
  double r1, c1, r2, c2;
  TYPE type2First;

  nItem1 = shChainSize(chain1);
  nItem2 = shChainSize(chain2);
  if ( (nItem1 <=0) || (nItem2 <= 0) ) {
    shErrStackPush("shChainCompare2:  no elements on one of the chains");
    return SH_GENERIC_ERROR;
  }

  /* Get the type of chain2 */
  type2First = shChainElementTypeGetByPos(chain2, HEAD);
  
  /* Get vectors for the two chains.  Bail out if row and col aren't defined */
  row1 = shVFromChain(chain1, row1Name);
  col1 = shVFromChain(chain1, col1Name);
  row2 = shVFromChain(chain2, row2Name);
  col2 = shVFromChain(chain2, col2Name);
  if ((row1==NULL) || (col1==NULL) || (row2==NULL) || (col2==NULL)) {
    shErrStackPush("shChainCompare2:  invalid member specified");
    shVectorDel(row1);  /* Don't worry - if NULL, nothing happens */
    shVectorDel(row2);
    shVectorDel(col1);
    shVectorDel(col2);
    return SH_GENERIC_ERROR;
  }

  for (iItem1=0; iItem1 < nItem1; iItem1++) {
    r1 = row1->vec[iItem1];
    c1 = col1->vec[iItem1];
    for (iItem2=0; iItem2 < nItem2; iItem2++) {
      r2 = row2->vec[iItem2];
      c2 = col2->vec[iItem2];
      delta = pow(r1-r2,2) + pow(c1-c2,2);
      if (delta<minimum || iItem2==0) {
	minimum = delta;
	iItem2Closest = iItem2;
      }
    }
    closest_item2 = shChainElementGetByPos(chain2, iItem2Closest);
    shChainElementAddByPos(closestChain, closest_item2, 
                           shNameGetFromType(type2First), TAIL, AFTER);
  }
  shVectorDel(row1);
  shVectorDel(row2);
  shVectorDel(col1);
  shVectorDel(col2);
  return SH_SUCCESS;
}


#ifdef DEBUG
static void
GetMemory(void *pChain, size_t nBytes, char *pFileName, int lineNo)
{
   static short init = 0;
   FILE   *fp;

   if (init == 0)
       unlink("dervish.debug");

   init = 1;
   fp = fopen("dervish.debug", "a+");
   fprintf(fp, "Malloc: %04d bytes at %X. File %s, Line %d\n", nBytes, pChain,
           pFileName, lineNo-3);
   fclose(fp);

   return;
}

      
static void 
FreeMemory(void *pMem, char *pFileName, int lineNo)
{
   FILE *fp;

   fp = fopen("dervish.debug", "a+");
   fprintf(fp, "Free  : Memory at %X. File %s, Line %d\n", pMem, pFileName,
           lineNo+2);
   fclose(fp);

   return;
}
#endif
