#ifndef ATSMOOTHREG_H
#define ATSMOOTHREG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dervish.h"

RET_CODE atRegSmoothWithMedian (
				REGION *inRegion, 
				REGION *outRegion, 
				int nSize
				);

#ifdef __cplusplus
}
#endif

#endif
