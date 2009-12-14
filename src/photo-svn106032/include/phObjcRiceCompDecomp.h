#if !defined(PHOBJCRICECOMPDECOMP_H)
#define PHOBJCRICECOMPDECOMP_H

#define ORCD_DEBUG 2
#define NODEBUG 0
#define NOMASK 0
#define DEBUG 1


RET_CODE
phObjcRiceCompress(CHAIN *objclist, int mask, int debug);
RET_CODE
phObjcRiceDecompress(CHAIN *objclist, int debug);

#endif
