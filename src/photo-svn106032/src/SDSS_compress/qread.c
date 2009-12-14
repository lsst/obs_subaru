/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* qread.c	Read binary data
 *
 * Programmer: R. White		Date: 11 March 1991
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ftcl.h"
#include "dervish.h"
#include "region.h"
#include "math.h"
#include "dervish_msg_c.h"
#include "errStack.h"
#include "phStructs.h"
#include "phCompUtils.h"

extern int readshort (MYFILE *infile);
extern int readint (MYFILE *infile);
extern int qread (MYFILE *infile, char *a, int n);
extern int myread(MYFILE *file, char *buffer, int n);


extern int
readshort(MYFILE *infile)
{
int a,i;
unsigned char b[2];

    /* Read integer A one byte at a time from infile.
     *
     * This is portable from Vax to Sun since it eliminates the
     * need for byte-swapping.
     */
    qread(infile,(char *) b, 2);
    a = b[0];
    for (i=1; i<2; i++) a = (a<<8) + b[i];
    return(a);
}

extern int
readint(MYFILE *infile)
{
int a,i;
unsigned char b[4];

    /* Read integer A one byte at a time from infile.
     *
     * This is portable from Vax to Sun since it eliminates the
     * need for byte-swapping.
     */
    qread(infile,(char *) b, 4);
    a = b[0];
    for (i=1; i<4; i++) a = (a<<8) + b[i];
    return(a);
}

extern int
qread(MYFILE *infile, char *a, int n)
{
    if(myread(infile, a, n) != n) {
        shError("Cant read file was not compressed");
	return(SH_GENERIC_ERROR);
    }
    return(SH_SUCCESS);
}

/*
 * read n bytes from file into buffer
 * returns number of bytes read (=n) if successful, <=0 if not
 */
extern int
myread(MYFILE *file, char buffer[], int n)
{
#ifdef TO_A_BUFFER
    /*
     * this version used when doing I/O to a buffer
     */
    if (file->current + n > file->end) return(-1);
    memcpy(buffer, file->current, n);
    file->current += n;
    return(n);
#else
    /*
     * this version used when doing I/O to a file
     *
     * this version is for VMS C: each read may return
     * fewer than n bytes, so multiple reads may be needed
     * to fill the buffer.
     *
     * I think this is unnecessary for Sun Unix, but it won't hurt
     * either, so I'll leave it in.
     */
    int nread, total;
    /* keep reading until we've read n bytes */
    total = 0;
    while ( (nread = fread(&buffer[total], 1, n-total, file)) > 0) {
        total += nread;
        if (total==n) return(total);
    }
    /* end-of-file or error occurred if we got to here */
    return(nread);
#endif
}
