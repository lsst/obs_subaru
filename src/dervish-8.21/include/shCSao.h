#ifndef SHCSAO_H
#define SHCSAO_H
/*****************************************************************************
******************************************************************************
**
** FILE:
**	shCSao.h
**
** ABSTRACT:
**	This file contains all necessary definitions, prototypes, and macros
**	for the routines used to simultaneously support command line input
**	(stdin) and communications with Fermi modified SAOimage programs.
**
** REQUIRED INCLUDE FILES:
**	"imtool.h"
**	"region.h"
**
** ENVIRONMENT:
**	ANSI C.
**
** AUTHOR:      Creation date: Aug. 17, 1992
**              Gary Sergey
**
******************************************************************************
******************************************************************************
*/
#include <sys/types.h>
#include "imtool.h"
#include "region.h"		/* For PIXDATATYPE */

#define	CMD_SZ_GEOM	    40			/*???*/
#define CMD_SZ_IMCURVAL	    40+CMD_SZ_GEOM	/*???*/

/*----------------------------------------------------------------------------
**
** DEFINITIONS
*/
#define CMD_SAO_CLRS	64	/* Maximum number of colors each SAOimage copy 
				   can reserve in the colormap */
#define CMD_SAO_MAX	8	/* Maximum number of copies of SAOimage that
				   can be executed (i.e., listened to by this
				   program */
#define CMD_SAO_MIN	0	/* Minimum number of copies of SAOimage that
				   can be executed (i.e., listened to by this
				   program */
#define CMD_SAO_TIMEOUT	6	/* Time (in seconds) to wait for reply from 
				   SAOimage (at startup) before giving up */
#define CMD_FITS_HDRLEN	2880	/* FITS header files must be some multiple
				   of this value */

#define CMD_SAO_OPTIONS "-upperleft -v -zero"  /* SAOimage command line
                                                  options */
#define CMD_SAO_WNAME	"Fermi SAOimage"  /* SAOimage window name */
#define CMD_SAO_PNAME	"fsaoimage"	  /* Fermi SAOimage executable name */

#define CMD_NUM_SAOBUTT 16     /* Number of command button types */
#define CMD_LEN_SAOBUTT 16     /* ASCII length of command button names */

#define CMD_INIT           0xdeadbeef
#define CMD_INIT_COMP      0xfeedface

#define SAOINDEXINITVAL -234    /* initial value of saoindex so can tell if
				   none entered */

#define MASKSIZE	1	/* size of one mask value in bytes */
#define LUTTOP		MASKLUTTOP	/* num of elements in the LUT */

#ifndef TRUE
#define TRUE            1
#endif
#ifndef FALSE
#define FALSE           0
#endif

/*----------------------------------------------------------------------------
**
** Value used to fill in the saonumbers initially in the SHSAOPROC structure
*/
#define INIT_SAONUM_VAL -1

/*----------------------------------------------------------------------------
**
** SAOIMAGE COMMAND STRUCTURE (HANDLE) & ENUMERATED TYPE
*/
typedef enum
   {
   SAO_UNUSED = 0,
   SAO_INUSE
   } CMD_SAOSTATE;

typedef struct cmd_saohandle
   {
   CMD_SAOSTATE	    state;	/* Indicates if FSAO copy is in use */
   char		    region[80];	/* ???Associated QV region */
   int              rows;	/* rows in image - used by saoMaskDisplay */
   int              cols;	/* cols in image - used by saoMaskDisplay */
   int		    region_cnt;	/* Copy of "# of times region was loaded" */
   int		    wfd;	/* Pipe file descriptor - write */
   int		    cwfd;	/* Pipe file descriptor for child - write */
   int		    rfd;	/* Pipe file descriptor - read */
   int		    crfd;	/* Pipe file descriptor for child - read */
   char             mousePos[CMD_SZ_IMCURVAL];  /* Mouse position in ASCII */
   } CMD_SAOHANDLE;

/* Set longword adjusted and padded size of SAO handle structure */
#define CMD_SAOHANDLE_SZ (((((sizeof(CMD_SAOHANDLE) - 1)/4) + 1) * 4) + 4)
  
typedef struct saoCmdHandle
   {
   int		    SAOEntered;
   unsigned int     initflag;
   CMD_SAOHANDLE    saoHandle[CMD_SAO_MAX];  /* Array of structure pointers */
   } SAOCMDHANDLE;

/*
 * SAO Image command button modes 
 */
typedef enum  {
   S_CIRCLE,
   S_POINT,
   S_ELLIPSE,
   S_RECTANGLE,
   S_UNKNOWN,
   S_UNSUPPORTED
} CMD_SAOSHAPES;

/*----------------------------------------------------------------------------
**
** STRUCTURE USED TO HOLD THE PIDS OF ALL THE FORKED FSAOIMAGE PROCESSES
*/

typedef struct shSaoProc
   {
   int     saonum;	/* sao number associated with this process */
   pid_t   pid;		/* pid of the forked fsaoimage process */
   } SHSAOPROC;

/*----------------------------------------------------------------------------
**
** FUNCTION PROTOTYPES
*/
#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

RET_CODE shSaoCircleDraw(int a_saoindex, char ***a_ptr_list_ptr,
			 int a_numGlyphs, int a_numGlyphElems,
			 int a_excludeFlag, int a_isAnnulus);
RET_CODE shSaoEllipseDraw(int a_saoindex, char ***a_ptr_list_ptr,
			 int a_numGlyphs, int a_numGlyphElems,
			 int a_excludeFlag, int a_isAnnulus);
RET_CODE shSaoBoxDraw(int a_saoindex, char ***a_ptr_list_ptr,
			 int a_numGlyphs, int a_numGlyphElems,
			 int a_excludeFlag, int a_isAnnulus);
RET_CODE shSaoPolygonDraw(int a_saoindex, char ***a_ptr_list_ptr,
			 int a_numGlyphs, int a_numGlyphElems,
			 int a_excludeFlag);
RET_CODE shSaoArrowDraw(int a_saoindex, char ***a_ptr_list_ptr,
			 int a_numGlyphs, int a_numGlyphElems,
			 int a_excludeFlag);
RET_CODE shSaoTextDraw(int a_saoindex, char ***a_ptr_list_ptr,
			 int a_numGlyphs, int a_numGlyphElems,
			 int a_excludeFlag);
RET_CODE shSaoPointDraw(int a_saoindex, char ***a_ptr_list_ptr,
			 int a_numGlyphs, int a_numGlyphElems,
			 int a_excludeFlag);
RET_CODE shSaoResetDraw(int a_saoindex);
RET_CODE shSaoLabel(int a_saoindex, int a_labelType);
RET_CODE shSaoZoom(int a_saoindex, double a_zoomF, int a_reset);
RET_CODE shSaoPan(int a_saoindex, int a_x, int a_y);
RET_CODE shSaoCenter(int a_saoindex);
RET_CODE shSaoGlyph(int a_saoindex, int a_glyphType);
RET_CODE p_shCheckSaoindex(int *a_saoindex);
RET_CODE shSaoMaskDisplay(char *a_maskData, MASK *a_maskPtr, char a_maskLUT[LUTTOP], int a_saoindex);
RET_CODE shSaoMaskColorSet(char **a_colorList);
RET_CODE shSaoMaskGlyphSet(int a_glyph, int a_saoindex);

RET_CODE shSaoWriteToPipe(int, char *, int);
RET_CODE shSaoSendImMsg(struct imtoolRec *, int);
RET_CODE shSaoSendFitsMsg(CMD_SAOHANDLE *, char **, char **, PIXDATATYPE, 
                          double, double, int, int, char *);
RET_CODE shSaoPipeOpen(CMD_SAOHANDLE *);
void shSaoPipeClose(CMD_SAOHANDLE *);
void shSaoCleanUp(void);
void shSaoClearMem(CMD_SAOHANDLE *);
int p_shSaoMouseCheckWait(int saoNum);
int p_shSaoMouseWaitGet(void);
RET_CODE p_shSaoMouseKeyGet(CMD_SAOHANDLE  *);
void p_shSaoRegionSet(CMD_SAOHANDLE *, char *);
void p_shSaoImageCleanFunc(void);
RET_CODE p_shSaoIndexCheck(int *);
RET_CODE p_shSaoSendMaskMsg(CMD_SAOHANDLE *a_shl, char a_maskLUT[LUTTOP],
			char *a_mask, int a_nrows, int a_ncols,
			char *a_colorTable);
RET_CODE p_shSaoSendMaskGlyphMsg(CMD_SAOHANDLE *a_shl, int a_glyph);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif
