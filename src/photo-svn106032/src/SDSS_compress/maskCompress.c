/* maskCompress
 *
 * Given Mask of int :
 * (1) Compress using H-transform compression
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
 *     Compress mask 
  *
 * R. White, 27 July 1994
 * A. Connolly implemented under DERVISH Aug 94
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

extern int htrans(int a[], int nx, int ny);
extern int digitize(int a[], int nx, int ny, int scale);
extern int encode(MYFILE *outfile, int a[], int nx, int ny,int scale);
extern int rice_encode(MYFILE *outfile, int a[] , int nx, int ny, int scale);

RET_CODE 
maskCompress( MASK **mask, int compRatio)
{
  
  int   nx, ny, j, k;
  int *array;
  int i, nqtree, nrice, nbytes;
  MYFILE *qtree_buffer, *rice_buffer, *buffer;
  MASK *temp;

  /* ------------ initialization ------------ 
   * copy mask to an array of long ints
   */
  nx = (*mask)->nrow;
  ny = (*mask)->ncol;
  array = (int *) shMalloc(nx * ny * sizeof(int));
  if (array == (int *) NULL) {
    shError ("maskCompress: cant allocate array MASK size");
    return(SH_GENERIC_ERROR);
  }
  k=0;
  for (i = 0; i < nx; i++){
    for (j = 0; j < ny; j++){
      array[k] = (int) (*mask)->rows[i][j];
      k++;
    }
  }

  /* ------------ compression ------------ */

  /*
   * H-transform
   */
  if (htrans(array, nx, ny) != SH_SUCCESS){
    shError ("htrans: cant H transform the data");
    shFree(array);
    return(SH_GENERIC_ERROR);
  }

  /*
   * Digitize
   */
  if (digitize(array, nx, ny, compRatio) != SH_SUCCESS){
    shError ("digitize: cant scale the data");
    shFree(array);
    return(SH_GENERIC_ERROR);
  }	

  /*
   * allocate buffers for qtree, Rice compressed data
   */
  qtree_buffer = (MYFILE *) shMalloc(sizeof(MYFILE));
  if (qtree_buffer == (MYFILE *) NULL) {
    shError("Cant allocate for qtree buffer");
    shFree(array);
    return(SH_GENERIC_ERROR);
  }
  if (bufalloc(qtree_buffer, 4*nx*ny+100) == (unsigned char *) NULL) {
    shError("Cant allocate for qtree buffer");
    shFree(array);
    return(SH_GENERIC_ERROR);
  }
	
  rice_buffer = (MYFILE *) shMalloc(sizeof(MYFILE));
  if (rice_buffer == (MYFILE *) NULL) {
    shError("Cant allocate for rice buffer");
    shFree(array);
    return(SH_GENERIC_ERROR);
  }
  if (bufalloc(rice_buffer, 4*nx*ny+100) == (unsigned char *) NULL) {
    shError("Cant allocate for rice buffer");
    shFree(array);
    return(SH_GENERIC_ERROR);
  }

  /*
   * Rice encode and write to buffer
   */
  if (rice_encode(rice_buffer, array, nx, ny, compRatio) 
      != SH_SUCCESS){
    shError("Cant encode rice buffer");
    shFree(array);
    buffree (rice_buffer);
    shFree(rice_buffer);
    buffree (qtree_buffer);
    shFree(qtree_buffer);
    return(SH_GENERIC_ERROR);
  }
  shDebug(COMP_DEBUG,"maskCompress: finished Rice encoding\n");

  /*
   * Qtree encode and write to buffer
   */
  if (encode(qtree_buffer, array, nx, ny, compRatio) 
      != SH_SUCCESS){
    shError("Cant encode qtree buffer");
    shFree(array);
    buffree (rice_buffer);
    shFree(rice_buffer);
    buffree (qtree_buffer);
    shFree(qtree_buffer);
    return(SH_GENERIC_ERROR);
  }
  shDebug(COMP_DEBUG,"maskCompress: finished qtree encoding\n");

  /* free array */
  shFree(array);

  /*
   * save number of bytes required, then set buffer
   * pointers to show proper array size
   */
  nqtree = qtree_buffer->current - qtree_buffer->start;
  qtree_buffer->end = qtree_buffer->current;
  qtree_buffer->current = qtree_buffer->start;
  shDebug(COMP_DEBUG,"qtree encoding %d bytes\n", nqtree);

  nrice = rice_buffer->current - rice_buffer->start;
  rice_buffer->end = rice_buffer->current;
  rice_buffer->current = rice_buffer->start;
  shDebug(COMP_DEBUG, "Rice  encoding %d bytes\n", nrice);

  shDebug(COMP_DEBUG, "Compression complete");

  /*
   * select best compression & free other buffer
   */
  if (nqtree > nrice) {
    /* Rice is best */
    buffree(qtree_buffer);
    shFree(qtree_buffer);
    buffer = rice_buffer;
    nbytes = nrice;
    shDebug(COMP_DEBUG,"Selecting Rice\n");
  } else {
    /* Qtree is best */
    buffree(rice_buffer);
    shFree(rice_buffer);
    buffer = qtree_buffer;
    nbytes = nqtree;
    shDebug(COMP_DEBUG,"Selecting Qtree\n");
  }
    shDebug(COMP_DEBUG,"maskCompress: %d x %d image required %d bytes\n",nx, ny, nbytes);

  /*
   * if compressed version takes more bytes than original
   * free everything and return;
   */
  if (nbytes > 2*nx*ny) {
    shDebug(COMP_DEBUG,"maskCompress: %d x %d image required %d bytes, no compression\n",nx, ny, nbytes);
    /* free storage */
    buffree(buffer);
    shFree(buffer);
    /* return number of bytes required for original image */
    return(SH_GENERIC_ERROR);
  } 

  /* create a new mask of U8 and populate with compressed mask 
   * One dimensional mask. 
   */
  if ((temp = shMaskNew("Compressed mask", 1, nbytes)) == NULL) {
    shError("Failed to allocate new MASK");
    buffree(buffer);
    shFree(buffer);
    return(SH_GENERIC_ERROR);
  }

  buffer->current = buffer->start;
  for (i = 0; i < nbytes; i++){
    temp->rows[0][i] = (int) getCB(buffer);
    buffer->current++;
  }

  /* propagate row0 and col0 */
  temp->row0 = (*mask)->row0;
  temp->col0 = (*mask)->col0;

  /* dereference mask pointer to temp */
  (*mask) = temp;

  return (SH_SUCCESS);
}
