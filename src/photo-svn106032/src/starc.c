/*
 * <AUTO>
 *
 * FILE: starc.c
 *
 * DESCRIPTION:
 * Support for the STARC structure 
 *
 * </AUTO>
 */


#include <stdio.h>
#include <stdlib.h>
#include "dervish.h"
#include "phStarc.h"

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * Create a new STARC and all associated memory. 
 *
 * return: STARC * to new structure          
 */

STARC * 
phStarcNew(
   int ncolors           /* I: number of STAR1s in this STARC */
   ) 
{

  int i;
  static int id = 0;

  STARC *new = shMalloc(sizeof(STARC));

  new->id = id++;
  new->ra = new->dec = -10000;
  new->raErr = new->decErr = 0;
  new->mjd = -1.0;
  new->ncolors = ncolors;
  new->mag = (MAG **)shMalloc(ncolors*sizeof(MAG *));
  for (i=0; i<ncolors; i++) {
      new->mag[i] = phMagNew();
  }

  return(new);

}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * delete the given STARC.
 */

void
phStarcDel(
   STARC *starc            /* I: structure to be deleted */
   )
{

  int i;

  for (i=0; i<starc->ncolors; i++) {
      phMagDel(starc->mag[i]);
  }
  shFree(starc->mag);
  shFree(starc);
}
