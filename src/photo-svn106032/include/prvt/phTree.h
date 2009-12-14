#if !defined(PHTREE_H)
#define PHTREE_H

#include "phObjc.h"

	/* structures and functions used in decision trees */
#include "phTreenode.h"

	/* 
	 * If you change this enum, you must be sure to update the
	 * "type_str" array in file "treeDump.c" to reflect the
	 * changes.
	 */
typedef enum {
	TREE_UNK = 0,
	TREE_STAR,
	TREE_GAL,
	TREE_QSO,
	TREE_CR,
	TREE_DEFECT,
	TREE_NTYPE
} TREE_TYPE;

	/* 
	 * this structure points to an "object structure", 
	 * but contains its own copy of 
	 * the object's statistics in the array "param" (of which there
	 * are "nparam" elements, and the name of the i'th parameter
	 * in the "structure" is "param_name[i]").  
	 * Its classification starts off as
	 * TREE_UNK, but should get set to some other value after it
	 * has gone through a tree.
	 */

typedef struct treeobj {
	void  *obj;
	char *name;
	int nparam;
	float *param;
        char **param_name;
	TREE_TYPE type;
	struct treeobj *prev;
	struct treeobj *next;
} TREEOBJ;

	/* some #defines */
#define NODE_ALL    1       /* applies to any tree node */
#define NODE_TERM   2       /* applies to terminal nodes only - no children */
#define TREE_BIG    1.0e7   /* a large number */

        /*
         * this is used to keep the number of parameters per OBJECT1
         * which we use for classification.  It is modified by
         *      p_phTreeSetParams()
         * and read via
         *      p_phTreeGetParams()
         */
#define TREE_MAX_PARAMS       20     /* there can be no more than this many */
                                     /*   parameters used for classification */
#define TREE_MAX_STRLEN      100     /* the name of each field can be at most */
                                     /*   this many characters */



int p_phNameNode(TREENODE *node);
TREEOBJ *p_phNewTreeObj(void);
void p_phDelTreeObj(TREEOBJ *tobj);
TREENODE *p_phCreateTree(CHAIN *tobj);
int phDumpTreeObjList(TREEOBJ *tobj, char *file);
TREEOBJ *phReadTreeObjList(char *file);
void p_phPrintTreeNode(TREENODE *node, float alpha);
int p_phNodeLabel(TREENODE *node, int *label, float *cost);
int p_phSplitNode(TREENODE *node);
TREENODE *p_phFindNodeForTobj(TREENODE *root, TREEOBJ *tobj);
int p_phTreePrune(TREENODE *node);
int p_phTreeEvaluate(TREENODE *root, CHAIN *tobjchain);

	/* the user should probably not call most of these */
int p_phObjcSetType(void *objc, const SCHEMA *schema_ptr, float class);
void p_phTreeSetType(char *name);
void p_phTreeSetParams(int nparam, char **names);
char *p_phTreeGetTypeName(void);
int p_phTreeGetParamNumber(void);
char *p_phTreeGetParamName(int n);
int p_phSplitNodeRecurs(TREENODE *node, int max_depth);
float p_phTreePrior(TREE_TYPE type);
int p_phTreeInit(CHAIN *tobjchain);
int p_phTreeNobjType(TREE_TYPE type);
int p_phTreeNobjTotal(void);
int p_phTreert(TREENODE *node, float *rt);
int p_phTreePjt(TREENODE *node, TREE_TYPE type, float *prob);
int p_phTreePt(TREENODE *node, float *prob);
int p_phTreePjgivent(TREENODE *node, TREE_TYPE type, float *prob);
float p_phTreeImpurity(TREENODE *node);
int p_phTreeNumNodes(TREENODE *root, int node_type);
int p_phTreeNumNodesAtDepth(TREENODE *root, int node_type, int depth);
TREENODE *p_phTreeRoot(TREENODE *node);
int p_phTreeDepth(TREENODE *node);
float p_phTreeNodeCost(TREENODE *node, float alpha);
float p_phTreeCost(TREENODE *node, float alpha);
float p_phTreeNodeStrength(TREENODE *node, float alpha);
void p_phTreePrintStrength(TREENODE *node, float alpha);
int p_phFindCost(int class[TREE_NTYPE], int *label, float *cost);
TREENODE *p_phTreeWeakNode(TREENODE *root);
float **p_phTreeGetCosts(void);
void p_phPrintTreeAlpha(TREENODE *node, float alpha);

#endif
