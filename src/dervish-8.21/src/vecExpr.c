/*
 * This is code to parse expressions, (including a symbol table for variables)
 *
 * 	Robert Lupton July 1990 (rhl@astro.princeton.edu)
 *
 * Converted to generate a parse tree, July 1994.
 *
 * The function calls to access the parser are:
 * 	float shVectorStringEval();	evaluate a string as an expression
 *
 * The grammar for expressions is:
 *
 * expr : logical_expr			# toplevel expression
 *	| expr ? expr : expr
 *	| expr IF expr
 *	| expr CONCAT expr
 *
 * logical_expr : rel_expr		# logical expression; no short circuit
 *	| rel_expr && rel_expr
 *	| rel_expr || rel_expr
 *
 * rel_expr : add_expr			# relational expression
 *	| add_expr == add_expr
 *	| add_expr != add_expr
 *	| add_expr < add_expr		see note at end of this comment
 *	| add_expr <= add_expr		 "   "   "   "  "    "   "   "
 *	| add_expr > add_expr
 *	| add_expr >= add_expr
 *	| add_expr, add_expr		implicit do; not really a relational op
 *	| add_expr, add_expr, unary_expr
 *
 * add_expr : mult_expr			# additive/bitwise expression
 *	| mult_expr + mult_expr
 *	| mult_expr - mult_expr
 *	| mult_expr & mult_expr
 *	| mult_expr | mult_expr
 *
 * mult_expr : pow_expr * pow_expr	# multiplicative expression
 *	| pow_expr / pow_expr
 *
 * pow_expr : unary_expr		# evaluate a power
 *	| unary_expr ^ unary_expr
 *
 * unary_expr : primary			# unary expression
 *	| + expr
 *	| - expr
 *	| ! expr
 *
 * primary : WORD			# primary
 *	| number
 *      | { number_list }
 *      | WORD < add_expr >		see note at end of this comment
 *      | WORD [ add_expr ]
 *	| ( expr )
 *	| ( INT ) expr
 *	| ABS ( expr )
 *	| ASIN ( expr )
 *	| ACOS ( expr )
 *	| ATAN ( expr )
 *	| ATAN2 ( expr : expr )
 *	| COS ( expr )
 *	| DIMEN ( expr )
 *	| EXP ( expr )
 *	| LG ( expr )
 *	| LN ( expr )
 *	| MAP ( expr1 : index: expr2 ) replace expr2 elements given by index with expr1
 *	| NTILE ( expr1 : expr2 )	 number of entries in expr1 less than expr2
 *	| RAND ( expr1 : expr2 )	 uniform random #'s between expr1 and expr2
 *	| RANDN ( expr1 : expr2 )	 normal random #'s of mean expr1 and sigma expr2
 *	| SELECT ( expr : expr )
 *	| SIN ( expr )
 *	| SORT ( expr )			sort expr
 *	| SORT ( expr : )		return sorted indices
 *	| SORT ( expr1 : expr2 )	sort expr1 into expr2's order
 *	| SQRT ( expr )
 *	| SUM ( expr )
 *	| TAN ( expr )
 *
 * Note that precedence increases _down_ this table
 *
 * Auxiliary rules:
 *
 * number_list : number
 *	| number_list number
 *
 * number : FLOAT
 *	| INT
 *	| - FLOAT
 *	| -INT
 *
 * Because TCL makes typing [] hard, by default this expression evaluator
 * permits you to use <> instead. But this makes the grammar ambiguous; this
 * is resolved by demanding that < used as a subscript operator must have NO
 * space between the array name and the <, and that < used as a relational
 * operator MUST have space. If you compile with NO_ANGLES defined, you cannot
 * use <> for arrays, and white space is not significant.
 */

/********************************************************************/
/*
 * Begin Code
 */
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#ifdef NOTCL   
#include "shCAssert.h"
#else                       
#include "dervish.h"
#endif                    

#include "shChain.h"
#include "shCVecChain.h"
#include "shCSchema.h"
#include "shCVecExpr.h"

#include "shTclHandle.h"
#include "shCGarbage.h"
#include "shCErrStack.h"
#include "shCGarbage.h"


#define NAMESIZE 40	         	/* length of a name */
/*
 * Token definitions
 */
typedef enum {
   UNKNOWN_NODE = 256,			/* node of unknown type */
   ERROR,				/* an error has occurred */
   FLOAT,				/* node is a float */
   INT,					/*         an int */
   WORD,				/*         a word */
   EQ, 					/* == */
   NE,					/* != */
   GE,					/* >= */
   LE,					/* <= */
   NOT,					/* ! */
   AND,					/* && */
   OR,					/* || */
   CAST_INT,				/* cast to int */
   UMINUS,				/* unary minus */
   DO,					/* an implicit do loop */
   IF,
   ABS,
   ACOS,
   ASIN,
   ATAN,
   ATAN2,
   CONCAT,				/* concatenate two vectors */
   COS,
   DIMEN,
   EXP,
   LG,
   LN,
   MAP,                 /* replace elements */
   NTILE,				/* n-tiles of a vector*/
   RAND,				/* uniform random */
   RANDN,				/* normal  random */
   SELECT,				/* select elements of a vector */
   SIN,
   SORT,
   SQRT,
   SUM,
   TAN
} NODE_TYPE;
/*
 * Parse Tree
 */
typedef struct node {
   struct node *left, *right;		/* pointers to children */
   NODE_TYPE type;			/* type of node; value or operator */
   union {
      VECTOR *f;
      long l;
      char c[NAMESIZE];
   } value;				/* value if a value node */
} NODE;

static NODE *newNode(int type);		/* make a new node */
static void delNode(NODE *node);	/* delete a node */
static VECTOR *eval_tree(NODE *tree);	/* evaluate a parse tree */
static void free_tree(NODE *node);	/* free a parse tree */
/*
 * Parser
 */
static NODE *add_expr(void);		/* see grammar in header */
static NODE *rel_expr(void);		/* for definitions of these */
static NODE *expr(void);
static NODE *logical_expr(void);
static NODE *primary(void);
static NODE *mult_expr(void);
static NODE *pow_expr(void);
static NODE *unary_expr(void);
static NODE *error(void);		/* report an error */

static NODE *cnode = NULL;		/* the NODE for the current value */
static int token;			/* current token */
/*
 * Lex analyser
 */
static void number_or_word(void);	/* is a token a FLOAT, INT, or WORD?
					   allocs and sets node */
static void check_for_keyword(void);	/* see if token is a keyword */
static void next_token(void);		/* get the next token */
static void print_token(void);		/* print current token */
/*
 * Symbol table
 */
#define TBLSIZE 23			/* size of hash table */
#define BUFFSIZE 100			/* buffer size */

char buffer[BUFFSIZE];			/* input buffer */

static VECTOR *value(char *name, NODE_TYPE *type,
		     int *allocated); /* return a symbol's value */
/*
 * I/O
 */
static int peek(void);			/* look at next character */
static int readc(void);			/* read a character */
static int unreadc(int c);		/* push a character */

static char *str,*sptr;

/********************************************************************/

VECTOR *
shVectorStringEval(char *line)
{
   NODE *root;				/* the parse tree for line */
   VECTOR *x;

   sptr = str = line;
   next_token();
   if((root = expr()) == NULL) {
      return(NULL);
   } else {
      x = eval_tree(root);
   }
   
   free_tree(root);

   return(x);
}

/********************************************************************/
/*
 * Functions to manipulate and build the parse tree
 */
static NODE *
newNode(int type)
{
   NODE *node = (NODE *)shMalloc(sizeof(NODE));
   node->type = (NODE_TYPE)type;
   node->left = node->right = NULL;
   
   return(node);
}

/*****************************************************************************/

static void
delNode(NODE *node)
{
   shFree((char *)node);
}

/*****************************************************************************/
/*
 * Free a parse tree
 */
static void
free_tree(NODE *node)
{
   if(node != NULL) {
      if(node->left == node->right) {
	 free_tree(node->left);
      } else {
	 free_tree(node->left);
	 free_tree(node->right);
      }
      delNode(node);
   }
}

/********************************************************************/
/*
 * Evaluate a parse tree. This is of course where all of the real work is done
 */
static VECTOR *eval_tree(NODE *node)
{
   VECTOR *ans;
   
   if(node == NULL) {
      shErrStackPush("Internal error: NULL in parse tree\n");
      return(NULL);
   }

   if(node->type == ERROR) {
      shErrStackPush("Mistakes have been made\n");
      return(NULL);
/*
 * Primitive types
 */
   } else if(node->type == FLOAT) {
      if(node->left != NULL || node->right != NULL) {
         shErrStackPush("Internal error: non-NULL children for FLOAT node\n");
      }
      return(node->value.f);
   } else if(node->type == INT) {
      if(node->left != NULL || node->right != NULL) {
         shErrStackPush("Internal error: non-NULL children for INT node\n");
      }
      ans = shVectorNew(1);
      ans->vec[0] = node->value.l;
      return(ans);
   } else if(node->type == WORD) {
      if(node->left != NULL || node->right != NULL) {
         shErrStackPush("Internal error: non-NULL children for WORD node\n");
      }
      {
	 int allocated;
	 NODE_TYPE type;
         VECTOR *val = value(node->value.c,&type,&allocated);

         switch (type) {
          case ERROR:
	    if(allocated) shVectorDel(val);
            shErrStackPush("Unknown variable: %s\n",node->value.c);
	    return(NULL);
          case FLOAT:
	    if(allocated) {
	      ans = val;
	    } else {
	      ans = shVectorNew(val->dimen);
	      (void)memcpy((void *)ans->vec,(void *)val->vec,
			   val->dimen*(sizeof(VECTOR_TYPE)));
	    }
	    return(ans);
          default:
	    if(allocated) shVectorDel(val);
            shErrStackPush("Unknown type for %s\n",node->value.c);
	    return(NULL);
         }
      }
   }
/*
 * Now deal with nodes that actually do work
 */
   switch (node->type) {
/*
 * Array indexing
 */
    case '[':
      return(shVectorSubscript(eval_tree(node->left),eval_tree(node->right)));
/*
 * implicit DO loops
 */
    case DO:
      return(shVectorDo(eval_tree(node->left),
			  eval_tree(node->right->left),
			  eval_tree(node->right->right)));
/*
 * Concatenate two vectors
 */
    case CONCAT:
      return(shVectorConcat(eval_tree(node->left),eval_tree(node->right)));
/*
 * only return elements matching a logical expression
 */
    case IF:
      return(shVectorIf(eval_tree(node->left),eval_tree(node->right)));
/*
 * Unary arithmetic
 */
    case UMINUS:
      if(node->right != NULL) {
         shErrStackPush("Internal error: non-NULL child for unary - node\n");
      }
      return(shVectorNegate(eval_tree(node->left)));
    case CAST_INT:
      if(node->right != NULL) {
         shErrStackPush("Internal error: non-NULL child for (INT) node\n");
      }
      return(shVectorInt(eval_tree(node->left)));
    case ABS:
      if(node->right != NULL) {
         shErrStackPush("Internal error: non-NULL child for ABS node\n");
      }
      return(shVectorAbs(eval_tree(node->left)));
    case ASIN:
      if(node->right != NULL) {
         shErrStackPush("Internal error: non-NULL child for ASIN node\n");
      }
      return(shVectorAsin(eval_tree(node->left)));
    case ACOS:
      if(node->right != NULL) {
         shErrStackPush("Internal error: non-NULL child for ACOS node\n");
      }
      return(shVectorAcos(eval_tree(node->left)));
    case ATAN:
      if(node->right != NULL) {
         shErrStackPush("Internal error: non-NULL child for ATAN node\n");
      }
      return(shVectorAtan(eval_tree(node->left)));
    case COS:
      if(node->right != NULL) {
         shErrStackPush("Internal error: non-NULL child for COS node\n");
      }
      return(shVectorCos(eval_tree(node->left)));
    case DIMEN:
      if(node->right != NULL) {
         shErrStackPush("Internal error: non-NULL child for DIMEN node\n");
      }
      return(shVectorDimen(eval_tree(node->left)));
    case EXP:
      if(node->right != NULL) {
         shErrStackPush("Internal error: non-NULL child for EXP node\n");
      }
      return(shVectorExp(eval_tree(node->left)));
    case LG:
      if(node->right != NULL) {
         shErrStackPush("Internal error: non-NULL child for LG node\n");
      }
      return(shVectorLg(eval_tree(node->left)));
    case LN:
      if(node->right != NULL) {
         shErrStackPush("Internal error: non-NULL child for LN node\n");
      }
      return(shVectorLn(eval_tree(node->left)));
    case SIN:
      if(node->right != NULL) {
         shErrStackPush("Internal error: non-NULL child for SIN node\n");
      }
      return(shVectorSin(eval_tree(node->left)));
    case SQRT:
      if(node->right != NULL) {
         shErrStackPush("Internal error: non-NULL child for SQRT node\n");
      }
      return(shVectorSqrt(eval_tree(node->left)));
    case SUM:
      if(node->right != NULL) {
         shErrStackPush("Internal error: non-NULL child for SUM node\n");
      }
      return(shVectorSum(eval_tree(node->left)));
    case TAN:
      if(node->right != NULL) {
         shErrStackPush("Internal error: non-NULL child for TAN node\n");
      }
      return(shVectorTan(eval_tree(node->left)));
/*
 * Binary arithmetic
 */
    case '+':
      return(shVectorAdd(eval_tree(node->left),eval_tree(node->right)));
    case '-':
      return(shVectorSubtract(eval_tree(node->left),eval_tree(node->right)));
    case '*':
      return(shVectorMultiply(eval_tree(node->left),eval_tree(node->right)));
    case '/':
      return(shVectorDivide(eval_tree(node->left),eval_tree(node->right)));
    case '^':
      return(shVectorPow(eval_tree(node->left),eval_tree(node->right)));
    case '&':
      return(shVectorLAnd(eval_tree(node->left),eval_tree(node->right)));
    case '|':
      return(shVectorLOr(eval_tree(node->left),eval_tree(node->right)));
    case ATAN2:
      return(shVectorAtan2(eval_tree(node->left),eval_tree(node->right)));
    case MAP:
      return(shVectorMap(eval_tree(node->left), eval_tree(node->right->left),
			  eval_tree(node->right->right)));
    case NTILE:
      return(shVectorNtile(eval_tree(node->left), eval_tree(node->right)));
    case RAND:
      return(shVectorRand(eval_tree(node->left), eval_tree(node->right)));
    case RANDN:
      return(shVectorRandN(eval_tree(node->left), eval_tree(node->right)));
    case SELECT:
      return(shVectorSelect(eval_tree(node->left), eval_tree(node->right)));
    case SORT:
      if(node->left == node->right) {
	 VECTOR *left = eval_tree(node->left);
	 return(shVectorSort(left, left));
      } else {
	 return(shVectorSort(eval_tree(node->left),eval_tree(node->right)));
      }
/*
 * Unary logical
 */
    case NOT:
      if(node->right != NULL) {
         shErrStackPush("Internal error: non-NULL child for NOT node\n");
      }
      return(shVectorNot(eval_tree(node->left)));
/*
 * Binary logical
 */
    case EQ:
      return(shVectorEq(eval_tree(node->left),eval_tree(node->right)));
    case NE:
      return(shVectorNot(
		     shVectorEq(eval_tree(node->left),eval_tree(node->right))));
    case '>':
      return(shVectorGt(eval_tree(node->left),eval_tree(node->right)));
    case GE:
      return(shVectorGe(eval_tree(node->left),eval_tree(node->right)));
    case '<':
      return(shVectorNot(
		     shVectorGe(eval_tree(node->left),eval_tree(node->right))));
    case LE:
      return(shVectorNot(
		     shVectorGt(eval_tree(node->left),eval_tree(node->right))));
/*
 * Binary logical withOUT shortcircuiting
 */
    case AND:
      return(shVectorAnd(eval_tree(node->left),eval_tree(node->right)));
    case OR:
      return(shVectorOr(eval_tree(node->left),eval_tree(node->right)));
/*
 * Ternary logical
 */
    case '?':
      return(shVectorLternary(eval_tree(node->left),
			  eval_tree(node->right->left),
			  eval_tree(node->right->right)));
      return(shVectorDo(eval_tree(node->left),
			  eval_tree(node->right->left),
			  eval_tree(node->right->right)));
    default:
      shErrStackPush("Internal error: unknown node type %d\n",node->type);
      return(NULL);
   }
  return(NULL);				/* NOTREACHED */
}

/********************************************************************/

static NODE *
expr(void)
{
   NODE *left = logical_expr();
   NODE *current;

   for(;;) {
      switch (token) {
       case '?':
         current = newNode('?');
	 current->left = left;
	 current->right = newNode(':');

	 next_token();
	 current->right->left = expr();

	 if(token != ':') {
	    free_tree(current);
	    return(error());
	 }

	 next_token();
	 current->right->right = expr();

	 left = current;
	 break;
       case CONCAT:
       case IF:
         current = cnode;
	 next_token();
	 current->left = left;
	 current->right = logical_expr();
	 return(current);
       case ERROR:
	 return(error());
       default:
	 return(left);
      }
   }

   return(NULL);			/* NOTREACHED */
}

/********************************************************************/

static NODE *
logical_expr(void)
{
   NODE *left = rel_expr();
   NODE *current;

   for(;;) {
      switch (token) {
       case AND:
       case OR:
         current = newNode(token);
	 next_token();
	 current->left = left;
	 current->right = rel_expr();
	 left = current;
	 break;
       case ERROR:
	 return(error());
       default:
	 return(left);
      }
   }

   return(NULL);			/* NOTREACHED */
}

/********************************************************************/

static NODE *
rel_expr(void)
{
   NODE *left = add_expr();
   NODE *current;

   for(;;) {
      switch (token) {
       case EQ:
       case NE:
       case '>':
       case GE:
       case '<':
       case LE:
         current = newNode(token);
	 next_token();
	 current->left = left;
	 current->right = add_expr();
	 left = current;
	 break;
       case ',':
	 current = newNode(DO);
	 current->left = left;
	 current->right = newNode(',');
	 next_token();
	 current->right->left = add_expr();
	 if(token == ',') {
	    next_token();
	    current->right->right = add_expr();
	 } else {
	    current->right->right = newNode(INT);
	    current->right->right->value.l = 1;
	 }
	 return(current);
       case ERROR:
	 return(error());
       default:
	 return(left);
      }
   }

   return(NULL);			/* NOTREACHED */
}

/********************************************************************/

static NODE *
add_expr(void)
{
   NODE *left = mult_expr();
   NODE *current;

   for(;;) {
      switch (token) {
       case '+':
       case '-':
       case '&':
       case '|':
         current = newNode(token);
         current->left = left;
	 next_token();
	 current->right = mult_expr();
	 left = current;
	 break;
       case ERROR:
	 return(error());
       default:
	 return(left);
      }
   }

   return(NULL);			/* NOTREACHED */
}

/********************************************************************/

static NODE *
mult_expr(void)
{
   NODE *left = pow_expr();
   NODE *current;

   for(;;) {
      switch (token) {
       case '*':
       case '/':
         current = newNode(token);
         current->left = left;
	 next_token();
	 current->right = pow_expr();
         left = current;
         break;
       case ERROR:
	 return(error());
       default:
	 return(left);
      }
   }

   return(NULL);			/* NOTREACHED */
}

/********************************************************************/

static NODE *
pow_expr(void)
{
   NODE *left = unary_expr();
   NODE *current;

   for(;;) {
      switch (token) {
       case '^':
         current = newNode(token);
         current->left = left;
	 next_token();
	 current->right = unary_expr();
	 left = current;
	 break;
#if 0
       case ',':
	 current = newNode(DO);
	 current->left = left;
	 current->right = newNode(',');
	 next_token();
	 current->right->left = unary_expr();
	 if(token == ',') {
	    next_token();
	    current->right->right = unary_expr();
	 } else {
	    current->right->right = newNode(INT);
	    current->right->right->value.l = 1;
	 }
	 return(current);
#endif
       case ERROR:
	 return(error());
       default:
	 return(left);
      }
   }

   return(NULL);			/* NOTREACHED */
}

/********************************************************************/

static NODE *
unary_expr(void)
{
   NODE *current;
   
   for(;;) {
      switch (token) {
       case '-':			/* unary minus */
       case NOT:			/* unary ! */
         current = newNode(token == '-' ? UMINUS : token);
	 next_token();
	 current->left = unary_expr();
	 return(current);
       case '+':			/* unary plus */
	 next_token();
	 return(unary_expr());
       case ERROR:
	 return(error());
       default:
	 return(primary());
      }
   }

   return(NULL);			/* NOTREACHED */
}

/********************************************************************/

static NODE *
primary(void)
{
   NODE *left, *right;
   NODE *middle;
   NODE *current;
   
   for(;;) {
      switch (token) {
       case INT:
       case FLOAT:
         left = cnode;
	 next_token();
	 return(left);
       case WORD:
         left = cnode;
	 next_token();
	 if(token != '[') {		/* simple; not an array reference */
	    return(left);
	 } else {			/* an array index */
	    next_token();

	    current = newNode('[');
	    current->left = left;
	    current->right = add_expr();
	    
	    if(token != ']' && token != '>') {
	       return(error());
	    }
	    next_token();

	    return(current);
	 }
       case CAST_INT:
	 current = cnode;
	 next_token();
	 current->left = expr();
	 return(current);
       case '(':
	 next_token();
	 if(token == CAST_INT) {	/* this is an (int)expr */
	    next_token();
	    if(token != ')') {
	       return(error());
	    }
	    current = cnode;
	    next_token();
	    current->left = expr();
	 } else {			/* a simple (expr) */
	    current = expr();

	    if(token != ')') {
	       return(error());
	    }
	    next_token();
	 }
	 return(current);
       case ')':
	 shErrStackPush("Unbalanced right paren\n");
	 return(NULL);
       case '{':
	 {
	    int negative;		/* is a number negative? */
	    int n;			/* current index */
	    int size = 30;		/* dimen of array */
	    VECTOR *val;		/* space for values */

	    val = shVectorNew(size);
	    
	    next_token();
	    for(n = 0;;) {
	       if(n >= size) {
		  size *= 2;
		  val = shVectorResize(val,size);
	       }

	       negative = 0;		/* is this number negative? */
	       if(token == '-') {
		  negative = 1;
		  next_token();
	       }
	       if(token == FLOAT) {
		  val->vec[n++] = cnode->value.f->vec[0];
		  shVectorDel( cnode->value.f );
	       } else if(token == INT) {
		  val->vec[n++] = cnode->value.l;
	       } else if(token == '}') {
		  break;
	       } else {
		  shErrStackPush("Expected FLOAT or '}'\n");
		  break;
	       }
	       if(negative) {
		  val->vec[n - 1] = -val->vec[n - 1];
	       }
	       delNode(cnode);
	       next_token();
	    }
	    current = newNode(FLOAT);
	    current->value.f = shVectorResize(val,n); /* discard unwanted space */
	 }
	 next_token();
	 return(current);
       case '}':
	 shErrStackPush("Unbalanced right curly bracket\n");
	 return(NULL);
       case ABS:
       case ASIN:
       case ACOS:
       case ATAN:
       case COS:
       case DIMEN:
       case EXP:
       case LG:
       case LN:
       case SIN:
       case SQRT:
       case SUM:
       case TAN:
	 current = cnode;
	 next_token();
	 if(token != '(') {
	    shErrStackPush("Require ( following function call\n");
	    return(error());
	 }

	 next_token();
	 left = expr();

	 if(token != ')') {
	    shErrStackPush("Missing ) at end of function call argument\n");
	    return(error());
	 }
	 next_token();

	 current->left = left;
	 
	 return(current);
       case ATAN2:
       case NTILE:
       case SELECT:
       case RAND:
       case RANDN:
	 current = cnode;
	 next_token();
	 if(token != '(') {
	    return(error());
	 }

	 next_token();
	 left = expr();

	 if(token != ':') {
	    return(error());
	 }
	 next_token();

	 right = expr();

	 if(token != ')') {
	    return(error());
	 }
	 next_token();

	 current->left = left;
	 current->right = right;
	 return(current);

       case MAP:
	 current = cnode;
	 next_token();
	 if(token != '(') {
	    return(error());
	 }

	 next_token();
	 left = expr();

	 if(token != ':') {
	    return(error());
	 }
	 next_token();

	 middle  = expr();

	 if(token != ':') {
	    return(error());
	 }
	 next_token();

	 right = expr();

	 if(token != ')') {
	    return(error());
	 }
	 next_token();

	 current->left = left;
	 current->right = newNode(':');
	 current->right->left = middle;
	 current->right->right = right;
	 return(current);

       case SORT:
	 current = cnode;
	 next_token();
	 if(token != '(') {
	    return(error());
	 }

	 next_token();
	 left = expr();

	 if(token == ')') {		/* sort(expr) => return sorted vector*/
	    next_token();
	    
	    current->left = left;
	    current->right = NULL;
	    return(current);
	 } else if(token != ':') {
	    return(error());
	 }
	 next_token();

	 if(token == ')') {		/* sort(expr:) => return indices */
	    next_token();
	    current->left = left;
	    current->right = left;

	    return(current);
	 }
	 
	 right = expr();

	 if(token != ')') {
	    return(error());
	 }
	 next_token();

	 current->left = left;
	 current->right = right;
	 return(current);
       case EOF:
	 next_token();
	 return(NULL);
       case ERROR:
	 return(error());
       default:
	 shErrStackPush("Primary expected, not ");
	 print_token();
	 next_token();
      }
   }

   return(NULL);			/* NOTREACHED */
}

/********************************************************************/

static NODE *
error(void)
{
   shErrStackPush("Syntax error:\n%s\n%*s^\n",str,sptr - str - 1,"");
   next_token();
   return(NULL);
}

/********************************************************************/
/*
 * End of parser, now the Lex analyser
 */
/********************************************************************/

static void
next_token(void)
{
   int c, c2;
   char *ptr = buffer;
   
   while(ptr < buffer + BUFFSIZE) {
      if((c = readc()) == EOF) {
	 if(ptr == buffer) {
	    token = EOF;
	    return;
	 }
	 *ptr = '\0';
	 number_or_word();
	 if(token == WORD) {
	    check_for_keyword();
	 }
	 return;
      }

      switch (c) {
       case '\n':
       case ' ':
       case '\t':
       case '(':
       case ')':
       case '[':
       case ']':
       case '{':
       case '}':
       case '-':
       case '+':
       case '*':
       case '/':
       case '=':
       case '!':
       case '<':
       case '>':
       case '|':
       case '&':
       case '?':
       case ':':
       case '^':
       case ',':
 	 if(ptr > buffer) {		/* terminate previous token */
	    (void)unreadc(c);
	    *ptr = '\0';
	    number_or_word();
	    if(token == WORD) {
	       check_for_keyword();
	    }
	 } else if(c == ' ' || c == '\t') {
	    break;
	 } else if(c == '=') {
	    if(peek() == '=') {
	       (void)readc();
	       token = EQ;
	    } else {
	       token = ERROR;
	    }
	 } else if(c == '!') {
	    if(peek() == '=') {
	       (void)readc();
	       token = NE;
	    } else {
	       token = NOT;
	    }
	 } else if(c == '<') {
	    if(peek() == '=') {
	       (void)readc();
	       token = LE;
	    } else {
	       token = '<';
	    }
	 } else if(c == '>') {
	    if(peek() == '=') {
	       (void)readc();
	       token = GE;
	    } else {
	       token = '>';
	    }
	 } else if(c == '&') {
	    if(peek() == '&') {
	       (void)readc();
	       token = AND;
	    } else {
	       token = '&';
	    }
	 } else if(c == '|') {
	    if(peek() == '|') {
	       (void)readc();
	       token = OR;
	    } else {
	       token = '|';
	    }
	 } else {
	    token = c;
	 }
#if 0
	 print_token();
#endif
	 return;
       default:
	 *ptr++ = c;
#if !defined(NO_ANGLES)
	 if((c = readc()) == '<') {	/* < not following white space */
	    c = '[';			/* replace with '[' */
	 }
	 unreadc(c);
#endif
	 c2 = readc();
	 if(c2 == 'e' || c2 == 'E') {	/* < not following white space */
	   if((c = readc()) == '+') {	/* < not following white space */
	     c = 'p';			/* replace with '[' */
	   }
	   unreadc(c);
	   if((c = readc()) == '-') {	/* < not following white space */
	     c = 'm';			/* replace with '[' */
	   }	   
	   unreadc(c);
	 }
	 unreadc(c2);
	 break;
      }
   }
}

/********************************************************************/

static void
number_or_word(void)
{
  char *conv_ptr;
  long conv_val = 0;
  char *ptr = buffer;

   if(*ptr == '-' || *ptr == '+') ptr++;

   if(ptr[0] == '0' && ptr[1] == 'x') {	/* probably a hex constant */
      conv_ptr=NULL;
      conv_val = strtol(buffer, &conv_ptr, 16);
      if(errno == ERANGE ) {  /* error */
	 cnode = newNode(token = FLOAT);
	 cnode->value.f = shVectorNew(1);
	 cnode->value.f->vec[0] =  strtod(buffer, &conv_ptr);
	 if(conv_ptr == buffer) {
	    shErrStackPush("Syntax error in number: %s\n", buffer);   
	    /* error*/
	    return;
	 }
	 return;
      } else {
	 cnode = newNode(token = INT);
	 cnode->value.l = conv_val;
	 return;
      }
   }

   if(isdigit(*ptr) || *ptr == '.') {	/* probably FLOAT or INT */
      while(isdigit(*ptr)) ptr++;
      if(*ptr == '.') {
         ptr++;
         while(isdigit(*ptr)) ptr++;
         if(*ptr == 'e' || *ptr == 'E') {
            ptr++;
            if(*ptr == 'p') {
	      *ptr = '+';
	      ptr++;	    
	    }
            if(*ptr == 'm') {
	      *ptr = '-';
	      ptr++;	    
	    }
            while(isdigit(*ptr)) ptr++;
	  }
         if(*ptr == '\0') {
	   cnode = newNode(token = FLOAT);
	   cnode->value.f = shVectorNew(1);
	   cnode->value.f->vec[0] = strtod(buffer, &conv_ptr);
	   if(conv_ptr == buffer) {
	     shErrStackPush("Syntax error in number: %s\n", buffer);	     
	     /* error*/
	     return;
	   }
	   return;
         }
      } else if(*ptr == '\0') {
	conv_ptr=NULL;
	conv_val = strtol(buffer, &conv_ptr, 10);
	if(errno == ERANGE ) {  /* error */
	  cnode = newNode(token = FLOAT);
	  cnode->value.f = shVectorNew(1);
	  cnode->value.f->vec[0] =  strtod(buffer, &conv_ptr);
	  if(conv_ptr == buffer) {
	    shErrStackPush("Syntax error in number: %s\n", buffer);   
	    /* error*/
	    return;
	  }
	  return;
	} else {
	  cnode = newNode(token = INT);
	  cnode->value.l = conv_val;
	  return;
	}
      }
    }
   cnode = newNode(token = WORD);
   strncpy(cnode->value.c,buffer,NAMESIZE);
}

/*****************************************************************************/
/*
 * See if the current word is a keyword; if so set token
 */
static void
check_for_keyword(void)
{
   switch (cnode->value.c[0]) {
    case 'a':
      if(strcmp(cnode->value.c,"abs") == 0) {
	 token = ABS;
      } else if(strcmp(cnode->value.c,"acos") == 0) {
	 token = ACOS;
      } else if(strcmp(cnode->value.c,"asin") == 0) {
	 token = ASIN;
      } else if(strcmp(cnode->value.c,"atan") == 0) {
	 token = ATAN;
      } else if(strcmp(cnode->value.c,"atan2") == 0) {
	 token = ATAN2;
      }
      break;
    case 'c':
      if(strcmp(cnode->value.c,"concat") == 0) {
	 token = CONCAT;
      } else if(strcmp(cnode->value.c,"cos") == 0) {
	 token = COS;
      }
      break;
    case 'd':
      if(strcmp(cnode->value.c,"dimen") == 0) {
	 token = DIMEN;
      }
      break;
    case 'e':
      if(strcmp(cnode->value.c,"exp") == 0) {
	 token = EXP;
      }
      break;
    case 'i':
      if(strcmp(cnode->value.c,"if") == 0) {
	 token = IF;
      } else if(strcmp(cnode->value.c,"int") == 0) {
	 token = CAST_INT;
      }
      break;
    case 'l':
      if(strcmp(cnode->value.c,"lg") == 0) {
	 token = LG;
      } else if(strcmp(cnode->value.c,"ln") == 0) {
	 token = LN;
      }
      break;
    case 'm':
      if(strcmp(cnode->value.c,"map") == 0) {
	 token = MAP;
      }
      break;
    case 'n':
      if(strcmp(cnode->value.c,"ntile") == 0) {
	 token = NTILE;
      }
      break;
    case 'r':
      if(strcmp(cnode->value.c,"rand") == 0) {
	 token = RAND;
      } else if(strcmp(cnode->value.c,"randn") == 0) {
	 token = RANDN;
      }
      break;
    case 's':
      if(strcmp(cnode->value.c,"select") == 0) {
	 token = SELECT;
      } else if(strcmp(cnode->value.c,"sin") == 0) {
	 token = SIN;
      } else if(strcmp(cnode->value.c,"sort") == 0) {
	 token = SORT;
      } else if(strcmp(cnode->value.c,"sqrt") == 0) {
	 token = SQRT;
      } else if(strcmp(cnode->value.c,"sum") == 0) {
	 token = SUM;
      }
      break;
    case 't':
      if(strcmp(cnode->value.c,"tan") == 0) {
	 token = TAN;
      }
      break;
    default:
      break;
   }

   cnode->type = (NODE_TYPE)token;
}

/********************************************************************/

static void
print_token(void)
{
   switch (token) {
    case '\n':
      shErrStackPush("\\n\n"); break;
    case ERROR:
      shErrStackPush("ERROR\n"); break;
    case FLOAT:
      shErrStackPush("FLOAT: %g\n",cnode->value.f); break;
    case INT:
      shErrStackPush("INT: %ld\n",cnode->value.l); break;
    case WORD:
      shErrStackPush("WORD: %s\n",buffer); break;
    case EQ:
      shErrStackPush("==\n"); break;
    case NE:
      shErrStackPush("!=\n"); break;
    case GE:
      shErrStackPush(">=\n"); break;
    case LE:
      shErrStackPush("<=\n"); break;
    case NOT:
      shErrStackPush("!\n"); break;
    case AND:
      shErrStackPush("&&\n"); break;
    case OR:
      shErrStackPush("||\n"); break;
    case ABS:
      shErrStackPush("ABS\n"); break;
    case ACOS:
      shErrStackPush("ACOS\n"); break;
    case ASIN:
      shErrStackPush("ASIN\n"); break;
    case ATAN:
      shErrStackPush("ATAN\n"); break;
    case ATAN2:
      shErrStackPush("ATAN2\n"); break;
    case COS:
      shErrStackPush("COS\n"); break;
    case CAST_INT:
      shErrStackPush("(INT)\n"); break;
    case DIMEN:
      shErrStackPush("DIMEN\n"); break;
    case LG:
      shErrStackPush("LG\n"); break;
    case LN:
      shErrStackPush("LN\n"); break;
    case MAP:
      shErrStackPush("MAP\n"); break;
    case NTILE:
      shErrStackPush("NTILE\n"); break;
    case RAND:
      shErrStackPush("RAND\n"); break;
    case RANDN:
      shErrStackPush("RANDN\n"); break;
    case SELECT:
      shErrStackPush("SELECT\n"); break;
    case SIN:
      shErrStackPush("SIN\n"); break;
    case SORT:
      shErrStackPush("SORT\n"); break;
    case SQRT:
      shErrStackPush("SQRT\n"); break;
    case SUM:
      shErrStackPush("SUM\n"); break;
    case TAN:
      shErrStackPush("TAN\n"); break;
    default:
      shErrStackPush("%c\n",token);
      break;
   }
}

/********************************************************************/
/*
 * End of Lex analyser, now the symbol table
 *
 * We don't maintain a true symbol table, but simply ask TCL if it knows
 * about a given handle. Rather than passing a TCL interpreter in to this
 * parser, we assume that the tcl level code (tclVector.c) provides the
 * desired function, p_shVectorGet
 */
static VECTOR *
value(char *name,
      NODE_TYPE *type,
      int *allocated)		/* did value allocate memory? */
{
   VECTOR *vec;

   if((vec = p_shVectorGet(name, allocated)) == NULL) {
      shErrStackPush("%s is not defined\n",name);
      *type = ERROR;
      return(NULL);
   }
   *type = FLOAT;
   return(vec);
}

/********************************************************************/
/*
 * End of symbol table, now I/O (read from a string)
 */
/********************************************************************/

static int
readc(void)
{
   int c;
   
   if((c = *sptr++) == '\0') {
      sptr--;
      return(EOF);
   } else {
      return(c);
   }
}

/********************************************************************/

static int
unreadc(int c)
{
   if(sptr > str) {
      *--sptr = (c == -1) ? '\0' : c;
   }
   return(c);
}

/********************************************************************/

static int
peek(void)
{
   return(*sptr == '\0' ? EOF : *sptr);
}

