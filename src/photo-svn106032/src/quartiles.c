
/*
 * <AUTO>
 *
 * FILE: quartiles.c
 *
 * DESCRIPTION:
 * Support for QUARTILESs
 *
 * </AUTO>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dervish.h"
#include "phQuartiles.h"
#include "phCorrectFrames_p.h"

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * Create a new QUARTILES structure, with the given number of rows and cols.
 * Create new TYPE_S32 regions for each quartile's data.
 *
 * return: QUARTILES * to new structure
 */

QUARTILES *
phQuartilesNew(
   int nrow,                  /* I: number of rows in new quartiles */
   int ncol                   /* I: number of cols in new quartiles */
   )
{
   QUARTILES *quartiles = (QUARTILES *) shMalloc(sizeof(QUARTILES));
   int i;

   quartiles->ntilesPercentiles[0] = 25;
   quartiles->ntilesPercentiles[1] = 50;
   quartiles->ntilesPercentiles[2] = 75;
   quartiles->ntilesPercentiles[3] = 0; /* This is the mode from DA */
   quartiles->nr = nrow;
   quartiles->nc = ncol;
   quartiles->tscal = 0;

   for (i = 0; i < 4; i++) {
      quartiles->data[i] = shRegNew("quartile",nrow,ncol,TYPE_S32);
   }

   return (quartiles);
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * Delete the given QUARTILES structure and all its associated regions.
 */
void
phQuartilesDel(
   QUARTILES *quartiles          /* I: structure to delete */
   )
{
   int i;
   REGION *reg;

   if (quartiles == NULL) return;

   if (quartiles->data != NULL) {
      for (i = 0; i < 4; i++) {
        reg = quartiles->data[i];
        shRegDel(reg);
      }
   }
   shFree((char *) quartiles);
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * This reads data from TBLCOL to QUARTILES - the regions in quartiles
 * must already be of the correct size.  The function translates one
 * row of data at a time.
 *
 * return: SH_SUCCESS          if all goes well
 *         SH_GENERIC_ERROR    if not
 */

RET_CODE
phTblcolToQuartiles(
   TBLCOL *tblcol,               /* I: read data from this TBLCOL */
   QUARTILES *quartiles,         /* O: place data into this QUARTILES */
   int row                       /* I: place data into this row of QUARTILES */
   ) 
{
  TBLFLD *tblFld;
  ARRAY *array;
  int tblFldIdx, nbytes, tshift;
  shAssert(tblcol->rowCnt == quartiles->nc);
  shAssert(row<=quartiles->nr);

  shHdrGetInt(&tblcol->hdr,"TSHIFT",&tshift);
  quartiles->tscal = tshift;

  /* This code puts the tblcol from a quartiles fits file of one frame
  into the appropriate rows of the quartiles structure */


  if (shTblFldLoc(tblcol, -1, "Q1", 0, SH_CASE_INSENSITIVE, &array, 
    &tblFld, &tblFldIdx) != SH_SUCCESS) {
    return SH_GENERIC_ERROR;
  }
  nbytes = array->data.size * array->dim[0];
  memcpy((void *)quartiles->data[0]->rows_s32[row],(void*)array->arrayPtr,nbytes);

  if (shTblFldLoc(tblcol, -1, "Q2", 0, SH_CASE_INSENSITIVE, &array, 
    &tblFld, &tblFldIdx) != SH_SUCCESS) {
    return SH_GENERIC_ERROR;
  }
  nbytes = array->data.size * array->dim[0];
  memcpy((void *)quartiles->data[1]->rows_s32[row],(void*)array->arrayPtr,nbytes);

  if (shTblFldLoc(tblcol, -1, "Q3", 0, SH_CASE_INSENSITIVE, &array, 
    &tblFld, &tblFldIdx) != SH_SUCCESS) {
    return SH_GENERIC_ERROR;
  }
  nbytes = array->data.size * array->dim[0];
  memcpy((void *)quartiles->data[2]->rows_s32[row],(void*)array->arrayPtr,nbytes);

  if (shTblFldLoc(tblcol, -1, "FLATVAL", 0, SH_CASE_INSENSITIVE, &array, 
    &tblFld, &tblFldIdx) != SH_SUCCESS) {
    return SH_GENERIC_ERROR;
  }
  nbytes = array->data.size * array->dim[0];
  memcpy((void *)quartiles->data[3]->rows_s32[row],(void*)array->arrayPtr,nbytes);

  return SH_SUCCESS;

}

