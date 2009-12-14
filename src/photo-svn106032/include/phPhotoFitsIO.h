#if !defined(PHPHOTOFITSIO_H)
#define PHPHOTOFITSIO_H

#include <sys/types.h>
#include <unistd.h>
#include "dervish.h"

typedef struct {
   TYPE type;				/* type of data being written */
   int mode;				/* mode file was opened in */
   int fd;				/* file's unix file descriptor */
   int heapfd;				/* unix file descriptor for heap */
   long old_fd_pos, old_heapfd_pos;	/* old seek pointers; really off_t */
   long hdr_start;			/* offset of header block (really an
					   off_t, but that confuses dervish) */
   long hdr_end;			/* offset of end of header (another
					   off_t) */
   long heap_start;			/* offset of start of heap */
   long heap_end;			/* offset of end of heap */
   char *row;				/* The buffer used to read and write
					   the main table */
   int rowlen;				/* number of bytes in a row */
   int nrow;				/* number of rows in table */
   struct phfitsb_machine **machine;	/* binary table I/O FSM;
					   see photoFitsIO.c */
   struct phfitsb_machine **omachine;	/* optimised binary table I/O FSM;
					   see photoFitsIO.c */
   int start_ind;			/* which machine to start with */
   void **stack;			/* stack for machine */
   int stacksize;			/* size of stack */
   int *istack;				/* integer stack for machine */
   int istacksize;			/* size of istack */
} PHFITSFILE;				/* pragma SCHEMA */

int phFitsBinDeclareSchemaTrans(SCHEMATRANS *trans, char *type);
void phFitsBinForgetSchemaTrans(char *type);
int phFitsBinSchemaTransIsDefined(const char *type);

int phFitsBinTblMachineTrace(char *type, int exec,  PHFITSFILE *fil);

PHFITSFILE *phFitsBinTblOpen(char *file, int flag, HDR *hdr);
int phFitsBinTblHdrRead(PHFITSFILE *fd, char *type, HDR *hdr,
			int *nrow, int quiet);
int phFitsBinTblHdrWrite(PHFITSFILE *fd, char *type, HDR *hdr);

int phFitsBinTblRowSeek(PHFITSFILE *fil, int n, int whence);
int phFitsBinTblRowRead(PHFITSFILE *fil, void *data);
int phFitsBinTblRowUnread(PHFITSFILE *fil);
int phFitsBinTblRowWrite(PHFITSFILE *fil, void *data);
int phFitsBinTblChainWrite(PHFITSFILE *fil, CHAIN *chain);

int phFitsBinTblEnd(PHFITSFILE *fd);
int phFitsBinTblClose(PHFITSFILE *fil);

int p_phFitsBinMachineSet(struct phfitsb_machine *mac);

#endif
