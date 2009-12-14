#if !defined(PHPHOTPAR_H)
#define PHPHOTPAR_H
#include "dervish.h"

/* This struct holds a set of values used for photometric calibration 
   of data in one color only */

typedef struct {
  float magZero;
  float extinction;
  float magZeroError;
  float extinctionError;
} PHOTPAR;

PHOTPAR * phPhotparNew(void);
void phPhotparDel(PHOTPAR *photpar);

#endif
