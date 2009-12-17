#include <stdio.h>
#include "operate.h"
#define NVEC 10
static float vec[NVEC] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0 };
main () 
{
   float ret;
   float expected;

   expected = 1.0*2.0*3.0*4.0*5.0*6.0*7.0*8.0*9.0*10.0;
   ret = spOperate (vec, NVEC, OP_MUL);
   if (ret != expected) fprintf (stderr, "bad  result for OP_MUL\n");

   expected = 1.0+2.0+3.0+4.0+5.0+6.0+7.0+8.0+9.0+10.0;
   ret = spOperate (vec, NVEC, OP_ADD);
   if (ret != expected) fprintf (stderr, "bad  result for OP_ADD\n");

   return 0;
}
