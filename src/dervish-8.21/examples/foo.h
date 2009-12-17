#if !defined(FOO_H)
#define FOO_H
#include "shCList.h"

#define FSIZE 10			/* size of FOO.arr */
#define NGOO 2				/* dimensions for FOO.goo */
/*
 * Type definitions used in testing schema and diskio code
 */
typedef struct {			/* a non-trivial thing to have an
					   array of in FOO */
   float f[2];
} GOO;

typedef struct foo {
   TYPE type;
   struct foo *next, *prev;
   char *name;
   int i;
   long l;
   float f;
   int arr[FSIZE];
   int arr2[3][FSIZE];
   GOO goo[NGOO][NGOO];
} FOO;

typedef struct {
   int nfoo;				/* number of FOOs */
   FOO **foo;				/* an array of FOOs */
   float f1;				/* misc other stuff */
   int i1,i2;
   float f2;
} BAR;					/*
					  pragma USER
					 */

BAR *tsBarNew(int n);
FOO *tsFooNew(void);
GOO *tsGooNew(void);
RET_CODE tsBarDel(BAR *bar);
RET_CODE tsFooDel(FOO *foo);
RET_CODE tsGooDel(GOO *goo);

RET_CODE tsDumpBarRead(FILE *fil, void **home);
RET_CODE tsDumpBarWrite(FILE *fil, BAR *bar);

#endif
