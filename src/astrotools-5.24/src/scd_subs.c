#include "scd_common.h"
#include "unistd.h"

int exit_stat = 0;     /* per-file status */
int perm_stat = 0;     /* permanent status */
int precious = 1;      /* Don't unlink output file on interrupt */
int verbose = 0;       /* 0>quiet, 1>summary, 2> details */
int force = 0;
char ofname [PATH_MAX];

void (*bgnd_flag)(int) = SIG_IGN;

void onintr (int signo)   /* NOTUSED */
{
  write(STDERR_FILENO,"A interupt has occured - unlinking output file\n", 47);
  if (!precious)
     unlink ( ofname );
  exit ( 1 );
}

/*
 * A memory access violation occurred and signal caught
 */ 
void oops (int signo)   /* wild pointer -- assume bad input */ /* NOTUSED */
{
  write(STDERR_FILENO,"A segv interupt has occured\n", 28);
  unlink ( ofname );
  exit ( 1 );
}

/*
 * The version number put in the first byte after the fits header
 */
void version(void)
{
   fprintf(stderr,"SDSS fits compression version %d\n",SRICE_VERSION);
}

/*
 * This routine returns 1 if we are running in the foreground and stderr
 * is a tty.
 */
int foreground(void)
{
  if(bgnd_flag) {  /* background? */
    return(0);
  } else {      /* foreground */
    if(isatty(2)) {    /* and stderr is a tty */
      return(1);
    } else {
      return(0);
    }
  }
}

/*
 * Report a write error and unlink the output file  - used after system errors
 */
void writeerr(void)
{
  perror ( ofname );
  unlink ( ofname );
  exit ( 1 );
}

/*
 * Report a header error and unlink the output file  - used after header errors
 */
void diehelp(char *msg)
{
  fprintf (stderr, "error: %s for (de)compression\n", msg);
  unlink ( ofname );
  exit(1);
}

/*
 * Print a ratio to the specified stream
 */
void prratio(FILE *stream, int num, int den)
{
   int q;                 /* Doesn't need to be long */
   if (verbose)
   {
      if(num > 214748L) {             /* 2147483647/10000 */
         q = num / (den / 10000L);
      } else {
         q = 10000L * num / den;         /* Long calculations, though */
      }
      if (q < 0) {
         putc('-', stream);
         q = -q;
      }
      fprintf(stream, "%d.%02d%%", q / 100, q % 100);
   }
}

/*
 * Copy the stat() information from the input file to the output file
 */
void copystat(char *ifname, char *ofname)
{
  struct stat statbuf;
  int mode;
  struct utimbuf timep;

  fclose(stdout);
  if (stat(ifname, &statbuf)) 
  {    /* Get stat on input file */
    perror(ifname);
    return;
  }
  if ((statbuf.st_mode & S_IFMT /*0170000*/ ) != S_IFREG /*0100000*/ ) 
  {
    fprintf(stderr, "%s: ", ifname);
    fprintf(stderr, " -- not a regular file: unchanged\n");
    exit_stat = 1;
    perm_stat = 1;
  } 
  else if (statbuf.st_nlink > 1) 
  {
    fprintf(stderr, "%s: ", ifname);
    fprintf(stderr, " -- has %d other links: unchanged\n",
    statbuf.st_nlink - 1);
    exit_stat = 1;
    perm_stat = 1;
  } 
/*  else if (exit_stat == 2 && (!force)) */
  else if (exit_stat == 2)
  { /* No compression: remove file.R */
       fprintf(stderr, " -- Negative compression, file unchanged\n");
  } 
  else 
  {      /*       Successful Compression       */
    exit_stat = 0;
    mode = statbuf.st_mode & 07777;
    if (chmod(ofname, mode))                        /* Copy modes */
      perror(ofname);
    chown(ofname, statbuf.st_uid, statbuf.st_gid);  /* Copy ownership */
    timep.actime = statbuf.st_atime;
    timep.modtime = statbuf.st_mtime;
    utime(ofname, &timep);                          /* Update times */
    if (unlink(ifname))                             /* Remove input file */
      perror(ifname);
    if(verbose)
    fprintf(stderr, " -- replaced with %s", ofname);
    return;                                         /* Successful return */
  }

  /* Unsuccessful return -- one of the tests failed */
  if (unlink(ofname))
      perror(ofname);
}


/*
 * A fits input file has a fits header that has 80 character records
 * that do not have trailing LF or CR.  These records are packed into
 * 2880 byte blocks.  The last record has an "END" and the remainder
 * of the header logical record is written.
 * This routine copies the header from stdin to stdout and checks for 
 * a few formatting things.  
 * 
 * It also modifies the SIMPLE record.  
 */
#define RECORDSIZE      2880    /* number of bytes in a FITS logical record  */
void put_fits_record(char *key, char *value)
{
int i, len;
   fwrite(key, 1, 8, stdout); 
   fwrite("= ", 1, 2, stdout);
   len=strlen(value);
   fwrite(value,1,len, stdout);
   for (i=0; i<80-9-1-len; i++) putchar(' ');
   if(ferror(stdout))
      writeerr();
}

int
put_fits_comment(char *key, char *value, int read_line, int die)
{
   if(read_line) {
      fread(value, 1, 80, stdin);
   }
   
   if (strncmp(key, value, 8) != 0)
   {
      if(die) {
	 diehelp("fits file invalid keyword");
      } else {
	 return(-1);
      }
   }
   fwrite("HISTORY =", 1, 9, stdout); 
   fwrite(&value[9],1,71,stdout);
   if(ferror(stdout)) {
      writeerr();
      return(-1);
   }

   return(0);
}

char comp_key[] = "*** SDSS COMPRESSED ***";

void
compress_header (int *bitpix)		/* value of BITPIX */
{
   int hc = 0;                  /* count number of bytes read in header */
   int hdr_remain;              /* number of bytes left in a 2880 block */
   char buf[RECORDSIZE];        /* a buffer to read read header */

   put_fits_record("SIMPLE   ","T");
   put_fits_record("BITPIX   ","8");
   put_fits_record("NAXIS    ","1");
   put_fits_record("NAXIS1   ","0");
   put_fits_record("HISTORY  ",comp_key);

   if(put_fits_comment("SIMPLE  ",buf,1, 0) < 0 &&
      put_fits_comment("XTENSION",buf,0, 0) < 0) {
      diehelp("fits file missing SIMPLE and XTENSION");
   }
   put_fits_comment("BITPIX  ",buf,1,1);
   *bitpix = atoi(buf+10);
   if (*bitpix != 16) {
      if(!force) {
	 diehelp("fits file invalid BITPIX");
      }
   }
   put_fits_comment("NAXIS   ",buf,1,1);
   if (atoi(buf+10)  != 2)    diehelp("fits file invalid NAXIS");
   put_fits_comment("NAXIS1  ",buf,1,1);
   put_fits_comment("NAXIS2  ",buf,1,1);
   while (fread(buf, 1, 80, stdin) == 80) {
      hc +=80 ;

      if(put_fits_comment("PCOUNT  ",buf, 0, 0) == 0 ||
	 put_fits_comment("GCOUNT  ",buf, 0, 0) == 0) {
	 continue;
      }

      fwrite (buf, 1, 80, stdout);
      if(ferror(stdout))
              writeerr();
      if (strncmp ("END     ",buf , 8) == 0 ) break;
   }
/*
 * Flush a fits 2880 FITS logical record
 */ 
   hdr_remain = RECORDSIZE - ((hc+400) % RECORDSIZE);
   if (hdr_remain == RECORDSIZE) hdr_remain=0;
   while (hdr_remain)
   {
      (void)getchar();
      hdr_remain--;
   };
   if(ferror(stdin))
      writeerr();

   hdr_remain = RECORDSIZE - ((hc+800) % RECORDSIZE);
   if (hdr_remain == RECORDSIZE) hdr_remain=0;
   while (hdr_remain) 
   {
      putchar(' ');
      hdr_remain--;
   }; 
   if(ferror(stdout))
      writeerr();
}

void reset_header(int byte_count)
{
char string[20];
   if ( fseek(stdout, 240, SEEK_SET) )
      return;
   sprintf(string, "%d", byte_count);
   put_fits_record("NAXIS1   ",string);
}

void get_fits_comment(char buf[80], char *key, int read_card)
{
   if(read_card) {
      fread(buf, 1, 80, stdin);
   }
   strncpy(buf,key,8);
   fwrite(buf,1,80,stdout);
   if(ferror(stdout))
      writeerr();
}


void
decompress_header (char buf[80],
		   int *bitpix,		/* value of BITPIX */
		   int *naxis,		/* value of NAXIS */
		   int *naxis1,		/*          NAXIS1 */
		   int *naxis2)		/*          NAXIS2 */
{
   int nextra = 0;			/* number of cards in header
					   that weren't written to output */
   int hc = 0;                  /* count number of bytes read in header */
   int hdr_remain;              /* number of bytes left in a 2880 block */
   int is_xtension = 0;			/* is this an XTENSION? */

#if 0					/* decompress() read the first card */
   fread(buf, 1, 80, stdin);
#endif
   nextra = 1;				/* we didn't write buf[] to stdout */

   if(strncmp(buf, "XTENSION= ", 10) == 0) { /* not compressed at all */
      hc +=80 ;

      fwrite (buf, 1, 80, stdout);
      if(ferror(stdout)) writeerr();

      nextra--;				/* we _did_ write it after all */
   } else {
      fread(buf, 1, 80, stdin); nextra++;
      fread(buf, 1, 80, stdin); nextra++;
      fread(buf, 1, 80, stdin); nextra++;
      fread(buf, 1, 80, stdin); nextra++;

      if (strncmp(&buf[10], comp_key, sizeof(comp_key)-1) )
	{
	   diehelp("fits file is not sdss compressed");
	}
      
      fread(buf, 1, 80, stdin); hc += 80;
      {
	 char *ptr = buf;
	 ptr += strlen("HISTORY =");
	 while(isspace(*ptr)) ptr++;
	 if(strncmp(ptr, "'IMAGE", 6) == 0) { /* an IMAGE extension */
	    is_xtension = 1;
	    get_fits_comment(buf, "XTENSION", 0);
	 } else {
	    get_fits_comment(buf, "SIMPLE  ", 0);
	 }
      }

      get_fits_comment(buf, "BITPIX  ", 1); hc += 80;
      *bitpix = atoi(&buf[10]);
      get_fits_comment(buf, "NAXIS   ", 1); hc += 80;
      *naxis = atoi(&buf[10]);
      get_fits_comment(buf, "NAXIS1  ", 1); hc += 80;
      *naxis1 = atoi(&buf[10]);
      get_fits_comment(buf, "NAXIS2  ", 1); hc += 80;
      *naxis2 = atoi(&buf[10]);
      
      if(is_xtension) {
	 get_fits_comment(buf, "PCOUNT  ", 1); hc += 80;
	 get_fits_comment(buf, "GCOUNT  ", 1); hc += 80;
      }
   }
   
   while (fread(buf, 1, 80, stdin) == 80) {
      hc +=80 ;

      if(strncmp(buf, "BITPIX   ", 8) == 0) {
	 *bitpix = atoi(&buf[10]);
      } else if(strncmp(buf, "NAXIS   ", 8) == 0) {
	 *naxis = atoi(&buf[10]);
      } else if(strncmp(buf, "NAXIS1  ", 8) == 0) {
	 *naxis1 = atoi(&buf[10]);
      } else if(strncmp(buf, "NAXIS2  ", 8) == 0) {
	 *naxis2 = atoi(&buf[10]);
      }

      fwrite (buf, 1, 80, stdout);
      if(ferror(stdout))
              writeerr();
      if (strncmp ("END     ",buf , 8) == 0 ) break;
   }
/*
 * Flush a FITS 2880 byte logical record
 */
   hdr_remain = RECORDSIZE - (hc % RECORDSIZE);
   if (hdr_remain == RECORDSIZE) hdr_remain=0;
   while (hdr_remain)
   {
      putchar(' ');
      hdr_remain--;
   };
   if(ferror(stdout))
      writeerr();
/*
 * Read to the end of the input FITS record.
 *
 * If the data was compressed there were an extra <nextra> cards read
 * but not written, so take them into account
 */
   hc += 80*nextra;
   hdr_remain = RECORDSIZE - (hc % RECORDSIZE);
   if (hdr_remain == RECORDSIZE) hdr_remain=0;
   while (hdr_remain) {
      (void)getchar();
      hdr_remain--;
   };
}
