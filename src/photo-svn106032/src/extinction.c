   /*
    * <AUTO>
    * FILE: extinction.c
    *
    * Support for the EXTINCTION structure 
    *
    * </AUTO>
    */




#include <stdio.h>
#include <stdlib.h>
#include "dervish.h"
#include "phExtinction.h"

   /*
    * <AUTO EXTRACT>
    *
    * create a new EXTINCTION structure.
    *
    * returns:
    *   pointer to new EXINCTION
    */

EXTINCTION * 
phExtinctionNew(
		int ncolors		/* number of colors in new structure */
		)
{

  int i;
  EXTINCTION *new = shMalloc(sizeof(EXTINCTION));

  new->mjd = -1.0;
  new->ncolors = ncolors;
  new->k = (MAG **)shMalloc(ncolors*sizeof(MAG *));
  for (i=0; i<ncolors; i++) {
      new->k[i] = phMagNew();
  }
  return(new);
}

   /*
    * <AUTO EXTRACT>
    *
    * delete a EXTINCTION structure.
    *
    * input:
    *   EXTINCTION *extinction       structure to delete
    */

void
phExtinctionDel(EXTINCTION *extinction) 
{
  int i;

  if(extinction == NULL) return;
  
  for (i=0; i<extinction->ncolors; i++) 
	shFree(extinction->k[i]);
  shFree(extinction->k);
  shFree(extinction);
}
