/*
 * Code to deal with Bright Objects
 */
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <math.h>
#include "dervish.h"

#include "phDgpsf.h"

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 */
void
phCompositeProfileDel(COMP_PROFILE *cprof) /* COMP_PROFILE to delete */
{
   if(cprof != NULL) {
     shFree(cprof->profs);
   }
   shFree(cprof);
}

