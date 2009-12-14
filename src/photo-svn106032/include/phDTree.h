#if !defined(PHDTREE_H)
#define PHDTREE_H

#include "dervish.h"
#include "prvt/phTree.h"
#include "phObjc.h"
#include "phCatobj.h"

#define TREE_ERRLEVEL 2

int phDumpTree(TREENODE *node, char *file);
TREENODE *phReadTree(char *file);
void phPrintTree(TREENODE *node);

   /* returns a CHAIN of CATOBJ structures */
CHAIN *phSimCatalogToChain(char *filename);

int phClassifyObjcChain(TREENODE *tree_root, CHAIN *objcchain);
int phClassifyObjc(TREENODE *tree_root, void *objc, const  SCHEMA *schema_ptr);


int phSimTypesAssign(CHAIN *sim, CHAIN *objclist);
CHAIN *phObjcChainToTreeobjChain(const CHAIN *objclist);
TREEOBJ *phObjcToTreeobj(void *obj, const SCHEMA *schema_ptr);
TREENODE *phTreeobjToTree(CHAIN *treeobj);
int phTreeGrow(TREENODE *root, int max_depth);
int phTreePrune(TREENODE *root, int *num_tree);
int phTreeDel(TREENODE *root);

#endif
