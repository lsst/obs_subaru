   /*
    * <AUTO>
    * FILE: mag.c
    *
    * creation/deletion for MAG structure
    *
    * </AUTO>
    */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dervish.h"
#include "phMag.h"

   /*
    * <AUTO EXTRACT>
    *
    * create a new DGPSF structure.
    *
    * returns:
    *   pointer to new MAG           always
    */

MAG * 
phMagNew(void) 
{

  MAG *new = shMalloc(sizeof(MAG));

  strncpy(new->passBand, "", FILTER_MAXNAME);
  new->mag = -1000;
  new->magErr = 0;

  return(new);

}


   /*
    * <AUTO EXTRACT>
    *
    * delete a MAG structure
    *
    * input:
    *   MAG *mag                     structure to delete
    *
    * output:
    *   none
    *
    */

void
phMagDel(MAG *mag) 
{
   shFree(mag);
}
