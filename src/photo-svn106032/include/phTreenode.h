#if !defined(PHTREENODE_H)
#define PHTREENODE_H
#include "dervish.h"
#include "shChain.h"

	/*
	 * the structure of each node of a decision tree, used for
	 * classifying objects.
	 */


typedef struct ph_treenode {
	char *name;               /* root's name is ".", others build on it */
	int label;                /* type given to all object in this node */
	                                /* or -1 if node not terminal */
	float impurity;           /* how impure is this node? */
	                                /* or -1.0 if node not terminal */
	float pt;                 /* prob that any case falls into this node */
                                    /* see Breiman, p. 28, eq. 2.3 */
	float rt;                 /* resub estimate of misclassification */
                                    /* see Breiman, p. 35, def. 2.13 */
	float Rt;                 /* similar - see Breiman, p. 35, def 2.13 */
	int nobj;                 /* number of objs in this node */
	CHAIN *objlist;           /* chain of objs belonging to this node */
	int chosen;               /* parameter used for decision */
	float value;              /* go left if chosen param < this value */
	struct ph_treenode *parent, *left, *right;
} TREENODE;


   /* 
    * these are the values of the "del_objs" argument in phTreenodeDel
    * 
    *    if PH_TREENODE_DELOBJ,     all TREEOBJ structures in the CHAIN
    *                                   belonging to the given node are
    *                                   deleted before the CHAIN itself.
    *                                   Appropriate if deleting an entire tree.
    *
    *    if PH_TREENODE_NODELOBJ,   only the CHAIN is deleted; the TREEOBJ
    *                                   structures are left as-is.
    *                                   Appropriate if pruning a true branch.
    */

#define PH_TREENODE_NODELOBJ 0
#define PH_TREENODE_DELOBJ   1

TREENODE *phTreenodeNew(void);
void phTreenodeDel(TREENODE *node, int del_objs);

#endif

