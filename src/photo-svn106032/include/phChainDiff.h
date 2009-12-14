#if !defined(CHAINDIFF_H)
#define CHAINDIFF_H 1
/*
 * diff two lists
 *
 * Describe the difference between two lists. For external use, only
 * the .ptr element of the union is valid.
 */
typedef struct edit {
   struct edit *memchain;		/* link to previous node allocated */
   struct edit *link;			/* link to previous edit command */
   enum { DELETE, INSERT, IDENTICAL } op; /* editing commands:
					     DELETE: delete from list1
					     INSERT: insert into list1
					     IDENTICAL: the lists are the same
					   */
   union {
      int n;				/* index of element */
      void *ptr;			/* pointer to element */
   } line1,line2;			/* elements in lists 1 and 2 */
} EDIT;


EDIT *
shChainDiff(
	   CHAIN *l1, CHAIN *l2,		/* the two lists */
	   int (*func)(void *a, void *b), /* the comparison function */
	   int max_d			/* bound on size of edit script */
	   );
void shChainDiffFree(void);		/* free the diff state */

void shChainDiffPrint(EDIT *diffs);	/* print the result of a shChainDiff */
RET_CODE shChainDiffAsList(Tcl_Interp *interp, EDIT *diffs);
					/* return a list of operations needed
					   to make two lists identical */

#endif
