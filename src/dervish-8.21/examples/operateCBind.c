/*
 * F77 Template .c
 * Implement an ANSI C Binding to the OPERATE package.
 * according to survey standards.
 *
 * We must make an input layer because it is not possible 
 * to make robust assumption about the exact manner that 
 * C and call FORTRAN. For example, some systems do not use an underscore,
 * Others append it to the front. This can be dealt with using the 
 * C preprocessor, but we must confine this sort of re-definition
 * to a clean layer of code.  
 * Since we must write the interface layer, we ought to do it
 * well. Scalars that are input only should be passed by value
 * INTO the c-binding, and then passed by REFERENCE into F77.
 *
 * Passing multi-dimensional arrays is more problematic.  
 */

#include "operate.h"
#include "string.h"		/* for strlen */
/* Declare that there prototypes for the calls to f77 code
   These are _NOT_ put in an .h file because the shall never be needed
   outside this file. The __cplusplus is boilerplate. While essential for
   the survey, we will not explain. Rather, be happy you work in FORTRAN :-)
*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus*/
float operate_(float *, int *, char *, int*, int);
#ifdef __cplusplus
}
#endif /* __cplusplus*/

/* The actual routine C programmers will see. Note the prefix guarantees
   that this binding won't clash with f77 which addend no underscores at all.
   Pipeline coders will have such a prefix at their disposal.
*/
float spOperate (float *vec, int nVec, char *operator) 
{
   int len = strlen(operator);
   /*All survey comilation environments happen to use append a trailing
     underscore in the early 1990's, so we will not actuall conditionalize
     the compilation 
   */
    /* Pass vaues by reference, add extra string length, passed by value 
       as extra parameters.
    */
   return operate_ (vec, &nVec, operator,  &len, len);
}




