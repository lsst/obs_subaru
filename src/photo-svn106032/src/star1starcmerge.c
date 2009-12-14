/*
 * <AUTO>
 *
 * FILE: star1starcmerge.c
 *
 * DESCRIPTION:
 * Support for the STAR1STARCMERGE structure 
 *
 * </AUTO>
 */


#include <stdio.h>
#include <stdlib.h>
#include "dervish.h"
#include "phStar1starcmerge.h"

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * Create a new STAR1STARCMERGE structure, return a pointer to it.
 *
 * return: STAR1STARCMERGE * to new structure
 */

STAR1STARCMERGE * 
phStar1starcmergeNew(void) 
{

  STAR1STARCMERGE *new = shMalloc(sizeof(STAR1STARCMERGE));

  new->star1 = NULL;
  new->starc = NULL;
  new->starcMagPos = -1;
  new->starcRefMagPos = -1;
  new->sep = -1.;
  new->mjd = -1;
  new->airmass = -1;
  new->flux20 = -1;
  return(new);
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * Delete the given STAR1STARCMERGE structure.
 */
void
phStar1starcmergeDel(
   STAR1STARCMERGE *star1starcmerge     /* I: structure to be deleted */
   ) 
{
   shFree(star1starcmerge);
}
