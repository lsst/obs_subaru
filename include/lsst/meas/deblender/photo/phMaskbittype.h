#if !defined(MASKBITTYPE_H)
#define MASKBITTYPE_H

   /* This enumerated type lists the values for each bit in a REGION's
      defect/object MASK.  The region may also have another MASK which
      is used to hold the estimated noise in each pixel */

typedef enum {
   MASK_INTERP = 0x1,      /* if set, pixel's value has been interpolated */
   MASK_SATUR  = 0x2,      /* if set, pixel is/was saturated  */
   MASK_NOTCHECKED = 0x4,  /* if set, pixel was NOT examined for an object */
   MASK_OBJECT = 0x8,      /* if set, pixel is part of some object */
   MASK_BRIGHTOBJECT = 0x10,   /* if set, pixel is part of bright object */
   MASK_CATOBJECT = 0x20,  /* if set, pixel is part of a catalogued object */
   MASK_SUBTRACTED = 0x40, /* if set, model has been subtracted from pixel */
   MASK_GHOST = 0x80       /* if set, pixel is part of a ghost */
} MASKBITTYPE;

   /* this macro tells whether the pixel corresponding to mask byte 'x'
      may be used in calculations: if 1, is ok.  If 0, not ok. */
#define IS_MASKOK(x) (!(x & MASK_NOTCHECKED))

#endif

