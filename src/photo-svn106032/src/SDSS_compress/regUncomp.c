/* regUncomp
 *
 * Given Region of U8 :
 * (1) Uncompress using H-transform compression
 * (3) Return modified pixel values in place of original image and
 *		return number of bytes required for compressed image as
 *		function value.
 *
 *
 * This version also tries Rice compression of the thresholded H-transform
 * and returns number of bytes used by Rice if smaller than the quadtree
 * number.
 *
 * Pass a REGION with associated mask (if present)
 *     Copy REGION 
 *     Uncompress region 
 *     Call maskUncomp for mask
 *     Create a new region and new associated mask 
 *
 * R. White, 27 July 1994
 * A. Connolly implemented under SHIVA Aug 94
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "ftcl.h"
#include "shiva.h"
#include "region.h"
#include "math.h"
#include "shiva_msg_c.h"
#include "errStack.h"
#include "phStructs.h"
#include "phCompUtils.h"

#define COMP_DEBUG 10

int verbose;

extern int undigitize(int a[], int nx, int ny, int scale);
extern int decode(MYFILE *infile, FILE *outfile, int **a, int *nx, 
		  int *ny, int *scale, char **format);
extern int hinv(int a[], int nx, int ny, int smooth, int scale);

RET_CODE 
regUncomp( REGION **reg)
{
  
  int   nx, ny, compRatio;
  int *array;
  int i, j, k;
  char value;
  MYFILE *buffer;
  REGION *temp;
  char *format;

  /* allocate a buffer to hold the data */
  buffer = (MYFILE *) malloc(sizeof(MYFILE));
  if ( bufalloc(buffer, (*reg)->ncol) == (unsigned char *) NULL){
    shError("Cant allocate buffer");
    return(SH_GENERIC_ERROR);
  }

  buffer->current = buffer->start;
  /* populate buffer with region data */
  for (i = 0; i < (*reg)->ncol; i++){
    value = (char) (*reg)->rows[0][i];
    putCB(value,buffer);
  }

  /*
   * Read from buffer and decode.  Returns address, size, compRatio,
   */
  format = "\0";
  decode(buffer, (FILE *)NULL, &array, &nx, &ny, &compRatio, &format);

  /*
   * Un-Digitize
   */
  undigitize(array, nx, ny, compRatio);

  /*
   * Inverse H-transform
   */
  hinv(array, nx, ny, 0, compRatio);
  
  /* ------------ finish ------------            */
  /* create new region the size of the original data */
  if ((temp = shRegNew("Uncompressed data", nx, ny, TYPE_U16)) == NULL) {
    shError("Failed to allocate new REGION");
    buffree(buffer);
    shFree(buffer);
    return(SH_GENERIC_ERROR);
  }

#if 0
  /* if mask exists uncompress mask */
  if ((*reg)->mask != NULL){
    if (maskUncomp (&(*reg)->mask) == SH_SUCCESS) {
      shError ("Failed to uncompress MASK");
    }
  }
  temp->mask = (*reg)->mask;
#endif

  /* Fill array with uncompressed data */

  temp->nrow = nx;
  temp->ncol = ny;
  k=0;
  for (i = 0; i < nx; i++){
    for (j = 0; j < ny; j++){
      temp->rows[i][j] = array[k];
      k++;
    }
  }
  /* dereference reg pointer to temp */
  (*reg) = temp;


  /* free storage */
  buffree(buffer);
  shFree(buffer);
  shFree(array);
  shDebug(COMP_DEBUG,"thcomp: decompression done\n");
  
  return(SH_SUCCESS);
}
