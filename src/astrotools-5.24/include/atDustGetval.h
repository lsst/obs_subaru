#ifndef ATDUSTGETVAL_H
#define ATDUSTGETVAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <dervish.h>
RET_CODE atExtinction(
		      double l,		/* IN:  Galactic longitude (degrees) */
		      double b,		/* IN:  Galactic latitude (degrees) */
		      float *u,		/* OUT: extinction in u band */
		      float *g,		/* OUT: extinction in g band */
		      float *r,		/* OUT: extinction in r band */
		      float *i,		/* OUT: extinction in i band */
		      float *z		/* OUT: extinction in z band */
		      );

RET_CODE atDustGetval(
		      double gall, /* galactic longitude in degrees */
		      double galb, /* galactic latitude in degrees */
		      char *pMapName,	/* Ebv, I100, X, T, mask */
		      int interpolate, /* 1=linear interp over 4 pixels */
		      int verbose, /* 1=verbose on */
		      double *value /* the return value */
		      );

RET_CODE atVDustGetval(
		       VECTOR *gall, /* galactic longitude in degrees */
		       VECTOR *galb, /* galactic latitude in degrees */
		       char *pMapName,	/* Ebv, I100, X, T, mask */
		       int interpolate, /* 1=linear interp over 4 pixels */
		       int verbose, /* 1=verbose on */
		       VECTOR *value /* the return value */
		       );

void lambert_getval(
		    int maptoUse,
		    char  *  pFileN,
		    char  *  pFileS,
		    long     nGal,
		    float *  pGall,
		    float *  pGalb,
		    int      qInterp,
		    int      qVerbose,
		    float *  pOutput   );

RET_CODE atDustGetvalFast(
			  double gall, /* galactic longitude in degrees */
			  double galb, /* galactic latitude in degrees */
			  char *pMapName,	/* Ebv, I100, X, T, mask */
			  int interpolate, /* 1=linear interp over 4 pixels */
			  int verbose, /* 1=verbose on */
			  double *value /* the return value */
			  );

RET_CODE atVDustGetvalFast(
		       VECTOR *gall, /* galactic longitude in degrees */
		       VECTOR *galb, /* galactic latitude in degrees */
		       char *pMapName,	/* Ebv, I100, X, T, mask */
		       int interpolate, /* 1=linear interp over 4 pixels */
		       int verbose, /* 1=verbose on */
		       VECTOR *value /* the return value */
		       );

void lambert_getvalFast(
			int maptoUse,
			char  *  pFileN,
			char  *  pFileS,
			long     nGal,
			float *  pGall,
			float *  pGalb,
			int      qInterp,
			int      qVerbose,
			float *  pOutput 
			);

RET_CODE atDustGetvalFastClean(
			  char *pMapName	/* Ebv, I100, X, T, mask */
		      );

RET_CODE atDustMaskParse(
			 double maskval,   /*value returned from dust mask */
			 char *maskinfo    /*the return info */
		      );

#define MAX_FILE_LINE_LEN 500 /* Maximum line length for data files */
#define MAX_FILE_NAME_LEN  80
#define IO_FOPEN_MAX       20  /* Files must be numbered 0 to IO_FOPEN_MAX-1 */
#define IO_FORTRAN_FL      80  /* Max length of file name from a Fortran call */
#define IO_GOOD             1
#define IO_BAD              0

extern FILE  *  pFILEfits[];

int inoutput_file_exist
  (char  *  pFileName);
int inoutput_free_file_pointer_(void);
int inoutput_open_file
  (int   *  pFilenum,
   char     pFileName[],
   char     pPriv[]);
int inoutput_close_file
  (int      filenum);

int getline
  (char  *  pLine,
   int      max);
float sexagesimal_to_float
  (char  *  pTimeString);
void trim_string
  (char  *  pName,
   int      nChar);
char * get_next_line
  (char  *  pNextLine,
   int      maxLen,
   FILE  *  pFILE);
void string_to_integer_list
  (char  *  pString,
   int   *  pNEntry,
   int   ** ppEntry);

typedef unsigned char uchar;
typedef long int HSIZE;
typedef long int DSIZE;

void mask_val_print_
  (float  pGall,
   float  pGalb,
   float  maskval);
void fits_read_file_fits_header_only_
  (char     pFileName[],
   HSIZE *  pNHead,
   uchar ** ppHead);
int fits_read_file_ascii_header_
  (char     pFileName[],
   HSIZE *  pNHead,
   uchar ** ppHead);
DSIZE fits_read_file_fits_r4_
  (char     pFileName[],
   HSIZE *  pNHead,
   uchar ** ppHead,
   DSIZE *  pNData,
   float ** ppData);
DSIZE fits_read_file_fits_i2_
  (char     pFileName[],
   HSIZE *  pNHead,
   uchar ** ppHead,
   DSIZE *  pNData,
   short int ** ppData);
DSIZE fits_read_subimg_
  (char     pFileName[],
   HSIZE    nHead,
   uchar *  pHead,
   DSIZE *  pStart,
   DSIZE *  pEnd,
   DSIZE *  pNVal,
   float ** ppVal);
void fits_read_subimg1
  (int      nel,
   DSIZE *  pNaxis,
   DSIZE *  pStart,
   DSIZE *  pEnd,
   int      fileNum,
   int      bitpix,
   DSIZE *  pNVal,
   uchar *  pData);
DSIZE fits_read_point_
  (char     pFileName[],
   HSIZE    nHead,
   uchar *  pHead,
   DSIZE *  pLoc,
   float *  pValue);
DSIZE fits_read_file_fits_noscale_
  (char     pFileName[],
   DSIZE *  pNHead,
   uchar ** ppHead,
   DSIZE *  pNData,
   int   *  pBitpix,
   uchar ** ppData);
DSIZE fits_write_file_fits_r4_
  (char     pFileName[],
   HSIZE *  pNHead,
   uchar ** ppHead,
   DSIZE *  pNData,
   float ** ppData);
DSIZE fits_write_file_fits_i2_
  (char     pFileName[],
   HSIZE *  pNHead,
   uchar ** ppHead,
   DSIZE *  pNData,
   short int ** ppData);
DSIZE fits_write_file_fits_noscale_
  (char     pFileName[],
   HSIZE *  pNHead,
   uchar ** ppHead,
   DSIZE *  pNData,
   int   *  pBitpix,
   uchar ** ppData);
DSIZE fits_read_fits_data_
  (int   *  pFilenum,
   int   *  pBitpix,
   DSIZE *  pNData,
   uchar ** ppData);
DSIZE fits_write_fits_data_
  (int   *  pFilenum,
   int   *  pBitpix,
   DSIZE *  pNData,
   uchar ** ppData);
void fits_read_fits_header_
  (int   *  pFilenum,
   HSIZE *  pNHead,
   uchar ** ppHead);
void fits_skip_header_
  (int   *  pFilenum);
void fits_add_required_cards_
  (HSIZE *  pNHead,
   uchar ** ppHead);
void fits_write_fits_header_
  (int   *  pFilenum,
   HSIZE *  pNHead,
   uchar ** ppHead);
void fits_create_fits_header_
  (HSIZE *  pNHead,
   uchar ** ppHead);
void fits_duplicate_fits_header_
  (HSIZE *  pNHead,
   uchar ** ppHead,
   uchar ** ppHeadCopy);
void fits_duplicate_fits_data_r4_
  (DSIZE *  pNData,
   float ** ppData,
   float ** ppDataCopy);
void fits_duplicate_fits_data_
  (int   *  pBitpix,
   DSIZE *  pNData,
   uchar ** ppData,
   uchar ** ppDataCopy);
void fits_create_fits_data_r4_
  (DSIZE *  pNData,
   float ** ppData);
void fits_create_fits_data_
  (int   *  pBitpix,
   DSIZE *  pNData,
   uchar ** ppData);
int fits_dispose_header_and_data_
  (uchar ** ppHead,
   uchar ** ppData);
int fits_dispose_array_
  (uchar ** ppHeadOrData);
DSIZE fits_compute_ndata_
  (HSIZE *  pNHead,
   uchar ** ppHead);
void fits_compute_axes_
  (HSIZE *  pNHead,
   uchar ** ppHead,
   int   *  pNumAxes,
   DSIZE ** ppNaxis);
float compute_vista_wavelength_
  (DSIZE *  pPixelNumber,
   int   *  pNCoeff,
   float ** ppCoeff);
void fits_compute_vista_poly_coeffs_
  (HSIZE *  pNHead,
   uchar ** ppHead,
   int   *  pNCoeff,
   float ** ppCoeff);
void fits_data_to_r4_
  (HSIZE *  pNHead,
   uchar ** ppHead,
   DSIZE *  pNData,
   uchar ** ppData);
void fits_data_to_i2_
  (HSIZE *  pNHead,
   uchar ** ppHead,
   DSIZE *  pNData,
   uchar ** ppData);
HSIZE fits_add_card_
  (uchar    pCard[],
   HSIZE *  pNHead,
   uchar ** ppHead);
HSIZE fits_add_cardblank_
  (uchar    pCard[],
   HSIZE *  pNHead,
   uchar ** ppHead);
HSIZE fits_add_card_ival_
  (int   *  pIval,
   uchar    pLabel[],
   HSIZE *  pNHead,
   uchar ** ppHead);
HSIZE fits_add_card_rval_
  (float *  pRval,
   uchar    pLabel[],
   HSIZE *  pNHead,
   uchar ** ppHead);
HSIZE fits_add_card_string_
  (char  *  pStringVal,
   uchar    pLabel[],
   HSIZE *  pNHead,
   uchar ** ppHead);
HSIZE fits_add_card_comment_
  (char  *  pStringVal,
   HSIZE *  pNHead,
   uchar ** ppHead);
HSIZE fits_add_card_history_
  (char  *  pStringVal,
   HSIZE *  pNHead,
   uchar ** ppHead);
HSIZE fits_purge_blank_cards_
  (HSIZE *  pNHead,
   uchar ** ppHead);
HSIZE fits_delete_card_
  (uchar    pLabel[],
   HSIZE *  pNHead,
   uchar ** ppHead);
HSIZE fits_find_card_
  (uchar    pLabel[],
   HSIZE *  pNHead,
   uchar ** ppHead);
void fits_swap_cards_ival_
  (uchar *  pLabel1,
   uchar *  pLabel2,
   HSIZE *  pNHead,
   uchar ** ppHead);
void fits_swap_cards_rval_
  (uchar *  pLabel1,
   uchar *  pLabel2,
   HSIZE *  pNHead,
   uchar ** ppHead);
int fits_get_card_ival_
  (int   *  pIval,
   uchar    pLabel[],
   HSIZE *  pNHead,
   uchar ** ppHead);
int fits_get_card_rval_
  (float *  pRval,
   uchar    pLabel[],
   HSIZE *  pNHead,
   uchar ** ppHead);
int fits_get_card_date_
  (int   *  pMonth,
   int   *  pDate,
   int   *  pYear,
   uchar    pLabelDate[],
   HSIZE *  pNHead,
   uchar ** ppHead);
int fits_get_card_time_
  (float *  pTime,
   uchar    pLabelTime[],
   HSIZE *  pNHead,
   uchar ** ppHead);
int fits_get_card_string_
  (char  ** ppStringVal,
   uchar    pLabel[],
   HSIZE *  pNHead,
   uchar ** ppHead);
HSIZE fits_change_card_
  (uchar    pCard[],
   HSIZE *  pNHead,
   uchar ** ppHead);
HSIZE fits_change_card_ival_
  (int   *  pIval,
   uchar    pLabel[],
   HSIZE *  pNHead,
   uchar ** ppHead);
HSIZE fits_change_card_rval_
  (float *  pRval,
   uchar    pLabel[],
   HSIZE *  pNHead,
   uchar ** ppHhead);
HSIZE fits_change_card_string_
  (char  *  pStringVal,
   uchar    pLabel[],
   HSIZE *  pNHead,
   uchar ** ppHead);
void fits_string_to_card_
  (uchar    pString[],
   uchar    pCard[]);
float fits_get_rval_
  (DSIZE *  pIloc,
   int   *  pBitpix,
   float *  pBscale,
   float *  pBzero,
   uchar ** ppDdata);
int fits_get_ival_
  (DSIZE *  pIloc,
   int   *  pBitpix,
   float *  pBscale,
   float *  pBzero,
   uchar ** ppDdata);
void fits_put_rval_
  (float *  pRval,
   DSIZE *  pIloc,
   int   *  pBitpix,
   float *  pBscale,
   float *  pBzero,
   uchar ** ppData);
int fits_qblankval_
  (DSIZE *  pIloc,
   int   *  pBitpix,
   float *  pBlankval,
   uchar ** ppDdata);
void fits_put_blankval_
  (DSIZE *  pIloc,
   int   *  pBitpix,
   float *  pBlankval,
   uchar ** ppData);
void fits_purge_nulls
  (uchar    pCard[]);
int fits_get_next_card_
  (int   *  pFilenum,
   uchar    pCard[]);
int fits_put_next_card_
  (int   *  pFilenum,
   uchar    pCard[]);
int fits_size_from_bitpix_
  (int   *  pBitpix);
void fits_pixshift_wrap_
  (int   *  pSAxis,
   DSIZE *  pShift,
   HSIZE *  pNHead,
   uchar ** ppHead,
   uchar ** ppData);
void fits_transpose_data_
  (HSIZE *  pNHead,
   uchar ** ppHead,
   uchar ** ppData);
void fits_ave_rows_r4_
  (int   *  iq,
   DSIZE *  pRowStart,
   DSIZE *  pNumRowAve,
   DSIZE *  pNaxis1,
   DSIZE *  pNaxis2,
   float ** ppData,
   float ** ppOut);
void fits_ave_obj_and_sigma_rows_r4_
  (int   *  iq,
   DSIZE *  pRowStart,
   DSIZE *  pNumRowAve,
   DSIZE *  pNaxis1,
   DSIZE *  pNaxis2,
   float ** ppObjData,
   float ** ppSigData,
   float ** ppObjOut,
   float ** ppSigOut);
void fits_byteswap
  (int      bitpix,
   DSIZE    nData,
   uchar *  pData);
void fits_bswap2
  (uchar *  pc1,
   uchar *  pc2);

extern uchar * datum_zero;
extern uchar * label_airmass;
extern uchar * label_bitpix;
extern uchar * label_blank;
extern uchar * label_bscale;
extern uchar * label_bzero;
extern uchar * label_cdelt1;
extern uchar * label_cdelt2;
extern uchar * label_crpix1;
extern uchar * label_crpix2;
extern uchar * label_crval1;
extern uchar * label_crval2;
extern uchar * label_date_obs;
extern uchar * label_dec;
extern uchar * label_empty;
extern uchar * label_end;
extern uchar * label_exposure;
extern uchar * label_extend;
extern uchar * label_filtband;
extern uchar * label_filter;
extern uchar * label_ha;
extern uchar * label_instrume;
extern uchar * label_lamord;
extern uchar * label_loss;
extern uchar * label_naxis;
extern uchar * label_naxis1;
extern uchar * label_naxis2;
extern uchar * label_object;
extern uchar * label_observer;
extern uchar * label_pa;
extern uchar * label_platescl;
extern uchar * label_ra;
extern uchar * label_rnoise;
extern uchar * label_rota;
extern uchar * label_seeing;
extern uchar * label_skyrms;
extern uchar * label_skyval;
extern uchar * label_slitwidt;
extern uchar * label_st;
extern uchar * label_telescop;
extern uchar * label_time;
extern uchar * label_tub;
extern uchar * label_ut;
extern uchar * label_vhelio;
extern uchar * label_vminusi;
extern uchar * card_simple;
extern uchar * card_empty;
extern uchar * card_null;
extern uchar * card_end;
extern uchar * text_T;
extern uchar * text_F;

void lambert_lb2fpix
  (float    gall,   /* Galactic longitude */
   float    galb,   /* Galactic latitude */
   HSIZE    nHead,
   uchar *  pHead,
   float *  pX,     /* X position in pixels from the center */
   float *  pY);    /* Y position in pixels from the center */
void lambert_lb2pix
  (float    gall,   /* Galactic longitude */
   float    galb,   /* Galactic latitude */
   HSIZE    nHead,
   uchar *  pHead,
   int   *  pIX,    /* X position in pixels from the center */
   int   *  pIY);   /* Y position in pixels from the center */
void lambert_lb2xy
  (float    gall,   /* Galactic longitude */
   float    galb,   /* Galactic latitude */
   int      nsgp,   /* +1 for NGP projection, -1 for SGP */
   float    scale,  /* Radius of b=0 to b=90 degrees in pixels */
   float *  pX,     /* X position in pixels from the center */
   float *  pY);    /* Y position in pixels from the center */


extern uchar * label_lam_nsgp;
extern uchar * label_lam_scal;

typedef size_t MEMSZ;

#ifdef OLD_SUNCC
void memmove
  (void  *  s,
   const void  *  ct,
   MEMSZ    n);
#endif
void ccalloc_resize_
  (MEMSZ *  pOldMemSize,
   MEMSZ *  pNewMemSize,
   void  ** ppData);
void ccrealloc_
  (MEMSZ *  pMemSize,
   void  ** ppData);
void ccalloc_init
  (MEMSZ *  pMemSize,
   void  ** ppData);
void ccalloc_
  (MEMSZ *  pMemSize,
   void  ** ppData);
void ccfree_
  (void  ** ppData);
float * ccvector_build_
  (MEMSZ    n);
double * ccdvector_build_
  (MEMSZ    n);
int * ccivector_build_
  (MEMSZ    n);
float ** ccpvector_build_
  (MEMSZ    n);
float *** ccppvector_build_
  (MEMSZ    n);
float * ccvector_rebuild_
  (MEMSZ    n,
   float *  pOldVector);
double * ccdvector_rebuild_
  (MEMSZ    n,
   double *  pOldVector);
int * ccivector_rebuild_
  (MEMSZ    n,
   int   *  pOldVector);
float ** ccpvector_rebuild_
  (MEMSZ    n,
   float ** ppOldVector);
float *** ccppvector_rebuild_
  (MEMSZ    n,
   float *** pppOldVector);
void ccvector_free_
  (float *  pVector);
void ccdvector_free_
  (double *  pVector);
void ccivector_free_
  (int   *  pVector);
void ccpvector_free_
  (float ** ppVector);
void ccppvector_free_
  (float *** pppVector);
float ** ccarray_build_
  (MEMSZ    nRow,
   MEMSZ    nCol);
double ** ccdarray_build_
  (MEMSZ    nRow,
   MEMSZ    nCol);
int ** cciarray_build_
  (MEMSZ    nRow,
   MEMSZ    nCol);
float ** ccarray_rebuild_
  (MEMSZ    nRow,
   MEMSZ    nCol,
   float ** ppOldArray);
double ** ccdarray_rebuild_
  (MEMSZ    nRow,
   MEMSZ    nCol,
   double ** ppOldArray);
int ** cciarray_rebuild_
  (MEMSZ    nRow,
   MEMSZ    nCol,
   int   ** ppOldArray);
void ccarray_free_
  (float ** ppArray);
void ccdarray_free_
  (double ** ppArray);
void cciarray_free_
  (int   ** ppArray);
void ccarray_zero_
  (float ** ppArray,
   MEMSZ    nRow,
   MEMSZ    nCol);
void ccvector_zero_
  (float *  pVector,
   MEMSZ    n);
void ccdvector_zero_
  (double *  pVector,
   MEMSZ    n);
void ccivector_zero_
  (int    *  pVector,
   MEMSZ    n);

#ifdef __cplusplus
}
#endif

#endif


