#if !defined(PHKNOWNOBJ_H)
#define PHKNOWNOBJ_H

#include "dervish.h"
#include "phOffset.h"

#define MAXNAME 50

typedef enum {TYPE_UNKNOWN=-1, STAR, GALAXY} KOCLASS;


/* This struct will hold the information for a known object */

typedef struct {
    char catalog[MAXNAME];	/* e.g. ugc, hst-gsc, etc*/
    char id[MAXNAME];		/* id for this object */
    int class;			/* type of object (not KOCLASS as fitsToSchema fails) */    
    double ra;
    double raErr;
    double dec;
    double decErr;
    float mag;                  /* What passband? Transformed to survey band?*/
    float magErr;
    float raSize;		/* These may need to be calculated from  */
    float decSize;		/* catalog information */
    float axRatio;
    float row;			/* These are filled by the known objects */
    float rowErr;		/* module */
    float col;
    float colErr;
    int nrow;
    int ncol;
    int objc_id;		/* id of objc containing atlas image */
} KNOWNOBJ;

/*
 * here's a catalog entry for an object
 */
#define INFO_OBJ_NPROP 10		/* number of extra properties */

typedef struct {
   int key[2];				/* key constructed from row, col */

   float row, col;			/* position of object */
   float mag;				/* object's approximate magnitude */
   float props[INFO_OBJ_NPROP];		/* other desired properties of object*/
} INFO_OBJ;				/* pragma SCHEMA */

KNOWNOBJ *phKnownobjNew(void);
void phKnownobjDel(KNOWNOBJ *knownobj);


CHAIN *
phFindKnownObjects(const CHAIN *catalog, const TRANS *frametrans,
		   const TRANS *surveytrans, int nrow, int ncol, int overlap);
CHAIN *
phCutKnownObjects(const CHAIN *knownobjs, const FIELDPARAMS *fiparams);

int
phInfofileRead(const char *file,	/* file to read */
	       const char *fmt,		/* format to use */
	       const int *which,	/* order of row, col, etc. */
	       float binsize);		/* binning to use for sorting objects*/
int
phInfofileSetFromChain(const CHAIN *chain, /* Chain of INFO_OBJs */
		       float binsize);	/* binning to use for sorting objects*/

void phInfofileFini(void);

const INFO_OBJ *
phInfofileMatch(float row, float col,	/* position */
		float dmax,		/* maximum allowable separation */
		float *dist);		/* distance of matched object */

#endif 
