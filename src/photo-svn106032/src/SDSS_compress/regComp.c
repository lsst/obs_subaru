/* regComp
 *
 * Given Region of int :
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
 * Pass a REGION with associated mask (if present)
 *     Copy REGION 
 *     Compress region 
 *     Call maskComp for mask
 *     Create a new region and new associated mask (note region
 *     and mask sizes may be different)
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

extern int htrans(int a[], int nx, int ny);
extern int digitize(int a[], int nx, int ny, int scale);
extern int encode(MYFILE *outfile, int a[], int nx, int ny,int scale);
extern int decode(MYFILE *infile, FILE *outfile, int **a, int *nx, int *ny, 
		  int *scale, char **format);
extern int rice_encode(MYFILE *outfile, int a[] , int nx, int ny, int scale);

RET_CODE 
regComp( REGION **reg, int compRatio)
{
  
  int   nx, ny, j, k;
  int *array;
  int i, nqtree, nrice, nbytes;
  MYFILE *qtree_buffer, *rice_buffer, *buffer;
  REGION *temp;

  /* ------------ initialization ------------ 
   * copy region to an array of long ints
   * copy mask to an array of long ints
   */
  nx = (*reg)->nrow;
  ny = (*reg)->ncol;
  array = (int *) shMalloc(nx * ny * sizeof(int));
  if (array == (int *) NULL) {
    shError ("regComp: cant allocate array REGION size");
    return(SH_GENERIC_ERROR);
  }
  k=0;
  for (i = 0; i < nx; i++){
    for (j = 0; j < ny; j++){
      array[k] = (int) (*reg)->rows[i][j];
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
  shDebug(COMP_DEBUG,"thcomp: finished Rice encoding\n");

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
  shDebug(COMP_DEBUG,"thcomp: finished qtree encoding\n");

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


    shDebug(COMP_DEBUG,"regComp: %d x %d image required %d bytes\n",nx, ny, nbytes);
  /*
   * if compressed version takes more bytes than original
   * free everything and return;
   */
  if (nbytes > 2*nx*ny) {
    shDebug(COMP_DEBUG,"regComp: %d x %d image required %d bytes, no compression\n",nx, ny, nbytes);
    /* free storage */
    buffree(buffer);
    shFree(buffer);
    /* return number of bytes required for original image */
    return(SH_GENERIC_ERROR);
  } 

  /* create a new region of U8 and populate with compressed region 
   * One dimensional region. The mask information in REGION is not
   * saved.
   */
  if ((temp = shRegNew("Compressed data", 1, nbytes, TYPE_U8)) == NULL) {
    shError("Failed to allocate new REGION");
    buffree(buffer);
    shFree(buffer);
    return(SH_GENERIC_ERROR);
  }

#if 0
  /* if mask exists compress mask */
  if ((*reg)->mask != NULL){
    if (maskComp (&(*reg)->mask, compRatio) == SH_SUCCESS) {
      shError ("Failed to compress MASK");
    }
  }
  temp->mask = (*reg)->mask;

#endif

    
  shDebug (COMP_DEBUG, " temp array size row %d col %d ",temp->nrow, 
	   temp->ncol);
  buffer->current = buffer->start;
  for (i = 0; i < nbytes; i++){
    temp->rows[0][i] = (int) getCB(buffer);
    shDebug (COMP_DEBUG, " value %d column %d bytes %d",temp->rows[0][i], 
	     i, nbytes);
  }

  /* propagate row0 and col0 */
  temp->row0 = (*reg)->row0;
  temp->col0 = (*reg)->col0;

  /* dereference reg pointer to temp */
  (*reg) = temp;

  return (SH_SUCCESS);
}

/*
 * Below are the functions for the buffer processing, replacing file
 * i/o with buffer i/o.
*/

unsigned char* bufalloc(MYFILE* mf, unsigned int n)
{
  mf->start = (unsigned char *) shMalloc(n) ;
  if(mf->start == NULL)
    return((unsigned char*)NULL) ;
  mf->end = mf->start + n ;
  mf->current = mf->start ;
  return(mf->start) ;
}

void buffree(MYFILE* mf)
{
  shFree(mf->start) ;
}


unsigned char getCB(MYFILE* mf)
{
  unsigned char c ;
  if(mf->current < mf->end){
    c = *mf->current ;
    mf->current++ ;
  }else
    c = (unsigned char)EOF ;
  return(c) ;
}

int putCB(unsigned char c, MYFILE* mf)
{
  int status = 1 ;
  if(mf->current < mf->end){
    status = fprintf(stderr,"putCB: Buffer Overflow.\n") ;
    exit(-1) ;
   }else{
     *mf->current = c ;
     mf->current++ ;
   }
  return(status) ;
}
