/*
 *++
   MODULE tst_transmul - Test Coordinate Transform Multiply Utility

   DESCRIPTION:
     This routine tests the smTransMultiply utility


   AUTHOR:
    J. Frederick Bartlett
    Experimental Astrophysics Department
    Computing Division
    Fermilab

   CREATION DATE: 26-Jul-1994

   MODIFICATIONS:


   AUTHOR:
    J. Frederick Bartlett
    Experimental Astrophysics Department
    Computing Division
    Fermilab

   CREATION DATE: 26-Jul-1994

   MODIFICATIONS:

 *--
*/

#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include "slalib.h"

  RET_CODE smTransComp(TRANS *ta_ptr, TRANS *tb_ptr);

  int main(void)

    {

    TRANS ta, tb, tc, td;

/*  Inititalize the translation transform */

    smTransClear(&ta);
    ta.a = 3.0;
    ta.b = 1.0;
    ta.c = 0.0;
    ta.d = -4.0;
    ta.e = 0.0;
    ta.f = 1.0;


/*  Inititalize the scaling transform */

    smTransClear(&tb);
    tb.a = 0.0;
    tb.b = 0.5;
    tb.c = 0.0;
    tb.d = 0.0;
    tb.e = 0.0;
    tb.f = 4.0;


/*  Inititalize the expected result transform */

    smTransClear(&tc);
    tc.a = 3.0;
    tc.b = 0.5;
    tc.c = 0.0;
    tc.d = -4.0;
    tc.e = 0.0;
    tc.f = 4.0;

/* Premultiply the scaling transform by the translation transform */

    smTransMultiply(&ta, &tb, &td);

/* Check that the results are within rounding error */

/* Wei Peng, normal returns in unix should be 0. So negation
   is added                                         */

    return !(smTransComp(&tc, &td) == 0);

    }

  RET_CODE smTransComp(TRANS *ta_ptr, TRANS *tb_ptr)

    {

    static double THRESH = 1E-5;

    if ((fabs(ta_ptr->a - tb_ptr->a) > THRESH) ||
        (fabs(ta_ptr->b - tb_ptr->b) > THRESH) ||
        (fabs(ta_ptr->c - tb_ptr->c) > THRESH) ||
        (fabs(ta_ptr->d - tb_ptr->d) > THRESH) ||
        (fabs(ta_ptr->e - tb_ptr->e) > THRESH) ||
        (fabs(ta_ptr->f - tb_ptr->f) > THRESH))
      {
      fprintf(stderr, "  Transforms do not match\n");
      return(1);
      }
    else
      return (0);
    }
