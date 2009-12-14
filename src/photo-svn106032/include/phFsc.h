#if !defined(PH_FSC_H)
#define PH_FSC_H 1
/*
 * This is support code for photo's FakeStampCollection pipeline
 */
#define SC_INT16VAL     32768		/* subtract this from SCSTAMP pixel
					   values to get SCFSTAMP 'pixel'
					   values */
#define SC_STAMPNROW     65		/* number of rows in a postage stamp */
#define SC_STAMPNCOL     65		/* number of cols in a postage stamp */
#define SC_NPIXEL      4225		/* ==  SC_STAMPNROW*SC_STAMPNCOL */

#if SC_NPIXEL !=  SC_STAMPNROW*SC_STAMPNCOL
#  error Incorrect definition of SC_NPIXEL
#endif

/*
 * Two definitions of stamps for Fang files; SCSTAMPs are used in FSC, while
 * SCFSTAMPs are written to FANG files.  Note that these SCSTAMPs have extra
 * fields not present in the SSC
 */
typedef struct SCSTAMP {
   TYPE type;
   int midRow;				/* centre */
   int midCol;				/*     of stamp */
   float rowc;				/* position of */
   float colc;				/*    peak in parent region */
   float peak;				/* peak intensity */
   int status;
   REGION *reg;
} SCSTAMP;				/* pragma SCHEMA */

typedef struct SCFSTAMP {
   short pixelMap[SC_NPIXEL];
   short midRow;
   short midCol;
} SCFSTAMP;				/* pragma SCHEMA */

SCSTAMP *phFscStampNew(void);
void phFscStampDel(SCSTAMP *stamp);
void phFscStampChainDel(CHAIN *chain);

SCFSTAMP *phFscFstampNew(void);
void phFscFstampDel(SCFSTAMP *fstamp);
void phFscFstampChainDel(CHAIN *chain);

SCFSTAMP *
phFscStampToFstamp(const SCSTAMP *stamp);	/* SCSTAMP to be converted */
CHAIN *
phFscStampChainToFstampChain(const CHAIN *stampchain); /* SCSTAMPs to convert*/

#endif
