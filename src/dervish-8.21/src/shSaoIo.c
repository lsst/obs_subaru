/*
 * FILE:
 *   shSaoIo.c
 *
 * ABSTRACT:
 *   This file contains routines that perform I/O between DERVISH and SAO
 *   images.
 *
 * ENTRY POINT            SCOPE    DESCRIPTION
 * ----------------------------------------------------------------------------
 * shSaoWriteToPipe       private  Write data to pipe connected to SAO image
 * shSaoSendImMsg         private  Send IMTOOL message to SAO image
 * shSaoSendFitsMsg       private  Send FITS file data to SAO image
 * p_shSaoSendMaskMsg     private  Send mask data to SAO image
 * p_shSaoSendMaskGlyphMsg private Send mask glyph request to SAO image
 * shSaoPipeOpen          private  Open a pipe to SAO image
 * shSaoPipeClose         private  Close a pipe to SAO image
 * shSaoCleanUp           private  Clean up pipe descriptors
 * shSaoClearMem          public   Zero out structures being used
 *
 * AUTHOR:
 *   Vijay K. Gurbani, July 8, 1993.
 *
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>   /* Prototypes for write() and friends */
#include <memory.h>   /* Prototypes for mem*() routines */
#include <string.h>   /* needed for strlen prototype */

#include "dervish_msg_c.h"
#include "region.h"
#include "tcl.h"
#include "shCSao.h"
#include "shCUtils.h"    /* Prototype for shFatal */ 

/* External variable s referenced here */
extern SAOCMDHANDLE g_saoCmdHandle;


/*
 * ROUTINE: 
 *   shSaoWriteToPipe
 *
 * CALL:
 *   (RET_CODE) shSaoWriteToPipe(
 *                      int  a_wfd,	 // IN:  File descriptor to write to
 *                      char *a_bufptr,  // IN:  Buffer to write
 *                      int  a_nbytes    // IN:  Number of bytes to write 
 *                   );   
 *
 * DESCRIPTION:
 *	Write a buffer to the specified file descriptor.
 *
 * RETURN VALUES:
 *	SH_SUCCESS
 *	SH_PIPE_WRITE_MAX
 *	dervish_pipeWriteErr
 */
RET_CODE
shSaoWriteToPipe(int a_wfd, char *a_bufptr, int a_nbytes)
{
#define	    L_MAX_WRITE_TRY   10    /* Maximum number of writes per buffer */

   RET_CODE  rstatus;
   int	     i;
   int	     zerocnt;
   int	     bytes_to_send;
   int	     bytes_xferred = 0;

   i = 0;
   zerocnt = 0;
   bytes_to_send = a_nbytes;
   while(bytes_to_send > 0)  {
      bytes_xferred = (int )write(a_wfd,
                      (char *)&a_bufptr[(a_nbytes - bytes_to_send)],
                      (unsigned int )bytes_to_send);

      if (bytes_xferred > (int )0)  {
         zerocnt = 0;
         bytes_to_send -= bytes_xferred;
      }
      else if (bytes_xferred == (int )0)  {
         zerocnt++;
         if (zerocnt > 3) {break;}
      }
      else
         break;

      i++;
      if (i >= L_MAX_WRITE_TRY)  {
          rstatus = SH_PIPE_WRITE_MAX; 
          goto err_exit;
      }
   }

   /* Check for I/O errors which may have occurred when sending */
   if (bytes_xferred <= (int )0)  {
       rstatus = SH_PIPE_WRITEERR; 
       goto err_exit;
   }

   return(SH_SUCCESS);

   err_exit:
   return(rstatus);
}

/*
 * ROUTINE: 
 *   shSaoSendImMsg
 *
 * CALL:
 *   (RET_CODE) shSaoSendImMsg(
 *                      struct imtoolRec *a_im, // MOD: Imtool structure
 *                      int    a_wfd            // IN: write file descriptor
 *                   );
 *
 * DESCRIPTION:
 *	Routine to write imtool message packet to SAOimage.
 *
 * RETURN VALUES:
 *	SH_SUCCESS
 *	status from shSaoWriteToPipe
 */
RET_CODE
shSaoSendImMsg(struct imtoolRec *a_im, int a_wfd)
{
   RET_CODE	    rstatus;
   register short	    *chk;
   int		    chksum, i;

   /* Zero the checksum first */
   a_im->checksum = 0;

   /* Calculate checksum */
   chk    = (short *)a_im;
   chksum = 0;
   for (i=0; i < 8; i++) 
        chksum += *chk++;
   a_im->checksum = (short)0xffff - (short)chksum;

   /*
   ** SEND COMMAND STRUCTURE TO SAOIMAGE 
   */
   rstatus = shSaoWriteToPipe(a_wfd, (char *)a_im, sizeof(struct imtoolRec));
   if (rstatus != SH_SUCCESS) 
       goto err_exit;

   return(SH_SUCCESS);

   err_exit:
   return(rstatus);
}

/*
 * ROUTINE: 
 *   saoWriteData
 *
 * CALL:
 *   (RET_CODE) saoWriteData(
 *           CMD_SAOHANDLE *a_shl,      // IN: Cmd SAO handle (internal struct*)
 *           char          **a_data,	// IN: data vector (cast to char*)
 *           int           a_pixelSize, // IN: Pixel data size
 *           int           a_nrows,	// IN: Number of rows in data
 *           int           a_ncols	// IN: Number of columns in data
 *         );   
 *
 * DESCRIPTION:
 *	Loop and fill the buffer with the data and send it off to fsao.
 *
 * RETURN VALUES:
 *	SH_SUCCESS
 *	status from shSaoWriteToPipe
 *
 */
RET_CODE saoWriteData
   (
   CMD_SAOHANDLE *a_shl,
   char          **a_data,
   int           a_pixelSize,
   int           a_nrows,
   int           a_ncols
   )
{
char		    *bufptr;
char		    *dataPtr;
char		    xferBuf[SZ_FIFOBUF];
int		    spaceLeft, dataWidth, curRow, rowBytesLeft;
RET_CODE	    rstatus = SH_GENERIC_ERROR;

dataWidth     = a_ncols * a_pixelSize;
rowBytesLeft  = dataWidth;
curRow        = 0;
dataPtr       = a_data[curRow];

while ((curRow < a_nrows) && (a_data[curRow] != NULL))
   {
   bufptr    = xferBuf;
   spaceLeft = SZ_FIFOBUF;

   /*
   ** FILL LOCAL XFER BUFFER WITH AS MUCH OF THE PIXEL IMAGE DATA AS POSSIBLE
   */
   while ((curRow < a_nrows) && (a_data[curRow] != NULL))
      {
      /* Is there enough room for the rest of the current row ? */
      if (spaceLeft >= rowBytesLeft)
         {
         memcpy(bufptr, dataPtr, rowBytesLeft);	/* Copy rest of current row */
         bufptr += rowBytesLeft;	/* Update pointer in xfer buffer */
         spaceLeft -= rowBytesLeft;	/* Update space left in xfer buffer */
         curRow++;			/* Update row index to next row */
         rowBytesLeft = dataWidth;	/* Entire next row width remains */
         dataPtr = a_data[curRow];	/* Point to next row */
         }

      /* Otherwise, there's not enough room for the rest of the current row */
      else
         {
         memcpy(bufptr, dataPtr, spaceLeft); /* Copy to fill xfer buffer */
         bufptr += spaceLeft;		/* Update pointer in xfer buffer */
         rowBytesLeft -= spaceLeft;	/* Update bytes left in row */
         spaceLeft = 0;			/* No space left in xfer buffer */
         /* Point to next byte in current row */
         dataPtr = &a_data[curRow][dataWidth - rowBytesLeft];
         }

      /* If buffer is full, go send it to FSAOimage */
      if (!spaceLeft)
        {break;}
      }

   /*
   ** NOW SEND THE XFER BUFFER TO SAOIMAGE
   */
   rstatus = shSaoWriteToPipe(a_shl->wfd, xferBuf, (bufptr - xferBuf));
   if (rstatus != SH_SUCCESS)
      {break;}

   }

return(rstatus);
}

/*
 * ROUTINE: 
 *   shSaoSendFitsMsg
 *
 * CALL:
 *   (RET_CODE) shSaoSendFitsMsg(
 *           CMD_SAOHANDLE *a_shl,      // IN: Cmd SAO handle (internal struct*)
 *           char	   **a_header,  // IN: Image header vector
 *           char          **a_image,	// IN: Image data vector (cast to char*)
 *           PIXDATATYPE   a_pixelType, // IN: Pixel data type
 *           double        a_bscale,	// IN: # to use when transforming pixels
 *           double        a_bzero,	// IN: # to use when transforming pixels
 *           int           a_nrows,	// IN: Number of rows in image
 *           int           a_ncols	// IN: Number of columns in image
 *         );   
 *
 * DESCRIPTION:
 *	Routine to write new image to SAOimage.
 *
 * RETURN VALUES:
 *	SH_SUCCESS
 *	dervish_SAOSyncErr
 *	status from shSaoWriteToPipe
 *	status from shSaoSendImMsg
 *
 */
RET_CODE
shSaoSendFitsMsg(CMD_SAOHANDLE *a_shl, char **a_header,
                 char **a_image, PIXDATATYPE  a_pixelType,
                 double a_bscale, double a_bzero,  int a_nrows, int a_ncols,
		 char *cmdOpt)
{
RET_CODE	    rstatus;
struct imtoolRec    im;		/* Command structure sent to SAOimage */
int		    i, bytes, nleft;
char		    *bufptr;
char		    headbuf[(80*156)+1]; /*???*/
int		    hdrbytes;
int		    pixelSize = 0;
union {
   int	    val;
   char	    chr[sizeof(int)];
}		    cmdOptLen;

/*
** CONVERT REGION HEADER TO A CONTIGUOUS BYTE STREAM
*/
headbuf[0] = (char)0;
hdrbytes   = 0;
for (i=0; a_header[i] != NULL; i++)
   {
   memcpy((void *)(headbuf + hdrbytes), (const void *)a_header[i], 80);
   hdrbytes += 80;
   }
hdrbytes = ( ((int)(hdrbytes-1)/CMD_FITS_HDRLEN) + 1) * CMD_FITS_HDRLEN;

/*
** TELL SAOIMAGE WE'RE GOING TO SEND A NEW IMAGE.
*/
/* Setup command structure that is recognized by SAOimage.  SAOimage expects
   commands ala iraf's imtool. */
im.tid      = 0;		/* Indicate this is a 16 bit image */
im.subunit  = MEMORY;
im.z        = 1;
im.checksum = 0;
im.t	    = 123;		/* ??? My kluge */
im.x        = 0;
im.y        = 0;
im.thingct  = -(hdrbytes);

/* Send if off to SAOimage */
rstatus = shSaoSendImMsg(&im, a_shl->wfd);
if (rstatus != SH_SUCCESS)
  {goto exit;}

   /*
    * In the first cut of Dervish, fsao was modified (?) to write something
    * on the pipe, which was read by Dervish. This was done primarily as a
    * synchronization mechanism. With the advent of Tk into Dervish, this is
    * no longer needed, since Tk now controls the pipe and will notify
    * Dervish as soon as there is some I/O activity on the pipe. However, to
    * make Dervish run under Tk, I had to put a forced while loop below which
    * reads from the pipe and if no data exists, keeps on reading till 
    * something comes from the pipe.
    *
    * Now a question arises that since if Tk is controlling the pipe, how
    * come it does not automatically inform Dervish when there is some I/O
    * on the pipe? After all, that is the whole idea of the Tk call
    * Tk_CreateFileHandler().
    * The answer is that it does, but the callback procedure that gets
    * invoked is SaoImageProc, but we want the data here, in this function;
    * hence the while loop.
    *
    * In the long run it will be a good idea to scrape this synchronization
    * mechanism completely. It worked okay for the first cut of Dervish, but
    * now with the advent of Tk it's more of a hassle.
    *
    * - vijay
    */
   while (1)  {
       bytes = read(a_shl->rfd, (char *)&im, sizeof(struct imtoolRec));
       if (bytes == -1 && errno == EAGAIN)
           continue;
       if (bytes != sizeof(struct imtoolRec))  {
           rstatus = SH_SAO_SYNC_ERR;
           goto exit;
       }
       else
           break;
   }

/*
** SEND ANY SAOIMAGE COMMAND LINE OPTIONS.
*/

cmdOptLen.val = (cmdOpt == ((char *)0)) ? 0 : strlen (cmdOpt);

if ((rstatus = shSaoWriteToPipe(a_shl->wfd, cmdOptLen.chr, sizeof (cmdOptLen.val))) != SH_SUCCESS) { goto exit; }

if (cmdOptLen.val != 0) {
   if ((rstatus = shSaoWriteToPipe(a_shl->wfd, cmdOpt, cmdOptLen.val)) != SH_SUCCESS) { goto exit; }
}

/*
** SEND THE FITS HEADER FIRST TO SAOIMAGE
*/
bytes = CMD_FITS_HDRLEN;
bufptr = headbuf;
for (nleft = hdrbytes; nleft > 0; nleft -= bytes)
   {
   if (nleft < CMD_FITS_HDRLEN)
      {bytes = nleft;}
   rstatus = shSaoWriteToPipe(a_shl->wfd, bufptr, bytes);
   if (rstatus != SH_SUCCESS)
      {goto exit;}
   bufptr += bytes;
   }

/*????*/

/*
 * Find out what's the size of the data contained in the FITS file
 */
switch(a_pixelType)  {
   case TYPE_U8   :
        pixelSize = sizeof(U8); break;
   case TYPE_S8   :
        pixelSize = sizeof(S8); break;
   case TYPE_U16  :
        pixelSize = sizeof(U16); break;
   case TYPE_S16  :
        pixelSize = sizeof(S16); break;
   case TYPE_U32  :
        pixelSize = sizeof(U32); break;
   case TYPE_S32  :
        pixelSize = sizeof(S32); break;
   case TYPE_FL32 :
        pixelSize = sizeof(FL32); break;
   default:
        /* We need a default statement since certain pixel types are
           not allowed by this function */
        shFatal("shSaoSendFitsMsg: pixel type %d is not handled", (int)a_pixelType);
        break;
}

/*
** LOOP AND SEND THE ENTIRE PIXEL IMAGE TO FSAOIMAGE
*/
rstatus = saoWriteData(a_shl, a_image, pixelSize, a_nrows, a_ncols);


exit:
return(rstatus);
}

/*
 * ROUTINE: 
 *   p_shSaoSendMaskMsg
 *
 * CALL:
 *   (RET_CODE) p_shSaoSendMaskMsg(
 *           CMD_SAOHANDLE *a_shl,       // IN: Cmd SAO handle (intrnl struct*)
 *           char          a_maskLUT[LUTTOP] // IN: Mask lookup table
 *           char          **a_mask,     // IN: Mask data vector (cast to char*)
 *           int           a_nrows,      // IN: Number of rows in image
 *           int           a_ncols,      // IN: Number of columns in image
 *           char          *a_colorTable // IN: List of colors to use for 
 *                                              displaying the mask
 *         );   
 *
 * DESCRIPTION:
 *	Routine to write mask data to fSAOimage.
 *
 * RETURN VALUES:
 *	SH_SUCCESS
 *	dervish_SAOSyncErr
 *	status from shSaoWriteToPipe
 *	status from shSaoSendImMsg
 *
 */
RET_CODE
p_shSaoSendMaskMsg(CMD_SAOHANDLE *a_shl,
                   char a_maskLUT[LUTTOP],
                   char *a_mask,
                   int a_nrows,
                   int a_ncols,
                   char *a_colorTable)   
{
RET_CODE	    rstatus;
struct imtoolRec    im;		/* Command structure sent to SAOimage */
int                 bytes;
int                 dataTot;

/*
** TELL SAOIMAGE WE'RE GOING TO SEND A MASK.
*/
/* Setup command structure that is recognized by SAOimage.  SAOimage expects
   commands ala iraf's imtool. */
im.tid      = 0;
im.subunit  = MASKDISP;
im.z        = 1;
im.checksum = 0;
im.t	    = 123;		/* ??? My kluge */
im.x        = a_ncols;		/* num of columns in the mask */
im.y        = a_nrows;		/* num of rows in the mask */
im.thingct  = -(strlen(a_colorTable));

/* Send if off to SAOimage */
rstatus = shSaoSendImMsg(&im, a_shl->wfd);
if (rstatus != SH_SUCCESS)
  {goto exit;}

   /*
    * Please see comments in function shSaoSendFitsMsg() before a similar
    * block of code.
    * - vijay
    */
   while (1)  {
       bytes = read(a_shl->rfd, (char *)&im, sizeof(struct imtoolRec));
       if (bytes == -1 && errno == EAGAIN)
           continue;
       if (bytes != sizeof(struct imtoolRec))  {
           rstatus = SH_SAO_SYNC_ERR;
           goto exit;
       }
       else
           break;
   }

/*
** SEND THE LIST OF COLORS TO BE USED WHEN DISPLAYING THE MASK
*/
rstatus = shSaoWriteToPipe(a_shl->wfd, a_colorTable, strlen(a_colorTable));

/*
** LOOP AND SEND THE LOOKUP TABLE TO FSAOIMAGE
*/

dataTot = LUTTOP;		/* Total number of bytes to send */

while (dataTot > 0)		/* while thre is still data left ... */
   {
   if (dataTot > SZ_FIFOBUF)
      {
      /* We have more data than can be sent this time.  so send the 
         maximum we can at a time. */
      if ((rstatus = shSaoWriteToPipe(a_shl->wfd, a_maskLUT, SZ_FIFOBUF))
          != SH_SUCCESS)
         {break;}
      a_maskLUT += (int )SZ_FIFOBUF;	/* point to the next non-sent byte */
      dataTot -= (int )SZ_FIFOBUF;	/* that many bytes less to send */
      }
   else
      {
      /* Send all the rest of the data that we have */
      if ((rstatus = shSaoWriteToPipe(a_shl->wfd, a_maskLUT, dataTot))
          != SH_SUCCESS)
         {break;}
      dataTot = 0;		/* we are done */
      }
   }

/*
** LOOP AND SEND THE ENTIRE MASK TO FSAOIMAGE
*/

dataTot = a_ncols * a_nrows;	/* Total number of bytes to send */

while (dataTot > 0)		/* while there is still data left ... */
   {
   if (dataTot > SZ_FIFOBUF)
      {
      /* We have more data than can be sent this time.  so send the 
         maximum we can at a time. */
      if ((rstatus = shSaoWriteToPipe(a_shl->wfd, a_mask, SZ_FIFOBUF))
          != SH_SUCCESS)
         {break;}
      a_mask += (int )SZ_FIFOBUF;	/* point to the next non-sent byte */
      dataTot -= (int )SZ_FIFOBUF;	/* that many bytes less to send */
      }
   else
      {
      /* Send all the rest of the data that we have */
      if ((rstatus = shSaoWriteToPipe(a_shl->wfd, a_mask, dataTot))
          != SH_SUCCESS)
         {break;}
      dataTot = 0;		/* we are done */
      }
   }

exit:
return(rstatus);
}

/*
 * ROUTINE: 
 *   p_shSaoSendMaskGlyphMsg
 *
 * CALL:
 *   (RET_CODE) p_shSaoSendMaskMsg(
 *           CMD_SAOHANDLE *a_shl,       // IN: Cmd SAO handle (intrnl struct*)
 *           int           a_glyph       // IN: Glyph for zoomed masked pixels
 *         );   
 *
 * DESCRIPTION:
 *	Routine to write mask glyph to fSAOimage.
 *
 * RETURN VALUES:
 *	SH_SUCCESS
 *	dervish_SAOSyncErr
 *	status from shSaoWriteToPipe
 *	status from shSaoSendImMsg
 *
 */
RET_CODE
p_shSaoSendMaskGlyphMsg(CMD_SAOHANDLE *a_shl,
                   int a_glyph)
{
RET_CODE	    rstatus;
struct imtoolRec    im;		/* Command structure sent to SAOimage */
int                 bytes;

/*
** TELL SAOIMAGE WE'RE GOING TO SEND A MASK.
*/
/* Setup command structure that is recognized by SAOimage.  SAOimage expects
   commands ala iraf's imtool. */
im.tid      = 0;
im.subunit  = MASKGLYPH;
im.z        = 1;
im.checksum = 0;
im.t	    = 123;		/* ??? My kluge */
im.x        = 0;
im.y        = 0;
im.thingct  = -sizeof (a_glyph);

/* Send if off to SAOimage */
rstatus = shSaoSendImMsg(&im, a_shl->wfd);
if (rstatus != SH_SUCCESS)
  {goto exit;}

   /*
    * Please see comments in function shSaoSendFitsMsg() before a similar
    * block of code.
    * - vijay
    */
   while (1)  {
       bytes = read(a_shl->rfd, (char *)&im, sizeof(struct imtoolRec));
       if (bytes == -1 && errno == EAGAIN)
           continue;
       if (bytes != sizeof(struct imtoolRec))  {
           rstatus = SH_SAO_SYNC_ERR;
           goto exit;
       }
       else
           break;
   }

/*
** SEND THE LIST OF COLORS TO BE USED WHEN DISPLAYING THE MASK
*/
rstatus = shSaoWriteToPipe(a_shl->wfd, (char *)&a_glyph, sizeof(a_glyph));

exit:
return(rstatus);
}

/*
 * ROUTINE: shSaoPipeOpen
 *
 * CALL:
 * (unsigned long) shSaoPipeOpen(
 *                    CMD_SAOHANDLE *a_shl   // MOD: Cmd SAO handle (internal
 *                 )                         //      structure ptr)
 *
 * DESCRIPTION:
 *      Open the pipes used for SAOimage communications.
 *
 * RETURN VALUES:
 *	SH_SUCCESS
 *	dervish_pipeOpenErr
 *
 */
RET_CODE
shSaoPipeOpen(CMD_SAOHANDLE *a_shl)
{
   RET_CODE	rstatus;
   int		fdin[2], fdout[2];

   /*
    * Open a communication path to saoimage (via pair of pipes).
    */
   if (pipe(fdin))  {
       rstatus = SH_PIPE_OPENERR; 
       goto err_exit;
   }
   if (pipe(fdout))  {
       rstatus = SH_PIPE_OPENERR;
       goto err_exit;
   }

   a_shl->rfd = fdin[0];
   a_shl->crfd = fdin[1];
   a_shl->wfd = fdout[1];
   a_shl->cwfd = fdout[0];

   /*
    * Make these pipes non-blocking in nature. Tk needs them to be non-
    * blocking.   -vijay
    */
   if (fcntl(a_shl->rfd, F_SETFL, O_NONBLOCK) == -1)  {
       fprintf(stdout, "shSaoPipeOpen(): fcntl returns -1\n");
       return SH_GENERIC_ERROR;
   }

   return(SH_SUCCESS);

   err_exit:
   return(rstatus);
}

/*
 * ROUTINE: 
 *   shSaoPipeClose
 *
 * CALL:
 *   (void) shSaoPipeClose(
 *             CMD_SAOHANDLE *a_shl   // MOD: Cmd SAO handle (internal 
 *          )                         //      structure ptr)
 *
 * DESCRIPTION
 *      Close any connection to SAOimage.
 *
 * RETURN VALUES:
 *      None.
 */
void 
shSaoPipeClose(CMD_SAOHANDLE *a_shl)
{
   /*
    *  Close any open pipes to saoimage
    */
   if (a_shl->rfd > 0)  {
      close(a_shl->rfd);
      a_shl->rfd = 0;
   }
 
   if (a_shl->wfd > 0)  {
      close(a_shl->wfd);
      a_shl->wfd = 0;
   }
}

/*
 * ROUTINE: 
 *   shSaoCleanUp     
 * 
 * CALL:
 *   (void) shSaoCleanUp()
 *
 * DESCRIPTION:
 *	Close all connections (SAOimage and stdin) and cleanup.
 *
 * RETURN VALUES:
 *	None.
 *
 * CALLED BY:
 *	User's application.
 *
 */
void shSaoCleanUp(void)
{
   int i;

   /*
   ** FIRST MAKE SURE THE COMMAND STRUCTURE HAS BEEN USED
   */
   if ((g_saoCmdHandle.initflag == (unsigned int)CMD_INIT) ||
       (g_saoCmdHandle.initflag == (unsigned int)CMD_INIT_COMP))
   {
      /*
       * Close any pipes to saoimage
       */
      for (i=0; i < CMD_SAO_MAX; i++)  {
         if (&(g_saoCmdHandle.saoHandle[i]))  {
            if (g_saoCmdHandle.saoHandle[i].state == SAO_INUSE)  {
               shSaoPipeClose(&(g_saoCmdHandle.saoHandle[i]));
               shSaoClearMem(&(g_saoCmdHandle.saoHandle[i]));  /* Clean up */
            }
         }
      }

      /*
       * Indicate initialization needs to be done in case user tries to continue
       */
      g_saoCmdHandle.initflag = 0;
   }
}


/*
 * ROUTINE: 
 *   shSaoClearMem
 *
 * CALL:
 *   (void) shSaoClearMem(
 *             CMD_SAOHANDLE *a_shl   // MOD: Cmd SAO handle (internal 
 *          );                        //      structure ptr)
 *
 * DESCRIPTION:
 *      Clear the SAOimage handle structure.
 *
 * RETURN VALUES:
 *      None.
 *
 */
void 
shSaoClearMem(CMD_SAOHANDLE *a_shl)
{
   /* Zero the structure */
   memset((VOID *)a_shl, 0, sizeof(CMD_SAOHANDLE));
 
   /* 
    * Set state to indicate handle is unused.  NOTE: We don't really have to
    * do this since SAO_UNUSED is zero, but we're doing anyhow - for clarity.
    */
   a_shl->state = SAO_UNUSED;
}
