#if !defined(PHCCDPARS_H)
#define PHCCDPARS_H
#include "shChain.h"

/*
 *   Structure Definition for CCD Defect Database
 *
 *   Written by: Marc Postman
 *   Creation date:  6 Dec 1993
 *   Last Modified: 18 Feb 1994  by Michael Richmond
 *
 *   CCD parameters and defect properties are stored
 *   in an ascii file. This file is read in during the
 *   correctFrames initialization and stored as an
 *   array of linked lists. The structure definition
 *   for the linked list is given here.
 *
 */

/*
 *   CCD Defect Types
 */
typedef enum { 
   DRKCUR = 0,				/* low level dark current */
   HOTCOL,				/* Hot column - bad dark current */
   DEPCOL,				/* Depressed column */
   CTECOL,				/* Bad charge transfer efficiency */
   BLKCOL,				/* blocked column */
   BADBLK,				/* badly blocked col */
   HOTSPOT,				/* a single hot pixel */
   NOISY,				/* a noisy column */
   TRAP					/* location of trap head */
} DFTYPE;

typedef enum { 
  BADCOL =0,                 /* Interpolate over column */
  ADDCOL,                    /* Additive correction - 
					 fix with debias vector */
  FILCOL                     /* Fill in column */
} DFACTION;

typedef struct {
  int iRow;                  /* camRow */
  int iCol;                  /* camCol */
  int dfcol0;                /* starting column of this defect */
  int dfncol;                /* number of columns in this defect */
  DFTYPE dftype;             /* defect type */
  DFACTION dfaction;	     /* repair action for defect */
} CCDDEFECT;


/* The new CCDPARS struct is taken from the scIC file as of 12/20/95 */
/* scIc file no longer used. This struct should be cleaned up
for now, add namps and change l/r to 0/1 */

#define N_LINEAR_COEFFS 4		/* Max. number of terms used by
					   linearity corrections
					   pragma typedef {
					   pragma } N_LINEAR_COEFFS */

/*
 * Types of linearity correction.
 *   R == raw DN (bias subtracted)
 *   C == corrected value
 */
typedef enum {
   LINEAR_ILLEGAL = 666,		/* illegal type; this amp isn't used */
   LINEAR_NONE = 1,			/* C/R = 1 */
   LINEAR_QUADRATIC,			/* C/R = 1 + c1 R */
   LINEAR_LOG,				/* C/R = 1 + c1 lg(R) */
   LINEAR_THRESHLOG,			/* C/R = 1 + (R<c2 ? 0 : c1 lg(R/c2))*/
   LINEAR_NTYPE				/* number of functional forms */
} LINEARITY_TYPE;

typedef struct {
    int iRow;		   /*  camRow */
    int iCol;		   /*  camCol */
    int filter;	           /*  filter id (stoughton specific) */
    int namps;		   /*  number of amps being used (1 or 2) */
    int rowRef;	           /*  Middle Pixel Of Data Rows In CCD */
    int colRef;	           /*  Middle Pixel Of Data Cols In CCD */
    double xPos;	   /*  mm offset from center of camera */
    double yPos;	   /*  mm offset from center of camera */
    double rotation;	   /*  rotation of ccd in camera (degrees) */
    int nrows;             /*  number of rows in image on disk (inc bias etc)*/
    int ncols;             /*  number of cols in image on disk */
    int nDataCol;	   /*  Data Columns For This CCD */
    int nDataRow;	   /*  Data Rows For This CCD */
    int binfac;		   /* how much chip is binned */
    int amp0;		   /*  which amp used for left side (low) pixels */
    int amp1;		   /*  which amp used for right side (high) pixels */
    int sPrescan0;        /*  starting col for left Prescan */
    int nPrescan0;	   /*  number of Prescan rows for left amp */
    int sPrescan1;        /*  starting col for right Prescan */
    int nPrescan1;	   /*  number for right amp */
    int sPostscan0;        /*  starting col for left Postscan */
    int nPostscan0;	   /*  number of Postscan rows for left amp */
    int sPostscan1;        /*  starting col for right Postscan */
    int nPostscan1;	   /*  number for right amp */
    int sPreBias0;            /*  starting col for left PreBias */
    int nPreBias0;	           /*  number of PreBias rows for left amp */
    int sPreBias1;            /*  starting col for right PreBias */
    int nPreBias1;	           /*  number for right amp */
    int sPostBias0;            /*  starting col for left PostBias */
    int nPostBias0;	           /*  number of PostBias rows for left amp */
    int sPostBias1;            /*  starting col for right PostBias */
    int nPostBias1;	           /*  number for right amp */
    int sData0;            /*  starting col for left data sec */
    int nData0;	           /*  number of data rows for left data sec */
    int sData1;            /*  starting col for right data sec */
    int nData1;	           /*  number for right data sec */
    double biasLevel0;	   /*  Bias Level (DN) For Left Amp */
    double biasLevel1;	   /*  Bias Level (DN) For Right Amp */
    double readNoise0;	   /*  Read noise (sigma; DN) for left amp */
    double readNoise1;	   /*  Read noise (sigma; DN) for right amp */
    double gain0;	   /*  Gain (e/DN) for left amp */
    double gain1;	   /*  Gain for right amp */
    double fullWell0; 	   /*  full well level for left side (DN) */
    double fullWell1;	   /*  full well level for right side (DN) */
    double xOff;	   /*  offset in pixels from nominal xPos */
    double yOff;	   /*  offset in pixels from nominal yPos */
    double rowOffset;	   /*  rows offset for astrom chips to align frames */
    int frameOffset;       /*  Number of frames this chip is offset from r' */

    int rowBinning;	   /* how much is data binned by row? */
    int colBinning;	   /* how much is data binned by column?*/
    int ccdIdent;	   /*  ccd serial number */ 
    CHAIN *CCDDefect;        /* Pointer to linked list of defects
                             as defined in struct CCDDEFECT   */
    LINEARITY_TYPE linearity_type;	/* type of linearity function */
    int n_linear_coeffs0;		/* number of coefficients (N); left */
    float linear_coeffs0[N_LINEAR_COEFFS]; /* coefficients C of expansion:
					   DN_out = sum_i=0^N-1 DN_in^i C_i */
    int n_linear_coeffs1;		/* number of coefficients (N); right */
    float linear_coeffs1[N_LINEAR_COEFFS]; /* coefficients C of expansion:
					   DN_out = sum_i=0^N-1 DN_in^i C_i */
} CCDPARS;


CCDDEFECT * phCcddefectNew(void);
void phCcddefectDel(CCDDEFECT *ccddefect);

CCDPARS * phCcdparsNew(void);
void phCcdparsDel(CCDPARS *ccdpars);

CCDPARS * phCcdparsFetchById(int row, int col);

#endif























































