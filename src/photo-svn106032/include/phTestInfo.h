#if !defined(PHTESTINFO_H)
#define PHTESTINFO_H
/*
 * extra parameters for testing photo; these are embedded in OBJC/OBJC_IOs
 *
 * If you change these values you'll have to edit TEST_INFO as requested
 */
#define TEST_2D_PROFILES 0
#define TEST_PETRO_RATIO 0
#define TEST_ASTROM_BIAS 1
#define NPEAK 10
#define NANN1 NANN+1			/* part of IRIX_OFFSETOF_BUG
					   workaround in make_io */
/*
 * make_io isn't clever enough to handle #if ... #endif
 */
#if 1 && TEST_PETRO_RATIO
#error Please include these elements in TEST_INFO
   float nPetroRatio[NCOLOR];		/* num. of values of Petrosian ratio */
   float petroRatio[NCOLOR][NANN1];	/* Petrosian ratio */
   float petroRatioErr[NCOLOR][NANN1];	/* Petrosian ratio error */

   float petroCounts[NCOLOR];		/* counts within n*R_P; n != 2*/
#endif

#if TEST_2D_PROFILES
#define ENCELL 169			/* should equal NCELL */
#if ENCELL != NCELL
#error Please fix ENCELL
#endif
#if 1
#error Please include these elements in TEST_INFO
   int nprof2D[NCOLOR];			/* number of cells */
   float profMean2D[NCOLOR][ENCELL];	/* 2-d profile */
   float profErr2D[NCOLOR][ENCELL];	/* 2-d profile errors */
   float profdeV2D[NCOLOR][ENCELL];	/* 2-d profile of deV fit*/
#endif
#endif

#if TEST_ASTROM_BIAS
#  if 0
#     error Please include these elements in TEST_INFO
   float row_bias[NCOLOR];
   float col_bias[NCOLOR];
#  endif
#elif 1
#  error Please remove these elements from TEST_INFO
#endif

typedef struct test_info {
   int id;				/* id number for this objc */
   const int ncolor;			/* number of colours */
   int true_type;			/* true type from some catalogue */
   int true_id;				/* ID from some catalogue */
   int obj1_id[NCOLOR];
   int objc_npeak;			/* number of peaks in OBJC */
   int npeak[NCOLOR];			/* number of peaks in each band */
   float objc_peak[NPEAK];		/* the first */
   float objc_peak_col[NPEAK];		/*           NPEAK peaks */
   float objc_peak_row[NPEAK];		/*                      in OBJC */
   float peak[NCOLOR][NPEAK];		/* the first */
   float peak_col[NCOLOR][NPEAK];	/*           NPEAK peaks */
   float peak_row[NCOLOR][NPEAK];	/*                      in each band */

   float psf_exp_coeffs[NCOLOR][4];	/* coeffs. of PSF expansion */

   int nu_star[NCOLOR];			/* number of d.o.f. for stellar fit */
   float chisq_star[NCOLOR];		/* chi^2 for stellar fit */
   int nu_deV[NCOLOR];			/* number of d.o.f. for deV fit */
   float chisq_deV[NCOLOR];		/* chi^2 for deV fit */
   int nu_exp[NCOLOR];			/* number of d.o.f. for disk fit */
   float chisq_exp[NCOLOR];		/* chi^2 for disk fit */

   float deV_ap_corr[NCOLOR];		/* aperture corrections for deV fits */
   float exp_ap_corr[NCOLOR];		/* aperture corrections for exp fits */

   float row_bias[NCOLOR];
   float col_bias[NCOLOR];
} TEST_INFO;				/* pragma SCHEMA */

TEST_INFO *phTestInfoNew(int ncolor);
void phTestInfoSetFromObjc(const OBJC *objc, TEST_INFO *tst);
void phTestInfoDel(TEST_INFO *test, int deep);

#endif
