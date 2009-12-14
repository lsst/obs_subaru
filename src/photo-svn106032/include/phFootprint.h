#if !defined(PH_FOOTPRINT_H)
#define PH_FOOTPRINT_H

#include <dervish.h>

/*
 * Support for basically Pan-STARRS data objects that we use;
 * see note at top of footprint.c
 */
#define TYPEDEF typedef                 /* fool diskio_gen.c */
TYPEDEF void (*psArrayDelFunc)(void *); /* function called to delete array elements */
#undef TYPEDEF

typedef struct {
    int n;                              /* number of elements in use */
    int size;                           /* number of elements allocated */
    void **data;                        /* the actual data */
    psArrayDelFunc del;           	/* function called to delete array elements */
} psArray;                              /* pragma SCHEMA */

psArray *psArrayNew(const int n, psArrayDelFunc del);
psArray *psArrayNewEmpty(const int size, psArrayDelFunc del);
psArray *psArrayRealloc(psArray *arr, const int n);
void psArrayDel(psArray *arr);
void psArrayAdd(psArray *arr, const int dsize, void *elem);
void *psArrayRemoveIndex(psArray *arr, const int i);
psArray *psArraySort(psArray *arr, int (*compar)(void *a, void *b));

typedef struct {
    int x0, x1, y0, y1;                 /* corner pixels of a subregion */
} psRegion;                             /* pragma SCHEMA */

/*
 *  Describe a segment of an image
 */
typedef struct {
    int y;                              /* Row that span's in */
    int x0;                             /* Starting column (inclusive) */
    int x1;                             /* Ending column (inclusive) */
} phSpan;                               /* pragma SCHEMA */

phSpan *phSpanNew(int y, int x1, int x2);
void phSpanDel(phSpan *span);
int phSpanSortByYX (const void **a, const void **b);

typedef struct {
    const int id;                       /* unique ID */
    int npix;                           /* number of pixels in this phFootprint */
    psArray *spans;                     /* the phSpans */
    psRegion bbox;                      /* the phFootprint's bounding box */
    psArray *peaks;                     /* the peaks lying in this footprint */
    psRegion region;   /* A region describing the REGION the footprints live in */
    int normalized;                    /* Are the spans sorted?  */
} phFootprint;                          /* pragma SCHEMA */

phFootprint *phFootprintNew(int nspan, const REGION *img);
void phFootprintDel(phFootprint *footprint);
phFootprint *phFootprintNormalize(phFootprint *fp);
int phFootprintSetNpix(phFootprint *fp);
void phFootprintSetBBox(phFootprint *fp);
psArray *phFindFootprints(const REGION *img, const float threshold, const int npixMin);
phFootprint *phFindFootprintAtPoint(const REGION *img,
                                    const float threshold,
                                    const psArray *peaks,
                                    int row, int col);

REGION *phSetFootprintArrayIDs(const psArray *footprints, const int relativeIDs);
REGION *phSetFootprintID(const phFootprint *fp, const int id);

int phFootprintArrayCullPeaks(const REGION *img, const float stdev, psArray *footprints,
                                 const float nsigma, const float threshold_min);
int phFootprintCullPeaks(const REGION *img, const float stdev, phFootprint *fp,
                                 const float nsigma, const float threshold_min);

#endif
