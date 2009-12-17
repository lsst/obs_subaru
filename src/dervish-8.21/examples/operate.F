C
C  Example  subroutine conforming the the survey's standards for FORTRAN
C  code.
C

C  The routines dervish_debug, dervish_error, and dervish_fatal are available
C  to help pass diagnostics to the main part of the interpreter.
C
C  The text of operate.h will be included in the the FORTRAN source. 
C  This illustrates the used of the #include directive to keep
C  the declataion of any common blocks coherent, and to 
C  declare any constants (i.e. PARAMETERS) which should be 
C  visible to C and FORTRAN
C
       REAL FUNCTION OPERATE(VEC, NVEC, OPERATOR, NOPERATOR)
C
#include "operate.h"
       INTEGER NVEC
       REAL     VEC(NVEC)
       CHARACTER*(*) OPERATOR
       INTEGER NOPERATOR

       INTEGER I

       IF (NOPERATOR .ne. 1) 
     &     CALL dervish_fatal('Operate called with bad argument')
C
       OPERATE = 0
       IF (OPERATOR .eq. OP_ADD) THEN
          DO 100 I=1,NVEC
             OPERATE = OPERATE + VEC(I)
100       CONTINUE
       ELSEIF (OPERATOR .eq. OP_MULT) THEN 
          OPERATE=1
          DO 200 I=1,NVEC
             OPERATE = OPERATE * VEC(I)
200       CONTINUE
       ELSE
       CALL dervish_error ('Operate called with illegal operator:')
       ENDIF
       RETURN
       END
