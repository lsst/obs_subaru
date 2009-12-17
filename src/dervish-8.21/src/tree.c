/*
 * These routines maintain a binary tree.
 * The tree used is a Weight-balanced tree (BB[1-sqrt(2)/2]).
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "region.h"
#include "shCUtils.h"
#include "shTclTree.h"

/*
 *  shTreeKeyInsert	public	Insert a key into a tree.
 *  shTreeMkeyInsert	public	Like above, but allow multiple copies of key.
 *  shTreeKeyDel	public	Delete key from tree
 *  shTreeListKeys	public	Traverse tree, print out the keys
 *  shTreeValFind	public  Get the value corresponding to a tree
 *  shTreeNodeFind	public	Get the node corresponding to a key
 *  shTreeDel		public	Delete an entire tree.
 *  shTreeTraverse      public  Traverse a tree, calling the user's function at each node.
 *
*/

#define WEIGHT(N) ((N == NULL) ? 1 : (N)->weight)	/* weight of a node */

static TNODE *lrot(TNODE *node);
static TNODE *makenode(IPTR key, IPTR val);
static TNODE *rrot(TNODE *node);

/***************************************************************/
/*
 * add a key to the tree; return the new root of the tree.
 */
TNODE *
shTreeKeyInsert(TNODE *node, IPTR key, IPTR val)
{
   if(node == NULL)                     /* make new node */
      return(makenode(key,val));

   if(key == node->key) {		/* redefinition */
      shFatal("shTreeKeyInsert: Key %ld is already in tree",key);
   } else if(key < node->key) {
      node->left = shTreeKeyInsert(node->left,key,val); /* in left branch */
   } else {
      node->right = shTreeKeyInsert(node->right,key,val); /* to right */
   }

   node->weight = WEIGHT(node->left) + WEIGHT(node->right);
   
   if(99*WEIGHT(node->left) > 70*WEIGHT(node)) {
      if(99*WEIGHT(node->left->left) > 41*WEIGHT(node->left)) {
      	 node = rrot(node);
      } else {
      	 node->left = lrot(node->left);
      	 node = rrot(node);
      }
   } else if(99*WEIGHT(node->left) < 29*WEIGHT(node)) {
      if(99*WEIGHT(node->left) < 58*WEIGHT(node->right)) {
      	 node = lrot(node);
      } else {
      	 node->right = rrot(node->right);
      	 node = lrot(node);
      }
   }
   return(node);
}

/*****************************************************************************/
/*
 * like insert_key, but allow multiple copies of an key.
 * Return the new root of the tree.
 */
TNODE *
shTreeMkeyInsert(TNODE *node, IPTR key, IPTR val)
{
   if(node == NULL)                     /* make new node */
      return(makenode(key,val));

   if(key >= node->key) {		/* redefinition or to right */
      node->right = shTreeMkeyInsert(node->right,key,val);
   } else {				/* in left branch */
      node->left = shTreeMkeyInsert(node->left,key,val);
   }

   node->weight = WEIGHT(node->left) + WEIGHT(node->right);
   
   if(99*WEIGHT(node->left) > 70*WEIGHT(node)) {
      if(99*WEIGHT(node->left->left) > 41*WEIGHT(node->left)) {
      	 node = rrot(node);
      } else {
      	 node->left = lrot(node->left);
      	 node = rrot(node);
      }
   } else if(99*WEIGHT(node->left) < 29*WEIGHT(node)) {
      if(99*WEIGHT(node->left) < 58*WEIGHT(node->right)) {
      	 node = lrot(node);
      } else {
      	 node->right = rrot(node->right);
      	 node = lrot(node);
      }
   }
   return(node);
}

/***************************************************************/
/*
 * make new node
 */
static TNODE *
makenode(IPTR key, IPTR val)
{
   TNODE *newnode;

   if((newnode = (TNODE *)malloc(sizeof(TNODE))) == NULL) {
      shFatal("makenode: malloc returns NULL");
   }

   newnode->key = key;
   newnode->val = val;
   newnode->left = NULL;
   newnode->right = NULL;
   newnode->weight = 2;		/* 1 for each sub-tree (here NULL) */

   return(newnode);
}

/***************************************************************/
/*
 * Two functions, lrot() and rrot() to do left and right rotations
 *
 * lrot :	  N		    R
 *		 / \		   / \
 *		L   R	  -->	  N   r
 *		   / \		 / \
 *		  l   r		L   l
 */
static TNODE *
lrot(TNODE *node)
{
   TNODE *temp;

   temp = node;
   node = node->right;
   temp->right = node->left;
   node->left = temp;
/*
 * Now update the weights
 */
   temp->weight = (WEIGHT(temp->left) + WEIGHT(temp->right));
   node->weight = (WEIGHT(node->left) + WEIGHT(node->right));

   return(node);
}

static TNODE *
rrot(TNODE *node)
{
   TNODE *temp;

   temp = node;
   node = node->left;
   temp->left = node->right;
   node->right = temp;
/*
 * Now update the weights
 */
   temp->weight = (WEIGHT(temp->left) + WEIGHT(temp->right));
   node->weight = (WEIGHT(node->left) + WEIGHT(node->right));

   return(node);
}

/************************************************
 * Delete a node named by KEY from the tree, return the
 * new root of the tree.
*/

TNODE *
shTreeKeyDel(TNODE *node,IPTR key)
{
   if(node == NULL) return(NULL);
   
   if(key < node->key) {
      node->left = shTreeKeyDel(node->left,key);
   } else if(key > node->key) {
      node->right = shTreeKeyDel(node->right,key);
   } else {				/* found it! */
      if(node->left == NULL) {		/* that is easy */
	 TNODE *temp = node->right;
	 free((char *)node);
      	 node = temp;
      } else if(node->right == NULL) {	/* so is that */
	 TNODE *temp = node->left;
	 free((char *)node);
      	 node = temp;
      } else {				/* but now we must do some work */
         if(WEIGHT(node->left) > WEIGHT(node->right)) {
            if(WEIGHT(node->left->left) < WEIGHT(node->left->right)) {
               node->left = lrot(node->left);
            }
            node = rrot(node);
            node->right = shTreeKeyDel(node->right,key);
         } else {
            if(WEIGHT(node->right->left) > WEIGHT(node->right->right)) {
               node->right = rrot(node->right);
            }
            node = lrot(node);
            node->left = shTreeKeyDel(node->left,key);
         }
      }    	 
   }
   if(node != NULL) {
      node->weight = WEIGHT(node->left) + WEIGHT(node->right);
   }

   return(node);
}

/*******************************************/

void
shTreeListKeys(TNODE *node)
{
   if(node == NULL) return;

   shTreeListKeys(node->left);
   printf("0x%-15lx  %ld\n",node->key,node->val);
   shTreeListKeys(node->right);
}

/**************************************************************/
/*
 * Get the value corresponding to an key
 * Return a 0 if not found. God help you if your value is null as well.
 */
IPTR
shTreeValFind(TNODE *node, IPTR key)
{
   if(node == NULL) {                   /* unknown key */
      return(~0L);
   }

   if(key == node->key) {		/* redefinition */
      return(node->val);
   } else if(key < node->key) {
      return(shTreeValFind(node->left,key));
   } else {
      return(shTreeValFind(node->right,key));
   }
}

/**************************************************************/
/*
 * Get the node corresponding to an key
 */
TNODE *
shTreeNodeFind(TNODE *node, IPTR key)
{
   if(node == NULL) {                   /* unknown key */
      return(NULL);
   }

   if(key == node->key) {		/* redefinition */
      return(node);
   } else if(key < node->key) {
      return(shTreeNodeFind(node->left,key));
   } else {
      return(shTreeNodeFind(node->right,key));
   }
}

/*****************************************************************************/
/*
 * Kill an entire tree structure
 * Always return NULL.
 */
TNODE *
shTreeDel(TNODE *node)
{
   if(node == NULL) return(NULL);

   shTreeDel(node->left);
   shTreeDel(node->right);
   free((char *)node);
   return(NULL);
}


/*
 * Traverse a tree, calling the user's function at each node.
 */
void
shTreeTraverse(TNODE *node, void (*funct)(const IPTR key, const IPTR val, void *userData), void * userData)
{
   if(node == NULL) return;

   shTreeTraverse(node->left, funct, userData);
   funct(node->key, node->val, userData);
   shTreeTraverse(node->right, funct, userData);
}





























