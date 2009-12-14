/* maskUncomp
 *
 * Given Mask of U8 :
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
 * Pass a MASK with associated mask (if present)
 *     Copy MASK 
 *     Uncompress mask 
 *     Create a new mask 
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
maskUncomp( MASK **mask)
{
  
  int   nx, ny, compRatio;
  int *array;
  int i, j, k;
  char value;
  MYFILE *buffer;
  MASK *temp;
  char *format;

  /* allocate a buffer to hold the data */
  bufalloc(buffer, (*mask)->ncol);

  /* populate buffer with mask data */
  for (i = 0; i < (*mask)->ncol; i++){
    value = (char) (*mask)->rows[0][i];
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
  /* create new mask the size of the original data */
  if ((temp = shMaskNew("Uncompressed data", nx, ny)) == NULL) {
    shError("Failed to allocate new MASK");
    buffree(buffer);
    shFree(buffer);
    return(SH_GENERIC_ERROR);
  }

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
  /* dereference mask pointer to temp */
  (*mask) = temp;


  /* free storage */
  buffree(buffer);
  shFree(buffer);
  shFree(array);
  shDebug(COMP_DEBUG,"thcomp: decompression done\n");
  
  return(SH_SUCCESS);
}
