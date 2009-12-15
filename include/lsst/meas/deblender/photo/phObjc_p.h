#if !defined(PHOBJC_P_H)
#define PHOBJC_P_H
/*
 * Handle AI_PIXs.  N.b. These are only exposed for the sake of dataIo.c
 */
typedef struct AI_PIX {
    OBJMASK *mask;                      /* shape and values of atlas image (in master_mask's coordinate system) */
    float val;                          /* value of virtual pixels (not PIX; keep full precision) */
} AI_PIX;

AI_PIX *phAiPixNew(const OBJMASK *mask, int allocate_data, PIX val);
void phAiPixFree(AI_PIX *aip);

PIX *phAtlasImagePixelExpand(const ATLAS_IMAGE *ai, int c, int in_place);
void phAtlasImagePixelCompress(ATLAS_IMAGE *ai, int c);

#endif
