#if !defined(PHFRAMEINFO_H)
#define PHFRAMEINFO_H

/* This struct will hold information about each frame */
typedef struct {
  int field;
  float airmass;   		
  double mjd;			/* observation time, MJD */

 } FRAMEINFO;

FRAMEINFO * phFrameinfoNew(void);
void phFrameinfoDel(FRAMEINFO *frameinfo);

#endif



