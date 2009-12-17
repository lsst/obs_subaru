#ifndef SHTCLTREE_H
#define SHTCLTREE_H
typedef struct tnode {
   IPTR key;
   IPTR val;
   int weight;				/* Weight of subtree */
   struct tnode *left,			/* Subtrees */
		*right;
} TNODE;

#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

TNODE *shTreeKeyDel(TNODE *root, IPTR key);
TNODE *shTreeNodeFind(TNODE *root, IPTR key);
IPTR shTreeValFind(TNODE *root, IPTR key);
TNODE *shTreeKeyInsert(TNODE *root, IPTR key, IPTR val);
TNODE *shTreeDel(TNODE *node);
void shTreeListKeys(TNODE *root);
TNODE *shTreeMkeyInsert(TNODE *root, IPTR key, IPTR val);
void shTreeTraverse (TNODE *root, void (*funct)(const IPTR key, const IPTR val, void *userData), void *userData);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */
#endif  /* ifndef SHTCLTREE_H */
