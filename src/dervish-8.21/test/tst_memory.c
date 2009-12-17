/*
 * Test memory allocation
 *
 * Vijay K. Gurbani
 */
#include <stdio.h>
#include <string.h>

#include "shCGarbage.h"
#include "shCUtils.h"
#include "tst_utils.h"
#include "prvt/utils_p.h"

static int  testCounters(void);
static int  testShMalloc(void);
static int  testIntegrity(void);
static int  testMath(void);
static int  testRefCount(void);
static int  testBlowAway(void);
static int  testCB(void);
static int  testRealloc(void);
static int  testShCalloc(void);

static void callBack(unsigned long, const SH_MEMORY *);
static unsigned long callBackData = 0L;

static struct  {
   int  (*funcp)(void);
   char *pFuncName;
} TestFuncs[] = { 
                  {testIntegrity, "testIntegrity()"},
                  {testRefCount,  "testRefCount()"},
                  {testMath,      "testMath()"},
                  {testShMalloc,  "testShMalloc()"},
                  {testCounters,  "testCounters()"},
                  {testBlowAway,  "testBlowAway()"},
                  {testCB,        "testCB()"},
                  {testRealloc,   "testRealloc()"},
		  {testShCalloc,  "testShCalloc()"},
                  {NULL,          NULL} 
                };

int main(int argc, char *argv[])
{
   int i;

   INIT_PROTECT;

   for (i = 0; TestFuncs[i].funcp != NULL; i++)
        if (TestFuncs[i].funcp())  {
            fprintf(stderr, "tst_memory: Failed in function %s\n", 
                    TestFuncs[i].pFuncName);
            return 1;
        }

   return 0;
}

static int testRealloc(void)
{
   char *sp1;
   char *string1 = "This is a huge string............................";
   char *string2 = "This is a huge";
   int  *ip;   
   int retValue;

   retValue = 0;
   ip = NULL;

   sp1 = (char *) shMalloc(200);
   strcpy(sp1, string1);
   sp1 = (char *) shRealloc(sp1, 5000);
   if (strcmp(sp1, string1) != 0)  {
       fprintf(stderr, "testRealloc(): expected shRealloc() to preserve"
               " memory contents. %s != %s\n", sp1, string1);
       retValue = 1;
   }

   shFree(sp1);

   sp1 = (char *) shMalloc(strlen(string1)+1);
   strcpy(sp1, string1);
   sp1 = (char *) shRealloc(sp1, strlen(string2)+1);
   if ((strlen(sp1) != 15) && (strncmp(sp1, string1, strlen(sp1)) != 0))  {
        fprintf(stderr, "testRealloc(): expected shRealloc() to preserve"
                " memory contents to the lesser\nof two lengths - 1\n");
        retValue = 1;
   }

   ip = (int *) shMalloc(sizeof(int) * 3);
   ip[0] = 1; ip[1] = 2; ip[2] = 3;
   ip = (int *) shRealloc(ip, sizeof(int) * 2);
   if (ip[0] != 1 && ip[1] != 2)  {
       fprintf(stderr, "testRealloc(): expected shRealloc() to preserve"
               " memory contents to the lesser\nof two lengths - 2\n");
       retValue = 1;
   }   

   shFree(sp1);
   if (ip)
       shFree(ip);

   return retValue;

}

static void callBack(unsigned long thresh, const SH_MEMORY *mem)
{
   callBackData = 1L;
}

static int testCB(void)
{
   unsigned long l;
   char *sp;
   
   l = shMemSerialNumberGet();
   l += 3;
 
   shMemSerialCB(l, callBack);

   sp = (char *) shMalloc(20);  shFree(sp);         
   sp = (char *) shMalloc(20);  shFree(sp);         
   sp = (char *) shMalloc(20);  shFree(sp);         
   sp = (char *) shMalloc(20);  shFree(sp);
   sp = (char *) shMalloc(20);  shFree(sp);
   sp = (char *) shMalloc(20);  shFree(sp);

   if (callBackData == 0L)  {
       fprintf(stderr, "testCB(): expected l to be 1, not %ld\n", l);
       return 1;
   }

   return 0; 
}


static int testBlowAway(void)
{
   void *pMem;
   int  i;
   unsigned long serial_start, serial_end,
                 bytes_start, bytes_end;

   /*
    * This function tests the "blow it all away" (shMemFreeBlocks())
    */
   serial_start = shMemSerialNumberGet();
   bytes_start  = shMemBytesInUse();
   
   for (i = 0; i < 10; i++)
        pMem = shMalloc(i * 10 + 1);

   serial_end = shMemSerialNumberGet();

   shMemFreeBlocks(serial_start, serial_end, NULL);
   bytes_end = shMemBytesInUse();

   if (bytes_end != bytes_start)  {
       fprintf(stderr, "testBlowAway(): shMemFreeBlocks() did not succeed."
               " Expected bytes_end (%ld) == bytes_start (%ld)\n",
               bytes_end, bytes_start);
       return 1;
   }

   /*
    * Now let's delete just one block and see. Let's allocate 40 bytes in 20
    * byte blocks. Then let's blow away one of the blocks. We should now have
    * only 20 bytes left in our pool.
    */
   serial_start = shMemSerialNumberGet();
   bytes_start  = shMemBytesInUse();

   (void *) shMalloc(20); (void *) shMalloc(20);

   shMemFreeBlocks(serial_start+1, serial_start+1, NULL);
   bytes_end = shMemBytesInUse();

   if (bytes_end != 20)  {
       fprintf(stderr, "testBlowAway(): shMemFreeBlocks() did not succeed."
               " Expected 20 bytes to remain in pool, not %d!!\n", bytes_end);
       return 1;
   }

   return 0;   
}

static int testCounters(void)
{
   void *pMem[10];
   int  i, size, retValue, start, end,
        start2, end2, j, start3, end3;

   /*
    * Test some counters maintained by shGarbage.c
    */

   size = 10;   /* of pMem[] */
   retValue = 0;
   j = 0;

   start3 = shMemNumFrees();
   start2 = shMemBytesInUse();
   start = shMemNumMallocs();

   for (i = 0; i < size; i++)  {
        pMem[i] = shMalloc(i * 20 + 1);
        j += (i * 20 + 1);
   }

   end = shMemNumMallocs();
   end2 = shMemBytesInUse();


   if ((end - start) != size)  {
       retValue = 1;
       fprintf(stderr, "testCounters(): expected %d shMallocs, got %d\n",
               size, end - start);
   }

   if ((end2 - start2) != j)  {
       retValue = 1;
       fprintf(stderr, "testCounters(): expected %d bytes malloced, got %d\n",
               j, end2 - start2);
   }

   for (i = 0; i < size; i++)
        shFree(pMem[i]);

   end3 = shMemNumFrees();

   if ((end3 - start3) != size)  {
       retValue = 1;
       fprintf(stderr, "testCounters(): expected %d shFrees, got %d\n",
               size, end3 - start3);
   }

   return retValue;
}

static int testShMalloc(void)
{
   void *pMemBlock;

   pMemBlock = shMalloc(200);
   shFree(pMemBlock);

   START_PROTECT;
   shFree(pMemBlock);  /* should generate a core dump... */
   END_PROTECT;
   report("above shFatal() in testShMalloc() ok - attempt to free "
          "memory already freed\n");

   /*
    * Okay, now malloc() a memory block and attempt to free it using shFree()
    */
   pMemBlock = malloc(200);

   START_PROTECT;
   shFree(pMemBlock);   /* Should generate a core dump... */
   END_PROTECT;
   report("above shFatal() in testShMalloc() ok - attempt to free memory "
          "not allocated by shMalloc()\n");

   free(pMemBlock);

   shFree(NULL);   /* should be OK - NO core dumps */

   START_PROTECT;
   pMemBlock = (void *) 0x1;
   shFree(pMemBlock);  
   END_PROTECT;
   report("above shFatal() in testShMalloc() okay - attempt to free 0x1\n");

   return 0;

}

static int testShCalloc(void)
{
  char *pMemBlock;
  int i;
  int size;
  int fail = 0;
  
  size = 200000;
  
  pMemBlock = (char*)shCalloc(size,1);

  for (i=0; i<size; i++) {
    if ( pMemBlock[i] != 0 ) {
      fail = 1;
    }
  }
  if ( fail != 0 ) {
         fprintf(stderr, "testCalloc(): shCalloc did not zero out all the memory\n");
  }

  shFree(pMemBlock);

  return fail;

}

static int testMath(void)
{
   int    *ip1[2], *ip2[2], i_result[2];
   long   *lp1[2], *lp2[2], l_result[2];
   short  *sp1[2], *sp2[2], s_result[2];
   float  *fp1[2], *fp2[2], f_result[2];
   double *dp1[2], *dp2[2], d_result[2];
   unsigned long start, end;

   /*
    * This function should also test the alignment. Since we do some extensive
    * math here, if the alignment is wrong, we should abort with a SIGBUS
    * violation.
    */

   start = shMemSerialNumberGet();

   ip1[0] = (int *)malloc(sizeof(int)); ip1[1] = (int *)malloc(sizeof(int));
   ip2[0] = (int *)shMalloc(sizeof(int)); 
   ip2[1] = (int *)shMalloc(sizeof(int));

   *ip1[0] = *ip2[0] = 1211; *ip1[1] = *ip2[1] = 9181;

   i_result[0] = *ip1[0] + *ip1[1];
   i_result[1] = *ip2[0] + *ip2[1];

   if (i_result[0] != i_result[1])  {
       fprintf(stderr, "testMath: Results of int addition are different!\n");
       return 1;
   }

   i_result[0] = *ip1[0] - *ip1[1];
   i_result[1] = *ip2[0] - *ip2[1];

   if (i_result[0] != i_result[1])  {
       fprintf(stderr, 
               "testMath: Results of int subtraction  are different!\n");
       return 1;
   }

   i_result[0] = *ip1[0] / *ip1[1];
   i_result[1] = *ip2[0] / *ip2[1];

   if (i_result[0] != i_result[1])  {
       fprintf(stderr, "testMath: Results of int division are different!\n");
       return 1;
   }

   i_result[0] = *ip1[0] * *ip1[1];
   i_result[1] = *ip2[0] * *ip2[1];

   if (i_result[0] != i_result[1])  {
       fprintf(stderr, 
               "testMath: Results of int multiplication are different!\n");
       return 1;
   }

   free(ip1[0]);   free(ip1[1]);

   /* -------------------------------------------------------------------- */

   lp1[0] = (long *)malloc(sizeof(long)); 
   lp1[1] = (long *)malloc(sizeof(long));
   lp2[0] = (long *)shMalloc(sizeof(long)); 
   lp2[1] = (long *)shMalloc(sizeof(long));

   *lp1[0] = *lp2[0] = 819871L; *lp1[1] = *lp2[1] = 801L;

   l_result[0] = *lp1[0] + *lp1[1];
   l_result[1] = *lp2[0] + *lp2[1];

   if (l_result[0] != l_result[1])  {
       fprintf(stderr, "testMath: Results of long addition are different!\n");
       return 1;
   }

   l_result[0] = *lp1[0] - *lp1[1];
   l_result[1] = *lp2[0] - *lp2[1];

   if (l_result[0] != l_result[1])  {
       fprintf(stderr, 
               "testMath: Results of long subtraction  are different!\n");
       return 1;
   }

   l_result[0] = *lp1[0] / *lp1[1];
   l_result[1] = *lp2[0] / *lp2[1];

   if (l_result[0] != l_result[1])  {
       fprintf(stderr, "testMath: Results of long division are different!\n");
       return 1;
   }

   l_result[0] = *lp1[0] * *lp1[1];
   l_result[1] = *lp2[0] * *lp2[1];

   if (l_result[0] != l_result[1])  {
       fprintf(stderr, 
               "testMath: Results of long multiplication are different!\n");
       return 1;
   }

   free(lp1[0]);   free(lp1[1]);

   /* -------------------------------------------------------------------- */

   sp1[0] = (short *)malloc(sizeof(short)); 
   sp1[1] = (short *)malloc(sizeof(short));
   sp2[0] = (short *)shMalloc(sizeof(short)); 
   sp2[1] = (short *)shMalloc(sizeof(short));

   *sp1[0] = *sp2[0] = -29; *sp1[1] = *sp2[1] = 819;

   s_result[0] = *sp1[0] + *sp1[1];
   s_result[1] = *sp2[0] + *sp2[1];

   if (s_result[0] != s_result[1])  {
       fprintf(stderr, "testMath: Results of short addition are different!\n");
       return 1;
   }

   s_result[0] = *sp1[0] - *sp1[1];
   s_result[1] = *sp2[0] - *sp2[1];

   if (s_result[0] != s_result[1])  {
       fprintf(stderr, 
               "testMath: Results of short subtraction are different!\n");
       return 1;
   }

   s_result[0] = *sp1[0] / *sp1[1];
   s_result[1] = *sp2[0] / *sp2[1];

   if (s_result[0] != s_result[1])  {
       fprintf(stderr, "testMath: Results of short division are different!\n");
       return 1;
   }

   s_result[0] = *sp1[0] * *sp1[1];
   s_result[1] = *sp2[0] * *sp2[1];

   if (s_result[0] != s_result[1])  {
       fprintf(stderr, 
               "testMath: Results of short multiplication are different!\n");
       return 1;
   }

   free(sp1[0]);   free(sp1[1]);

   /* -------------------------------------------------------------------- */

   fp1[0] = (float *)malloc(sizeof(float));
   fp1[1] = (float *)malloc(sizeof(float));
   fp2[0] = (float *)shMalloc(sizeof(float));
   fp2[1] = (float *)shMalloc(sizeof(float));

   *fp1[0] = *fp2[0] = 81981.11; *fp1[1] = *fp2[1] = 1012.91;

   f_result[0] = *fp1[0] + *fp1[1];
   f_result[1] = *fp2[0] + *fp2[1];

   if (f_result[0] != f_result[1])  {
       fprintf(stderr, "testMath: Results of float addition are different!\n");
       return 1;
   }

   f_result[0] = *fp1[0] - *fp1[1];
   f_result[1] = *fp2[0] - *fp2[1];

   if (f_result[0] != f_result[1])  {
       fprintf(stderr, 
               "testMath: Results of float subtraction are different!\n");
       return 1;
   }

   f_result[0] = *fp1[0] / *fp1[1];
   f_result[1] = *fp2[0] / *fp2[1];

   if (f_result[0] != f_result[1])  {
       fprintf(stderr, "testMath: Results of float division are different!\n");
       return 1;
   }

   f_result[0] = *fp1[0] * *fp1[1];
   f_result[1] = *fp2[0] * *fp2[1];

   if (f_result[0] != f_result[1])  {
       fprintf(stderr, 
               "testMath: Results of float multiplication are different!\n");
       return 1;
   }

   free(fp1[0]);   free(fp1[1]);

   /* -------------------------------------------------------------------- */

   dp1[0] = (double *)malloc(sizeof(double));
   dp1[1] = (double *)malloc(sizeof(double));
   dp2[0] = (double *)shMalloc(sizeof(double));
   dp2[1] = (double *)shMalloc(sizeof(double));

   *dp1[0] = *dp2[0] = 41541.919112; *dp1[1] = *dp2[1] = 187181.2212;

   d_result[0] = *dp1[0] + *dp1[1];
   d_result[1] = *dp2[0] + *dp2[1];

   if (d_result[0] != d_result[1])  {
       fprintf(stderr, 
               "testMath: Results of double addition are different!\n");
       return 1;
   }

   d_result[0] = *dp1[0] - *dp1[1];
   d_result[1] = *dp2[0] - *dp2[1];

   if (d_result[0] != d_result[1])  {
       fprintf(stderr, 
               "testMath: Results of double subtraction are different!\n");
       return 1;
   }

   d_result[0] = *dp1[0] / *dp1[1];
   d_result[1] = *dp2[0] / *dp2[1];

   if (d_result[0] != d_result[1])  {
       fprintf(stderr, 
               "testMath: Results of double division  are different!\n");
       return 1;
   }

   d_result[0] = *dp1[0] * *dp1[1];
   d_result[1] = *dp2[0] * *dp2[1];

   if (d_result[0] != d_result[1])  {
       fprintf(stderr, 
               "testMath: Results of double multiplication are different!\n");
       return 1;
   }

   free(dp1[0]);   free(dp1[1]);

   end = shMemSerialNumberGet();

   shMemFreeBlocks(start, end, NULL);

   return 0;
}

static int testRefCount(void)
{
   char *sp;

   sp = (char *)shMalloc(30);

   strcpy(sp, "Hello World!");

   shMemRefCntrIncr(sp);
   shMemRefCntrIncr(sp);
   shMemRefCntrIncr(sp);

   shFree((void *)sp);   /* The next three shFree()s should simply */
   shFree((void *)sp);   /* decrement the reference counter. The   */
   shFree((void *)sp);   /* block pointer to by sp still exists    */

   shFree((void *)sp);   /* Now we actually free the block pointed */
                         /* to by sp                               */
   START_PROTECT;
   shFree((void *)sp);   /* Should generate a core dump... */
   END_PROTECT;
   report("above shFatal() in testRefCount() expected\n");

   return 0;
}

static int testIntegrity(void)
{
   char *pShMalloc,
        *pSystemMalloc;
   int  i, 
        retValue;

   pSystemMalloc = (char *) malloc(30);
   pShMalloc     = (char *) shMalloc(30);

   retValue = 0;

   i = shIsShMallocPtr(pSystemMalloc);
   if (i)  {
       printf("shIsShMallocPtr(%p) returns %d! Should return 0!!\n", 
              pSystemMalloc, i);
       retValue = 1;
   }

   i = shIsShMallocPtr(pShMalloc);
   if (i == 0)  {
       printf("shIsShMallocPtr(%p) return %d! Should return 1!!\n",
              pShMalloc, i);
       retValue = 1;
   }

   free(pSystemMalloc);
   shFree(pShMalloc);

   return retValue;
}
   


