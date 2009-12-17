/* <AUTO>
   FILE:
                vecChain.c
<HTML>
Interface between vectors and chains and regions, etc.
</HTML>
</AUTO> */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <float.h>
#if !defined(DARWIN)
#  include <values.h>
#endif
#include <alloca.h>
#include "shCAssert.h"
#include "shCDiskio.h"

#if !defined(NOTCL)
#include "pgplot_c.h"
#endif

#include "shChain.h"
#include "shCVecChain.h"
#include "shCSchema.h"
#include "shCVecExpr.h"
#if !defined(NOTCL)
#include "cpgplot.h"
#endif

#include "shCGarbage.h"
#include "shCErrStack.h"
#include "shTclHandle.h"

RET_CODE shExprEval(SCHEMA* schema,  char *line, char* address_in, 
	 char** address_out, int* out_type, char** userPtr);

RET_CODE shExprTypeEval(SCHEMA *, char *, int *);

#define isgood(x)   ( ( ((x) < FLT_MAX)) && ((x) > -FLT_MAX) ? (1) : (0))
#define IQR_TO_SIGMA 0.741301		/* sigma = 0.741*(interquartile range)
					   for a Gaussian */


static void
get_quartile_from_array(const void *arr, /* the data values */
			int type,	/* type of data */
			int n,		/* number of points */
			int clip,	/* should we clip histogram? */
			float qt[3],	/* quartiles */
			float *mean,	/* mean (may be NULL) */
			float *sig);	/* s.d. (may be NULL) */

/* External variables referenced here */
/* extern SAOCMDHANDLE g_saoCmdHandle; */

/*<AUTO EXTRACT>
  ROUTINE: shVFromChain
  DESCRIPTION:
  <HTML>
  Create a vector based on the contents of the chain
  </HTML>
  RETURN VALUES:
  <HTML>
  Pointer to a VECTOR
  </HTML>
</AUTO>*/
VECTOR *shVFromChain(
			  CHAIN *chain, 
			  char *member
			  )
{
  int i;
  int is_enum;				/* the element type is an enum */
  const int nel = shChainSize(chain);
  VECTOR *vector;			/* desired VECTOR */
  const SCHEMA *schema;
  TYPE type;
  const TYPE UCHAR  = shTypeGetFromName("UCHAR");
  const TYPE CHAR   = shTypeGetFromName("CHAR");
  const TYPE USHORT = shTypeGetFromName("USHORT");
  const TYPE SHORT  = shTypeGetFromName("SHORT");
  const TYPE UINT   = shTypeGetFromName("UINT");
  const TYPE INT    = shTypeGetFromName("INT");
  const TYPE ULONG  = shTypeGetFromName("ULONG");
  const TYPE LONG   = shTypeGetFromName("LONG");
  const TYPE FLOAT  = shTypeGetFromName("FLOAT");
  const TYPE DOUBLE = shTypeGetFromName("DOUBLE");
  void *item=NULL, *element=NULL;
  char *dummy;
  RET_CODE rstatus;

  if(nel == 0) {
     return(NULL);
  }

  vector = shVectorNew(nel);
  shVNameSet(vector, member);
  
  type = shChainElementTypeGetByPos(chain, HEAD);
  schema = shSchemaGet(shNameGetFromType(type));
/*
 * Get the first element and see if it's an enum
 */
  item = shChainElementGetByPos(chain, HEAD);
  shAssert(item != NULL);
  
  rstatus = shExprEval((SCHEMA *)schema, member, (char *)item, 
		       (char **)&element, &type, &dummy);
  if(rstatus != SH_SUCCESS) {
     shVectorDel(vector);
     return NULL;
  }
  {
     const SCHEMA *elem_schema = shSchemaGet(shNameGetFromType(type));
     is_enum = (elem_schema->type == ENUM) ? 1 : 0;
  }
/*
 * OK; process the whole chain
 */
  for(i = 0; i < nel; i++) {
     item = shChainElementGetByPos(chain, i);
     shAssert(item != NULL);
     
     rstatus = shExprEval((SCHEMA *)schema, member, (char *)item, 
			  (char **)&element, &type, &dummy);
     if (rstatus != SH_SUCCESS) {
	shVectorDel(vector);
	return NULL;
     }
     
     if(type == DOUBLE) {
	vector->vec[i] = (VECTOR_TYPE) *(double *)element;
     } else if(type == INT) {
	vector->vec[i] = (VECTOR_TYPE) *(int *)element;
     } else if(type == FLOAT) {
	vector->vec[i] = (VECTOR_TYPE) *(float *)element;
     } else if(type == SHORT) {
	vector->vec[i] = (VECTOR_TYPE) *(short *)element;
     } else if(type == USHORT) {
	vector->vec[i] = (VECTOR_TYPE) *(unsigned short *)element;
     } else if(type == UINT) {
	vector->vec[i] = (VECTOR_TYPE) *(unsigned int *)element;
     } else if(type == LONG) {
	vector->vec[i] = (VECTOR_TYPE) *(long *)element;
     } else if(type == ULONG) {
	vector->vec[i] = (VECTOR_TYPE) *(unsigned long *)element;
     } else if(type == UCHAR) {
	vector->vec[i] = (VECTOR_TYPE) *(unsigned char *)element;
     } else if(type == CHAR) {
	vector->vec[i] = (VECTOR_TYPE) *(char *)element;
     } else {
	if(is_enum) {
	   vector->vec[i] = (VECTOR_TYPE) *(int *)element;
	} else {
	   shErrStackPush("shVFromChain: "
			  "I don't know how to process %s", member);
	   shVectorDel(vector);
	   return NULL;
	}
     }
  }

  return vector;
}

/*****************************************************************************/
/*
 * Convert from TYPE to this short enum (which can be used in switches)
 */
enum {
   UCHAR, CHAR, USHORT, SHORT, UINT, INT, ULONG, LONG, FLOAT, DOUBLE, UNK
};
static int *itype_type = NULL;		/* itypes corresponding to TYPEs */
static int max_known_type = 0;		/* TYPE of largest primitive type */

static int
itype_from_type(TYPE type)		/* type to convert */
{
   const SCHEMA *elem_schema;		/* schema for member */
   int i;
   int itype;				/* type as translated to that enum */
   TYPE t;				/* a value of a TYPE */
   const char *types[] = { "DOUBLE", "FLOAT", "SHORT", "USHORT", "INT", "UINT",
			     "LONG", "ULONG", "CHAR", "UCHAR", NULL };

   if(itype_type == NULL) {
      for(i = 0; types[i] != NULL; i++) {
	 t = shTypeGetFromName((char *)types[i]);
	 if(t > max_known_type) {
	    max_known_type = t;
	 }
      }

      itype_type = shMalloc((max_known_type + 1)*sizeof(int));

      for(i = 0; i <= max_known_type; i++) {
	 itype_type[i] = UNK;
      }
      
      for(i = 0; types[i] != NULL; i++) {
	 t = shTypeGetFromName((char *)types[i]);

	 if(t == shTypeGetFromName("DOUBLE")) {
	    itype = DOUBLE;
	 } else if(t == shTypeGetFromName("INT")) {
	    itype = INT;
	 } else if(t == shTypeGetFromName("FLOAT")) {
	    itype = FLOAT;
	 } else if(t == shTypeGetFromName("SHORT")) {
	    itype = SHORT;
	 } else if(t == shTypeGetFromName("USHORT")) {
	    itype = USHORT;
	 } else if(t == shTypeGetFromName("UINT")) {
	    itype = UINT;
	 } else if(t == shTypeGetFromName("LONG")) {
	    itype = LONG;
	 } else if(t == shTypeGetFromName("ULONG")) {
	    itype = ULONG;
	 } else if(t == shTypeGetFromName("UCHAR")) {
	    itype = UCHAR;
	 } else if(t == shTypeGetFromName("CHAR")) {
	    itype = CHAR;
	 } else {
	    itype = UNK;
	 }

	 itype_type[t] = itype;
      }
   }
/*
 * OK, we've built the lookup table.  Time to do the conversion
 */
   if(type < 0) {
      return(UNK);
   } else if(type <= max_known_type) {
      return(itype_type[type]);
   }
/*
 * Hmm, an unknown type.  Try to look it up
 */
   elem_schema = shSchemaGet(shNameGetFromType(type));
   if(elem_schema->type == ENUM) {
      switch(elem_schema->size) {
       case 1: return(UCHAR);
       case 2: return(USHORT);
       case 4: return(ULONG);
       default:
	 shError("itype_from_type: an enum %s of length %d!?",
		 type, elem_schema->size);
	 break;
      }
   }

   return(UNK);
}

/*
 * Process the request for an element `member' of struct item (with
 * specified schema) and set itype/element and return offset
 */
static int
parse_element(const SCHEMA *schema,	/* schema that member belongs to */
	      const char *item,		/* data item with specified schema */
	      const char *member,	/* desired member */
	      int *offset,		/* returned offset */
	      int *itype,		/* returned type, or NULL */
	      char **element)		/* pointer to desired member, or NULL*/
{
   char *dummy;
   char *element_s;			/* storage for element, if needed */
   int itype_s;				/* storage for itype if needed */
   TYPE type;				/* type of a schema element */

   if(element == NULL) {
      element = &element_s;
   }
   if(itype == NULL) {
      itype = &itype_s;
   }

   if(shExprEval((SCHEMA *)schema, (char *)member, (char *)item, 
		 element, &type, &dummy) != SH_SUCCESS) {
      return(-1);
   }
/*
 * Convert that TYPE to a small integer
 */
   *itype = itype_from_type(type);
   
   *offset = *element - item;		/* n.b. sizeof(*item) == 1 */

   return(0);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Create an array of vectors based on the contents of the chain
 *
 * Three cases are handled:
 *   1/ `member' is the name of a member of the struct in the chain,
 *   or an element of such a member if its' an array
 *   2/ `member' is the name a member of a member of the struct in the chain,
 *   or an element of such a member if it's an array
 *   3/ anything more complex
 *
 * schematically,
 * For case 1, value = *(struct + offsetMember)
 * For case 2, value = *(*(struct + offsetStruct) + offsetMember)
 * For case 3, call shExprEval to find value
 *
 * In all cases this is faster than shVFromChain as the chain is
 * only scanned once; in cases 1 and 2 it is _much_ faster as the
 * expression is only parsed once (factors of ~ 10 per element).
 *
 * E.g., given
 *  typedef struct {
 *     int j<2>;
 * } FOO;
 *
 * typedef struct {
 *    int i;
 *    int a<2>;
 *    REGION *reg;
 *    FOO *foo;
 * } TST;
 *
 * then:
 *  vectorsFromChain $tst_chain i               case = 1
 *  vectorsFromChain $tst_chain a<1>            case = 1
 *  vectorsFromChain $tst_chain foo             case = 1
 *             Error: shVectorsFromChain: TST.foo is not a primitive type
 *  vectorsFromChain $tst_chain foo->j<0>       case = 2
 *  vectorsFromChain $tst_chain reg->nrow       case = 2
 *  vectorsFromChain $tst_chain reg->rows<1><1> case = 3
 */
VECTOR **
shVectorsFromChain(const CHAIN *chain,	/* the CHAIN */
		   int nvec,		/* number of desired members */
		   char **members)	/* names of desired members */
{
   unsigned char *cases;		/* which of the three cases is it? */
   TYPE chtype;				/* chain's TYPE */
   char *element = NULL;		/* pointer to desired member */
   const SCHEMA *elem_schema;		/* schema for members[] */
   int i, j;
   int *offsetsStruct;			/* offsets of structs in schema */
   int *offsetsMember;			/* offsets of members in schema */
   const int nel = shChainSize(chain);
   const SCHEMA *schema;		/* schema of an element of the chain */
   int *itypes;				/* members' TYPEs */
   char *item=NULL;			/* we want sizeof(*item) == 1 */
   char *ptr;				/* useful pointer */
   VECTOR **vectors;			/* array of vectors to return */
   
   if(nel == 0) {
      return(NULL);
   }
/*
 * Allocate vectors
 */
   vectors = shMalloc(nvec*sizeof(VECTOR *));
   for(i = 0; i < nvec; i++) {
      vectors[i] = shVectorNew(nel);
      shVNameSet(vectors[i], members[i]);
   }
   cases = alloca(nvec*sizeof(unsigned char));
   offsetsMember = alloca(nvec*sizeof(int));
   offsetsStruct = alloca(nvec*sizeof(int));
   itypes = alloca(nvec*sizeof(TYPE));
/*
 * Get the first element, find the offsets of desired members in struct,
 * and see if they're enums
 */
   chtype = shChainElementTypeGetByPos(chain, HEAD);
   schema = shSchemaGet(shNameGetFromType(chtype));
   
   item = shChainElementGetByPos(chain, HEAD);
   shAssert(item != NULL);
   
   for(i = 0; i < nvec; i++) {
      if(parse_element(schema, item, members[i], &offsetsMember[i],
				       &itypes[i], &element) < 0) {
	 for(i = 0; i < nvec; i++) {
	    shVectorDel(vectors[i]);
	 }
	 shFree(vectors);
	 shFree(itype_type); itype_type = NULL;
	   
	 return NULL;
      }
/*
 * See which case we've got
 */
      if(offsetsMember[i] >= 0 && offsetsMember[i] < schema->size) {
	 cases[i] = 1;			/* easy */
	 continue;
      }
/*
 * Was there just a single indirection and no array dereference?
 */
      if((ptr = strchr(members[i], '-')) != NULL && ptr[1] == '>' &&
	 strchr(ptr + 2, '-') == NULL && strchr(ptr + 2, '.') == NULL &&
	 strchr(ptr + 2, '*') == NULL &&
	 strchr(ptr + 2, '<') == NULL && strchr(ptr + 2, '[') == NULL) {

/*
 * get schema for the `member base' (mbase), the part before the "->"
 */
	 char mbase[100];
	 strcpy(mbase, members[i]); mbase[ptr - members[i]] = '\0';
	 
	 elem_schema = NULL;
	 for(j = 0; j < schema->nelem; j++) {
	    if(strcmp(mbase, schema->elems[j].name) == 0) {
	       offsetsStruct[i] = schema->elems[j].offset;
	       elem_schema = shSchemaGet(schema->elems[j].type);
	       break;
	    }
	 }
	 if(elem_schema == NULL) {
	    for(i = 0; i < nvec; i++) {
	       shVectorDel(vectors[i]);
	    }
	    shFree(vectors);
	    shFree(itype_type); itype_type = NULL;
	    
	    return NULL;
	 }
      
	 ptr += 2;
	 if(offsetsStruct[i] < 0 ||
	    parse_element(elem_schema, *(void**)(item + offsetsStruct[i]), ptr,
			  &offsetsMember[i], &itypes[i], NULL) < 0) {
	    for(i = 0; i < nvec; i++) {
	       shVectorDel(vectors[i]);
	    }
	    shFree(vectors);
	    shFree(itype_type); itype_type = NULL;
	    
	    return NULL;
	 }
/*
 * See which case we've got
 */
	 if(offsetsMember[i] >= 0 && offsetsMember[i] < elem_schema->size) {
	    cases[i] = 2;			/* easyish */
	    continue;
	 }
      }

      cases[i] = 3;
   }
/*
 * Check that we got a type for all elements
 */
   {
      int bad = 0;
      for(j = 0; j < nvec; j++) {
	 if(cases[j] < 3 && itypes[j] == UNK) {
	    shErrStackPush("shVectorsFromChain: %s.%s is not a primitive type",
		    schema->name, members[j]);
	    bad++;
	 }
      }

      if(bad) {
	 for(i = 0; i < nvec; i++) {
	    shVectorDel(vectors[i]);
	 }
	 shFree(vectors);
	 shFree(itype_type); itype_type = NULL;
	 
	 return NULL;
      }
   }
/*
 * OK; process the whole chain
 */
   for(i = 0; i < nel; i++) {
      item = shChainElementGetByPos(chain, i);
      shAssert(item != NULL);
      
      for(j = 0; j < nvec; j++) {
	 const char *iptr;		/* item, or a derived pointer */
	 int itype = itypes[j];
	 int off = offsetsMember[j];
	 VECTOR_TYPE *vec = vectors[j]->vec;
	 
	 switch (cases[j]) {
	  case 1:			/* member of CHAIN's schema */
	    iptr = item;
	    break;
	  case 2:			/* one level of indirection's needed */
	    iptr = *(void **)(item + offsetsStruct[j]);
	    break;
	  case 3:			/* general expression */
	    {
	       char *dummy;
	       TYPE type;			/* type of a schema element */
	       
	       if(shExprEval((SCHEMA *)schema, members[j], (char *)item, 
			     (char **)&element, &type, &dummy) != SH_SUCCESS) {
		  for(i = 0; i < nvec; i++) {
		     shVectorDel(vectors[i]);
		  }
		  shFree(vectors);
		  shFree(itype_type); itype_type = NULL;
		  
		  return NULL;
	       }

	       iptr = item;
	       off = element - item;
	       itype = itype_from_type(type);
	    }
	    break;
	  default:
	    iptr = NULL;		/* make gcc happy */
	    shFatal("shVectorsFromChain: You can't get here (I)");
	    break;			/* NOTREACHED */
	 }

	 switch (itype) {
	  case DOUBLE:
	    vec[i] = *(double *)(iptr + off);
	    break;
	  case INT:
	    vec[i] = *(int *)(iptr + off);
	    break;
	  case FLOAT:
	    vec[i] = *(float *)(iptr + off);
	    break;
	  case SHORT:
	    vec[i] = *(short *)(iptr + off);
	    break;
	  case USHORT:
	    vec[i] = *(unsigned short *)(iptr + off);
	    break;
	  case UINT:
	    vec[i] = *(unsigned int *)(iptr + off);
	    break;
	  case LONG:
	    vec[i] = *(long *)(iptr + off);
	    break;
	  case ULONG:
	    vec[i] = *(unsigned long *)(iptr + off);
	    break;
	  case UCHAR:
	    vec[i] = *(unsigned char *)(iptr + off);
	    break;
	  case CHAR:
	    vec[i] = *(char *)(iptr + off);
	    break;
	  default:
	    shFatal("shVectorsFromChain: You can't get here (II)");
	    break;			/* NOTREACHED */
	 }
      }
   }

   shFree(itype_type); itype_type = NULL;

   return vectors;
}


/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * extract some range of rows and columns of a REGION
 * into a VECTOR.
 *
 * The rectangular region with corners (row0, col0) and (row1, col1)
 * is averaged over row (dirn == 0) or over column (dirn == 1). Note
 * that dirn == 1 gives a _vertical_ slice through the data (note that
 * a 1 looks a little like a column...)
 *
 * As a special case, a negative value of row1 will be taken to be
 *   nrow - |row1|
 * and similarily for columns
 */
VECTOR *
shVectorGetFromRegion(const REGION *reg, /* the region with the data */
		      int row0, int row1, /* row range, inclusive */
		      int col0, int col1, /* column range, inclusive */
		      int dirn, /* 0 for collapse onto a column,
				   1 for collapsing onto a row */
		      int use_median)	/* use median rather than mean */
{
   U16 *arr;
   int dimen;			/* dimension of vec */
   double i_drow;		/* == 1/(double)(row1 - row0 + 1) */
   double i_dcol;		/* == 1/(double)(col1 - col0 + 1) */
   int i, j;
   float qt[3];			/* quartiles */
   double sum;			/* used in filling vector */
   VECTOR *vec;			/* vector to return */
   VECTOR_TYPE *vec_vec;	/* == vec->vec */

   shAssert(reg != NULL);

   if(row1 < 0) row1 = reg->nrow + row1;
   if(col1 < 0) col1 = reg->ncol + col1;

   if(row0 > row1 || col0 > col1) {
      shErrStackPush("shVectorGetFromRegion: row/col out of order");
      return(NULL);
   }
   if(row0 < 0 || row1 >= reg->nrow || col0 < 0 || col1 >= reg->ncol) {
      shErrStackPush("shVectorGetFromRegion: row/col out of range");
      return(NULL);
   }

   if(dirn == 0) {
      dimen = col1 - col0 + 1;
   } else if(dirn == 1) {
      dimen = row1 - row0 + 1;
   } else {
      shErrStackPush("shVectorGetFromRegion: illegal value of dirn: %d",dirn);
      return(NULL);
   }
   vec = shVectorNew(dimen); vec_vec = vec->vec;
/*
 * OK, time to do the work
 */
   i_dcol = 1/(double)(col1 - col0 + 1);
   i_drow = 1/(double)(row1 - row0 + 1);
/*
 * The work is _identical_ for all data types, so let's use a macro;
 * this makes it hard to debug, of course....
 */
#define DO_WORK \
   if(dirn == 0) { \
      if(use_median) { \
	 arr = alloca((row1 - row0 + 1)*sizeof(U16)); \
	 for(j = col0; j <= col1; j++) { \
	    for(i = row0; i <= row1; i++) { \
	       arr[i - row0] = rows[i][j]; \
	    } \
	    get_quartile_from_array(arr, reg->type, row1 - row0 + 1, 1, qt, NULL, NULL); \
	    vec_vec[j - col0] = qt[1]; \
	 } \
      } else { \
	 for(j = col0; j <= col1; j++) { \
	    vec_vec[j - col0] = 0; \
	 } \
	 \
	 for(i = row0; i <= row1; i++) { \
	    row = rows[i]; \
	    \
	    for(j = col0; j <= col1; j++) { \
	       vec_vec[j - col0] += row[j]; \
	    } \
	 } \
	 for(j = col0; j <= col1; j++) { \
	    vec_vec[j - col0] = vec->vec[j - col0]*i_drow; \
	 } \
      } \
   } else { \
      for(i = row0; i <= row1; i++) { \
	 row = rows[i]; \
	 sum = 0; \
         if(use_median) { \
	    get_quartile_from_array(&row[col0], reg->type, col1 - col0 + 1, 1, qt, NULL, NULL); \
	    vec_vec[i - row0] = qt[1]; \
	 } else { \
	    for(j = col0; j <= col1; j++) { \
	       sum += row[j]; \
	    } \
	    vec_vec[i - row0] = sum*i_dcol; \
	 } \
      } \
   }
   
   switch (reg->type) {
    case TYPE_U8:
      {
	 U8 **rows = reg->rows_u8, *row;

	 DO_WORK;
      }

      break;
    case TYPE_S8:
      {
	 S8 **rows = reg->rows_s8, *row;

	 DO_WORK;
      }
      break;
    case TYPE_U16:
      {
	 U16 **rows = reg->rows_u16, *row;

	 DO_WORK;
      }

      break;
    case TYPE_S16:
      {
	 S16 **rows = reg->rows_s16, *row;

	 DO_WORK;
      }

      break;
    case TYPE_U32:
      {
	 U32 **rows = reg->rows_u32, *row;

	 DO_WORK;
      }

      break;
    case TYPE_S32:
      {
	 S32 **rows = reg->rows_s32, *row;

	 DO_WORK;
      }

      break;
    case TYPE_FL32:
      {
	 FL32 **rows = reg->rows_fl32, *row;

	 DO_WORK;
      }

      break;
    default:
      shErrStackPush("shVectorGetFromRegion: unsupported type %d",reg->type);
      shVectorDel(vec);
      return(NULL);
   }
   
   return(vec);
}



/*
 * sort a U16 array in place using Shell's method
 */
static void
shshsort(U16 *arr, int n)
{
    unsigned i, j, inc;
    unsigned t;
    inc=1;
    do{
        inc *= 3;
        inc++;
    }while(inc <= n);
    do{
        inc /= 3;
        for(i=inc;i<n;i++){
            t = arr[i];
            j=i;
            while(arr[j-inc] > t){
                arr[j] = arr[j-inc];
                j -= inc;
                if(j<inc) break;
            }
            arr[j] = t;
        }
    } while(inc > 1);
}


/*
 * sorts an array using a Shell sort, and find the mean and quartiles.
 *
 * If clip is false, use all the data; otherwise find the quartiles for
 * the main body of the histogram, and reevaluate the mean for the main body.
 *
 * The quartile algorithm assumes that the data are distributed uniformly
 * in the histogram cells, and the quartiles are thus linearly interpolated
 * in the cumulative histogram. This is about as good as one can do with
 * dense histograms, and for sparse ones is as good as anything.
 *
 * This code is taken from photo/src/extract_utils.c
 */
static void
get_quartile_from_array(const void *arr, /* the data values */
			int type,	/* type of data */
			int n,		/* number of points */
			int clip,	/* should we clip histogram? */
			float qt[3],	/* quartiles */
			float *mean,	/* mean (may be NULL) */
			float *sig)	/* s.d. (may be NULL) */
{
    int i;
    int sum;
    U16 *data;				/* a modifiable copy of arr */
    register int np;
    const U16 *p;
    int ldex,udex;
    float fdex;
    float fldex;
    int npass;				/* how many passes? 2 => trim */
    int pass;				/* which pass through array? */
    int cdex;    
    int dcell;
    int dlim = 0;

    shAssert(type == TYPE_U16);
    
    npass = clip ? 2 : 1;

    data = alloca(n*sizeof(U16)); memcpy(data, arr, n*sizeof(U16));
    shshsort(data, n);

    for(pass=0;pass < npass;pass++){
       for(i = 0;i < 3;i++) {
	  fdex = 0.25*(float)((i+1)*n);	/*float index*/
	  cdex = fdex;
	  dcell = data[cdex];
	  ldex = cdex;
	  if(ldex > 0) {
	     while(data[--ldex] == dcell && ldex > 0) continue;
	     /* ldex is now the last index for which data<cdex */
	     
	     if(ldex > 0 || data[ldex] != dcell) {
		/* we stopped before the end or we stopped at the
		 * end but would have stopped anyway, so bump it up; 
		 */
		ldex++;
	     }
	  }
	  /* The value of the cumulative histogram at the left edge of the
	   * dcell cell is ldex; ie exactly ldex values lie strictly below
	   * dcell, and data=dcell BEGINS at ldex.
	   */
	  udex = cdex;
	  while(udex < n && data[++udex] == dcell) continue;
	  /* first index for which data>cdex or the end of the array, 
	   * whichever comes first. This can run off the end of
	   * the array, but it does not matter; if data[n] is accidentally
	   * equal to dcell, udex == n on the next go and it falls out 
	   * before udex is incremented again. */
	  
	  /* now the cumulative histogram at the right edge of the dcell
	   * cell is udex-1, and the number of instances for which the data
	   * are equal to dcell exactly is udex-ldex. Thus if we assume
	   * that the data are distributed uniformly within a histogram
	   * cell, the quartile can be computed:
	   */
	  fldex = ldex; 
	  
	  shAssert(udex != ldex);
	  qt[i] = dcell - 1 + 0.5 + (fdex - fldex)/(float)(udex-ldex);
	  
	  /* The above is all OK except for one singular case: if the
	   * quartile is EXACTLY at a histogram cell boundary (a half-integer) 
	   * as computed above AND the previous histogram cell is empty, the
	   * result is not intuitively correct, though the 'real' answer 
	   * is formally indeterminate even with the unform-population-in-
	   * cells ansatz. The cumulative histogram has a segment of zero
	   * derivative in this cell, and intuitively one would place the
	   * quartile in the center of this segment; the algorithm above
	   * places it always at the right end. This code, which can be
	   * omitted, fixes this case.
	   *
	   * We only have to do something if the quartile is exactly at a cell
	   * boundary; in this case ldex cannot be at either end of the array,
	   * so we do not need to worry about the array boundaries .
	   */
	  if(4*ldex == (i+1)*n) {
	     int zext = dcell - data[ldex-1] - 1;
	     
	     if(zext > 0) {
		/* there is at least one empty cell in the histogram
		 * prior to the first data==dcell one
		 */
		qt[i] -= 0.5*zext;
	     }
	  }
       }

       if(npass == 1) {			/* no trimming to be done */
	  if(sig != NULL) {
	     *sig = IQR_TO_SIGMA*(qt[2] - qt[0]);
	  }
       } else {
	  /*
	   * trim the histogram if possible to the first percentile below
	   * and the +2.3 sigma point above
	   */
	  if(pass==0){
	     /* terminate data array--there must be room */
	     data[n] = 0x7fff;
	     /* trim histogram */
	     ldex = .01*n;		/* index in sorted data array at first
					   percentile */
	     dlim = qt[1] + 2.3*IQR_TO_SIGMA*(qt[2] - qt[0]) + 0.5;
	     if(dlim >= data[n-1] || udex >= n) {  /* off top of data or
						      already at end */
		if(ldex == 0) {
		   if(sig != NULL) {
		      *sig = IQR_TO_SIGMA*(qt[2] - qt[0]);
		   }
		   break;		/* histogram is too small; we're done*/
		}
	     } else {
		/* find the index corresponding to 2.3 sigma; this should be
		 * done by a binary search */
		udex--; 
		while(data[++udex] <= dlim){;}
		n = udex - ldex;
		data = data + ldex;
	     }
	  }else{   /* have trimmed hist and recomputed quartiles */
	     if(sig != NULL) {
		*sig = 0.749*(qt[2]-qt[0]);
	     }
	  }
       }
    }

    if(mean != NULL) {
       sum = 0;
       np = n;
       p = data;
       while(np--){
	  sum += *p++;
       }
       *mean = (float)sum/(float)n;
    }
}    


/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Set a regions with the contents of a vector
 * This replaces either a row or a col of a region with the vector
 * dirn = 1 is replace col, dirn = 0 replaces row
 *
 */
RET_CODE
shVectorSetInRegion(const REGION *reg, /* the region with the data */
		      VECTOR *vec,
		      int row,
		      int col,
		      int dirn) /* 0 to insert row, 1 to insert col */
{
   int i;

   shAssert(reg != NULL);
   shAssert(vec != NULL);
   if (reg->type != TYPE_FL32) {
      shErrStackPush("shVectorSetFromRegion: unsupported type %d",reg->type);
      return(NULL);
   }

   if (dirn) { /* replace a col */
   		if (reg->nrow != vec->dimen) {
      		shErrStackPush("shVectorSetFromRegion: nrows != vec.dimen");
      		return(NULL);
   		}
	   for (i = 0; i < reg->nrow; i++) {
		   reg->rows_fl32[i][col] = vec->vec[i];
	   }
   } else { /* replace a row */
   		if (reg->ncol != vec->dimen) {
      		shErrStackPush("shVectorSetFromRegion: ncols != vec.dimen");
      		return(NULL);
   		}
	   for (i = 0; i < reg->ncol; i++) {
		   reg->rows_fl32[row][i] = vec->vec[i];
	   }
   }
	
  return (SH_SUCCESS);
}

#if !defined(NOTCL)
/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shVPlot
  DESCRIPTION:
  <HTML>
  Plot one VECTOR vs. another VECTOR
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS if successfull
  </HTML>
</AUTO>*/

RET_CODE shVPlot(
		  PGSTATE *pgstate, /* where to plot it */
		  VECTOR *vectorX,       /* vector to use for x */
		  VECTOR *vectorXErr,       /* vector to use for x error */
		  VECTOR *vectorY,       /* vector to use for y */
		  VECTOR *vectorYErr,       /* vector to use for y error*/
		  VECTOR *vectorMask,    /* mask to pick only part of data */
		  VECTOR *vectorSymbol,  /* vector for symbol */
		  VECTOR *vectorColor,   /* vector for color */
		  double xminIn,    /* min value for x */
		  double xmaxIn,    /* max value for x */
		  double yminIn,    /* min value for y */
		  double ymaxIn,    /* max value for y */
		  int xyOptMask /* XMINOPT | XMAXOPT, etc., to use above 
				   values, not calculated ones */ 
		  ) {
  float temp, tempErr;
  float xmin,xmax,xdelta, ymin,ymax,ydelta;

  float *xtempV, *xtemp1, *xtemp2;
  float *ytempV, *ytemp1, *ytemp2;

  int nPoint, iPoint;
  int nUnmaskedPoint, iUnmaskedPoint; 
  int isGood, isMask;
  int symb;
  float tSizeX, tSizeY;

  int icSave, icTemp;
  int lineWidthSave, lineStyleSave;
  int lineWidthTemp, lineStyleTemp;
  int isSymbol, isColor, *symbV=NULL, *colorV=NULL;

  /* always check for argument validity */

  if(vectorX==NULL || vectorY == NULL || pgstate == NULL) {
    shErrStackPush("shVPlot: Illegal argument list");
    return SH_GENERIC_ERROR;
  }

  if (vectorMask == NULL ) { 
    isMask = 0;
  } else {
    isMask = 1;
  }
  if (vectorSymbol == NULL ) { 
    isSymbol = 0;
  } else {
    isSymbol = 1;
  }
  if (vectorColor == NULL ) { 
    isColor = 0;
  } else {
    isColor = 1;
  }
  nPoint = vectorX->dimen; 
  if (nPoint > vectorY->dimen) nPoint = vectorY->dimen;
  /* if (nPoint <= 0) return SH_SUCCESS; */

  if (isMask) {
      nUnmaskedPoint = 0;
      for (iPoint=0; iPoint < nPoint; iPoint++) {
          if (vectorMask->vec[iPoint]) nUnmaskedPoint++;
      }
  } else {
     nUnmaskedPoint = nPoint;
  }

  xmin = xmax = ymin = ymax = 0;
  if (nUnmaskedPoint > 0) {

    /* load up the vectors to be plotted */
    
    xtempV = (float *) shMalloc(nUnmaskedPoint*sizeof(float));
    xtemp1 = (float *) shMalloc(nUnmaskedPoint*sizeof(float));
    xtemp2 = (float *) shMalloc(nUnmaskedPoint*sizeof(float));
    ytempV = (float *) shMalloc(nUnmaskedPoint*sizeof(float));
    ytemp1 = (float *) shMalloc(nUnmaskedPoint*sizeof(float));
    ytemp2 = (float *) shMalloc(nUnmaskedPoint*sizeof(float));
    if (isSymbol) symbV = (int *) shMalloc(nUnmaskedPoint*sizeof(int));
    if (isColor) colorV = (int *) shMalloc(nUnmaskedPoint*sizeof(int));
    
    if(xtempV==NULL || xtemp1==NULL || xtemp2==NULL ||
       ytempV==NULL || ytemp1==NULL || ytemp2==NULL)
      {
	shErrStackPush("Failed to malloc memory for plotting vector");
	
	/* Free anything that might have gotten malloced before the error */
	if (xtempV != NULL)
	  shFree(xtempV);
	if (xtemp1 != NULL)
	  shFree(xtemp1);
	if (xtemp2 != NULL)
	  shFree(xtemp2);
	if (ytempV != NULL)
	  shFree(ytempV);
	if (ytemp1 != NULL)
	  shFree(ytemp1);
	if (ytemp2 != NULL)
	  shFree(ytemp2);
	if (symbV != NULL)
	  shFree(symbV);
	if (colorV != NULL)
	  shFree(colorV);

	return SH_GENERIC_ERROR;
      }
    
    iUnmaskedPoint = 0;
    for (iPoint = 0; iPoint < nPoint; iPoint++) {
      if (isMask!=1) {isGood = 1;} else {isGood = vectorMask->vec[iPoint];}
      if (isGood) {
	if (iUnmaskedPoint==0) {
	  xmin = xmax = vectorX->vec[iPoint];
	  ymin = ymax = vectorY->vec[iPoint];
	}
	xtempV[iUnmaskedPoint] = vectorX->vec[iPoint];
	temp = xtempV[iUnmaskedPoint];
	if (temp > xmax) xmax = temp;
	if (temp < xmin) xmin = temp;
	
	if (vectorXErr != NULL ) {
	  tempErr = vectorXErr->vec[iPoint];
	  temp += tempErr/2.0;
	  xtemp1[iUnmaskedPoint] = temp;
	  if (temp > xmax) xmax = temp;
	  temp -= tempErr;
	  xtemp2[iUnmaskedPoint] = temp;
	  if (temp < xmin) xmin = temp;
	}
	
	ytempV[iUnmaskedPoint] = vectorY->vec[iPoint];
	temp = ytempV[iUnmaskedPoint];
	if (temp > ymax) ymax = temp;
	if (temp < ymin) ymin = temp;
	if (vectorYErr != NULL ) {
	  tempErr = vectorYErr->vec[iPoint];
	  temp += tempErr/2.0;
	  ytemp1[iUnmaskedPoint] = temp;
	  if (temp > ymax) ymax = temp;
	  temp -= tempErr;
	  ytemp2[iUnmaskedPoint] = temp;
	  if (temp < ymin) ymin = temp;
	}
	/* Add 0.4 to get float to int conversion correct */
	if (isSymbol) symbV[iUnmaskedPoint] = vectorSymbol->vec[iPoint]+0.4;
	if (isColor) colorV[iUnmaskedPoint] = vectorColor->vec[iPoint]+0.4;
	iUnmaskedPoint++;
      }
    }
  }
  /* if this is to be a "new" plot, figure out the dimensions of the box and
     draw in the axes */

  if (pgstate->isNewplot == 1) {
    shPgstateNextWindow(pgstate);
    
    /* Figure out whether to use the calculated minima and maxima or if the
       user has entered values */
    if (xyOptMask & XMINOPT)
      xmin = xminIn;               /* User entered a value so use it. */
    if (xyOptMask & XMAXOPT)
      xmax = xmaxIn;               /* User entered a value so use it. */
    if (xyOptMask & YMINOPT)
      ymin = yminIn;               /* User entered a value so use it. */
    if (xyOptMask & YMAXOPT)
      ymax = ymaxIn;               /* User entered a value so use it. */

    /* Give the user a warning if the minimum value is greater than the
       maximum.  It is up to the user to decide what to do about it. */
    if (xmin > xmax)
       shErrStackPush("shVPlot: xmin value is greater than xmax value");
    if (ymin > ymax)
       shErrStackPush("shVPlot: ymin value is greater than ymax value");

    xdelta = xmax-xmin; 
    if (fabs((double) xdelta) < 1e-8) xdelta = 10.0;
    xmax += xdelta/10.0; xmin -= xdelta/10.0;
    
    ydelta = ymax-ymin; 
    if (fabs((double) ydelta) < 1e-8) ydelta = 10.0;
    ymax += ydelta/10.0; ymin -= ydelta/10.0;
    if (pgstate->just == 0) {
      cpgswin(xmin, xmax, ymin, ymax);
    } else {
      cpgwnad(xmin, xmax, ymin, ymax);
    }
    cpgqci(&icSave);
    icTemp = pgstate->icBox;
    cpgsci(icTemp);
    if (pgstate->isTime == 1) {
      cpgtbox(pgstate->xopt, pgstate->xtick, pgstate->nxsub, 
	     pgstate->yopt, pgstate->ytick, pgstate->nysub);
    } else {
      cpgbox(pgstate->xopt, pgstate->xtick, pgstate->nxsub, 
	     pgstate->yopt, pgstate->ytick, pgstate->nysub);
    }
    cpgsci(icSave);
  }

  if (nUnmaskedPoint > 0) {
    /* plot the points OR connect the points with lines */
    if (pgstate->isLine == 0) {
      symb = pgstate->symb;
      cpgqci(&icSave);
      icTemp = pgstate->icMark;
      cpgsci(icTemp);
      if (isSymbol || isColor) {
	for (iPoint = 0; iPoint < nUnmaskedPoint; iPoint++) {
	  if (isSymbol) symb = symbV[iPoint];
	  if (isColor) cpgsci(colorV[iPoint]);
	  cpgpt(1, &(xtempV[iPoint]), &(ytempV[iPoint]), symb);
	}
      } else {
	cpgpt(nUnmaskedPoint, xtempV, ytempV, symb);
      }
      cpgsci(icSave);
    } else {
      cpgqci(&icSave);
      cpgqls(&lineStyleSave);
      cpgqlw(&lineWidthSave);
      icTemp = pgstate->icLine;
      lineStyleTemp = pgstate->lineStyle;
      lineWidthTemp = pgstate->lineWidth;
      cpgsls(lineStyleTemp);
      cpgslw(lineWidthTemp);
      cpgsci(icTemp);
      cpgline(nUnmaskedPoint, xtempV, ytempV);
      cpgsci(icSave);
      cpgsls(lineStyleSave);
      cpgslw(lineWidthSave);
    }
    
    cpgqci(&icSave);
    icTemp = pgstate->icError;
    if (icTemp != 0) { /* don't plot err bars if icError is zero */
      cpgsci(icTemp);
      tSizeX = 1.1;
      tSizeY = 1.1;
      if (isColor) {
	for (iPoint = 0; iPoint < nUnmaskedPoint; iPoint++) {
	  cpgsci(colorV[iPoint]);
	  if (vectorXErr != NULL) 
	    cpgerrx(1, &(xtemp1[iPoint]), &(xtemp2[iPoint]), &(ytempV[iPoint]),
	      tSizeX);
	  if (vectorYErr != NULL)
	    cpgerry(1, &(xtempV[iPoint]), &(ytemp1[iPoint]), &(ytemp2[iPoint]),
	      tSizeY);
	}
      } else {
	if (vectorXErr != NULL) 
	  cpgerrx(nUnmaskedPoint, xtemp1, xtemp2, ytempV, tSizeX);
	if (vectorYErr != NULL)
	  cpgerry(nUnmaskedPoint, xtempV, ytemp1, ytemp2, tSizeY);
      }
      cpgsci(icSave);
    }
    cpgqci(&icSave);
    icTemp = pgstate->icLabel;
    cpgsci(icTemp);
  }
  /* put the text titles */
  if (pgstate->isNewplot == 1) {
    if(!strcmp(vectorX->name,"none")) {
      strcpy(vectorX->name,"");
    }
    if(!strcmp(vectorY->name,"none")) {
      strcpy(vectorY->name,"");
    }
    if(!strcmp(pgstate->plotTitle,"none")) {
      strcpy(pgstate->plotTitle,"");
    }
    cpglab((char*)vectorX->name, 
	   (char*)vectorY->name, pgstate->plotTitle);
  }
  cpgsci(icSave);
  
  /* free up the space we used */
  if (nUnmaskedPoint > 0) {
    shFree(xtempV);
    shFree(xtemp1);
    shFree(xtemp2);
    shFree(ytempV);
    shFree(ytemp1);
    shFree(ytemp2);
    if (symbV != NULL) shFree(symbV);
    if (colorV != NULL) shFree(colorV);
  }
  /* do some bookkeeping in pgstate */
  strcpy(pgstate->plotType, "VECTOR");
  pgstate->hg = NULL;
     
  strncpy(pgstate->xSchemaName, (char*) vectorX->name, HG_SCHEMA_LENGTH);
  pgstate->xSchemaName[HG_SCHEMA_LENGTH-1] = '\0';
  strncpy(pgstate->ySchemaName, (char*) vectorY->name, HG_SCHEMA_LENGTH);
  pgstate->ySchemaName[HG_SCHEMA_LENGTH-1] = '\0';
	
  return (SH_SUCCESS);
}
#endif

/*<AUTO EXTRACT>
  ROUTINE: shVExtreme
  DESCRIPTION:
  <HTML>
  Return the min or the max value in a vector, EXACT values.
  Set char to "min" or "max".  Use the returned values to define a HG.
  </HTML>
  RETURN VALUES:
  <HTML>
  Min or max as VECTOR_TYPE, or 0.0 if error or no min or max
  </HTML>
</AUTO>*/
VECTOR_TYPE shVExtreme(
		       VECTOR *vector, /* the vector of which a limit is sought */
		       VECTOR *vMask, /* use only this part of vector */
		       char *minORmax /* set to min or max to get result */
		       ) {
  unsigned int position, first=1;
  VECTOR_TYPE min=0.0;  
  VECTOR_TYPE max=0.0;
  if (vector != NULL) {
    if(vector->dimen == 0) {  /* test for an empty vector */
      return 0.0;
    }
    if(vMask!=NULL) {              /* is there a mask? */
      for (position=0;
	   position < vector->dimen; position++) {
        if(vMask->vec[position]==1) {  /* count only those with mask == 1 */
	  if (first==1) {
	    min=max=vector->vec[position]; 
	    first = 0; 
	  }
          if (vector->vec[position] > max) max = vector->vec[position];
          if (vector->vec[position] < min) min = vector->vec[position];
        }
      }
    } else {
      for (position=0,min=max=vector->vec[0]; 
	   position < vector->dimen; position++) {	
	if (vector->vec[position] > max) max = vector->vec[position];
	if (vector->vec[position] < min) min = vector->vec[position];
      }
    }
    if (strcmp(minORmax, "min") == 0) return min;
    if (strcmp(minORmax, "max") == 0) return max;
    return 0.0;
  } else {
    return 0.0;
  }
}


/*<AUTO EXTRACT>
  ROUTINE: shVLimit
  DESCRIPTION:
  <HTML>
  Return the min or the max value in a vector WITH 10% buffer.
  Set char to "min" or "max".  Use the returned values to define a HG.
  </HTML>
  RETURN VALUES:
  <HTML>
  Min or max as VECTOR_TYPE, or 0.0 if error or no min or max
  </HTML>
</AUTO>*/
VECTOR_TYPE shVLimit(
		     VECTOR *vector, /* the vector of which a limit is sought */
		     VECTOR *vMask, /* use only this part of vector */
		     char *minORmax /* set to min or max to get result */
		     ) {
  unsigned int position, first=1;
  VECTOR_TYPE min=0.0, max=0.0, delta = 0.0;
  if (vector != NULL) {    
    if(vector->dimen == 0) {  /* test for an empty vector */
      return 0.0;
    }
    if(vMask!=NULL) {              /* is there a mask? */
      for (position=0;
	   position < vector->dimen; position++) {
        if(vMask->vec[position]==1) {  /* count only those with mask == 1 */
	  if (first==1) {
	    min=max=vector->vec[position]; 
	    first = 0; 
	  }
          if (vector->vec[position] > max) max = vector->vec[position];
          if (vector->vec[position] < min) min = vector->vec[position];
        }
      }
    } else {
      for (position=0,min=max=vector->vec[0]; 
	   position < vector->dimen; position++) {	
	if (vector->vec[position] > max) max = vector->vec[position];
	if (vector->vec[position] < min) min = vector->vec[position];
      }
    }    
    delta = max - min; if (delta == 0) {delta = 20.0;}
    min = min - delta/10.0; max = max + delta/10.0;
    if (strcmp(minORmax, "min") == 0) return min;
    if (strcmp(minORmax, "max") == 0) return max;
    return 0.0;
  } else {
    return 0.0;
  }
}


/*<AUTO EXTRACT>
  ROUTINE: shVNameSet
  DESCRIPTION:
  <HTML>
  Sets the name of the vector
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS if successfull SH_GENERIC_ERROR if vector is null
  </HTML>
</AUTO>*/
RET_CODE shVNameSet(
		    VECTOR *vector, /* the vector to set */
		    char* name /* what to name it */
		    ) {
    if(vector == NULL) 
    {
        shErrStackPush("Bad VECTOR info: pointer NULL");
        return SH_GENERIC_ERROR;
    }
    if(name!=NULL)
    {
      strncpy(vector->name, name, VECTOR_NAME_LENGTH);
      vector->name[VECTOR_NAME_LENGTH - 1] = '\0';
    }
    return SH_SUCCESS;
}

/*<AUTO EXTRACT>
  ROUTINE: shVNameGet
  DESCRIPTION:
  <HTML>
  Gets the name of the vector
  </HTML>
  RETURN VALUES:
  <HTML>
  the name of the vector, or NULL if null pointer
  </HTML>
</AUTO>*/
char *shVNameGet(
		    VECTOR *vector /* the vector to get the name from */
		    ) {
  if(vector == NULL) {
    shErrStackPush("Bad VECTOR info: pointer NULL");
    return NULL;
  }
  return vector->name;
}
     



/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shVStatistics
  DESCRIPTION:
  <HTML>
  Return either the mean or sigma of the values in a VECTOR
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS if successfull, and the result in result
  </HTML>
</AUTO>*/

RET_CODE shVStatistics(
			VECTOR* vector, /* IN:  the vector to evaluate */
			VECTOR* vMask, /* IN:  the vector to evaluate */
			char* operation, /* IN:  either "mean" or "sigma" */
			VECTOR_TYPE* result /* OUT:  the answer */
			)
{
  int i, nelem;
  int dimen;				/* == vector->dimen */
  VECTOR_TYPE value = 0.0, mean = 0.0, sqr = 0.0;
  const VECTOR_TYPE *const mask = (vMask == NULL) ? NULL : vMask->vec;
  VECTOR_TYPE *vec;			/* == vector->vec */
  
  if(vector == NULL ) {
    shErrStackPush("Bad VECTOR address: NULL");
    return SH_GENERIC_ERROR;
  }
  
  if(operation == NULL || result == NULL) return SH_SUCCESS;

  dimen = vector->dimen; vec = vector->vec; /* unalias */

  nelem = 0;
  if(mask == NULL) {
     for(i=0; i < dimen; i++) {
	nelem++;
	value += vec[i];
     }
  } else {
     for(i=0; i < dimen; i++) {
	if(mask[i] == 1) {
	   nelem++;
	   value += vec[i];
	}
     }
  }
  
  if (nelem <= 0) {
    shErrStackPush("All values in VECTOR are masked, can't get mean");
    return SH_GENERIC_ERROR;
  }
  
  mean = value/nelem;
  if(strcmp(operation, "mean")==0) {
    *result = mean;
    return SH_SUCCESS;
  }
  
  if(strcmp(operation, "sigma")==0) {
    if (nelem <= 1) {
      shErrStackPush("Not enough elements to do sigma");
      return SH_GENERIC_ERROR;
    }
    
    if(mask == NULL) {
       for(i=0; i < dimen; i++) {
	  value = vec[i] - mean;
	  sqr += value*value;
       }
    } else {
       for(i=0; i < dimen; i++) {
	  if(mask[i] == 1) {
	     value = vec[i] - mean;
	     sqr += value*value;
	  }
       }
    }

    *result = sqrt(sqr/(nelem-1));
    
    return SH_SUCCESS;
  }
  
  shErrStackPush("Unsupported operation %s", operation);
  return SH_GENERIC_ERROR;
}

/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shVMedian
  DESCRIPTION:
  <HTML>
  Find the median of the values in an vector
  </HTML>
  RETURN VALUES:
  <HTML>
  the median value
  </HTML>
</AUTO>*/
VECTOR_TYPE  shVMedian(
                   VECTOR *vector, /* the VECTOR to return the median of */
                   VECTOR *vMask /* use corresponding vector value if  1 */
                   ) {
  VECTOR_TYPE median;
  int nGood = 0;
  int i;
  VECTOR_TYPE *temp = shMalloc(vector->dimen*sizeof(VECTOR_TYPE));
  for (i=0; i<vector->dimen; i++) {
    if ( (vMask == NULL) || (vMask->vec[i] == 1)) {
      temp[nGood] = vector->vec[i];
      nGood++;
    }
  }
  if (nGood>0) {
    qsort(temp, nGood, sizeof(VECTOR_TYPE), VECTOR_TYPECompare);
    if (nGood/2==(nGood-1)/2) { /* is it odd? */
      /* use middle one. */
      median = temp[nGood/2];
    } else { /* it is even */
      /* use avg of middle two */
      median = (temp[nGood/2]+temp[nGood/2-1])/2;
    }
  } else {
    median = 0;
  }
  shFree(temp);
  return median;
}


int VECTOR_TYPECompare(const void *d1, const void *d2) {
  VECTOR_TYPE *v1, *v2;
  v1 = (VECTOR_TYPE *) d1;
  v2 = (VECTOR_TYPE *) d2;
  return (*v1<*v2) ? -1 : (*v1>*v2) ? 1: 0;
}




/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shVSigmaClip
  DESCRIPTION:
  <HTML>
  Sigma clip average of an VECTOR 
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS if successful, results in mean and sqrtVar
  </HTML>
</AUTO>*/

RET_CODE shVSigmaClip(
       VECTOR *vector,          /* vector to use */
       VECTOR *vMask,      /* mask to select part of vector */
       VECTOR_TYPE sigmaClip,   /* sigma clip value */
       unsigned int nIter, /* number of iterations */
       VECTOR_TYPE *mean,        /* return: mean */
       VECTOR_TYPE *sqrtVar)     /* return: sqrt var */
{
     unsigned int iVal, nVal;
     int trigger;
     VECTOR_TYPE value, sum, sum2, delta;

     /* check for argument validity */

     if(vector == NULL || mean == NULL || sqrtVar == NULL)
     {
          shErrStackPush("shVSigmaClip: illegal argument(s)");
          return SH_GENERIC_ERROR;
     }

     if(shVStatistics(vector,vMask,"mean", mean)!=SH_SUCCESS) { 
       return SH_GENERIC_ERROR;
     }

 /*    *mean = (shVLimit(vector, vMask, "max") 
	      + shVLimit(vector, vMask, "min")) / 2.0; */
/* Chris S.  October 24, 2001:  "max" and "min" are not a reliable way to get the initial guess of sigma.
     *sqrtVar = shVLimit(vector, vMask, "max") - shVLimit(vector, vMask, "min");
so do this instead:
*/
     if(shVStatistics(vector,vMask,"sigma", sqrtVar)!=SH_SUCCESS) {
       shErrStackPush("shVSigmaClip: error from shVStatistics");
       return SH_GENERIC_ERROR;
     }
    
     while (nIter > 0 ) 
     {
        nVal=0; sum=0; sum2=0;
        for (iVal = 0; iVal < vector->dimen; iVal++) 
        {
             if(vMask != NULL && vMask->vec[iVal]) continue;        /* WP, don't count masked values */
             value = vector->vec[iVal];
             if (vMask != NULL && vMask->vec[iVal] != 1) value = sqrt(-1-vector->dimen);
             if (isgood(value) && (fabs(value-(*mean)) < sigmaClip*(*sqrtVar))) 
             {
	        sum  += value;
	        sum2 += value*value;
	        nVal++;
             }
        }
       if (nVal == 0) {
	 /* If all of the values are the same, then decleare the mean to be
	    that value and sqrtVar to be 0.0
	    */
	 trigger=0;
	 for (iVal = 0; iVal < vector->dimen; iVal++) {
	   if (vMask != NULL && vMask->vec[iVal]) continue;
	   if (trigger == 0) {
	     value = vector->vec[iVal];
	     trigger = 1;
	   }
	   if (value != vector->vec[iVal]) {
	     trigger = 2; /* There is a different value */
	     break;
	   }
	 }

	 if (trigger == 1) { /* All unmasked values are the same */
	   *mean = value; *sqrtVar = 0.0; return SH_SUCCESS;
	 }
	 *mean = 0.0; *sqrtVar=0.0; 
	 shErrStackPush("shVSigmaClip:  mean and sqrtVar are 0.0");
	 return SH_GENERIC_ERROR;
       }
       *mean = sum/nVal;
       delta = sum2/nVal - (*mean)*(*mean);
       if (delta > 0) *sqrtVar = sqrt(delta);
       else *sqrtVar = 0.0;
       nIter--;
     }
  
     return SH_SUCCESS;
}

     
/***********************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shVToChain
  DESCRIPTION:
  <HTML>
  Copy an vector to a chain
  </HTML>
  RETURN VALUES:
  <HTML>
  chain, or NULL if error
  </HTML>
</AUTO>*/
		   /*int QbutR */			/* do a quick but risky insert */

CHAIN* shVToChain(
		   VECTOR *vector,         /* vector to use */
		   CHAIN *chain,      /* destination chain */
		   char* member       /* member */
		  ) {

      void *item=NULL, *element=NULL;
      const SCHEMA* schema;
      char* dummy;
      TYPE type, firstType;
      TYPE UCHAR  = shTypeGetFromName("UCHAR");
      TYPE CHAR   = shTypeGetFromName("CHAR");
      TYPE USHORT = shTypeGetFromName("USHORT");
      TYPE SHORT  = shTypeGetFromName("SHORT");
      TYPE UINT   = shTypeGetFromName("UINT");
      TYPE INT    = shTypeGetFromName("INT");
      TYPE ULONG  = shTypeGetFromName("ULONG");
      TYPE LONG   = shTypeGetFromName("LONG");
      TYPE FLOAT  = shTypeGetFromName("FLOAT");
      TYPE DOUBLE = shTypeGetFromName("DOUBLE");

      int nItem, iItem;

      if(chain ==NULL || member==NULL || vector == NULL)
      {
           shErrStackPush("shVToChain: Illegal arguments");
           return NULL;
      }

      if(vector->vec == NULL)
      {
           shErrStackPush("VECTOR doesn't have data");
           return NULL;
      }
     
      nItem = shChainSize(chain);

      type = shChainElementTypeGetByPos(chain, HEAD);
      firstType = type;
      schema = (SCHEMA *) shSchemaGet(shNameGetFromType(type));
      if (schema == NULL) 
      {
    	  shErrStackPush("shVToChain:  can not get schema for %s\n",
                   shNameGetFromType(type));
    	  return NULL;
      } 

      for (iItem=0; iItem < nItem; iItem++) {
		item = shChainElementGetByPos(chain, iItem);

		if(item == NULL) {  
	  		shErrStackPush("shVToChain:  error getting element %d",iItem);
	  		return NULL;
		}
		type = shChainElementTypeGetByPos(chain, iItem);
		if (type != firstType) {
	  		shErrStackPush("shVToChain:  error getting value for element %d",
			 iItem);
	  		shErrStackPush("shVToChain:  type=%s not same as firstType=%s",
			 shNameGetFromType(type), schema->name);
	  		return NULL;
		}
		if(shExprEval((SCHEMA *)schema, member, (char *)item, 
		      (char **)&element, &type, &dummy)!=SH_SUCCESS) {
	  		shErrStackPush("shVToChain:  error getting value for element %d",
			 iItem);
	  		return NULL;
		}
	
	
        /* set the element according to returned type */
        if(type==UCHAR) 
	  		*(unsigned char*)element = (unsigned char) vector->vec[iItem];
        else if(type==CHAR)  
	  		*(char*)element = (char) vector->vec[iItem];
        else if(type==SHORT)
	  		*(short *)element = (short) vector->vec[iItem];
        else if(type==USHORT)
	  		*(unsigned short *)element = (unsigned short) vector->vec[iItem];
        else if(type==INT)    
	  		*(int *)element = (int) vector->vec[iItem];
        else if(type==UINT)
	  		*(unsigned int*)element = (unsigned int) vector->vec[iItem];
        else if(type==LONG)
	  		*(long *)element = (long) vector->vec[iItem];
        else if(type==ULONG)
	  		*(unsigned long *)element = (unsigned long) vector->vec[iItem];
        else if(type==DOUBLE)
	  		*(double *)element = (double) vector->vec[iItem];
        else if(type==FLOAT) 
	  		*(float *)element  = (float) vector->vec[iItem];
        else {   
	  		shErrStackPush("shVToChain: unsupported type %s",
			 shNameGetFromType(type));
	  		return NULL;
         }
      }
     return chain;
}

#if !defined(NOTCL)
/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shHgFillFromV
  DESCRIPTION:
  <HTML>
  Put the values in an VECTOR in a HG
  </HTML>
  RETURN VALUES:
  <HTML>
  SH_SUCCESS; or SH_GENERIC_ERROR if there is a problem,
  or vector has no data. 
  </HTML>
</AUTO>*/

RET_CODE shHgFillFromV(
			HG *hg, /* the HG to accept the values */
			VECTOR *vector, /* the VECTOR that is the source of values */
			VECTOR *vMask /* determines what part of the vector will be used */
			) {
  unsigned int position;
  if(vector==NULL) return SH_GENERIC_ERROR;
  if(vector->vec == NULL )
  {
       shErrStackPush("VECTOR has no data");
       return SH_GENERIC_ERROR;
  }
  if(vMask!=NULL)
  {
      if(vMask->vec == NULL )
      {
          shErrStackPush("vMask has no data");
          return SH_GENERIC_ERROR;
      }
  }
  if (vMask == NULL) {
    for (position=0; position<vector->dimen; position++) {
      shHgFill(hg, vector->vec[position], 1.0);
    }
  } else {
    for (position=0; position<vector->dimen; position++) {
      if (vMask->vec[position] == 1) {
	shHgFill(hg, vector->vec[position], 1.0);
      }
    }
  }
  return (SH_SUCCESS);
}



/************************************************************************/
/*<AUTO EXTRACT>
  ROUTINE: shHgNewFromV
  DESCRIPTION:
  <HTML>
  Create a new HG and put the values from a VECTOR into the HG. 
  </HTML>
  RETURN VALUES:
  <HTML>
  Pointer to a hg if successful or NULL if Illegal VECTOR
  </HTML>
</AUTO>*/
HG *shHgNewFromV(
		 VECTOR *vector, /* the VECTOR that will supply values */
		 VECTOR *vMask, /* what part of the vector to use */
		 VECTOR *vWeight, /* weight vector */
		 unsigned int nbin, /* the number of bins to create in HG */
		 VECTOR_TYPE min, /* min calculated if >= max */
                 VECTOR_TYPE max, /* max */
                 char *name       /* name to give hg, vector name otherwise */
                 ) {
  unsigned int position;
  HG *hg=NULL;
  char useName[VECTOR_NAME_LENGTH];
  if(vector==NULL || vector->vec==NULL)
  {
      shErrStackPush("Illegal VECTOR");
      return NULL;
  }
  if(vMask!=NULL) {
    if(vMask->vec == NULL ) {
      shErrStackPush("vMask has no data");
      return NULL;
    }
  }
  strncpy(useName, (name == NULL)? "" : name, VECTOR_NAME_LENGTH);
  useName[VECTOR_NAME_LENGTH-1] ='\0';

/* If you did not get a name then copy the name from the vector */

  if(strlen(useName)==0 && vector->name!=NULL) {
    strncpy(useName, (char*) vector->name, VECTOR_NAME_LENGTH);
    useName[VECTOR_NAME_LENGTH-1] ='\0';
  }
  if (min >= max) {
    min = shVLimit(vector, vMask, "min");
    max = shVLimit(vector, vMask, "max");
  }
  /*printf("in tclHgNewFromV:  tempMin, tempMax %f %f \n", tempMin, tempMax);*/
  hg = shHgNew();
  shHgDefine(hg, useName, "no. entries", useName, min, max, nbin);

  if (vWeight == NULL) {
    if (vMask == NULL) {
      for (position=0; position<vector->dimen; position++) {
	shHgFill(hg, vector->vec[position], 1.0);
      }
    } else {
      for (position=0; position<vector->dimen; position++) {
	if (vMask->vec[position] == 1) {
	  shHgFill(hg, vector->vec[position], 1.0);
	}
      }
    }
  } else {
    if (vMask == NULL) {
      for (position=0; position<vector->dimen; position++) {
	shHgFill(hg, vector->vec[position], vWeight->vec[position]);
      }
    } else {
      for (position=0; position<vector->dimen; position++) {
	if (vMask->vec[position] == 1) {
	  shHgFill(hg, vector->vec[position], vWeight->vec[position]);
	}
      }
    }
  }


  return hg;
}

#endif

