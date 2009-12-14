#if !defined(PHSTARC_H)
#define PHSTARC_H
#include "phMag.h"

/* This struct holds the information from one star in a catalog */

typedef struct {
  int id;
  double ra;
  double raErr;
  double dec;
  double decErr;
  double mjd;		/* This must be some average of times in diff bands */
  int ncolors;
  MAG **mag;
} STARC; /* pragma SCHEMA */

STARC * phStarcNew(int ncolors);
void phStarcDel(STARC *star1);

#endif
