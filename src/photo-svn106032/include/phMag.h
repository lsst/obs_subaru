#if !defined(PHMAG_H)
#define PHMAG_H
#include "phFilter.h"

/* This struct will hold the magnitude in one color for an object */
typedef struct {
  char passBand[FILTER_MAXNAME];
  float  mag;
  float  magErr;
} MAG;

MAG* phMagNew(void);
void phMagDel(MAG *mag);

#endif
