/*
 * <AUTO>
 *
 * FILE: knowobj.c
 * 
 * DESCRIPTION:
 * Support for KNOWNOBJ structure
 *
 * </AUTO>
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "dervish.h"
#include "phKnownobj.h"


/***************************************************************************
 * <AUTO EXTRACT>
 *
 * create a new KNOWNOBJ structure and return a pointer to it.
 */
KNOWNOBJ *
phKnownobjNew(void)
{

  KNOWNOBJ *new = shMalloc(sizeof(KNOWNOBJ));

  strncpy(new->catalog, "", MAXNAME);
  strncpy(new->id, "", MAXNAME);
  new->class = TYPE_UNKNOWN;
  new->ra = 1000.;
  new->raErr = 1000.;
  new->dec = 1000.;
  new->decErr = 1000.;
  new->mag = 0;
  new->magErr = 0;
  new->raSize = 0.;
  new->decSize = 0;
  new->axRatio = 0;
  new->row = 0;
  new->rowErr = 0;
  new->col = 0;
  new->colErr = 0;
  new->nrow = 0;
  new->ncol =0;
  new->objc_id = -1;

  return(new);
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * delete the given KNOWNOBJ
 */
void
phKnownobjDel(KNOWNOBJ *knownobj) 
{
   if(knownobj != NULL) {
      shFree(knownobj);
   }
}

/*****************************************************************************/
/*
 * some routines to provide fast matching of catalogs with objects. The
 * relative astrometry is assumed known
 */
#define LSIZE 200			/* maximum line length */
#define NFIELD 3			/* number of required fields */

static float bucketsize;		/* size of row, column buckets */
static int info_nobj;			/* number of objects */
static int infosize = 0;		/* number of INFO_OBJs allocated */
static INFO_OBJ **objinfo = NULL;	/* pointers to INFO_OBJs */
static INFO_OBJ *objinfo_s = NULL;	/* storage for INFO_OBJs */

static int
compar(const void *aa, const void *bb)
{
   const INFO_OBJ *a = *(const INFO_OBJ **)aa;
   const INFO_OBJ *b = *(const INFO_OBJ **)bb;

   return(memcmp(a->key, b->key, 2*sizeof(int)));
}

/*
 * <AUTO EXTRACT>
 *
 * Read a file of information about objects, and prepare to match against it.
 *
 * Returns SH_SUCCESS or SH_GENERIC_ERROR
 */
int
phInfofileRead(const char *file,	/* file to read */
	       const char *fmt,		/* format to use */
	       const int *which,	/* order of row, col, etc. */
	       float binsize)		/* binning to use for sorting objects*/
{
   FILE *fd;				/* file descriptor */
   int i;
   INFO_OBJ *info;			/* info on an object */
   char line[LSIZE];			/* line read from file */
   int lineno;				/* line number in file */
   char *lptr;				/* pointer to line */
   int nfield;				/* number of fields to read */
   int nobj;				/* number of objects */
   float t[NFIELD + INFO_OBJ_NPROP];	/* temp variables for reading file */

   if(sizeof(binsize) < 0.01) {
      shErrStackPush("phInfofileRead: binsize must be >= 0.01");
      return(SH_GENERIC_ERROR);
   }

   for(i = 0;i < NFIELD; i++) {
      if(which[i] - 1 < 0 || which[i] - 1 >= NFIELD + INFO_OBJ_NPROP) {
	 shErrStackPush("phInfofileRead: "
			"required \"which\" fields must be in 1..%d (saw %d)",
					    NFIELD + INFO_OBJ_NPROP, which[i]);
	 return(SH_GENERIC_ERROR);
      }
   }

   for(;i < NFIELD + INFO_OBJ_NPROP; i++) {
      if(which[i] < 0) {
	 break;
      }
      
      if(which[i] == 0 || which[i] - 1 >= NFIELD + INFO_OBJ_NPROP) {
	 shErrStackPush("phInfofileRead: "
			"optional \"which\" fields must be in 1..%d (saw %d)",
					    NFIELD + INFO_OBJ_NPROP, which[i]);
	 return(SH_GENERIC_ERROR);
      }
   }
   nfield = i;
/*
 * Get the storage for objects, and prepare to read the file
 */
   bucketsize = binsize;
   
   if((fd = fopen(file,"r")) == NULL) {
      shErrStackPush("phInfofileRead: Can't open %s",file);
      return(SH_GENERIC_ERROR);
   }

   for(lineno = 1, nobj = 0;fgets(line, LSIZE, fd) != NULL; lineno++) {
      for(lptr = line; isspace(*lptr); lptr++) ;
      if(*lptr == '\0' || *line == '#') {
	 continue;
      }

      shAssert(INFO_OBJ_NPROP == 10);	/* as hard-wired into sscanf call */
      if(sscanf(lptr, fmt, &t[0], &t[1], &t[2],
		           &t[3], &t[4], &t[5], &t[6], &t[7],
		           &t[8], &t[9], &t[10], &t[11], &t[12]) != nfield) {
	 shError("phInfofileRead: failed to read %s, line %d", file, lineno);
	 continue;
      }

      if(nobj >= infosize) {		/* need some/more space */
	 infosize = (infosize == 0) ? 100 : 2*infosize;
	 objinfo_s = shRealloc(objinfo_s, infosize*sizeof(INFO_OBJ));
      }

      info = &objinfo_s[nobj++];

      info->row =  t[which[0]-1];
      info->col =  t[which[1]-1];
      info->key[0] = (int)(info->row/binsize + 0.5);
      info->key[1] = (int)(info->col/binsize + 0.5);
      info->mag  = t[which[2]-1];
      
      for(i = 0; i < nfield; i++) {
	 info->props[i - NFIELD] = t[which[i] - 1];
      }
   }
/*
 * sort that array
 */
   info_nobj = nobj;			/* copy to global */
   
   objinfo = shRealloc(objinfo, nobj*sizeof(INFO_OBJ *));
   for(i = 0; i < nobj;i++) {
      objinfo[i] = &objinfo_s[i];
   }
   qsort(objinfo, nobj, sizeof(INFO_OBJ *), compar);
/*
 * cleanup
 */
   fclose(fd);

   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Copies the elements of a chain of INFO_OBJs into infofile's private
 * storage, and prepares to use them for matching
 *
 * The chain may be safely destroyed when this routine has returned
 */
int
phInfofileSetFromChain(const CHAIN *chain, /* Chain of INFO_OBJs */
		       float binsize)	/* binning to use for sorting objects*/
{
   const INFO_OBJ *cinfo;		/* an element on the chain */
   int i;
   int nobj;				/* number of objects */

   shAssert(chain != NULL && chain->type == shTypeGetFromName("INFO_OBJ"));

   if(sizeof(binsize) < 0.01) {
      shErrStackPush("phInfofileRead: binsize must be >= 0.01");
      return(SH_GENERIC_ERROR);
   }
/*
 * Check that we have enough storage, and copy chain elements into array
 * belonging to infofile code
 */
   bucketsize = binsize;

   nobj = chain->nElements;
   if(nobj >= infosize) {		/* need some/more/less space */
      infosize = nobj;
      objinfo_s = shRealloc(objinfo_s, infosize*sizeof(INFO_OBJ));
      objinfo = shRealloc(objinfo, nobj*sizeof(INFO_OBJ *));
   }

   for(i = 0; i < nobj; i++) {
      cinfo = shChainElementGetByPos(chain, i);
      objinfo_s[i] = *cinfo;
      objinfo_s[i].key[0] = (int)(cinfo->row/binsize + 0.5);
      objinfo_s[i].key[1] = (int)(cinfo->col/binsize + 0.5);
   }
/*
 * sort that array
 */
   info_nobj = nobj;			/* copy to global */
   
   for(i = 0; i < nobj;i++) {
      objinfo[i] = &objinfo_s[i];
   }
   qsort(objinfo, nobj, sizeof(INFO_OBJ *), compar);

   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Free memory allocated by phInfofileRead
 */
void
phInfofileFini(void)
{
   if(infosize > 0) {
      shFree(objinfo); objinfo = NULL;
      shFree(objinfo_s); objinfo_s = NULL;
      infosize = 0;
   }
}

/*****************************************************************************/

static int
find_bucket(int key[2],			/* desired key */
	    int i0)			/* initial guess for index */
{
   const int *km;			/* key of mid's object */
   int lo, hi;				/* range on candidate buckets */
   int mid;				/* mid point of lo and hi */
   const INFO_OBJ *const *const objs = (const INFO_OBJ **)objinfo; /* unalias*/
/*
 * start looking for desired bucket at i0, if >= 0
 */
   if(i0 < 0) {				/* no initial guess */
      lo = 0; hi = info_nobj - 1;
   } else {
      unsigned int step = 1;		/* how much to step up/down */

      if(i0 > info_nobj - 1) i0 = info_nobj - 1;
      
      if(memcmp(key, objs[i0]->key, 2*sizeof(int)) > 0) { /* expand upwards */
	 if(i0 == info_nobj - 1) {	/* off top of array */
	    return(info_nobj - 1);
	 }

	 lo = i0; hi = lo + 1;
	 while(memcmp(key, objs[hi]->key, 2*sizeof(int)) >= 0) {
	    lo = hi;
	    step += step;		/* double step size */
	    hi = lo + step;
	    if(hi >= info_nobj) {	/* reached top of array */
	       hi = info_nobj - 1;
	       break;
	    }
	 }
      } else {				/* expand it downwards */
	 if(i0 == 0) {			/* off bottom of array */
	    return(-1);
	 }

	 hi = i0; lo = i0 - 1;
	 while(memcmp(key, objs[lo]->key, 2*sizeof(int)) < 0) {
	    hi = lo;
	    step += step;		/* double step size */
	    lo = hi - step;
	    if(lo < 0) {		/* off bottom of array */
	       lo = -1;
	       break;
	    }
	 }
      }
   }
/*
 * Find bucket by binary search
 */
   while(hi - lo > 1) {
      mid = (lo + hi)/2;
      km = objs[mid]->key;

      if(memcmp(key, km, 2*sizeof(int)) <= 0) {
	 hi = mid;
      } else {
	 lo = mid;
      }
   }
/*
 * If there are many entries with the same pair of keys, choose first
 */
   while(hi > 0 && memcmp(key, objs[hi - 1]->key, 2*sizeof(int)) == 0) {
      hi--;
   }

   return(hi);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Return a best-match INFO_OBJ
 */
const INFO_OBJ *
phInfofileMatch(float row, float col,	/* position */
		float dmax,		/* maximum allowable separation */
		float *dist)		/* distance of matched object */
{
   float d2;				/* separation^2 */
   const float dmax2 = dmax*dmax;	/* == dmax^2 */
   float dist2;				/* separation for brightest match */
   int i;
   int key[2];				/* key for desired position */
   float magmin;			/* minimum magnitude of matched obj */
   int match;				/* index of best match, or -1 */
   const INFO_OBJ *const *const objs = (const INFO_OBJ **)objinfo; /* unalias*/
   float r, c;				/* LL corner of a bucket */

   if(infosize == 0) {
      if(dist != NULL) {
	 *dist = -1;
      }
      return(NULL);
   }
/*
 * match in the catalog
 */
   match = -1;				/* no match */
   dist2 = magmin = 1e30;

   i = -1;				/* no initial guess for index */
   for(r = row - dmax; r <= row + dmax; r += bucketsize) {
      for(c = col - dmax; c <= col + dmax; c += bucketsize) {
/*
 * find appropriate bucket
 */
	 key[0] = (r < 0) ? 0 : (int)(r/bucketsize + 0.5);
	 key[1] = (c < 0) ? 0 : (int)(c/bucketsize + 0.5);

	 if((i = find_bucket(key, i)) < 0) {
	    continue;
	 }
/*
 * and search it for the desired object
 */
	 for(; i < info_nobj &&
			  memcmp(key, objs[i]->key, 2*sizeof(int)) == 0; i++) {
	    float dr = objs[i]->row - row;
	    float dc = objs[i]->col - col;

	    d2 = dr*dr + dc*dc;

	    if(d2 > dmax2) {
	       continue;
	    }

	    if(objs[i]->mag < magmin) {
	       magmin = objs[i]->mag;
	       dist2 = d2;
	       match = i;
	    }
	 }

      }
   }
   
   if(dist != NULL) {
      *dist = sqrt(dist2);
   }

   return(match == -1 ? NULL : objs[match]);
}
