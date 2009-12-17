#include <unistd.h>
#include <string.h>

#include "shCSchema.h"
#include "shChain.h"
#include "region.h"
#include "shCHg.h"
#include "libfits.h"

#if defined(DARWIN)
   void *g_saoCmdHandle;
#endif

static int testAdd(void);
static int testAddAndRemove(void);
static int testElementTypeGetByPos(void);
static int testElementGetByPos(void);
static int testMisc(void);
static int testJoin(void);
static int testCopy(void);
static int testTypeDefine(void);
static int testCursors(void);
static int testSort(void);
static int testWalk(void);
static int testWalk2(void);
static int testElementRemByCursor(void);
static int testChainCursorCopy(void);

static void printChain(const char *debugTag, const CHAIN *);

extern RET_CODE shSchemaLoadFromDervish(void);   
                                        /* load the DERVISH schema definitions */

static struct  {
   int (*funcp)(void);
   char *pFuncName;
} TestFuncs[] = {
                    {testAdd,                 "testAdd()"},
                    {testElementTypeGetByPos, "testElementTypeGetByPos()"},
                    {testElementGetByPos,     "testElementGetByPos()"},
                    {testAddAndRemove,        "testAddAndRemove()"},
                    {testMisc,                "testMisc()"},
                    {testJoin,                "testJoin()"},
                    {testCopy,                "testCopy()"},
                    {testTypeDefine,          "testTypeDefine()"},
                    {testCursors,             "testCursors()"},
                    {testWalk,                "testWalk()"},
                    {testWalk2,               "testWalk2()"},
                    {testElementRemByCursor,  "testElementRemByCursor()"},
                    {testSort,                "testSort()"},
                    {testChainCursorCopy,     "testChainCursorCopy()"},
                    {NULL,                    NULL}
		};
int main(void)
{
   int i;

   load_libfits(1);		/* force ld to load some symbols;
				   usually called by f_fopen() */
   (void) shSchemaLoadFromDervish();

   for (i = 0; TestFuncs[i].funcp != NULL; i++)  {
        if (TestFuncs[i].funcp())  {
            fprintf(stderr, "tst_chain: Failed in function %s\n",
                    TestFuncs[i].pFuncName);
            return 1;
        }
   }

   return 0;
}

static void
printChain(const char *debugTag, const CHAIN *pChain)
{
   CHAIN_ELEM *pElem;
   char *pMyData;

   printf("-------- %s --------\n", debugTag);
   pElem = pChain->pFirst;

   for (; pElem != NULL; pElem = pElem->pNext)  {
        pMyData = (char *)pElem->pElement;
        printf("(%X)Data:   [%s]\n", pElem, pMyData);
        if (pElem->pPrev == NULL)
            printf("   Previous = NULL\n");
        else
            printf("   Previous = %X\n", pElem->pPrev);
        if (pElem->pNext == NULL)
            printf("   Next = NULL\n");
        else
            printf("   Next = %X\n", pElem->pNext);
   }
   printf("------------------\n");
}

static int
testChainCursorCopy(void)
{
   int   retValue = 0;
   char *expectedChain[5] = {"MASK0", "MASK1", "MASK2", "MASK3", "MASK4"};
   CURSOR_T  origCrsr, newCrsr;
   CHAIN    *pChain;
   MASK     *pMask;
   char     *pFuncName;

   pChain = shChainNew("MASK");
   origCrsr = newCrsr = -1;
   pFuncName = "testChainCursorCopy";

   if (shChainElementAddByPos(pChain, shMaskNew("MASK0", 10, 10), "MASK", 
                              TAIL, AFTER) != SH_SUCCESS)
       return 1;
   if (shChainElementAddByPos(pChain, shMaskNew("MASK1", 10, 10), "MASK", 
                              TAIL, AFTER) != SH_SUCCESS)
       return 1;
   if (shChainElementAddByPos(pChain, shMaskNew("MASK2", 10, 10), "MASK", 
                              TAIL, AFTER) != SH_SUCCESS)
       return 1;
   if (shChainElementAddByPos(pChain, shMaskNew("MASK3", 10, 10), "MASK", 
                              TAIL, AFTER) != SH_SUCCESS)
       return 1;
   if (shChainElementAddByPos(pChain, shMaskNew("MASK4", 10, 10), "MASK", 
                              TAIL, AFTER) != SH_SUCCESS)
       return 1;
   if (shChainElementAddByPos(pChain, shMaskNew("MASK5", 10, 10), "MASK",
                              TAIL, AFTER) != SH_SUCCESS)
       return 1;

   if ( (origCrsr = shChainCursorNew(pChain)) == INVALID_CURSOR ) {
     fprintf(stderr,"%s: shChainCursorNew() failed\n",pFuncName);
     retValue = 1;
     goto done;
   }
   
   pMask = shChainWalk(pChain, origCrsr, NEXT);  /* On MASK1 */
   if (shChainCursorCopy(pChain, origCrsr, &newCrsr) != SH_SUCCESS)  {
       fprintf(stderr, "%s: shChainCursorCopy() failed 1\n", pFuncName);
               retValue = 1;
               goto done;
   }
   if (shChainWalk(pChain, newCrsr, THIS) != pMask)  {
       fprintf(stderr, "%s: copied cursor != original cursor!\n", pFuncName);
       retValue = 1;
       goto done;
   }

   /*
    * Now both should return MASK2
    */
   if (shChainWalk(pChain, origCrsr, NEXT) != 
                                         shChainWalk(pChain, newCrsr, NEXT))  {
       fprintf(stderr, "%s: NEXT on origCrsr != NEXT on newCrsr\n", pFuncName);
       retValue = 1;
       goto done;
   }

done:
   if (origCrsr != -1)  {
       shChainCursorSet(pChain, origCrsr, HEAD);
       while ((pMask = shChainWalk(pChain, origCrsr, NEXT)) != NULL)
               shMaskDel(pMask);
       shChainCursorDel(pChain, origCrsr);
       if (newCrsr != -1)
           shChainCursorDel(pChain, newCrsr);
       shChainDel(pChain);
   }

   return retValue;
}

static int
testAdd(void)
{
   CHAIN  *pChain;
   MASK   *pMask1, *pMask2, *pMask3, *pMask4, *pMask5, *pMask;
   REGION *pReg1,  *pReg2,  *pReg3,  *pReg4,  *pReg5,  *pReg;
   TYPE   type, mType, rType;
   char   *elemName;
   int    retVal,i;
   char   *expectedChain[10] = {"MASK2", "MASK3", "REG5",  "MASK1", "REG1",
                                "REG2",  "REG4",  "MASK5", "MASK4", "REG3"};

   retVal = 0;
   type  = shTypeGetFromName("GENERIC");
   mType = shTypeGetFromName("MASK");
   rType = shTypeGetFromName("REGION");

   pChain = shChainNew("GENERIC");

   pMask1 = shMaskNew("MASK1", 10, 10);
   pMask2 = shMaskNew("MASK2", 10, 10);
   pMask3 = shMaskNew("MASK3", 10, 10);
   pMask4 = shMaskNew("MASK4", 10, 10);
   pMask5 = shMaskNew("MASK5", 10, 10);

   pReg1  = shRegNew("REG1", 10, 10, TYPE_S8);
   pReg2  = shRegNew("REG2", 10, 10, TYPE_S8);
   pReg3  = shRegNew("REG3", 10, 10, TYPE_S8);
   pReg4  = shRegNew("REG4", 10, 10, TYPE_S8);
   pReg5  = shRegNew("REG5", 10, 10, TYPE_S8);

 if (shChainElementAddByPos(pChain, pMask1, "MASK", TAIL, AFTER) != SH_SUCCESS)
     return 1;
 if (shChainElementAddByPos(pChain, pReg1, "REGION", TAIL, AFTER)!= SH_SUCCESS)
     return 1;
 if (shChainElementAddByPos(pChain, pMask2, "MASK", HEAD, BEFORE)!= SH_SUCCESS)
     return 1;
 if (shChainElementAddByPos(pChain, pReg2, "REGION", 2, AFTER) != SH_SUCCESS)
     return 1;
 if (shChainElementAddByPos(pChain, pReg3, "REGION", 3, AFTER) != SH_SUCCESS)
     return 1;
 if (shChainElementAddByPos(pChain, pMask3, "MASK", 1, BEFORE) != SH_SUCCESS)
     return 1;
 if (shChainElementAddByPos(pChain, pMask4, "MASK", 5, BEFORE) != SH_SUCCESS)
     return 1;
 if (shChainElementAddByPos(pChain, pReg4, "REGION", 4, AFTER) != SH_SUCCESS)
     return 1;
 if (shChainElementAddByPos(pChain, pMask5, "MASK", 5, AFTER) != SH_SUCCESS)
     return 1;
 if (shChainElementAddByPos(pChain, pReg5, "REGION", 2, BEFORE) != SH_SUCCESS)
     return 1;

   for (i = 0; i < shChainSize(pChain); i++)  {
        type = shChainElementTypeGetByPos(pChain, i);
        if (type == mType)  {
            pMask = shChainElementGetByPos(pChain, i);
            elemName = pMask->name;
	} else {
            pReg = shChainElementGetByPos(pChain, i);
            elemName = pReg->name;
        }
        if (strncmp(elemName, expectedChain[i], strlen(elemName)) != 0)  {
            retVal = 1;
            fprintf(stderr, "testAdd: expected element \"%s\", got \"%s\"\n",
                    expectedChain[i], elemName);
            goto done;
        }
   }

 shChainDel(pChain);
 pChain = shChainNew("MASK");   /* Let's test some more */
 if (shChainElementAddByPos(pChain, pMask4, "MASK", HEAD, BEFORE)!= SH_SUCCESS)
     return 1;
 if (shChainElementAddByPos(pChain, pMask1, "MASK", TAIL, AFTER) != SH_SUCCESS)
     return 1;
 if (shChainElementAddByPos(pChain, pMask3, "MASK", TAIL, AFTER) != SH_SUCCESS)
     return 1;
 if (shChainElementAddByPos(pChain, pMask5, "MASK", TAIL, AFTER)!= SH_SUCCESS)
     return 1;
 if (shChainElementAddByPos(pChain, pMask2, "MASK", TAIL, AFTER) != SH_SUCCESS)
     return 1;

   expectedChain[0] = "MASK4"; expectedChain[1] = "MASK1";
   expectedChain[2] = "MASK3"; expectedChain[3] = "MASK5";
   expectedChain[4] = "MASK2";

   for (i = 0; i < shChainSize(pChain); i++)  {
        pMask = shChainElementGetByPos(pChain, i);
        if (strncmp(pMask->name, expectedChain[i], 5) != 0)  {
            retVal = 1;
            fprintf(stderr, "testAdd: expected element \"%s\", got \"%s\"\n",
                    expectedChain[i], pMask->name);
            goto done;
        }
   }

   shChainDel(pChain);
   pChain = shChainNew("REGION");  /* One more test */
 if (shChainElementAddByPos(pChain, pReg5, "REGION", TAIL, BEFORE)!=SH_SUCCESS)
     return 1;
 if (shChainElementAddByPos(pChain, pReg3, "REGION", HEAD, BEFORE)!=SH_SUCCESS)
     return 1;
 if (shChainElementAddByPos(pChain, pReg1, "REGION", HEAD, BEFORE)!=SH_SUCCESS)
     return 1;
 if (shChainElementAddByPos(pChain, pReg4, "REGION", HEAD, BEFORE)!=SH_SUCCESS)
     return 1;
 if (shChainElementAddByPos(pChain, pReg2, "REGION", HEAD, BEFORE)!=SH_SUCCESS)
     return 1;

   expectedChain[0] = "REG2"; expectedChain[1] = "REG4";
   expectedChain[2] = "REG1"; expectedChain[3] = "REG3";
   expectedChain[4] = "REG5";
 
   for (i = 0; i < shChainSize(pChain); i++)  {
        pReg = shChainElementGetByPos(pChain, i);
        if (strncmp(pReg->name, expectedChain[i], 4) != 0)  {
            retVal = 1;
            fprintf(stderr, "testAdd: expected element \"%s\", got \"%s\"\n",
                    expectedChain[i], pReg->name);
            break;
        } 
   }

done:
   shChainDel(pChain);
   shMaskDel(pMask1);
   shMaskDel(pMask2);
   shMaskDel(pMask3);
   shMaskDel(pMask4);
   shMaskDel(pMask5);
   shRegDel(pReg1);
   shRegDel(pReg2);
   shRegDel(pReg3);
   shRegDel(pReg4);
   shRegDel(pReg5);

   return retVal;
}

static int
testElementTypeGetByPos(void)
{
   /*
    * This routine creates a generic chain of 4 elements as follows:
    *   elem_1 (REGION) --> elem_2 (MASK) --> elem_3 (REGION) --> elem_4 (MASK)
    *
    * It then tests routine shChainElementTypeGetByPos()
    */
   CHAIN  *pChain;
   REGION *pRegion;
   MASK   *pMask;
   TYPE   type;
   char   *pGotType;

   pChain = shChainNew("GENERIC");

   pRegion = shRegNew("reg1", 10, 10, TYPE_U8);
   pMask   = shMaskNew("mask1", 10, 10);

   if (shChainElementAddByPos(pChain, pRegion, "REGION", TAIL, AFTER) != 
       SH_SUCCESS)  {
       fprintf(stderr, 
          "testElementTypeGetByPos: error inserting a REGION on a chain\n");
       return 1;
   }
   if (shChainElementAddByPos(pChain, pMask, "MASK", TAIL, AFTER) != 
       SH_SUCCESS)  {
       fprintf(stderr, 
          "testElementTypeGetByPos: error inserting a MASK on a chain\n");
       return 1;
   }
   if (shChainElementAddByPos(pChain, pRegion, "REGION", TAIL, AFTER) != 
       SH_SUCCESS)  {
       fprintf(stderr, 
          "testElementTypeGetByPos: error inserting a REGION on a chain\n");
       return 1;
   }
   if (shChainElementAddByPos(pChain, pMask, "MASK", TAIL, AFTER) != 
       SH_SUCCESS)  {
       fprintf(stderr, 
          "testElementTypeGetByPos: error inserting a MASK on a chain\n");
       return 1;
   }

   pGotType = NULL;
   type = shChainElementTypeGetByPos(pChain, HEAD);
   pGotType = shNameGetFromType(type);
   if (strncmp(pGotType, "REGION", 6) != 0)  {
       fprintf(stderr, "testElementTypeGetByPos: 1: expecting return of REGION"
           ". Got a %s instead!!\n", pGotType);
       return 1;
   }

   pGotType = NULL;
   type = shChainElementTypeGetByPos(pChain, TAIL);
   pGotType = shNameGetFromType(type);
   if (strncmp(pGotType, "MASK", 4) != 0)  {
       fprintf(stderr, "testElementTypeGetByPos: 2: expecting return of MASK"
           ". Got a %s instead!!\n", pGotType);
       return 1;
   }

   pGotType = NULL;
   type = shChainElementTypeGetByPos(pChain, 1);
   pGotType = shNameGetFromType(type);
   if (strncmp(pGotType, "MASK", 4) != 0)  {
       fprintf(stderr, "testElementTypeGetByPos: 3: expecting return of MASK"
           ". Got a %s instead!!\n", pGotType);
       return 1;
   }

   pGotType = NULL;
   type = shChainElementTypeGetByPos(pChain, 2);
   pGotType = shNameGetFromType(type);
   if (strncmp(pGotType, "REGION", 6) != 0)  {
       fprintf(stderr, "testElementTypeGetByPos: 4: expecting return of REGION"
           ". Got a %s instead!!\n", pGotType);
       return 1;
   }

   shChainDel(pChain);
   shMaskDel(pMask);
   shRegDel(pRegion);

   return 0;
}

static int
testElementGetByPos(void)
{
   /* 
    * This routine puts some elements on a chain and then tries to get an
    * address to them.
    */
   char *p01 = "One", *p02 = "Two", *p03 = "Three", *p04 = "Four", *pData;
   CHAIN *pChain;
   int   retValue;
   
   pChain = shChainNew("UNKNOWN");
   retValue = 0;
   /* 
    * Elements are added at the end of the chain
    */
   shChainElementAddByPos(pChain, p01, "UNKNOWN", TAIL, AFTER);
   shChainElementAddByPos(pChain, p02, "UNKNOWN", TAIL, AFTER);
   shChainElementAddByPos(pChain, p03, "UNKNOWN", TAIL, AFTER);
   shChainElementAddByPos(pChain, p04, "UNKNOWN", TAIL, AFTER);

   pData = (char *)shChainElementGetByPos(pChain, HEAD);
   if (strncmp(pData, "One", 3) != 0)  {
       fprintf(stderr, 
         "testElementGetByPos: Expecting the HEAD element to be \"One\""
         "\nBut got %s instead\n", pData);
       retValue = 1;
       goto done;
   }

   pData = (char *)shChainElementGetByPos(pChain, TAIL);
   if (strncmp(pData, "Four", 4) != 0)  {
       fprintf(stderr, 
         "testElementGetByPos: Expecting the TAIL element to be \"Four\""
         "\nBut got %s instead\n", pData);
       retValue = 1;
       goto done;
   }

   pData = (char *)shChainElementGetByPos(pChain, 3);
   if (strncmp(pData, "Four", 4) != 0)  {
       fprintf(stderr, 
         "testElementGetByPos: Expecting the 3rd element to be \"Four\""
         "\nBut got %s instead\n", pData);
       retValue = 1;
       goto done;
   }

   pData = (char *)shChainElementGetByPos(pChain, 2);
   if (strncmp(pData, "Three", 5) != 0)  {
       fprintf(stderr, 
         "testElementGetByPos: Expecting the 2 element to be \"Three\""
         "\nBut got %s instead\n", pData);
       retValue = 1;
       goto done;
   }

   pData = (char *)shChainElementGetByPos(pChain, 1);
   if (strncmp(pData, "Two", 3) != 0)  {
       fprintf(stderr, 
         "testElementGetByPos: Expecting the 1st element to be \"Two\""
         "\nBut got %s instead\n", pData);
       retValue = 1;
       goto done;
   }

   pData = (char *)shChainElementGetByPos(pChain, 0);
   if (strncmp(pData, "One", 3) != 0)  {
       fprintf(stderr, 
         "testElementGetByPos: Expecting the 0th element to be \"One\""
         "\nBut got %s instead\n", pData);
       retValue = 1;
       goto done;
   }

   pData = (char *)shChainElementGetByPos(pChain, 4);
   if (pData != NULL)  {
       fprintf(stderr, 
         "testElementGetByPos: There should not be a 4th element, stupid!\n"
         "\nBut still got %s\n", pData);
       retValue = 1;
       goto done;
   }

done:
   shChainDel(pChain);
   return retValue;
}

static int
testAddAndRemove(void)
{
   CHAIN *pChain;
   char *p1 = "One", *p2 = "Two", *p3 = "Three", *p4 = "Four", *pData;
   int retValue;

   retValue = 0;
   pChain = shChainNew("UNKNOWN");

   /*
    * Now create a chain of 3 elements. Elements are added to the end
    */
   if (shChainElementAddByPos(pChain, p1, "UNKNOWN", TAIL, AFTER) != 
       SH_SUCCESS)  {
       fprintf(stderr, 
               "testAddAndRemove: cannot add first element to chain\n");
       retValue = 1;
       goto done;
   }

   if (shChainElementAddByPos(pChain, p2, "UNKNOWN", TAIL, AFTER) != 
       SH_SUCCESS)  {
       fprintf(stderr, 
               "testAddAndRemove: cannot add second element to chain\n");
       retValue = 1;
       goto done;
   }

   if (shChainElementAddByPos(pChain, p3, "UNKNOWN", TAIL, AFTER) != 
       SH_SUCCESS)  {
       fprintf(stderr, 
               "testAddAndRemove: cannot add third element to chain\n");
       retValue = 1;
       goto done;
   }

   if (pChain->nElements != 3)  {
       fprintf(stderr,
           "testAddAndRemove: expecting 3 elements on chain, but have %d\n",
           pChain->nElements);
       retValue = 1;
       goto done;
    }

   pData = (char *)shChainElementGetByPos(pChain, HEAD);
   if (strncmp(pData, "One", 3) != 0)  {
      fprintf(stderr, 
          "testAddAndRemove: Expecting \"One\" to be the first element on"
          "chain\nbut got %s instead!\n", pData);
      retValue = 1;
      goto done;
   }

   pData = (char *)shChainElementGetByPos(pChain, TAIL);
   if (strncmp(pData, "Three", 5) != 0)  {
      fprintf(stderr, 
          "testAddAndRemove: Expecting \"Three\" to be the last element on"
          "chain\nbut got %s instead!\n", pData);
      retValue = 1;
      goto done;
   }

   pData = (char *)shChainElementGetByPos(pChain, 1);
   if (strncmp(pData, "Two", 3) != 0)  {
      fprintf(stderr,
          "testAddAndRemove: Expecting \"Two\" to be the second element on"
          "chain\nbut got %s instead!\n", pData);
      retValue = 1;
      goto done;
   }

   /*
    * Now insert p4 in various positions and see if the resulting chain is
    * correct. Also, delete the newly inserted p4 and see if the resulting
    * chain is correct
    */
   if (shChainElementAddByPos(pChain, p4, "UNKNOWN", HEAD, BEFORE) != 
       SH_SUCCESS)  {
       fprintf(stderr, 
           "testAddAndRemove: cannot add new element BEFORE the HEAD\n");
       retValue = 1;
       goto done;
   }

   if (pChain->nElements != 4)  {
       fprintf(stderr,
           "testAddAndRemove: expecting 4 elements on chain, but have %d\n",
            pChain->nElements);
       retValue = 1;
       goto done;
    }
        
   pData = (char *)shChainElementRemByPos(pChain, HEAD);

   if (strncmp(pData, "Four", 4) != 0)  {
       fprintf(stderr,
           "testAddAndRemove: expecting element \"Four\" to be removed"
           " but removed %s\n", pData);
       retValue = 1;
       goto done;
   }

   if (pChain->nElements != 3)  {
       fprintf(stderr,
           "testAddAndRemove: expecting 3 elements on chain, but have %d\n",
           pChain->nElements);
       retValue = 1;
       goto done;
    }

   if (shChainElementAddByPos(pChain, p4, "UNKNOWN", 2, AFTER) != SH_SUCCESS) {
       fprintf(stderr, 
           "testAddAndRemove: cannot add new element AFTER the 2nd\n");
       retValue = 1;
       goto done;
   }

   if (pChain->nElements != 4)  {
       fprintf(stderr,
           "testAddAndRemove: expecting 4 elements on chain, but have %d\n",
            pChain->nElements);
       retValue = 1;
       goto done;
    }
        
   pData = (char *)shChainElementRemByPos(pChain, 3);

   if (strncmp(pData, "Four", 4) != 0)  {
       fprintf(stderr,
           "testAddAndRemove: expecting element \"Four\" to be removed"
           " but removed %s\n", pData);
       retValue = 1;
       goto done;
   }

   if (pChain->nElements != 3)  {
       fprintf(stderr,
           "testAddAndRemove: expecting 3 elements on chain, but have %d\n",
           pChain->nElements);
       retValue = 1;
       goto done;
    }

   if (shChainElementAddByPos(pChain, p4, "UNKNOWN", 1, AFTER) != SH_SUCCESS) {
       fprintf(stderr, 
           "testAddAndRemove: cannot add new element AFTER the 1st\n");
       retValue = 1;
       goto done;
   }

   if (pChain->nElements != 4)  {
       fprintf(stderr,
           "testAddAndRemove: expecting 4 elements on chain, but have %d\n",
            pChain->nElements);
       retValue = 1;
       goto done;
    }
        
   pData = (char *)shChainElementRemByPos(pChain, 2);

   if (strncmp(pData, "Four", 4) != 0)  {
       fprintf(stderr,
           "testAddAndRemove: expecting element \"Four\" to be removed"
           " but removed %s\n", pData);
       retValue = 1;
       goto done;
   }

   if (pChain->nElements != 3)  {
       fprintf(stderr,
           "testAddAndRemove: expecting 3 elements on chain, but have %d\n",
           pChain->nElements);
       retValue = 1;
       goto done;
    }

   shChainElementRemByPos(pChain, HEAD);
   shChainElementRemByPos(pChain, HEAD);
   shChainElementRemByPos(pChain, HEAD);

   if (shChainElementGetByPos(pChain, HEAD) != NULL)  {
       fprintf(stdout, 
           "testAddAndRemove: expecting a NULL return here...\n");
       retValue = 1;
       goto done;
   }

done:
   shChainDel(pChain);
   return retValue;
}

static int
testMisc(void)
{
   /*
    * Test the following methods: shChainSize(), shChainTypeGet(), and
    * shChainTypeSet()
    */
   CHAIN *pChain;
   TYPE  chainType;
   char  *p1 = "One", *p2 = "Two", *p3 = "Three";
   int   retValue;

   retValue = 0;
   pChain = shChainNew("UNKNOWN");

   if (shChainSize(pChain) != 0)  {
       fprintf(stderr, "testMisc: expecting a size of 0, got %d\n", 
               shChainSize(pChain));
       retValue = 1;
       goto done;
   }
  
   (void) shChainElementAddByPos(pChain, p1, "UNKNOWN",  TAIL, AFTER);
   (void) shChainElementAddByPos(pChain, p2, "UNKNOWN",  TAIL, AFTER);
   (void) shChainElementAddByPos(pChain, p3, "UNKNOWN",  TAIL, AFTER);

   if (shChainSize(pChain) != 3)  {
       fprintf(stderr, "testMisc: expecting a size of 3, got %d\n", 
               shChainSize(pChain));
       retValue = 1;
       goto done;
   }

   chainType = shChainTypeGet(pChain);
   if (chainType != shTypeGetFromName("UNKNOWN"))  {
       fprintf(stderr, "testMisc: expecting a TYPE of UNKNOWN, got %s\n", 
               shNameGetFromType(chainType));
       retValue = 1;
       goto done;
   }
 
   (void) shChainTypeSet(pChain, "GENERIC");
   chainType = shChainTypeGet(pChain);
   if (chainType != shTypeGetFromName("GENERIC"))  {
       fprintf(stderr, "testMisc: expecting a TYPE of GENERIC, got %s\n", 
               shNameGetFromType(chainType));
       retValue = 1;
       goto done;
   }

done:
   shChainDel(pChain);
   return retValue;
}

static int
testJoin(void)
{
   CHAIN *pSrc, *pTarget;
   char  *p1 = "Chain1.1", *p2 = "Chain1.2";
   char  *p3 = "Chain2.1", *p4 = "Chain2.2";
   char  *pData;
   RET_CODE rc;
   int   retValue;

   retValue = 0;
   pSrc     = shChainNew("UNKNOWN");
   pTarget  = shChainNew("UNKNOWN");

   (void) shChainElementAddByPos(pSrc, p1, "UNKNOWN", TAIL, AFTER);
   (void) shChainElementAddByPos(pSrc, p2, "UNKNOWN", TAIL, AFTER);

   (void) shChainElementAddByPos(pTarget, p3, "UNKNOWN", TAIL, AFTER);
   (void) shChainElementAddByPos(pTarget, p4, "UNKNOWN", TAIL, AFTER);

   rc = shChainJoin(pSrc, pTarget);
   if (rc != SH_SUCCESS)  {
       fprintf(stderr, "testJoin: shChainJoin() failed!\n");
       retValue = 1;
       goto done;
   }

   if (pSrc->nElements != 4)  {
       fprintf(stderr, 
            "testJoin: expected 4 elements on joined chain, got %d\n",
            pSrc->nElements);
       retValue = 1;
       goto done;
   }

   pData = shChainElementGetByPos(pSrc, HEAD);
   if (strncmp(pData, "Chain1.1", 8) != 0)  {
       fprintf(stderr, 
          "testJoin: First element should be \"Chain1.1\", but got \"%s\"\n",
          pData);
       retValue = 1;
       goto done;
   }

   pData = shChainElementGetByPos(pSrc, 1);
   if (strncmp(pData, "Chain1.2", 8) != 0)  {
       fprintf(stderr, 
          "testJoin: Second element should be \"Chain1.2\", but got \"%s\"\n",
          pData);
       retValue = 1;
       goto done;
   }

   pData = shChainElementGetByPos(pSrc, 2);
   if (strncmp(pData, "Chain2.1", 8) != 0)  {
       fprintf(stderr, 
          "testJoin: Third element should be \"Chain2.1\", but got \"%s\"\n",
          pData);
       retValue = 1;
       goto done;
   }

   pData = shChainElementGetByPos(pSrc, TAIL);
   if (strncmp(pData, "Chain2.2", 8) != 0)  {
       fprintf(stderr, 
          "testJoin: Fourth element should be \"Chain2.2\", but got \"%s\"\n",
          pData);
       retValue = 1;
       goto done;
   }

#if 0
   {
      CHAIN_ELEM *pElem;
      char *pMyData;

      pElem = pSrc->pFirst;

      for (; pElem != NULL; pElem = pElem->pNext)  {
           pMyData = (char *)pElem->pElement;
           printf("(%X)Data:   [%s]\n", pElem, pMyData);
           if (pElem->pPrev == NULL)
               printf("   Previous = NULL\n");
           else
               printf("   Previous = %X\n", pElem->pPrev);
           if (pElem->pNext == NULL)
               printf("   Next = NULL\n");
           else
               printf("   Next = %X\n", pElem->pNext);
      }
   } 
#endif
  
done:
   shChainDel(pSrc);
   return retValue;
}

static int
testCopy(void)
{
   CHAIN *pSrc, *pNewChain;
   char *p1 = "One", *p2 = "Two", *p3 = "Three";
   char *pSrcData, *pNewChainData;
   int  retValue;

   retValue = 0;
   pSrc = shChainNew("UNKNOWN");
   (void) shChainElementAddByPos(pSrc, p1, "UNKNOWN",  TAIL, AFTER);
   (void) shChainElementAddByPos(pSrc, p2, "UNKNOWN",  TAIL, AFTER);
   (void) shChainElementAddByPos(pSrc, p3, "UNKNOWN",  TAIL, AFTER);

   pNewChain = shChainCopy(pSrc);
   if (pNewChain != NULL)  {
       if (pSrc->type != pNewChain->type)  {
           fprintf(stderr, "testCopy: copied chain type does not match orignal"
                           " chain type\n");
           retValue = 1;
        }
       
       if (pSrc->nElements != pNewChain->nElements)  {
           fprintf(stderr, "testCopy: copied chain does not have the same"
                           " number of elements as the orignal");
           retValue = 1;
       }
       
       pSrcData = shChainElementGetByPos(pSrc, HEAD);
       pNewChainData = shChainElementGetByPos(pNewChain, HEAD);
       if (pSrcData != pNewChainData)  {
           fprintf(stderr, "testCopy: HEAD element on copied chain DOES NOT"
                           "match the original\n");
           retValue = 1;
       }

       pSrcData = shChainElementGetByPos(pSrc, 1);
       pNewChainData = shChainElementGetByPos(pNewChain, 1);
       if (pSrcData != pNewChainData)  {
           fprintf(stderr, "testCopy: 2nd element on copied chain DOES NOT"
                           "match the original\n");
           retValue = 1;
       }

       pSrcData = shChainElementGetByPos(pSrc, TAIL);
       pNewChainData = shChainElementGetByPos(pNewChain, TAIL);
       if (pSrcData != pNewChainData)  {
           fprintf(stderr, "testCopy: TAIL element on copied chain DOES NOT"
                           "match the original\n");
           retValue = 1;
       }
#if 0
   {
      CHAIN_ELEM *pElem;
      char *pMyData;

      pElem = pSrc->pFirst;

      printf("SOURCE CHAIN---->\n");
      for (; pElem != NULL; pElem = pElem->pNext)  {
           pMyData = (char *)pElem->pElement;
           printf("(%X)Data:   [%s]\n", pElem, pMyData);
           if (pElem->pPrev == NULL)
               printf("   Previous = NULL\n");
           else
               printf("   Previous = %X\n", pElem->pPrev);
           if (pElem->pNext == NULL)
               printf("   Next = NULL\n");
           else
               printf("   Next = %X\n", pElem->pNext);
      }

      pElem = pNewChain->pFirst;

      printf("COPIED CHAIN---->\n");
      for (; pElem != NULL; pElem = pElem->pNext)  {
           pMyData = (char *)pElem->pElement;
           printf("(%X)Data:   [%s]\n", pElem, pMyData);
           if (pElem->pPrev == NULL)
               printf("   Previous = NULL\n");
           else
               printf("   Previous = %X\n", pElem->pPrev);
           if (pElem->pNext == NULL)
               printf("   Next = NULL\n");
           else
               printf("   Next = %X\n", pElem->pNext);
      }
   } 
#endif
       shChainDel(pNewChain);
   }


   shChainDel(pSrc);
   return retValue;
}

static int 
testTypeDefine(void)
{
   CHAIN  *pChain;
   MASK   *pMask1, *pMask2;
   REGION *pRegion;
   TYPE   type;
   int    retValue;

   pChain   = shChainNew("GENERIC");
   pMask1   = shMaskNew("mask1", 10, 10);
   pMask2   = shMaskNew("mask2", 20, 20);
   pRegion  = shRegNew("reg1", 10, 10, TYPE_U8);
   retValue = 0;

   type = shChainTypeDefine(pChain);
   if (type != shTypeGetFromName("GENERIC"))  {
       fprintf(stderr, 
               "testTypeDefine: expected a TYPE of GENERIC, got %s\n",
               shNameGetFromType(type));
       retValue = 1;
       goto done;
   }

   (void) shChainElementAddByPos(pChain, pMask1, "MASK", TAIL, AFTER);
   (void) shChainElementAddByPos(pChain, pRegion, "REGION", TAIL, AFTER);
   (void) shChainElementAddByPos(pChain, pMask2, "MASK", TAIL, AFTER);

   type = shChainTypeDefine(pChain);
   if (type != shTypeGetFromName("GENERIC"))  {
       fprintf(stderr, 
               "testTypeDefine: expected a TYPE of GENERIC, got %s\n",
               shNameGetFromType(type));
       retValue = 1;
       goto done;
   }

   /*
    * Now remove the region. shChainTypeDefine() should return MASK
    */
   (void)shChainElementRemByPos(pChain, 1);

   type = shChainTypeDefine(pChain);
   if (type != shTypeGetFromName("MASK"))  {
       fprintf(stderr, 
               "testTypeDefine: expected a TYPE of MASK, got %s\n",
               shNameGetFromType(type));
       retValue = 1;
       goto done;
   }

done:
   shChainDel(pChain);
   shMaskDel(pMask1);
   shMaskDel(pMask2);
   shRegDel(pRegion);

   return retValue;
}

static int
testCursors(void)
{
   CHAIN    *pChain;
   CURSOR_T crsr;
   int      retVal;
   char     *p1 = "One",  *p2 = "Two",  *p3 = "Three", 
            *p4 = "Four", *pData;

   retVal = 0;

   /*
    * Test 1: create a chain with two elements. Create a cursor, set it to
    *         the head of the chain, and add a new element using the cursor.
    */
   pChain = shChainNew("GENERIC");
 
   (void) shChainElementAddByPos(pChain, p1, "UNKNOWN", TAIL, AFTER);
   (void) shChainElementAddByPos(pChain, p2, "UNKNOWN", TAIL, AFTER);

   if ( (crsr  = shChainCursorNew(pChain)) == INVALID_CURSOR ) {
       fprintf(stderr, "testCursor: shChainCursorNew failed\n");
       retVal = 1;
       goto done;
   }
   
   pData = shChainWalk(pChain, crsr, THIS);
   
   if (pData != p1)  {
       fprintf(stderr, "testCursors: (1)  Do not match\n");
       retVal = 1;
       goto done;
   } 

   if (shChainElementAddByCursor(pChain, p3, "UNKNOWN", crsr, AFTER) != 
       SH_SUCCESS)  {
       fprintf(stderr, "testCursors: 1. shChainElementAddByCursor() does NOT"
               "return SH_SUCCESS!\n");
       retVal = 1;
       goto done;
   }

   if (shChainSize(pChain) != 3)  {
       fprintf(stderr, "testCursors: Chain size should be three after "
               "adding a new element!\n");
       retVal = 1;
       goto done;
   }

   /*
    * Now add a new element from the tail.
    */
   (void) shChainCursorSet(pChain, crsr, TAIL);
   pData = shChainWalk(pChain, crsr, THIS);
   if (pData != p2)  {
       fprintf(stderr, "testCursors: (2)  Do not match\n");
       retVal = 1;
       goto done;
   }

   if (shChainElementAddByCursor(pChain, p4, "UNKNOWN", crsr, AFTER) != 
       SH_SUCCESS)  {
       fprintf(stderr, "testCursors: 2. shChainElementAddByCursor() does NOT"
               "return SH_SUCCESS!\n");
       retVal = 1;
       goto done;
   }

   if (shChainSize(pChain) != 4)  {
       fprintf(stderr, "testCursors: Chain size should be four after "
               "adding a new element!\n");
       retVal = 1;
       goto done;
   }

   /*
    * So our chain should now contain 4 elements: 
    *             One -> Three -> Two -> Four -> 0
    * Let's make sure.
    */
   pData = shChainElementGetByPos(pChain, HEAD);
   if (pData != p1)  {
       fprintf(stderr, "testCursors: expecting first element to be %s. "
               "Got %s\n", p1, pData);
       retVal = 1;
       goto done;
   }
   pData = shChainElementGetByPos(pChain, 1);
   if (pData != p3)  {
       fprintf(stderr, "testCursors: expecting second element to be %s. "
               "Got %s\n", p3, pData);
       retVal = 1;
       goto done;
   }
   pData = shChainElementGetByPos(pChain, 2);
   if (pData != p2)  {
       fprintf(stderr, "testCursors: expecting third element to be %s. "
               "Got %s\n", p2, pData);
       retVal = 1;
       goto done;
   }
   pData = shChainElementGetByPos(pChain, TAIL);
   if (pData != p4)  {
       fprintf(stderr, "testCursors: expecting forth element to be %s. "
               "Got %s\n", p4, pData);
       retVal = 1;
       goto done;
   }

   shChainCursorDel(pChain, crsr);
   shChainDel(pChain);

   /*
    * Now for the next test, let's create a chain with three elements, and
    * remove each of them in turn until the chain is empty. We'll use a 
    * cursor of course.
    */
   pChain = shChainNew("GENERIC");
   if ( (crsr   = shChainCursorNew(pChain)) == INVALID_CURSOR) {
       fprintf(stderr, "testCursors: shChainCursorNew failed \n");
       retVal = 1;
       goto done;
   }
   
   if (shChainElementAddByCursor(pChain, p1, "UNKNOWN", crsr, AFTER) != 
       SH_SUCCESS)
   {
       fprintf(stderr, "testCursors: error adding element 1 using cursor\n");
       retVal = 1;
       goto done;
   }
   if (shChainElementAddByCursor(pChain, p2, "UNKNOWN", crsr, AFTER) != 
       SH_SUCCESS)
   {
       fprintf(stderr, "testCursors: error adding element 2 using cursor\n");
       retVal = 1;
       goto done;
   }
   if (shChainElementAddByCursor(pChain, p3, "UNKNOWN", crsr, AFTER) != 
       SH_SUCCESS)
   {
       fprintf(stderr, "testCursors: error adding element 3 using cursor\n");
       retVal = 1;
       goto done;
   }

   if (shChainSize(pChain) != 3)  {
       fprintf(stderr, "testCursors: Test 2-chain count should be 3, not %d\n",
               shChainSize(pChain));
       retVal = 1;
       goto done;
   }

   if ((pData = shChainElementGetByPos(pChain, HEAD)) != p1)  {
       fprintf(stderr, "testCursors: 2 - expecting first element to be %s. "
               "Got %s\n", p1, pData);
       retVal = 1;
       goto done;
   }
   if ((pData = shChainElementGetByPos(pChain, 1)) != p2)  {
       fprintf(stderr, "testCursors: 2 - expecting second element to be %s. "
               "Got %s\n", p2, pData);
       retVal = 1;
       goto done;
   }
   if ((pData = shChainElementGetByPos(pChain, TAIL)) != p3)  {
       fprintf(stderr, "testCursors: 2 - expecting third element to be %s. "
               "Got %s\n", p3, pData);
       retVal = 1;
       goto done;
   }

   (void) shChainCursorSet(pChain, crsr, 1);
   pData = shChainWalk(pChain, crsr, NEXT);
   if (pData != p2)  {
       fprintf(stderr, "testCursors: 2 - error setting cursor!\n");
       retVal = 1;
       goto done;
   }

   if ((pData = shChainElementRemByCursor(pChain, crsr)) == NULL)  {
        fprintf(stderr, "testCursors: 2 - removing cursor element failed\n");
        retVal = 1;
        goto done;
   }

   if (shChainSize(pChain) != 2)  {
       fprintf(stderr, "testCursors: Test 2-chain count should be 2, not %d\n",
               shChainSize(pChain));
       retVal = 1;
       goto done;
   }

   pData = shChainWalk(pChain, crsr, NEXT);
   pData = shChainElementRemByCursor(pChain, crsr);
   if (pData == NULL)  {
       fprintf(stderr, "testCursors: 2 - error removing cursor element\n");
       retVal = 1;
       goto done;
   }

   if (pData != p3)  {
       fprintf(stderr, "testCursors: expected a removal of %s, not %s\n",
               p3, pData);
       retVal = 1;
       goto done;
   }

   if (shChainSize(pChain) != 1)  {
       fprintf(stderr, "testCursors: Test 2-chain count should be 1, not %d\n",
               shChainSize(pChain));
       retVal = 1;
       goto done;
   }

done:
   shChainCursorDel(pChain, crsr);
   shChainDel(pChain);
   return retVal;
}

static int 
testSort(void)
{
   PT    *pt1, *pt2, *pt3, *pt;
   CHAIN *pChain;
   int   retVal, increasing, i;
   CURSOR_T crsr;
   RET_CODE rc;

   crsr       = -1;
   retVal     = 0;
   increasing = 0;
   pChain     = shChainNew("PT");

   pt1    = shPtNew();  
   pt2    = shPtNew();
   pt3    = shPtNew();

   if (shPtDefine(pt1, 30, 30, 10) != SH_SUCCESS)  {
       fprintf(stdout, "testSort: cannot define pt1\n");
       retVal = 1;
       goto done;
   }

   if (shPtDefine(pt2, 20, 30, 10) != SH_SUCCESS)  {
       fprintf(stdout, "testSort: cannot define pt2\n");
       retVal = 1;
       goto done;
   }

   if (shPtDefine(pt3, 40, 30, 10) != SH_SUCCESS)  {
       fprintf(stdout, "testSort: cannot define pt3\n");
       retVal = 1;
       goto done;
   }

   if (shChainElementAddByPos(pChain, pt1, "PT", TAIL, AFTER) != SH_SUCCESS)  {
       fprintf(stdout, "testSort: cannot add pt1 to chain\n");
       retVal = 1;
       goto done;
   }
   
   if (shChainElementAddByPos(pChain, pt2, "PT", TAIL, AFTER) != SH_SUCCESS)  {
       fprintf(stdout, "testSort: cannot add pt2 to chain\n");
       retVal = 1;
       goto done;
   }

   if (shChainElementAddByPos(pChain, pt3, "PT", TAIL, AFTER) != SH_SUCCESS)  {
       fprintf(stdout, "testSort: cannot add pt3 to chain\n");
       retVal = 1;
       goto done;
   }

   rc = shChainSort(pChain, "row", increasing);
   if (rc != SH_SUCCESS)  {
       fprintf(stdout, "testSort: shChainSort did not return SH_SUCCESS\n");
       retVal = 1;
       goto done;
   }

   /*
    * Get a cursor and go thru the chain. The sorted chain should contain
    * pt3 -> pt1 -> pt2
    */
   if ( (crsr = shChainCursorNew(pChain)) == INVALID_CURSOR) {
       fprintf(stderr,"testSort: shChainCursorNew failed\n");
       retVal = 1;
       goto done;
   }
     
   (void) shChainCursorSet(pChain, crsr, HEAD);
   i = 0;
   while ((pt = (PT *) shChainWalk(pChain, crsr, NEXT)) != NULL)  {
           if (i == 0)  {
               if (pt != pt3)  {
                   fprintf(stdout, "testSort: expected pt3 in the HEAD "
                           "position; did not get it\n");
                   retVal = 1;
                   goto done;

               }
           } else if (i == 1)  {
               if (pt != pt1)  {
                   fprintf(stdout, "testSort: expected pt1 in the 2nd position"
                           "; did not get it - 1\n");
                   retVal = 1;
                   goto done;
               }
	   } else if (i == 2)  {
               if (pt != pt2)  {
                   fprintf(stdout, "testSort: expected pt2 in the TAIL "
                           "position; did not get it\n");
                   retVal = 1;
                   goto done;
               }

           }
           i++;
    }

   if (pt != (PT *) NULL)  {
       fprintf(stdout, "testSort: expected NULL; did not get it\n");
       retVal = 1;
       goto done;
   }
   
   /*
    * Ok, now let's test it with increasing = 1. Now we should expect the
    * chain to contain pt2 -> pt1 -> pt3
    * Let's delete the chain and create a new one
    */
   increasing = 1;
   shChainCursorDel(pChain, crsr);
   crsr = -1;
   shChainDel(pChain);
   pChain = shChainNew("PT");
   
   if (shChainElementAddByPos(pChain, pt1, "PT", TAIL, AFTER) != SH_SUCCESS)  {
       fprintf(stdout, "testSort: cannot add pt1 to chain\n");
       retVal = 1;
       goto done;
   }
   
   if (shChainElementAddByPos(pChain, pt2, "PT", TAIL, AFTER) != SH_SUCCESS)  {
       fprintf(stdout, "testSort: cannot add pt2 to chain\n");
       retVal = 1;
       goto done;
   }

   if (shChainElementAddByPos(pChain, pt3, "PT", TAIL, AFTER) != SH_SUCCESS)  {
       fprintf(stdout, "testSort: cannot add pt3 to chain\n");
       retVal = 1;
       goto done;
   }

   rc = shChainSort(pChain, "row", increasing);
   if (rc != SH_SUCCESS)  {
       fprintf(stdout, "testSort: shChainSort did not return SH_SUCCESS\n");
       retVal = 1;
       goto done;
   }

   if ( (crsr = shChainCursorNew(pChain)) == INVALID_CURSOR) {
       fprintf(stderr,"testSort: shChainCursorNew failed\n");
       retVal = 1;
       goto done;
   }
   pt   = (PT *)shChainWalk(pChain, crsr, NEXT);
   if (pt != pt2)  {
       fprintf(stdout, "testSort: expected pt2 in the HEAD position"
               "; did not get it\n");
       retVal = 1;
       goto done;
   }

   pt = (PT *)shChainWalk(pChain, crsr, NEXT);
   if (pt != pt1)  {
       fprintf(stdout, "testSort: expected pt1 in the 2nd position"
               "; did not get it - 2\n");
       retVal = 1;
       goto done;
   }

   pt = (PT *)shChainWalk(pChain, crsr, NEXT);
   if (pt != pt3)  {
       fprintf(stdout, "testSort: expected pt3 in the TAIL position"
               "; did not get it\n");
       retVal = 1;
       goto done;
   }

   pt = (PT *)shChainWalk(pChain, crsr, NEXT);
   if (pt != (PT *) NULL)  {
       fprintf(stdout, "testSort: expected NULL; did not get it\n");
       retVal = 1;
       goto done;
   }
   
done:
   if (crsr != -1)
       shChainCursorDel(pChain, crsr);    
   shChainDel(pChain);
   shPtDel(pt1);
   shPtDel(pt2);
   shPtDel(pt3);
   return retVal;
}

static int 
testWalk(void)
{
   char *p[10];
   char *pData;
   CHAIN *pChain;
   int   retValue;
   CURSOR_T crsr;
   int   i;

   pChain = shChainNew("UNKNOWN");
   retValue = 0;
   p[0] = "Zero"; p[1] = "One"; p[2] = "Two"; p[3] = "Three"; p[4] = "Four";

   /* 
    * Elements are added at the end of the chain
    */
   if ( (crsr = shChainCursorNew(pChain)) == INVALID_CURSOR) {
       fprintf(stderr,"testWalk: shChainCursorNew failed\n");
       retValue = 1;
       goto done;
   }
   shChainElementAddByCursor(pChain, p[0], "UNKNOWN", crsr, AFTER);
   shChainElementAddByCursor(pChain, p[1], "UNKNOWN", crsr, AFTER);
   shChainElementAddByCursor(pChain, p[2], "UNKNOWN", crsr, AFTER);
   shChainElementAddByCursor(pChain, p[3], "UNKNOWN", crsr, AFTER);
   shChainElementAddByCursor(pChain, p[4], "UNKNOWN", crsr, AFTER);

   (void) shChainCursorSet(pChain, crsr, HEAD);
   i = 0;
   while ((pData = shChainWalk(pChain, crsr, NEXT)) != NULL)  {
           if (pData != p[i])  {
               fprintf(stderr, "testWalk: NEXT - expected %s, got %s\n", 
                                                                 p[i], pData);
               retValue = 1;
               break;
           }
           i++;
   }

   if (retValue == 1)
       goto done;

   if (pData != NULL)  {
       fprintf(stderr, "testWalk: after first traversal, expected cursor to"
               " be NULL; instead it is %s\n", pData);
       retValue = 1;
       goto done;
   }

   pData = shChainWalk(pChain, crsr, NEXT);
   if (pData != p[0])  {
       fprintf(stderr, "testWalk: after first traversal, traversing again."
               " Expected\nfetched element to be %s, got %s\n", p[0], pData);
       retValue = 1;
       goto done;
   }

   /*
    * Now let's walk from the back of the chain
    */
   (void) shChainCursorSet(pChain, crsr, TAIL);
   pData = shChainWalk(pChain, crsr, THIS);
   if (pData != p[4])  {
       fprintf(stderr, "testWalk: after first traversal, TAIL element should"
               " be %s, got %s\n", p[4], pData);
       goto done;
   }

   i = shChainSize(pChain);
   i--;
   (void) shChainCursorSet(pChain, crsr, TAIL);
   while ((pData = shChainWalk(pChain, crsr, PREVIOUS)) != NULL)  {
           if (pData != p[i])  {
               fprintf(stderr, "testWalk: PREVIOUS - expected %s, got %s\n", 
                                                               p[i], pData);
               retValue = 1;
               break;
           }
           i--;
   }
 
   if (retValue == 1)
       goto done;

   if (pData != NULL)  {
       fprintf(stderr, "testWalk: after second traversal, expected cursor to"
               " be NULL; instead it is %s\n", pData);
       retValue = 1;
       goto done;
   }

   pData = shChainWalk(pChain, crsr, PREVIOUS);
   if (pData != p[4])  {
       fprintf(stderr, "testWalk: after second traversal, traversing again."
               " Expected\nfetched element to be %s, got %s\n", p[4], pData);
       goto done;
   }

   /*
    * Okay, now set the cursor to the 2nd element and traverse back and forth.
    */
   (void) shChainCursorSet(pChain, crsr, 1);  /* Chains are 0-indexed */
   pData = shChainWalk(pChain, crsr, THIS);

   if (pData != p[1])  {
       fprintf(stderr, "testWalk: third traversal. Expected %s, got %s\n",
	       p[1], pData);
       retValue = 1;
       goto done;
   }

   pData = shChainWalk(pChain, crsr, NEXT);
   if (pData != p[2])  {
       fprintf(stderr, "testWalk: fourth traversal. Expected %s, got %s\n",
               p[2], pData);
       retValue = 1;
       goto done;
   }

   pData = shChainWalk(pChain, crsr, PREVIOUS);
   if (pData != p[1])  {
       fprintf(stderr, "testWalk: fifth traversal. Expected %s, got %s\n",
               p[1], pData);
       retValue = 1;
       goto done;
   }
   pData = shChainWalk(pChain, crsr, PREVIOUS);
   if (pData != p[0])  {
       fprintf(stderr, "testWalk: sixth traversal. Expected %s, got %s\n",
               p[0], pData);
       retValue = 1;
       goto done;
   }
   pData = shChainWalk(pChain, crsr, PREVIOUS);  /* should return NULL */
   if (pData != NULL)  {
       fprintf(stderr, "testWalk: seventh traversal. Expected NULL, got %s\n",
               pData);
       retValue = 1;
       goto done;
   }

   /*
    * Now set the cursor to the first element, third element, and the last
    * element and exercise the THIS option of shChainWalk()
    */
   (void) shChainCursorSet(pChain, crsr, HEAD);
   pData = shChainWalk(pChain, crsr, THIS);
   if (pData != p[0])  {
       fprintf(stderr, "testWalk: Excercising THIS (1). Expected %s, got %s\n",
               p[0], pData);
       retValue = 1;
       goto done;
   }
   pData = shChainWalk(pChain, crsr, THIS);
   if (pData != p[0])  {
       fprintf(stderr, "testWalk: Excercising THIS (2). Expected %s, got %s\n",
               p[0], pData);
       retValue = 1;
       goto done;
   }

   (void) shChainCursorSet(pChain, crsr, 2);
   pData = shChainWalk(pChain, crsr, THIS);
   if (pData != p[2])  {
       fprintf(stderr, "testWalk: Excercising THIS (3). Expected %s, got %s\n",
               p[2], pData);
       retValue = 1;
       goto done;
   }
   pData = shChainWalk(pChain, crsr, THIS);
   if (pData != p[2])  {
       fprintf(stderr, "testWalk: Excercising THIS (4). Expected %s, got %s\n",
               p[2], pData);
       retValue = 1;
       goto done;
   }

   (void) shChainCursorSet(pChain, crsr, TAIL);
   pData = shChainWalk(pChain, crsr, THIS);
   if (pData != p[4])  {
       fprintf(stderr, "testWalk: Excercising THIS (5). Expected %s, got %s\n",
               p[4], pData);
       retValue = 1;
       goto done;
   }
   pData = shChainWalk(pChain, crsr, THIS);
   if (pData != p[4])  {
       fprintf(stderr, "testWalk: Excercising THIS (6). Expected %s, got %s\n",
               p[4], pData);
       retValue = 1;
       goto done;
   }

done:

   shChainCursorDel(pChain, crsr);
   shChainDel(pChain);

   return retValue;  
}

static int testWalk2(void)
{
   /*
    * Test walking a chain in a variety of ways (using a cursor) 
    */
   CURSOR_T crsr;
   CHAIN    *pChain;
   int      retVal, i;
   char     *pData;
   static char *elems[] =    { "Zero", "One", "Two", "Three", "Four" };
   static char *expected[] = { "One", "Two", "Zero", "Four", "Three" };

   retVal = 0;
   crsr = -1;

   pChain = shChainNew("UNKNOWN");
   if (shChainElementAddByPos(pChain, elems[0], "UNKNOWN", TAIL, AFTER) !=
       SH_SUCCESS)  {
       retVal = 1;
       goto done;
   }

   if (shChainElementAddByPos(pChain, elems[1], "UNKNOWN", HEAD, BEFORE) !=
       SH_SUCCESS)  {
       retVal = 1;
       goto done;
   }

   if (shChainElementAddByPos(pChain, elems[2], "UNKNOWN", HEAD, AFTER) !=
       SH_SUCCESS)  {
       retVal = 1;
       goto done;
   }

   if (shChainElementAddByPos(pChain, elems[4], "UNKNOWN", TAIL, AFTER) !=
       SH_SUCCESS)  {
       retVal = 1;
       goto done;
   }

   if (shChainElementAddByPos(pChain, elems[3], "UNKNOWN", TAIL, AFTER) !=
       SH_SUCCESS)  {
       retVal = 1;
       goto done;
   }

   /* ------ Traverse from the front ------- */

   i = 0;
   if ( (crsr = shChainCursorNew(pChain)) == INVALID_CURSOR) {
       fprintf(stderr,"testWalk2: shChainCursorNew failed\n");
       retVal = 1;
       goto done;
   }
   while ((pData = shChainWalk(pChain, crsr, NEXT)) != NULL)  {
           if (strcmp(pData, expected[i]) != 0)  {
               fprintf(stderr, "testWalk2: Walk 1 - expected %s, got %s\n",
                        expected[i], pData);
               retVal = 1;
               break;
           }
           i++;
   }

   if (retVal == 1)
       goto done;

   if (g_shChainErrno != SH_SUCCESS)  {
       fprintf(stderr, "testWalk2: expected g_shChainErrno to be SH_SUCCESS"
               " here (1)\n");
       retVal = 1;
       goto done;
   }

   (void) shChainCursorSet(pChain, crsr, 2);
   i = 2;
   while ((pData = shChainWalk(pChain, crsr, NEXT)) != NULL)  {
           if (strcmp(pData, expected[i]) != 0)  {
               fprintf(stderr, "testWalk2: Walk 2 - expected %s, got %s\n",
                        expected[i], pData);
               retVal = 1;
               break;
           }
           i++;
   }
 
   /* --------- Traverse from the back -------- */
   (void) shChainCursorSet(pChain, crsr, TAIL);
   i = shChainSize(pChain) - 1;
   while ((pData = shChainWalk(pChain, crsr, PREVIOUS)) != NULL)  {
           if (strcmp(pData, expected[i]) != 0)  {
               fprintf(stderr, "testWalk2: Walk 3 - expected %s, got %s\n",
                        expected[i], pData);
               retVal = 1;
               break;
           }
           i--;
   }

   if (retVal == 1)
       goto done;

   if (g_shChainErrno != SH_SUCCESS)  {
       fprintf(stderr, "testWalk2: expected g_shChainErrno to be SH_SUCCESS"
               " here (1)\n");
       retVal = 1;
       goto done;
   }

   (void) shChainCursorSet(pChain, crsr, 3);
   i = 3;
   while ((pData = shChainWalk(pChain, crsr, PREVIOUS)) != NULL)  {
           if (strcmp(pData, expected[i]) != 0)  {
               fprintf(stderr, "testWalk2: Walk 3 - expected %s, got %s\n",
                        expected[i], pData);
               retVal = 1;
               break;
           }
           i--;
   }

done:
   if (crsr != -1)
       shChainCursorDel(pChain, crsr);
   shChainDel(pChain);

   return retVal;
}

static int testElementRemByCursor(void)
{
   int retVal, i;
   CHAIN *pChain;
   CURSOR_T crsr[3];
   char  *pData;
   static char *elems[] =    { "Zero", "One", "Two", "Three", "Four" };

   retVal = 0;
   for (i = 0; i < 3; i++)
        crsr[i] = -1;

   pChain = shChainNew("UNKNOWN");
   if (shChainElementAddByPos(pChain, elems[0], "UNKNOWN", TAIL, AFTER) !=
       SH_SUCCESS)  {
       retVal = 1;
       goto done;
   }

   if (shChainElementAddByPos(pChain, elems[1], "UNKNOWN", HEAD, BEFORE) !=
       SH_SUCCESS)  {
       retVal = 1;
       goto done;
   }

   if (shChainElementAddByPos(pChain, elems[2], "UNKNOWN", HEAD, AFTER) !=
       SH_SUCCESS)  {
       retVal = 1;
       goto done;
   }

   if (shChainElementAddByPos(pChain, elems[4], "UNKNOWN", TAIL, AFTER) !=
       SH_SUCCESS)  {
       retVal = 1;
       goto done;
   }

   if (shChainElementAddByPos(pChain, elems[3], "UNKNOWN", TAIL, AFTER) !=
       SH_SUCCESS)  {
       retVal = 1;
       goto done;
   }

   /*
    * The chain created by the above commands looks like:
    *
    *    One -> Two -> Zero -> Four -> Three -> 0
    *
    * Get 3 cursors, and set them all to the 3rd element on the chain
    */
   for (i = 0; i < 3; i++)  {
       if ( (crsr[i] = shChainCursorNew(pChain)) == INVALID_CURSOR) {
	    fprintf(stderr,"testElementRemByCursor: shChainCursorNew failed\n");
	    retVal = 1;
	    break;
	}
        if (shChainCursorSet(pChain, crsr[i], 2) != SH_SUCCESS)  {
            fprintf(stderr, "testElementRemByCursor: shChainCursorSet() "
                   "returns an error code. Iteration = %d\n", i);
            retVal = 1;
            break;
        }
   }

   if (retVal == 1)
       goto done;

   /*
    * Now remove the element under crsr[0]. This should make other 2 cursors
    * now point to invalid elements.
    */
   pData = shChainElementRemByCursor(pChain, crsr[0]);
   if (strcmp(pData, "Zero") != 0)  {
       fprintf(stderr, "testElementRemByCursor: expected Zero, got %s\n", 
               pData);
       retVal = 1;
       goto done;
   }
   /*
    * Now if we try to retrieve THIS, we should get a NULL return value and
    * g_shChainErrno should be set to SH_CHAIN_INVALID_ELEM
    */
   pData = shChainWalk(pChain, crsr[0], THIS);

   if (pData != NULL && g_shChainErrno != SH_CHAIN_INVALID_ELEM)  {
       fprintf(stderr, "testElementRemByCursor: 0-expected a return value of "
               "NULL and g_shChainErrno to be SH_CHAIN_INVALID_ELEM\n");
       retVal = 1;
       goto done;
   }

   /*
    * If we use other cursors to retrieve THIS, we should also get a return
    * value of NULL and g_shChainErrno should be set to SH_CHAIN_INVALID_ELEM
    */
   for (i = 1; i < 3; i++)  {
        pData = shChainWalk(pChain, crsr[i], THIS);
        if (pData != NULL && g_shChainErrno != SH_CHAIN_INVALID_ELEM)  {
            fprintf(stderr, "testElementRemByCursor: %d-expected a return "
                    "value of NULL and g_shChainErrno to be "
                    "SH_CHAIN_INVALID_ELEM\n", i);
            retVal = 1;
            goto done;
        }
        i++;
   }
 
   /*
    * We should, however, be able to retrieve the NEXT and PREVIOUS elements
    */
   pData = shChainWalk(pChain, crsr[0], NEXT);
   if (strcmp(pData, "Four") != 0)  {
       retVal = 1;
       fprintf(stderr, "testElementRemByCursor: NEXT for crsr[0] should "
               "return a Four, not a %s\n", pData);
       goto done;
   }

   pData = shChainWalk(pChain, crsr[1], PREVIOUS);
   if (strcmp(pData, "Two") != 0)  {
       retVal = 1;
       fprintf(stderr, "testElementRemByCursor: PREVIOUS for crsr[1] should "
               "return a Two, not a %s\n", pData);
       goto done;
   }

   pData = shChainWalk(pChain, crsr[2], PREVIOUS);
   pData = shChainWalk(pChain, crsr[2], PREVIOUS);
   if (strcmp(pData, "One") != 0)  {
       retVal = 1;
       fprintf(stderr, "testElementRemByCursor: PREVIOUS for crsr[2] should "
               "return a One, not a %s\n", pData);
       goto done;
   }

   if (shChainWalk(pChain, crsr[2], PREVIOUS) != NULL)  {
       retVal = 1;
       fprintf(stderr, "testElementRemByCursor: PREVIOUS for crsr[2] should"
               " now return a NULL!\n");
       goto done;
   }

   pData = shChainWalk(pChain, crsr[2], PREVIOUS);
   if (strcmp(pData, "Three") != 0)  {
       retVal = 1;
       fprintf(stderr, "testElementRemByCursor: PREVIOUS for crsr[2] should"
               " now return a Three, not a %s\n", pData);
       goto done;
   }
   
done:
   for (i = 0; i < 3; i++)
        if (crsr[i] != -1)
            shChainCursorDel(pChain, crsr[i]);
   shChainDel(pChain);

   return retVal;
}
