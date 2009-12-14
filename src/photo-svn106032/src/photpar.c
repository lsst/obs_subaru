/*
 * <AUTO>
 *
 * FILE: photpar.c
 *
 * DESCRIPTION:
 * Support for the PHOTPAR structure 
 *
 * </AUTO>
 */

#include <stdio.h>
#include <stdlib.h>
#include "dervish.h"
#include "phPhotpar.h"

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * create a new PHOTPAR, set all fields to -1.
 *
 * return: 
 *   PHOTPAR * to new structure
 */

PHOTPAR * 
phPhotparNew(void) 
{

  PHOTPAR *new;

  new = shMalloc(sizeof(PHOTPAR));
  new->magZero = -1;
  new->extinction = -1;
  new->magZeroError = -1;
  new->extinctionError = -1;

  return(new);
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * delete the given PHOTPAR structure.
 */

void
phPhotparDel(
   PHOTPAR *photpar           /* I: structure to be deleted */
   ) 
{

  if (photpar == NULL) return;
  
  shFree(photpar);
}
