#if !defined(PHSTAR1STARCMERGE_H)
#define PHSTAR1STARCMERGE_H
#include "phStar1.h"
#include "phStarc.h"

/* This struct stores the matched objects for two sets of stars: one of
	type STAR1PC, and one of type STARC		       	*/

typedef struct  {
  STAR1PC *star1;              /* postage stamp info */
  STARC *starc;                /* PT information */
  int starcMagPos;	       /* magnitude array element that matches star1 */
  int starcRefMagPos;	       /* magnitude array element for refcolor */
  double sep;		       /* separation between merge in arcsec */
  float airmass;	       /* airmass at observation of star1 */
  double mjd;		       /* time of observation of star1 */
  float flux20;		       /* Flux in a 20th magnitude star */
			       /* These 2 could be put in star1 if needed */
			       /* in generel */
} STAR1STARCMERGE; /* pragma SCHEMA */

STAR1STARCMERGE *phStar1starcmergeNew(void);
void phStar1starcmergeDel(STAR1STARCMERGE *star1starcMerge);

#endif
