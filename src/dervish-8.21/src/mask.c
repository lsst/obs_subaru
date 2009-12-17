/*****************************************************************************
r*****************************************************************************
**
** FILE:
**      mask.c
**
** ABSTRACT:
**      This file contains routines that read/write FITS format files that
**       contain masks.
**
** ENTRY POINT          SCOPE   DESCRIPTION
** -------------------------------------------------------------------------
** shMaskReadAsFits      public  Reads a FITS file into a mask
** shMaskWriteAsFits     public  Writes a mask in a FITS format file
**
** ENVIRONMENT:
**      ANSI C.
**
** REQUIRED PRODUCTS:
**      libfits
**
** AUTHORS:
**      Creation date:  Mar 10, 1994
**      George Jetson
**      Smokey Stover
**
******************************************************************************
******************************************************************************
*/
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include "libfits.h"
#include "shCFitsIo.h"
#include "shCMask.h"
#include "shCErrStack.h"
#include "shCEnvscan.h"
#include "shCGarbage.h"
#include "shCUtils.h"

#ifdef __sgi    /* for kill prototype */
extern int   kill(pid_t pid, int sig);
extern FILE *fdopen(int, const char *);
#endif

/*============================================================================
**============================================================================
**
** LOCAL MACROS, DEFINITIONS, ETC.
**
**============================================================================
*/
/*----------------------------------------------------------------------------
**
** LOCAL DEFINITIONS
*/
static RET_CODE maskReadHdr(char *a_fname, FILE **a_f, char  **a_hdrVec);
/*----------------------------------------------------------------------------
**
** GLOBAL VARIABLES
*/

/*============================================================================
**============================================================================
**
** ROUTINE: maskReadHdr
**
** DESCRIPTION:
**      Initialize to Read a FITS Mask header.
**
** RETURN VALUES:
**      SH_SUCCESS
**      SH_FITS_OPEN_ERR
**
** CALLS TO:
**      shErrStackPush
**      f_rdhead, f_save
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static RET_CODE maskReadHdr
   (
   char  *a_fname,      /* IN:  translated file name */
   FILE  **a_f,         /* OUT: file descriptor pointer */
   char  **a_hdrVec     /* MOD: vector for array of header line addresses */
   )
{
RET_CODE rstatus;
int      i;
char     mode[] = "r";

/* Initialize the storage for the header vector */
for (i = 0 ; i < (MAXHDRLINE+1) ; ++i)
   {a_hdrVec[i] = 0;}

if ((*a_f != NULL) || (*a_f = f_fopen(a_fname, mode)))
{
   /* Now read the header */
   if (f_rdhead(a_hdrVec, MAXHDRLINE, *a_f))
   {
      /* Save the file information as f_rdhead will check for it. */
      if (! f_save(*a_f, a_hdrVec, mode))
      {
         shErrStackPush("ERROR : Could not save FITS file information.");
         rstatus = SH_FITS_HDR_SAVE_ERR;
      }
      else
         {rstatus = SH_SUCCESS;}
   }
   else
   {
      shErrStackPush("ERROR : Could not read FITS header.");
      rstatus = SH_FITS_HDR_READ_ERR;
   }
}
else
{
   shErrStackPush("ERROR : Could not open FITS file : %s", a_fname);
   rstatus = SH_FITS_OPEN_ERR;
}

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: maskWriteHdr
**
** DESCRIPTION:
**      Initialize to write a FITS Mask header.
**
** RETURN VALUES:
**      SH_SUCCESS
**      SH_FITS_OPEN_ERR
**
** CALLS TO:
**      shErrStackPush
**      f_newhdr
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static RET_CODE maskWriteHdr
   (
   char  *a_fname,      /* IN:  translated file name */
   FILE  **a_f,         /* OUT: file descriptor pointer */
   char  **a_hdrVec     /* MOD: vector for array of header line addresses */
   )
{
RET_CODE rstatus;
   if ((*a_f != NULL) || (*a_f = f_fopen(a_fname, "w")))
   {
      /* Save the file information as f_wrhead will check for it. */
      if (f_save(*a_f, a_hdrVec, "w"))
      {
        /* Now write the header */
        if (f_wrhead(a_hdrVec, *a_f))
        {
           rstatus = SH_SUCCESS;
        }
        else
        {
           shErrStackPush("ERROR : Could not write FITS header.");
           rstatus = SH_FITS_HDR_WRITE_ERR;
        }
      }
      else 
      {
         shErrStackPush("ERROR : Could not save FITS file information.");
         rstatus = SH_FITS_HDR_SAVE_ERR;
      }
   }
   else
   {
      shErrStackPush("ERROR : Could not open FITS file : %s", a_fname);
      rstatus = SH_FITS_OPEN_ERR;
   };

return(rstatus);
}


/*============================================================================
**============================================================================
**
** ROUTINE: shMaskReadAsFits
**
** DESCRIPTION:
**      This routine reads in  a FITS file into a mask.
**
** RETURN VALUES:
**      SH_SUCCESS   -   Successful completion.
**      SH_...       -   Signifies errors
**
** CALLS TO:
**      shFileSpecExpand
**
** GLOBALS REFERENCED:
**
** NOTE:
**     The maskFilePtr arguement added Aug 95 - kurt
**
**============================================================================
*/
RET_CODE shMaskReadAsFits
    (
     MASK   *a_maskPtr,       /* IN: Pointer to mask */
     char   *a_file,          /* IN: Name of FITS file */
     DEFDIRENUM a_FITSdef,    /* IN: Directory/file extension default type */
     FILE   *maskFilePtr,     /* IN:  If != NULL then is a pipe to read from */
     int     a_readtape       /* IN:  Flag to indicate that we must fork/exec an
                                      instance of "dd" to read a tape drive */
    )
{
RET_CODE    rstatus;
char        *fname = 0;
char        *l_filePtr=0;
int         nrow, ncol;
int         i;
U8          **u8ptr;
char        *maskHeader[MAXHDRLINE+1];
char        *lcl_ddCmd = NULL;
int         tapeFd[2], /*tapeStderr,*/ ddSts;
pid_t       tapePid, rPid;
char        tapeBuf;

maskHeader[0] = (char *)0;      /* No mask header as of yet */

/*
 * Get the full expanded filename
 */
if (a_FITSdef == DEF_DEFAULT) {a_FITSdef = DEF_MASK_FILE;}

/* If a_readtape, we shouldn't have an open a_inPipePtr.  If so, complain
   and return.  If not, form a dd command and try a "dd" command. */
if (a_readtape)
{
   if (maskFilePtr != NULL)
   {
      rstatus = SH_TAPE_BAD_FILE_PTR;
      shErrStackPush("ERROR : maskFilePtr not NULL with a_readtape.");
      goto the_exit;
   }
   if ((lcl_ddCmd = (char*)shMalloc (strlen (a_file) + 4)) == ((char *)0))
   {
      rstatus = SH_MALLOC_ERR;
      shErrStackPush ("Unable to shMalloc space for dd command");
      goto the_exit;
   }
   sprintf(lcl_ddCmd, "if=%s", a_file);
   /* OK, here we go.  Open a pipe, fork a child, dup the child's stdout,
      exec in the child dd, associate parent's pipe end with a stream */
   if (pipe(tapeFd) < 0)
   {
      shErrStackPush(strerror(errno));
      rstatus = SH_TAPE_DD_FAIL;
      shErrStackPush("ERROR : could not create pipe for dd process.");
      shFree(lcl_ddCmd);
      goto the_exit;
   }
   if ( (tapePid = fork()) < 0)
   {
      shErrStackPush(strerror(errno));
      rstatus = SH_TAPE_DD_FAIL;
      shErrStackPush("ERROR : dd process could not be forked.");
      shFree(lcl_ddCmd);
      goto the_exit;
   }
   else if (tapePid > 0)         /* tapePid > 0 - parent process */
   {
      close (tapeFd[1]);         /* close the write end of the pipe */
      maskFilePtr = fdopen(tapeFd[0], "r");  /* attempt to get FILE* from fd */
      if (maskFilePtr == NULL)
      {
	 rstatus = SH_TAPE_DD_FAIL;
	 shErrStackPush("ERROR : could not open FILE * for dd process.");
	 shFree(lcl_ddCmd);
	 goto the_exit;
      }
      rstatus = SH_SUCCESS;
   }
   else                          /* tapePid == 0 - child process */
   {
      close (tapeFd[0]);         /* close the read end of the pipe */
      if (tapeFd[1] != STDOUT_FILENO)    /* dup the write end onto STDOUT */
      {
	 if (dup2(tapeFd[1],STDOUT_FILENO) != STDOUT_FILENO)
	 {
	    fprintf(stderr,"ERROR : couldn't dup2 in dd child.");
	    _exit(EXIT_FAILURE);
	 }
	 close(tapeFd[1]);      /* don't need this fd anymore */
      }
#if 0
      if ((tapeStderr=open("/dev/null",O_WRONLY)) > 0)
      {
	 dup2(tapeStderr, STDERR_FILENO);
	 close(tapeStderr);
      }
#endif
      execlp ("dd", "dd", lcl_ddCmd, "bs=144000", "files=1", (char *)0);  /* do the dd call */
      _exit(EXIT_FAILURE);       /* if exec fails return an error */
   }
}
   /* else if we were passed a pipe name then we do not have to do any file spec
      stuff. Also the pipe is already opened. */
else if (maskFilePtr == NULL)
{
     /* Get the full path name to the FITS file */
      rstatus = shFileSpecExpand(a_file, &l_filePtr, &fname, a_FITSdef);
}
else
{
      /* Use the passed in file descriptor as it is associated with a pipe
         and is already open */
      rstatus = SH_SUCCESS;
}
   
if (rstatus == SH_SUCCESS)
{
      /*
       * Open the file and get header information
       */
      if((rstatus =maskReadHdr(fname, &maskFilePtr, maskHeader)) != SH_SUCCESS)
      {
	 shErrStackPush("shMaskReadAsFits: error reading header from MASK file %s",
			fname);
      }
      else
      {
	 /*
	  * Get number of rows and columns from the header
	  */
	 if (!f_ikey(&nrow, maskHeader, "NAXIS1"))
	 {
	    /* Error - the NAXIS1 keyword is missing. */
	    rstatus = SH_NO_NAXIS1_KWRD;
	    shErrStackPush("shMaskReadAsFits: Invalid FITS File for Masks, no NAXIS1 keyword.\n");
	 }
	 else
	 {
	    if (!f_ikey(&ncol, maskHeader, "NAXIS2"))
	    {
	       /* Error - the NAXIS2 keyword is missing. */
	       rstatus = SH_NO_NAXIS2_KWRD;
	       shErrStackPush("shMaskReadAsFits: Invalid FITS File for Masks, no NAXIS2 keyword.\n");
	    }
	    else
	    {
	       /*
		* Now keep the existing mask structure.  However compare the
		* size of the current mask in memory with the mask in the FITS
		* file.  They must be exactly the same or we will delete the
		* mask storage and the mask vector array and malloc new ones.
		* Exactly the same means the two masks have the same number of
		* rows and the same number of columns.
		*/
	       if ((nrow != a_maskPtr->nrow) || (ncol != a_maskPtr->ncol))
	       {
		  /*
		   * The masks are not the same size.  Since we have to
		   * delete the mask, make sure it is not a sub mask or does
		   * not have sub masks.
		   */
		  if ((rstatus = p_shMaskTest(a_maskPtr)) == SH_SUCCESS)
		  {
		     /* 
		      * Free the old mask data and vectors and get new ones 
		      */
		     p_shMaskRowsFree(a_maskPtr);      /* Free mask storage */
		     p_shMaskVectorFree(a_maskPtr);    /* Free vector array */

		     /* 
		      * Now get a new mask vector array and data storage
		      */
		     p_shMaskVectorGet(a_maskPtr, nrow);
		     p_shMaskRowsGet(a_maskPtr, nrow, ncol);
		  }
	       }
               if (rstatus  == SH_SUCCESS)
               {
   	          /*
   		   * Now do actual work of reading in data from the FITS file.
   		   */
   	          u8ptr = (U8 **)a_maskPtr->rows;
   	          /* change ncol to nrow in the following line; Chris S. */
   	          for (i = 0; i < nrow; i++)
   		   if (!(f_get8((int8 *)u8ptr[i], a_maskPtr->ncol,
   			      maskFilePtr)))
   		   {
   		      shErrStackPush(
                         "shMaskReadAsFits: Unable to read pixels from FITS file %s",
   			 fname);
   		     rstatus = SH_FITS_PIX_READ_ERR;
   		     break;
   		   }
               }  
	    }
         }
      }
};

the_exit:
/*
 * Reallocate and free resources...
 */
if (maskHeader[0] != (char *)0)
   {
   for (i = 0; i < MASK_HDR_SIZE; i++)
      shFree(maskHeader[i]);
   }

if (l_filePtr != (char *)0)
   {shFree(l_filePtr);}
if (lcl_ddCmd != (char *)0)
   {shFree(lcl_ddCmd);}
if (fname != (char *)0)
   {shEnvfree(fname);}

if (maskFilePtr != (FILE *)0)
   {
   if (!a_readtape)
      {(void) f_fclose(maskFilePtr);}
   else 
      /* We have to handle the termination of our child process, dd.  If
         all went well, we've read to EOF from the pipe from dd's stdout.
         All we need to do is waitpid on dd.  If we're not at EOF, most
         likely the tape image has extra characters.  Report same as an
         error, issue a kill, and continue - waitpid will report a SIGKILL. */
      {
      fread(&tapeBuf,1,1,maskFilePtr);  /* attempt to read another byte */
      if (!feof(maskFilePtr))           /* should have EOF set if dd closed pipe */
	 {
	 shErrStackPush("ERROR : excess data available from dd.");
	 if (kill(tapePid, SIGKILL) != 0)   /* have to kill dd ourselves */
	    {
	    shErrStackPush(strerror(errno));
	    shErrStackPush("ERROR : attempt to kill dd failed.");
	    }
         }
      f_fclose(maskFilePtr);              /* close our end of the pipe */
      rPid = waitpid(tapePid, &ddSts, 0);
      if (rPid != tapePid)
	 {
	 if (!rPid)
	    {
	    rstatus = SH_TAPE_DD_ERR;
	    shErrStackPush("ERROR : dd did not exit, killing...");
	    if (kill(tapePid, SIGKILL) != 0)
	       {
	       shErrStackPush(strerror(errno));
	       shErrStackPush("ERROR : attempt to kill dd failed.");
	       }
	    else
	       {
	       shErrStackPush("ERROR : ... succeeded in killing dd.");
	       }
	    }
	 else
	    {
	    shErrStackPush(strerror(errno));
	    rstatus = SH_TAPE_DD_ERR;
	    shErrStackPush("ERROR : while waiting for dd to terminate.");
	    }
	 }   
      else if (WIFEXITED(ddSts))
	 {
	 if (WEXITSTATUS(ddSts))
	    {
	    rstatus = SH_TAPE_DD_ERR;
	    shErrStackPush("ERROR : dd return status %d.", WEXITSTATUS(ddSts));
	    }
         }
      else if (WIFSIGNALED(ddSts))
	 {
	 rstatus = SH_TAPE_DD_ERR;
	 shErrStackPush("ERROR : dd returned signal %d.", WTERMSIG(ddSts));
         }
      else if (WIFSTOPPED(ddSts))
	 {
	 rstatus = SH_TAPE_DD_ERR;
	 shErrStackPush("ERROR : dd stopped with signal %d, killing...",
			WSTOPSIG(ddSts));
	 if (kill(tapePid, SIGKILL) != 0)
	    {
	    shErrStackPush(strerror(errno));
	    shErrStackPush("ERROR : attempt to kill dd failed.");
	    }
	 else
	    {
	    shErrStackPush("ERROR : ... succeeded in killing dd.");
	    }
         }  
      }  /* end else wait on our child */
   }  /* end if f != NULL */


return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: shMaskWriteAsFits
**
** DESCRIPTION:
**      This routine writes a mask as a FITS file.
**
** RETURN VALUES:
**      SH_SUCCESS   -   Successful completion.
**      SH_...       -   Signifies errors
**
** CALLS TO:
**      shFileSpecExpand
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
RET_CODE shMaskWriteAsFits
    (
     MASK   *a_maskPtr,       /* IN: Pointer to mask */
     char   *a_file,          /* IN: Name of FITS file */
     DEFDIRENUM a_FITSdef,    /* IN: Directory/file extension default type */
     FILE   *maskFilePtr,     /* IN:  If != NULL then is a pipe to read from */
     int     a_writetape      /* IN:  Flag to indicate that we must fork/exec an
                                      instance of "dd" to write a tape drive */
    )
{
FILE        *f = NULL;
RET_CODE    rstatus;
char        *fname = 0;
char        *maskHeader[MAXHDRLINE+1];
char        *l_filePtr=0;
int         i, naxis[3];
U8          **u8ptr;
char        *lcl_ddCmd = NULL;
int         tapeFd[2], /*tapeStderr,*/ ddSts;
pid_t       tapePid, rPid;
/*char        tapeBuf;*/

maskHeader[0] = (char *)0;      /* No mask header as of yet */

   /*
   * If there is no data to write, quit right away
   */
if (a_maskPtr->nrow <= 0)  
{
   shErrStackPush("shMaskWriteAsFits: Number of rows = 0. No data to write.");
   return (SH_NO_MASK_DATA);
}
   
/*
 * Get the full expanded filename
 */
if (a_FITSdef == DEF_DEFAULT) {a_FITSdef = DEF_MASK_FILE;}

/* If a_writetape, we shouldn't have an open a_outPipePtr.  If so, complain
   and return.  If not, form a dd command and try a "dd" command. */
if (a_writetape)
{
   if (maskFilePtr != NULL)
   {
      rstatus = SH_TAPE_BAD_FILE_PTR;
      shErrStackPush("ERROR : maskFilePtr not NULL with a_writetape.");
      goto rtn_return;
   }
   if ((lcl_ddCmd = (char*)shMalloc (strlen (a_file) + 4)) == ((char *)0))
   {
      rstatus = SH_MALLOC_ERR;
      shErrStackPush ("Unable to shMalloc space for dd command");
      goto rtn_return;
   }
   sprintf(lcl_ddCmd, "of=%s", a_file);
   /* OK, here we go.  Open a pipe, fork a child, dup the child's stdin,
      exec in the child dd, associate parent's pipe end with a stream */
   if (pipe(tapeFd) < 0)
   {
      shErrStackPush(strerror(errno));
      rstatus = SH_TAPE_DD_FAIL;
      shErrStackPush("ERROR : could not create pipe for dd process.");
      shFree(lcl_ddCmd);
      goto rtn_return;
   }
   if ( (tapePid = fork()) < 0)
   {
      shErrStackPush(strerror(errno));
      rstatus = SH_TAPE_DD_FAIL;
      shErrStackPush("ERROR : dd process could not be forked.");
      shFree(lcl_ddCmd);
      goto rtn_return;
   }
   else if (tapePid > 0)         /* tapePid > 0 - parent process */
   {
      close (tapeFd[0]);         /* close the read end of the pipe */
      f = fdopen(tapeFd[1], "w");  /* attempt to get FILE* from fd */
      if (f == NULL)
      {
	 rstatus = SH_TAPE_DD_FAIL;
	 shErrStackPush("ERROR : could not open FILE * for dd process.");
	 shFree(lcl_ddCmd);
	 goto rtn_return;
      }
      rstatus = SH_SUCCESS;
   }
   else                          /* tapePid == 0 - child process */
   {
      close (tapeFd[1]);         /* close the write end of the pipe */
      if (tapeFd[0] != STDIN_FILENO)    /* dup the read end onto STDIN */
      {
	 if (dup2(tapeFd[0],STDIN_FILENO) != STDIN_FILENO)
	 {
	    fprintf(stderr,"ERROR : couldn't dup2 in dd child.");
	    _exit(EXIT_FAILURE);
	 }
	 close(tapeFd[0]);      /* don't need this fd anymore */
      }
#if 0
      if ((tapeStderr=open("/dev/null",O_WRONLY)) > 0)
      {
	 dup2(tapeStderr, STDERR_FILENO);
	 close(tapeStderr);
      }
#endif
      execlp ("dd", "dd", lcl_ddCmd, "bs=144000", "files=1", (char *)0);  /* do the dd call */
      _exit(EXIT_FAILURE);       /* if exec fails return an error */
   }
}

/* else if we were passed a pipe name then we do not have to do any file spec
   stuff. Also the pipe is already opened. */
else if (maskFilePtr == NULL)
{
      /* Get the full path name to the FITS file */
      rstatus = shFileSpecExpand(a_file, &l_filePtr, &fname, a_FITSdef);
}
else
{
      /* Use the passed in file descriptor as it is associated with a pipe
	 and is already open */
      f = maskFilePtr;
      rstatus = SH_SUCCESS;
};

if (rstatus == SH_SUCCESS)
{
   /* Create a new header */
   naxis[0] = 2;
   naxis[1] = a_maskPtr->ncol;
   naxis[2] = a_maskPtr->nrow;

   if (f_newhdr(maskHeader, MASK_HDR_SIZE, TRUE, 8, 2, naxis, "") == FALSE)
   {
      shErrStackPush("shMaskWriteAsFits: Cannot make a new mask header");
      rstatus = SH_CANT_MAKE_HDR;
   }
   else
   {
      if((rstatus =maskWriteHdr(fname, &f, maskHeader)) 
               != SH_SUCCESS)
      {
            shErrStackPush("shMaskWriteAsFits: cannot open FITS file %s for writing", 
   	         	fname);
      }
      else
      {
         /*
          * Now write out the data contained in a_maskPtr->rows
          */
         u8ptr = (U8 **)a_maskPtr->rows;
         for (i = 0; i < a_maskPtr->nrow; i++)  
         {
            if (!(f_put8((int8 *) u8ptr[i], a_maskPtr->ncol, f)))
            {
              shErrStackPush("shMaskWriteAsFits: Unable to write mask pixels to FITS file %s",
	   	             fname);
              rstatus = SH_FITS_PIX_WRITE_ERR;
              break;
            }
         }
      }
   }
};

rtn_return:

if (f == maskFilePtr) {			/* we don't own this file pointer */
   (void)f_lose(f);			/* but we registered it with libfits */
   f = NULL;
}

/*
 * Reallocate and free resources...
 */
if (maskHeader[0] != (char *)0)
{
   for (i = 0; i < MASK_HDR_SIZE; i++) shFree(maskHeader[i]);
}

if (l_filePtr != (char *)0)
   {shFree(l_filePtr);}
if (lcl_ddCmd != (char *)0)
   {shFree(lcl_ddCmd);}
if (fname != (char *)0)
   {shEnvfree(fname);}

if (f != (FILE *)0)
   {
   if (!a_writetape)
      {(void) f_fclose(f);}
   else
      /* We have to handle the termination of our child process, dd.  We
         close the write end of our pipe, which should send an EOF to dd.
	 All we need to do is waitpid on dd. */
      {
      f_fclose(f);                 /* close our end of the pipe */
      rPid = waitpid(tapePid, &ddSts, 0);
      if (rPid != tapePid)
	 {
	 if (!rPid)
	    {
	    rstatus = SH_TAPE_DD_ERR;
	    shErrStackPush("ERROR : dd did not exit, killing...");
	    if (kill(tapePid, SIGKILL) != 0)
	       {
	       shErrStackPush(strerror(errno));
	       shErrStackPush("ERROR : attempt to kill dd failed.");
	       }
	    else
	       {
	       shErrStackPush("ERROR : ... succeeded in killing dd.");
	       }
	    }
	 else
	    {
	    shErrStackPush(strerror(errno));
	    rstatus = SH_TAPE_DD_ERR;
	    shErrStackPush("ERROR : while waiting for dd to terminate.");
	    }
	 }   
      else if (WIFEXITED(ddSts))
	 {
	 if (WEXITSTATUS(ddSts))
	    {
	    rstatus = SH_TAPE_DD_ERR;
	    shErrStackPush("ERROR : dd return status %d.", WEXITSTATUS(ddSts));
	    }
         }
      else if (WIFSIGNALED(ddSts))
	 {
	 rstatus = SH_TAPE_DD_ERR;
	 shErrStackPush("ERROR : dd returned signal %d.", WTERMSIG(ddSts));
         }
      else if (WIFSTOPPED(ddSts))
	 {
	 rstatus = SH_TAPE_DD_ERR;
	 shErrStackPush("ERROR : dd stopped with signal %d, killing...",
			WSTOPSIG(ddSts));
	 if (kill(tapePid, SIGKILL) != 0)
	    {
	    shErrStackPush(strerror(errno));
	    shErrStackPush("ERROR : attempt to kill dd failed.");
	    }
	 else
	    {
	    shErrStackPush("ERROR : ... succeeded in killing dd.");
	    }
         }  
      }  /* end else wait on our child */
   }  /* end if f != NULL */
      
return(rstatus);
}

