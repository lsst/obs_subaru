#ifndef SHTCLSAO_H
#define SHTCLSAO_H
/*****************************************************************************
******************************************************************************
**
** FILE:
**	shTclSao.h
**
** ABSTRACT:
**	This file contains all necessary Tcl definitions, prototypes, and
**	macros for the routines used to simultaneously support command line
**	input (stdin) and communications with Fermi modified SAOimage programs.
**
** REQUIRED INCLUDE FILES:
**	"tcl.h"		    -	Tcl_Interp definition
**	"ftclCmdLine.h"	    -	Fermi TCL command line definitions
**	"imtool.h"
**	"region.h"
**
** ENVIRONMENT:
**	ANSI C.
**
** AUTHOR:      Creation date: Jul. 1, 1994
**              Eileen Berman
**
******************************************************************************
******************************************************************************
*/
#include <sys/types.h>
#include "tcl.h"
#include "ftclCmdLine.h"
#include "imtool.h"
#include "region.h"		/* For PIXDATATYPE */
#include "shCSao.h"

/*----------------------------------------------------------------------------
**
** DEFINITIONS
*/
#define TCL_BUTT_FUNC_NAME_LEN 30 /* Length of TCL function names assigned to
                                     SAOimage buttons */
#define CMD_PROMPT_LEN	20	/* Command line prompt length */
#define ANYFSAO -1             /* wait for mouse input from any fsao */

/* Different versions of UNIX store the maximum file number in different
 * places. 
 */

#ifndef OPEN_MAX
#ifdef NOFILE
#define OPEN_MAX NOFILE
#endif
#endif

#ifndef OPEN_MAX
#ifdef _NFILE
#define OPEN_MAX _NFILE
#endif
#endif

#ifndef OPEN_MAX
#ifdef _POSIX_OPEN_MAX
#define OPEN_MAX _POSIX_OPEN_MAX
#endif
#endif

/*----------------------------------------------------------------------------
**
** COMMAND EDIT HISTORY STRUCTURE
*/
typedef struct cmd_handle
   {
   int		    lineEntered;
   struct cmd_edithndl lineHandle;
   CMD_EDITHIST     history;
   Tcl_Interp       *interp;                  /* TCL interp structure pointer */
   char             *line;                    /* Entered command line */
   char             prompt[CMD_PROMPT_LEN+1]; /* User prompt */
   Tcl_DString      TclBuf;                   /* TCL command buffer */
   } CMD_HANDLE;

typedef CMD_HANDLE SHMAIN_HANDLE;	/* For backwards compatibility */

/*----------------------------------------------------------------------------
**
** FUNCTION PROTOTYPES
*/
#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

RET_CODE shSaoWriteToPipe(int, char *, int);
RET_CODE shSaoSendImMsg(struct imtoolRec *, int);
RET_CODE shSaoSendFitsMsg(CMD_SAOHANDLE *, char **, char **, PIXDATATYPE, 
                          double, double, int, int, char *);
RET_CODE shSaoPipeOpen(CMD_SAOHANDLE *);
void shSaoPipeClose(CMD_SAOHANDLE *);
void shSaoCleanUp(void);
RET_CODE tclSaoInit(Tcl_Interp *);
void shSaoClearMem(CMD_SAOHANDLE *);
int p_shSaoMouseKeyMap(CMD_SAOHANDLE *, int, char *);
int p_shSaoMouseCheckWait(int saoNum);
int p_shSaoMouseWaitGet(void);
RET_CODE p_shSaoMouseKeyGet(CMD_SAOHANDLE  *);
void p_shSaoRegionSet(CMD_SAOHANDLE *, char *);
RET_CODE shMainTcl_Init(SHMAIN_HANDLE *, int *, char **, char *);
void shMainTcl_PromptReset(CMD_HANDLE *, char *);
void p_shSaoImageCleanFunc(void);
RET_CODE p_shSaoIndexCheck(int *);
RET_CODE p_shSaoSendMaskMsg(CMD_SAOHANDLE *a_shl, char a_maskLUT[LUTTOP],
			char *a_mask, int a_nrows, int a_ncols,
			char *a_colorTable);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif




