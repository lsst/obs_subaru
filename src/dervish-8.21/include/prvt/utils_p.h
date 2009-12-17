#ifndef UTILS_P_H
#define UTILS_P_H

/*
 * structures and prototypes private to Dervish
 *
 */
#if defined(REGION_H)
/*
 * Stuff to do with CRCs, checksums, etc.
 */
int p_shGetRegionCheck(REGION *region);
int p_shGetMaskCheck(MASK *mask);

/*
 * Physical region support
 */
#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

typedef struct physReg_p PHYS_CONFIG;
PHYS_CONFIG *p_shPhysReserve(void);
void p_shPhysUnreserve(PHYS_CONFIG *a_physPtr);
void p_shRegPixFreeFromPhys(REGION *a_reg);
RET_CODE p_shRegPixNewFromPhys(REGION *a_reg, int a_nrow, int a_ncol);
RET_CODE p_shRegHdrCopyFromPhys(REGION *a_reg, int *a_nrow, int *a_ncol);
RET_CODE p_shRegReadFromPool(REGION *a_reg, char *a_frame, int a_read);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif /* #if defined(REGION_H) */

#endif
