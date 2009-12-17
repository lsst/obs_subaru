#ifdef F77
C
C Parameters in the interface for the OPERATE package. part 
C of the SPECT pipeline. 
C
       CHARACTER*1 OP_ADD
       PARAMETER (OP_ADD = '+')
       CHARACTER*1 OP_MULT
       PARAMETER (OP_MULT= '*')
#else  /* ANSI C*/
/* ANSI-C Declarations for the C interface to the OPERATE package
   1) Equivalent ANSI C declarations for the PARAMETERS in the interface
      to the OPERATE PACKAGE. 
   2) Decalartions of the ANSI-C prototypes for the interface to the
      OPERATE package
*/

#define OP_ADD "+"
#define OP_MUL "*"

float spOperate (float * vec, int nVec, char *operator); 
#endif
