#if !defined(ATPMATCH_H)
#define ATPMATCH_H

/*
 * <AUTO>
 *
 * FILE: atPMatch.h
 * 
 * DESCRIPTION:
 * Definitions for structures used in the matching of two CHAINs
 * of objects to find the transformation that takes one to the other.
 *
 * The algorithms used are based on the paper by Valdes et al.,
 * PASP 107, 1119 (1995).
 *
 */

   /*
    * any warning messages printed with "shDebug" will come out
    * at this error level 
    */
#define AT_MATCH_ERRLEVEL   2

   /* 
    * this is the default radius for matching triangles in triangle space.
    * The units are NOT pixels, or arcseconds, but a fraction of the 
    * range of normalized ratios of side lengths in a triangle, 0.0 to 1.0.
    *
    * This is the default value.  The user can override it with an optional
    * command-line argument.
    */
#define AT_MATCH_RADIUS  0.002    

   /*
    * This is the largest permitted distance between the coords of
    * a matched pair of stars in the common coord system, after
    * we've made an attempt at transforming them.  This value
    * is used in the "iter_trans" procedure.  
    *
    * Like "AT_MATCH_RADIUS", above, it assumes the second list has
    * units close to the size of a star.
    *
    * This is the default value, but can be overridden with the
    * optional command-line argument "maxdist"
    */
#define AT_MATCH_MAXDIST  500.0


   /*
    * as a first step in the matching process, we sort each group of
    * stars by magnitude and select the AT_MATCH_NBRIGHT brightest items; 
    * these are used to find a rough coordinate transformation.
    *
    * This is the default value.  The user can override it with an optional
    * command-line argument.
    */
#define AT_MATCH_NBRIGHT   20

   /*
    * at the very start of the iterative procedure which calculates
    * the final transformation, we (attempt to) throw out any
    * pairs whose number of votes is less than AT_MATCH_QUALIFY
    * times the top number of votes.  For example, if the best
    * pair of points had 100 votes, and AT_MATCH_QUALIFY=0.50,
    * we'd throw out any pairs with < 50 votes.
    *
    * Valdes et al. state that they achieve satisfactory results
    * with AT_MATCH_QUALIFY=0.0, but I have found that when
    * two catalogs come from different passbands (and so don't
    * really match up that well in magnitude), this can help.
    */
#define AT_MATCH_QUALIFY  0.33

   /*
    * ignore all triangles which have (b/a) > AT_MATCH_RATIO when 
    * trying to match up sets of triangles.  If AT_MATCH_RATIO
    * is set to 1.0, then all triangles will be used.
    */
#define AT_MATCH_RATIO    0.9

   /*
    * We require AT LEAST this many matched pairs of stars to 
    * calculate a TRANS structure; fail if we are given fewer,
    * or if we discard so many that we have fewer than this many
    */
#define AT_MATCH_REQUIRE    3

   /*
    * We start with the top "AT_MATCH_STARTN" candidates of
    * matched pairs, when we enter the iterative process
    * of finding a TRANS.  Must be >= AT_MATCH_REQUIRE
    */
#define AT_MATCH_STARTN     6

   /*
    * when iterating to throw out mis-matched pairs of stars,
    * use this percentile in a sorted array of discrepancies
    * as an effective "sigma".  Throw out any pairs with 
    * discrepancy more than 2*"sigma".
    */
#define AT_MATCH_PERCENTILE   0.60

   /* If desired, when comparing two triangles, we count them as a match
    * only if the ratio of their sides (indicated by "a_length")
    * is within this many percent of an expected value.
    * The default is to allow any ratio of triangle sizes.
    *
    * A value of "10" means "ratio must match expected value to 10 percent"
    */
#define AT_MATCH_PERCENT      10



   /* 
    * these functions are PUBLIC, and may be called by users
    */

int atFindTrans(CHAIN *chainA, char *xnameA, char *ynameA, char *magnameA,
                CHAIN *chainB, char *xnameB, char *ynameB, char *magnameB, 
                float radius, int nbright, float maxdist, float scale,
                TRANS *trans);

int atApplyTrans(CHAIN *chain, char *xname, char *yname, TRANS *trans);

int atMatchChains(CHAIN *chainA, char *xnameA, char *ynameA,
                  CHAIN *chainB, char *xnameB, char *ynameB,
                  float radius, 
                  CHAIN **listJ, CHAIN **listK, CHAIN **listL, CHAIN **listM);



/*                    end of PUBLIC information                          */
/*-----------------------------------------------------------------------*/
/*                  start of PRIVATE information                         */

   /*
    * the following structures are used by functions internal
    * to the matching process, and shouldn't be accessed by
    * users or other functions.
    */

   /* 
    * this holds information on a single star (or object) 
    */
typedef struct s_star {
   int id;                 /* used for internal debugging purposes only */
   int index;              /* position of this star in its linked list */
   float x;                /* star's "X" coordinate */
   float y;                /* star's "Y" coordinate */
   float mag;              /* some measure of star's brightness */
   int match_id;           /* ID of star in other list which matches */
   struct s_star *next;    /* we use linked lists internally */
} s_star;                  


   /* 
    * this holds information on triangles, used internally for matching.
    * note that when a triangle is formed, the vertices are identified
    * so that
    *           side a = dist(bc)    is the longest side
    *                b = dist(ac)    is the second-longest side
    *                c = dist(ab)    is the shortest side
    */
typedef struct s_triangle {
   int id;                  /* used for internal debugging purposes only */
   int index;               /* position of this triangle in its linked list */
   float a_length;          /* length of side a (not normalized) */
   float ba;                /* ratio of lengths b/a   ... must be 0.0-1.0 */
   float ca;                /* ratio of lengths c/a   ... must be 0.0-1.0 */
   int a_index;             /* index of the star opposite side a */
   int b_index;             /* index of the star opposite side b */
   int c_index;             /* index of the star opposite side c */
   int match_id;            /* ID of triangle in other list which matches */
   struct s_triangle *next; /* we use linked lists internally */
} s_triangle;               
   



#endif
