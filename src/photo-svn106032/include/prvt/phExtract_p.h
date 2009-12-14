#if !defined(EXTRACT_P_H)
#define EXTRACT_P_H

/*
 * NSIMPLE is the number of elements below which we should do a simple search
 * NINSERT is the number of elements below which it is ALWAYS faster to
 * do an insertion sort than a Shellsort. For very small dynamic ranges,
 * it is still faster to do a histsort for more than ~6 objects; we
 * probably don't care.
 */
#define NSIMPLE 5
#define NINSERT 25 

/*
 * We have to choose between sorting using Shellsort and building a
 * histogram of pixel values, and then processing it (`histsort'). Let
 * the buffer be of size B, and the dynamic range R
 *
 * Shellsort is about 5.5 times slower than histsort for very small R and
 * B==256. The time scales as ~ B**1.25, and let us approximate this as B**1
 *
 * Experiments reveal that histsort takes a time proportional to (B + R*3/8);
 * the coefficient 3/8 was determined for B == 256. We should thus use a Shell
 * sort rather than histsort if
 *   5.5*B < (B + R*3/8)
 *
 * Resulting in the very fast criterion given below as SHELLWINS (we write
 * the factor 1/5.5 as 46/256 == 1/5.56)
 */
#define SHELLWINS(B,R) ((unsigned)(B)*256 < ((B) + (unsigned)(R)*3/8)*46)
 
/*
 * threshold number for main body histogram processing--we may want to
 * make this very large, or only invoke it for sky, or something.
 *
 * MBTHRESH is ignored if -ve
 */
#define MBTHRESH 2000
#if MBTHRESH < 0
#  define PROFILE_NSIGMA -1		/* NOTUSED */
#  define PROFILE_NTILE -1
#  define IQR_TO_SIGMA_NSIGMA_CLIPPED -1
#else
#  define PROFILE_INSIGMA 400000	/* At how many sigma should we clip
					   the profile histograms? */
#  define PROFILE_NSIGMA (1e-5*PROFILE_INSIGMA)
/*
 * Set quantile corresponding to PROFILE_NSIGMA
 */
#  if PROFILE_INSIGMA ==   232634
#     define PROFILE_NTILE 0.01
#     define IQR_TO_SIGMA_NSIGMA_CLIPPED (1.02376*IQR_TO_SIGMA)
#  elif PROFILE_INSIGMA == 400000
#     define PROFILE_NTILE 3.165e-5
#     define IQR_TO_SIGMA_NSIGMA_CLIPPED (1.0*IQR_TO_SIGMA)
#  else
#     error Non-supported value of PROFILE_NSIGMA
#  endif
#endif

#endif
