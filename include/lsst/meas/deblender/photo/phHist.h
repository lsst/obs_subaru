#if !defined(PHHIST_H)
#define PHHIST_H
   /*
    * this is the structure we use for storing histograms of image data.
    * For example, given this distribution:
    *
    *         6 7 6 8 7 20 8 6 6 7
    *
    * we could construct a PHHIST with
    * 
    *    num_bin       = 15
    *    min_bin       = 6
    *    total_number  = 10
    *    value[]       = [4, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1]
    */

typedef struct {
   int num_bin;          /* number of bins (and values) in the histogram */
   int min_bin;		 /* minimum bin value; max bin = (min_bin+num_bin-1) */
   int total_number;     /* total number of entries in histogram */
                         /*   i.e. sum of all "value"s */
   int *value;           /* number of counts in each bin */
   float sum;				/* sum of all pixels; not always set */
} PHHIST;

typedef struct {
   int min_bin;				/* starting value of histogram */
   int num_bin;				/* number of DN values in histogram */
   int total_number;			/* number of pixels in histogram*/
   unsigned short *value;		/* the histogram itself */
} PHHIST_S;				/*
					   pragma NOCONSTRUCTOR
					 */


/*
 * Structure to describe a cumulative histogram.
 */

typedef struct {
   int min_bin;				/* starting value of histogram */
   int num_bin;				/* number of values */
   unsigned int *cvalue;		/* the cumulated histogram */
} PHHIST_CUMUL;				/*
					   pragma NOCONSTRUCTOR
					 */

PHHIST *phPhhistNew(int num_bin, int min_bin);
PHHIST_S *phPhhist_sNew(int num_bin, int min_bin);
PHHIST_CUMUL *phPhhist_cumulNew(int num_bin, int min_bin);

void phPhhistDel(PHHIST *hist);
void phPhhist_sDel(PHHIST_S *hist);
void phPhhist_cumulDel(PHHIST_CUMUL *hist);

#endif
