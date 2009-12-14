/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* qtree_encode.c	Encode values in quadrant of 2-D array using binary
 *					quadtree coding for each bit plane.  Assumes array is
 *					positive.
 *
 * Programmer: R. White		Date: 14 June 1994
 */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "region.h"
#include "dervish_msg_c.h"
#include "phCompUtils.h"

/*
 * Huffman code values and number of bits in each code
 */
static int code[16] =
	{
	0x3e, 0x00, 0x01, 0x08, 0x02, 0x09, 0x1a, 0x1b,
	0x03, 0x1c, 0x0a, 0x1d, 0x0b, 0x1e, 0x3f, 0x0c
	};
static int ncode[16] =
	{
	6,    3,    3,    4,    3,    4,    5,    5,
	3,    5,    4,    5,    4,    5,    6,    4
	};

/*
 * variables for bit output to buffer when Huffman coding
 */
static int bitbuffer;
static int bits_to_go;

/*
 * macros to write out 4-bit nybble, Huffman code for this value
 */
#define output_nybble(outfile,c)	output_nbits(outfile,c,4)
#define output_huffman(outfile,c)	output_nbits(outfile,code[c],ncode[c])

extern void output_nbits(MYFILE *outfile, int bits, int n);

static void qtree_onebit(int a[], int n, int nx, int ny, unsigned char b[], 
			 int bit);
static void qtree_reduce(unsigned char a[], int n, int nx, int ny, 
			 unsigned char b[]);
static int  bufcopy(unsigned char a[], int n, unsigned char **bptr, unsigned char *bufend);
static void write_bdirect(MYFILE *outfile, unsigned char a[], int nqx, int nqy);

extern void output_bit(MYFILE *outfile, int bit);

extern void
qtree_encode(MYFILE *outfile, int a[], int n, int nqx, int nqy, int nbitplanes)
{
int log2n, k, bit, bmax, nqmax, nqx2, nqy2, nx, ny;
unsigned char *scratch, *scr1, *sinput, *buffer, *bptr, *bufend;

	/*
	 * log2n is log2 of max(nqx,nqy) rounded up to next power of 2
	 */
	nqmax = (nqx>nqy) ? nqx : nqy;
	log2n = log((float) nqmax)/log(2.0)+0.5;
	if (nqmax > (1<<log2n)) {
		log2n += 1;
	}
	/*
	 * initialize buffer point, max buffer size
	 */
	nqx2 = (nqx+1)/2;
	nqy2 = (nqy+1)/2;
	bmax = (nqx2*nqy2+1)/2;
	/*
	 * We're indexing A as a 2-D array with dimensions (nqx,nqy).
	 * Scratch is 2-D with dimensions (nqx/2,nqy/2) rounded up.
	 * Scr1 is used to store first level of quadtree in case direct
	 * coding is needed.
	 * Buffer is used to store string of codes for output.
	 */
	scratch = (unsigned char *) malloc(2*bmax);
	scr1    = (unsigned char *) malloc(2*bmax);
	buffer  = (unsigned char *) malloc(bmax);
	if ((scratch == (unsigned char *) NULL) ||
		(buffer  == (unsigned char *) NULL) ||
		(scr1    == (unsigned char *) NULL)) {
		fprintf(stderr, "qtree_encode: insufficient memory\n");
		exit(-1);
	}
	bufend = &buffer[bmax];
	/*
	 * now encode each bit plane, starting with the top
	 */
	for (bit=nbitplanes-1; bit >= 0; bit--) {
		/*
		 * initialize bit buffer
		 */
		bptr = buffer;
		bitbuffer = 0;
		bits_to_go = 0;
		/*
		 * on first pass copy A to scr1 array
		 */
		qtree_onebit(a,n,nqx,nqy,scr1,bit);
		nx = (nqx+1)>>1;
		ny = (nqy+1)>>1;
		/*
		 * copy non-zero values to output buffer, which will be written
		 * in reverse order
		 */
		if (bufcopy(scr1,nx*ny,&bptr,bufend)) {
			/*
			 * quadtree is expanding data,
			 * change warning code and just fill buffer with bit-map
			 */
			write_bdirect(outfile,scr1,nqx,nqy);
			goto bitplane_done;
		}
		/*
		 * do log2n reductions
		 * sinput is pointer to input scratch array; it changes to
		 * scratch after the first pass.
		 */
		sinput = scr1;
		for (k = 1; k<log2n; k++) {
			qtree_reduce(sinput,ny,nx,ny,scratch);
			sinput = scratch;
			nx = (nx+1)>>1;
			ny = (ny+1)>>1;
			if (bufcopy(scratch,nx*ny,&bptr,bufend)) {
				write_bdirect(outfile,scr1,nqx,nqy);
				goto bitplane_done;
			}
		}
		/*
		 * OK, we've got the code in buffer
		 * Write quadtree warning code, then write buffer in reverse order
		 */
		output_bit(outfile,1);
		if (bptr == buffer) {
			if (bits_to_go>0) {
				/*
				 * put out the last few bits
				 */
				output_nbits(outfile, bitbuffer & ((1<<bits_to_go)-1),
					bits_to_go);
			} else {
				/*
				 * have to write a zero nybble if there are no 1's in array
				 */
				output_huffman(outfile,0);
			}
		} else {
			if (bits_to_go>0) {
				/*
				 * put out the last few bits
				 */
				output_nbits(outfile, bitbuffer & ((1<<bits_to_go)-1),
					bits_to_go);
			}
			/*
			 * write in blocks of 24 bits to speed things up
			 */
			for (bptr--; bptr >= buffer+2; bptr -= 3)
				output_nbits(outfile,
					(*bptr<<16) | (*(bptr-1)<<8) | *(bptr-2),24);
			for ( ; bptr >= buffer; bptr--)
				output_nbits(outfile,*bptr,8);
		}
		bitplane_done: ;
	}
	free(buffer);
	free(scratch);
	free(scr1);
}

/*
 * copy non-zero codes from array to buffer
 */
static int
bufcopy(unsigned char a[], int n, unsigned char **bptr, unsigned char *bufend)
{
unsigned char *p, *buffer;
/* local copies of global variables */
int lbitbuffer;
int lbits_to_go;

	buffer = *bptr;
	lbits_to_go = bits_to_go;
	lbitbuffer = bitbuffer;
	for (p = a; p < &a[n]; p++) {
		if (*p != 0) {
			/*
			 * add Huffman code for a[i] to buffer
			 */
			lbitbuffer |= code[*p] << lbits_to_go;
			lbits_to_go += ncode[*p];
			if (lbits_to_go >= 24) {
				/*
				 * return warning code if we're going to fill buffer
				 */
				if (buffer+3 >= bufend) return(1);
				/*
				 * move 3 bytes to buffer
				 */
				*buffer++ =  lbitbuffer      & 0xFF;
				*buffer++ = (lbitbuffer>> 8) & 0xFF;
				*buffer++ = (lbitbuffer>>16) & 0xFF;
				lbitbuffer >>= 24;
				lbits_to_go -= 24;
			}
		}
	}
	*bptr = buffer;
	bitbuffer = lbitbuffer;
	bits_to_go = lbits_to_go;
	return(0);
}

/*
 * Do first quadtree reduction step on bit BIT of array A.
 * Results put into B.
 * 
 */
static void
qtree_onebit(int a[], int n, int nx, int ny, unsigned char b[], int bit)
{
int i, *p, *pend;
unsigned char *pb, *pb0;
int mbit, bitm2;

	/*
	 * mask to get selected bit
	 */
	mbit = 1<<bit;
	pb = b;
	bitm2 = bit - 2;
	for (i = 0; i<nx; i += 2) {
		pb0 = pb;
		pend = &a[n*i+ny-1];
		switch (bit) {
			case 0:
				for (p = &a[n*i]; p < pend; p += 2, pb += 1)
					*pb = ((*p & mbit)<<3) | ((*(p+1) & mbit)<<2);
				break;
			case 1:
				for (p = &a[n*i]; p < pend; p += 2, pb += 1)
					*pb = ((*p & mbit)<<2) | ((*(p+1) & mbit)<<1);
				break;
			case 2:
				for (p = &a[n*i]; p < pend; p += 2, pb += 1)
					*pb = ((*p & mbit)<<1) | ((*(p+1) & mbit)   );
				break;
			default:
				for (p = &a[n*i]; p < pend; p += 2, pb += 1)
					*pb = ( ((*p & mbit)<<1) | (*(p+1) & mbit) ) >> bitm2;
		}
		if (p == pend) {
			/*
			 * row size is odd, do last element in row
			 * *(p+1) is off edge
			 */
			*pb = ((*p & mbit)<<3) >> bit;
			pb += 1;
		}
		if (i < nx-1) {
			/*
			 * not on last row, add in next row
			 */
			pb = pb0;
			pend = &a[n*(i+1)+ny-1];
			switch (bit) {
				case 0:
					for (p = &a[n*(i+1)]; p < pend; p += 2, pb += 1)
						*pb = ( ((*p & mbit)<<1) |  (*(p+1) & mbit)     ) | *pb;
					break;
				case 1:
					for (p = &a[n*(i+1)]; p < pend; p += 2, pb += 1)
						*pb = (  (*p & mbit)     | ((*(p+1) & mbit)>>1) ) | *pb;
					break;
				default:
					for (p = &a[n*(i+1)]; p < pend; p += 2, pb += 1)
						*pb = ( ( ((*p & mbit)<<1) | (*(p+1) & mbit) ) >> bit) | *pb ;
					break;
			}
			if (p == pend) {
				/* odd row size */
				*pb = ( ((*p & mbit)<<1) >> bit) | *pb ;
				pb += 1;
			}
		}
	}
}

/*
 * do one quadtree reduction step on array a
 * results put into b (which may be the same as a)
 */
static void
qtree_reduce(unsigned char a[], int n, int nx, int ny, unsigned char b[])
{
int i;
unsigned char *p, *pend, *pb, *pb0;

	pb = b;
	for (i = 0; i<nx; i += 2) {
		pb0 = pb;
		pend = &a[n*i+ny-1];
		for (p = &a[n*i]; p < pend; p += 2, pb += 1)
			*pb = ( ((*p != 0) << 3) | ((*(p+1) != 0) << 2) );
		if (p == pend) {
			/*
			 * row size is odd, do last element in row
			 * *(p+1) is off edge
			 */
			*pb = (*p != 0) << 3;
			pb += 1;
		}
		if (i < nx-1) {
			/*
			 * not on last row, add in next row
			 */
			pb = pb0;
			pend = &a[n*(i+1)+ny-1];
			for (p = &a[n*(i+1)]; p < pend; p += 2, pb += 1)
				*pb = ((*p != 0) << 1) | (*(p+1) != 0) | *pb;
			if (p == pend) {
				/* odd row size */
				*pb = ((*p != 0) << 1) | *pb;
				pb += 1;
			}
		}
	}
}

static void
write_bdirect(MYFILE *outfile, unsigned char a[], int nqx, int nqy)
{
int i;
int ilast;

	/*
	 * Write the direct bitmap warning code
	 */
	output_bit(outfile,0);
	/*
	 * write quads for original bitplane to outfile
	 * write in blocks of 24 bits to speed things up
	 */
	ilast = ((nqx+1)/2) * ((nqy+1)/2);
	for (i = 0; i < ilast-5; i += 6) {
		output_nbits(outfile,
			(a[i  ]<<20) |
			(a[i+1]<<16) |
			(a[i+2]<<12) |
			(a[i+3]<< 8) |
			(a[i+4]<< 4) |
			 a[i+5],
			24);
	}
	for (; i < ilast; i++) output_nybble(outfile,a[i]);
}
