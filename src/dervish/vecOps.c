/*<AUTO>
 * FILE: vecOps.c
 *<HTML>
 * This file contains a number of functions to do arithmetic operations
 * on vectors.
 *</HTML>
 *</AUTO>
 */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef NOTCL
#include "shCAssert.h"
#else
#include "dervish.h"
#endif
#include "shCVecExpr.h"
#include "shCGarbage.h"
#include "shCErrStack.h"

/*****************************************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorNew
 *
 * DESCRIPTION:
 * <HTML>
 * Create a vector.
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorNew(int dimen)
{
   VECTOR *vec = (VECTOR*)shMalloc(sizeof(VECTOR));
   vec->dimen = dimen;
   if(dimen <= 0) {
      vec->vec = (VECTOR_TYPE *)shMalloc(1);
   } else {
      vec->vec = (VECTOR_TYPE *)shMalloc(dimen*sizeof(VECTOR_TYPE));
   }
   strcpy(vec->name,"none");

   return(vec);
}

/*<AUTO EXTRACT>
 * ROUTINE: shVectorDel
 *
 * DESCRIPTION:
 * <HTML>
 * Destroy a vector.
 * </HTML>
 * </AUTO>
 */
void
shVectorDel(VECTOR *vec)
{
   if(vec != NULL) {
      if(vec->vec != NULL) shFree((char *)vec->vec);
      shFree((char *)vec);      
   }
}

/*<AUTO EXTRACT>
 * ROUTINE: shVectorResize
 *
 * DESCRIPTION:
 * <HTML>
 * Realloc the vector's values.
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorResize(
	       VECTOR *vec,
	       int dimen
	       )
{
   if(vec == NULL) {
      return(shVectorNew(dimen));
   }
   vec->vec = (VECTOR_TYPE *)shRealloc(vec->vec,dimen*sizeof(VECTOR_TYPE));
   vec->dimen = dimen;

   return(vec);
}

/*****************************************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorPrint
 *
 * DESCRIPTION:
 * <HTML>
 * Print a vector
 * </HTML>
 * </AUTO>
 */
void
shVectorPrint(VECTOR *x)
{
   if(x != NULL) {
      int i;
      for(i = 0;i < x->dimen;i++) {
	 printf("%g%c",x->vec[i],i%8 == 7 ? '\n' : ' ');
      }
      if(i%8 != 0) printf("\n");
   }
}

/*****************************************************************************/
/*
 * Read a logical line from the file, allowing for \ continuation lines
 */
static int line_ctr;			/* line number counter */

static char *
getline(char *line,
	int line_size,
	FILE *fil)
{
   int len;

   if(fgets(line,line_size,fil) == NULL) {
      return(NULL);
   }
/*
 * Check whether the newline was escaped with a \, and if so discard both the
 * \ and the \n and read the next line as a continuation line
 */
   len = strlen(line);
   while(line[len - 2] == '\\' && line[len - 1] == '\n') {
      char *ret = fgets(&line[len - 2],line_size-len+2,fil);
      len += strlen(&line[len]);
      if(ret == NULL) {
	 line[len - 2] = '\n';
	 line[len - 1] = '\0';
      }
   }

   line_ctr++;
   
   if(line[len - 1] != '\n') {
      int c,c_old;
      
      if(len >= line_size - 1) {
	 shError("Line %d is too long",line_ctr);
      } else {
	 shError("Line %d has no terminating newline",line_ctr);
      }

      for(c = '\0';;c_old = c) {	/* read rest of long line */
	 c_old = c;
	 if((c = getc(fil)) == EOF) {
	    break;
	 } else if(c == '\n') {
	    if(c_old != '\\') {
	       break;
	    }
	 }
      }
   }
   line[len - 1] = '\0';

   return(line);
}

/*****************************************************************************/
/*
 * Read a set of vectors from an ascii file
 */
int
shVectorsReadFromFile(FILE *fil,	/* file descriptor to read from */
		      VECTOR **vecs,	/* vectors to read */
		      int *col,		/* from these columns */
		      int nvec,		/* size of vecs and ncol */
		      const char *type,	/* type specifier for desired rows */
		      int nline)	/* max number of lines to read, or 0 */
{
   int c;				/* column counter in line */
   char *endptr = NULL;			/* end of parsed part of line */
   int i,j,k;
   char *line;				/* buffer for one line of file */
   int linesize = 2500;			/* size of line */
   char *lptr;				/* pointer to line */
   int n_elem = 0;			/* number of elements read */
   const float no_value = 1.1e30;	/* a bad data value */
   const int typelen = (type == NULL) ? 0 : strlen(type); /* length of type */
   VECTOR_TYPE value;			/* value to read */
   int verbose = 0;			/* be chatty? */
   unsigned int size;			/* current size of vector->vec.f */
   VECTOR *vtemp;			/* used in sorting */

   shAssert(fil != NULL && vecs != NULL && col != NULL);
/*
 * Sort col and vector arrays.
 * The sort is a straight insertion, fine for a few columns
 */
   for(i = 1;i < nvec;i++) {
      k = col[i];
      vtemp = vecs[i];
      j = i - 1;
      while(j >= 0 && col[j] > k) {
	 col[j+1] = col[j];
	 vecs[j+1] = vecs[j];
	 j--;
      }
      col[j+1] = k;
      vecs[j+1] = vtemp;
   }
/*
 * Allocate space for vectors that we are going to read
 */
   size = 100;
   for(i = 0;i < nvec;i++) {
      if(vecs[i] == NULL) {
	 vecs[i] = shVectorNew(size);
      } else if(vecs[i]->dimen < size) {
	 vecs[i] = shVectorResize(vecs[i],size);
      }
   }

   line = shMalloc(linesize); line_ctr = 0;
/*
 * Finally read data
 */
   while(getline(line,linesize,fil) != NULL) {
      lptr = line;
      while(isspace(*lptr)) lptr++;

      if(type != NULL &&
	 !(strncmp(lptr, type, typelen) == 0 && isspace(lptr[typelen]))) {
	 continue;
      }

      if(*lptr == '\0') {		/* blank line */
	 if(verbose > 0 && nline > 0) { /* upper limit to lines */
	    shError("couldn't read line %d", line_ctr);
	 }
	 nline = line_ctr - 1;
	 goto end_read;			/* break out of nested loop */
      }
      
      for(c = 1,j = 0;;c++) {		/* count through columns */
	 while(isspace(*lptr)) lptr++;

	 if(c == col[j]) {		/* read this column */
	    if(*lptr == 'N' && strncmp(lptr, "NaN", 3) == 0) {
	       value = no_value;
	       while(*lptr != '\0' && !isspace(*lptr)) lptr++;
	    } else {
	       value = strtod(lptr, &endptr);
	       if(lptr == endptr) {	/* not a float */
		  if(nline > 0 && verbose > 0) {
		     shError("couldn't read column %d of line %d",
			     col[j], line_ctr);
		  }
		  nline = line_ctr - 1;
		  goto end_read; /* break out of nested loop */
	       }
	       lptr = endptr;
	    }
	    do {
	       vecs[j++]->vec[n_elem] = value;
	    } while(j < nvec && c == col[j]);
	 } else {
	    if(*lptr == '{') {		/* value's in {} */
	       while(*lptr != '\0' && *lptr != '}') lptr++;
	       if(*lptr != '\0') lptr++;
	    } else if(*lptr == '"') {	/* value's in "" */
	       while(*lptr != '\0' && *lptr != '"') lptr++;
	       if(*lptr != '\0') lptr++;
	    } else {
	       while(*lptr != '\0' && !isspace(*lptr)) lptr++;
	    }
	 }
	 if(j >= nvec) break;
      }

      if(++n_elem >= size) {		/* need more space on vecs */
	 size *= 2;
	 for(j = 0;j < nvec;j++) {
	    if(vecs[j]->dimen < size) { /* may have been preallocated */
	       vecs[j] = shVectorResize(vecs[j], size);
	    }
	 }
      }

      if(nline > 0 && line_ctr >= nline) { /* we've read our last line */
	 break;
      }
   }
   nline = line_ctr;

end_read:		/* Target of two breaks from nested loop above */

   shFree(line);
   if(n_elem == 0) {			/* we didn't read any data */
      shErrStackPush("No lines read");

      return(-1);
   }
/*
 * recover un-wanted storage
 */
   for(j = 0;j < nvec;j++) {
      vecs[j] = shVectorResize(vecs[j],n_elem);
   }

   return(0);
}


/**********************************************************/
/*
 * Check if v1 (and maybe v2) are either of size 1 or the same size.
 *
 * One of the two vectors v1 and v2 is returned as the place to put the
 * answer; the other vector should be freed upon exit from the vec_op routine
 */
static VECTOR *
check_vec(
   VECTOR *v1,			/* vector to operate on */
   VECTOR *v2			/* second operand, NULL if absent */
   )
{
   VECTOR *ans;			/* the answer */
   int dimen = 1;

   if(v1 == NULL || v2 == NULL) return(NULL);

   if(v1->dimen > dimen) {
      dimen = v1->dimen;
   }
   if(v2->dimen > dimen) {
      dimen = v2->dimen;
   }
   
   if(v1->dimen != 1 && v1->dimen != dimen) return(NULL);
   if(v2->dimen != 1 && v2->dimen != dimen) return(NULL);

   ans = (v1->dimen == dimen) ? v1 : v2;
   
   return(ans);
}

static VECTOR *
check_one_vec(VECTOR *v1)			/* vector to operate on */
{
   return(v1 == NULL ? NULL : v1);
}

/*****************************************************************************/
/*
 *
 * Misc operations on vectors
 *
 */
/*<AUTO EXTRACT>
 * ROUTINE: shVectorDo
 *
 * DESCRIPTION:
 * <HTML>
 * Return a vector created by an implicit do loop, e.g.
 * shVectorDo(1,9,2) would return a vector {1 3 5 7 9}.
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorDo(
       VECTOR *x1,
       VECTOR *x2,
       VECTOR *dx
       )
{
   VECTOR *ans;
   int d;
   int i;

   if(x1 == NULL || x2 == NULL || dx == NULL) {
     if (x1!=NULL)  shVectorDel(x1);
     if (x2!=NULL)  shVectorDel(x2);
     if (dx!=NULL)  shVectorDel(dx);
     return(NULL);
   };

   if(x1->dimen != 1 || x2->dimen != 1 || dx->dimen != 1) {
      shErrStackPush("implicit DO values must be scalars");
      shVectorDel(x1); shVectorDel(x2); shVectorDel(dx);
      return(NULL);
   }
      
   d = (x2->vec[0] - x1->vec[0])/dx->vec[0] + 1.0001;
   if(d <= 0) {
      shErrStackPush("implicit DO has dimension %d <= 0",d);
      shVectorDel(x1); shVectorDel(x2); shVectorDel(dx);
      return(NULL);
   }
   ans = shVectorNew(d);
      
   for(i = 0;i < d;i++) {
      ans->vec[i] = x1->vec[0] + i*dx->vec[0];
   }

   shVectorDel(x1); shVectorDel(x2); shVectorDel(dx);
   
   return(ans);
}

/******************************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorIf
 *
 * DESCRIPTION:
 * <HTML>
 * Return those elements of v1 for which v2 is true
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorIf(VECTOR *v1,			/* vector to be trimmed */
	   VECTOR *v2)			/* logical values */
{
   VECTOR *ans;
   int i,j;

   if(v1 == NULL || v2 == NULL) { 
     if (v1!=NULL) shVectorDel(v1);
     if (v2!=NULL) shVectorDel(v2);
     return(NULL);
   }

   ans = v1;
   if(v2->dimen == 1) {
      if(!v2->vec[0]) {
	 ans = shVectorResize(v1,0);
      }
   } else {
      if(v1->dimen != v2->dimen) {
	 shErrStackPush("Logical vector size must match expression in IF");
	 shVectorDel(v1);
	 ans = NULL;
      } else {
	 for(i = j = 0;i < v1->dimen;i++) {
	    if(v2->vec[i]) {
	       ans->vec[j++] = v1->vec[i];
	    }
	 }
	 ans = shVectorResize(ans,j);
      }
   }

   shVectorDel(v2);
   
   return(ans);
}

/******************************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorSubscript
 *
 * DESCRIPTION:
 * <HTML>
 * subscript vector v1 with v2
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorSubscript(
   VECTOR *v1,			/* vector to be indexed */
   VECTOR *v2			/* indexes */
   )
{
   VECTOR *ans;
   int i;
   int ind;				/* index desired */
   int first_err = 1;			/* First error? */

   if(v1 == NULL || v2 == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     if (v2!=NULL)  shVectorDel(v2);
     return(NULL);
   }
   ans = shVectorNew(v2->dimen);
   
   for(i = 0;i < v2->dimen;i++) {
      ind = v2->vec[i];
      if(ind < 0 || ind >= v1->dimen) {
	 if(first_err) {
	    first_err = 0;
	    shErrStackPush("Indices must lie in 0:%d",v1->dimen-1);
	 }
	 ans->vec[i] = 0;
      } else {
	 ans->vec[i] = v1->vec[ind];
      }
   }

   shVectorDel(v1); shVectorDel(v2);

   return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorAbs
 *
 * DESCRIPTION:
 * <HTML>
 * Take the absolute value of a vector
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorAbs(
	VECTOR *v1		/* vector to be absed */
	)
{
   VECTOR *ans = check_one_vec(v1);
   int i;

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     return(NULL);
   }
   
   for(i = 0;i < v1->dimen;i++) {
      ans->vec[i] = fabs(v1->vec[i]);
   }

   return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorAcos
 *
 * DESCRIPTION:
 * <HTML>
 * Take the acos of a vector
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorAcos(
   VECTOR *v1			/* vector to be acosed */
   )
{
   VECTOR *ans = check_one_vec(v1);
   int i,
       first_err = 1;			/* First arithmetic error? */

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     return(NULL);
   }
   
   for(i = 0;i < v1->dimen;i++) {
      if(fabs(v1->vec[i]) <= 1) {
	 ans->vec[i] = acos(v1->vec[i]);
      } else {
	 if(first_err) {
	    first_err = 0;
	    shErrStackPush("Attempt to take acos of %g",v1->vec[i]);
	 }
	 ans->vec[i] = 0;
      }
   }

   return(ans);
}

/********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorAdd
 *
 * DESCRIPTION:
 * <HTML>
 * Add two vectors
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorAdd(
   VECTOR *v1,
   VECTOR *v2			/* vectors to be added */
   )
{
   VECTOR *ans = check_vec(v1,v2);
   int i;

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     if (v2!=NULL)  shVectorDel(v2);
     return(NULL);
   }
   
   if(v1->dimen == 1) {
      for(i = 0;i < v2->dimen;i++) {
         ans->vec[i] = v1->vec[0] + v2->vec[i];
      }
   } else if(v2->dimen == 1) {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = v1->vec[i] + v2->vec[0];
      }
   } else {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = v1->vec[i] + v2->vec[i];
      }
   }

   shVectorDel((ans == v2) ? v1 : v2);
   
   return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorAsin
 *
 * DESCRIPTION:
 * <HTML>
 * Take the asin of a vector
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorAsin(
   VECTOR *v1			/* vector to be asined */
   )
{
   VECTOR *ans = check_one_vec(v1);
   int i;
   int first_err = 1;			/* First arithmetic error? */

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     return(NULL);
   }
   
   for(i = 0;i < v1->dimen;i++) {
      if(fabs(v1->vec[i]) <= 1) {
	 ans->vec[i] = asin(v1->vec[i]);
      } else {
	 if(first_err) {
	    first_err = 0;
	    shErrStackPush("Attempt to take asin of %g",v1->vec[i]);
	 }
	 ans->vec[i] = 0;
      }
   }

   return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorAtan
 *
 * DESCRIPTION:
 * <HTML>
 * Take the atan of a vector
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorAtan(
   VECTOR *v1			/* vector to be ataned */
   )
{
   VECTOR *ans = check_one_vec(v1);
   int i;

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     return(NULL);
   }
   
   for(i = 0;i < v1->dimen;i++) {
      ans->vec[i] = atan(v1->vec[i]);
   }

   return(ans);
}

/********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorAtan2
 *
 * DESCRIPTION:
 * <HTML>
 * Take the atan of a pair of vectors, v1 and v2
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorAtan2(
   VECTOR *v1,
   VECTOR *v2
   )
{
   VECTOR *ans = check_vec(v1,v2);
   int i;

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     if (v2!=NULL)  shVectorDel(v2);
     return(NULL);
   }
   
   if(v1->dimen == 1) {
      for(i = 0;i < v2->dimen;i++) {
         ans->vec[i] = atan2(v2->vec[i],v1->vec[0]);
      }
   } else if(v2->dimen == 1) {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = atan2(v2->vec[0],v1->vec[i]);
      }
   } else {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = atan2(v2->vec[i],v1->vec[i]);
      }
   }

   shVectorDel((ans == v2) ? v1 : v2);   

   return(ans);
}

/******************************************************************/
/*<AUTO EXTRACT>
 *
 * Return a vector consisting of all of v1 followed by all of v2
 */
VECTOR *
shVectorConcat(VECTOR *v1,		/* vectors to */
	       VECTOR *v2)		/*   be concatenated */
{
   VECTOR *ans;

   if(v1 == NULL || v2 == NULL) { 
     if (v1!=NULL) shVectorDel(v1);
     if (v2!=NULL) shVectorDel(v2);
     return(NULL);
   }

   ans = shVectorNew(v1->dimen + v2->dimen);
   memcpy(ans->vec, v1->vec, v1->dimen*sizeof(VECTOR_TYPE));
   memcpy(&ans->vec[v1->dimen], v2->vec, v2->dimen*sizeof(VECTOR_TYPE));

   shVectorDel(v1); shVectorDel(v2);
   
   return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorCos
 *
 * DESCRIPTION:
 * <HTML>
 * Take the cosine of a vector
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorCos(
   VECTOR *v1			/* vector to be cosed */
   )
{
   VECTOR *ans = check_one_vec(v1);
   int i;

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     return(NULL);
   }
   
   for(i = 0;i < v1->dimen;i++) {
      ans->vec[i] = cos(v1->vec[i]);
   }

   return(ans);
}

/********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorDivide
 *
 * DESCRIPTION:
 * <HTML>
 * divide two vectors
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorDivide(
   VECTOR *v1,
   VECTOR *v2
   )
{
   VECTOR *ans = check_vec(v1,v2);
   int i;
   int first_err = 1;			/* First arithmetic error? */

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     if (v2!=NULL)  shVectorDel(v2);
     return(NULL);
   }
   
   if(v1->dimen == 1) {
      for(i = 0;i < v2->dimen;i++) {
         if(v2->vec[i] == 0) {
            if(first_err) {
	       first_err = 0;
               shErrStackPush("Attempt to take divide by 0");
	    }
            ans->vec[i] = 0;
         } else {
            ans->vec[i] = v1->vec[0]/v2->vec[i];
         }
      }
   } else if(v2->dimen == 1) {
      for(i = 0;i < v1->dimen;i++) {
         if(v2->vec[0] == 0) {
            if(first_err) {
	       first_err = 0;
               shErrStackPush("Attempt to take divide by 0");
	    }
            ans->vec[i] = 0;
         } else {
            ans->vec[i] = v1->vec[i]/v2->vec[0];
         }
      }
   } else {
      for(i = 0;i < v1->dimen;i++) {
         if(v2->vec[i] == 0) {
            if(first_err) {
	       first_err = 0;
               shErrStackPush("Attempt to take divide by 0");
	    }
            ans->vec[i] = 0;
         } else {
            ans->vec[i] = v1->vec[i]/v2->vec[i];
         }
      }
   }

   shVectorDel((ans == v2) ? v1 : v2);
   
   return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorExp
 *
 * DESCRIPTION:
 * <HTML>
 * Take the exponential of a vector
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorExp(
   VECTOR *v1			/* vector to be exped */
   )
{
   VECTOR *ans = check_one_vec(v1);
   int i;

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     return(NULL);
   }
   
   for(i = 0;i < v1->dimen;i++) {
      ans->vec[i] = exp(v1->vec[i]);
   }

   return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorInt
 *
 * DESCRIPTION:
 * <HTML>
 * Take the integral part of a vector
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorInt(
   VECTOR *v1			/* vector to be inted */
   )
{
   VECTOR *ans = check_one_vec(v1);
   int i;

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     return(NULL);
   }
   
   for(i = 0;i < v1->dimen;i++) {
      ans->vec[i] = (int)(v1->vec[i]);
   }

   return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 *
 * Evaluate the number of elements of a vector
 */
VECTOR *
shVectorDimen(VECTOR *v1)		/* vector whose size is desired */
{
   VECTOR *ans;
   int dimen;
   
   if(v1 == NULL) {
     return(NULL);
   }
      
   dimen = v1->dimen;
   ans = shVectorResize(v1, 1);
   ans->vec[0] = dimen;

   return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorLg
 *
 * DESCRIPTION:
 * <HTML>
 * Take the lg of a vector
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorLg(
   VECTOR *v1			/* vector to be logged (base 10) */
   )
{
   VECTOR *ans = check_one_vec(v1);
   int i,
       first_err = 1;			/* First arithmetic error? */

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     return(NULL);
   }
   
   for(i = 0;i < v1->dimen;i++) {
      if(v1->vec[i] > 0) {
	 ans->vec[i] = log10(v1->vec[i]);
      } else {
	 if(first_err) {
	    first_err = 0;
	    shErrStackPush("Attempt to take lg of %g",v1->vec[i]);
	 }
	 ans->vec[i] = 0;
      }
   }

   return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorLn
 *
 * DESCRIPTION:
 * <HTML>
 * Take the ln of a vector
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorLn(
   VECTOR *v1			/* vector to be logged (base 10) */
   )
{
   VECTOR *ans = check_one_vec(v1);
   int i,
       first_err = 1;			/* First arithmetic error? */

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     return(NULL);
   }
   
   for(i = 0;i < v1->dimen;i++) {
      if(v1->vec[i] > 0) {
	 ans->vec[i] = log(v1->vec[i]);
      } else {
	 if(first_err) {
	    first_err = 0;
	    shErrStackPush("Attempt to take ln of %g",v1->vec[i]);
	 }
	 ans->vec[i] = 0;
      }
   }

   return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorMultiply
 *
 * DESCRIPTION:
 * <HTML>
 * Multiply two vectors
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorMultiply(
   VECTOR *v1,
   VECTOR *v2			/* vectors to be multiplied */
   )
{
   VECTOR *ans = check_vec(v1,v2);
   int i;

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     if (v2!=NULL)  shVectorDel(v2);
     return(NULL);
   }
   
   if(v1->dimen == 1) {
      for(i = 0;i < v2->dimen;i++) {
         ans->vec[i] = v1->vec[0] * v2->vec[i];
      }
   } else if(v2->dimen == 1) {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = v1->vec[i] * v2->vec[0];
      }
   } else {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = v1->vec[i] * v2->vec[i];
      }
   }

   shVectorDel((ans == v2) ? v1 : v2);
   
   return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorNegate
 *
 * DESCRIPTION:
 * <HTML>
 * negate a vector
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorNegate(
   VECTOR *v1			/* vector to be negated */
   )
{
   VECTOR *ans = check_one_vec(v1);
   int i;

   if(ans == NULL) return(NULL);
   
   for(i = 0;i < v1->dimen;i++) {
      ans->vec[i] = -v1->vec[i];
   }

   return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorPow
 *
 * DESCRIPTION:
 * <HTML>
 * Calculate a power of a vector 
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorPow(
   VECTOR *v1,
   VECTOR *v2
   )
{
   VECTOR *ans = check_vec(v1,v2);
   VECTOR_TYPE base;			/* we want base^index */
   int first_err = 1;		/* First arithmetic error? */
   int i;
   VECTOR_TYPE index;
   int integral;		/* is index integral ? */

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     if (v2!=NULL)  shVectorDel(v2);
     return(NULL);
   }
   
   if(v1->dimen == 1) {
      base = v1->vec[0];
      for(i = 0;i < v2->dimen;i++) {
         index = v2->vec[i];
         integral = (fabs(index) - (int)fabs(index) == 0) ? 1 : 0;

         if(!integral && base < 0) {
            if(first_err) {
	       first_err = 0;
               shErrStackPush("Fractional power of -ve number: %g^%g",base,index);
	    }
            ans->vec[i] = 0;
         } else {
            ans->vec[i] = pow(base,index);
         }
      }
   } else if(v2->dimen == 1) {
      index = v2->vec[0];
      integral = (fabs(index) - (int)fabs(index) == 0) ? 1 : 0;
      for(i = 0;i < v1->dimen;i++) {
         base = v1->vec[i];

         if(!integral && base < 0) {
            if(first_err) {
	       first_err = 0;
               shErrStackPush("Fractional power of -ve number: %g^%g",base,index);
	    }
            ans->vec[i] = 0;
         } else {
            ans->vec[i] = pow(base,index);
         }
      }
   } else {
      for(i = 0;i < v1->dimen;i++) {
         base = v1->vec[i];
         index = v2->vec[i];
         integral = (fabs(index) - (int)fabs(index) == 0) ? 1 : 0;
         if(!integral && base < 0) {
            if(first_err) {
	       first_err = 0;
               shErrStackPush("Fractional power of -ve number: %g^%g",base,index);
	    }
            ans->vec[i] = 0;
         } else {
            ans->vec[i] = pow(base,index);
         }
      }
   }

   shVectorDel((ans == v2) ? v1 : v2);
   
   return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 *
 * Select some elements of a vector
 */
VECTOR *
shVectorSelect(VECTOR *v1,		/* vector to be chosen from */
	       VECTOR *v2)		/* desired elements */
{
   VECTOR *ans;
   int i,j;
   int ind;				/* desired index */

   if(v1 == NULL || v2 == NULL) { 
     if (v1!=NULL) shVectorDel(v1);
     if (v2!=NULL) shVectorDel(v2);
     return(NULL);
   }

   ans = shVectorNew(v2->dimen);
   for(i = j = 0;i < v2->dimen;i++) {
      ind = v2->vec[i];
      if(ind >= 0 && ind < v1->dimen) {
	 ans->vec[j++] = v1->vec[ind];
      } else {
	 shError("Index %d is out of range", ind);
      }
   }
   ans = shVectorResize(ans,j);

   shVectorDel(v1); shVectorDel(v2);
   
   return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 *
 * Overwrite the elements of vec2 with the elements of vec1
 * at the index positions given by index. Index must be the same
 * size as vec1
 */
VECTOR *
shVectorMap(VECTOR *v1,		/* vector with elemets to use for overwriting*/
	       VECTOR *index,		/* index into v2 */
	       VECTOR *v2)		/* vector with elements to be overwritten */
{
	VECTOR *ans;
	int i,j;

	if(v1 == NULL || index == NULL || v2 == NULL) { 
		if (v1!=NULL) shVectorDel(v1);
		if (v2!=NULL) shVectorDel(v2);
		if (index!=NULL) shVectorDel(index);
		return(NULL);
	}

	if(v1->dimen != index->dimen) {
		shErrStackPush("Index must be the same size as v1");
	}

	ans = shVectorNew(v2->dimen);
	for(i = 0; i < v2->dimen; i++) {
 		ans->vec[i] = v2->vec[i];
	}
	for(i = 0; i < v1->dimen; i++) {
		j = index->vec[i];
 		ans->vec[j] = v1->vec[i];
	}

	shVectorDel(v1); shVectorDel(v2); shVectorDel(index);
   
	return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 *
 * Count the number of elements in vector 1 that are less
 * than the value in vector 2. Vector 2 may be either 
 * of size 1 or of size Dimen(vector 2)
 */
VECTOR *
shVectorNtile(VECTOR *v1,		/* vector to count in */
	       VECTOR *v2)		/* values to compare */
{
	VECTOR *ans;
	int i,j;
	double value;				/* desired index */
	int counter;

	if(v1 == NULL || v2 == NULL) { 
		if (v1!=NULL) shVectorDel(v1);
		if (v2!=NULL) shVectorDel(v2);
		return(NULL);
	}

	if(v1->dimen != v2->dimen) {
		if (v2->dimen != 1) {
			shErrStackPush("Vectors must be same size, or vec2 of size 1");
	   }
	}

	ans = shVectorNew(v2->dimen);
	for(i = 0; i < v2->dimen; i++) {
		counter = 0;
 		value = v2->vec[i];
		for(j = 0; j < v1->dimen; j++) {
			if (v1->vec[j] < value) { counter++; }
		}
		ans->vec[i] = counter;
	}

	shVectorDel(v1); shVectorDel(v2);
   
	return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 *
 * Construct a uniform random number distribution
 * between v1 and v2.
 */
VECTOR *
shVectorRand(VECTOR *v1,		/* min vector*/
	       VECTOR *v2)			/* max vector*/
{
	VECTOR *ans;
	int i;
	int size;
	int min,max;
	double random;

	if(v1 == NULL || v2 == NULL) { 
		if (v1!=NULL) shVectorDel(v1);
		if (v2!=NULL) shVectorDel(v2);
		return(NULL);
	}

	if(v1->dimen != v2->dimen) {
		if (v1->dimen != 1 && v2->dimen != 1) {
			shErrStackPush("Vectors must be same size, or of size 1");
	   }
	}

	size = v1->dimen;
	if (v2->dimen > size) {size = v2->dimen;}

	ans = shVectorNew(size);
	for(i = 0; i < size; i++) {
		if (v1->dimen == 1 ) { 
			min = v1->vec[0];
		} else {
			min = v1->vec[i];
		}
		if (v2->dimen == 1 ) { 
			max = v2->vec[0];
		} else {
			max = v2->vec[i];
		}
		if (min >= max) {
			shErrStackPush("shVectorRand: min must be smaller than max, in doubt at index %d: %f %f",i,min,max);
		}
		/* random in (0,1] */
		random= ((double)rand()+1.0) / ((double)RAND_MAX+1.0); 
		random = min + (max-min)*random;

		ans->vec[i] = random;
	}

	shVectorDel(v1); shVectorDel(v2);
   
	return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 *
 * Construct a normal random number distribution
 * of mean v1 and dispersion v2
 */
VECTOR *
shVectorRandN(VECTOR *v1,		/* mean vector*/
	       VECTOR *v2)			/* sigma vector*/
{
	VECTOR *ans;
	int i;
	int size;
	double mean, sigma;
	double r1, r2, delta;

	if(v1 == NULL || v2 == NULL) { 
		if (v1!=NULL) shVectorDel(v1);
		if (v2!=NULL) shVectorDel(v2);
		return(NULL);
	}

	if(v1->dimen != v2->dimen) {
		if (v1->dimen != 1 && v2->dimen != 1) {
			shErrStackPush("Vectors must be same size, or of size 1");
	   }
	}

	size = v1->dimen;
	if (v2->dimen > size) {size = v2->dimen;}

	ans = shVectorNew(size);
	for(i = 0; i < size; i++) {
		if (v1->dimen == 1 ) { 
			mean = v1->vec[0];
		} else {
			mean = v1->vec[i];
		}
		if (v2->dimen == 1 ) { 
			sigma = v2->vec[0];
		} else {
			sigma = v2->vec[i];
		}
		/* random in (0,1] */
		r1 = (double) (rand()) / (double) (RAND_MAX);
		/* r2 is not allowed to be zero */
		r2=0;
		do {
			r2 = (double) (rand()) / (double) (RAND_MAX);
		} while (r2 == 0);
		delta = sin(3.14159*2.0*r1) * sqrt(-2.0*log(r2));
		ans->vec[i] = mean + delta*sigma;

	}

	shVectorDel(v1); shVectorDel(v2);
   
	return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorSin
 *
 * DESCRIPTION:
 * <HTML>
 * Take the sine of a vector
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorSin(
   VECTOR *v1			/* vector to be sined */
   )
{
   VECTOR *ans = check_one_vec(v1);
   int i;

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     return(NULL);
   }
  
   for(i = 0;i < v1->dimen;i++) {
      ans->vec[i] = sin(v1->vec[i]);
   }

   return(ans);
}

/********************************************************/
/*
 * This is a shell sort
 */
#define LN2I 1.442695022		/* 1/ln(e) */
#define TINY 1e-5

static void
sort_flt_inds(VECTOR_TYPE *vec,		/* array of vectors to sort */
	      int *ind,			/* index array for sorted vectors */
	      int dimen)		/* dimension of vector */
{
   int i,j,m,n,				/* counters */
       lognb2,				/* (int)(log_2(dimen)) */
       temp;

   if(dimen <= 0) return;
   for(i = 0;i < dimen;i++) ind[i] = i;

   lognb2 = log((double)dimen)*LN2I + TINY;	/* ~ log_2(dimen) */
   m = dimen;
   for(n = 0;n < lognb2;n++) {
      m /= 2;
      for(j = m;j < dimen;j++) {
         i = j - m;
	 temp = ind[j];
	 while(i >= 0 && vec[temp] < vec[ind[i]]) {
	    ind[i + m] = ind[i];
	    i -= m;
	 }
	 ind[i + m] = temp;
      }
   }
}

/*********************************************************/
/*
 * Do the sort directly 
 */
static void
sort_flt(VECTOR_TYPE *vec,		/* array of vectors to sort */
	 int dimen)			/* dimension of vector */
{
   VECTOR_TYPE temp;
   int i,j,m,n,				/* counters */
       lognb2;				/* (int)(log_2(dimen)) */

   if(dimen <= 0) return;
   lognb2 = log((double)dimen)*LN2I + TINY;	/* ~ log_2(dimen) */
   m = dimen;
   for(n = 0;n < lognb2;n++) {
      m /= 2;
      for(j = m;j < dimen;j++) {
         i = j - m;
	 temp = vec[j];
	 while(i >= 0 && temp < vec[i]) {
	    vec[i + m] = vec[i];
	    i -= m;
	 }
	 vec[i + m] = temp;
      }
   }
}

/*<AUTO EXTRACT>
 *
 * Take vector v1; if v2 is present, set its values to be indices to sort
 * other vectors the same way (using vectorExprEval select<index>)
 */
VECTOR *
shVectorSort(VECTOR *v1,
	     VECTOR *v2)
{
   VECTOR *ans;

   if(v1 == NULL) {
      shVectorDel(v2);
      return(NULL);
   }

   if(v2 == NULL) {
      sort_flt(v1->vec, v1->dimen);
      ans = v1;
   } else {
      int i;
      int *ind;

      if(v1->dimen != v2->dimen) {
	 shErrStackPush("Sort: Both vectors must have same dimension");
	 shVectorDel(v1);
	 return(NULL);
      }

      ind = shMalloc(v2->dimen*sizeof(VECTOR_TYPE));
      sort_flt_inds(v2->vec, ind, v2->dimen);
      if(v1 == v2) {
	 for(i = 0; i < v2->dimen; i++) {
	    v2->vec[i] = ind[i];
	 }
      } else {
	 for(i = 0; i < v1->dimen; i++) {
	    v2->vec[i] = v1->vec[ind[i]];
	 }
      }
      shFree(ind);

      if(v2 != v1) {
	 shVectorDel(v1);
      }
      ans = v2;
   }

   return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorSqrt
 *
 * DESCRIPTION:
 * <HTML>
 * Take the sqrt of a vector
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorSqrt(
   VECTOR *v1			/* vector to be sqrted */
   )
{
   VECTOR *ans = check_one_vec(v1);
   int i;
   int first_err = 1;			/* First arithmetic error? */

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     return(NULL);
   }
   
   for(i = 0;i < v1->dimen;i++) {
      if(v1->vec[i] >= 0) {
	 ans->vec[i] = sqrt(v1->vec[i]);
      } else {
	 if(first_err) {
	    first_err = 0;
	    shErrStackPush("Attempt to take sqrt of %g",v1->vec[i]);
	 }
	 ans->vec[i] = 0;
      }
   }

   return(ans);
}

/********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorSubtract
 *
 * DESCRIPTION:
 * <HTML>
 * Subtract two vectors
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorSubtract(
   VECTOR *v1,
   VECTOR *v2			/* vectors to be subtracted */
   )
{
   VECTOR *ans = check_vec(v1,v2);
   int i;

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     if (v2!=NULL)  shVectorDel(v2);
     return(NULL);
   }
   
   if(v1->dimen == 1) {
      for(i = 0;i < v2->dimen;i++) {
         ans->vec[i] = v1->vec[0] - v2->vec[i];
      }
   } else if(v2->dimen == 1) {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = v1->vec[i] - v2->vec[0];
      }
   } else {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = v1->vec[i] - v2->vec[i];
      }
   }

   shVectorDel((ans == v2) ? v1 : v2);

   return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 *
 * Evaluate the sum of the elements of a vector
 */
VECTOR *
shVectorSum(VECTOR *v1)			/* vector to be summed */
{
   VECTOR *ans;
   int i;
   double sum;

   if(v1 == NULL) {
     return(NULL);
   }
   
   sum = 0;
   for(i = 0;i < v1->dimen;i++) {
      sum += v1->vec[i];
   }
   shVectorDel(v1);
   
   ans = shVectorNew(1);
   ans->vec[0] = sum;

   return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorTan
 *
 * DESCRIPTION:
 * <HTML>
 * Take the tangent of a vector
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorTan(
   VECTOR *v1			/* vector to be tanned */
   )
{
   VECTOR *ans = check_one_vec(v1);
   int i;

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     return(NULL);
   }
   
   for(i = 0;i < v1->dimen;i++) {
      ans->vec[i] = tan(v1->vec[i]);
   }

   return(ans);
}

/*****************************************************************************/
/*
 * bitwise operations
 */

/********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorLAnd
 *
 * DESCRIPTION:
 * <HTML>
 * return v1 &amp; v2
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorLAnd(VECTOR *v1,
	     VECTOR *v2)		/* vectors to be ANDed together */
{
   VECTOR *ans = check_vec(v1,v2);
   int i;

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     if (v2!=NULL)  shVectorDel(v2);
     return(NULL);
   }
   
   if(v1->dimen == 1) {
      for(i = 0;i < v2->dimen;i++) {
         ans->vec[i] = (long)v1->vec[0] & (long)v2->vec[i];
      }
   } else if(v2->dimen == 1) {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = (long)v1->vec[i] & (long)v2->vec[0];
      }
   } else {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = (long)v1->vec[i] & (long)v2->vec[i];
      }
   }

   shVectorDel((ans == v2) ? v1 : v2);

   return(ans);
}

/********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorLOr
 *
 * DESCRIPTION:
 * <HTML>
 * return v1 | v2
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorLOr(VECTOR *v1,
	    VECTOR *v2)			/* vectors to be ORd together */
{
   VECTOR *ans = check_vec(v1,v2);
   int i;

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     if (v2!=NULL)  shVectorDel(v2);
     return(NULL);
   }
   
   if(v1->dimen == 1) {
      for(i = 0;i < v2->dimen;i++) {
         ans->vec[i] = (long)v1->vec[0] | (long)v2->vec[i];
      }
   } else if(v2->dimen == 1) {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = (long)v1->vec[i] | (long)v2->vec[0];
      }
   } else {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = (long)v1->vec[i] | (long)v2->vec[i];
      }
   }

   shVectorDel((ans == v2) ? v1 : v2);

   return(ans);
}

/*****************************************************************************/
/*
 * logical operations
 */

/********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorAnd
 *
 * DESCRIPTION:
 * <HTML>
 * return v1 &amp;&amp; v2
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorAnd(
       VECTOR *v1,
       VECTOR *v2			/* vectors to be tested */
       )
{
   VECTOR *ans = check_vec(v1,v2);
   int i;

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     if (v2!=NULL)  shVectorDel(v2);
     return(NULL);
   }
   
   if(v1->dimen == 1) {
      for(i = 0;i < v2->dimen;i++) {
         ans->vec[i] = v1->vec[0] && v2->vec[i];
      }
   } else if(v2->dimen == 1) {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = v1->vec[i] && v2->vec[0];
      }
   } else {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = v1->vec[i] && v2->vec[i];
      }
   }

   shVectorDel((ans == v2) ? v1 : v2);

   return(ans);
}

/*****************************************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorEq
 *
 * DESCRIPTION:
 * <HTML>
 * are two vectors equal (element by element)
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorEq(
	   VECTOR *v1,
	   VECTOR *v2			/* vectors to be tested */
	   )
{
   VECTOR *ans = check_vec(v1,v2);
   int i;

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     if (v2!=NULL)  shVectorDel(v2);
     return(NULL);
   }
   
   if(v1->dimen == 1) {
      for(i = 0;i < v2->dimen;i++) {
         ans->vec[i] = v1->vec[0] == v2->vec[i];
      }
   } else if(v2->dimen == 1) {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = v1->vec[i] == v2->vec[0];
      }
   } else {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = v1->vec[i] == v2->vec[i];
      }
   }

   shVectorDel((ans == v2) ? v1 : v2);

   return(ans);
}

/********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorGe
 *
 * DESCRIPTION:
 * <HTML>
 * is v1 >= v2?
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorGe(
       VECTOR *v1,
       VECTOR *v2			/* vectors to be tested */
       )
{
   VECTOR *ans = check_vec(v1,v2);
   int i;   

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     if (v2!=NULL)  shVectorDel(v2);
     return(NULL);
   }
   
   if(v1->dimen == 1) {
      for(i = 0;i < v2->dimen;i++) {
         ans->vec[i] = v1->vec[0] >= v2->vec[i];
      }
   } else if(v2->dimen == 1) {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = v1->vec[i] >= v2->vec[0];
      }
   } else {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = v1->vec[i] >= v2->vec[i];
      }
   }

   shVectorDel((ans == v2) ? v1 : v2);

   return(ans);
}

/********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorGt
 *
 * DESCRIPTION:
 * <HTML>
 * is v1 > v2?
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorGt(
       VECTOR *v1,
       VECTOR *v2			/* vectors to be tested */
       )
{
   VECTOR *ans = check_vec(v1,v2);
   int i;

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     if (v2!=NULL)  shVectorDel(v2);
     return(NULL);
   }
   
   if(v1->dimen == 1) {
      for(i = 0;i < v2->dimen;i++) {
         ans->vec[i] = v1->vec[0] > v2->vec[i];
      }
   } else if(v2->dimen == 1) {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = v1->vec[i] > v2->vec[0];
      }
   } else {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = v1->vec[i] > v2->vec[i];
      }
   }

   shVectorDel((ans == v2) ? v1 : v2);

   return(ans);
}

/********************************************************************/

/*<AUTO EXTRACT>
 * ROUTINE: shVectorLternary
 *
 * DESCRIPTION:
 * <HTML>
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorLternary(
	     VECTOR *logical,
	     VECTOR *expr1,
	     VECTOR *expr2
	     )
{
   VECTOR *ans;
   int i;

   if(logical == NULL) {
     if (expr1!=NULL)  shVectorDel(expr1);
     if (expr2!=NULL)  shVectorDel(expr2);
     return(NULL);
   }

   if(logical->dimen == 1) {
     if (logical->vec[0]) {
       shVectorDel(logical);
       if (expr2!=NULL)  shVectorDel(expr2);
       return expr1;
     } else {
       shVectorDel(logical);
       if (expr1!=NULL)  shVectorDel(expr1);
       return expr2;
     }
   }

   if(expr1 == NULL || expr2 == NULL) {
     if (expr1!=NULL)  shVectorDel(expr1);
     if (expr2!=NULL)  shVectorDel(expr2);
     return(NULL);
   }

   if(expr1->dimen == 1) {
      if(expr2->dimen == 1) {
	 ans = shVectorNew(logical->dimen);
	 for(i = 0;i < logical->dimen;i++) {
	    ans->vec[i] = (logical->vec[i]) ? expr1->vec[0] : expr2->vec[0];
	 }
      } else {
	 if(logical->dimen != expr2->dimen) {
	    shErrStackPush("Expr following : in ?: has wrong dimension");
	    shVectorDel(logical); shVectorDel(expr1); shVectorDel(expr2);
	    return(NULL);
	 }
	 ans = shVectorNew(logical->dimen);
	 for(i = 0;i < logical->dimen;i++) {
	    ans->vec[i] = (logical->vec[i]) ? expr1->vec[0] : expr2->vec[i];
	 }
      }
   } else {
      if(logical->dimen != expr1->dimen) {
	 shErrStackPush("Expr following ? in ?: has wrong dimension");
	 shVectorDel(logical); shVectorDel(expr1); shVectorDel(expr2);
	 return(NULL);
      }
      if(expr2->dimen == 1) {
	 ans = shVectorNew(logical->dimen);
	 for(i = 0;i < logical->dimen;i++) {
	    ans->vec[i] = (logical->vec[i]) ? expr1->vec[i] : expr2->vec[0];
	 }
      } else {
	 if(logical->dimen != expr2->dimen) {
	    shErrStackPush("Expr following : in ?: has wrong dimension");
	    shVectorDel(logical); shVectorDel(expr1); shVectorDel(expr2);
	    return(NULL);
	 }
	 ans = shVectorNew(logical->dimen);
	 for(i = 0;i < logical->dimen;i++) {
	    ans->vec[i] = (logical->vec[i]) ? expr1->vec[i] : expr2->vec[i];
	 }
      }
   }

   shVectorDel(logical); shVectorDel(expr1); shVectorDel(expr2);

   return(ans);
}

/***********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorNot
 *
 * DESCRIPTION:
 * <HTML>
 * find the logical not of a vector
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorNot(
	VECTOR *v1			/* vector to be !ed */
	)
{
   VECTOR *ans = check_one_vec(v1);
   int i;

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     return(NULL);
   }
   
   for(i = 0;i < v1->dimen;i++) {
      ans->vec[i] = !v1->vec[i];
   }

   return(ans);
}

/********************************************************/
/*<AUTO EXTRACT>
 * ROUTINE: shVectorOr
 *
 * DESCRIPTION:
 * <HTML>
 * return v1 || v2
 * </HTML>
 * </AUTO>
 */
VECTOR *
shVectorOr(
       VECTOR *v1,
       VECTOR *v2			/* vectors to be tested */
       )
{
   VECTOR *ans = check_vec(v1,v2);
   int i;

   if(ans == NULL) {
     if (v1!=NULL)  shVectorDel(v1);
     if (v2!=NULL)  shVectorDel(v2);
     return(NULL);
   }
   
   if(v1->dimen == 1) {
      for(i = 0;i < v2->dimen;i++) {
         ans->vec[i] = v1->vec[0] || v2->vec[i];
      }
   } else if(v2->dimen == 1) {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = v1->vec[i] || v2->vec[0];
      }
   } else {
      for(i = 0;i < v1->dimen;i++) {
         ans->vec[i] = v1->vec[i] || v2->vec[i];
      }
   }

   shVectorDel((ans == v2) ? v1 : v2);

   return(ans);
}
