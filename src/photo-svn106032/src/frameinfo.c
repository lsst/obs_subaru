   /*
    * <AUTO>
    * FILE: frameinfo.c
    *
    * Support for the Frameinfo structure 
    *
    * </AUTO>
    */

#include <stdio.h>
#include <stdlib.h>
#include "dervish.h"
#include "phFrameinfo.h"

   /*
    * <AUTO EXTRACT>
    *
    * create a new FRAMEINFO structure.
    *
    * returns:
    *   pointer to new FRAMEINFO     always
    */

FRAMEINFO * 
phFrameinfoNew(void) 
{

  FRAMEINFO *new = shMalloc(sizeof(FRAMEINFO));

  new->field = -1;
  new->airmass = -1.;
  new->mjd = -1.0;

  return(new);
}

   /*
    * <AUTO EXTRACT>
    *
    * delete a FRAMEINFO structure
    *
    * input:
    *   FRAMEINFO *frameinfo         structure to delete
    */
void
phFrameinfoDel(FRAMEINFO *frameinfo) 
{
   shFree(frameinfo);
}
