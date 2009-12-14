/*
 * <AUTO>
 *
 * FILE: phAnalysis.c
 *
 * DESCRIPTION:
 * A set of routines for reading objects from catalogs and PHOTO output,
 * matching them up, and comparing the results
 *
 * </AUTO>
 */

#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "dervish.h"
#include "phObjc.h"
#include "phAnalysis.h"
#include "cpgplot.h"

#undef DEBUG

   /* 
    * this structure is used to make sorting a chain of OBJCs easier 
    */
struct s_sort {
   float rowc;
   int index;
};

   /*
    * and this "private" function allows "qsort" to do the sorting 
    */
typedef int (*PFI)(const void *, const void *);
static int
compare_rowc(struct s_sort *a, struct s_sort *b);

static int
remove_repeated_elements(CHAIN *listJ, CHAIN *listK, int color);

static void
remove_same_elements(CHAIN *listA, CHAIN *listB, int color);


/***************************************************************************
 * <AUTO EXTRACT>
 *
 * ROUTINE: phReadCatalog
 *
 * DESCRIPTION:
 * read data from a simulation catalog; create a chain of OBJCs
 * with OBJECT1s from the catalog.
 *
 * return: CHAIN * of OBJCs          if all goes well
 *         NULL                      if not
 *
 * </AUTO>
 */

CHAIN *
phReadCatalog(
   char *filename,             /* I: file containing catalog */
   float offsetx,              /* I: offset to add to each catalog row */
   float offsety               /* I: offset to add to each catalog col */
   )
{
 CHAIN *objclist;
 OBJC *objc;
 OBJECT1 *obj1;
 FILE *fp;
 long lnum=0; 
 char line[BUFSIZ];
 float id,xc,yc,f_c,r_b,r_d,ar_b,ar_d,tanth,f_in,aj,f_cat; 
 float color_p,color_b,color_d,cl,redshift;

 objclist=shChainNew("OBJC");

 if ((fp = fopen(filename, "r")) == NULL) {
   shError("phReadCatalog: can't open file %s"
	   ,filename);
   return NULL;
 }
 for(lnum=0;lnum<23;lnum++)
   {
     if(fgets(line,255,fp)==NULL)
       {
	 shError("phReadCatalog: error reading catalog file %s line %d",filename,lnum);
	 fclose(fp);
	 return NULL;
       }
   }
 
 while(fgets(line,BUFSIZ,fp) != NULL) {
     sscanf (line,"%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f ", 
        &id,&xc,&yc,&f_c,&r_b,&r_d,&ar_b,&ar_d,&tanth,&f_in,&aj,&f_cat, 
        &color_p,&color_b,&color_d,&cl,&redshift);

     if (f_in == 0)
	 continue;

     objc=phObjcNew(1);      /* num of color is 1 */
     obj1=phObject1New();   /* for initializing ? */

     objc->color[0]=obj1;

     (objc->color[0])->rowc=xc+offsetx;
     (objc->color[0])->colc=yc+offsety;
     (objc->color[0])->type=(OBJ_TYPE)(int)cl;
     (objc->color[0])->flags=(int)cl;
     (objc->color[0])->petroCounts=f_in;
     (objc->color[0])->psfCounts=f_c;

     shChainElementAddByPos(objclist,objc, "OBJC", TAIL, AFTER);
   }

 fclose(fp);
 return objclist;
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * ROUTINE: phObjcChainMSortC0Xc 
 *
 * DESCRIPTION:
 * sort all the elements of the given CHAIN of OBJCs in increasing
 * order by the color[c]->rowc value.  
 *
 * currently uses qsort, rather than slower bubble-sort of orig version.
 *
 * return: CHAIN * to sorted chain          if all goes well 
 *            (is same as given CHAIN *)
 *         NULL                             if not
 *
 * </AUTO>
 */

CHAIN *
phObjcChainMSortC0Xc(
   CHAIN *chain,                /* I/O: CHAIN to be sorted */
   int c                        /* I: color of OBJECT1s by which to sort */
   )
{
   int i, pos1, size;
   OBJC *objc1;
   struct s_sort *sort_array;
   CHAIN *temp_chain;

   temp_chain = shChainNew("OBJC");

   if (shChainTypeGet(chain) != shChainTypeGet(temp_chain)) {
      shError("phObjcChainMSortC0Xc: given chain not type OBJC");
      return(NULL);
   }

   size = shChainSize(chain);
   for (pos1 = 0; pos1 < size; pos1++) {
      objc1 = (OBJC *) shChainElementGetByPos(chain, pos1);
      if (objc1->color[c] == NULL) {
	 shError("phObjcChainMSortC0Xc: elem %d has NULL color %d", pos1, c);
	 return(NULL);
      }
   }

   /* 
    * to sort the CHAIN, we create an auxiliary array containing
    * the rowc values and index values for each element.  we sort
    * that array, create a new (temporary) CHAIN with elements in 
    * the proper order, then delete elements in the original CHAIN, 
    * copy elements
    * back into it from the temporary CHAIN, and finally delete the
    * temporary CHAIN.
    */
   if ((sort_array = (struct s_sort *) shMalloc(size*sizeof(struct s_sort))) ==
		     	    NULL) {
      shError("phObjcChainMSortC0Xc: can't allocate for sorting array");
      return(NULL);
   }

   for (pos1 = 0; pos1 < size; pos1++) {
      objc1 = (OBJC *) shChainElementGetByPos(chain, pos1);
      sort_array[pos1].rowc = objc1->color[c]->rowc;
      sort_array[pos1].index = pos1;
   }
   qsort(sort_array, size, sizeof(struct s_sort), (PFI) compare_rowc);

   /* create the temporary chain, with elems in sorted order ... */
   for (i = 0; i < size; i++) {
      pos1 = sort_array[i].index;

      if ((objc1 = (OBJC *) shChainElementGetByPos(chain, pos1)) == NULL) {
	 shError("phObjcChainMSortC0Xc: can't get element %d to copy it", pos1);
	 return(NULL);
      }
      if (shChainElementAddByPos(temp_chain, objc1, "OBJC", TAIL, AFTER) != SH_SUCCESS) {
	 shError("phObjcChainMSortC0Xc: can't insert element %d in temp_chain",
		  i);
	 return(NULL);
      }
   }
   shFree(sort_array);

   /*
    * now delete elements from the original chain to empty it (recall that
    * the OBJCs will not be deleted, just the chain structures).
    */
   for (i = 0; i < size; i++) {
      if ((objc1 = (OBJC *) shChainElementRemByPos(chain, HEAD)) == NULL) {
	 shError("phObjcChainMSortC0Xc: can't get elem %d to delete from orig chain", i + 1);
	 return(NULL);
      }
   }

   /* copy elems back in order to orig chain from temp_chain */
   for (i = 0; i < size; i++) {
      if ((objc1 = (OBJC *) shChainElementGetByPos(temp_chain, i)) == NULL) {
	 shError("phObjcChainMSortC0Xc: can't get element %d to copy it back", i + 1);
	 return(NULL);
      }
      if (shChainElementAddByPos(chain, objc1, "OBJC", TAIL, AFTER) != SH_SUCCESS) {
	 shError("phObjcChainMSortC0Xc: can't insert element %d back into chain",
		  i + 1);
	 return(NULL);
      }
   }

   /* finally, delete the temporary chain */
   shChainDel(temp_chain);

   return(chain);
}

static int
compare_rowc(struct s_sort *a, struct s_sort *b)
{
   if (a->rowc > b->rowc) {
      return(1);
   }
   if (a->rowc < b->rowc) {
      return(-1);
   }
   return(0);
}




/***************************************************************************
 * <AUTO EXTRACT>
 *
 * ROUTINE: phObjcDoubleChainMSortC0Xc 
 *
 * DESCRIPTION:
 * sort all the elements of the first CHAIN of OBJCs in increasing
 * order by the color[c]->rowc value.  Also, reorder the
 * elements of the _second_ CHAIN in exactly the same way, so that
 * the elements of both chains which matched BEFORE the sorting
 * will match again _after_ the sorting.
 *
 * currently uses qsort, rather than slower bubble-sort of orig version.
 *
 * return: CHAIN * to sorted chain          if all goes well 
 *            (is same as given CHAIN *)
 *         NULL                             if not
 *
 * </AUTO>
 */

CHAIN *
phObjcDoubleChainMSortC0Xc(
   CHAIN *chain1,               /* I/O: CHAIN to be sorted */
   CHAIN *chain2,               /* I/O: CHIAN to be re-ordered just as chain1 */
   int c                        /* I: color of OBJECT1s by which to sort */
   )
{
   int i, pos1, size;
   OBJC *objc1;
   struct s_sort *sort_array;
   CHAIN *temp_chain1, *temp_chain2;

   temp_chain1 = shChainNew("OBJC");
   temp_chain2 = shChainNew("OBJC");

   if (shChainTypeGet(chain1) != shChainTypeGet(temp_chain1)) {
      shError("phObjcChainMSortC0Xc: given chain1 not type OBJC");
      return(NULL);
   }
   if (shChainTypeGet(chain2) != shChainTypeGet(temp_chain2)) {
      shError("phObjcChainMSortC0Xc: given chain2 not type OBJC");
      return(NULL);
   }
   shAssert(shChainSize(chain1) == shChainSize(chain2));

   size = shChainSize(chain1);
   for (pos1 = 0; pos1 < size; pos1++) {
      objc1 = (OBJC *) shChainElementGetByPos(chain1, pos1);
      if (objc1->color[c] == NULL) {
	 shError("phObjcChainMSortC0Xc: elem %d has NULL color %d", pos1, c);
	 return(NULL);
      }
   }

   /* 
    * to sort the CHAIN, we create an auxiliary array containing
    * the rowc values and index values for each element.  we sort
    * that array, create a new (temporary) CHAIN with elements in 
    * the proper order, then delete elements in the original CHAIN, 
    * copy elements
    * back into it from the temporary CHAIN, (and copy elems from
    * chain2 into temp_chain2 in the same order),
    * and finally delete the temporary CHAINs.
    */
   if ((sort_array = (struct s_sort *) shMalloc(size*sizeof(struct s_sort))) ==
		     	    NULL) {
      shError("phObjcChainMSortC0Xc: can't allocate for sorting array");
      return(NULL);
   }

   for (pos1 = 0; pos1 < size; pos1++) {
      objc1 = (OBJC *) shChainElementGetByPos(chain1, pos1);
      sort_array[pos1].rowc = objc1->color[c]->rowc;
      sort_array[pos1].index = pos1;
   }
   qsort(sort_array, size, sizeof(struct s_sort), (PFI) compare_rowc);

   /* fill the temporary chains, with elems in sorted order ... */
   for (i = 0; i < size; i++) {
      pos1 = sort_array[i].index;

      if ((objc1 = (OBJC *) shChainElementGetByPos(chain1, pos1)) == NULL) {
	 shError("phObjcChainMSortC0Xc: can't get element %d to copy it", pos1);
	 return(NULL);
      }
      if (shChainElementAddByPos(temp_chain1, objc1, "OBJC", TAIL, AFTER) != 
		  SH_SUCCESS) {
	 shError("phObjcChainMSortC0Xc: can't insert element %d in temp_chain1",
		  i);
	 return(NULL);
      }
      if ((objc1 = (OBJC *) shChainElementGetByPos(chain2, pos1)) == NULL) {
	 shError("phObjcChainMSortC0Xc: can't get element %d to copy it", pos1);
	 return(NULL);
      }
      if (shChainElementAddByPos(temp_chain2, objc1, "OBJC", TAIL, AFTER) != 
		  SH_SUCCESS) {
	 shError("phObjcChainMSortC0Xc: can't insert element %d in temp_chain2",
		  i);
	 return(NULL);
      }
   }
   shFree(sort_array);

   /*
    * now delete elements from the original chains to empty them (recall that
    * the OBJCs will not be deleted, just the chain structures).
    */
   for (i = 0; i < size; i++) {
      if ((objc1 = (OBJC *) shChainElementRemByPos(chain1, HEAD)) == NULL) {
	 shError("phObjcChainMSortC0Xc: can't get elem %d to delete from orig chain1", i + 1);
	 return(NULL);
      }
      if ((objc1 = (OBJC *) shChainElementRemByPos(chain2, HEAD)) == NULL) {
	 shError("phObjcChainMSortC0Xc: can't get elem %d to delete from orig chain2", i + 1);
	 return(NULL);
      }
   }

   /* copy elems back in order to orig chains from temp_chains */
   for (i = 0; i < size; i++) {
      if ((objc1 = (OBJC *) shChainElementGetByPos(temp_chain1, i)) == NULL) {
	 shError("phObjcChainMSortC0Xc: can't get element %d to copy it back", 
		  i + 1);
	 return(NULL);
      }
      if (shChainElementAddByPos(chain1, objc1, "OBJC", TAIL, AFTER) != 
		  SH_SUCCESS) {
	 shError("phObjcChainMSortC0Xc: can't insert element %d back into chain1",
		  i + 1);
	 return(NULL);
      }
      if ((objc1 = (OBJC *) shChainElementGetByPos(temp_chain2, i)) == NULL) {
	 shError("phObjcChainMSortC0Xc: can't get element %d to copy it back", 
		  i + 1);
	 return(NULL);
      }
      if (shChainElementAddByPos(chain2, objc1, "OBJC", TAIL, AFTER) != 
		  SH_SUCCESS) {
	 shError("phObjcChainMSortC0Xc: can't insert element %d back into chain2",
		  i + 1);
	 return(NULL);
      }
   }

   /* finally, delete the temporary chains */
   shChainDel(temp_chain1);
   shChainDel(temp_chain2);

   return(chain1);
}









/***************************************************************************
 * <AUTO EXTRACT>
 *
 * ROUTINE: phObjcListMatchSlow
 *
 * DESCRIPTION:
 * given two CHAINs of OBJCs [A and B], find all matching elements,
 * where a match is coincidence of centers to within "radius" pixels.
 *
 * Use a slow, but sure, algorithm.
 *
 * We will match objects from A --> B.  It is possible to have several
 * As that match to the same B:
 *
 *           A1 -> B5   and A2 -> B5
 *
 * This function finds such multiple-match items and deletes all but
 * the closest of the matches.
 * 
 * place the elems of A that are matches into output chain J
 *                    B that are matches into output chain K
 *                    A that are not matches into output chain L
 *                    B that are not matches into output chain M
 *
 * return: SH_SUCCESS          if all goes well
 *         SH_GENERIC_ERROR    if not
 *
 * </AUTO>
 */


RET_CODE
phObjcListMatchSlow(
   CHAIN *listA,            /* I: first input chain of OBJCs to be matched */
   CHAIN *listB,            /* I: second "     "    "   "    "   " "    "   */
   float radius,            /* I: matching radius, in pixels */
   CHAIN **listJ,           /* O: all OBJCs in A that match */
   CHAIN **listK,           /* O: all OBJCs in B that match */
   CHAIN **listL,           /* O: chain of OBJCs from A that didn't match */
   CHAIN **listM            /* O: chain of OBJCs from B that didn't match */
   )
{
  OBJC *objA,*objB;
  float Ax,Ay,Bx,By;
  float dist,limit;
  int color;
  int posA, posB;
  float deltax, deltay;
  float Axm, Axp, Aym, Ayp;

#ifdef DEBUG
   printf("entering phObjcListMatchSlow ");
#endif

  /* examine only zero-th color objects, for now */
  color = 0;     

  *listJ=shChainNew("OBJC");
  *listK=shChainNew("OBJC");
  *listL=shChainNew("OBJC");
  *listM=shChainNew("OBJC");

  /* Copy ListB into ListM */

  if ((*listM = shChainCopy(listB)) == NULL) {
       shError("phObjcListMatchSlow: can't copy list B");
       return SH_GENERIC_ERROR;
  }

  /* Copy ListA onto ListL */

  if ((*listL = shChainCopy(listA)) == NULL) {
       shError("phObjcListMatchSlow: can't copy list A");
       return SH_GENERIC_ERROR;
  }

  phObjcChainMSortC0Xc(*listL, color);
  phObjcChainMSortC0Xc(*listM, color);


  limit=radius*radius;


  /*
   * the first step is to go slowly through listA, checking against
   * every object in listB.  If there's a match, we copy the matching
   * elements onto lists J and K, respectively.  We do NOT check
   * yet to see if there are multiply-matched elements.
   */
#ifdef DEBUG
   printf(" size of listA is %d, listB is %d\n", shChainSize(listA),
	 shChainSize(listB));
   printf(" about to step through listA looking for matches\n");
#endif
   for (posA = 0; posA < shChainSize(listA); posA++) {
      
      shAssert((objA = shChainElementGetByPos(listA, posA)) != NULL);
      if (objA->color[color] == NULL) {
	 continue;
      }
      Ax = objA->color[color]->rowc;
      Ay = objA->color[color]->colc;

      Axm = Ax - radius;
      Axp = Ax + radius;
      Aym = Ay - radius;
      Ayp = Ay + radius;

      for (posB = 0; posB < shChainSize(listB); posB++) {

         shAssert((objB = shChainElementGetByPos(listB, posB)) != NULL);
         if (objB->color[color] == NULL) {
   	    continue;
         }
         Bx = objB->color[color]->rowc;
         By = objB->color[color]->colc;

	 /* check quickly to see if we can avoid a multiply */
	 if ((Bx < Axm) || (Bx > Axp) || (By < Aym) || (By > Ayp)) {
	    continue;
	 }

	 /* okay, we actually have to calculate a distance here. */
	 deltax = Ax - Bx;
	 deltay = Ay - By;
	 dist = deltax*deltax + deltay*deltay;
	 if (dist < limit) {

	    /*
	     * we have a match (at least, a possible match).  So, copy
	     * objA onto listJ and objB onto listK.  But do NOT remove
	     * these objects from listA and listB!  We may end up
	     * matching another objA to the same objB later on, and
	     * we will continue trying to match this same objA to other
	     * objBs.
	     */
	    shChainElementAddByPos(*listJ, objA, "OBJC", TAIL, AFTER);
	    shChainElementAddByPos(*listK, objB, "OBJC", TAIL, AFTER);

	 }
      }
   }

#ifdef DEBUG
   printf(" done with stepping through listA \n");
   printf(" listJ has %d, listK has %d \n", shChainSize(*listJ),
	 shChainSize(*listK));
#endif
   
#ifdef DEBUG
   /* for debugging only */
   for (posA = 0; posA < shChainSize(*listJ); posA++) {
      objA = (OBJC *) shChainElementGetByPos(*listJ, posA);
      objB = (OBJC *) shChainElementGetByPos(*listK, posA);
      if ((objA->color[color] == NULL) || (objB->color[color] == NULL)) {
	 printf(" %4d   NULL \n", posA);
      }
      printf(" %4d  J: (%8.2f, %8.2f)  K: (%8.2f, %8.2f) \n",
	    posA, objA->color[color]->rowc, objA->color[color]->colc,
	    objB->color[color]->rowc, objB->color[color]->colc);
   }
#endif


   /*
    * at this point, all _possible_ matches have been placed into 
    * corresponding elements of listJ and listK.  Now, we go through
    * listJ to find elements which appear more than once.  We'll
    * resolve them by throwing out all but the closest match.
    */

   /* 
    * first, sort listJ by the rowc values.  This allows us to find 
    * repeated elements easily.  Re-order listK in exactly the same
    * was as listJ, so matching elements still match. 
    */
#ifdef DEBUG
   printf(" sorting listJ \n");
#endif
   if (phObjcDoubleChainMSortC0Xc(*listJ, *listK, color) == NULL) {
       shError("phObjcListMatchSlow: can't sort list J");
       return(SH_GENERIC_ERROR);
   }

   /* remove repeated elements from listJ, keeping the closest matches */
#ifdef DEBUG
   printf(" before remove_repeated_elements, listJ has %d\n",
	 shChainSize(*listJ));
#endif
   if (remove_repeated_elements(*listJ, *listK, color) != SH_SUCCESS) {
       shError("phObjcListMatchSlow: remove_repeated fails for list J");
       return(SH_GENERIC_ERROR);
   }
#ifdef DEBUG
   printf(" after remove_repeated_elements, listJ has %d\n",
	 shChainSize(*listJ));
#endif

   /* 
    * next, do the same for listK: sort it, then find and remove any
    * repeated elements, keeping only the closest matches.
    */
#ifdef DEBUG
   printf(" sorting listK \n");
#endif
   if (phObjcDoubleChainMSortC0Xc(*listK, *listJ, color) == NULL) {
       shError("phObjcListMatchSlow: can't sort list K");
       return(SH_GENERIC_ERROR);
   }
#ifdef DEBUG
   printf(" before remove_repeated_elements, listK has %d\n",
	 shChainSize(*listK));
#endif
   if (remove_repeated_elements(*listK, *listJ, color) != SH_SUCCESS) {
       shError("phObjcListMatchSlow: remove_repeated fails for list K");
       return(SH_GENERIC_ERROR);
   }
#ifdef DEBUG
   printf(" after remove_repeated_elements, listK has %d\n",
	 shChainSize(*listK));
#endif

   /*
    * finally, we have unique set of closest-pair matching elements
    * in listJ and listK.  Now we can remove any element from listL
    * which appears in listJ, and remove any element from listM
    * which appears in listK.  First, we'll sort listL and listM,
    * to make the comparisons easier.
    */
#ifdef DEBUG
   printf(" sorting listL \n");
#endif
   if (phObjcChainMSortC0Xc(*listL, color) == NULL) {
       shError("phObjcListMatchSlow: can't sort list L");
       return(SH_GENERIC_ERROR);
   }
#ifdef DEBUG
   printf(" sorting listM \n");
#endif
   if (phObjcChainMSortC0Xc(*listM, color) == NULL) {
       shError("phObjcListMatchSlow: can't sort list M");
       return(SH_GENERIC_ERROR);
   }

   /* now remove elements from listL which appear in listJ */
#ifdef DEBUG
   printf(" before remove_same_elements, listL has %d\n",
	 shChainSize(*listL));
#endif
   remove_same_elements(*listJ, *listL, color);
#ifdef DEBUG
   printf(" after remove_same_elements, listL has %d\n",
	 shChainSize(*listL));
#endif
   /* and remove elements from listM which appear in listK */
#ifdef DEBUG
   printf(" before remove_same_elements, listM has %d\n",
	 shChainSize(*listM));
#endif
   remove_same_elements(*listK, *listM, color);
#ifdef DEBUG
   printf(" after remove_same_elements, listM has %d\n",
	 shChainSize(*listM));
#endif

#ifdef DEBUG
   /* for debugging only */
   for (posA = 0; posA < shChainSize(*listJ); posA++) {
      objA = (OBJC *) shChainElementGetByPos(*listJ, posA);
      objB = (OBJC *) shChainElementGetByPos(*listK, posA);
      if ((objA->color[color] == NULL) || (objB->color[color] == NULL)) {
	 printf(" %4d   NULL \n", posA);
      }
      printf(" %4d  J: (%8.2f, %8.2f)  K: (%8.2f, %8.2f) \n",
	    posA, objA->color[color]->rowc, objA->color[color]->colc,
	    objB->color[color]->rowc, objB->color[color]->colc);
   }
#endif


   return(SH_SUCCESS);
}


/*********************************************************************
 *
 * step through the first chain argument, listJ, checking for successive 
 * elements which are the same. for each such pair, calculate the 
 * distance between the matching elements of listJ and listK objects.  
 * Throw the less-close pair off the two chains.
 *
 * The two chains must have the same number of elements, and must
 * ALReady have been sorted by some means.
 *
 */

static int
remove_repeated_elements
   (
   CHAIN *listJ,          /* I/O: look in this chain for repeats */
   CHAIN *listK,          /* I/O: do to this chain what we do to listJ */
   int color              /* I:   use this color's OBJECT1s  */
   )
{
   int posJ, posK;
   OBJECT1 *lastJ1, *lastK1, *objJ1, *objK1;
   OBJC *objJ, *objK;
   float thisdist, lastdist;

   shAssert(shChainTypeGet(listJ) == shTypeGetFromName("OBJC"));
   shAssert(shChainTypeGet(listK) == shTypeGetFromName("OBJC"));
   shAssert(shChainSize(listJ) == shChainSize(listK));

   posJ = 0;
   posK = 0;

   lastJ1 = NULL;
   lastK1 = NULL;
   while (posJ < shChainSize(listJ)) {

      if (((objJ = (OBJC *) shChainElementGetByPos(listJ, posJ)) == NULL) ||
          ((objK = (OBJC *) shChainElementGetByPos(listK, posK)) == NULL)) {
         shError("remove_repeated_elements: missing elem in chain J or K");
         return(SH_GENERIC_ERROR);
      }
      objJ1 = objJ->color[color];
      objK1 = objK->color[color];
      if ((objJ1 != NULL) && (objK1 != NULL)) {

	 if (lastJ1 == NULL) {
	    lastJ1 = objJ1;
	    lastK1 = objK1;
	 }
	 else if (objJ1->id == lastJ1->id) {

	    /* there is a repeated element.  We must find the closer match */
	    thisdist = (objJ1->rowc - objK1->rowc)*(objJ1->rowc - objK1->rowc) +
	               (objJ1->colc - objK1->colc)*(objJ1->colc - objK1->colc);
	    lastdist = (lastJ1->rowc - lastK1->rowc)*(lastJ1->rowc - lastK1->rowc) +
	               (lastJ1->colc - lastK1->colc)*(lastJ1->colc - lastK1->colc);
	    if (thisdist < lastdist) {
	       
	       /* remove the "last" item from listJ and listK */
	       shChainElementRemByPos(listJ, posJ - 1);
	       shChainElementRemByPos(listK, posK - 1);
	       lastJ1 = objJ1;
	       lastK1 = objK1;
	    }
	    else {

	       /* remove the current item from listJ and listK */
	       shChainElementRemByPos(listJ, posJ);
	       shChainElementRemByPos(listK, posK);
	    }
	    posJ--;
	    posK--;
	 }
	 else {

	    /* no repeated element.  Prepare for next step forward */
	    lastJ1 = objJ1;
	    lastK1 = objK1;
	 }
      }
      posJ++;
      posK++;
   }
   return(SH_SUCCESS);
}
	 

/*********************************************************************
 *
 * given two chains of OBJCs which have been sorted by their 
 * some color's rowc values, try to find OBJCs which appear 
 * on both lists.  Remove any such OBJCs from the second chain.
 *
 */

static void
remove_same_elements
   (
   CHAIN *listA,          /* I: look for elems which match those in listB */
   CHAIN *listB,          /* I/O: remove elems which are also in listA */
   int color              /* I: use the color'th OBJECT1 as reference */
   )
{
   int posA, posB, posB_top;
   OBJECT1 *objA1, *objB1;
   OBJC *objA, *objB;

   shAssert(shChainTypeGet(listA) == shTypeGetFromName("OBJC"));
   shAssert(shChainTypeGet(listB) == shTypeGetFromName("OBJC"));

   posA = 0;
   posB_top = 0;

   while (posA < shChainSize(listA)) {

      if ((objA = (OBJC *) shChainElementGetByPos(listA, posA)) == NULL) {
         shError("remove_same_elements: missing elem in chain A");
         return;
      }
      objA1 = objA->color[color];
      if (objA1 == NULL) {
	 posA++;
	 continue;
      }

      for (posB = posB_top; posB < shChainSize(listB); posB++) {
         if ((objB = (OBJC *) shChainElementGetByPos(listB, posB)) == NULL) {
            shError("remove_same_elements: missing elem in chain B");
            return;
         }
         objB1 = objB->color[color];
	 if (objB1 == NULL) {
	    continue;
	 }
	 if (objA1->id == objB1->id) {
	    shChainElementRemByPos(listB, posB);
	    if (--posB_top < 0) {
	       posB_top = 0;
	    }
	 }
	 else {
	    if (objB1->rowc < objA1->rowc) {
	       posB_top = posB + 1;
	    }
	 }
      }
      posA++;
   }
}
