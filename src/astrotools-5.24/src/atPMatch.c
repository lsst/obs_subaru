
/*
 * <AUTO>
 * FILE: atPMatch.c
 *
 * <HTML>
 * This file contains routines that try to match up items
 * in two different CHAINs, which might have very different
 * coordinate systems. 
 * </HTML>
 *
 * </AUTO>
 *
 */

 /*
  * -------------------------------------------------------------------------
  * atFindTrans             public  Find TRANS to match coord systems of
  *                                 two CHAINs of items 
  * atApplyTrans            public  Apply a TRANS to a CHAIN of items, 
  *                                 modifying two of the elements in each item
  * atMatchChains           public  Find all pairs of items on two CHAINs
  *                                 which match (and don't match)
  *
  * All public functions appear at the start of this source-code file.
  * "Private" static functions appear following them.
  *
  * Conditional compilation is controlled by the following macros:
  *
  *    DEBUG
  *    DEBUG2
  * 
  * AUTHORS:  Creation date: Jan 22, 1996
  *           Michael Richmond
  *
  */


#include <stdio.h>
#include <math.h>           /* need this for 'sqrt' in calc_distances */
#include <string.h>
#include "dervish.h"
#include "atTrans.h"
#include "atPMatch.h"

#undef  DEBUG            /* get some of diagnostic output */
#undef  DEBUG2           /* get LOTS more diagnostic output */


   /*
    * The following #define values are used to identify the type 
    * of a schema element in functions
    *
    *     chain_to_array 
    *     get_elem_type
    */
#define MY_INVALID     0
#define MY_FLOAT       1
#define MY_DOUBLE      2
#define MY_INT         3


   /*
    * If two stars have the exace same coords, so that the difference
    * in position is precisely 0.0, we set the distance between
    * them to this very small (but non-zero) value, to avoid
    * division by zero.  See the "set_triangle" function.
    */
#define TINY_LENGTH    1.0e-6


   /* 
    * the following are "private" functions, used internally only. 
    */

   /* this typedef is used several sorting routines */
typedef int (*PFI)(const void *, const void *);

static int set_star(s_star *star, float x, float y, float mag);
static void copy_star(s_star *from_ptr, s_star *to_ptr);
static void copy_star_array(s_star *from_array, s_star *to_array, int num);
static void free_star_array(s_star *array);
#ifdef DEBUG
static void print_star_array(s_star *array, int num);
#endif
static float **calc_distances(s_star *star_array, int numstars);
static void free_distances(float **array, int num);
#ifdef DEBUG
static void print_dist_matrix(float **matrix, int num);
#endif
static void set_triangle(s_triangle *triangle, s_star *star_array, 
                         int i, int j, int k, float **dist_matrix);
#ifdef DEBUG2
static void print_triangle_array(s_triangle *t_array, int numtriangles,
                                 s_star *star_array, int numstars);
#endif
static s_star *chain_to_array(CHAIN *chain, char *xname, char *yname,
                              char *magname, int *numstars);
static int get_elem_offset(char *type_string, int *offset);
static int get_elem_type(char *type_string);
static float get_elem_value(void *item_ptr, SCHEMA_ELEM *elem, int type,
                            int offset);
static int set_elem_value(void *item_ptr, SCHEMA_ELEM *elem, int elem_type,
                          float value);
static s_triangle *stars_to_triangles(s_star *star_array, int numstars, 
                                      int nbright, int *numtriangles);
static void sort_star_by_mag(s_star *array, int num);
static int compare_star_by_mag(s_star *star1, s_star *star2);
static void sort_star_by_x(s_star *array, int num);
static int compare_star_by_x(s_star *star1, s_star *star2);
static void sort_star_by_match_id(s_star *array, int num);
static int compare_star_by_match_id(s_star *star1, s_star *star2);
static int fill_triangle_array(s_star *star_array, int numstars,
                               float **dist_matrix,
                               int numtriangles, s_triangle *t_array);
static void sort_triangle_array(s_triangle *array, int num);
static int compare_triangle(s_triangle *triangle1, s_triangle *triangle2);
static int find_ba_triangle(s_triangle *array, int num, float ba0);
static void prune_triangle_array(s_triangle *t_array, int *numtriangles);
static int **make_vote_matrix(s_star *star_array_A, 
                              s_star *star_array_B,
                              s_triangle *t_array_A, int num_triangles_A,
                              s_triangle *t_array_B, int num_triangles_B,
                              int nbright, float radius, float scale);
#ifdef DEBUG
static void print_vote_matrix(int **vote_matrix, int numcells);
#endif
static int top_vote_getters(int **vote_matrix, int num, int **winner_votes,
                            int **winner_index_A, int **winner_index_B);
static int calc_trans(int nbright, s_star *star_array_A, int num_stars_A,
                          s_star *star_array_B, int num_stars_B, 
                          int *winner_index_A, int *winner_index_B,
                          TRANS *trans);
#ifdef DEBUG
static void print_trans(TRANS *trans);
#endif

   /* 
    * these are functions used to solve a matrix equation which
    * gives us the transformation from one coord system to the other
    */
static int gauss_jordon(float **matrix, int num, float *vector);

static float ** alloc_matrix(int n);
static void free_matrix(float **matrix, int n);
#ifdef DEBUG
static void print_matrix(float **matrix, int n);
#endif

#ifdef DEBUG2
static void test_routine (void);
#endif

static int iter_trans(int nbright, s_star *star_array_A, int num_stars_A, 
                                   s_star *star_array_B, int num_stars_B,
                                   int *winner_votes,       
                                   int *winner_index_A, int *winner_index_B,
                                   float maxdist, TRANS *trans);
static int compare_float(float *f1, float *f2);
static float find_percentile(float *array, int num, float perc);
static int apply_trans(s_star *star_array, int num_stars, TRANS *trans);
static int array_to_chain(s_star *star_array, int num_stars,
                          CHAIN *chain, char *xname, char *yname);
static int double_sort_by_match_id(s_star *star_array_A, int num_stars_A,
                                   s_star *star_array_B, int num_stars_B);
static int match_arrays_slow(s_star *star_array_A, int num_stars_A,
                             s_star *star_array_B, int num_stars_B,
                             float radius,
                             s_star **star_array_J, int *num_stars_J,
                             s_star **star_array_K, int *num_stars_K,
                             s_star **star_array_L, int *num_stars_L,
                             s_star **star_array_M, int *num_stars_M);
static int add_element(s_star *new_star, s_star **star_array, int *total_num,
                       int *current_num);
static void remove_elem(s_star *star_array, int num, int *num_stars);
static int remove_repeated_elements(s_star *star_array_1, int *num_stars_1,
                                    s_star *star_array_2, int *num_stars_2);
static void remove_same_elements(s_star *star_array_1, int num_stars_1,
                                 s_star *star_array_2, int *num_stars_2);
static CHAIN *copy_chain_from_array(CHAIN *old_chain, s_star *star_array,
                                    int num_stars);




/************************************************************************
 * <AUTO EXTRACT>
 *
 * ROUTINE: atFindTrans
 *
 * DESCRIPTION:
 * This function is based on the algorithm described in Valdes et al.,
 * PASP 107, 1119 (1995).  It tries to 
 *         a. match up objects in the two chains
 *         a. find a coordinate transformation that takes coords in
 *               objects in chainA and changes to those in chainB.
 *
 *
 * Actually, this is a top-level "driver" routine that calls smaller
 * functions to perform actual tasks.  It mostly creates the proper
 * inputs and outputs for the smaller routines.
 *
 * Only the affine elemnts of the TRANS are set.  The higer-order distortion
 * and color terms are all set to 0.
 *
 * RETURN:
 *    SH_SUCCESS         if all goes well
 *    SH_GENERIC_ERROR   if an error occurs
 *
 * </AUTO>
 */

int
atFindTrans
   (
   CHAIN *chainA,        /* I: match this set of objects with chain B */
   char *xnameA,         /* I: name of the field in CHAIN A with X coord */
   char *ynameA,         /* I: name of the field in CHAIN A with Y coord */
   char *magnameA,       /* I: name of the field in CHAIN A with "mag" */
   CHAIN *chainB,        /* I: match this set of objects with chain A */
   char *xnameB,         /* I: name of the field in CHAIN B with X coord */
   char *ynameB,         /* I: name of the field in CHAIN B with Y coord */
   char *magnameB,       /* I: name of the field in CHAIN B with "mag" */
   float radius,         /* I: max radius in triangle-space allowed for */
                         /*       a pair of triangles to match */
   int nobj,             /* I: max number of bright stars to use in creating */
                         /*       triangles for matching from each list */
   float maxdist,        /* I: after initial guess at coord transform */
                         /*       accept as valid pairs which are at most */
                         /*       this far apart in coord system B */
   float scale,          /* I: expected relative scale factor */
                         /*       if -1, any scale factor is allowed */
   TRANS *trans          /* O: place into this TRANS structure's fields */
                         /*       the coeffs which convert coords of chainA */
                         /*       into coords of chainB system. */
   )
{
   int i, nbright, min;
   int num_stars_A;          /* number of stars in chain A */
   int num_stars_B;          /* number of stars in chain B */
   int num_triangles_A;      /* number of triangles formed from chain A */
   int num_triangles_B;      /* number of triangles formed from chain B */
   int **vote_matrix; 
   int *winner_votes;        /* # votes gotten by top pairs of matched stars */
   int *winner_index_A;      /* elem i in this array is index in star array A */
                             /*    which matches ... */
   int *winner_index_B;      /* elem i in this array, index in star array B */
   s_star *star_array_A, *star_array_B;
   s_triangle *triangle_array_A = NULL;
   s_triangle *triangle_array_B = NULL;


#ifdef DEBUG2
   test_routine();
#endif

   star_array_A = chain_to_array(chainA, xnameA, ynameA, magnameA, 
	    &num_stars_A); 
   if (star_array_A == NULL) {
      shError("atFindTrans: chain_to_array fails for chainA");
      return(SH_GENERIC_ERROR);
   }
   star_array_B = chain_to_array(chainB, xnameB, ynameB, magnameB, 
	    &num_stars_B); 
   if (star_array_B == NULL) {
      shError("atFindTrans: chain_to_array fails for chainB");
      free_star_array(star_array_A);
      return(SH_GENERIC_ERROR);
   }

   /*
    * here we check to see if each CHAIN of stars contains a
    * required minimum number of stars.  If not, we return with
    * an error message, and SH_GENERIC_ERROR.  
    *
    * In addition, we check to see that each CHAIN has at least 'nobj'
    * items.  If not, we set 'nbright' to the minimum of the two
    * CHAIN lengths, and print a warning message so the user knows 
    * that we're using fewer stars than he asked.
    *
    * On the other hand, if the user specifies a value of "nobj" which
    * is too SMALL, then we ignore it and use the smallest valid
    * value (which is AT_MATCH_STARTN).
    */
   min = (num_stars_A < num_stars_B ? num_stars_A : num_stars_B);
   if (min < AT_MATCH_STARTN) {
      shError("atFindTrans: only %d stars in CHAIN(s), require at least %d",
	       min, AT_MATCH_STARTN);
      free_star_array(star_array_A);
      free_star_array(star_array_B);
      return(SH_GENERIC_ERROR);
   }
   if (nobj > min) {
      shDebug(AT_MATCH_ERRLEVEL,
	       "atFindTrans: using only %d stars, fewer than requested %d",
	       min, nobj);
      nbright = min;
   }
   else {
      nbright = nobj;
   }
   if (nbright < AT_MATCH_STARTN) {
      shDebug(AT_MATCH_ERRLEVEL,
	       "atFindTrans: must use %d stars, more than requested %d",
	       AT_MATCH_STARTN, nobj);
      nbright = AT_MATCH_STARTN;
   }

   /* this is a sanity check on the above checks */
   shAssert((nbright >= AT_MATCH_STARTN) && (nbright <= min));


#ifdef DEBUG2
   printf("here comes star array A\n");
   print_star_array(star_array_A, num_stars_A);
   printf("here comes star array B\n");
   print_star_array(star_array_B, num_stars_B);
#endif
   
   /*
    * we now convert each list of stars into a list of triangles, 
    * using only a subset of the "nbright" brightest items in each list.
    */
   triangle_array_A = stars_to_triangles(star_array_A, num_stars_A, 
	    nbright, &num_triangles_A);
   shAssert(triangle_array_A != NULL);
   triangle_array_B = stars_to_triangles(star_array_B, num_stars_B, 
	    nbright, &num_triangles_B);
   shAssert(triangle_array_B != NULL);


   /*
    * Now we prune the triangle arrays to eliminate those with pairs
    * since Valdes et al. say that this speeds things up and eliminates
    * lots of closely-packed triangles.
    */
   prune_triangle_array(triangle_array_A, &num_triangles_A);
   prune_triangle_array(triangle_array_B, &num_triangles_B);
#ifdef DEBUG2
   printf("after pruning, here comes triangle array A\n");
   print_triangle_array(triangle_array_A, num_triangles_A,
	       star_array_A, num_stars_A);
   printf("after pruning, here comes triangle array B\n");
   print_triangle_array(triangle_array_B, num_triangles_B,
	       star_array_B, num_stars_B);
#endif


   /*
    * Next, we want to try to match triangles in the two arrays.
    * What we do is to create a "vote matrix", which is a 2-D array
    * with "nbright"-by-"nbright" cells.  The cell with
    * coords [i][j] holds the number of matched triangles in which
    * 
    *        item [i] in star_array_A matches item [j] in star_array_B
    *
    * We'll use this "vote_matrix" to figure out a first guess
    * at the transformation between coord systems.
    *
    * Note that if there are fewer than "nbright" stars
    * in either list, we'll still make the vote_matrix 
    * contain "nbright"-by-"nbright" cells ...
    * there will just be a lot of cells filled with zero.
    */
   vote_matrix = make_vote_matrix(star_array_A, 
                                  star_array_B,
                                  triangle_array_A, num_triangles_A,
                                  triangle_array_B, num_triangles_B,
                                  nbright, radius, scale);


   /*
    * having made the vote_matrix, we next need to pick the 
    * top 'nbright' vote-getters.  We call 'top_vote_getters'
    * and are given, in its output arguments, pointers to three
    * arrays, each of which has 'nbright' elements pertaining
    * to a matched pair of STARS:
    * 
    *       winner_votes[]    number of votes of winners, in descending order
    *       winner_index_A[]  index of star in star_array_A 
    *       winner_index_B[]  index of star in star_array_B
    *
    * Thus, the pair of stars which matched in the largest number
    * of triangles will be 
    *
    *       star_array_A[winner_index_A[0]]    from array A
    *       star_array_B[winner_index_A[0]]    from array B
    *
    * and the pair of stars which matched in the second-largest number
    * of triangles will be 
    *
    *       star_array_A[winner_index_A[1]]    from array A
    *       star_array_B[winner_index_A[1]]    from array B
    * 
    * and so on.
    */
   top_vote_getters(vote_matrix, nbright, 
	       &winner_votes, &winner_index_A, &winner_index_B);


   /*
    * next, we take the "top" matched pairs of coodinates, and
    * figure out a transformation of the form
    *
    *       x' = A + Bx + Cx
    *       y' = D + Ex + Fy
    *
    * (i.e. a TRANS structure) which converts the coordinates
    * of objects in chainA to those in chainB.
    */
   if (iter_trans(nbright, star_array_A, num_stars_A, 
                       star_array_B, num_stars_B,
                       winner_votes, winner_index_A, winner_index_B, 
                       maxdist, trans) != SH_SUCCESS) {

      shError("atFindTrans: iter_trans unable to create a valid TRANS");
      free_star_array(star_array_A);
      free_star_array(star_array_B);
      for (i = 0; i < nbright; i++) {
         shFree(vote_matrix[i]);
      }
      shFree(vote_matrix);
      shFree(winner_votes);
      shFree(winner_index_A);
      shFree(winner_index_B);
      shFree(triangle_array_A);
      shFree(triangle_array_B);
      return(SH_GENERIC_ERROR);
   }

#ifdef DEBUG
   printf("  after calculating new TRANS structure, here it is\n");
   print_trans(trans);
#endif

   /*
    * clean up memory we've used
    */
   free_star_array(star_array_A);
   free_star_array(star_array_B);
   for (i = 0; i < nbright; i++) {
      shFree(vote_matrix[i]);
   }
   shFree(vote_matrix);
   shFree(winner_votes);
   shFree(winner_index_A);
   shFree(winner_index_B);
   shFree(triangle_array_A);
   shFree(triangle_array_B);


   return(SH_SUCCESS);
}



/************************************************************************
 * <AUTO EXTRACT>
 *
 * ROUTINE: atApplyTrans
 *
 * DESCRIPTION:
 * Given a CHAIN of structures, each of which has an element containing
 * something used as an "x" coordinate, and an element for something
 * used as a "y" coordinate, apply the given TRANS to each item in
 * the CHAIN, modifying the "x" and "y" values.
 *
 * Only the affine terms of the TRANS structure are used.  The higher-order
 * distortion and color terms are ignfored.  Thus:
 *
 *       x' = a + bx + cx
 *       y' = d + ex + fy
 *
 * Actually, this is a top-level "driver" routine that calls smaller
 * functions to perform actual tasks.  It mostly creates the proper
 * inputs and outputs for the smaller routines.
 *
 *
 * RETURN:
 *    SH_SUCCESS         if all goes well
 *    SH_GENERIC_ERROR   if an error occurs
 *
 * </AUTO>
 */

int
atApplyTrans
   (
   CHAIN *chain,         /* I/O: modify x,y coords of objects in this CHAIN */
   char *xname,          /* I: name of the field in CHAIN with X coord */
   char *yname,          /* I: name of the field in CHAIN with Y coord */
   TRANS *trans          /* I: use this TRANS to transform the coords of */
                         /*       items in the CHAIN */
   )
{
   int num_stars;            /* number of stars in chain */
   char magname[10];
   s_star *star_array;

   /*
    * We're not going to use "magname" in the 'chain_to_array'
    * function, so set "magname" equal to "".
    */
   sprintf(magname, "");

   /* 
    * read each item on the CHAIN into an array of little structures
    */

   star_array = chain_to_array(chain, xname, yname, magname, &num_stars); 
   if (star_array == NULL) {
      shError("atApplyTrans: chain_to_array fails for chain");
      return(SH_GENERIC_ERROR);
   }

   /*
    * next, apply the transformation to each element of the array
    */
   apply_trans(star_array, num_stars, trans);

   /*
    * finally, feed the modified coordinates back into the CHAIN
    */
   if (array_to_chain(star_array, num_stars, chain, xname, yname) !=
		  SH_SUCCESS) {
      shError("atApplyTrans: array_to_chain fails");
      return(SH_GENERIC_ERROR);
   }

   /*
    * all done!
    */
   free_star_array(star_array);

   return(SH_SUCCESS);
}


/************************************************************************
 * <AUTO EXTRACT>
 *
 * ROUTINE: atMatchChains
 *
 * DESCRIPTION:
 * Given 2 CHAINs of structures, the items on each of which have 
 * an element containing something used as an "x" coordinate, 
 * and an element for something  used as a "y" coordinate, 
 * and which have ALREADY been transformed so that the "x"
 * and "y" coordinates of each CHAIN are close to each other
 * (i.e. matching items from each CHAIN have very similar "x" and "y")
 * this routine attempts to find all instances of matching items
 * from the 2 CHAINs.
 *
 * We consider a "match" to be the closest coincidence of centers 
 * which are within "radius" pixels of each other.  
 *
 * Use a slow, but sure, algorithm.
 *
 * We will match objects from A --> B.  It is possible to have several
 * As that match to the same B:
 *
 *           A1 -> B5   and A2 -> B5
 *
 * This function finds such multiple-match items and deletes all but
 * the closest of the matches.
 *
 * place the elems of A that are matches into output chain J
 *                    B that are matches into output chain K
 *                    A that are not matches into output chain L
 *                    B that are not matches into output chain M
 *
 *
 * RETURN:
 *    SH_SUCCESS         if all goes well
 *    SH_GENERIC_ERROR   if an error occurs
 *
 * </AUTO>
 */

int
atMatchChains
   (
   CHAIN *chainA,           /* I: first input chain of items to be matched */
   char *xnameA,            /* I: name of field in items in listA which */
                            /*       contain the "x" coordinates */
   char *ynameA,            /* I: name of field in items in listA which */
                            /*       contain the "y" coordinates */
   CHAIN *chainB,           /* I: second chain of items to be matched */
   char *xnameB,            /* I: name of field in items in listB which */
                            /*       contain the "x" coordinates */
   char *ynameB,            /* I: name of field in items in listB which */
                            /*       contain the "y" coordinates */
   float radius,            /* I: maximum radius for items to be a match */
   CHAIN **chainJ,          /* O: output chain of all items in A that match */
   CHAIN **chainK,          /* O: output chain of all items in B that match */
   CHAIN **chainL,          /* O: chain of items from A that didn't match */
   CHAIN **chainM           /* O: chain of items from B that didn't match */
   )
{
   s_star *star_array_A;
   int num_stars_A;
   s_star *star_array_B;
   int num_stars_B;
   s_star *star_array_J, *star_array_K, *star_array_L, *star_array_M;
   int num_stars_J, num_stars_K, num_stars_L, num_stars_M;
   char magname[10];

   /* 
    * read each item on the CHAIN into an array of little structures
    * Since we don't want to use any "mag" information, set 
    * "magname" equal to the empty string "".
    */
   sprintf(magname, "");

   star_array_A = chain_to_array(chainA, xnameA, ynameA, magname, 
	       &num_stars_A); 
   if (star_array_A == NULL) {
      shError("atMatchChains: chain_to_array fails for chain A");
      return(SH_GENERIC_ERROR);
   }
   star_array_B = chain_to_array(chainB, xnameB, ynameB, magname, 
	       &num_stars_B); 
   if (star_array_B == NULL) {
      shError("atMatchChains: chain_to_array fails for chain B");
      return(SH_GENERIC_ERROR);
   }

   if (match_arrays_slow(star_array_A, num_stars_A,
                         star_array_B, num_stars_B,
                         radius,
                         &star_array_J, &num_stars_J,
                         &star_array_K, &num_stars_K,
                         &star_array_L, &num_stars_L,
                         &star_array_M, &num_stars_M) != SH_SUCCESS) {
      shError("atMatchChains: match_arrays_slow fails");
      free_star_array(star_array_A);
      free_star_array(star_array_B);
      return(SH_GENERIC_ERROR);
   }


   /*
    * now we have to convert the star_arrays J,K,L,M into CHAINs ...
    */
   *chainJ = copy_chain_from_array(chainA, star_array_J, num_stars_J);
   *chainK = copy_chain_from_array(chainB, star_array_K, num_stars_K);
   *chainL = copy_chain_from_array(chainA, star_array_L, num_stars_L);
   *chainM = copy_chain_from_array(chainB, star_array_M, num_stars_M);

   /*
    * all done!
    */
   free_star_array(star_array_A);
   free_star_array(star_array_B);
   free_star_array(star_array_J);
   free_star_array(star_array_K);
   free_star_array(star_array_L);
   free_star_array(star_array_M);

   return(SH_SUCCESS);
}






/*                    end of PUBLIC information                          */
/*-----------------------------------------------------------------------*/
/*                  start of PRIVATE information                         */

   /* 
    * the functions listed from here on are intended to be used only
    * "internally", called by the PUBLIC functions above.  Users
    * should be discouraged from accessing them directly.
    */


/************************************************************************
 * 
 *
 * ROUTINE: set_star
 *
 * DESCRIPTION:
 * Given a pointer to an EXISTING s_star, initialize its values
 * and set x, y, and mag to the given values.
 *
 * RETURN:
 *    SH_SUCCESS        if all goes well
 *    SH_GENERIC_ERROR  if not
 *
 * </AUTO>
 */

static int
set_star
   (
   s_star *star,           /* I: pointer to existing s_star structure */
   float x,                /* I: star's "X" coordinate */
   float y,                /* I: star's "Y" coordinate */
   float mag               /* I: star's "mag" coordinate */
   )
{
   static int id_number = 0;

   if (star == NULL) {
      shError("set_star: given a NULL star");
      return(SH_GENERIC_ERROR);
   }
   star->id = id_number++;
   star->index = -1;
   star->x = x;
   star->y = y;
   star->mag = mag;
   star->match_id = -1;
   star->next = (s_star *) NULL;
   
   return(SH_SUCCESS);
}



/************************************************************************
 * 
 *
 * ROUTINE: copy_star
 *
 * DESCRIPTION:
 * Copy the contents of the "s_star" to which "from_ptr" points
 * to the "s_star" to which "to_ptr" points.
 *
 * RETURN:
 *    nothing
 *
 * </AUTO>
 */

static void
copy_star
   (
   s_star *from_ptr,       /* I: copy contents of _this_ star ... */
   s_star *to_ptr          /* O: into _this_ star */
   )
{
   shAssert(from_ptr != NULL);
   shAssert(to_ptr != NULL);

   to_ptr->id = from_ptr->id;
   to_ptr->index = from_ptr->index;
   to_ptr->x = from_ptr->x;
   to_ptr->y = from_ptr->y;
   to_ptr->mag = from_ptr->mag;
   to_ptr->match_id = from_ptr->match_id;
   to_ptr->next = from_ptr->next;
   
}




/************************************************************************
 * 
 *
 * ROUTINE: copy_star_array
 *
 * DESCRIPTION:
 * Given to arrays of "s_star" structures, EACH OF WHICH MUST 
 * ALREADY HAVE BEEN ALLOCATED and have "num" elements,
 * copy the contents of the items in "from_array" 
 * to those in "to_array".
 *
 * RETURN:
 *    nothing
 *
 * </AUTO>
 */

static void
copy_star_array
   (
   s_star *from_array,     /* I: copy contents of _this_ array ... */
   s_star *to_array,       /* O: into _this_ array */
   int num_stars           /* I: each aray must have this many elements */
   )
{
   int i;
   s_star *from_ptr, *to_ptr;

   shAssert(from_array != NULL);
   shAssert(to_array != NULL);

   for (i = 0; i < num_stars; i++) {
      from_ptr = &(from_array[i]);
      to_ptr = &(to_array[i]);
      shAssert(from_ptr != NULL);
      shAssert(to_ptr != NULL);

      to_ptr->id = from_ptr->id;
      to_ptr->index = from_ptr->index;
      to_ptr->x = from_ptr->x;
      to_ptr->y = from_ptr->y;
      to_ptr->mag = from_ptr->mag;
      to_ptr->match_id = from_ptr->match_id;
      to_ptr->next = from_ptr->next;
   }
   
}



/************************************************************************
 * 
 *
 * ROUTINE: free_star_array
 *
 * DESCRIPTION:
 * Delete an array of "num" s_star structures.
 *
 * RETURN:
 *    nothing
 *
 * </AUTO>
 */

static void
free_star_array
   (
   s_star *first    /* first star in the array to be deleted */
   )
{
   shFree(first);
}




/************************************************************************
 * 
 *
 * ROUTINE: print_star_array
 *
 * DESCRIPTION:
 * Given an array of "num" s_star structures, print out
 * a bit of information on each in a single line.
 *
 * For debugging purposes.
 *
 * RETURN:
 *    nothing
 *
 * </AUTO>
 */

#ifdef DEBUG

static void
print_star_array
   (
   s_star *array,         /* I: first star in array */
   int num                /* I: number of stars in the array to print */
   )
{
   int i;
   s_star *star;

   for (i = 0; i < num; i++) {
      star = &(array[i]);
      shAssert(star != NULL);
      printf(" %4d %4d %10.4f %10.4f %6.2f\n", i, star->id, star->x,
	       star->y, star->mag);
   }
}

#endif  /* DEBUG */


/************************************************************************
 * 
 *
 * ROUTINE: calc_distances
 *
 * DESCRIPTION:
 * Given an array of N='numstars' s_star structures, create a 2-D array
 * called "matrix" with NxN elements and fill it by setting
 *
 *         matrix[i][j] = distance between stars i and j
 *
 * where 'i' and 'j' are the indices of their respective stars in 
 * the 1-D array.
 *
 * RETURN:
 *    float **array      pointer to array of pointers to each row of array
 *    NULL               if something goes wrong.
 *
 * </AUTO>
 */

static float **
calc_distances
   (
   s_star *star_array,      /* I: array of s_stars */
   int numstars             /* I: with this many elements */
   )
{
   int i, j;
   float **matrix;
   double dx, dy, dist;
   
   if (numstars == 0) {
      shError("calc_distances: given an array of zero stars");
      return(NULL);
   }

   /* allocate the array, row-by-row */
   matrix = (float **) shMalloc(numstars*sizeof(float *));
   for (i = 0; i < numstars; i++) {
      matrix[i] = (float *) shMalloc(numstars*sizeof(float));
   }

   /* fill up the array */
   for (i = 0; i < numstars - 1; i++) {
      for (j = i + 1; j < numstars; j++) {
	 dx = star_array[i].x - star_array[j].x;
	 dy = star_array[i].y - star_array[j].y;
	 dist = sqrt(dx*dx + dy*dy);
	 matrix[i][j] = (float) dist;
	 matrix[j][i] = (float) dist;
      }
   }
   /* for safety's sake, let's fill the diagonal elements with zeros */
   for (i = 0; i < numstars; i++) {
      matrix[i][i] = 0.0;
   }

   /* okay, we're done.  return a pointer to the array */
   return(matrix);
}


/************************************************************************
 * 
 *
 * ROUTINE: free_distances
 *
 * DESCRIPTION:
 * Given a 2-D array of "num"-by-"num" float elements, free up
 * each row of the array, then free the array itself.
 *
 * RETURN:
 *    nothing
 *
 * </AUTO>
 */

static void
free_distances
   (
   float **array,          /* I: square array we'll free */
   int num                 /* I: number of elems in each row */
   )
{
   int i;

   for (i = 0; i < num; i++) {
      shFree(array[i]);
   }
   shFree(array);
}


/************************************************************************
 * 
 *
 * ROUTINE: print_dist_matrix
 *
 * DESCRIPTION:
 * Given a 2-D array of "num"-by-"num" distances between pairs of
 * stars, print out the 2-D array in a neat fashion.
 *
 * For debugging purposes.
 *
 * RETURN:
 *    nothing
 *
 * </AUTO>
 */

#ifdef DEBUG

static void
print_dist_matrix
   (
   float **matrix,        /* I: pointer to start of 2-D square array */
   int num                /* I: number of rows and columns in the array */
   )
{
   int i, j;

   for (i = 0; i < num; i++) {
      shAssert(matrix[i] != NULL);
      for (j = 0; j < num; j++) {
         printf("%5.1f ", matrix[i][j]);
      }
      printf("\n");
   }
}

#endif /* DEBUG */



/************************************************************************
 * 
 *
 * ROUTINE: set_triangle
 *
 * DESCRIPTION:
 * Set the elements of some given, EXISTING instance of an "s_triangle" 
 * structure, given (the indices to) three s_star structures for its vertices.  
 * We check to make sure
 * that the three stars are three DIFFERENT stars, asserting
 * if not.
 *
 * The triangle's "a_index" is set to the position of the star opposite
 * its side "a" in its star array, and similarly for "b_index" and "c_index".
 *
 * RETURN:
 *    nothing
 *
 * </AUTO>
 */

static void
set_triangle
   (
   s_triangle *tri,     /* we set fields of this existing structure */
   s_star *star_array,  /* use stars in this array as vertices */
   int s1,              /* index in 'star_array' of one vertex */
   int s2,              /* index in 'star_array' of one vertex */
   int s3,              /* index in 'star_array' of one vertex */
   float **darray       /* array of distances between stars */
   )
{
   static int id_number = 0;
   float d12, d23, d13;
   float a=0, b=0, c=0;
   s_star *star1, *star2, *star3;
   int fail;             /* this used to prevent compiler warning with shAssert */

   shAssert(tri != NULL);
   shAssert((s1 != s2) && (s1 != s3) && (s2 != s3));
   star1 = &star_array[s1];
   star2 = &star_array[s2];
   star3 = &star_array[s3];
   shAssert((star1 != NULL) && (star2 != NULL) && (star3 != NULL));

   tri->id = id_number++;
   tri->index = -1;

   /* 
    * figure out which sides is longest and shortest, and assign
    *
    *     "a" to the length of the longest side
    *     "b"                      intermediate 
    *     "c"                      shortest
    *
    * We use temp variables   d12 = distance between stars 1 and 2
    *                         d23 = distance between stars 2 and 3
    *                         d13 = distance between stars 1 and 3
    * for convenience.
    *
    */
   d12 = darray[s1][s2];
   d23 = darray[s2][s3];
   d13 = darray[s1][s3];

   /*
    * If a side has length exactly == 0.0, then print a warning message
    * and set the length equal to a very small (but non-zero) value
    * TINY_LENGTH.  This will prevent division by zero at the end
    * of this function.
    */
   if (d12 == 0.0) {
      shDebug(AT_MATCH_ERRLEVEL, 
		  "set_triangle: stars %d and %d have same coords",
		  s1, s2);
      d12 = TINY_LENGTH;
   }
   if (d23 == 0.0) {
      shDebug(AT_MATCH_ERRLEVEL, 
		  "set_triangle: stars %d and %d have same coords",
		  s2, s3);
      d23 = TINY_LENGTH;
   }
   if (d13 == 0.0) {
      shDebug(AT_MATCH_ERRLEVEL, 
		  "set_triangle: stars %d and %d have same coords",
		  s1, s3);
      d13 = TINY_LENGTH;
   }
   
   if ((d12 >= d23) && (d12 >= d13)) {
      /* this applies if the longest side connects stars 1 and 2 */
      tri->a_index = star3->index;
      a = d12;
      if (d23 >= d13) {
	 tri->b_index = star1->index;
	 b = d23;
	 tri->c_index = star2->index;
	 c = d13;
      }
      else {
	 tri->b_index = star2->index;
	 b = d13;
	 tri->c_index = star1->index;
	 c = d23;
      }
   }
   else if ((d23 > d12) && (d23 >= d13)) {
      /* this applies if the longest side connects stars 2 and 3 */
      tri->a_index = star1->index;
      a = d23;
      if (d12 > d13) {
	 tri->b_index = star3->index;
	 b = d12;
	 tri->c_index = star2->index;
	 c = d13;
      }
      else {
	 tri->b_index = star2->index;
	 b = d13;
	 tri->c_index = star3->index;
	 c = d12;
      }
   }
   else if ((d13 > d12) && (d13 > d23)) {
      /* this applies if the longest side connects stars 1 and 3 */
      tri->a_index = star2->index;
      a = d13;
      if (d12 > d23) {
	 tri->b_index = star3->index;
	 b = d12;
	 tri->c_index = star1->index;
	 c = d23;
      }
      else {
	 tri->b_index = star1->index;
	 b = d23;
	 tri->c_index = star3->index;
	 c = d12;
      }
   }
   else {
      /* we should never get here! */
      shError("set_triangle: impossible situation?!");
      fail = 1;
      shAssert(fail==0);
   }

   /* 
    * now that we've figured out the longest, etc., sides, we can 
    * fill in the rest of the triangle's elements
    *
    * We need to make a special check, in case a == 0.  In that
    * case, we'll just set the ratios ba and ca = 1.0, and hope
    * that these triangles are ignored.
    */
   tri->a_length = a;
   if (a > 0.0) {
      tri->ba = b/a;
      tri->ca = c/a;
   } 
   else {
      tri->ba = 1.0;
      tri->ca = 1.0;
   }

   tri->match_id = -1;
   tri->next = (s_triangle *) NULL;
}


/************************************************************************
 * 
 *
 * ROUTINE: print_triangle_array
 *
 * DESCRIPTION:
 * Given an array of "numtriangle" s_triangle structures, 
 * and an array of "numstars" s_star structures that make them up,
 * print out
 * a bit of information on each triangle in a single line.
 *
 * For debugging purposes.
 *
 * RETURN:
 *    nothing
 *
 * </AUTO>
 */

#ifdef DEBUG2

static void
print_triangle_array
   (
   s_triangle *t_array,   /* I: first triangle in array */
   int numtriangles,      /* I: number of triangles in the array to print */
   s_star *star_array,    /* I: array of stars which appear in triangles */
   int numstars           /* I: number of stars in star_array */
   )
{
   int i;
   s_triangle *triangle;
   s_star *sa, *sb, *sc;

   for (i = 0; i < numtriangles; i++) {
      triangle = &(t_array[i]);
      shAssert(triangle != NULL);

      sa = &(star_array[triangle->a_index]);
      sb = &(star_array[triangle->b_index]);
      sc = &(star_array[triangle->c_index]);

      printf("%4d %4d %3d (%5.1f,%5.1f) %3d (%5.1f,%5.1f) %3d (%5.1f, %5.1f)  %5.3f %5.3f\n",
	       i, triangle->id, 
	       triangle->a_index, sa->x, sa->y,
	       triangle->b_index, sb->x, sb->y, 
	       triangle->c_index, sc->x, sc->y,
	       triangle->ba, triangle->ca);
   }
}

#endif /* DEBUG */



/************************************************************************
 * 
 *
 * ROUTINE: chain_to_array
 *
 * DESCRIPTION:
 * Convert a CHAIN of objects of some indeterminate schema into 
 * an array of "s_star" structures.  Use the given names
 * to identify the fields inside the schema into "x", "y" and "mag".
 * Place the number of s_star structures in the array into the
 * final argument, "numstars".  
 *
 * If the user passes a value of "" for 'magname', then do not
 * try to read any 'mag' value from the structures; read only
 * 'x' and 'y' values.
 *
 * The "match_id" field of each created "s_star" structure is set
 * equal to the position of its corresponding object in the
 * CHAIN.
 *
 * RETURN:
 *    s_star *array      pointer to start of array of s_star structures
 *    NULL               if error occursurs
 *
 * </AUTO>
 */

static s_star *
chain_to_array
   (
   CHAIN *chain,         /* I: match this set of objects ... */
   char *xname,          /* I: name of the field in schema with X coord */
   char *yname,          /* I: name of the field in schema with Y coord */
   char *magname,        /* I: name of the field in schema with mag */
                         /*         or "" if we should ignore 'mag' */
   int *numstars         /* O: number of stars in array */
   )
{
   int i, num;
   int found_xname, found_yname, found_magname;
   int xtype, ytype, magtype=0;
   int xoffset, yoffset, magoffset;
   s_star *array, *star;
   float x, y, mag;
   TYPE chain_type;
   char *schema_name;
   const SCHEMA *schema_ptr;
   SCHEMA_ELEM elem, xname_elem, yname_elem, magname_elem;
   void *item_ptr;

   /* first, get the type of the CHAIN as a string */
   chain_type = shChainTypeGet(chain);
   schema_name = shNameGetFromType(chain_type);
   shAssert(strcmp(schema_name, "UNKNOWN") != 0);

   /* next, we get a pointer to the SCHEMA describing the items on the CHAIN */
   schema_ptr = shSchemaGet(schema_name);


   /*
    * check to see if any names have components inside angle brackets,
    * which indicate that the element must be part of an array.
    * If so, parse the array subscript from inside the angle brackets,
    * and then strip the angle brackets from the name.
    * I.e., if the user passes 
    * 
    *      magname = "petrosian_mag<2>"
    *
    * then we'll set "magoffset" to 2, and cause magname to be
    *
    *      magname = "petrosian_mag"
    */
   if (get_elem_offset(xname, &xoffset) != SH_SUCCESS) {
      shError("chain_to_array: get_elem_offset fails for x field");
      return(NULL);
   }
   if (get_elem_offset(yname, &yoffset) != SH_SUCCESS) {
      shError("chain_to_array: get_elem_offset fails for y field");
      return(NULL);
   }
   if (get_elem_offset(magname, &magoffset) != SH_SUCCESS) {
      shError("chain_to_array: get_elem_offset fails for mag field");
      return(NULL);
   }

   /* verify that this SCHEMA has fields with the given names */
   found_xname = 0;
   found_yname = 0;
   found_magname = 0;
   for (i = 0; i < schema_ptr->nelem; i++) {
      elem = schema_ptr->elems[i];
      if (strcmp(elem.name, xname) == 0) {
	 found_xname++;
	 xname_elem = elem;
      }
      if (strcmp(elem.name, yname) == 0) {
	 found_yname++;
	 yname_elem = elem;
      }
      if (strcmp(elem.name, magname) == 0) {
	 found_magname++;
	 magname_elem = elem;
      }
   }
   if ((found_xname != 1) || (found_yname != 1) || 
       ((strcmp(magname, "") != 0) && (found_magname != 1))) {
      shError("chain_to_array: found_xname %d found_yname %d found_magname %d\n",
	       found_xname, found_yname, found_magname);
      return(NULL);
   }

   /*
    * check to see that the "x", "y" and (if desired) "mag" fields are either
    * 
    *         float, double, int
    *
    * return with error if not.   
    */
   if ((xtype = get_elem_type(xname_elem.type)) == MY_INVALID) {
      shError("chain_to_array: x field has invalid type");
   }
   if ((ytype = get_elem_type(yname_elem.type)) == MY_INVALID) {
      shError("chain_to_array: y field has invalid type");
   }
   if (strcmp(magname, "") != 0) {
      if ((magtype = get_elem_type(magname_elem.type)) == MY_INVALID) {
         shError("chain_to_array: mag field has invalid type");
      }
   }
   if ((xtype == MY_INVALID) || (ytype == MY_INVALID) ||
       ((strcmp(magname, "") != 0) && (magtype == MY_INVALID))) {
      return(NULL);
   }


   /*
    * okay, now we can walk down the CHAIN and create a new s_star
    * for each item on the CHAIN.  
    */
   num = shChainSize(chain);
   *numstars = num;
   array = (s_star *) shMalloc(num*sizeof(s_star));
   mag = 99.0;
   for (i = 0; i < num; i++) {
      item_ptr = (void *) shChainElementGetByPos(chain, i);

      x = (float) get_elem_value(item_ptr, &xname_elem, xtype, xoffset);
      y = (float) get_elem_value(item_ptr, &yname_elem, ytype, yoffset);
      if (strcmp(magname, "") != 0) {
         mag = (float) get_elem_value(item_ptr, &magname_elem, magtype,
	                                magoffset);
      }

      star = &(array[i]);
      set_star(star, x, y, mag);
      star->match_id = i;
   }

   return(array);
}


/************************************************************************
 * 
 *
 * ROUTINE: get_elem_offset
 *
 * DESCRIPTION:
 * Given the name of some schema element which the user wishes to
 * use as either "x", "y" or "mag", look to see if the name of the element 
 * contains an integer inside angle brackets, like this:
 *
 *            "petrosian_counts<2>"
 *
 * If so, then place the value of the integer into the "offset"
 * argument; we'll use it to grab the correct value from the
 * schema later on.  Strip the "<2>" section from the name.
 * type the variable is.  
 *
 * RETURN:
 *    SH_SUCCESS         if all goes well
 *    SH_GENERIC_ERROR   if can't parse; if user supplies "counts<2><3>",
 *                             for example
 *
 * </AUTO>
 */

#define OFFSET_ARRAY_LENGTH 10

static int 
get_elem_offset
   (
   char *name,           /* I: name of user-supplied element */
   int *offset           /* O: index into array (or 0, if no array) */
   )
{
   char array_value[OFFSET_ARRAY_LENGTH + 1];
   int i, j, off, len;
   int val_length;

   off = 0;
   len = strlen(name);

   /* look for a starting angle bracket */
   for (i = 0; (i < len) && (name[i] != '<'); i++) {
      ;
   }
   /* if found, then make sure that closing bracket ends string */
   if (name[i] == '<') {
      if (name[len - 1] != '>') {
	 shError("get_elem_offset: field name %s has invalid array format");
	 return(SH_GENERIC_ERROR);
      }
      if ((val_length = ((len - 1) - (i + 1))) > OFFSET_ARRAY_LENGTH) {
	 shError("get_elem_offset: field name %s has too long an array subscript",
		  name);
	 return(SH_GENERIC_ERROR);
      }

      /* now copy the array subscript digits into "array_value" */
      for (j = 0; j < val_length; j++) {
	 array_value[j] = name[i + 1 + j];
      }
      array_value[j] = '\0';

      /* 
       * make sure that all these characters are digits -- we must
       * be sure the user doesn't supply "<4><5>" or "<5.23>"
       * or "<4sdf>", or any other invalid format.  We can't
       * handle double indirections, unfortunately.
       */
      for (j = 0; j < val_length; j++) {
	 if (!isdigit(array_value[j])) {
	    shError("get_elem_offset: invalid field name format %s", 
		  name);
	    return(SH_GENERIC_ERROR);
	 }
      }

      /* and read them as an integer */
      if (sscanf(array_value, "%d", &off) != 1) {
	 shError("get_elem_offset: can't parse array subscript of field name %s",
		  name);
	 return(SH_GENERIC_ERROR);
      }
      *offset = off;

      /*
       * finally, strip the angle-bracket section from the name,
       * so that later functions don't have to deal with the subscript.
       */
      name[i] = '\0';

   }
   else {
      /* there were no angle brackets in the name */
      *offset = 0;
   }

   return(SH_SUCCESS);
}



/************************************************************************
 * 
 *
 * ROUTINE: get_elem_type
 *
 * DESCRIPTION:
 * Given a SCHEMA_ELEM structure, figure out what
 * type the variable is.  Return one of the
 * following (private) macro #define'd values for the type:
 *  
 *    MY_FLOAT                 if "name" string is "float" or "FLOAT"
 *    MY_DOUBLE                if "name" string is "double" or "DOUBLE"
 *    MY_INT                   if "name" string is "int" or "INT"
 *    MY_INVALID               if "name" string is none of the above
 *
 * RETURN:
 *    type of structure
 *
 * </AUTO>
 */

static int 
get_elem_type
   (
   char *name            /* I: name of schema element type */
   )
{
   int type;

   type = MY_INVALID;
   if ((strcmp(name, "float") == 0) || (strcmp(name, "FLOAT") == 0)) {
      type = MY_FLOAT;
   }
   if ((strcmp(name, "double") == 0) || (strcmp(name, "DOUBLE") == 0)) {
      type = MY_DOUBLE;
   }
   if ((strcmp(name, "int") == 0) || (strcmp(name, "INT") == 0)) {
      type = MY_INT;
   }

   return(type);
}


/************************************************************************
 * 
 *
 * ROUTINE: get_elem_value
 *
 * DESCRIPTION:
 * Given a schema item, the name of the field of interest, and the
 * type of that field, get the numerical value of that field
 * and return it as a "float".
 *
 * Use the "offset" argument to pick out some item from an array of values,
 * if its value is > 0.
 *
 *
 * RETURN:
 *    float                     value of the given element
 *
 * </AUTO>
 */

static float
get_elem_value
   (
   void *item_ptr,       /* I: pointer to schema structure */
   SCHEMA_ELEM *elem,    /* I: pointer to one element inside schema */
   int elem_type,        /* I: type of the element */
   int offset            /* I: offset from base of element, if this element */
                         /*      contains an array of values; the offset is */
                         /*      the index of the desired item in the array */
                         /*      (NOT a true offset in bytes!) */
   )
{
   TYPE dummy_type;
   float ret=0;
   int byte_offset;
   int *iptr;
   float *fptr;
   double *dptr;
   int fail;            /* this used to prevent compiler warning with shAssert */

   shAssert((elem_type == MY_FLOAT) || (elem_type == MY_DOUBLE) || 
            (elem_type == MY_INT));

   /* prevent compiler warnings */
   dummy_type = (TYPE) 0;

   switch (elem_type) {
   case MY_FLOAT:
      byte_offset = offset*sizeof(float);
      fptr = (float *) ((float *)shElemGet(item_ptr, elem, &dummy_type));
      fptr = (float *) (((char *) fptr) + byte_offset);
      ret = (float) *fptr;
      break;
   case MY_DOUBLE:
      byte_offset = offset*sizeof(double);
      dptr = (double *) ((double *)shElemGet(item_ptr, elem, &dummy_type));
      dptr = (double *) (((char *) dptr) + byte_offset);
      ret = (float) *dptr;
      break;
   case MY_INT:
      byte_offset = offset*sizeof(int);
      iptr = (int *) ((int *)shElemGet(item_ptr, elem, &dummy_type));
      iptr = (int *) (((char *) iptr) + byte_offset);
      ret = (float) *iptr;
      break;
   default:
      fail = 1;
      shAssert(fail==0);
      break;
   }

   return(ret);
}


/************************************************************************
 * 
 *
 * ROUTINE: set_elem_value
 *
 * DESCRIPTION:
 * Given a schema item, the name of the field of interest, the
 * type of that field, and a numerical value, set the value of 
 * that field to the given value.
 *
 * RETURN:
 *    SH_SUCCESS          if all goes well
 *    SH_GENERIC_ERROR    if shElemSet returns with an error
 *
 * </AUTO>
 */

static int
set_elem_value
   (
   void *item_ptr,       /* I/O: pointer to schema structure */
   SCHEMA_ELEM *elem,    /* I: pointer to one element inside schema */
   int elem_type,        /* I: type of the element */
   float value           /* I: value to which we set the element */
   )
{
   char valstring[100];
   int ival, ret=0;
   int fail;             /* this used to prevent compiler warning with shAssert */

   shAssert((elem_type == MY_FLOAT) || (elem_type == MY_DOUBLE) || 
            (elem_type == MY_INT));

   /* 
    * recall that shElemSet requires that we pass the value as
    * a string, so we have to convert it appropriately for 
    * each possible type.
    */

   switch (elem_type) {
   case MY_FLOAT:
   case MY_DOUBLE:
      sprintf(valstring, "%f", value);
      ret = shElemSet(item_ptr, elem, valstring);
      break;
   case MY_INT:
      ival = (int) floor(value + 0.5);
      sprintf(valstring, "%d", ival);
      ret = shElemSet(item_ptr, elem, valstring);
      break;
   default:
      fail = 1;
      shAssert(fail==0);
      break;
   }

   return(ret);
}


/************************************************************************
 * 
 *
 * ROUTINE: stars_to_triangles
 *
 * DESCRIPTION:
 * Convert an array of s_stars to an array of s_triangles.
 * We use only the brightest 'nbright' objects in the linked list.
 * The steps we need to take are:
 *
 *     1. sort the array of s_stars by magnitude, and
 *             set "index" values in the sorted list.
 *     2. calculate star-star distances in the sorted list,
 *             (for the first 'nbright' objects only)
 *             (creates a 2-D array of distances)
 *     3. create a linked list of all possible triangles
 *             (again using the first 'nbright' objects only)
 *     4. clean up -- delete the 2-D array of distances
 *
 * We place the number of triangles in the final argument, and
 * return a pointer to the new array of s_triangle structures.
 *
 * RETURN:
 *    s_triangle *             pointer to new array of triangles
 *                                  (and # of triangles put into output arg)
 *    NULL                     if error occurs
 *
 * </AUTO>
 */

static s_triangle *
stars_to_triangles
   (
   s_star *star_array,   /* I: array of s_stars */
   int numstars,         /* I: the total number of stars in the array */
   int nbright,          /* I: use only the 'nbright' brightest stars */
   int *numtriangles     /* O: number of triangles we create */
   )
{
   int numt;
   float **dist_matrix;
   s_triangle *triangle_array;

   /* 
    * check to see if 'nbright' > 'numstars' ... if so, we re-set
    *          nbright = numstars
    *
    * so that we don't have to try to keep track of them separately.
    * We'll be able to use 'nbright' safely from then on in this function.
    */
   if (numstars < nbright) {
      nbright = numstars;
   }

   /* 
    * sort the stars in the array by their 'mag' field, so that we get
    * them in order "brightest-first".
    */
   sort_star_by_mag(star_array, numstars);

#ifdef DEBUG
   printf("stars_to_triangles: here comes star array after sorting\n");
   print_star_array(star_array, numstars);
#endif

   /*
    * calculate the distances between each pair of stars, placing them
    * into the newly-created 2D array called "dist_matrix".  Note that
    * we only need to include the first 'nbright' stars in the
    * distance calculations.
    */
   dist_matrix = calc_distances(star_array, nbright);
   shAssert(dist_matrix != NULL);

#ifdef DEBUG
   printf("stars_to_triangles: here comes distance matrix\n");
   print_dist_matrix(dist_matrix, nbright);
#endif


   /*
    * create an array of the appropriate number of triangles that
    * can be formed from the 'nbright' objects.  
    */
   numt = (nbright*(nbright - 1)*(nbright - 2))/6;
   *numtriangles = numt;
   triangle_array = (s_triangle *) shMalloc(numt*sizeof(s_triangle));


   /* 
    * now let's fill that array by making all the possible triangles
    * out of the first 'nbright' objects in the array of stars.
    */
   fill_triangle_array(star_array, nbright, dist_matrix, 
	    *numtriangles, triangle_array);

#ifdef DEBUG2
   printf("stars_to_triangles: here comes the triangle array\n");
   print_triangle_array(triangle_array, *numtriangles, star_array, nbright);
#endif


   /* 
    * we've successfully created the array of triangles, so we can
    * now get rid of the "dist_matrix" array.  We won't need it 
    * any more.
    */
   free_distances(dist_matrix, nbright);

   return(triangle_array);
}


/************************************************************************
 * 
 *
 * ROUTINE: sort_star_by_mag
 *
 * DESCRIPTION:
 * Given an array of "num" s_star structures, sort it in order
 * of increasing magnitude.  
 *
 * After sorting, walk through the array and set each star's
 * "index" field equal to the star's position in the array.
 * Thus, the first star will have index=0, and the second index=1,
 * and so forth.
 *
 * Calls the "compare_star_by_mag" function, below.
 *
 * RETURN:
 *    nothing
 *
 * </AUTO>
 */

static void
sort_star_by_mag
   (
   s_star *array,             /* I: array of structures to be sorted */
   int num                    /* I: number of stars in the array */
   )
{
   int i;

   qsort((char *) array, num, sizeof(s_star), (PFI) compare_star_by_mag);

   /* now set the "index" field for each star */
   for (i = 0; i < num; i++) {
      array[i].index = i;
   }
}

/************************************************************************
 * 
 *
 * ROUTINE: compare_star_by_mag
 *
 * DESCRIPTION:
 * Given two s_star structures, compare their "mag" values.
 * Used by "sort_star_by_mag".
 *
 * RETURN:
 *    1                  if first star has larger "mag" 
 *    0                  if the two have equal "mag"
 *   -1                  if the first has smaller "mag"
 *
 * </AUTO>
 */

static int
compare_star_by_mag
   (
   s_star *star1,             /* I: compare "mag" field of THIS star ... */
   s_star *star2              /* I:  ... with THIS star  */
   )
{
   shAssert((star1 != NULL) && (star2 != NULL));

   if (star1->mag > star2->mag) {
      return(1);
   }
   if (star1->mag < star2->mag) {
      return(-1);
   }
   return(0);
}


/************************************************************************
 * 
 *
 * ROUTINE: sort_star_by_x
 *
 * DESCRIPTION:
 * Given an array of "num" s_star structures, sort it in order
 * of increasing "x" values.
 *
 * In this case, we do NOT re-set the "index" field of each
 * s_star after sorting!
 *
 * Calls the "compare_star_by_x" function, below.
 *
 * RETURN:
 *    nothing
 *
 * </AUTO>
 */

static void
sort_star_by_x
   (
   s_star *array,             /* I: array of structures to be sorted */
   int num                    /* I: number of stars in the array */
   )
{
   qsort((char *) array, num, sizeof(s_star), (PFI) compare_star_by_x);
}

/************************************************************************
 * 
 *
 * ROUTINE: compare_star_by_x
 *
 * DESCRIPTION:
 * Given two s_star structures, compare their "x" values.
 * Used by "sort_star_by_x".
 *
 * RETURN:
 *    1                  if first star has larger "x" 
 *    0                  if the two have equal "x"
 *   -1                  if the first has smaller "x"
 *
 * </AUTO>
 */

static int
compare_star_by_x
   (
   s_star *star1,             /* I: compare "x" field of THIS star ... */
   s_star *star2              /* I:  ... with THIS star  */
   )
{
   shAssert((star1 != NULL) && (star2 != NULL));

   if (star1->x > star2->x) {
      return(1);
   }
   if (star1->x < star2->x) {
      return(-1);
   }
   return(0);
}

/************************************************************************
 * 
 *
 * ROUTINE: sort_star_by_match_id
 *
 * DESCRIPTION:
 * Given an array of "num" s_star structures, sort it in order
 * of increasing "match_id" values.
 *
 * In this case, we do NOT re-set the "index" field of each
 * s_star after sorting!
 *
 * Calls the "compare_star_by_match_id" function, below.
 *
 * RETURN:
 *    nothing
 *
 * </AUTO>
 */

static void
sort_star_by_match_id
   (
   s_star *array,             /* I: array of structures to be sorted */
   int num                    /* I: number of stars in the array */
   )
{
   qsort((char *) array, num, sizeof(s_star), (PFI) compare_star_by_match_id);
}

/************************************************************************
 * 
 *
 * ROUTINE: compare_star_by_match_id
 *
 * DESCRIPTION:
 * Given two s_star structures, compare their "match_id" values.
 * Used by "sort_star_by_match_id".
 *
 * RETURN:
 *    1                  if first star has larger "match_id" 
 *    0                  if the two have equal "match_id"
 *   -1                  if the first has smaller "match_id"
 *
 * </AUTO>
 */

static int
compare_star_by_match_id
   (
   s_star *star1,             /* I: compare "match_id" field of THIS star ... */
   s_star *star2              /* I:  ... with THIS star  */
   )
{
   shAssert((star1 != NULL) && (star2 != NULL));

   if (star1->match_id > star2->match_id) {
      return(1);
   }
   if (star1->match_id < star2->match_id) {
      return(-1);
   }
   return(0);
}



/************************************************************************
 * 
 *
 * ROUTINE: fill_triangle_array
 *
 * DESCRIPTION:
 * Given an array of stars, and a matrix of distances between them,
 * form all the triangles possible; place the properties of these
 * triangles into the "t_array" array, which must already have
 * been allocated and contain "numtriangles" elements.
 *
 * RETURN:
 *    SH_SUCCESS           if all goes well
 *    SH_GENERIC_ERROR     if error occurs
 *
 * </AUTO>
 */

static int
fill_triangle_array
   (
   s_star *star_array,        /* I: array of stars we use to form triangles */
   int numstars,              /* I: use this many stars from the array */
   float **dist_matrix,       /* I: numstars-by-numstars matrix of distances */
                              /*       between stars in the star_array */
   int numtriangles,          /* I: number of triangles in the t_array */
   s_triangle *t_array        /* O: we'll fill properties of triangles in  */
                              /*       this array, which must already exist */
   )
{
   int i, j, k, n;
   s_triangle *triangle;

   shAssert((star_array != NULL) && (dist_matrix != NULL) && (t_array != NULL));


   n = 0;
   for (i = 0; i < numstars - 2; i++) {
      for (j = i + 1; j < numstars - 1; j++) {
	    for (k = j + 1; k < numstars; k++) {

	       triangle = &(t_array[n]);
	       set_triangle(triangle, star_array, i, j, k, dist_matrix);
		  
	       n++;
	    }
      }
   }
   shAssert(n == numtriangles);

   return(SH_SUCCESS);
}


/************************************************************************
 * 
 *
 * ROUTINE: sort_triangle_array
 *
 * DESCRIPTION:
 * Given an array of "num" s_triangle structures, sort it in order
 * of increasing "ba" value (where "ba" is the ratio of lengths 
 * of side b to side a).
 *
 * Calls the "compare_triangle" function, below.
 *
 * RETURN:
 *    nothing
 *
 * </AUTO>
 */

static void
sort_triangle_array
   (
   s_triangle *array,         /* I: array of structures to be sorted */
   int num                    /* I: number of triangles in the array */
   )
{
   qsort((char *) array, num, sizeof(s_triangle), (PFI) compare_triangle);
}


/************************************************************************
 * 
 *
 * ROUTINE: compare_triangle
 *
 * DESCRIPTION:
 * Given two s_triangle structures, compare their "ba" values.
 * Used by "sort_triangle_array".
 *
 * RETURN:
 *    1                  if first star has larger "ba" 
 *    0                  if the two have equal "ba"
 *   -1                  if the first has smaller "ba"
 *
 * </AUTO>
 */

static int
compare_triangle
   (
   s_triangle *triangle1,     /* I: compare "ba" field of THIS triangle ... */
   s_triangle *triangle2      /* I:  ... with THIS triangle  */
   )
{
   shAssert((triangle1 != NULL) && (triangle2 != NULL));

   if (triangle1->ba > triangle2->ba) {
      return(1);
   }
   if (triangle1->ba < triangle2->ba) {
      return(-1);
   }
   return(0);
}

/************************************************************************
 * 
 *
 * ROUTINE: find_ba_triangle
 *
 * DESCRIPTION:
 * Given an array of "num" s_triangle structures, which have already
 * been sorted in order of increasing "ba" value, and given one 
 * particular "ba" value ba0, return the index of the first triangle
 * in the array which has "ba" >= ba0.
 *
 * We use a binary search, on the "ba" element of each structure.
 *
 * If there is no such triangle, just return the index of the last
 * triangle in the list.
 *
 * Calls the "compare_triangle" function, above.
 *
 * RETURN:
 *    index of closest triangle in array         if all goes well
 *    index of last triangle in array            if nothing close
 *
 * </AUTO>
 */

static int
find_ba_triangle
   (
   s_triangle *array,         /* I: array of structures which been sorted */
   int num,                   /* I: number of triangles in the array */
   float ba0                  /* I: value of "ba" we seek */
   )
{
   int top, bottom, mid;

#ifdef DEBUG2
   printf("find_ba_triangle: looking for ba = %.2f\n", ba0);
#endif

   top = 0;
   if ((bottom = num - 1) < 0) {
      bottom = 0;
   }
   
   while (bottom - top > 1) {
      mid = (top + bottom)/2;
#ifdef DEBUG2
      printf(" array[%4d] ba=%.2f   array[%4d] ba=%.2f  array[%4d] ba=%.2f\n",
		  top, array[top].ba, mid, array[mid].ba, 
		  bottom, array[bottom].ba);
#endif
      if (array[mid].ba < ba0) {
         top = mid;
      }
      else {
	 bottom = mid;
      }
   }
#ifdef DEBUG2
      printf(" array[%4d] ba=%.2f                        array[%4d] ba=%.2f\n",
		  top, array[top].ba, bottom, array[bottom].ba);
#endif

   /* 
    * if we get here, then the item we seek is either "top" or "bottom"
    * (which may point to the same item in the array).
    */
   if (array[top].ba < ba0) {
#ifdef DEBUG2
      printf(" returning array[%4d] ba=%.2f \n", bottom, array[bottom].ba);
#endif
      return(bottom);
   }
   else {
#ifdef DEBUG2
      printf(" returning array[%4d] ba=%.2f \n", top, array[top].ba);
#endif
      return(top);
   }
}


/************************************************************************
 * 
 *
 * ROUTINE: prune_triangle_array
 *
 * DESCRIPTION:
 * Given an array of triangles, sort them in increasing order
 * of the side ratio (b/a), and then "ignore" all triangles
 * with (b/a) > AT_MATCH_RATIO.  
 * 
 * We re-set the arg "numtriangles" as needed, but leave the
 * space in the array allocated (since the array was allocated
 * as a single block).
 *
 * RETURN:
 *    nothing
 *
 * </AUTO>
 */

static void
prune_triangle_array
   (
   s_triangle *t_array,       /* I/O: array of triangles to sort and prune  */
   int *numtriangles          /* I/O: number of triangles in the t_array */
   )
{
   int i;

   shAssert(t_array != NULL);
   shAssert(numtriangles != NULL);

   /* first, sort the array */
   sort_triangle_array(t_array, *numtriangles);

   /* 
    * now, find the first triangle with "ba" > AT_MATCH_RATIO and 
    * re-set "numtriangles" to be just before it.  
    *
    * if this would make "numtriangles" < 1, assert 
    */
   for (i = (*numtriangles) - 1; i >= 0; i--) {
      if (t_array[i].ba <= AT_MATCH_RATIO) {
	 break;
      }
   }
   *numtriangles = i;
   shAssert(*numtriangles >= 0);
}



/************************************************************************
 * 
 *
 * ROUTINE: make_vote_matrix
 *
 * DESCRIPTION:
 * Given two arrays of triangles, and the arrays of stars that make 
 * up each set of triangles, try to match up triangles in the two
 * arrays.  Triangles can be considered to match only when the
 * Euclidean distance in "triangle space", created from the two
 * coordinates "ba" and "ca", is within "max_radius".  That is,
 * for two triangles to match, we must satisfy
 *
 *     sqrt[ (t1.ba - t2.ba)^2 + (t1.ca - t2.ca)^2 ] <= max_radius
 *
 * Note that there may be more than one triangle from array A which
 * matches a particular triangle from array B!  That's okay --
 * we treat any 2 which satisfy the above equation as "matched".
 * We rely upon the "vote_array" to weed out false matches.
 * 
 * If "scale" is not -1, then disallow any match for which the
 * ratio of triangles (indicated by "a_length" members)
 * is more than +/- AT_MATCH_PERCENT away from the given "scale" value.
 *
 * For each pair of triangles that matches, increment
 * the "vote" in each "vote cell" for each pair of matching
 * vertices.
 *
 * The "vote matrix" is a 2-D array of 'nbright'-by-'nbright'
 * integers.  We allocate the array in this function, and
 * return a pointer to the array.  Each cell in the array, vote[i][j],
 * contains the number of triangles in which
 *      
 *        star_array_A[i] matched star_array_B[j] 
 * 
 *
 * RETURN:
 *    int **             pointer to new "vote matrix"
 *
 * </AUTO>
 */

static int **
make_vote_matrix
   (
   s_star *star_array_A,      /* I: first array of stars */
   s_star *star_array_B,      /* I: second array of stars */
   s_triangle *t_array_A,     /* I: array of triangles from star_array_A */
   int num_triangles_A,       /* I: number of triangles in t_array_A */
   s_triangle *t_array_B,     /* I: array of triangles from star_array_B */
   int num_triangles_B,       /* I: number of triangles in t_array_B */
   int nbright,               /* I: consider at most this many stars */
                              /*       from each array; also the size */
                              /*       of the output "vote_matrix". */
   float max_radius,          /* I: max radius in triangle-space allowed */
                              /*       for 2 triangles to be considered */
                              /*       a matching pair. */
   float scale                /* I: expected scale factor between triangles */
                              /*       if -1, any scale factor is allowed */
   )
{
   int i, j, start_index;
   int **vote_matrix;
   float ba_A, ba_B, ca_A, ca_B, ba_min, ba_max;
   float rad2;
   float ratio, min_ratio, max_ratio;

   shAssert(star_array_A != NULL);
   shAssert(star_array_B != NULL);
   shAssert(t_array_A != NULL);
   shAssert(t_array_B != NULL);
   shAssert(nbright > 0);

   min_ratio = scale - (0.01*AT_MATCH_PERCENT*scale);
   max_ratio = scale + (0.01*AT_MATCH_PERCENT*scale);
     
   /* allocate and initialize the "vote_matrix" */
   vote_matrix = (int **) shMalloc(nbright*sizeof(int *));
   for (i = 0; i < nbright; i++) {
      vote_matrix[i] = (int *) shMalloc(nbright*sizeof(int));
      for (j = 0; j < nbright; j++) {
	 vote_matrix[i][j] = 0;
      }
   }

#ifdef DEBUG
   printf("make_vote_matrix: here come the %d brightest in list A \n",
	    nbright);
   print_star_array(star_array_A, nbright);
   printf("make_vote_matrix: here come the %d brightest in list B \n",
	    nbright);
   print_star_array(star_array_B, nbright);
#endif


   /* 
    * now, the triangles in "t_array_A" have been sorted by their "ba"
    * values.  Therefore, we walk through the OTHER array, "t_array_B",
    * and for each triangle tri_B in it
    *  
    *      1a. set  ba_min = tri_B.ba - max_radius
    *      1b. set  ba_max = tri_B.ba + max_radius
    *
    * We'll use these values to limit our selection from t_array_A.
    *
    *      2. find the first triangle in t_array_A which has
    *                     ba > ba_min
    *      3. starting there, step through t_array_A, calculating the
    *                     Euclidean distance between tri_B and
    *                     the current triangle from array A.
    *      4. stop stepping through t_array_A when we each a triangle
    *                     with ba > ba_max
    */
   rad2 = max_radius*max_radius;
   for (j = 0; j < num_triangles_B; j++) {

#ifdef DEBUG2
      printf("make_vote_matrix: looking for matches to B %d\n", j);
#endif
      ba_B = t_array_B[j].ba;
      ca_B = t_array_B[j].ca;
      ba_min = ba_B - max_radius;
      ba_max = ba_B + max_radius;
#ifdef DEBUG2
      printf("   ba_min = %7.3f  ba_max = %7.3f\n", ba_min, ba_max);
#endif

      start_index = find_ba_triangle(t_array_A, num_triangles_A, ba_min);
      for (i = start_index; i < num_triangles_A; i++) {
#ifdef DEBUG2
         printf("   looking at A %d\n", i);
#endif
	 ba_A = t_array_A[i].ba;
	 ca_A = t_array_A[i].ca;

	 /* check to see if we can stop looking through A yet */
	 if (ba_A > ba_max) {
	    break;
	 }

	 if ((ba_A - ba_B)*(ba_A - ba_B) + (ca_A - ca_B)*(ca_A - ca_B) < rad2) {

	    /*
	     * check the ratio of lengths of side "a", and discard this
	     * candidate if its outside the allowed range
	     */
	    if (scale != -1) {
	       ratio = t_array_A[i].a_length/t_array_B[j].a_length;
	       if (ratio < min_ratio || ratio > max_ratio) {
#ifdef DEBUG2
		  printf("   skip    ratio = %7.3f \n", ratio);
#endif
	          continue;
	       }
	    }


	    /* we have a (possible) match! */
#ifdef DEBUG2
	    printf("   match!  A: (%6.3f, %6.3f)   B: (%6.3f, %6.3f) \n",
		  ba_A, ca_A, ba_B, ca_B);
#endif
	    /* 
	     * increment the vote_matrix cell for each matching pair 
	     * of stars, one at each vertex
	     */
	    vote_matrix[t_array_A[i].a_index][t_array_B[j].a_index]++;
	    vote_matrix[t_array_A[i].b_index][t_array_B[j].b_index]++;
	    vote_matrix[t_array_A[i].c_index][t_array_B[j].c_index]++;

	 }
      }
   }

#ifdef DEBUG
   print_vote_matrix(vote_matrix, nbright);
#endif
	    

   return(vote_matrix);
}


/************************************************************************
 * 
 *
 * ROUTINE: print_vote_matrix
 *
 * DESCRIPTION:
 * Print out the "vote_matrix" in a nice format.
 *
 * For debugging purposes.
 *
 * RETURN:
 *    nothing
 *
 * </AUTO>
 */

#ifdef DEBUG

static void
print_vote_matrix
   (
   int **vote_matrix,     /* I: the 2-D array we'll print out */
   int numcells           /* I: number of cells in each row and col of matrix */
   )
{
   int i, j;

   printf("here comes vote matrix\n");
   for (i = 0; i < numcells; i++) {
      for (j = 0; j < numcells; j++) {
	 printf(" %3d", vote_matrix[i][j]);
      }
      printf("\n");
   }
}

#endif /* DEBUG */


/************************************************************************
 * 
 *
 * ROUTINE: top_vote_getters
 *
 * DESCRIPTION:
 * Given a vote_matrix which has been filled in, 
 * which has 'num' rows and columns, we need to pick the
 * top 'num' vote-getters.  We call 'top_vote_getters'
 * and are given, in its output arguments, pointers to three
 * arrays, each of which has 'num' elements pertaining
 * to a matched pair of STARS:
 * 
 *       winner_votes[]    number of votes of winners, in descending order
 *       winner_index_A[]  index of star in star_array_A 
 *       winner_index_B[]  index of star in star_array_B
 *
 * Thus, the pair of stars which matched in the largest number
 * of triangles will be 
 *
 *       star_array_A[winner_index_A[0]]    from array A
 *       star_array_B[winner_index_A[0]]    from array B
 *
 * and the pair of stars which matched in the second-largest number
 * of triangles will be 
 *
 *       star_array_A[winner_index_A[1]]    from array A
 *       star_array_B[winner_index_A[1]]    from array B
 * 
 * and so on.
 *
 * RETURN:
 *    SH_SUCCESS         if all goes well
 *    SH_GENERIC_ERROR   if not
 *
 * </AUTO>
 */

static int
top_vote_getters
   (
   int **vote_matrix,     /* I: the 2-D array, already filled in */
   int num,               /* I: # of rows and cols in vote_matrix */
                          /*      also the number of elements in the next */
                          /*      three output arrays */
   int **winner_votes,    /* O: create this array of # of votes for the */
                          /*      'num' cells with the most votes */
   int **winner_index_A,  /* O: create this array of index into star array A */
                          /*      of the 'num' cells with most votes */
   int **winner_index_B   /* O: create this array of index into star array B */
                          /*      of the 'num' cells with most votes */
   )
{
   int i, j, k, l;
   int *w_votes;       /* local ptr to (*winner_votes), for convenience */
   int *w_index_A;     /* local ptr to (*winner_index_A), for convenience */
   int *w_index_B;     /* local ptr to (*winner_index_B), for convenience */

   /* first, create the output arrays */
   *winner_votes = (int *) shMalloc(num*sizeof(int));
   *winner_index_A = (int *) shMalloc(num*sizeof(int));
   *winner_index_B = (int *) shMalloc(num*sizeof(int));

   /* this will simplify code inside this function */
   w_votes = *winner_votes;
   w_index_A = *winner_index_A;
   w_index_B = *winner_index_B;

   /* 
    * initialize all elements of the output arrays.  Use -1 as the
    * index in "w_index" arrays, to indicate an empty place
    * with no real winner.
    */
   for (i = 0; i < num; i++) {
      w_votes[i] = 0;
      w_index_A[i] = -1;
      w_index_B[i] = -1;
   }

   /* 
    * now walk through the vote_matrix, using insertion sort to place
    * a cell into the "winner" arrays if it has more votes than the
    * least popular winner so far (i.e. w_votes[num - 1])
    */ 
   for (i = 0; i < num; i++) {
      for (j = 0; j < num; j++) {
	 if (vote_matrix[i][j] > w_votes[num - 1]) {

	    /* have to insert this cell's values into the winner arrays */
	    for (k = 0; k < num; k++) {
	       if (vote_matrix[i][j] > w_votes[k]) {

		  /* move all other winners down one place */
		  for (l = num - 2; l >= k; l--) {
		     w_votes[l + 1] = w_votes[l];
		     w_index_A[l + 1] = w_index_A[l];
		     w_index_B[l + 1] = w_index_B[l];
		  }
		  /* insert the new item in its place */
		  w_votes[k] = vote_matrix[i][j];
		  w_index_A[k] = i;
		  w_index_B[k] = j;
	          break;
	       }
	    }
	 }
      }
   }

#ifdef DEBUG
   printf("  in top_vote_getters, we have top %d \n", num);
   for (i = 0; i < num; i++) {
      printf("   index_A %4d    index_B %4d    votes %4d\n", 
		  w_index_A[i], w_index_B[i], w_votes[i]);
   }
#endif

   return(SH_SUCCESS);
}



/************************************************************************
 * 
 *
 * ROUTINE: calc_trans
 *
 * DESCRIPTION:
 * Given a set of "nbright" matched pairs of stars, which we can
 * extract from the "winner_index" and "star_array" arrays,
 * figure out a TRANS structure which takes coordinates of 
 * objects in set A and transforms then into coords for set B.
 * A TRANS contains the six coefficients in the equations
 *
 *                x' =  A + Bx + Cy
 *                y' =  D + Ex + Fy
 *
 * where (x,y) are coords in set A and (x',y') are corresponding
 * coords in set B.  The higher-order distortion and color terms are all
 * set to 0.
 *
 * Internally, I'm going to solve for the very similar equations
 *
 *                x' = Ax + By + C
 *                y' = Dx + Ey + F
 *
 * and then just re-arrange the coefficients at the very end.  OK?
 *              
 *
 * What we do is to treat each of the two equations above
 * separately.  We can write down 3 equations relating quantities
 * in the two sets of points (there are more than 3 such equations,
 * but we don't seek an exhaustive list).  For example,
 *
 *       a.       x'    =  Ax     + By    +  C
 *       b.       x'x   =  Ax^2   + Bxy   +  Cx      (mult both sides by x)
 *       c.       x'y   =  Axy    + By^2  +  Cy      (mult both sides by y)
 *
 * Now, since we have "nbright" matched pairs, we can take each of 
 * the above 3 equations and form the sums on both sides, over
 * all "nbright" points.  So, if S(x) represents the sum of the quantity
 * "x" over all nbright points, and if we let N=nbright, then
 *
 *       a.     S(x')   =  AS(x)   + BS(y)   +  CN
 *       b.     S(x'x)  =  AS(x^2) + BS(xy)  +  CS(x) 
 *       c.     S(x'y)  =  AS(xy)  + BS(y^2) +  CS(y)
 *
 * At this point, we have a set of three equations, and 3 unknowns: A, B, C.
 * We can write this set of equations as a matrix equation
 *
 *               b       = M * v
 *
 * where we KNOW the quantities
 *
 *        vector b = ( S(x'), S(x'x), S(x'y) )
 *
 *        matrix M = ( S(x)   S(y)    1      )
 *                   ( S(x^2) S(xy)   S(x)   )
 *                   ( S(xy)  S(y^2)  S(y)   )
 *
 *
 * and we want to FIND the unknown 
 *
 *        vector v = ( A,     B,      C      )
 *
 * So, how to solve this matrix equation?  We use an LU decomposition 
 * method described in "Numerical Recipes", Chapter 2.   We solve
 * for A, B, C (and equivalently for D, E, F), then fill in the fields
 * of the given TRANS structure argument.
 *
 * It's possible that the matrix will be singular, and we can't find
 * a solution.  In that case, we print an error message and don't touch
 * the TRANS' fields.
 *
 *    [should explain how we make an iterative solution here,
 *     but will put in comments later.  MWR ]
 *
 * RETURN:
 *    SH_SUCCESS           if all goes well
 *    SH_GENERIC_ERROR     if we can't find a solution
 *
 * </AUTO>
 */

static int
calc_trans
   (
   int nbright,             /* I: max number of stars we use in calculating */
                            /*      the transformation; we may cut down to */
                            /*      a more well-behaved subset. */
   s_star *star_array_A,    /* I: first array of s_star structure we match */
                            /*      the output TRANS takes their coords */
                            /*      into those of array B */
   int num_stars_A,         /* I: total number of stars in star_array_A */
   s_star *star_array_B,    /* I: second array of s_star structure we match */
   int num_stars_B,         /* I: total number of stars in star_array_B */
   int *winner_index_A,     /* I: index into "star_array_A" of top */
                            /*      vote-getters */
   int *winner_index_B,     /* I: index into "star_array_B" of top */
                            /*      vote-getters */
   TRANS *trans             /* O: place solved coefficients into this */
                            /*      existing structure's fields */
   )
{
   int i; 
   float **matrix;
   float vector[3];
   float solved_a, solved_b, solved_c, solved_d, solved_e, solved_f;
   s_star *s1, *s2;
/* */
   float sum, sumx1, sumy1, sumx2, sumy2;
   float sumx1sq, sumy1sq;
   float sumx1y1, sumx1x2, sumx1y2;
   float sumy1x2, sumy1y2;


   shAssert(nbright >= AT_MATCH_REQUIRE);


   /*
    * allocate a matrix we'll need for this function
    */
   matrix = alloc_matrix(3);


   /* 
    * first, we consider the coefficients A, B, C in the trans.
    * we form the sums that make up the elements of matrix M 
    */
   sum = 0.0;
   sumx1 = 0.0;
   sumy1 = 0.0;
   sumx2 = 0.0;
   sumy2 = 0.0;
   sumx1sq = 0.0;
   sumy1sq = 0.0;
   sumx1x2 = 0.0;
   sumx1y1 = 0.0;
   sumx1y2 = 0.0;
   sumy1x2 = 0.0;
   sumy1y2 = 0.0;

   for (i = 0; i < nbright; i++) {
      
      /* sanity checks */
      shAssert(winner_index_A[i] < num_stars_A);
      shAssert(winner_index_A[i] >= 0);
      s1 = &(star_array_A[winner_index_A[i]]);
      shAssert(winner_index_B[i] < num_stars_B);
      shAssert(winner_index_B[i] >= 0);
      s2 = &(star_array_B[winner_index_B[i]]);

      /* elements of the matrix */
      sum += 1.0;
      sumx1 += s1->x;
      sumx2 += s2->x;
      sumy1 += s1->y;
      sumy2 += s2->y;
      sumx1sq += s1->x*s1->x;
      sumy1sq += s1->y*s1->y;
      sumx1x2 += s1->x*s2->x;
      sumx1y1 += s1->x*s1->y;
      sumx1y2 += s1->x*s2->y;
      sumy1x2 += s1->y*s2->x;
      sumy1y2 += s1->y*s2->y;

   }


   /* 
    * now turn these sums into a matrix and a vector
    */
   matrix[0][0] = sumx1sq;
   matrix[0][1] = sumx1y1;
   matrix[0][2] = sumx1;
   matrix[1][0] = sumx1y1;
   matrix[1][1] = sumy1sq;
   matrix[1][2] = sumy1;
   matrix[2][0] = sumx1;
   matrix[2][1] = sumy1;
   matrix[2][2] = sum;

   vector[0] = sumx1x2;
   vector[1] = sumy1x2;
   vector[2] = sumx2;

#ifdef DEBUG
   printf("before calling solution routines for ABC, here's matrix\n");
   print_matrix(matrix, 3);
#endif

   /*
    * and now call the Numerical Recipes routines to solve the matrix.
    * The solution for TRANS coefficients A, B, C will be placed
    * into the elements on "vector" after "gauss_jordon" finishes.
    */
   if (gauss_jordon(matrix, 3, vector) != SH_SUCCESS) {
      shError("calc_trans: can't solve for coeffs A,B,C ");
      return(SH_GENERIC_ERROR);
   }

#ifdef DEBUG
   printf("after  calling solution routines, here's matrix\n");
   print_matrix(matrix, 3);
#endif

   solved_a = vector[0];
   solved_b = vector[1];
   solved_c = vector[2];


   /*
    * Okay, now we solve for TRANS coefficients D, E, F, using the
    * set of equations that relates y' to (x,y)
    *
    *       a.       y'    =  Dx     + Ey    +  F
    *       b.       y'x   =  Dx^2   + Exy   +  Fx      (mult both sides by x)
    *       c.       y'y   =  Dxy    + Ey^2  +  Fy      (mult both sides by y)
    *
    */
   matrix[0][0] = sumx1sq;
   matrix[0][1] = sumx1y1;
   matrix[0][2] = sumx1;
   matrix[1][0] = sumx1y1;
   matrix[1][1] = sumy1sq;
   matrix[1][2] = sumy1;
   matrix[2][0] = sumx1;
   matrix[2][1] = sumy1;
   matrix[2][2] = sum;

   vector[0] = sumx1y2;
   vector[1] = sumy1y2;
   vector[2] = sumy2;

#ifdef DEBUG
   printf("before calling solution routines for DEF, here's matrix\n");
   print_matrix(matrix, 3);
#endif

   /*
    * and now call the Numerical Recipes routines to solve the matrix.
    * The solution for TRANS coefficients D, E, F will be placed
    * into the elements on "vector" after "gauss_jordon" finishes.
    */
   if (gauss_jordon(matrix, 3, vector) != SH_SUCCESS) {
      shError("calc_trans: can't solve for coeffs D,E,F ");
      return(SH_GENERIC_ERROR);
   }

#ifdef DEBUG
   printf("after  calling solution routines, here's matrix\n");
   print_matrix(matrix, 3);
#endif

   solved_d = vector[0];
   solved_e = vector[1];
   solved_f = vector[2];


   /*
    * assign the coefficients we've just calculated to the output
    * TRANS structure.  Recall that we've solved equations 
    *
    *     x' = Ax + By + C
    *     y' = Dx + Ey + F
    *
    * but that the TRANS structure assigns its coefficients assuming
    *
    *     x' = A + Bx + Cy
    *     y' = D + Ex + Fy
    * 
    * so, here, we have to re-arrange the coefficients a bit.
    */
   trans->a = solved_c;
   trans->b = solved_a;
   trans->c = solved_b;
   trans->d = solved_f;
   trans->e = solved_d;
   trans->f = solved_e;
   trans->dRow0 = 0;
   trans->dRow1 = 0;
   trans->dRow2 = 0;
   trans->dRow3 = 0;
   trans->dCol0 = 0;
   trans->dCol1 = 0;
   trans->dCol2 = 0;
   trans->dCol3 = 0;
   trans->csRow = 0;
   trans->csCol = 0;
   trans->ccRow = 0;
   trans->ccCol = 0;
   trans->riCut = 0;

   /* 
    * free up memory we allocated for this function
    */
   free_matrix(matrix, 3);

   return(SH_SUCCESS);
}



/************************************************************************
 * 
 *
 * ROUTINE: alloc_matrix
 *
 * DESCRIPTION:
 * Allocate space for an NxN matrix of floating-point values,
 * return a pointer to the new matrix.
 *
 * RETURNS:
 *   float **           pointer to new matrix
 *
 *
 * </AUTO>
 */

static float **
alloc_matrix
   (
   int n              /* I: number of elements in each row and col */
   )
{
   int i;
   float **matrix;

   matrix = (float **) shMalloc(n*sizeof(float *));
   for (i = 0; i < n; i++) {
      matrix[i] = (float *) shMalloc(n*sizeof(float));
   }
   
   return(matrix);
}


/************************************************************************
 * 
 *
 * ROUTINE: free_matrix 
 *
 * DESCRIPTION:
 * Free the space allocated for the given nxn matrix.
 *
 * RETURNS:
 *   nothing
 *
 * </AUTO>
 */

static void
free_matrix
   (
   float **matrix,    /* I: pointer to 2-D array to be freed */
   int n              /* I: number of elements in each row and col */
   )
{
   int i;

   for (i = 0; i < n; i++) {
      shFree(matrix[i]);
   }
   shFree(matrix);
}


/************************************************************************
 * 
 *
 * ROUTINE: print_matrix 
 *
 * DESCRIPTION:
 * print out a nice picture of the given matrix.  
 *
 * For debugging purposes.
 *
 * RETURNS:
 *   nothing
 *
 * </AUTO>
 */

#ifdef DEBUG

static void
print_matrix
   (
   float **matrix,    /* I: pointer to 2-D array to be printed */
   int n              /* I: number of elements in each row and col */
   )
{
   int i, j;

   for (i = 0; i < n; i++) {
      for (j = 0; j < n; j++) {
         printf(" %12.5f", matrix[i][j]);
      }
      printf("\n");
   }
}

#endif /* DEBUG */


/************************************************************************
 * 
 *
 * ROUTINE: print_trans 
 *
 * DESCRIPTION:
 * Print the elements of a TRANS structure.
 *
 * For use in debugging.
 *
 * RETURNS:
 *   nothing
 *
 * </AUTO>
 */

#ifdef DEBUG

static void
print_trans
   (
   TRANS *trans       /* I: TRANS to print out */
   )
{

   printf("TRANS: a=%7.2f b=%7.2f c=%7.2f d=%7.2f e=%7.2f f=%7.2f\n",
      trans->a, trans->b, trans->c, trans->d, trans->e, trans->f);
}

#endif /* DEBUG */



/*
 * check to see if my versions of NR routines have bugs.
 * Try to invert a matrix.
 * 
 * debugging only.
 */

#ifdef DEBUG2

static void
test_routine (void)
{
	int i, j, k, n;
	int *permutations;
	float **matrix1, **matrix2, **inverse;
	float *vector;
	float *col;
	float d_sign = 1;
	float sum;

	n = 2;
	matrix1 = (float **) shMalloc(n*sizeof(float *));
	matrix2 = (float **) shMalloc(n*sizeof(float *));
	inverse = (float **) shMalloc(n*sizeof(float *));
	vector = (float *) shMalloc(n*sizeof(float));
   	for (i = 0; i < n; i++) {
		matrix1[i] = (float *) shMalloc(n*sizeof(float));
		matrix2[i] = (float *) shMalloc(n*sizeof(float));
		inverse[i] = (float *) shMalloc(n*sizeof(float));
	}
	permutations = (int *) shMalloc(n*sizeof(int));
	col = (float *) shMalloc(n*sizeof(float));


	/* fill the matrix */
	matrix1[0][0] = 1.0;
	matrix1[0][1] = 2.0;
	matrix1[1][0] = 3.0;
	matrix1[1][1] = 4.0;

	/* fill the vector */
	for (i = 0; i < n; i++) {
		vector[i] = 0;
	}

	/* copy matrix1 into matrix2, so we can compare them later */
	for (i = 0; i < n; i++) {
		for (j = 0; j < n; j++) {
			matrix2[i][j] = matrix1[i][j];
		}
	}

	/* now check */
	printf(" here comes original matrix \n");
	print_matrix(matrix1, n);


	/* now invert matrix1 */
	for (i = 0; i < n; i++) {
		for (j = 0; j < n; j++) {
			inverse[i][j] = matrix1[i][j];
		}
	}
	gauss_jordon(inverse, n, vector);

	/* now check */
	printf(" here comes inverse matrix \n");
	print_matrix(inverse, n);

	/* find out if the product of "inverse" and "matrix2" is identity */
	sum = 0.0;
	for (i = 0; i < n; i++) {
		for (j = 0; j < n; j++) {
			for (k = 0; k < n; k++) {
				sum += inverse[i][k]*matrix2[k][j];
			}
			matrix1[i][j] = sum;
			sum = 0.0;
		}
	}

	printf(" here comes what we hope is identity matrix \n");
	print_matrix(matrix1, n);
}

#endif /* DEBUG2 */


/************************************************************************
 * 
 *
 * ROUTINE: gauss_jordon
 *
 * DESCRIPTION:
 * Given an square NxN matrix A, and a 1xN vector b, solve the
 * matrix equation
 *
 *        A x = b
 * 
 * for the unknown 1xN vector "x".  
 *
 * This function is an implementation of Gauss-Jordon elimination,
 * based on the "gaussj" routine in Numerical
 * Recipes.  See that text for details.
 *
 * The input matrix is inverted in-place, so that the original matrix
 * values are lost.  
 *
 * The input vector is replaced by the output vector values.
 *
 * If an error occurs (if the matrix is singular), this prints an error
 * message and returns with error code.
 *
 * RETURN:
 *    SH_SUCCESS          if all goes well
 *    SH_GENERIC_ERROR    if not -- if matrix is singular
 *
 * </AUTO>
 */

#define SWAP(a,b)  { float temp = (a); (a) = (b); (b) = temp; }

static int
gauss_jordon
   (
   float **matrix,        /* I/O: the square 2-D matrix we'll invert */
                          /*      will hold inverse matrix on output */
   int num,               /* I: number of rows and cols in matrix */
   float *vector          /* I/O: vector which holds "b" values in input */
                          /*      and the solution vector "x" on output */
   )
{
   int *indxc, *indxr, *ipiv;
   int i, j, k, l, ll;
   int icol=0, irow=0;
   float big, dummy, pivinv;
   
   indxc = (int *) shMalloc(num*sizeof(int));
   indxr = (int *) shMalloc(num*sizeof(int));
   ipiv = (int *) shMalloc(num*sizeof(int));

   for (j = 0; j < num; j++) {
      ipiv[j] = 0;
   }
   for (i = 0; i < num; i++) {
      big = 0.0;
      for (j = 0; j < num; j++) {
	 if (ipiv[j] != 1) {
	    for (k = 0; k < num; k++) {
	       if (ipiv[k] == 0) {
		  if (fabs(matrix[j][k]) >= big) {
		     big = fabs(matrix[j][k]);
		     irow = j;
		     icol = k;
		  }
	       }
	       else {
		  if (ipiv[k] > 1) {
		     shError("gauss_jordon: singular matrix 1");
		     shFree(indxc);
		     shFree(indxr);
		     shFree(ipiv);
		     return(SH_GENERIC_ERROR);
		  }
	       }
	    }
	 }
      }
      ++(ipiv[icol]);

      if (irow != icol) {
	 for (l = 0; l < num; l++) {
	    SWAP(matrix[irow][l], matrix[icol][l]);
	 }
	 SWAP(vector[irow], vector[icol]);
      }

      indxr[i] = irow;
      indxc[i] = icol;
      if (matrix[icol][icol] == 0.0) {
	 shError("gauss_jordon: singular matrix 1");
	 shFree(indxr);
	 shFree(indxc);
	 shFree(ipiv);
	 return(SH_GENERIC_ERROR);
      }

      pivinv = 1.0/matrix[icol][icol];
      matrix[icol][icol] = 1.0;
      for (l = 0; l < num; l++) {
	 matrix[icol][l] *= pivinv;
      }
      vector[icol] *= pivinv;
      for (ll = 0; ll < num; ll++) {
	 if (ll != icol) {
	    dummy = matrix[ll][icol];
	    matrix[ll][icol] = 0.0;
	    for (l = 0; l < num; l++) {
	       matrix[ll][l] -= matrix[icol][l]*dummy;
	    }
	    vector[ll] -= vector[icol]*dummy;
	 }
      }
   }

   for (l = num - 1; l >= 0; l--) {
      if (indxr[l] != indxc[l]) {
	 for (k = 0; k < num; k++) {
	    SWAP(matrix[k][indxr[l]], matrix[l][indxc[l]]);
	 }
      }
   }

   shFree(indxr);
   shFree(indxc);
   shFree(ipiv);

   return(SH_SUCCESS);
}



/************************************************************************
 * 
 *
 * ROUTINE: iter_trans 
 *
 * DESCRIPTION:
 * We want to find a TRANS structures that takes coords of objects in
 * set A and transforms to coords of objects in set B.  We have a
 * a subset of 'nmatched' candidates for matched pairs of points.
 * However, some of these may be false matches.  Here's how we try 
 * to eliminate them, and use all remaining true matches to derive
 * the transformation.
 *
 *    1. start with N = nmatched candidate pairs of points
 *
 *           Try to get rid of "clearly" spurious matches
 *
 *    2. let Mv = # votes given to best single pair
 *    3. calculate M = Mv*AT_MATCH_QUALIFY 
 *    4. attempt to disquality any pair with < M votes; however,
 *            leave at least AT_MATCH_STARTN pairs.  This will
 *            decrease N, usually.
 *
 *           Prepare to calculate initial guess at TRANS
 *
 *    5. choose AT_MATCH_STARTN best pairs (assert that N >= AT_MATCH_STARTN)
 *    6. set Nr = AT_MATCH_STARTN
 *
 *           Enter loop in which we calculate TRANS over and over
 *      
 *    7. calculate a TRANS structure using the best Nr points
 *            (where "best" means "highest in winner_index" arrays)
 *    8.   transform all N points from coords in A to coords in B
 *    9.   calculate Euclidean square-of-distance between all Nr points
 *                             in coord system B
 *   10.   sort these Euclidean values
 *   11.   remove any pairs where distance is > maxdist
 *   12.   pick the AT_MATCH_PERCENTILE'th value from the sorted array
 *             (call it "sigma")
 *   13.   let Nb = number of candidate matched pairs which have 
 *                       square-of-distance > 2*sigma
 *   14.   if Nb == 0, we're done -- quit
 *   15.   if Nb > 0, 
 *                       remove all Nb candidates from matched pair arrays
 *                       set Nr = Nr - Nb
 *                       go to step 7
 *
 * Note that if we run out of candidate pairs, so that Nr < AT_MATCH_REQUIRE, 
 * we print an error message and return SH_GENERIC_ERROR.
 *
 * RETURNS:
 *   SH_SUCCESS          if we were able to determine a good TRANS
 *   SH_GENERIC_ERROR    if we couldn't
 *
 * </AUTO>
 */

static int
iter_trans
   (
   int nbright,             /* I: max number of stars we use in calculating */
                            /*      the transformation; we may cut down to */
                            /*      a more well-behaved subset. */
   s_star *star_array_A,    /* I: first array of s_star structure we match */
                            /*      the output TRANS takes their coords */
                            /*      into those of array B */
   int num_stars_A,         /* I: total number of stars in star_array_A */
   s_star *star_array_B,    /* I: second array of s_star structure we match */
   int num_stars_B,         /* I: total number of stars in star_array_B */
   int *winner_votes,       /* I: number of votes gotten by the top 'nbright' */
                            /*      matched pairs of stars */
                            /*      We may modify this array */
   int *winner_index_A,     /* I: index into "star_array_A" of top */
                            /*      vote-getters */
                            /*      We may modify this array */
   int *winner_index_B,     /* I: index into "star_array_B" of top */
                            /*      vote-getters */
                            /*      We may modify this array */
   float maxdist,           /* I: after initial guess, only accept pairs */
                            /*      which are closer than this distance */
                            /*      in coord system B */
   TRANS *trans             /* O: place solved coefficients into this */
                            /*      existing structure's fields */
   )
{
   int i, j;
   int nr;           /* number of matched pairs remaining in solution */
   int nb;           /* number of bad pairs in any iteration */
   int is_ok;
   int top_votes, min_votes;
   float *dist2, *dist2_sorted;
   float xdiff, ydiff;
   float max_dist2;
   float sigma;
   s_star *sa, *sb;
   s_star *a_prime;  /* will hold transformed version of stars in set A */
   int true;         /* used to prevent compiler warning in while loop */


   shAssert(nbright >= AT_MATCH_REQUIRE);
   shAssert(star_array_A != NULL);
   shAssert(star_array_B != NULL);
   shAssert(winner_votes != NULL); 
   shAssert(winner_index_A != NULL);
   shAssert(winner_index_B != NULL);
   shAssert(trans != NULL);

   /* these should already have been checked, but it doesn't hurt */
   shAssert(num_stars_A >= nbright);
   shAssert(num_stars_B >= nbright);

   /*
    * Do a first attempt at pruning away bad matches.  We try to get
    * rid of any match with fewer votes than AT_MATCH_QUALIFY times
    * the top number of votes.  However, we keep at least AT_MATCH_STARTN
    * pairs.  Re-set the argument "nbright" to be equal to the
    * number of matches which satisfy this condition.
    */
   top_votes = winner_votes[0];
   min_votes = top_votes*AT_MATCH_QUALIFY;
   for (i = nbright - 1; i > AT_MATCH_STARTN; i--) {
      if (winner_votes[i] >= min_votes) {
	 break;
      }
   }
   nbright = i;


   /*
    * check to see if there are at least AT_MATCH_STARTN valid 
    * matches.  An invalid match is one with 0 votes, or one with
    * a "winner_index" value of -1.
    *
    * If there are fewer than AT_MATCH_STARTN valid matches,
    * we have to give up and return SH_GENERIC_ERROR.
    */
   for (i = 0; i < nbright; i++) {
      if ((winner_votes[i] == 0) || (winner_index_A[i] < 0) ||
          (winner_index_B[i] < 0)) {
	 shError("iter_trans: too few valid matches to calc initial TRANS");
	 return(SH_GENERIC_ERROR);
      }
   }

   /*
    * calculate the TRANS for the "best" AT_MATCH_STARTN points
    */
   if (calc_trans(AT_MATCH_STARTN, 
                  star_array_A, num_stars_A,
                  star_array_B, num_stars_B,
                  winner_index_A, winner_index_B,
                  trans) != SH_SUCCESS) {
      shError("iter_trans: calc_trans returns with error");
      return(SH_GENERIC_ERROR);
   }

   /*
    * Now, we are going to enter the iteration with a set of the "best" 
    * matched pairs.  Recall that
    * "winner_index" arrays are already sorted in decreasing order
    * of goodness, so that "winner_index_A[0]" is the best.
    * As we iterate, we may discard some matches, and then 'nr' will
    * get smaller.  It must always be more than AT_MATCH_REQUIRE,
    * or else 'calc_trans' will fail.
    */
   nr = nbright;

   /*
    * We're going to need an array of (at most) 'nbright' stars
    * which hold the coordinates of stars in set A, after they've
    * been transformed into coordinates system of set B.
    */
   a_prime = (s_star *) shMalloc(nbright*sizeof(s_star));

   /*
    * And this will be an array to hold the Euclidean square-of-distance
    * between a transformed star from set A and its partner from set B.
    *
    * "dist2_sorted" is a copy of the array which we'll sort .. but we need
    * to keep the original order, too.
    */
   dist2 = (float *) shMalloc(nbright*sizeof(float));
   dist2_sorted = (float *) shMalloc(nbright*sizeof(float));

   /*
    * we don't allow any candidate matches which cause the stars to
    * differ by more than this much in the common coord system.
    */
   max_dist2 = maxdist*maxdist;

   /*
    * now, we enter a loop that may execute several times.
    * We calculate the transformation for current 'nr' best points,
    * then check to see if we should throw out any matches because
    * the resulting transformed coordinates are too discrepant.
    * We break out of this loop near the bottom, with a status
    * provided by "is_ok" 
    * 
    *       is_ok = 1              all went well, can return success
    *       is_ok = 0              we failed for some reason. 
    */
   is_ok = 0;

/* include true to prevent compiler warning under IRIX6 */
   true = 1;
   while (true==1) {

#ifdef DEBUG
      printf("iter_trans: at top of loop, nr=%4d\n", nr);
#endif

      nb = 0;

      /*
       * apply the TRANS to the A stars in all 'nr' matched pairs.
       * we make a new set of s_stars with the transformed coordinates,
       * called "a_prime".
       */
      for (i = 0; i < nr; i++) {
	 sa = &(star_array_A[winner_index_A[i]]);
	 a_prime[i].x = trans->a + trans->b*sa->x + trans->c*sa->y;
	 a_prime[i].y = trans->d + trans->e*sa->x + trans->f*sa->y;
      }
      

      /*
       * calculate the square-of-distance between a transformed star 
       * (from set A) and its partner from set B, in the coordinate system 
       * of set B.
       */
      for (i = 0; i < nr; i++) {
	 sb = &(star_array_B[winner_index_B[i]]);
	 xdiff = a_prime[i].x - sb->x;
	 ydiff = a_prime[i].y - sb->y;
	 dist2[i] = (xdiff*xdiff + ydiff*ydiff);
	 dist2_sorted[i] = dist2[i];
#ifdef DEBUG
	 printf("   match %3d  (%10.3f,%10.3f) vs. (%10.3f,%10.3f)  d2=%8.4f\n",
		  i, a_prime[i].x, a_prime[i].y, sb->x, sb->y, dist2[i]);
#endif
      }

      /*
       * sort the array of square-of-distances
       */
      qsort((char *) dist2_sorted, nr, sizeof(float), (PFI) compare_float);


      /*
       * now, check to see if any matches have dist2 > max_dist2.
       * If so,
       *
       *     - remove them from the winner_votes and winner_index arrays
       *     - decrement 'nr'
       *     - also decrement the loop counter 'i', because we're going
       *            to move up all items in the "winner" and "dist2" arrays
       *            as we discard the bad match
       *     - increment 'nb'
       */
      for (i = 0; i < nr; i++) {
         if (dist2[i] > max_dist2) {

            /*
             * remove the entry for the "bad" match from the "winner" arrays
             * and from the "dist2" array
             */
#ifdef DEBUG
            printf("  removing old match with d2=%8.0f\n", dist2[i]);
#endif
            for (j = i + 1; j < nr; j++) {
               winner_votes[j - 1] = winner_votes[j];
               winner_index_A[j - 1] = winner_index_A[j];
               winner_index_B[j - 1] = winner_index_B[j];
               dist2[j - 1] = dist2[j];
            }

            /*
             * and modify our counters of "remaining good matches" and
             * "bad matches this time", too.
             */
            nr--;          /* one fewer good match remains */
            nb++;          /* one more bad match during this iteration */

            /*
             * and decrement 'i', too, since we must moved element
             * i+1 to the place i used to be, and we must check _it_.
             */
            i--;
         }
      }
#ifdef DEBUG
      printf("   nr now %4d, nb now %4d\n", nr, nb);
#endif



      /*
       * pick the square-of-distance which occurs at the AT_MATCH_PERCENTILE
       * place in the sorted array.  Call this value "sigma".  We'll clip
       * any matches that are more than 2*"sigma".
       */
      sigma = find_percentile(dist2_sorted, nr, (float) AT_MATCH_PERCENTILE);
#ifdef DEBUG
      printf("   sigma = %10.3f\n", sigma);
#endif

      /*
       * now, check to see if any matches have dist2 > 2*sigma.  If so,
       *
       *     - remove them from the winner_votes and winner_index arrays
       *     - decrement 'nr' 
       *     - also decrement the loop counter 'i', because we're going 
       *            to move up all items in the "winner" and "dist2" arrays
       *            as we discard the bad match
       *     - increment 'nb'
       */
      for (i = 0; i < nr; i++) {
	 if (dist2[i] > 2.0*sigma) {

	    /* 
	     * remove the entry for the "bad" match from the "winner" arrays
	     * and from the "dist2" array
	     */
#ifdef DEBUG
	    printf("  removing old match with d2=%8.4f\n", dist2[i]);
#endif
	    for (j = i + 1; j < nr; j++) {
	       winner_votes[j - 1] = winner_votes[j];
	       winner_index_A[j - 1] = winner_index_A[j];
	       winner_index_B[j - 1] = winner_index_B[j];
	       dist2[j - 1] = dist2[j];
	    }

	    /* 
	     * and modify our counters of "remaining good matches" and
	     * "bad matches this time", too.
	     */
	    nr--;          /* one fewer good match remains */
	    nb++;          /* one more bad match during this iteration */

	    /*
	     * and decrement 'i', too, since we must moved element
	     * i+1 to the place i used to be, and we must check _it_.
	     */
	    i--;
	 }
      }
#ifdef DEBUG
      printf("   nr now %4d, nb now %4d\n", nr, nb);
#endif


      /*
       * Okay, let's evaluate what has happened so far:
       *    - if nb == 0, then all remaining matches are good
       *    - if nb > 0, we need to iterate again
       *    - if nr < AT_MATCH_REQUIRE, we've thrown out too many points,
       *             and must quit in shame
       */
      if (nb == 0) {
	 is_ok = 1;
	 break;
      }

      if (nr < AT_MATCH_REQUIRE) {
	 shError("iter_trans: only %d points remain, fewer than %d required",
		  nr, AT_MATCH_REQUIRE);
	 is_ok = 0;
	 break;
      }


      /*
       * calculate the TRANS for the remaining set of matches
       */
      if (calc_trans(nr, star_array_A, num_stars_A,
                         star_array_B, num_stars_B,
                         winner_index_A, winner_index_B,
                         trans) != SH_SUCCESS) {
	 shError("iter_trans: calc_trans returns with error");
	 return(SH_GENERIC_ERROR);
      }

   }

   /*
    * free up the arrays we allocated
    */
   shFree(a_prime);
   shFree(dist2);
   shFree(dist2_sorted);

   /* 
    * and decide whether we succeeded, or failed 
    */
   if (is_ok == 0) {
      return(SH_GENERIC_ERROR);
   }
   else {
      return(SH_SUCCESS);
   }
}



/************************************************************************
 * 
 *
 * ROUTINE: compare_float
 *
 * DESCRIPTION:
 * Given pointers to two floating point numbers, return the comparison.
 * Used by "iter_trans"
 *
 * RETURN:
 *    1                  if first float is larger than second
 *    0                  if the two are equal
 *   -1                  if first float is smaller than second
 *
 * </AUTO>
 */

static int
compare_float
   (
   float *f1,                 /* I: compare size of FIRST float value */
   float *f2                  /* I:  ... with SECOND float value  */
   )
{
   shAssert((f1 != NULL) && (f2 != NULL));

   if (*f1 > *f2) {
      return(1);
   }
   if (*f1 < *f2) {
      return(-1);
   }
   return(0);
}


/************************************************************************
 * 
 *
 * ROUTINE: find_percentile
 *
 * DESCRIPTION:
 * Given an array of 'num' floating point values, which have been
 * sorted, find the value corresponding to the value which is at
 * the 'perc'th percentile in the list array.  Return this value.
 *
 * RETURN:
 *   float               value of the number at 'perc'th percentile in array
 *
 * </AUTO>
 */

static float
find_percentile
   (
   float *array,           /* I: look in this SORTED array */
   int num,                /* I: which has this many elements */
   float perc              /* I: for entry at this percentile */
   )
{
   int index;

   shAssert(array != NULL);
   shAssert(num > 0);
   shAssert((perc > 0.0) && (perc <= 1.0));
   
   index = (int) floor(num*perc + 0.5);
   if (index >= num) {
      index = num - 1;
   }
   return(array[index]);
}
   

/************************************************************************
 * 
 *
 * ROUTINE: apply_trans
 *
 * DESCRIPTION:
 * Given an array of 'num_stars' s_star structures, apply the
 * given TRANS structure to the coordinates of each one.
 * Recall 
 *
 *       x' = A + Bx + Cx
 *       y' = D + Ex + Fy
 *
 * Higher-order distortion and color terms are ignored.
 *
 * RETURN:
 *   SH_SUCCESS             if all goes well
 *   SH_GENERIC_ERROR       if some problem occurs
 *
 * </AUTO>
 */

static int
apply_trans
   (
   s_star *star_array,     /* I/O: array of structures to modify */
   int num_stars,          /* I: number of stars in the array */
   TRANS *trans            /* I: contains coefficients of transformation */
   )
{
   int i;
   float newx, newy;
   s_star *sp;
   
   if (num_stars == 0) {
      return(SH_SUCCESS);
   }
   shAssert(star_array != NULL);

   for (i = 0; i < num_stars; i++) {
      sp = &(star_array[i]);
      newx = trans->a + trans->b*sp->x + trans->c*sp->y;
      newy = trans->d + trans->e*sp->x + trans->f*sp->y;
      sp->x = newx;
      sp->y = newy;
   }

   return(SH_SUCCESS);
}
   
/************************************************************************
 * 
 *
 * ROUTINE: array_to_chain
 *
 * DESCRIPTION:
 * (almost) the inverse of "chain_to_array", this function
 * takes values for "x" and "y" from an array of 'num_stars' "s_star" 
 * structures and places them into the items on the given CHAIN.
 * The "x" value is placed in the element whose name is "xname",
 * and "y" value is placed in the element whose name is "yname".
 *
 * We assume that the items on the CHAIN match the items in the 
 * array one-for-one.  That is, 
 *
 *       star_array[0]       is same as 0'th item on CHAIN
 *       star_array[1]       is same as 1'st item on CHAIN
 *
 * and so on.
 *
 *
 * RETURN:
 *    SH_SUCCESS         if all goes well
 *    SH_GENERIC_ERROR   if error occursurs
 *
 * </AUTO>
 */

static int
array_to_chain
   (
   s_star *star_array,   /* I: take "x" and "y" values from these objects */
   int num_stars,        /* I: number of stars in the array */   
   CHAIN *chain,         /* I: place values into items on this CHAIN */
   char *xname,          /* I: name of the field in schema with X coord */
   char *yname           /* I: name of the field in schema with Y coord */
   )
{
   int i, num;
   int found_xname, found_yname;
   int xtype, ytype;
   s_star *star;
   TYPE chain_type;
   char *schema_name;
   const SCHEMA *schema_ptr;
   SCHEMA_ELEM elem, xname_elem, yname_elem;
   void *item_ptr;

   /*
    * verify that the number of star in the array matches the number
    * of items on the CHAIN.
    */
   num = shChainSize(chain);
   if (num != num_stars) {
      shError("apply_trans: number of stars %d != number items on CHAIN %d",
	       num_stars, num);
      return(SH_GENERIC_ERROR);
   }

   /* first, get the type of the CHAIN as a string */
   chain_type = shChainTypeGet(chain);
   schema_name = shNameGetFromType(chain_type);
   shAssert(strcmp(schema_name, "UNKNOWN") != 0);

   /* next, we get a pointer to the SCHEMA describing the items on the CHAIN */
   schema_ptr = shSchemaGet(schema_name);

   /* verify that this SCHEMA has fields with the given names */
   found_xname = 0;
   found_yname = 0;

   for (i = 0; i < schema_ptr->nelem; i++) {
      elem = schema_ptr->elems[i];
      if (strcmp(elem.name, xname) == 0) {
	 found_xname++;
	 xname_elem = elem;
      }
      if (strcmp(elem.name, yname) == 0) {
	 found_yname++;
	 yname_elem = elem;
      }
   }
   if ((found_xname != 1) || (found_yname != 1)) {
      shError("array_to_chain: found_xname %d found_yname %d \n",
	       found_xname, found_yname);
      return(SH_GENERIC_ERROR);
   }

   /*
    * check to see that the "x", "y" fields are either
    * 
    *         float, double, int
    *
    * return with error if not.
    */
   if ((xtype = get_elem_type(xname_elem.type)) == MY_INVALID) {
      shError("chain_to_array: x field has invalid type");
   }
   if ((ytype = get_elem_type(yname_elem.type)) == MY_INVALID) {
      shError("chain_to_array: y field has invalid type");
   }
   if ((xtype == MY_INVALID) || (ytype == MY_INVALID)) {
      return(SH_GENERIC_ERROR);
   }

   /*
    * okay, now we can walk down the CHAIN and modify the "x" and "y"
    * values in each item.
    */
   for (i = 0; i < num; i++) {
      item_ptr = (void *) shChainElementGetByPos(chain, i);
      star = &(star_array[i]);

      if (set_elem_value(item_ptr, &xname_elem, xtype, star->x) != SH_SUCCESS) {
	 shError("apply_trans: shElemGet fails on X at element %d", i);
	 return(SH_GENERIC_ERROR);
      }
      if (set_elem_value(item_ptr, &yname_elem, ytype, star->y) != SH_SUCCESS) {
	 shError("apply_trans: shElemGet fails on Y at element %d", i);
	 return(SH_GENERIC_ERROR);
      }
   }

   return(SH_SUCCESS);
}










/***************************************************************************
 * 
 *
 * ROUTINE: double_sort_by_match_id
 *
 * DESCRIPTION:
 * sort all the elements of the first array of "s_star" in increasing
 * order by "match_id" value.  Also, reorder the
 * elements of the _second_ array in exactly the same way, so that
 * the elements of both array which matched BEFORE the sorting
 * will match again _after_ the sorting.
 *
 * return: 
 *   SH_SUCCESS                 if all goes well
 *   SH_GENERIC_ERROR           if not
 *
 * </AUTO>
 */

static int 
double_sort_by_match_id
   (
   s_star *star_array_A,        /* I/O: array to be sorted */
   int num_stars_A,             /* I: number of stars in array A */
   s_star *star_array_B,        /* I/O: array to be re-ordered just as A */
   int num_stars_B              /* I: number of stars in array B */
   )
{
   int i;
   struct s_star *temp_array;
   struct s_star *sb, *stemp;

   shAssert(num_stars_A == num_stars_B);
   if (num_stars_A == 0) {
      return(SH_SUCCESS);
   }
   shAssert(star_array_A != NULL);
   shAssert(star_array_B != NULL);
   
   /* 
    * first, let's set the "index" field of each element of each
    * star_array its position in the array.
    */
   for (i = 0; i < num_stars_A; i++) {
      star_array_A[i].index = i;
      star_array_B[i].index = i;
   }

   /*
    * next, we create a temporary array of the same size as A and B.
    */
   temp_array = (s_star *) shMalloc(num_stars_A*sizeof(s_star));


   /*
    * Now, the two arrays A and B are currently arranged so that
    * star_array_A[i] matches star_array_B[i].  We want to sort
    * star_array_A, and re-arrange star_array_B so that the
    * corresponding elements still match up afterwards.
    *
    *    - sort star_array_A
    *    - loop i through sorted star_array_A
    *           copy star_array_B element matching star_array_A[i] 
    *                                                 into temp_array[i]
    *    - loop i through star_array_B
    *           copy temp_array[i] into star_array_B[i]
    *
    *    - delete temp_array
    *
    * We end up with star_array_A sorted by "x", and star_array_B
    * re-arranged in exactly the same order.
    */

   sort_star_by_match_id(star_array_A, num_stars_A);
   for (i = 0; i < num_stars_A; i++) {
      sb = &(star_array_B[star_array_A[i].index]);
      shAssert(sb != NULL);
      stemp = &(temp_array[i]);
      shAssert(stemp != NULL);
      copy_star(sb, stemp);
   }

   /* 
    * now copy the elements of the temp_array back into star_array_B 
    */
   for (i = 0; i < num_stars_A; i++) {
      sb = &(star_array_B[i]);
      shAssert(sb != NULL);
      stemp = &(temp_array[i]);
      shAssert(stemp != NULL);
      copy_star(stemp, sb);
   }

   /*
    * and we're done!  Delete the temporary array 
    */
   free_star_array(temp_array);

   return(SH_SUCCESS);
}






/***************************************************************************
 * 
 *
 * ROUTINE: match_arrays_slow
 *
 * DESCRIPTION:
 * given two arrays of s_stars [A and B], find all matching elements,
 * where a match is coincidence of centers to within "radius" pixels.
 *
 * Use a slow, but sure, algorithm (and an inefficient implementation,
 * I'm sure.  As of 1/18/96, trying for correctness, not speed).
 *
 * We will match objects from A --> B.  It is possible to have several
 * As that match to the same B:
 *
 *           A1 -> B5   and A2 -> B5
 *
 * This function finds such multiple-match items and deletes all but
 * the closest of the matches.
 *
 * This array creates 4 new arrays of s_stars, and returns a pointer
 * to each array, as well as the number of stars in each array.
 * 
 * place the elems of A that are matches into output array J
 *                    B that are matches into output array K
 *                    A that are not matches into output array L
 *                    B that are not matches into output array M
 *
 * return: SH_SUCCESS          if all goes well
 *         SH_GENERIC_ERROR    if not
 *
 * </AUTO>
 */


static int
match_arrays_slow
   (
   s_star *star_array_A,    /* I: first array of s_stars to be matched */
   int num_stars_A,         /* I: number of stars in A */
   s_star *star_array_B,    /* I: second array of s_stars to be matched */
   int num_stars_B,         /* I: number of stars in B */
   float radius,            /* I: matching radius */
   s_star **star_array_J,   /* O: all stars in A which match put in here */
   int *num_stars_J,        /* O: number of stars in output array J */
   s_star **star_array_K,   /* O: all stars in B which match put in here */
   int *num_stars_K,        /* O: number of stars in output array K */
   s_star **star_array_L,   /* O: all stars in A which don't match put here */
   int *num_stars_L,        /* O: number of stars in output array L */
   s_star **star_array_M,   /* O: all stars in B which don't match put here */
   int *num_stars_M         /* O: number of stars in output array M */
   )
{
   float Ax, Ay, Bx, By;
   float dist, limit;
   int posA, posB;
   int current_num_J, current_num_K;
   float deltax, deltay;
   float Axm, Axp, Aym, Ayp;
   s_star *sa, *sb;

#ifdef DEBUG
   printf("entering match_arrays_slow ");
#endif

   /* 
    * first, we create each of the 4 output arrays.  We start with
    * each as big as the input arrays, but we'll shrink them down
    * to their proper sizes before we return.
    */
   *star_array_J = (s_star *) shMalloc(num_stars_A*sizeof(s_star));
   *num_stars_J = num_stars_A;
   *star_array_K = (s_star *) shMalloc(num_stars_B*sizeof(s_star));
   *num_stars_K = num_stars_B;
   *star_array_L = (s_star *) shMalloc(num_stars_A*sizeof(s_star));
   *num_stars_L = num_stars_A;
   *star_array_M = (s_star *) shMalloc(num_stars_B*sizeof(s_star));
   *num_stars_M = num_stars_B;

   /*
    * make some sanity checks 
    */
   shAssert(num_stars_A >= 0);
   shAssert(num_stars_B >= 0);
   if ((num_stars_A == 0) || (num_stars_B == 0)) {
      return(SH_SUCCESS);
   }
   shAssert(star_array_A != NULL);
   shAssert(star_array_B != NULL);

   /*
    * First, we sort arrays A and B by their "x" coordinates,
    * to facilitate matching.
    */
   sort_star_by_x(star_array_A, num_stars_A);
   sort_star_by_x(star_array_B, num_stars_B);

   /* 
    * We copy array A into L, and array B into M.
    * We will remove all non-matching elements from these
    * output arrays later on in this function.
    */

   copy_star_array(star_array_A, *star_array_L, num_stars_A);
   copy_star_array(star_array_B, *star_array_M, num_stars_B);


   /* 
    * this is the largest distance that two stars can be from
    * each other and still be a match.
    */
   limit = radius*radius;


   /*
    * the first step is to go slowly through array A, checking against
    * every object in array B.  If there's a match, we copy the matching
    * elements onto lists J and K, respectively.  We do NOT check
    * yet to see if there are multiply-matched elements.
    *
    * This implementation could be speeded up a LOT by sorting the
    * two arrays in "x" and then making use of the information to check
    * only stars which are close to each other in "x".  Do that
    * some time later.... MWR 1/18/96.
    */
#ifdef DEBUG
   printf(" size of array A is %d, array B is %d\n", num_stars_A, num_stars_B);
   printf(" about to step through array A looking for matches\n");
#endif

   current_num_J = 0;
   current_num_K = 0;

   for (posA = 0; posA < num_stars_A; posA++) {
      
      shAssert((sa = &(star_array_A[posA])) != NULL);
      Ax = sa->x;
      Ay = sa->y;

      Axm = Ax - radius;
      Axp = Ax + radius;
      Aym = Ay - radius;
      Ayp = Ay + radius;

      for (posB = 0; posB < num_stars_B; posB++) {

         shAssert((sb = &(star_array_B[posB])) != NULL);
         Bx = sb->x;
         By = sb->y;

	 /* check quickly to see if we can avoid a multiply */
	 if ((Bx < Axm) || (Bx > Axp) || (By < Aym) || (By > Ayp)) {
	    continue;
	 }

	 /* okay, we actually have to calculate a distance here. */
	 deltax = Ax - Bx;
	 deltay = Ay - By;
	 dist = deltax*deltax + deltay*deltay;
	 if (dist < limit) {

	    /*
	     * we have a match (at least, a possible match).  So, copy
	     * objA onto listJ and objB onto listK.  But do NOT remove
	     * these objects from listA and listB!  We may end up
	     * matching another objA to the same objB later on, and
	     * we will continue trying to match this same objA to other
	     * objBs.
	     */
	    add_element(sa, star_array_J, num_stars_J, &current_num_J);
	    add_element(sb, star_array_K, num_stars_K, &current_num_K);

	 }
      }
   }

   /* 
    * at this point, let's re-set "*num_stars_J" to the proper number.
    * Recall that the "add_element" function may increase "*num_stars_J"
    * by factors of 2, while the variable "current_num_J" keeps track
    * of the actual number of stars in the array.  It ought to be the
    * case that
    *              num_stars_J <= *num_stars_J
    *
    * and likewise for K.
    */
   *num_stars_J = current_num_J;
   *num_stars_K = current_num_K;

#ifdef DEBUG
   printf(" done with stepping through array A \n");
   printf(" array J has %d, array K has %d \n", current_num_J, current_num_K);
#endif
   
#ifdef DEBUG
   /* for debugging only */
   for (posA = 0; posA < *num_stars_J; posA++) {
      sa = &((*star_array_J)[posA]);
      sb = &((*star_array_K)[posA]);
      printf(" %4d  J: %4d (%8.2f, %8.2f)  K: %4d (%8.2f, %8.2f) \n",
	    posA, sa->match_id, sa->x, sa->y, sb->match_id, sb->x, sb->y);
   }
#endif


   /*
    * at this point, all _possible_ matches have been placed into 
    * corresponding elements of arrays J and K.  Now, we go through
    * array J to find elements which appear more than once.  We'll
    * resolve them by throwing out all but the closest match.
    */

   /* 
    * first, sort array J by the "match_id" values.  This allows us to find 
    * repeated elements easily.  Re-order array K in exactly the same
    * way, so matching elements still match. 
    */
#ifdef DEBUG
   printf(" sorting array J by match_id\n");
#endif
   if (double_sort_by_match_id(*star_array_J, *num_stars_J, 
                               *star_array_K, *num_stars_K) != SH_SUCCESS) {
       shError("match_arrays_slow: can't sort array J");
       return(SH_GENERIC_ERROR);
   }
#ifdef DEBUG
   for (posA = 0; posA < *num_stars_J; posA++) {
      sa = &((*star_array_J)[posA]);
      sb = &((*star_array_K)[posA]);
      printf(" %4d  J: %4d (%8.2f, %8.2f)  K: %4d (%8.2f, %8.2f) \n",
	    posA, sa->match_id, sa->x, sa->y, sb->match_id, sb->x, sb->y);
   }
#endif

   /* 
    * now remove repeated elements from array J, keeping the closest matches 
    */
#ifdef DEBUG
   printf(" before remove_repeated_elements, array J has %d\n", *num_stars_J);
#endif
   if (remove_repeated_elements(*star_array_J, num_stars_J, 
                                *star_array_K, num_stars_K) != SH_SUCCESS) {
       shError("match_arrays_slow: remove_repeated_elements fails for array J");
       return(SH_GENERIC_ERROR);
   }
#ifdef DEBUG
   printf(" after remove_repeated_elements, array J has %d\n", *num_stars_J);
   for (posA = 0; posA < *num_stars_J; posA++) {
      sa = &((*star_array_J)[posA]);
      sb = &((*star_array_K)[posA]);
      printf(" %4d  J: %4d (%8.2f, %8.2f)  K: %4d (%8.2f, %8.2f) \n",
	    posA, sa->match_id, sa->x, sa->y, sb->match_id, sb->x, sb->y);
   }
#endif
   shAssert(*num_stars_J == *num_stars_K);

   /* 
    * next, do the same for array K: sort it by "match_id" 
    * (and re-arrange array J to match),
    * then find and remove any
    * repeated elements, keeping only the closest matches.
    */
#ifdef DEBUG
   printf(" sorting array K by match_id\n");
#endif
   if (double_sort_by_match_id(*star_array_K, *num_stars_K, 
                               *star_array_J, *num_stars_J) != SH_SUCCESS) {
       shError("match_arrays_slow: can't sort array K");
       return(SH_GENERIC_ERROR);
   }
#ifdef DEBUG
   for (posA = 0; posA < *num_stars_J; posA++) {
      sa = &((*star_array_J)[posA]);
      sb = &((*star_array_K)[posA]);
      printf(" %4d  J: %4d (%8.2f, %8.2f)  K: %4d (%8.2f, %8.2f) \n",
	    posA, sa->match_id, sa->x, sa->y, sb->match_id, sb->x, sb->y);
   }
#endif


#ifdef DEBUG
   printf(" before remove_repeated_elements, array K has %d\n", *num_stars_K);
   for (posA = 0; posA < *num_stars_J; posA++) {
      sa = &((*star_array_J)[posA]);
      sb = &((*star_array_K)[posA]);
      printf(" %4d  J: %4d (%8.2f, %8.2f)  K: %4d (%8.2f, %8.2f) \n",
	    posA, sa->match_id, sa->x, sa->y, sb->match_id, sb->x, sb->y);
   }
#endif
   if (remove_repeated_elements(*star_array_K, num_stars_K, 
                                *star_array_J, num_stars_J) != SH_SUCCESS) {
       shError("match_arrays_slow: remove_repeated_elements fails for array K");
       return(SH_GENERIC_ERROR);
   }
#ifdef DEBUG
   printf(" after remove_repeated_elements, arrary K has %d\n", *num_stars_K);
   for (posA = 0; posA < *num_stars_J; posA++) {
      sa = &((*star_array_J)[posA]);
      sb = &((*star_array_K)[posA]);
      printf(" %4d  J: %4d (%8.2f, %8.2f)  K: %4d (%8.2f, %8.2f) \n",
	    posA, sa->match_id, sa->x, sa->y, sb->match_id, sb->x, sb->y);
   }
#endif
   shAssert(*num_stars_J == *num_stars_K);

   /*
    * finally, we have unique set of closest-pair matching elements
    * in arrays J and K.  Now we can remove any element from array L
    * which appears in array J, and remove any element from array M
    * which appears in array K.  First, we'll sort arrays L and M
    * to make the comparisons easier.
    */
#ifdef DEBUG
   printf(" sorting array L \n");
#endif
   sort_star_by_match_id(*star_array_L, *num_stars_L);
#ifdef DEBUG
   printf(" sorting array M \n");
#endif
   sort_star_by_match_id(*star_array_M, *num_stars_M);

   /* 
    * Recall that array K is already sorted by "match_id", but that
    * we may have thrown J out of order when we forced it to follow
    * the sorting of K.  So, first we'll sort J by "match_id",
    * (and re-order K match it), then we can remove repeated elements
    * from L easily.
    */
#ifdef DEBUG
   printf(" sorting array J by match_id\n");
#endif
   if (double_sort_by_match_id(*star_array_J, *num_stars_J, 
                               *star_array_K, *num_stars_K) != SH_SUCCESS) {
       shError("match_arrays_slow: can't sort array J");
       return(SH_GENERIC_ERROR);
   }
   /* 
    * now remove elements from array L which appear in array J
    */
#ifdef DEBUG
   printf(" before remove_same_elements, array L has %d\n", *num_stars_L);
   for (posA = 0; posA < *num_stars_L; posA++) {
      sa = &((*star_array_L)[posA]);
      printf(" %4d  L: %4d (%8.2f, %8.2f)  \n",
	    posA, sa->match_id, sa->x, sa->y);
   }
#endif
   remove_same_elements(*star_array_J, *num_stars_J,
                        *star_array_L, num_stars_L);
#ifdef DEBUG
   printf(" after remove_same_elements, array L has %d\n", *num_stars_L);
   for (posA = 0; posA < *num_stars_L; posA++) {
      sa = &((*star_array_L)[posA]);
      printf(" %4d  L: %4d (%8.2f, %8.2f)  \n",
	    posA, sa->match_id, sa->x, sa->y);
   }
#endif


   /* 
    * Recall that we threw K out of order when we forced it to follow
    * the sorting of J.  So, we'll sort K by "match_id",
    * (and re-order J match it), then we can remove repeated elements
    * from M easily.
    */
#ifdef DEBUG
   printf(" sorting array K by match_id\n");
#endif
   if (double_sort_by_match_id(*star_array_K, *num_stars_K, 
                               *star_array_J, *num_stars_J) != SH_SUCCESS) {
       shError("match_arrays_slow: can't sort array K");
       return(SH_GENERIC_ERROR);
   }
   /* 
    * and remove elements from array M which appear in array K
    */
#ifdef DEBUG
   printf(" before remove_same_elements, array M has %d\n", *num_stars_M);
   for (posA = 0; posA < *num_stars_M; posA++) {
      sb = &((*star_array_M)[posA]);
      printf(" %4d  M: %4d (%8.2f, %8.2f) \n",
	    posA, sb->match_id, sb->x, sb->y);
   }
#endif
   remove_same_elements(*star_array_K, *num_stars_K, 
                        *star_array_M, num_stars_M);
#ifdef DEBUG
   printf(" after remove_same_elements, array M has %d\n", *num_stars_M);
   for (posA = 0; posA < *num_stars_M; posA++) {
      sb = &((*star_array_M)[posA]);
      printf(" %4d  M: %4d (%8.2f, %8.2f) \n",
	    posA, sb->match_id, sb->x, sb->y);
   }
#endif


   return(SH_SUCCESS);
}



/**************************************************************************
 * 
 *
 * ROUTINE: add_element
 *
 * DESCRIPTION:
 * We are given a pointer to s_star, an array of "total_num" s_stars,
 * and a count of the current number of s_stars set in the array.
 *
 * We want to copy the contents of the single star into
 * the "current_num"'th element of the array.  
 *
 *   If current_num < total_num,    just perform copy,
 *                                  increment current_num
 *
 *   If current_num == total_num,   we must allocate more space in array
 *                                  allocate an array 2x as big as total_num
 *                                  copy existing elements into new array
 *                                  copy new element into new array
 *                                  free old array
 *                                  make old array pointer point to new array
 *                                  increment current_num
 *
 * We could avoid all this by using linked lists, but I think
 * that we will only rarely have to increase the size of an array,
 * and never increase its size more than once.  So this isn't so bad.
 *
 * RETURN: SH_SUCCESS          if all goes well
 *         SH_GENERIC_ERROR    if not
 *
 */

static int
add_element
   (
   s_star *new_star,      /* I: want to copy this into next slot in array */
   s_star **star_array,   /* I/O: will copy into this array */
                          /*       if necessary, will allocate a new array, */
                          /*       copy entire contents into it, including */
                          /*       new_star, then free the old array */
   int *total_num,        /* I/O: total number of stars allocated in */
                          /*       star_array.  We may increase this if */
                          /*       we have to extend star_array */
   int *current_num       /* I/O: current number of stars in star_array */
                          /*       which have been set.  This number should */
                          /*       always increase by 1 if we succeed in */
                          /*       in adding the "new_star" */
   )
{
   int num;
   s_star *new_array;
   int fail;               /* this used to prevent compiler warning with shAssert */
   
   shAssert(new_star != NULL);
   shAssert((star_array != NULL) && (*star_array != NULL));
   shAssert((total_num != NULL) && (*total_num >= 0));
   shAssert((current_num != NULL) && (*current_num >= 0));


   /*
    * check for the easy case: if current_num < total_num, we can
    * just set star_array[current_num] and increment current_num.
    */
   if (*current_num < *total_num) {
      copy_star(new_star, &((*star_array)[*current_num]));
      (*current_num)++;
   }
   else if (*current_num == *total_num) {

      /* 
       * this is the tricky case, in which we have to allocate space
       * for a larger array, copy all existing elements, and then
       * copy over the new_star.
       */
      num = (*total_num)*2;
      new_array = (s_star *) shMalloc(num*sizeof(s_star));
      copy_star_array((*star_array), new_array, (*total_num));
      free_star_array(*star_array);
      *star_array = new_array;
      *total_num = num;
      copy_star(new_star, &((*star_array)[*current_num]));
      (*current_num)++;
   } 
   else {

      /*
       * this should never occur!
       */
      fail = 1;
      shAssert(fail==0);
   }

   return(SH_SUCCESS);
}





/*********************************************************************
 *
 * ROUTINE: remove_repeated_elements
 *
 * DESCRIPTION:
 * step through the first array argument, star_array_1, checking for 
 * successive elements which are the same. for each such pair, calculate the 
 * distance between the matching elements of objects in arrays 1 and 2.
 * Throw the less-close pair out of the two array, modifying the number
 * of elements in each accordingly (and moving all other elements
 * up one place in the array).
 *
 * The two arrays must have the same number of elements, 
 * and array 1 must already have been sorted by the "match_id" field.
 *
 * RETURN:
 *    SH_SUCCESS              if all goes well
 *    SH_GENERIC_ERROR        if something goes wrong
 *
 */

static int
remove_repeated_elements
   (
   s_star *star_array_1,  /* I/O: look in this array for repeats */
   int *num_stars_1,      /* I/O: number of stars in array 1 */
   s_star *star_array_2,  /* I/O: do to this array what we do to array 1 */
   int *num_stars_2       /* I/O: number of stars in array 2 */
   )
{
   int pos1, pos2;
   float thisdist, lastdist;
   s_star *s1, *s2;
   s_star *last1, *last2;

   shAssert(star_array_1 != NULL);
   shAssert(star_array_2 != NULL);
   shAssert(*num_stars_1 == *num_stars_2);

   pos1 = 0;
   pos2 = 0;

   last1 = NULL;
   last2 = NULL;
   while (pos1 < *num_stars_1) {

      s1 = &(star_array_1[pos1]);
      s2 = &(star_array_2[pos2]);
      if ((s1 == NULL) || (s2 == NULL)) {
         shError("remove_repeated_elements: missing elem in array 1 or 2");
         return(SH_GENERIC_ERROR);
      }

      if (last1 == NULL) {
	 last1 = s1;
	 last2 = s2;
      }
      else if (s1->match_id == last1->match_id) {

	 /* there is a repeated element.  We must find the closer match */
	 thisdist = (s1->x - s2->x)*(s1->x - s2->x) +
	            (s1->y - s2->y)*(s1->y - s2->y);
	 lastdist = (last1->x - last2->x)*(last1->x - last2->x) +
	            (last1->y - last2->y)*(last1->y - last2->y);

	 if (thisdist < lastdist) {
	      
	    /* 
	     * remove the "last" item from arrays 1 and 2.
	     * We move the "current" items up one position in the arrays,
	     * (into spaces [pos1 - 1] and [pos2 - 1]), and make
	     * them the new "last" items.
	     */
	    remove_elem(star_array_1, pos1 - 1, num_stars_1);
	    remove_elem(star_array_2, pos2 - 1, num_stars_2);
	    last1 = &(star_array_1[pos1 - 1]);
	    last2 = &(star_array_2[pos2 - 1]);
	  }
	  else {

	    /*
	     * remove the current item from arrays 1 and 2.
	     * We can leave the "last" items as they are, since
	     * we haven't moved them.
	     */
	    remove_elem(star_array_1, pos1, num_stars_1);
	    remove_elem(star_array_2, pos2, num_stars_2);
	 }
	 pos1--;
	 pos2--;
      }
      else {

         /* no repeated element.  Prepare for next step forward */
	 last1 = s1;
	 last2 = s2;
      }
      pos1++;
      pos2++;
   }
   return(SH_SUCCESS);
}


/*********************************************************************
 *
 * ROUTINE: remove_elem
 *
 * DESCRIPTION:
 * Remove the i'th element from the given array.  
 *
 * What we do (slow as it is) is 
 *
 *       1. move all elements after i up by 1
 *       2. subtract 1 from the number of elements in the array
 *
 * There's probably a better way of doing this, but let's
 * go with it for now.  1/19/96  MWR
 *
 * RETURN:
 *    nothing
 *
 */

static void
remove_elem
   (
   s_star *star_array,    /* I/O: we remove one element from this array */
   int num,               /* I: remove _this_ element */
   int *num_stars         /* I/O: on input: number of stars in array */
                          /*      on output: ditto, now smaller by one */
   )
{
   int i;
   s_star *s1, *s2;

   shAssert(star_array != NULL);
   shAssert(num < *num_stars);
   shAssert(num >= 0);
   shAssert(*num_stars > 0);

   s1 = &(star_array[num]);
   s2 = &(star_array[num + 1]);
   for (i = num; i < *num_stars; i++, s1++, s2++) {
      copy_star(s2, s1);
   }

   (*num_stars)--;
}
      

/*********************************************************************
 *
 * ROUTINE: remove_same_elements
 *
 * DESCRIPTION:
 * given two arrays of s_stars which have been sorted by their 
 * "match_id" values, try to find s_stars which appear 
 * in both arrays.  Remove any such s_stars from the second array.
 *
 * RETURN:
 *   nothing
 * 
 */

static void
remove_same_elements
   (
   s_star *star_array_1,  /* I: look for elems which match those in array 2 */
   int num_stars_1,       /* I: number of elems in array 1 */
   s_star *star_array_2,  /* I/O: remove elems which match those in array 1 */
   int *num_stars_2       /* I/O: number of elems in array 2 */
                          /*         will probably be smaller on output */
   )
{
   int pos1, pos2, pos2_top;
   s_star *s1, *s2;

   shAssert(star_array_1 != NULL);
   shAssert(star_array_2 != NULL);
   shAssert(num_stars_2 != NULL);

   pos1 = 0;
   pos2_top = 0;

   while (pos1 < num_stars_1) {

      s1 = &(star_array_1[pos1]);
      shAssert(s1 != NULL);

      for (pos2 = pos2_top; pos2 < *num_stars_2; pos2++) {
	 s2 = &(star_array_2[pos2]);
	 shAssert(s2 != NULL);

	 if (s1->match_id == s2->match_id) {
	    remove_elem(star_array_2, pos2, num_stars_2);
	    if (--pos2_top < 0) {
	       pos2_top = 0;
	    }
	 }
	 else {
	    if (s2->match_id < s1->match_id) {
	       pos2_top = pos2 + 1;
	    }
	 }
      }
      pos1++;
   }
}
	 

   
/*********************************************************************
 *
 * ROUTINE: copy_chain_from_array
 *
 * DESCRIPTION:
 * Given a CHAIN of some type, and an array of s_stars, 
 * we want to create and return a new CHAIN, identical to the
 * first, except that it should have only those elements for which
 *
 *     position in CHAIN  =  "match_id" of an s_star in array
 *
 * That is, suppose we have a CHAIN of 10 items.
 *
 *     CHAIN         0   1   2   3   4   5   6   7   8   9
 *
 * and we have an array of 3 s_stars with match_id values
 *
 *     s_star        0   1   2   3
 *      match_id     4   9   1   6
 *
 * What we want is to create a new CHAIN of items, such that
 *
 *   new CHAIN item  0       =   old CHAIN item 4
 *   new CHAIN item  1       =   old CHAIN item 9
 *   new CHAIN item  2       =   old CHAIN item 1
 *   new CHAIN item  3       =   old CHAIN item 6
 *   
 * Create this new CHAIN, and return a pointer to it.
 * Note that the new CHAIN should have the same number of elements
 * as the input array of s_stars.
 *
 * Note that the items on the new CHAIN are the _same_ items
 * as those on the old; they are not copies!  So, if you delete
 * items on the old chain, the ones on the new CHAIN will disappear, too!
 * 
 * RETURN:
 *   CHAIN *                  to new CHAIN, if all goes well
 *   NULL                     if something goes wrong
 * 
 */

static CHAIN *
copy_chain_from_array
   (
   CHAIN *old_chain,      /* I: start with a copy of this CHAIN */
   s_star *star_array,    /* I: pick items with "match_id" from these stars */
   int num_stars          /* I: number of stars in "star_array" */
   )
{
   int i;
   int old_size, new_size;
   int match_id;
   CHAIN *new_chain;
   s_star *star;
   TYPE chain_type;
   char *schema_name;
   void *item;

   shAssert(old_chain != NULL);
   shAssert(star_array != NULL);
   old_size = shChainSize(old_chain);
   shAssert(num_stars <= old_size);


   /* 
    * first, get the type of the CHAIN as a string 
    */
   chain_type = shChainTypeGet(old_chain);
   schema_name = shNameGetFromType(chain_type);
   shAssert(strcmp(schema_name, "UNKNOWN") != 0);

   /* 
    * create a new CHAIN of the same type 
    */
   new_chain = shChainNew(schema_name);
   new_size = shChainSize(new_chain);

   /*
    * now walk through the array, picking the "match_id" from each s_star.
    * then copy the "match_id"'th item in the old chain into the new chain.
    */
   for (i = 0; i < num_stars; i++) {
      star = &(star_array[i]);
      match_id = star->match_id;
      shAssert((match_id >= 0) && (match_id < old_size));
      item = (void *) shChainElementGetByPos(old_chain, match_id);
      if (i == 0) {
         if (shChainElementAddByPos(new_chain, item, schema_name, 
		  i, AFTER) != SH_SUCCESS) {
	    shError("shChainElementAddByPos returns with error");
	    shError("g_shChainErrno is %d\n", g_shChainErrno);
         }
      }
      else {
         if (shChainElementAddByPos(new_chain, item, schema_name, 
		  i - 1, AFTER) != SH_SUCCESS) {
	    shError("shChainElementAddByPos returns with error");
	    shError("g_shChainErrno is %d\n", g_shChainErrno);
         }
      }
      new_size = shChainSize(new_chain);
   }

   /*
    * sanity check 
    */
   new_size = shChainSize(new_chain);
   shAssert(new_size == num_stars);

   return(new_chain);
}
