#include "scd_common.h"
#include "scd_proto.h"
/*
 * rice_comp - rice compress sdss fits image files
 */

int forcebits = 0;      /* user settable max # bits/code */
int debug = 0;

void Usage(void)
{
  fprintf(stderr,"Usage: sdsscompress [options] [file ...]\n");
  fprintf(stderr,"-a     Show compression estimate calculations\n");
  fprintf(stderr,"-b n   Parameter forces compression of the <n> MSB\n");
  fprintf(stderr,"-c     Write output on stdout, don't remove original.\n");
  fprintf(stderr,"-d     Write extra debugging output.\n");
  fprintf(stderr,"-h     Print this message.\n");
  fprintf(stderr,"-m     Use mode in algorithm \n");
  fprintf(stderr,"-f     Forces output file to be generated\n");
  fprintf(stderr,"-v     Write compression statistics\n");
  fprintf(stderr,"-V     show version\n");
}
  
/*
 * Return a fits pixel in host ended order
 */
#ifdef SDSS_LITTLE_ENDIAN
#define BSWAP_PIX(pval) (pval.pixbyte[0]<<8 | pval.pixbyte[1])
#else
#define BSWAP_PIX(pval) pval.pixel 
#endif 

/*
 * Put a long value to stdout according to endedness
 */
#if SDSS_LITTLE_ENDIAN && (LONG_MAX != 2147483647)
#define PUT_OUT() \
{  \
  putchar(COMPCHAR[7]); \
  putchar(COMPCHAR[6]); \
  putchar(COMPCHAR[5]); \
  putchar(COMPCHAR[4]); \
  putchar(COMPCHAR[3]); \
  putchar(COMPCHAR[2]); \
  putchar(COMPCHAR[1]); \
  putchar(COMPCHAR[0]); \
  comp_count++;         \
}
#endif
#if SDSS_LITTLE_ENDIAN && (LONG_MAX == 2147483647)
#define PUT_OUT() \
{  \
  putchar(COMPCHAR[3]); \
  putchar(COMPCHAR[2]); \
  putchar(COMPCHAR[1]); \
  putchar(COMPCHAR[0]); \
  comp_count++;         \
}
#endif


#if !defined(SDSS_LITTLE_ENDIAN)  && (LONG_MAX != 2147483647)
#define PUT_OUT() \
{  \
  putchar(COMPCHAR[0]); \
  putchar(COMPCHAR[1]); \
  putchar(COMPCHAR[2]); \
  putchar(COMPCHAR[3]); \
  putchar(COMPCHAR[4]); \
  putchar(COMPCHAR[5]); \
  putchar(COMPCHAR[6]); \
  putchar(COMPCHAR[7]); \
  comp_count++;         \
}
#endif

#if !defined(SDSS_LITTLE_ENDIAN)  && (LONG_MAX == 2147483647)
#define PUT_OUT() \
{  \
  putchar(COMPCHAR[0]); \
  putchar(COMPCHAR[1]); \
  putchar(COMPCHAR[2]); \
  putchar(COMPCHAR[3]); \
  comp_count++;         \
}
#endif

const int shiftmask[] =
    {0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191,16383,32767, 65535};
/*
 * Put n bits from val into the compressions stream
 */
#define PUT_VAL(val,n) \
{ \
  bitsleft = compbitsremain - n; \
  if (bitsleft > 0) \
  { \
     COMPWORD = (COMPWORD << n) | (val & shiftmask[n]); \
     compbitsremain = bitsleft; \
  } \
  else if (bitsleft == 0) \
  { \
     COMPWORD = (COMPWORD << n) | (val & shiftmask[n]); \
     PUT_OUT(); \
     COMPWORD = 0; \
     compbitsremain =  COMP_ELT_BITS; \
  } \
  else \
  { \
     bitsleft = -bitsleft; \
     COMPWORD = (COMPWORD << compbitsremain) | \
            ((val >> bitsleft) & shiftmask[compbitsremain]); \
     PUT_OUT(); \
     COMPWORD = (val & shiftmask[bitsleft]); \
     compbitsremain =  COMP_ELT_BITS-bitsleft; \
  } \
}
    
/*
 * Put a comma code into the compressions stream
 */
#define PUT_CODE(code) \
{ \
  bitsleft = compbitsremain - code; \
  if (bitsleft > 0) \
  { \
     COMPWORD = (COMPWORD << code) | 1; \
     compbitsremain -= code; \
  } \
  else if (bitsleft == 0) \
  { \
     COMPWORD = (COMPWORD << code) | 1; \
     PUT_OUT(); \
     COMPWORD = 0; \
     compbitsremain =  COMP_ELT_BITS; \
  }  \
  else \
  { \
     bitsleft = -bitsleft; \
     COMPWORD = COMPWORD << compbitsremain; \
     PUT_OUT(); \
     COMPWORD = 1; \
     compbitsremain =  COMP_ELT_BITS-bitsleft; \
  } \
}
/*
 * Determine the number of random exponential bits in the LSB portion 
 * of a buffer of pixels.    Do this by 
 *     1) build a histogram 
 *     2) while compression improves 
 *     3)  roll the odd buckets for this number of LSB into the even buckets
 *         which produces the histogram with one LSB dropped.
 *
 * Assume there are is at least 1 LSB of the 16 bits so use 2^15 buckets
 * for the histogram.  This is only to save memory.
 * Returns:
 *   function value = #of LSB that produce the best estimated compression
 *                    -1 if compression is not positive.
 *   mode_bucket    = The mode value in LSB terms, i.e. with shifted lsb bits.
 */
int find_lsb( PIXEL_VALUE inbuf[], int npixs, int *mode_bucket )
{
int lsb_count;          /* # of bits in the least significant part of a pixel */
int bno;                /* index for the histogram buckets */
int pixno;              /* index for the pixels in the image file */
int bucket_inc;         /* # of bits we have added to the LSB count */
int mode;               /* the number of points in the mode bucket */

/* work variables for accuming compress estimate */
int curr_mode_bucket=0;   /* the mode bucket for a current # of bits */
int codeindex;          /* current bucket - mode bucket in LSB terms */
int weight;             /* number of bits this bucket will require */
int est_comp;           /* estimated compression for this bucket */
int tot_comp;           /* compression total for all buckets for this dist */
int best_comp;          /* best compression total for all distributions */
int best_lsb;           /* the LSB that produces the best compr so far */

#define NBUCKETS 65536
short histogram[NBUCKETS];
/*
 * Build a histogram
 */
   for (bno=0;   bno < NBUCKETS; bno++) histogram[bno]=0;
   for (pixno=0; pixno < npixs;  pixno++)
      histogram[BSWAP_PIX(inbuf[pixno])]++;
/*
 * Step thru the buckets accumulating estimated compressed size
 *  until the compressed total is greater than previous compressed totl
 */
   bucket_inc =1;
   lsb_count = 0;
   best_lsb  = 0;
   best_comp = INT_MAX;
   while (lsb_count <= BITS_PER_PIX)
   {
/*
 * Find the mode
 */
      mode = 0; tot_comp=0;
      for (bno=0; bno<NBUCKETS; bno+=bucket_inc)
      {
         if ((int)histogram[bno] > mode)
         {
             mode = histogram[bno];
             curr_mode_bucket = bno;
         }
      }
/*
 * Calculate the compression using this mode
 */
      curr_mode_bucket = curr_mode_bucket >> lsb_count;
      for (bno=0; bno<NBUCKETS; bno+=bucket_inc)
      {
         codeindex = (bno >> lsb_count)-curr_mode_bucket;
         weight = (codeindex >= -MAX_TAIL && codeindex <= MAX_TAIL)
            ? code_table[codeindex+MAX_TAIL]+lsb_count 
            : OUTLIE_CODE+BITS_PER_PIX ;
         est_comp = histogram[bno]*weight;
         tot_comp+= est_comp; 
         if (verbose>1)
            if (histogram[bno])  
               fprintf(stderr,"%8x has %d points weighted %d = %d\n",
                  bno>>lsb_count, histogram[bno], weight, est_comp);
      }
      tot_comp /= BITS_PER_PIX;
      if (verbose>1)
         fprintf(stderr,"%4d LSB mode %x sample size %d estimates %d\n",
            lsb_count, curr_mode_bucket, npixs, tot_comp);
/*
 * If this compressions is worse than previous then exit with previous
 */
      if (forcebits)
      {
         if (lsb_count==forcebits) 
         {
            best_comp = 0;
            best_lsb  = lsb_count;
            *mode_bucket = curr_mode_bucket;
            break;
         }
      }
      else
         if (tot_comp < best_comp)
         {
            best_comp = tot_comp;
            best_lsb  = lsb_count;
            *mode_bucket = curr_mode_bucket;
         } 
         else
         {
            lsb_count = best_lsb;
            break;
         }
/*
 * If compression is worse then roll adjacent buckets and increment # of LSB 
 */ 
      for(bno=0; bno<NBUCKETS; bno+=bucket_inc*2)
         histogram[bno] += histogram[bno+bucket_inc];
      bucket_inc *= 2;
      lsb_count+= 1;
   }
/*
 * If we can't compress 
 */
/*   if (best_comp < npixs || force) */
   if (best_comp < npixs)
   {
     return(best_lsb);
   }
   else
   {
      if(debug) {
	 fprintf(stderr,"find_lsb returns -1\n");
      }
      return(-1);
   }
}
/*
 * compress stdin to stdout
 *  return values 2 => no compression
 *                1 => an error occured
 *                0 => normal completion
 */
int comp_count;       /* total # of output bytes for stats */

int compress(
  int algorithm    /* 0 => use mode; 1 => use differences */
)

{
int base_value;         /* the value to be used as the base for the pixal 
                           difference in calculating the coma code 
                           Either the mode or the previous pixal-in MSB terms */
int bitpix = 0;		/* value of BITPIX */
int pixno;              /* index for the pixels in the image file */
int lsb_count=0;        /* # of bits in the least sig part of a pixel */
int msb_count=0;	/* # of bits in the most sig part of a pixel */
int pix_count=0;        /* total # of compressed elements for stats */
int nchar;              /* # of characters in the input buffer */
int npixs;		/* # of pixels in the input buffer */
int codeindex;          /* difference between mode and this pixel */
int code_count;         /* codeindex translated into a comman code # */
/*
 *
 */
COMP_UNION comp_elt;
#define COMPWORD comp_elt.compval
#define COMPCHAR comp_elt.compbyte
int compbitsremain = COMP_ELT_BITS;    /* bits left in the compress elt */
int bitsleft;                          /* bits that won't fit in this elt */

int pixval;
#define PIXBUFSIZ 16384
PIXEL_VALUE pixbuf[PIXBUFSIZ / BYTES_PER_PIX];

  compress_header(&bitpix);
  while ((nchar=fread ( pixbuf, 1, sizeof(pixbuf), stdin)) != 0 )
  {
     if(bitpix != 16) {			/* don't even try to compress */
	fwrite(pixbuf, 1, nchar, stdout);
	continue;
     }

    npixs = nchar/BYTES_PER_PIX;
/*
 * lsb_count  number or "random bits" in the data 
 * msb_count  number of "constant bits" in the data
 */
    if (!lsb_count)
    {
       lsb_count = find_lsb(pixbuf, npixs, &base_value);
       if (lsb_count < 0)
          return(2);
       COMPWORD = 0;
       msb_count = BITS_PER_PIX - lsb_count;
       putchar (SRICE_VERSION);
       putchar ((char)lsb_count);
       putchar ((char)algorithm);
       if (algorithm == SRICE_DIF_ALGORITHM )
       {
          base_value =0;
       }
       PUT_VAL(base_value,msb_count);
    }
  
    for (pixno=0; pixno<npixs; pixno++)
    {
       pixval= BSWAP_PIX(pixbuf[pixno]);
       pix_count++;
/* 
 * Put the fixed sized, least significant part in the output buffer 
 *     pixval         short data word to be compressed
 *     shiftcount     width in bits of the fixed sized, least sig part 
 *     compbitsremain number of bits left in the output word
 *     bitsleft       number of bits that did not fit in the first word
 */
       PUT_VAL(pixval,lsb_count);
       pixval = pixval >> lsb_count; 
  
/* Determine the comma code to use
 *     pixval         input value with the LSB shifted off
 *     codeindex      the difference code
 *     code_count     number of bits in comma code (codecount-1 zeros 
 *                        followed by a one)
 *                    code_count = OUTLIE_CODE is a big change in value.  
 */
       codeindex = 
                 (unsigned short)pixval - (unsigned short)base_value;
       if (codeindex >= -MAX_TAIL && codeindex <= MAX_TAIL)
          code_count = code_table[codeindex+MAX_TAIL];
       else
          code_count = OUTLIE_CODE;
       PUT_CODE(code_count);
  
/* If we put a code OUTLIE_CODE of then put the whole pixval
 */
       if ( code_count == OUTLIE_CODE)
          PUT_VAL(pixval, msb_count);

/* If using the difference algorithm then previous pixval = current pixval
 */
       if (algorithm == SRICE_DIF_ALGORITHM ) 
          base_value = pixval;
#ifdef DEBUG
       if (pixno < 4099)
       {
        fprintf(stderr,"%4d %4x %6d %6d %6d %6d %x %x ", 
          pixno ,pixbuf[pixno], pixval, codeindex, code_count, compbitsremain, 
          COMPWORD);
        fprintf(stderr,"\n");
       }
#endif /* DEBUG */
    }
  }
/*
 *  Mark the end of file with a lsb portion = 0 and a code EOF_CODE.
 */
  if(bitpix == 16) {
     pixval = 0;
     PUT_VAL(pixval,lsb_count);
     PUT_CODE(EOF_CODE);
     if (compbitsremain != COMP_ELT_BITS ) 
       {
	  COMPWORD = COMPWORD << compbitsremain;
	  PUT_OUT();
       }
/*
 * Fits requires 2880 bye blocks 3 is for the compress control info
 */
     nchar = 2880 - (( COMP_ELT_BYTES*comp_count+3) % 2880);
     while (nchar)
       {
	  (void)putchar(0);
	  nchar--;
       };
     if(ferror(stdin))
       writeerr();
     
     prratio(stderr, COMP_ELT_BYTES*comp_count, BYTES_PER_PIX*pix_count);
     if ( COMP_ELT_BYTES*comp_count > BYTES_PER_PIX*pix_count ) {
	if(debug) {
	   fprintf(stderr,"Unable to compress: %d v. %d",
		   COMP_ELT_BYTES*comp_count, BYTES_PER_PIX*pix_count);
	   
	}
	return(2);
     }
  }
  return(0);
}

/*****************************************************************
 *
 * Usage: rice [-options] [-b bits] [file ...]
 * Inputs:
 *
 *  -a:   Show compression estimate calculations
 *
 *  -b:   Parameter forces compression of the <b> MSB
 *
 *  -c:   Write output on stdout, don't remove original.
 *
 *  -m:   Use mode in algorithm
 *
 *  -f:   Forces output file to be generated, even if one already
 *        exists, and even if no space is saved by compressing.
 *        If -f is not used, the user will be prompted if stdin is
 *        a tty, otherwise, the output file will not be overwritten.
 *
 *  -v:   Write compression statistics
 *
 *  -V => show version
 *
 *   file ...:   Files to be compressed.  If none specified, stdin
 *        is used.
 * Outputs:
 *  file.R:      Compressed form of file with same mode, owner, and utimes
 *  or stdout   (if stdin used as input)
 *
 * Assumptions:
 *  When filenames are given, replaces with the compressed version
 *  (.R suffix) only if the file decreases in size.
 * Algorithm:
 *   Modified Rice Algorithm.  File manipulation based on the unix pack
 */

int main(int argc, char *argv[] )
{
  int c;
  int diff_flg = 1;

  extern char *optarg;
  extern int optind;

  int overwrite = 0;  /* Do not overwrite unless given -f flag */
  char **filelist, **fileptr;
  int zcat_flg = 0;
  struct stat statbuf;

  bgnd_flag = signal ( SIGINT, SIG_IGN );
  if ( bgnd_flag != SIG_IGN ) {
    signal ( SIGINT, onintr );
    signal ( SIGSEGV, oops );
  }

  filelist = fileptr = (char **)(malloc(argc * sizeof(*argv)));
  *filelist = NULL;

  while ((c = getopt(argc, argv, "hVvacdmfb:")) != EOF)
  {
     switch (c) 
     {
     case 'V':
        version();
        break;
     case 'v':
        verbose = 1;
        break;
     case 'd':
        debug = 1;
	break;
     case 'f':
        overwrite = 1;
        force = 1;
        break;
     case 'b':
        forcebits = atoi(optarg);
        if (forcebits >16 || forcebits < 1) 
        {
           fprintf(stderr, "-b value gt 16 or lt 1\n");
           Usage();
           exit(1);
        }
        break;
     case 'm':
        diff_flg = 0;
        break;
     case 'c':
        zcat_flg = 1;
        break;
     case 'a':
        verbose = 2;
        break;
     case 'h':
        Usage();
        exit(0);
     default:
        fprintf(stderr, "Unknown flag: '%c'; ", **argv);
        Usage();
        exit(1);
     }
   }
   for ( ; optind < argc; optind++)
   {    /* Input file list */
      *fileptr++ = argv[optind];  
      *fileptr = NULL;
   }

  if (*filelist != NULL) {
    for (fileptr = filelist; *fileptr; fileptr++) {
      comp_count= 0;
      exit_stat = 0;
      if (strcmp(*fileptr + strlen(*fileptr) - 2, ".R") == 0) {
        fprintf(stderr, "%s: already has .R suffix -- no change\n", *fileptr);
        continue;
      }
      /* Open input file */
      if ((freopen(*fileptr, "r", stdin)) == NULL) {
        perror(*fileptr);
        perm_stat = 1;
        continue;
      }
      stat ( *fileptr, &statbuf );

      /* Generate output filename */
      strcpy(ofname, *fileptr);
      strcat(ofname, ".R");
      /* Check for overwrite of existing file */
      if (overwrite == 0 && zcat_flg == 0) {
        if (stat(ofname, &statbuf) == 0) {
          char response[2];
          response[0] = 'n';
          fprintf(stderr, "%s already exists;", ofname);
          if (foreground()) {
            fprintf(stderr, " do you wish to overwrite %s (y or n)? ",
            ofname);
            fflush(stderr);
            read(2, response, 2);
            while (response[1] != '\n') {
              if (read(2, response+1, 1) < 0) {  /* Ack! */
                perror("stderr"); break;
              }
            }
          }
          if (response[0] != 'y') {
            fprintf(stderr, "\tnot overwritten\n");
            continue;
          }
        }
      }
      if(zcat_flg == 0) {         /* Open output file */
         if (freopen(ofname, "w", stdout) == NULL) {
            perror(ofname);
            perm_stat = 1;
            continue;
         }
         precious = 0;
         if(verbose)
            fprintf(stderr, "%s: \n", *fileptr);
      }
        /* Actually do the compression/decompression */
      exit_stat = compress(diff_flg);
      if(zcat_flg == 0) {
         reset_header(COMP_ELT_BYTES*comp_count);
         (void) copystat(*fileptr, ofname);  /* Copy stats */
         precious = 1;
         if((exit_stat == 1) || (verbose))
            putc('\n', stderr);
      }
    }
  } else {    /* Standard input */
    exit_stat = compress(diff_flg);
    if(verbose)
      putc('\n', stderr);
  }

  if(debug) {
     fprintf(stderr,"Exit code: %d\n", perm_stat ? perm_stat : exit_stat);
  }
  return(perm_stat ? perm_stat : exit_stat);
}
