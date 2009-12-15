/*
 * <INTRO>
 *
 * Support for OBJCs, OBJC_IOs, ATLAS_IMAGES, and TEST_INFOs
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dervish.h"
#include "phPeaks.h"
#include "phObjc.h"

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * create a new OBJC structure and return a pointer to it.
 *
 * return: pointer to new structure
 */
OBJC *
phObjcNew(int ncolor)                   /* number of OBJECT1s in OBJC */
{
   OBJC *objc = (OBJC *) shMalloc(sizeof(OBJC));
   int i;
   static int id = 0;

   *(int *) &(objc->id) = id++;

   *(int *) &(objc->ncolor) = ncolor;

   objc->color = shMalloc(ncolor * sizeof(OBJECT1 *));
   for (i = 0; i < ncolor; i++) {
      objc->color[i] = NULL;
   }

   objc->type = OBJ_UNK;
   objc->prob_psf = VALUE_IS_BAD;
   objc->aimage = phAtlasImageNew(ncolor);
   objc->test = NULL;
   objc->rowc = objc->rowcErr = VALUE_IS_BAD;
   objc->colc = objc->colcErr = VALUE_IS_BAD;
   objc->rowv = objc->rowvErr = VALUE_IS_BAD;
   objc->colv = objc->colvErr = VALUE_IS_BAD;
   objc->catID = 0;
   objc->flags = objc->flags2 = objc->flags3 = 0x0;
   objc->peaks = NULL;

   objc->nchild = 0;
   objc->parent = objc->sibbs = objc->children = NULL;

   return (objc);
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * delete the OBJECT1s and ATLAS_IMAGE associated with the given OBJC,
 * then delete the OBJC itself.
 */

void
phObjcDel(OBJC *objc,			/* OBJC to delete */
	  int deep)			/* should we destroy siblings
					   and children? */
{
   int i;

   if(objc == NULL) return;
   
   if(deep) {
      phObjcDel(objc->children, 1);	/* n.b. will recurse */
      phObjcDel(objc->sibbs, 1);	/*      down lists */

      objc->children = objc->sibbs = NULL;      
   }

   if(p_shMemRefCntrGet(objc) > 0) {	/* still referenced somewhere */
      shMemRefCntrDecr(objc);
      return;
   }

   phObjcDel(objc->parent, 0);		/* may be NULL */
   
   if (objc->color != NULL) {		/* now kill objc itself */
      for (i = 0; i < objc->ncolor; i++) {
	 phObject1Del(objc->color[i]);
      }
      shFree(objc->color);
   }
   phAtlasImageDel(objc->aimage,1);
   phPeaksDel(objc->peaks);

   shFree((char *) objc);
}

/*****************************************************************************/
/*
 * make a copy of an OBJC.
 *
 * If deep is true, make copies of the sub-components (OBJECT1s, ATLAS_IMAGES)
 *
 * If move_ai is true, move (not copy) the ATLAS_IMAGE
 */
OBJC *
phObjcNewFromObjc(const OBJC *objc,	/* OBJC to copy */
		  int deep,		/* copy object1s, ATLAS_IMAGEs etc? */
		  int copy_ai)		/* copy atlas images? */
{
   int c;
   int id;				/* id number of new OBJECT1s */
   OBJC *nobjc;
		  
   if(objc == NULL) {
      return(NULL);
   }

   nobjc = phObjcNew(objc->ncolor);
   if(deep) {				/* we'll make a copy of objc's below */
      phAtlasImageDel(nobjc->aimage, 1);
   }

   nobjc->type = objc->type;
   nobjc->prob_psf = objc->prob_psf;

   nobjc->rowc = objc->rowc;
   nobjc->rowcErr = objc->rowcErr;
   nobjc->colc = objc->colc;
   nobjc->colcErr = objc->colcErr;

   nobjc->rowv = objc->rowv;
   nobjc->rowvErr = objc->rowvErr;
   nobjc->colv = objc->colv;
   nobjc->colvErr = objc->colvErr;

   nobjc->catID = objc->catID;
   nobjc->flags = objc->flags;
   nobjc->flags2 = objc->flags2;

   if(deep) {
      nobjc->peaks = objc->peaks;
      if(objc->peaks != NULL) {
	 p_shMemRefCntrIncr(objc->peaks);
      }

      nobjc->test = objc->test;
      if(objc->test != NULL) {
	 p_shMemRefCntrIncr(objc->test);
      }

      if(copy_ai) {
	 nobjc->aimage = phAtlasImageCopy(objc->aimage, 1);
      } else {
	 nobjc->aimage = objc->aimage;
	 if(objc->aimage != NULL) {
	    p_shMemRefCntrIncr(objc->aimage);
	 }
      }

      for(c = 0; c < objc->ncolor; c++) {
	 if(objc->color[c] != NULL) {
	    nobjc->color[c] = phObject1New();
/*
 * we'd like to simply say
 *   *nobjc->color[c] = *objc->color[c]
 * but OBJECT1->id is declared const; hence the memcpy
 */
	    id = nobjc->color[c]->id;
	    memcpy(nobjc->color[c], objc->color[c], sizeof(OBJECT1));
	    *((int *)&nobjc->color[c]->id) = id; /* cast away const */

	    if(objc->color[c]->mask != NULL) {
		objc->color[c]->mask->refcntr++;
	    }
	    nobjc->color[c]->region = NULL;

	    if(objc->color[c]->peaks != NULL) {
	       p_shMemRefCntrIncr(objc->color[c]->peaks);
	    }

	    if(objc->color[c]->model != NULL) {
	       p_shMemRefCntrIncr(objc->color[c]->model);
	    }
	 }
      }
   }

   return(nobjc);
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * delete the REGIONs associated with the given OBJC and its family
 */
void
phRegDelFromObjc(OBJC *objc,	/* OBJC whose REGIONs are to be deleted */
		 int deep)	/* should we destroy siblings
				   and children? */
{
   int c;
   OBJECT1 *obj1;

   if(objc == NULL) return;
   
   if(deep) {
      phRegDelFromObjc(objc->children, 1); /* n.b. will recurse */
      phRegDelFromObjc(objc->sibbs, 1); /*         down lists */
   }

   for(c = 0;c < objc->ncolor;c++) {
      if((obj1 = objc->color[c]) != NULL && obj1->region != NULL) {
	 SPANMASK *regmask = (SPANMASK *)obj1->region->mask;
	 if(p_shMemRefCntrGet(regmask) > 0) { /* still referenced somewhere */
	    p_shMemRefCntrDecr(regmask);
	 } else {
	    shAssert(regmask != NULL && regmask->cookie == SPAN_COOKIE);
	    phSpanmaskDel(regmask); obj1->region->mask = NULL;
	 }
	 if(obj1->region != NULL &&
	    p_shMemRefCntrGet(obj1->region) > 0) { /* still referenced */
	    p_shMemRefCntrDecr(obj1->region);
	 } else {
	    shRegDel(obj1->region); obj1->region = NULL;
	 }
      }
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * traverse the maze of an OBJC and all of its descendents, returning each
 * in turn, starting with the OBJC itself, then its children, then its
 * siblings. This is repeated recursively, so grandchildren will be listed
 * after children, but before siblings --- which in turn come before nieces
 *
 * Usage:
 *	(void)phObjcDescendentNext(objc0);     (returns objc0)
 *	objc = phObjcDescendentNext(NULL);
 *	objc = phObjcDescendentNext(NULL);
 * until eventually a NULL is returned
 */
OBJC *
phObjcDescendentNext(const OBJC *objc)
{
   typedef struct OBJC_STACK {
       struct OBJC_STACK *next;		/* next OBJC_STACK with a family of OBJCs */
       OBJC *objc;			/* root of a family */
   } OBJC_STACK;			/* stack to save pending families */

   static OBJC_STACK *stack = NULL;	/* stack of families of OBJCs waiting to be processed */
   static const OBJC *next = NULL;	/* the objc we'll return next time */
   static const OBJC *prev = NULL;	/* the previous objc; useful in gdb */
   static const OBJC *pprev = NULL;	/* the previous-previous objc; useful in gdb */

   prev = next;
   if(objc != NULL) {			/* reset the saved stack */
       while (stack != NULL) {
	   OBJC_STACK *tmp = stack->next;
	   shFree(stack);
	   stack = tmp;
       }
   } else {				/* return next family member */
      if (next != NULL) {
	  objc = next;
      } else {				/* no-one's left in this family */
	  if(stack == NULL) {		/* and there are no more families */
	      return(NULL);
	  }

	  objc = stack->objc;		/* there might be a whole family here */
	  {				/* pop stack */
	      OBJC_STACK *tmp = stack->next;
	      shFree(stack);
	      stack = tmp;
	  }
	  shAssert (objc != NULL);
      }
   }

   if(objc->children != NULL) {
       if(objc->sibbs != NULL) {	/* push sibbs onto the stack */
	   OBJC_STACK *tmp = stack;
	   stack = shMalloc(sizeof(OBJC_STACK));
	   stack->next = tmp;
	   stack->objc = objc->sibbs;
       }

      next = objc->children;
   } else if(objc->sibbs != NULL) {
      next = objc->sibbs;
   } else {
       next = NULL;
   }

   pprev = prev;
   prev = next;

   return((OBJC *)objc);
}
	 
/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Destroy a chain of OBJCs and all the elements on it
 *
 * return: nothing
 */

void
phObjcChainDel(CHAIN *chain,		/* chain of OBJCs to destroy */
	       int deep)		/* wreak deep destruction? */
{
   OBJC *objc;
   int nel;

   shAssert(chain != NULL && shChainTypeGet(chain) == shTypeGetFromName("OBJC"));

   nel = chain->nElements;
   
   while(--nel >= 0) {
      objc = shChainElementRemByPos(chain, nel);
      phObjcDel(objc, deep);
   }

   shChainDel(chain);
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * print out information on most of the fields of each
 * color OBJECT1 for each OBJC in the given list; do all color[0]
 * objects first, then all color[1] objects, etc.  Send the
 * output to stdout, if fname = "", or to files of form
 * "fname".0 (color[0]), "fname".1 (color[1]), etc. 
 *
 * return: nothing
 */

void
phObjcListPrint(
   CHAIN *objclist,            /* I: chain of OBJCs to print */
   char *fname                 /* I: where to place output.  if "", then */
                               /*     send to stdout; otherwise, place */
                               /*     color[0] info in "fname.0", etc. */
   )
{
   int i, ncolor;
   char file[50];
   FILE *fp;
   OBJECT1 *obj;
   OBJC *objc;
   CURSOR_T crsr;

   shAssert(shChainTypeGet(objclist) == shTypeGetFromName("OBJC"));

   /* figure out the maximum number of colors any OBJC has */
   ncolor = 0;
   crsr = shChainCursorNew(objclist);
   while ((objc = (OBJC *) shChainWalk(objclist, crsr, NEXT)) != NULL) {
      if (objc->ncolor > ncolor)
	 ncolor = objc->ncolor;
   }

   for (i = 0; i < ncolor; i++) {
      if (strcmp(fname, "") == 0) {
	 fp = stdout;
      }
      else {
	 sprintf(file, "%s.%d", fname, i);
	 if ((fp = fopen(file, "w")) == NULL) {
	    shError("phObjcListPrint: can't open file %s for writing",
	       file);
	    return;
	 }
      }
      while ((objc = (OBJC *) shChainWalk(objclist, crsr, NEXT)) != NULL) {
	 if ((objc->color != NULL) && ((obj = objc->color[i]) != NULL)) {
	    phObject1PrintTerse(obj, fp);
	 }
      }
      if (fp != stdout)
	 fclose(fp);
   }

   shChainCursorDel(objclist, crsr);
}


/***************************************************************************
 * <AUTO EXTRACT>
 *
 * print out a pretty list of information on some of the fields of each
 * color OBJECT1 for the given OBJC; do color[0]
 * object first, then color[1] object, etc.  Send the
 * output to stdout, if fname = "", or to files of form
 * "fname".0 (color[0]), "fname".1 (color[1]), etc. 
 *
 * return: nothing
 */

void
phObjcPrintPretty(
   OBJC *objc,                 /* I: OBJC whose info we print */
   char *fname                 /* I: where to place output.  if "", then */
                               /*     send to stdout; otherwise, place */
                               /*     color[0] info in "fname.0", etc. */
   )
{
   int i, ncolor;
   char file[50];
   FILE *fp;
   OBJECT1 *obj;

   if (objc == NULL) {
      return;
   }
   ncolor = objc->ncolor;

   for (i = 0; i < ncolor; i++) {
      if (strcmp(fname, "") == 0) {
	 fp = stdout;
      }
      else {
	 sprintf(file, "%s.%d", fname, i);
	 if ((fp = fopen(file, "w")) == NULL) {
	    shError("phObjcPrintPretty: can't open file %s for writing",
	       file);
	    return;
	 }
      }
      if ((objc->color == NULL) || (objc->color[i] == NULL)) {
	    continue;
      }
      obj = objc->color[i];
      phObject1PrintPretty(obj, fp);
      if (fp != stdout)
	 fclose(fp);
   }
}

/************************************************************************
 * <AUTO EXTRACT>
 *
 * compare two OBJCs; return 0 if they are the same, or non-zero if not.
 * use phObject1Compare to do the comparison on corresponding OBJECT1s.
 *
 * return: 0                   if the two are the same
 *         1                   if not
 */
int
phObjcCompare(
	      const OBJC *objc1,                /* first OBJC to compare */
	      const OBJC *objc2                 /* second OBJC to compare */
	      )
{
   int i;

   if (objc1->ncolor != objc2->ncolor) {
      return(1);
   }
   for (i = 0; i < objc1->ncolor; i++) {
      if (phObject1Compare(objc1->color[i], objc2->color[i]) != 0) {
	 return(1);
      }
   }
   return(0);
}


/************************************************************************
 * <AUTO EXTRACT>
 *
 * ROUTINE: phObjcClosest
 *
 * DESCRIPTION:
 * given an (x,y) pair, compare the (rowc, colc) of all OBJECT1s in the 
 * given "OBJC list" to it; find the closest OBJC overall, and return it.
 * here, "OBJC list" means an OBJC and all members of the linked
 * list that start with its 'next' field.
 *
 * return: OBJC * to closest OBJC  
 *
 * </AUTO>
 */

OBJC *
phObjcClosest(
   CHAIN *chain,              /* I: search these OBJCs for closest one */
   float xc,                  /* I: desired row coordinate */
   float yc,                  /* I: desired column coordinate */
   int color                  /* I: examine only OBJECT1s in this color */
   )
{
   int col;
   float dr, dc, dist, min, num;
   OBJECT1 *obj;
   OBJC *objc, *best;
   CURSOR_T crsr;

   min = 1e6;
   crsr = shChainCursorNew(chain);
   best = NULL;	/* Note: will return NULL if no objc closer than sqrt(min)*/
   while ((objc = (OBJC *) shChainWalk(chain, crsr, NEXT)) != NULL) {
      dist = 0.0;
      num = 0.0;
#if 0
      for (col = 0; col < objc->ncolor; col++) {
#endif
	  col=color;
	 if ((obj = objc->color[col]) == NULL) {
	    continue;
	 }
	 if ((obj = objc->color[col]) != NULL) {	 
	     dr = obj->rowc - xc;
	     dc = obj->colc - yc;
	     dist += (dr*dr + dc*dc);
	     num++;
	 }
#if 0
      }
#endif
      if (num == 0.0) {
	  continue;
      }
      dist /= num;
      if (dist < min) {
	 min = dist;
	 best = objc;
      }
   }
   shChainCursorDel(chain, crsr);

   return(best);
}
