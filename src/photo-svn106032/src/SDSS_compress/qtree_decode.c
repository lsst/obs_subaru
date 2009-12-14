/* Copyright (c) 1993 Association of Universities for Research
 * in Astronomy. All rights reserved. Produced under National
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* qtree_decode.c	Read stream of codes from infile and construct bit planes
 *					in quadrant of 2-D array using binary quadtree coding
 *
 * Programmer: R. White		Date: 16 June 1994
 */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "region.h"
#include "dervish_msg_c.h"
#include "phCompUtils.h"

/* bit buffer from bit_input.c */

static int buffer;					/* Bits waiting to be input				*/
static int bits_to_go;				/* Number of bits still in buffer		*/

/*
 * input 1 bit without EOF error checking
 * (EOFs get picked up in dodecode after calls to qtree_decode are done)
 */
#define input_bit(infile) (bits_to_go ? \
			((buffer>>(--bits_to_go)) & 1) : \
			(((buffer = getCB(infile))>>(bits_to_go = 7)) & 1))
/*
 * input N bits without EOF error checking
 */
#define input_nbits(infile,n) ((bits_to_go<n) ? \
			(((buffer = (buffer<<8) | getCB(infile)) \
				>>  (bits_to_go += 8-n)) & ((1<<n)-1)) : \
			((buffer>>(bits_to_go -= n)) & ((1<<n)-1)) )
/*
 * input 4 bits
 */
#define input_nybble(infile)    input_nbits(infile,4)

static void qtree_expand(MYFILE *infile, unsigned char a[] ,int nx, int ny, 
	     unsigned char b[], int limits[2]);
static void qtree_copy(unsigned char a[], int nx, int ny, unsigned char b[], 
		       int limits[2]);
static void qtree_bitins(unsigned char a[], int nx, int ny, int b[], int n, 
	     int bit, int limits[2]);
static void read_bdirect(MYFILE *infile, int a[], int n, int nqx, int nqy, 
	     unsigned char scratch[], int bit);
static int  input_huffman(MYFILE *infile);

extern void
qtree_decode(MYFILE *infile, int a[], int n, int nqx, int nqy, int nbitplanes)
{
int log2n, k, bit, b, nqmax;
int nx,ny,nfx,nfy,c;
int nqx2, nqy2;
unsigned char *scratch;
int limits[2];

	/*
	 * log2n is log2 of max(nqx,nqy) rounded up to next power of 2
	 */
	nqmax = (nqx>nqy) ? nqx : nqy;
	log2n = log((float) nqmax)/log(2.0)+0.5;
	if (nqmax > (1<<log2n)) {
		log2n += 1;
	}
	/*
	 * allocate scratch array for working space
	 */
	nqx2=(nqx+1)/2;
	nqy2=(nqy+1)/2;
	scratch = (unsigned char *) malloc(nqx2*nqy2);
	if (scratch == (unsigned char *) NULL) {
		fprintf(stderr, "qtree_decode: insufficient memory\n");
		exit(-1);
	}
	/*
	 * now decode each bit plane, starting at the top
	 * A is assumed to be initialized to zero
	 */
	for (bit = nbitplanes-1; bit >= 0; bit--) {
		/*
		 * Was bitplane was quadtree-coded or written directly?
		 */
		b = input_bit(infile);
		if(b == 0) {
			/*
			 * bit map was written directly
			 */
			read_bdirect(infile,a,n,nqx,nqy,scratch,bit);
		} else if (b != 1) {
			fprintf(stderr, "qtree_decode: bad format code %x\n",b);
			exit(-1);
		} else {
			/*
			 * bitmap was quadtree-coded, do log2n expansions
			 *
			 * read first code
			 */
			scratch[0] = input_huffman(infile);
			/*
			 * if code is zero, implying all bits are zero, just
			 * skip the rest for this plane
			 */
			if (scratch[0] != 0) {
				limits[0] = 0;
				limits[1] = 0;
				/*
				 * now do log2n expansions, reading codes from file as necessary
				 */
				nx = 1;
				ny = 1;
				nfx = nqx;
				nfy = nqy;
				c = 1<<log2n;
				for (k = 1; k<log2n; k++) {
					/*
					 * this somewhat cryptic code generates the sequence
					 * n[k-1] = (n[k]+1)/2 where n[log2n]=nqx or nqy
					 */
					c = c>>1;
					nx = nx<<1;
					ny = ny<<1;
					if (nfx <= c) { nx -= 1; } else { nfx -= c; }
					if (nfy <= c) { ny -= 1; } else { nfy -= c; }
					qtree_expand(infile, scratch, nx, ny, scratch, limits);
				}
				/*
				 * now copy last set of 4-bit codes to bitplane bit of array a
				 */
				qtree_bitins(scratch, nqx, nqy, a, n, bit, limits);
			}
		}
	}
	free(scratch);
}


/*
 * do one quadtree expansion step on array a[(nx+1)/2,(ny+1)/2]
 * results put into b[nx,ny] (which may be the same as a)
 */
static void
qtree_expand(MYFILE *infile, unsigned char a[] ,int nx, int ny, 
	     unsigned char b[], int limits[2])
/* int limits[2]; limits gives max row, column index of non-zero elements */
{
int i;
unsigned char *p, *pend;

	/*
	 * first copy a to b, expanding each 4-bit value
	 * resets limits
	 */
	qtree_copy(a,nx,ny,b,limits);
	/*
	 * now read new 4-bit values into b for each non-zero element
	 */
	for (i = limits[1]; i >= 0; i--) {
		pend = &b[i*ny];
		for (p = pend + limits[0] ; p >= pend; p--) {
			if (*p != 0) *p = input_huffman(infile);
		}
	}
}

/*
 * copy 4-bit values from a[(nx+1)/2,(ny+1)/2] to b[nx,ny], expanding
 * each value to 2x2 pixels
 * a,b may be same array
 */
static void
qtree_copy(unsigned char a[], int nx, int ny, unsigned char b[], int limits[2])
{
int i, ny2, xmax, ymax, xlim2, ylim2;
unsigned char tmp, *p00, *p10, *pa, *pend;

	ny2 = (ny+1)/2;
	xlim2 = 2*limits[1];
	ylim2 = 2*limits[0];
	/* xmax, ymax used to set new limits[1], limits[0] value */
	xmax = 0;
	ymax = 0;
	/*
	 * Start copy+expand at end in case a,b are same array.
	 * Don't waste time on zero elements.
	 * When nx,ny are even check last row,column for non-zero values
	 * and reduce limits if none are found.
	 *
	 * ---- last row ----
	 */
	i = limits[1];
	if (xlim2 == nx-1) {
		/*
		 * p10, p10+1 are off edge
		 */
		pend = &b[2*ny*i];
		p00 = pend + ylim2;
		pa = &a[ny2*i+limits[0]];
		tmp = *pa;
		if (ylim2 == ny-1) {
			/*
			 * p00+1, p10+1 are off edge
			 */
			*(p00  ) = tmp & 8;
		} else {
			*(p00+1) = tmp & 4;
			*(p00  ) = tmp & 8;
			/* check last column for non-zero values */
			if (*(p00+1) != 0) ymax = 1;
		}
		for (p00 -= 2, pa-- ; p00 >= pend; p00 -= 2, pa--) {
			tmp = *pa;
			*(p00+1) = tmp & 4;
			*(p00  ) = tmp & 8;
		}
	} else {
		pend = &b[2*ny*i];
		p00 = pend + ylim2;
		p10 = p00 + ny;
		pa = &a[ny2*i+limits[0]];
		tmp = *pa;
		if (ylim2 == ny-1) {
			/*
			 * p00+1, p10+1 are off edge
			 */
			*(p10  ) = tmp & 2;
			*(p00  ) = tmp & 8;
			/* check last row for non-zero values */
			if (*p10 != 0) xmax = 1;
		} else {
			*(p10+1) = tmp & 1;
			*(p10  ) = tmp & 2;
			*(p00+1) = tmp & 4;
			*(p00  ) = tmp & 8;
			/* check last column and last row for non-zero values */
			if (*(p00+1) != 0 || *(p10+1) != 0) ymax = 1;
			if (*(p10  ) != 0 || *(p10+1) != 0) xmax = 1;
		}
		for (p00 -= 2, p10 -= 2, pa-- ; p00 >= pend; p00 -= 2, p10 -= 2, pa--) {
			tmp = *pa;
			*(p10+1) = tmp & 1;
			*(p10  ) = tmp & 2;
			*(p00+1) = tmp & 4;
			*(p00  ) = tmp & 8;
			/* check last row for non-zero values */
			if (*p10 != 0 || *(p10+1) != 0) xmax = 1;
		}
	}
	/*
	 * ---- all rows but last ----
	 */
	for (i--; i >= 0; i--) {
		pend = &b[2*ny*i];
		p00 = pend + ylim2;
		p10 = p00 + ny;
		pa = &a[ny2*i+limits[0]];
		tmp = *pa;
		if (ylim2 == ny-1) {
			/*
			 * p00+1, p10+1 are off edge
			 */
			*(p10  ) = tmp & 2;
			*(p00  ) = tmp & 8;
		} else {
			*(p10+1) = tmp & 1;
			*(p10  ) = tmp & 2;
			*(p00+1) = tmp & 4;
			*(p00  ) = tmp & 8;
			/* check last column for non-zero values */
			if (*(p00+1) != 0 || *(p10+1) != 0) ymax = 1;
		}
		for (p00 -= 2, p10 -= 2, pa-- ; p00 >= pend; p00 -= 2, p10 -= 2, pa--) {
			tmp = *pa;
			*(p10+1) = tmp & 1;
			*(p10  ) = tmp & 2;
			*(p00+1) = tmp & 4;
			*(p00  ) = tmp & 8;
		}
	}
	/*
	 * update limits
	 */
	limits[1] = xlim2 + xmax;
	limits[0] = ylim2 + ymax;
}

/*
 * Copy 4-bit values from a[(nx+1)/2,(ny+1)/2] to b[nx,ny], expanding
 * each value to 2x2 pixels and inserting into bitplane BIT of B.
 * A,B may NOT be same array (it wouldn't make sense to be inserting
 * bits into the same array anyway.)
 * Note A is unsigned char and B is int.
 */
static void
qtree_bitins(unsigned char a[], int nx, int ny, int b[], int n, 
	     int bit, int limits[2])
/* int n;	       declared y dimension of b */
{
int i, xend, ny2, yend;
unsigned char *pa;
int *p00, *p10, *pend, tmp;

	/*
	 * don't waste time on zero elements; stop after
	 * row containing first nonzero a value
	 */
	ny2 = (ny+1)/2;

	xend = 2*limits[1] + 1;
	if (xend > nx - 1) xend = nx - 1;

	yend = 2*limits[0]+1;
	if (yend > ny - 1) yend = ny - 1;

	/*
	 * expand each 2x2 block
	 */
	for (i = 0; i<xend; i += 2) {
		p00 = &b[n*i];
		p10 = p00 + n;
		pend = p00 + yend;
		pa = &a[ny2 * (i/2)];
		/*
		 * special cases to try to make loop a bit more efficient
		 */
		switch (bit) {
			case 0:
				for ( ; p00 < pend; p00 += 2, p10 += 2) {
					if ((tmp = *pa++) != 0) {
						*(p10+1) |= (tmp & 1)     ;
						*(p10  ) |= (tmp & 2) >> 1;
						*(p00+1) |= (tmp & 4) >> 2;
						*(p00  ) |= (tmp & 8) >> 3;
					}
				}
				break;
			case 1:
				for ( ; p00 < pend; p00 += 2, p10 += 2) {
					if ((tmp = *pa++) != 0) {
						*(p10+1) |= (tmp & 1) << 1;
						*(p10  ) |= (tmp & 2)     ;
						*(p00+1) |= (tmp & 4) >> 1;
						*(p00  ) |= (tmp & 8) >> 2;
					}
				}
				break;
			case 2:
				for ( ; p00 < pend; p00 += 2, p10 += 2) {
					if ((tmp = *pa++) != 0) {
						*(p10+1) |= (tmp & 1) << 2;
						*(p10  ) |= (tmp & 2) << 1;
						*(p00+1) |= (tmp & 4)     ;
						*(p00  ) |= (tmp & 8) >> 1;
					}
				}
				break;
			case 3:
				for ( ; p00 < pend; p00 += 2, p10 += 2) {
					if ((tmp = *pa++) != 0) {
						*(p10+1) |= (tmp & 1) << 3;
						*(p10  ) |= (tmp & 2) << 2;
						*(p00+1) |= (tmp & 4) << 1;
						*(p00  ) |= (tmp & 8)     ;
					}
				}
				break;
			default:
				for ( ; p00 < pend; p00 += 2, p10 += 2) {
					if ((tmp = *pa++) != 0) {
						*(p10+1) |= (tmp & 1) << bit;
						*(p10  ) |= (tmp & 2) << (bit-1);
						*(p00+1) |= (tmp & 4) << (bit-2);
						*(p00  ) |= (tmp & 8) << (bit-3);
					}
				}
				break;
		}
		if (p00 == pend) {
			/*
			 * row size is odd, do last element in row
			 * p00+1 is off edge
			 */
			tmp = *pa++;
			*p10 |= ((tmp>>1) & 1) << bit;
			*p00 |= ((tmp>>3) & 1) << bit;
		}
	}
	if (i == xend) {
		/*
		 * column size is odd, do last row
		 * p10, p10+1 are off edge
		 */
		p00 = &b[n*i];
		pend = p00 + yend;
		pa = &a[ny2 * (i/2)];
		/*
		 * special cases to try to make loop a bit more efficient
		 */
		switch (bit) {
			case 0:
				for ( ; p00 < pend; p00 += 2) {
					if ((tmp = *pa++) != 0) {
						*(p00+1) |= (tmp & 4) >> 2;
						*(p00  ) |= (tmp & 8) >> 3;
					}
				}
				break;
			case 1:
				for ( ; p00 < pend; p00 += 2) {
					if ((tmp = *pa++) != 0) {
						*(p00+1) |= (tmp & 4) >> 1;
						*(p00  ) |= (tmp & 8) >> 2;
					}
				}
				break;
			case 2:
				for ( ; p00 < pend; p00 += 2) {
					if ((tmp = *pa++) != 0) {
						*(p00+1) |= (tmp & 4)     ;
						*(p00  ) |= (tmp & 8) >> 1;
					}
				}
				break;
			case 3:
				for ( ; p00 < pend; p00 += 2) {
					if ((tmp = *pa++) != 0) {
						*(p00+1) |= (tmp & 4) << 1;
						*(p00  ) |= (tmp & 8)     ;
					}
				}
				break;
			default:
				for ( ; p00 < pend; p00 += 2) {
					if ((tmp = *pa++) != 0) {
						*(p00+1) |= (tmp & 4) << (bit-2);
						*(p00  ) |= (tmp & 8) << (bit-3);
					}
				}
				break;
		}
		if (p00 == pend) {
			/*
			 * row size is odd, do last element in row
			 * p00+1, p10, p10+1 are off edge
			 */
			*p00 |= ((*pa++>>3) & 1) << bit;
		}
	}
}

static void
read_bdirect(MYFILE *infile, int a[], int n, int nqx, int nqy, 
	     unsigned char scratch[], int bit)
{
int nx2, ny2, limits[2];
unsigned char *p;

	/*
	 * read bit image packed 4 pixels/nybble
	 */
	nx2 = (nqx+1)/2;
	ny2 = (nqy+1)/2;
	for (p = scratch; p < &scratch[nx2*ny2]; p++) {
		*p = input_nybble(infile);
	}
	limits[1] = nx2-1;
	limits[0] = ny2-1;
	/*
	 * insert in bitplane BIT of image A
	 */
	qtree_bitins(scratch,nqx,nqy,a,n,bit,limits);
}

/*
 * Huffman decoding for fixed codes
 *
 * Coded values range from 0-15
 *
 * Huffman code values (hex):
 *
 *	3e, 00, 01, 08, 02, 09, 1a, 1b,
 *	03, 1c, 0a, 1d, 0b, 1e, 3f, 0c
 *
 * and number of bits in each code:
 *
 *	6,  3,  3,  4,  3,  4,  5,  5,
 *	3,  5,  4,  5,  4,  5,  6,  4
 */

/*
 * table of Huffman code translated values
 * -1 means no such code
 */
static int tabhuff[31] =
	/* 00  01  02  03  04  05  06  07  08  09  0a  0b  0c  0d  0e  0f */
	 {  1,  2,  4,  8, -1, -1, -1, -1,  3,  5, 10, 12, 15, -1, -1, -1,
	   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  6,  7,  9, 11, 13     };
	/* 10  11  12  13  14  15  16  17  18  19  1a  1b  1c  1d  1e     */

static int
input_huffman(MYFILE *infile)
{
int c;

	/*
	 * get first 3 bits to start
	 */
	c = input_nbits(infile,3);
	if (c < 4) return(tabhuff[c]);
	/*
	 * get the next bit
	 */
	c = input_bit(infile) | (c<<1);
	if (c < 13) return(tabhuff[c]);
	/*
	 * get yet another bit
	 */
	c = input_bit(infile) | (c<<1);
	if (c < 31) return(tabhuff[c]);
	/*
	 * the 6th bit decides
	 */
	if (input_bit(infile)) {
		/*
		 * c = 63
		 */
		return(14);
	} else {
		/*
		 * c = 62
		 */
		return(0);
	}
}
