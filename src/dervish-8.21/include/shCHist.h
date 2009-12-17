#ifndef SHCHISTS_H
#define SHCHISTS_H
/* This defines the structures used by hists.c */

typedef struct {
  unsigned int base;        /* lowest histogrammed value */
  unsigned int nbins;       /* number of bins, neglecting the overflow
						 and underflow bins */
  unsigned int binwid;       /* width of each bin */ 
  unsigned int underflow;   /* Accumulates underflow */
  unsigned int *bins;       /* Accumulates histogrammed values */
  unsigned int overflow;   /* Accumulates overflow */
  unsigned int naccum;      /* number of things accumulated in the histogram */
} HIST;

#ifdef __cplusplus
extern "C"
{

#endif  /* ifdef cpluplus */
unsigned int shHistFindPeak(HIST *hists);
HIST *shHistConstructHists(unsigned int nhists, unsigned int sbin, 
	unsigned int nbins, unsigned int binwid);
void shHistInitHists (HIST *hists, unsigned int nhists);
void shHistDestructHists(HIST *hists , unsigned int nhists);
unsigned int **shHistConstructNtiles(int numPercentiles, unsigned int nhists);
void     shHistDestructNtiles(unsigned int **ntiles , unsigned int nhists);
void shHistArrayAccumulateLine(HIST *hists, unsigned int *line, 
	unsigned int nele);
void shHistAccumulateLine(HIST *hists, unsigned int *line, unsigned int nele);
void shHistDoNtiles(HIST *hists, int nhists, unsigned int **ntiles, 
	int *ntilesWantedPercentiles);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif
