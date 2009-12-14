/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* undigitize.c		undigitize H-transform
 *
 * Programmer: R. White		Date: 9 May 1991
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "ftcl.h"
#include "dervish.h"
#include "region.h"
#include "math.h"
#include "dervish_msg_c.h"
#include "errStack.h"
#include "phStructs.h"
#include "phCompUtils.h"

#define COMP_DEBUG 10

extern int
undigitize(int a[], int nx, int ny, int scale)
{
  int *p;
  
  /*
   * multiply by scale
   */
  if (scale <= 1) return(SH_SUCCESS);
  for (p=a; p <= &a[nx*ny-1]; p++) *p = (*p)*scale;
  return(SH_SUCCESS);
}
