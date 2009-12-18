#include "scd_common.h"
#include "scd_proto.h"
/*
 * rice - rice decompress sdss image files
 */

void Usage(void) {
fprintf(stderr,"Usage: sdssdecompress [options] [file ...]\n");
fprintf(stderr,"-c     Write output on stdout, don't remove original.\n");
fprintf(stderr,"-f     Forces output file to be generated\n");
fprintf(stderr,"-H n   Read only nth HDU\n");
fprintf(stderr,"-h     Print this mesage\n");
fprintf(stderr,"-v     Write compression statistics\n");
fprintf(stderr,"-V     Show version\n");
fprintf(stderr,"By default, all HDUs will be processed\n");
}

void diehelp(char *msg);

/*
 * Mask[i] & compelt get the i LSBs from az compressed element
 */
COMPTYPE lsbmask[] = {
  0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff, 0x1ff, 0x3ff, 0x7ff, 0xfff,
  0x1fff, 0x3fff, 0x7fff, 0xffff };

/*
 * A mask to determine if the MSB of a compressed value is set
 */
#if (LONG_MAX > 2147483647 )
#define MSB_MASK 0x8000000000000000
#endif
#if (LONG_MAX == 2147483647)
#define MSB_MASK 0x80000000
#endif

/*
 * Get the next compressed element depending on endedness
 */
#if SDSS_LITTLE_ENDIAN && (LONG_MAX > 2147483647)
#define GET_COMP() \
{ \
  if (feof(stdin)) diehelp("Unexpected error or eof in input file"); \
  COMPCHAR[7]=getchar(); \
  COMPCHAR[6]=getchar(); \
  COMPCHAR[5]=getchar(); \
  COMPCHAR[4]=getchar(); \
  COMPCHAR[3]=getchar(); \
  COMPCHAR[2]=getchar(); \
  COMPCHAR[1]=getchar(); \
  COMPCHAR[0]=getchar(); \
  nread += 8;            \
  comp_count++;          \
}
#endif

#if SDSS_LITTLE_ENDIAN && (LONG_MAX == 2147483647)
#define GET_COMP() \
{ \
  if (feof(stdin)) diehelp("Unexpected error or eof in input file"); \
  COMPCHAR[3]=getchar(); \
  COMPCHAR[2]=getchar(); \
  COMPCHAR[1]=getchar(); \
  COMPCHAR[0]=getchar(); \
  nread += 4;            \
  comp_count++;          \
}
#endif

#if !defined(SDSS_LITTLE_ENDIAN) && (LONG_MAX > 2147483647)
#define GET_COMP() \
{  \
  if (feof(stdin)) diehelp("Unexpected error or eof in input file"); \
  COMPCHAR[0]=getchar(); \
  COMPCHAR[1]=getchar(); \
  COMPCHAR[2]=getchar(); \
  COMPCHAR[3]=getchar(); \
  COMPCHAR[4]=getchar(); \
  COMPCHAR[5]=getchar(); \
  COMPCHAR[6]=getchar(); \
  COMPCHAR[7]=getchar(); \
  nread += 8;            \
  comp_count++;          \
}
#endif

#if !defined(SDSS_LITTLE_ENDIAN) && (LONG_MAX == 2147483647)
#define GET_COMP() \
{  \
  if (feof(stdin)) diehelp("Unexpected error or eof in input file"); \
  COMPCHAR[0]=getchar(); \
  COMPCHAR[1]=getchar(); \
  COMPCHAR[2]=getchar(); \
  COMPCHAR[3]=getchar(); \
  nread += 4;            \
  comp_count++;          \
}
#endif

/*
 * Put the next pixel to the uncompressed fits file - stdout
 */
#ifdef SDSS_LITTLE_ENDIAN
#define PUT_PIX(p) \
{ \
  putchar(p.pixbyte[1]); \
  putchar(p.pixbyte[0]); \
  pix_count++;           \
}

#else
#define PUT_PIX(p) \
{  \
  putchar(p.pixbyte[0]); \
  putchar(p.pixbyte[1]); \
  pix_count++;           \
}
#endif


/*
 * Get n bits from stdin into v
 *   compbitsremain - number of bits remaining in this compressed word
 *   bitsleft   - number of bits needed from next compressed word
 */
#define GET_BITS(v,n) \
{  \
  bitsleft = compbitsremain - n; \
  if (bitsleft > 0) \
  { \
     v = (COMPWORD >> (COMP_ELT_BITS - n)) & lsbmask[n]; \
     COMPWORD = COMPWORD << n; \
     compbitsremain = bitsleft; \
  } \
  else if (bitsleft == 0) \
  { \
     v = (COMPWORD >> (COMP_ELT_BITS - n)) & lsbmask[n]; \
     GET_COMP(); \
     compbitsremain =  COMP_ELT_BITS; \
  } \
  else \
  { \
     bitsleft = -bitsleft; \
     v = (COMPWORD >> (COMP_ELT_BITS-compbitsremain)) & lsbmask[compbitsremain]; \
     GET_COMP(); \
     v = (v << bitsleft) | \
           ((COMPWORD >> (COMP_ELT_BITS-bitsleft)) & lsbmask[bitsleft]); \
     COMPWORD = COMPWORD << bitsleft; \
     compbitsremain = COMP_ELT_BITS - bitsleft; \
  } \
}

/*
 * Get the next bit from the compressed bit stream
 */
#define GET1BIT() \
{ \
  COMPWORD = COMPWORD<<1; \
  compbitsremain--; \
  if (!compbitsremain) \
  { \
    GET_COMP(); \
    compbitsremain = COMP_ELT_BITS; \
  } \
} 

/*****************************************************************************/
/*
 * If requested skip over HDUs.  This is hard as the header doesn't tell
 * us how long the compressed section of the file is (and it isn't easy
 * to fix this with GCOUNT/PCOUNT keywords as sdsscompress may have been
 * writing to a pipe, which makes it impossible to seek back and fixup
 * the header when the length is known).
 *
 * All that we know is that each HDU is a multiple of 2880 bytes long. Things
 * are a little complicated by the need to only read each byte once as
 * once again we may be reading from a pipe.  The result of this is that
 * exactly 80 bytes of the _next_ record are always read by this routine,
 * even when not skipping over any HDUs; decompress_header() knows this.
 */
static int
skip_hdus(char buf[80],			/* buffer to read lines */
	  int hdu)			/* desired HDU */
{
   int i = 0;

   for(;;) {
      if(fread(buf, 1, 80, stdin) != 80) {
	 return(-1);
      }

      if(strncmp(buf, "SIMPLE  = ", 10) == 0 ||
	 strncmp(buf, "XTENSION= ", 10) == 0) {
	 if(i == hdu) {
	    return(0);
	 }
	 i++;
      }

      if(fseek(stdin, 2880 - 80, SEEK_CUR) != 0) {
	 return(-1);
      }
   }

   return(-1);
}

/*****************************************************************************/
/*
 * Decompress stdin and write pixels to stdout.
 */
static int
decompress(int hdu,			/* desired HDU (relative to current
					   point in file) */
	   int mortal)			/* if false only return a status,
					   and don't die */
{
int bitpix = 0;				/* value of BITPIX from header */
char buf[80] = "";
int compbitsremain;
int bitsleft;
int code_count=0;
int msbval=0;
int lsbval=0;
int lsb_count=0;
int msb_count;
int modeval;
int naxis = 0, naxis1 = 0, naxis2 = 0;	/* NAXIS etc. from header */
int nread;				/* number of chars read from data
					   section of file (following header)*/
int rice_version;
int comp_count=0;       /* total # of input compressed elts for stats */
int pix_count=0;        /* total # of output pixels for stats */
int algorithm;

COMP_UNION COMP_ELT;
#define COMPWORD COMP_ELT.compval
#define COMPCHAR COMP_ELT.compbyte

PIXEL_VALUE pixval;

   if(skip_hdus(buf, hdu) < 0) {
      if(mortal) {
	 diehelp("Cannot find requested HDU");
      } else {
	 return(-1);
      }
   }

   decompress_header(buf, &bitpix, &naxis, &naxis1, &naxis2);
/*
 * Is file uncompressed? If so, simply read it
 */
   if(bitpix != 16) {
      char record[2880];		/* i/o record */
      int nchar;			/* number of chars read this time */

      nread = 0;
      while(nread < naxis1*naxis2) {
	 if((nchar = fread(record, 1, sizeof(record), stdin)) <= 0) {
	    break;
	 }

	 fwrite(record, 1, nchar, stdout);
	 if(ferror(stdout)) writeerr();
	 
	 nread += nchar;
      }

      if(nread%2880 != 0) {		/* write complete FITS records */
	 memset(record, 2880, ' ');
	 fwrite(record, 1, 2880 - nread%2880, stdout);
      }

      return(0);
   }
/*
 *  Get intialization stuff stored in the file and prime the pump
 */
  nread = 0;
  rice_version= getchar(); nread++;
  lsb_count   = getchar(); nread++;
  msb_count   = BITS_PER_PIX - lsb_count;
  algorithm   = getchar(); nread++;
  if(verbose) 
     fprintf(stderr,"SDSS compress version %d compressed with %d LSB and algorithm %d\n",
        rice_version, lsb_count, algorithm);
  GET_COMP();
  compbitsremain = COMP_ELT_BITS;
  GET_BITS(modeval,msb_count);

  while (code_count != EOF_CODE-1)
  {
#ifdef DEBUG
     fprintf(stderr," %x remain %d ", COMPWORD, compbitsremain);
#endif /* DEBUG */
/*
 * Get lsb_count LSB of the pixel into lsbval
 */
     GET_BITS(lsbval,lsb_count);
/*
 * Get the MSB bits from comma code
 */
     code_count=0;
     while (!(COMPWORD&MSB_MASK))
     {
        code_count++;
        GET1BIT();
     } 
     GET1BIT();
     if (code_count == OUTLIE_CODE-1)
     {
        GET_BITS(msbval,msb_count);
     } else if (code_count == EOF_CODE-1 )
     {
        break;
     } else
     { 
        msbval = (modeval + inv_table[code_count]);
     }
     if (algorithm == SRICE_DIF_ALGORITHM )
        modeval = msbval;
/*
 * The pixel is the MSB pls the LSB
 */
     pixval.pixel = lsbval | (msbval << lsb_count);
     PUT_PIX(pixval);
#ifdef DEBUG
     fprintf(stderr,"val %x lsbval %x prvmsb %x msbval %x  \n", 
             pixval.pixel, lsbval, modeval, msbval);
#endif /* DEBUG */
  }
/*
 * Read out to a FITS record boundary
 */
  if(nread%2880 != 0) {
     if(fseek(stdin, 2880 - nread%2880, SEEK_CUR) != 0) {
	fprintf(stderr,"Error seeking to end of FITS record");
     }
  }

  prratio(stderr, BYTES_PER_PIX*pix_count, COMP_ELT_BYTES*comp_count);

  if(ferror(stdout))
      writeerr();
  return(0);
}

/*****************************************************************************/

int main(int argc,char **argv )
{
  int c;
  extern char *optarg;
  extern int optind;

  int all_hdus = 1;			/* process all HDUs */
  int hdu = -1;				/* desired HDU */
  int i;
  int overwrite = 0;			/* Don't overwrite without -f flag */
  char tempname[PATH_MAX];
  char **filelist, **fileptr;
  char *cp;
  int zcat_flg = 0;			/* write to stdout */
  struct stat statbuf;

  bgnd_flag = signal ( SIGINT, SIG_IGN );
  if ( bgnd_flag != SIG_IGN ) 
  {
	signal ( SIGINT, onintr );
	signal ( SIGSEGV, oops );
  }

  filelist = fileptr = (char **)(malloc(argc * sizeof(*argv)));
  *filelist = NULL;

  if((cp = rindex(argv[0], '/')) != 0) {
	cp++;
  } else {
	cp = argv[0];
  }
  if(strcmp(cp, "rcat") == 0) {
	zcat_flg = 1;
  }

  /* Argument Processing
   * All flags are optional.
   * -V => show version
   * -v => verbose
   * -f => force overwrite of output file
   * -c => cat all output to stdout
   * if a string is left, must be an input filename.
   */
  while ((c = getopt(argc, argv, "cfhH:vV")) != EOF)
  {
     switch (c) 
     {
     case 'V':
        version();
        break;
     case 'v':
        verbose = 1;
        break;
     case 'f':
        overwrite = 1;
        force = 1;
        break;
      case 'H':
	hdu = atoi(optarg);
	break;
     case 'c':
        zcat_flg = 1;
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

  if(hdu >= 0) {
     all_hdus = 0;			/* they chose an HDU */
  }

  for ( ; optind < argc; optind++)
  {    /* Input file list */
      *fileptr++ = argv[optind];  
      *fileptr = NULL;
  }

  if (*filelist == NULL) {
     zcat_flg = 1;
     *filelist = "-";			/* stdout */
  }
  
  for (fileptr = filelist; *fileptr; fileptr++) {
     if(strcmp(*fileptr, "-") != 0) {	/* a real file*/
	/* Check for .R suffix */
	if (strcmp(*fileptr + strlen(*fileptr) - 2, ".R") != 0) 
	  {
	     /* No .R: tack one on */
	     strcpy(tempname, *fileptr);
	     strcat(tempname, ".R");
	     *fileptr = tempname;
	  }
	/* Open input file */
	if ((freopen(*fileptr, "r", stdin)) == NULL) 
	  {
	     perror(*fileptr);
	     perm_stat = 1;
	     continue;
	  }
	/* Generate output filename */
	strcpy(ofname, *fileptr);
	ofname[strlen(*fileptr) - 2] = '\0';  /* Strip off .R */
	
	/* Check for overwrite of existing file */
	if (overwrite == 0 && zcat_flg == 0) {
	   if (stat(ofname, &statbuf) == 0) {
	      char response[2];
	      response[0] = 'n';
	      fprintf(stderr, "%s already exists;", ofname);
	      if (foreground()) {
		 fprintf(stderr," do you wish to overwrite %s (y or n)? ",
			 ofname);
		 fflush(stderr);
		 read(2, response, 2);
		 while (response[1] != '\n') {
		    if (read(2, response+1, 1) < 0) {        /* Ack! */
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
	if(zcat_flg == 0) {                /* Open output file */
	   if (freopen(ofname, "w", stdout) == NULL) {
              perror(ofname);
              perm_stat = 1;
              continue;
	   }
	   precious = 0;
	   if(verbose)
	     fprintf(stderr, "%s: ", *fileptr);
	}
     }
     
     /* Actually do the decompression */
     if(all_hdus) {
	for(i = 0; ; i++) {
	   exit_stat = decompress(0, 0);
	   if(exit_stat) {
	      if(i > 0) {
		 exit_stat = 0;	/* we reached the end of the file */
	      }
	      
	      break;
	   }
	}
     } else {
	exit_stat = decompress(hdu, 1);
     }
     
     if(zcat_flg == 0) {
	(void) copystat(*fileptr, ofname);        /* Copy stats */
	precious = 1;
	if((exit_stat == 1) || (verbose))
	  putc('\n', stderr);
     }
  }

  return(perm_stat ? perm_stat : exit_stat);
}

