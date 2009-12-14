#if !defined(PHJPGUTILS1_H)
#define PHJPGUTILS1_H
#include "phObjc.h"

CHAIN *phReadCatalog(char *filename,float offsetx,float offsety);
RET_CODE phObjcListMatchSlow(CHAIN *listA,CHAIN *listB,float radius,
			 CHAIN **listJ,CHAIN **listK,CHAIN **listL,
                         CHAIN **listM);

CHAIN *phObjcChainMSortC0Xc(CHAIN *list, int color);
CHAIN *phObjcDoubleChainMSortC0Xc(CHAIN *list1, CHAIN *list2, int color);
RET_CODE phAfCorCalc(ARRAY *afx,ARRAY *afy,ARRAY *afMask,
		     float *mean_x,float *mean_y,
		     float *sqrtVar_x,float *sqrtVar_y,
		     float *Covar_xy);
RET_CODE phAfRegress(ARRAY *afx,ARRAY *afy,ARRAY *afMask,
		     float *a,float *b,float *r,float *s,int a_fix,int b_fix);

OBJC *phObjcFromValueOfArrays
(CHAIN *list,ARRAY *afx,ARRAY *afy,ARRAY *afMask,float vx,float vy);

RET_CODE phAfClip(ARRAY *af,
		  char *op,
		  float value);

#endif

