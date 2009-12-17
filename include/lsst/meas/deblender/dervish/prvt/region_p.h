#ifndef _REGION_P_H
#define _REGION_P_H

typedef enum {
   SHREGINVALID,		/* Invalid or improperly initialized region */
   SHREGVIRTUAL,		/* Region in virtual memory */
   SHREGPHYSICAL		/* Region in physical memory */
  } shRegionType;


struct physReg_p {
   void		*physRegPtr;	    /* Pointer to owner region */
   int		physIndex;	    /* Physical region index */
   char		*physHdrPtr;	    /* Pointer to physical region header */
   char		*(*physPixAlloc)(int, int);	/* Routine called to get 
						   physical region pixel ptr */
   void		(*physPixFree)(int, char *);	/* Routine called when done with
				                   physical region pixels */
   char		*(*physHdrAlloc)(int, int);	/* Routine called to get 
						   physical region header ptr */
   void		(*physHdrFree)(int, char *);	/* Routine called when done with
				      		   physical region header */
   int		(*physFillCall)(int, char *, char *); /* Routine called to fill
                                                          physical region
				                          with a frame */
};

struct region_p {
   shRegionType	 type;		/* Region type */
   unsigned int ModCntr;	/* Counter incremented each time region is
				   modified or re-loaded */
   int		flags;
   int		crc;
   REGION	*parent;	/* parent image = NULL if this isn't a child */
   int row0, col0;		/* origin of region in parent. Note that this
				   is not the same as region->{col,row}0 */
   int		nchild;
   struct tnode *children;	/* tree giving the children, (struct tnode *)0 if none */
   void		*pixels;	/* pointer to the allocated space */
   struct physReg_p *phys;	/* pointer to physical region specifics */
};

struct mask_p {
   int	flags;				/* information about the mask */
   int	crc;				/* Cyclic Redundancy Check */
   int	nchild;				/* number of sub masks associated
					   with this mask */
   struct tnode *children;              /* sub masks of this mask */
   struct mask *parent;			/* NULL if this isn't a child */
   int row0, col0;		/* origin of region in parent. Note that this
				   is not the same as mask->{col,row}0 */
};

/*****************************************************************************/
/*
 * Prototypes for private functions; these would be static if they
 * could be confined to a single file. 
 */
#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

void **p_shRegPtrGet(const REGION *reg, int *size);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif
