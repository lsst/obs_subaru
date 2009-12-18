extern char *rindex(const char*, int);          /* no proto in irix */

/*
 * NB - IF THESE TABLES CHANGE; CODING MUST BE ADDED TO THE DECOMP PROGRAM
 * TO HANDLE OLD VERSIONS!!!!!
 *
 * Pixels are packed with a fixed number of Least Significant Bits followed
 * by a comma code that is calculated by the difference between the MSB portion
 * of the pixel and the mode.   The modal pixel and the number of LSB
 * are determined from the distribution of the pixels in the first block.
 * See find_lsb().
 *
 * The comma code is calculated by the difference value translated by the
 * following table.  Diffs with an absolute value greater than MAX_TAIL are
 * given code OUTLIE_CODE followed by the actual MSB.  The value of OUTLIE_CODE
 * is arbitrary but MAX_TAIL is set so its translation <= OUTLIE_CODE+16.
 * The OUTLIE scheme is a win because the tails seem unexpectedly long for
 * an exponential, some samples are bimodal, and zeros occur in the data.
 *
 * The defines should probably go in rice_common.h and the vaiables in in 
 * subs.c but it is very important have this stuff together.
 * DO NOT CHANGE THESE TABLES UNLESS YOU UNDERSTAND THE CODE
 */
#define MAX_TAIL 12
#define OUTLIE_CODE 10
#define EOF_CODE 27

/*
 * Coma code table
 *   difference in values yields # of bits in comma code
 *    Note, no input value can yield OUTLIE_CODE
 * Input value
 * -12-11-10 -9 -8 -7 -6 -5 -4 -3 -2 -1  0 +1 +2 +3 +4 +5 +6 +7  8  9 10 11 12
 * Returns
 */
const int code_table[] =
   {26,24,22,20,18,16,14,12, 9, 7, 5, 3, 1, 2, 4, 6, 8,11,13,15,17,19,21,23,25};

/*
 * Inverse Coma code table
 *   # of leading zeros in code => difference from modal value
 *
 *   Note, the OUTLIE_CODE entry is not used, the MSB value follows in the data.
 * Input
 *   0, 1, 2, 3, 4, 5, 6, 7, 8, imposibl, 10,11,12,13,14,15,16,17,18,19,
 *    20, 21,22, 23,24, 25, 26
 * Returns
 */
const int inv_table[]  =
    {0, 1,-1, 2,-2, 3,-3, 4,-4,OUTLIE_CODE,5,-5, 6,-6, 7,-7, 8,-8, 9,-9,
      10,-10,11,-11,12,-12, EOF_CODE};
/*
 * Global variables and functions contained in rice_subs.c
 */
extern void (*bgnd_flag)(int);
extern char ofname [];
extern int exit_stat;     /* per-file status */
extern int perm_stat;     /* permanent status */
extern int precious;      /* Don't unlink output file on interrupt */
extern int verbose;       /* don't tell me about compression */
extern int force;         /* force writting of output if negative compress */

void onintr (int signo);
void oops (int signo);
void version(void);
int foreground(void);
void writeerr(void);
void prratio(FILE *stream, int num, int den);
void copystat(char *ifname, char *ofname);
int compress_header(int *bitpix);
int decompress_header(char buf[80],
		      int *bitpix, int *naxis, int *naxis1, int *naxis2);
void reset_header(int byte_count);


