/*<AUTO>
******************************************************************************
**
** FILE: saoDraw.c
**<HTML>
**  All 'glyphs' drawn on the display by
**  any saoDraw routine can be saved into a file and then redrawn on a future
**  display.  This saving and restoring of glyphs is done through the FSAOimage
**  cursor read and write commands available on the FSAOimage button panel.
**  <P>
**  All of the draw routines share the following three parameters in common
**  which need more discussion -
**
**  <UL>
**  <LI>a_ptr_list_ptr
**  <LI>a_numGlyphs
**  <LI>a_numGlyphElems
**  </UL>
**  <P>
**  The parameter a_ptr_list_ptr is of type <EM>char ***</EM>.  What this pointer
**  is doing is mimicing the Tcl lists that can be input to the associated Tcl
**  Sao draw commands.  Thus for example, if I wanted to draw 3 separate circle
**  glyphs in fSAO (a_numGlyphs = 3) , each of which was an annulus with two
**  rings of radius rad1 and rad2, (a_numGlyphElems = 2), my a_ptr_list_ptr
**  parameter would look as follows -
**
**  <PRE>
**
**      ----------------       ------             ------
**     | a_ptr_list_ptr | --> | row  | --------> | row1 |
**      ----------------       ------             ------
**                            | col  | ------.   | row2 |
**                             ------        |    ------
**                            | rad1 | ----. |   | row3 |
**                             ------      | |    ------
**                            | rad2 | -.  | |
**                             ------   |  | |    ------
**                                      |  | --->| col1 |
**                                      .  |      ------
**                                      .  |     | col2 |
**                                      .  |      ------
**                                         |     | col3 |
**                                         |      ------
**                                         |
**                                         |      -------
**                                         ----->| rad1a |
**                                                -------
**                                               | rad1b |
**                                                -------
**                                               | rad1c |
**                                                -------
**
**  </PRE>
**  <P>
**  Unfortunately, a_numGlyphsElems must be the same for each separate glyph.
**  So you cannot include in a_ptr_list_ptr (at the same time) a glyph which is
**  a circle which is an annulus and one that is not.  You would need to make
**  two separate calls to the appropriate routine, one for the annulus case and
**  one for the other.
**</HTML>
**</AUTO>
**   ENTRY POINT	    SCOPE   DESCRIPTION
**   -------------------------------------------------------------------------
**   saoDrawWrite	    local   Write SAO region drawing information to
**				    SAOimage.
**   saoResetWrite	    local   Write SAO region reset information to 
**				    SAOimage.
**   saoLabelWrite	    local   Write SAO region labeling information to 
**				    SAOimage.
**   saoGlyphWrite	    local   Write SAO region information to SAOimage.
**   p_shSaoIndexCheck	    local   Check SAO index to see if it is valid
**   shSaoCircleDraw	    global  routine to send an SAO circle region to
**                                  SAOimage.
**   shSaoEllipseDraw	    global  routine to send an SAO ellipse region to
**                                  SAOimage.
**   shSaoBoxDraw	    global  routine to send an SAO box region to
**                                  SAOimage.
**   shSaoPolygonDraw	    global  routine to send an SAO polygon region to
**                                  SAOimage.
**   shSaoArrowDraw	    global  routine to send an SAO arrow region to
**                                  SAOimage.
**   shSaoTextDraw	    global  routine to send an SAO text region to
**                                  SAOimage.
**   shSaoPointDraw	    global  routine to send an SAO point region to
**                                  SAOimage.
**   shSaoResetDraw	    global  routine to reset the SAO regions drawn
**                                  in SAOimage by the Draw TCL commands.
**   shSaoLabel 	    global  Send a SAO label command to fsao
**   shSaoGlyph 	    global  Send a SAO glyph command to fsao
**
** ENVIRONMENT:
**	ANSI C.
**
** AUTHORS:     Creation date: Jun. 30, 1994
**
******************************************************************************
******************************************************************************
*/
#include <stdio.h>
#include <string.h>	/* Needed for strcat */
#include <errno.h>
#include <unistd.h>     /* Prototype for read() and friends */
#include <ctype.h>      /* Prototype for isdigit() and friends */
#include <stdlib.h>     /* Prototype for atoi(). God, I'm starting to hate */
                        /* the IRIX C compiler */
#include <alloca.h>
#include "region.h"
#include "shCSao.h"
#include "dervish_msg_c.h"        /* Murmur message file for Dervish */
#include "shCUtils.h"
#include "shCGarbage.h"
#include "shCErrStack.h"

/* I added this when changing the maximum size from 10.  Hope I got em all
   Chris S. */
#define PARAM_MAX_SIZE 50


/*============================================================================
**============================================================================
**
** LOCAL PROTOTYPES, DEFINITIONS, MACROS, ETC.
** (NOTE: NONE OF THESE WILL BE VISIBLE OUTSIDE THIS MODULE)
**
**============================================================================
*/

/*----------------------------------------------------------------------------
**
** LOCAL FUNCTION PROTOTYPES
*/
RET_CODE saoDrawWrite(CMD_SAOHANDLE *, int, int, int, char *);
static RET_CODE saoResetWrite(CMD_SAOHANDLE *);
static RET_CODE saoLabelWrite(CMD_SAOHANDLE *, int);
static RET_CODE saoGlyphWrite(CMD_SAOHANDLE *, int);
static RET_CODE annulusParse(int , int, char ***, int, char *);

/*----------------------------------------------------------------------------
**
** LOCAL DEFINITIONS
*/

/*
** Used in all the glyph drawing routines
*/
static char   oparen[2] = "(";
static char   cparen[2] = ")";
static char   comma[2]  = ",";
static char   cparenAmper[3]  = ")&";
static char   amper[2]  = "&";
static char   annulus[] = "ANNULUS(";
static char   circle[] = "CIRCLE(";
static char   ellipse[] = "ELLIPSE(";
static char   box[] = "BOX(";
static char   polygon[] = "POLYGON(";
static char   arrow[] = "ARROW(";
static char   text[] = "TEXT(";
static char   point[] = "POINT(";
static char   exclude[2] = "-";
static char   space[2] = " ";
static char   annulusDivider[5] = " & !";
static char   *genPtr1, *genPtr2;


/*----------------------------------------------------------------------------
**
** SAOIMAGE SAO REGION TYPES
*/
static char saoRegions[8][8] =
   {
   "ANNULUS",       /*     DO       */
   "CIRCLE",        /*     NOT      */
   "ELLIPSE",       /*   MODIFY     */
   "BOX",           /*     THE      */
   "POLYGON",       /*    ORDER     */
   "ARROW",         /*      OF      */
   "TEXT",          /*    THESE     */
   "POINT"          /* DEFINITIONS  */
   };

/*----------------------------------------------------------------------------
**
** GLOBAL BOGUS PARAMETER FOR ARROW GLYPHS DRAWN ON THE FSAO DISPLAY.  FSAO
** EXPECTS THERE TO BE 5 PARAMETERS BUT THE USER ONLY INPUTS 4.  FSAO IGNORES
** THE VALUE OF THIS PARAMETER TOO.
*/
static char	g_bogusParam[] = "10,";

/*----------------------------------------------------------------------------
**
** GLOBAL ERROR LEVEL FOR CALL TO shDebug
**
*/
static int   g_errorLevel = 1;

/* External variables referenced here */
extern SAOCMDHANDLE g_saoCmdHandle;


/*
**============================================================================
**
** ROUTINE: saoDrawWrite
**
** DESCRIPTION:
**  Routine to write SAO region drawing information to SAOimage.
**
** RETURN VALUES:
**  SH_SUCCESS
**  dervish_SAOSyncErr
**  status from shSaoWriteToPipe
**  status from shSaoSendImMsg
**
** CALLED BY:
**  tclSaoCircleDraw
**  tclSaoEllipseDraw
**  tclSaoBoxDraw
**  tclSaoPolygonDraw
**  tclSaoArrowDraw
**  tclSaoTextDraw
**  tclSaoPointDraw
**
** CALLS TO:
**  shSaoSendImMsg
**  shSaoWriteToPipe
**
**============================================================================
*/
RET_CODE saoDrawWrite
   (
   CMD_SAOHANDLE *a_shl,    /* IN: Cmd SAO handle (internal structure ptr)*/
   int		type,		/* IN: type of SAO region to draw */
   int		exclude_flag,	/* IN: Region excluded or not? */
   int		annulus,	/* IN: Region is an annulus or not? */
   char		*params		/* IN: Parameter list to send */
   )   
{
RET_CODE	    rstatus;
struct imtoolRec    im;		/* Command structure sent to SAOimage */
int		    p_bytes = 0, bytes;

   /* Get length of string to send only if is a real pointer */
   p_bytes = strlen(params);

   /*
   ** TELL SAOIMAGE WE'RE GOING TO SEND AN SAO REGION INSTRUCTION
   */
   /* Setup command structure that is recognized by SAOimage.  SAOimage expects
      commands ala iraf's imtool. */
   im.tid      = type;		/* Indicate type of SAO region to draw */
   im.subunit  = DRAWCURSOR;	/* Type of fsao command */
   im.z        = 0;		/* unused */
   im.checksum = 0;		/* will get filled in later */
   im.t	    = 123;		/* ??? Gary's kluge */
   im.x        = exclude_flag;	/* Is this SAO region to be excluded? */
   im.y        = 0;                
   im.z        = annulus;          /* Is this SAO region an annulus? */
   im.thingct  = -(p_bytes);	/* Number of bytes in 2nd message : fsao
				   expects this to be a negative */

   /* Send if off to SAOimage */
   rstatus = shSaoSendImMsg(&im, a_shl->wfd);
   if (rstatus != SH_SUCCESS)
     {goto err_exit;}

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
           goto err_exit;
       }
       else
           break;
   }

   /*
   ** SEND THE PARAMETERS
   */
   rstatus = shSaoWriteToPipe(a_shl->wfd, params, p_bytes);
   if (rstatus != SH_SUCCESS)
      {goto err_exit;}

   return(SH_SUCCESS);

   err_exit:
   return(rstatus);
}


/*============================================================================
**============================================================================
**
** ROUTINE: saoResetWrite
*/
static RET_CODE saoResetWrite
   (
   CMD_SAOHANDLE *a_shl     /* IN: Cmd SAO handle (internal structure ptr)*/
   )   
/*
** DESCRIPTION:
**	Routine to write SAO region reset information to SAOimage.
**
** RETURN VALUES:
**	SH_SUCCESS
**	dervish_SAOSyncErr
**	status from shSaoWriteToPipe
**	status from shSaoSendImMsg
**
** CALLED BY:
**	tclSaoResetDraw
**
** CALLS TO:
**	shSaoSendImMsg
**	shSaoWriteToPipe
**
**============================================================================
*/
{

RET_CODE	    rstatus;
struct imtoolRec    im;		/* Command structure sent to SAOimage */

/*
** TELL SAOIMAGE WE'RE GOING TO SEND A RESET SAO REGIONS INSTRUCTION
*/
/* Setup command structure that is recognized by SAOimage.  SAOimage expects
   commands ala iraf's imtool. */
im.tid      = 0;		/* Indicate type of SAO region to draw */
im.subunit  = RESETCURSOR;	/* Type of fsao command */
im.z        = 0;		/* unused */
im.checksum = 0;		/* will get filled in later */
im.t	    = 123;		/* ??? Gary's kluge */
im.x        = 0;
im.y        = 0;
im.z        = 0;
im.thingct  = 0;

/* Send if off to SAOimage */
rstatus = shSaoSendImMsg(&im, a_shl->wfd);
if (rstatus != SH_SUCCESS)
  {goto err_exit;}

return(SH_SUCCESS);

err_exit:
return(rstatus);
}


/*============================================================================
**============================================================================
**
** ROUTINE: saoLabelWrite
*/
static RET_CODE saoLabelWrite
   (
   CMD_SAOHANDLE *a_shl,    /* IN: Cmd SAO handle (internal structure ptr) */
   int      label_value     /* IN: either 0 (no labels), 1 (do labels) or
                               toggle current labels */
   )   
/*
** DESCRIPTION:
**	Routine to write SAO region labeling information to SAOimage.
**
** RETURN VALUES:
**	SH_SUCCESS
**	dervish_SAOSyncErr
**	status from shSaoWriteToPipe
**	status from shSaoSendImMsg
**
** CALLED BY:
**	tclSaoLabel
**
** CALLS TO:
**	shSaoSendImMsg
**	shSaoWriteToPipe
**
**============================================================================
*/
{
RET_CODE	    rstatus;
struct imtoolRec    im;		/* Command structure sent to SAOimage */

/*
** TELL SAOIMAGE WE'RE GOING TO SEND AN SAO LABEL INSTRUCTION
*/
/* Setup command structure that is recognized by SAOimage.  SAOimage expects
   commands ala iraf's imtool. */
im.tid      = 0;		/* Indicate type of SAO region to draw */
im.subunit  = SETLABEL; 	/* Type of fsao command */
im.z        = 0;		/* unused */
im.checksum = 0;		/* will get filled in later */
im.t	    = 123;		/* ??? Gary's kluge */
im.x        = label_value;	/* If = 0, no labels, if = 1, labels, else
				   toggle labels */
im.y        = 0;
im.z        = 0;
im.thingct  = 0;

/* Send if off to SAOimage */
rstatus = shSaoSendImMsg(&im, a_shl->wfd);
if (rstatus != SH_SUCCESS)
  {goto err_exit;}

return(SH_SUCCESS);

err_exit:
return(rstatus);
}


/*============================================================================
**============================================================================
**
** ROUTINE: saoGlyphWrite
*/
static RET_CODE saoGlyphWrite
   (
   CMD_SAOHANDLE *a_shl,    /* IN: Cmd SAO handle (internal structure ptr) */
   int      glyph_value     /* IN: either 0 (no glyphs), 1 (do glyphs) or
                               toggle current glyphs */
   )   
/*
** DESCRIPTION:
**	Routine to write SAO region visibility information to SAOimage.
**
** RETURN VALUES:
**	SH_SUCCESS
**	dervish_SAOSyncErr
**	status from shSaoWriteToPipe
**	status from shSaoSendImMsg
**
** CALLED BY:
**	tclSaoGlyph
**
** CALLS TO:
**	shSaoSendImMsg
**	shSaoWriteToPipe
**
**============================================================================
*/
{
RET_CODE	    rstatus;
struct imtoolRec    im;		/* Command structure sent to SAOimage */

/*
** TELL SAOIMAGE WE'RE GOING TO SEND AN SAO GLYPH INSTRUCTION
*/
/* Setup command structure that is recognized by SAOimage.  SAOimage expects
   commands ala iraf's imtool. */
im.tid      = 0;		/* Indicate type of SAO region to draw */
im.subunit  = SETGLYPH; 	/* Type of fsao command */
im.z        = 0;		/* unused */
im.checksum = 0;		/* will get filled in later */
im.t	    = 123;		/* ??? Gary's kluge */
im.x        = glyph_value;	/* If = 0, no glyphs, if = 1, glyphs, else
				   toggle glyphs */
im.y        = 0;
im.z        = 0;
im.thingct  = 0;

/* Send if off to SAOimage */
rstatus = shSaoSendImMsg(&im, a_shl->wfd);
if (rstatus != SH_SUCCESS)
  {goto err_exit;}

return(SH_SUCCESS);

err_exit:
return(rstatus);
}


/*============================================================================
**============================================================================
**
** ROUTINE: annulusParse
*/
static RET_CODE annulusParse
  (
    int         numparams,		/* IN: number of parameters found */
    int		type,			/* IN: is it BOX or ELLIPSE annulus? */
    char	***ptr_list_ptr,	/* IN: array of pointers to params */
    int         listElem,		/* IN: list element to parse */
    char	*command		/* MOD: array of parameters */
  )   
/*
** DESCRIPTION:
**	Create an annulus command for fsao.
**
** RETURN VALUES:
**	
**
** CALLED BY:
**	shSaoEllipseDraw
**	shSaoBoxDraw
**
** CALLS TO:
**
**============================================================================
*/
{ /* annulusParse */

int   i, j, length, length2;

/* Construct the initial line  - name(Xc, Yc, Rx1, Ry1, Angle) */
command = strcat(command, genPtr1);		/* exclude option */
command = strcat(command, saoRegions[type]);
command = strcat(command, oparen);		/* Add the '(' */

/* put in center points of glyph. reverse the order as we specified them as
   'row column' and fsao expects the input as 'column row' */
command = strcat(command, ptr_list_ptr[1][listElem]);
command = strcat(command, comma);
command = strcat(command, ptr_list_ptr[0][listElem]);

/* put in radii of glyph. reverse the order as we specified them as
   'rowradii columnradii' and fsao expects the input as 
   'columnradii rowradii' */
command = strcat(command, comma);
command = strcat(command, ptr_list_ptr[3][listElem]);
command = strcat(command, comma);
command = strcat(command, ptr_list_ptr[2][listElem]);

/* Add in the angle */
command = strcat(command, comma);
command = strcat(command, ptr_list_ptr[numparams-1][listElem]);

/* Must add a ')' */
command = strcat(command, cparen);

length = strlen(command);		/* this is the end of the first group */

/* Now construct each additional line.  These additional lines are formatted
   like - name(Xc, Yc, Rx2, Ry2, Angle) & !name(Xc, Yc, Rx1, Ry1, Angle) and 
   there is one line for each extra set of Rx and Ry values. */
for (i = 4 ; i < (numparams - 1) ; i = i + 2)
   {
   command = strcat(command, amper);  		/* Add '&' */
   command = strcat(command, genPtr1);		/* exclude option */
   command = strcat(command, saoRegions[type]);   /* add the name */
   command = strcat(command, oparen);		/* Add the '(' */

   /* put in center points of glyph. reverse the order as we specified them as
      'row column' and fsao expects the input as 'column row' */
   command = strcat(command, ptr_list_ptr[1][listElem]);
   command = strcat(command, comma);
   command = strcat(command, ptr_list_ptr[0][listElem]);

   /* put in radii of glyph. reverse the order as we specified them as
      'rowradii columnradii' and fsao expects the input as 
      'columnradii rowradii' */
   command = strcat(command, comma);
   command = strcat(command, ptr_list_ptr[i+1][listElem]);
   command = strcat(command, comma);
   command = strcat(command, ptr_list_ptr[i][listElem]);

   /* Add in the angle */
   command = strcat(command, comma);
   command = strcat(command, ptr_list_ptr[numparams-1][listElem]);

   /* Must add a ')' */
   command = strcat(command, cparen);

   command = strcat(command, annulusDivider);  /* Add annulus separator */
   /* Add a copy of the initial line to the end */
   length2 = strlen(command);
   for (j = 0; j < length; ++j)
      {
      command[length2 + j] = command[j];
      }
   command[length2 + j] = '\0';		/* null terminate the string */
 }

/* Must remove the last ')' in params buffer and make it a ')&' */
command[strlen(command) - 1] = '\0';
command = strcat(command, cparenAmper);

/*---------------------------------------------------------------------------
**
** RETURN
*/
return(SH_SUCCESS);
} /* annulusParse */


/*============================================================================
**============================================================================
**
** ROUTINE: p_shCheckSaoindex
*/
RET_CODE p_shCheckSaoindex
  (
   int *a_saoindex
  )   
/*
** DESCRIPTION:
**	Check the passed saoindex value to see if it is valid.  Valid means one
**      or more of the following :
**
**             o Must be > 0
**             o Must be < CMD_SAO_MAX
**             o Must have an associated fsao process 
**
**      If the value of -1 is entered, find the first fsao process and return
**      its saoindex.
**
** RETURN VALUES:
**	SH_SUCCESS
**	SH_SAO_NUM_INV
**      SH_RANGE_ERR
**      SH_NO_FSAO
**
** CALLED BY:
**	TCL command
**
** CALLS TO:
**
**============================================================================
*/
{ /* p_shCheckSaoindex */

RET_CODE rstatus = SH_SUCCESS;

if (*a_saoindex == SAOINDEXINITVAL)
   {
   /* Saoindex was not passed as an option.
      Look for first saoindex that belongs to us */
   *a_saoindex = CMD_SAO_MIN;
   while (*a_saoindex < CMD_SAO_MAX)
      {
      if (g_saoCmdHandle.saoHandle[*a_saoindex].state == SAO_INUSE)
	 {
	 break;
	 }
      ++(*a_saoindex);
      }
   if (*a_saoindex == CMD_SAO_MAX)
      {
      /* No fSAO was found that belongs to this process */
      rstatus = SH_NO_FSAO;
      shErrStackPush("No fSAO associated with this process.");
      }
   }
else if (*a_saoindex < 0)
   {
   /* Invalid number, must be greater than 0 */
   rstatus = SH_SAO_NUM_INV;
   shErrStackPush("ERROR : Invalid SAO number - must be > zero.");
   }
else
   {
   if ((*a_saoindex > CMD_SAO_MAX) || 
       (g_saoCmdHandle.saoHandle[*a_saoindex].state == SAO_UNUSED))
      {
      rstatus = SH_RANGE_ERR;
      shErrStackPush("ERROR : No fSAO associated with SAO number.");
      }
   }

return(rstatus);
} /* p_shCheckSaoindex */

/*<AUTO EXTRACT>
**============================================================================
**
** ROUTINE: shSaoCircleDraw
**
** DESCRIPTION:
**<HTML>
**  The shSaoCircleDraw routine enables the user to place a circle of user
**  defined size on the FSAOimage display.
**</HTML>
**
** RETURN VALUES:
**   SH_SUCCESS           - Successful completion
**   SH_SAO_NUM_INV       - An invalid saoindex was specified.
**   SH_RANGE_ERR         - The specified saoindex value was too large.
**   SH_NO_FSAO           - No fSAO process associated with this process
**   SH_MALLOC_ERR        - Error mallocing enough space to contain the command
**   SH_PIPE_WRITE_MAX    - Could not write to fSAO process.
**   SH_PIPE_WRITEERR     - Error writing to pipe.
**
**</AUTO>
**
**============================================================================
*/
RET_CODE shSaoCircleDraw
  (
   int  a_saoindex,          /* IN: fSAOimage program in which to draw the
                                    glyph.  If = SAOINDEXINITVAL then use the
                                    first fSAO process we know about. */
   char ***a_ptr_list_ptr,   /* IN: Pointer to a list of parameters, described
                                    more precisely above. */
   int  a_numGlyphs,         /* IN: Number of individual glyphs to send to
                                    fSAO. (See above) */
   int  a_numGlyphElems,     /* IN: Number of parameters in each glyph. (See
                                    above) */
   int  a_excludeFlag,       /* IN: If TRUE, mark this glyph (or set of glyphs)                                    as an exclude glyph.  Same as exclude for
                                    SAOimage cursor regions. */
   int  a_isAnnulus          /* IN: Is this glyph an annulus. (See above) */
  )   
{ /* shSaoCircleDraw */

RET_CODE rstatus;
int    i, j;
char   *command = 0;

/* Check saoindex.  If not passed as option (= -1), use the first one that
   points to an fsao that belongs to us */
if ((rstatus = p_shCheckSaoindex(&a_saoindex)) == SH_SUCCESS)
   {
   /* Malloc enough space to hold the entire command.  Assume each
      parameter takes at most PARAM_MAX_SIZE characters. */
   command = (char *)alloca(a_numGlyphElems*PARAM_MAX_SIZE*a_numGlyphs);
   if (command == 0)
      {
      /* Could not malloc the space. */
      shErrStackPush("Unable to alloca space for command list.\n");
      rstatus = SH_MALLOC_ERR;
      }
   else
      {
      /* Construct the ascii command that fsao expects. Do this once for each
         glyph we are supposed to create. */
      command[0] = '\0';       /* Initialize */
      if (a_isAnnulus == TRUE)
	genPtr2 = annulus;
      else
	genPtr2 = circle;

      if (a_excludeFlag == TRUE)
	genPtr1 = exclude;
      else
	genPtr1 = space;

      for (i = 0 ; i < a_numGlyphs ; ++i)
	 {
	 command = strcat(command, genPtr1);
	 command = strcat(command, genPtr2);

	 /* put in center points of glyph. reverse the order as we specified
	    them as 'row column' and fsao expects the input as 'column row' */
	 command = strcat(command, a_ptr_list_ptr[1][i]);
	 command = strcat(command, comma);
	 command = strcat(command, a_ptr_list_ptr[0][i]);

	 
	 /* Add the rest of the parameters to the command. */
	 for (j = 2 ; j < a_numGlyphElems ; ++j)
	    {
	    command = strcat(command, comma);
	    command = strcat(command, a_ptr_list_ptr[j][i]);
	    }
	 
	 /* Must make the last element a ')&' */
	 command = strcat(command, cparenAmper);
	 }

      /* Remove the last '&' */
      command[strlen(command)-1] = '\0';

      shDebug(g_errorLevel, "Command = %s\n", command);

      /* Send the command to fsao */
      rstatus = saoDrawWrite(&(g_saoCmdHandle.saoHandle[a_saoindex]),
			     D_CIRCLE, a_excludeFlag, a_isAnnulus, command);
      if (rstatus != SH_SUCCESS)
	{
	shErrStackPush("Trouble when writing to FSAOimage.\nMake sure the fsao product has been setup before doing saoDisplay.\n");
	rstatus = SH_PIPE_WRITEERR;
        }
      }
   }

return(rstatus);
} /* tclSaoCircleDraw */


/*<AUTO EXTRACT>
**============================================================================
**
** ROUTINE: shSaoEllipseDraw
**
** DESCRIPTION:
**<HTML>
**  The shSaoEllipseDraw routine enables the user to place an ellipse of user 
**  defined size on the FSAOimage display.
**</HTML>
**
** RETURN VALUES:
**   SH_SUCCESS           - Successful completion
**   SH_SAO_NUM_INV       - An invalid saoindex was specified.
**   SH_RANGE_ERR         - The specified saoindex value was too large.
**   SH_NO_FSAO           - No fSAO process associated with this process
**   SH_MALLOC_ERR        - Error mallocing enough space to contain the command
**   SH_PIPE_WRITE_MAX    - Could not write to fSAO process.
**   SH_PIPE_WRITEERR     - Error writing to pipe.
**
**</AUTO>
**
**============================================================================
*/
RET_CODE shSaoEllipseDraw
  (
   int  a_saoindex,          /* IN: fSAOimage program in which to draw the
                                    glyph.  If = SAOINDEXINITVAL then use the
                                    first fSAO process we know about. */
   char ***a_ptr_list_ptr,   /* IN: Pointer to a list of parameters, described
                                    more precisely above. */
   int  a_numGlyphs,         /* IN: Number of individual glyphs to send to
                                    fSAO. (See above) */
   int  a_numGlyphElems,     /* IN: Number of parameters in each glyph. (See
                                    above) */
   int  a_excludeFlag,       /* IN: If TRUE, mark this glyph (or set of glyphs)                                    as an exclude glyph.  Same as exclude for
                                    SAOimage cursor regions. */
   int  a_isAnnulus          /* IN: Is this glyph an annulus. (See above) */
  )   
{ /* shSaoEllipseDraw */

RET_CODE rstatus;
int    i;
char   *command = 0;

/* Check saoindex.  If not passed as option (= -1), use the first one that
   points to an fsao that belongs to us */
if ((rstatus = p_shCheckSaoindex(&a_saoindex)) == SH_SUCCESS)
   {
   /* Malloc enough space to hold the entire command.  Assume each
      parameter takes at most PARAM_MAX_SIZE characters. */
   if (a_isAnnulus)
     /* Malloc appropriate amount of space for the new parameter list .  Assume
      ** each parameter takes at most PARAM_MAX_SIZE characters. To get the
      ** number of parameters needed for the annulus, use the following -
      **
      **      needed params = (a_numGlyphElems - 4) x 6
      **  thus
      **      if a_numGlyphElems = 7 (or (Xc, Yc, Rx1, Ry1, Rx2, Ry2, Angle)
      **      then needed params = 18 or [6 params per group within () & add 1
      **                        for each ()'s etc, 
      **                        remember 1 param is PARAM_MAX_SIZE characters.]
      **       name(Xc, Yc, Rx1, Ry1, Angle)
      **       name(Xc, Yc, Rx2, Ry2, Angle) & !name(Xc, Yc, Rx1, Ry1, Angle)
      **
      ** These two lines make up the expression defining the annulus where
      ** 'name' will be either ELLIPSE or BOX.
      */
     command = (char *)alloca(
			   (a_numGlyphElems - 4)*6*PARAM_MAX_SIZE*a_numGlyphs);
   else
     command = (char *)alloca(a_numGlyphElems*PARAM_MAX_SIZE*a_numGlyphs);
   if (command == 0)
      {
      /* Could not alloca the space. */
      shErrStackPush("Unable to alloca space for command list.\n");
      rstatus = SH_MALLOC_ERR;
      }
   else
      {
      /* Construct the ascii command that fsao expects. Do this once for each
         glyph we are supposed to create. */
      command[0] = '\0';       /* Initialize */
      genPtr2 = ellipse;       /* assume is not an annulus */

      if (a_excludeFlag == TRUE)
        genPtr1 = exclude;
      else
        genPtr1 = space;

      for (i = 0 ; i < a_numGlyphs ; ++i)
	 {
	 /* Check if an annulus was specified. */
	 if (a_isAnnulus)
	    {
	    annulusParse(a_numGlyphElems, D_ELLIPSE,
			 a_ptr_list_ptr, i, &command[strlen(command)]);
	    }
	 else
	    {
	    command = strcat(command, genPtr1);
	    command = strcat(command, genPtr2);

	    /* put in center points of glyph. reverse the order as we specified
	       them as 'row column' and fsao expects the input as
	       'column row' */
	    command = strcat(command, a_ptr_list_ptr[1][i]);
	    command = strcat(command, comma);
	    command = strcat(command, a_ptr_list_ptr[0][i]);
	    
	    /* put in radii of glyph. reverse the order as we specified them as
	       'rowradii columnradii' and fsao expects the input as 
	       'columnradii rowradii' */
	    command = strcat(command, comma);
	    command = strcat(command, a_ptr_list_ptr[3][i]);
	    command = strcat(command, comma);
	    command = strcat(command, a_ptr_list_ptr[2][i]);
	    
	    /* Add in the angle */
	    command = strcat(command, comma);
	    command = strcat(command, a_ptr_list_ptr[4][i]);
	    
	    /* Must make the last element a ')&' */
	    command = strcat(command, cparenAmper);
	    }
	 }

      /* Remove the last '&' */
      command[strlen(command)-1] = '\0';

      shDebug(g_errorLevel, "Command = %s\n", command);

      /* Send the command to fsao */
      rstatus = saoDrawWrite(&(g_saoCmdHandle.saoHandle[a_saoindex]),
			     D_ELLIPSE, a_excludeFlag, a_isAnnulus, command);
      if (rstatus != SH_SUCCESS)
	{
	shErrStackPush("Trouble when writing to FSAOimage.\nMake sure the fsao product has been setup before doing saoDisplay.\n");
	rstatus = SH_PIPE_WRITEERR;
        }
      }
   }

return(rstatus);
} /* shSaoEllipseDraw */

/*<AUTO EXTRACT>
**============================================================================
**
** ROUTINE: shSaoBoxDraw
**
** DESCRIPTION:
**<HTML>
**  The shSaoBoxDraw routine enables the user to place a box of user defined
**  size on the FSAOimage display.
**</HTML>
**
** RETURN VALUES:
**   SH_SUCCESS           - Successful completion
**   SH_SAO_NUM_INV       - An invalid saoindex was specified.
**   SH_RANGE_ERR         - The specified saoindex value was too large.
**   SH_NO_FSAO           - No fSAO process associated with this process
**   SH_MALLOC_ERR        - Error mallocing enough space to contain the command
**   SH_PIPE_WRITE_MAX    - Could not write to fSAO process.
**   SH_PIPE_WRITEERR     - Error writing to pipe.
**
**</AUTO>
**============================================================================
*/
RET_CODE shSaoBoxDraw
  (
   int  a_saoindex,          /* IN: fSAOimage program in which to draw the
                                    glyph.  If = SAOINDEXINITVAL then use the
                                    first fSAO process we know about. */
   char ***a_ptr_list_ptr,   /* IN: Pointer to a list of parameters, described
                                    more precisely above. */
   int  a_numGlyphs,         /* IN: Number of individual glyphs to send to
                                    fSAO. (See above) */
   int  a_numGlyphElems,     /* IN: Number of parameters in each glyph. (See
                                    above) */
   int  a_excludeFlag,       /* IN: If TRUE, mark this glyph (or set of glyphs)                                    as an exclude glyph.  Same as exclude for
                                    SAOimage cursor regions. */
   int  a_isAnnulus          /* IN: Is this glyph an annulus. (See above) */
  )   
{ /* shSaoBoxDraw */

RET_CODE rstatus;
int    i;
char   *command = 0;

/* Check saoindex.  If not passed as option (= -1), use the first one that
   points to an fsao that belongs to us */
if ((rstatus = p_shCheckSaoindex(&a_saoindex)) == SH_SUCCESS)
   {
   /* Malloc enough space to hold the entire command.  Assume each
      parameter takes at most PARAM_MAX_SIZE characters. */
   command = (char *)alloca(a_numGlyphElems*PARAM_MAX_SIZE*a_numGlyphs);
   if (command == 0)
      {
      /* Could not malloc the space. */
      shErrStackPush("Unable to alloca space for command list.\n");
      rstatus = SH_MALLOC_ERR;
      }
   else
      {
      /* Construct the ascii command that fsao expects. Do this once for each
         glyph we are supposed to create. */
      command[0] = '\0';       /* Initialize */
      genPtr2 = box;

      if (a_excludeFlag == TRUE)
        genPtr1 = exclude;
      else
        genPtr1 = space;

      for (i = 0 ; i < a_numGlyphs ; ++i)
	 {
	 /* Check if an annulus was specified. */
	 if (a_isAnnulus)
	    {
	    annulusParse(a_numGlyphElems, D_BOX, a_ptr_list_ptr, i,
			 &command[strlen(command)]);
	    }
	 else
	    {
	    command = strcat(command, genPtr1);
	    command = strcat(command, genPtr2);

	    /* put in center points of glyph. reverse the order as we specified
	       them as 'row column' and fsao expects the input as
	       'column row' */
	    command = strcat(command, a_ptr_list_ptr[1][i]);
	    command = strcat(command, comma);
	    command = strcat(command, a_ptr_list_ptr[0][i]);
	    
	    /* put in radii of glyph. reverse the order as we specified them as
	       'rowradii columnradii' and fsao expects the input as 
	       'columnradii rowradii' */
	    command = strcat(command, comma);
	    command = strcat(command, a_ptr_list_ptr[3][i]);
	    command = strcat(command, comma);
	    command = strcat(command, a_ptr_list_ptr[2][i]);
	    
	    /* Add in the angle */
	    command = strcat(command, comma);
	    command = strcat(command, a_ptr_list_ptr[4][i]);
	    
	    /* Must make the last element a ')&' */
	    command = strcat(command, cparenAmper);
	    }
         }

      /* Remove the last '&' */
      command[strlen(command)-1] = '\0';

      shDebug(g_errorLevel, "Command = %s\n", command);
	 
      /* Send the command to fsao */
      rstatus = saoDrawWrite(&(g_saoCmdHandle.saoHandle[a_saoindex]), D_BOX,
			     a_excludeFlag, a_isAnnulus, command);
      if (rstatus != SH_SUCCESS)
	{
	shErrStackPush("Trouble when writing to FSAOimage.\nMake sure the fsao product has been setup before doing saoDisplay.\n");
	rstatus = SH_PIPE_WRITEERR;
        }
      }
   }

return(rstatus);
} /* shSaoBoxDraw */


/*<AUTO EXTRACT>
**============================================================================
**
** ROUTINE: shSaoPolygonDraw
**
** DESCRIPTION:
**<HTML>
**  The shSaoPolygonDraw routine enables the user to place a polygon of user
**  defined size on the FSAOimage display.
**</HTML>
**
** RETURN VALUES:
**   SH_SUCCESS           - Successful completion
**   SH_SAO_NUM_INV       - An invalid saoindex was specified.
**   SH_RANGE_ERR         - The specified saoindex value was too large.
**   SH_NO_FSAO           - No fSAO process associated with this process
**   SH_MALLOC_ERR        - Error mallocing enough space to contain the command
**   SH_PIPE_WRITE_MAX    - Could not write to fSAO process.
**   SH_PIPE_WRITEERR     - Error writing to pipe.
**
**</AUTO>
**============================================================================
*/
RET_CODE shSaoPolygonDraw
  (
   int  a_saoindex,          /* IN: fSAOimage program in which to draw the
                                    glyph.  If = SAOINDEXINITVAL then use the
                                    first fSAO process we know about. */
   char ***a_ptr_list_ptr,   /* IN: Pointer to a list of parameters, described
                                    more precisely above. */
   int  a_numGlyphs,         /* IN: Number of individual glyphs to send to
                                    fSAO. (See above) */
   int  a_numGlyphElems,     /* IN: Number of parameters in each glyph. (See
                                    above) */
   int  a_excludeFlag        /* IN: If TRUE, mark this glyph (or set of glyphs)                                    as an exclude glyph.  Same as exclude for
                                    SAOimage cursor regions. */
  )   
{ /* shSaoPolygonDraw */

RET_CODE rstatus;
int    i, j;
int    isAnnulus = FALSE;
char   *command = 0;

/* Check saoindex.  If not passed as option (= -1), use the first one that
   points to an fsao that belongs to us */
if ((rstatus = p_shCheckSaoindex(&a_saoindex)) == SH_SUCCESS)
   {
   /* Malloc enough space to hold the entire command.  Assume each
      parameter takes at most PARAM_MAX_SIZE characters. */
   command = (char *)alloca(a_numGlyphElems*PARAM_MAX_SIZE*a_numGlyphs);
   if (command == 0)
      {
      /* Could not malloc the space. */
      shErrStackPush("Unable to alloca space for command list.\n");
      rstatus = SH_MALLOC_ERR;
      }
   else
      {
      /* Construct the ascii command that fsao expects. Do this once for each
         glyph we are supposed to create. */
      command[0] = '\0';       /* Initialize */
      genPtr2 = polygon;

      if (a_excludeFlag == TRUE)
	genPtr1 = exclude;
      else
	genPtr1 = space;

      for (i = 0 ; i < a_numGlyphs ; ++i)
	 {
	 command = strcat(command, genPtr1);
	 command = strcat(command, genPtr2);
	 
	 /* put in center points of glyph. reverse the order as we specified
	    them as 'row column' and fsao expects the input as 'column row' */
	 for (j = 0 ; j < a_numGlyphElems ; j = j + 2)
	    {
	    command = strcat(command, a_ptr_list_ptr[j+1][i]);
	    command = strcat(command, comma);
	    command = strcat(command, a_ptr_list_ptr[j][i]);
	    command = strcat(command, comma);
	    }
	 
         /* Must make the last element a ')&' */
	 command[strlen(command) - 1] = '\0';
         command = strcat(command, cparenAmper);
         }

      /* Remove the last '&' */
      command[strlen(command)-1] = '\0';

      shDebug(g_errorLevel, "Command = %s\n", command);
      
      /* Send the command to fsao */
      rstatus = saoDrawWrite(&(g_saoCmdHandle.saoHandle[a_saoindex]),
			     D_POLYGON, a_excludeFlag, isAnnulus, command);
      if (rstatus != SH_SUCCESS)
	{
	shErrStackPush("Trouble when writing to FSAOimage.\nMake sure the fsao product has been setup before doing saoDisplay.\n");
	rstatus = SH_PIPE_WRITEERR;
        }
      }
   }

return(rstatus);
} /* shSaoPolygonDraw */

/*<AUTO EXTRACT>
**============================================================================
**
** ROUTINE: shSaoArrowDraw
** 
** DESCRIPTION:
**<HTML>
**  The shSaoArrowDraw routine enables the user to place an arrow of user
**  defined size on the FSAOimage display. 
**</HTML>
**
** RETURN VALUES:
**   SH_SUCCESS           - Successful completion
**   SH_SAO_NUM_INV       - An invalid saoindex was specified.
**   SH_RANGE_ERR         - The specified saoindex value was too large.
**   SH_NO_FSAO           - No fSAO process associated with this process
**   SH_MALLOC_ERR        - Error mallocing enough space to contain the command
**   SH_PIPE_WRITE_MAX    - Could not write to fSAO process.
**   SH_PIPE_WRITEERR     - Error writing to pipe.
**
**</AUTO>
**============================================================================
*/
RET_CODE shSaoArrowDraw
  (
   int  a_saoindex,          /* IN: fSAOimage program in which to draw the
                                    glyph.  If = SAOINDEXINITVAL then use the
                                    first fSAO process we know about. */
   char ***a_ptr_list_ptr,   /* IN: Pointer to a list of parameters, described
                                    more precisely above. */
   int  a_numGlyphs,         /* IN: Number of individual glyphs to send to
                                    fSAO. (See above) */
   int  a_numGlyphElems,     /* IN: Number of parameters in each glyph. (See
                                    above) */
   int  a_excludeFlag        /* IN: If TRUE, mark this glyph (or set of glyphs)                                    as an exclude glyph.  Same as exclude for
                                    SAOimage cursor regions. */
  )   

{ /* shSaoArrowDraw */

RET_CODE rstatus;
int    i;
int    isAnnulus = FALSE;
char   *command = 0;

/* Check saoindex.  If not passed as option (= -1), use the first one that
   points to an fsao that belongs to us */
if ((rstatus = p_shCheckSaoindex(&a_saoindex)) == SH_SUCCESS)
   {
   /* Malloc enough space to hold the entire command.  Assume each
      parameter takes at most PARAM_MAX_SIZE characters. */
   command = (char *)alloca(a_numGlyphElems*PARAM_MAX_SIZE*a_numGlyphs);
   if (command == 0)
      {
      /* Could not malloc the space. */
      shErrStackPush("Unable to alloca space for command list.\n");
      rstatus = SH_MALLOC_ERR;
      }
   else
      {
      /* Construct the ascii command that fsao expects. Do this once for each
         glyph we are supposed to create. */
      command[0] = '\0';       /* Initialize */
      genPtr2 = arrow;

      if (a_excludeFlag == TRUE)
	genPtr1 = exclude;
      else
	genPtr1 = space;   /* uses the same beginning as text glyph */

      for (i = 0 ; i < a_numGlyphs ; ++i)
	 {
         command = strcat(command, genPtr1);
         command = strcat(command, genPtr2);

	 /* put in center points of glyph. reverse the order as we specified
	    them as 'row column' and fsao expects the input as 'column row' */
	 command = strcat(command, a_ptr_list_ptr[1][i]);
	 command = strcat(command, comma);
	 command = strcat(command, a_ptr_list_ptr[0][i]);
	 command = strcat(command, comma);
	 command = strcat(command, a_ptr_list_ptr[2][i]);

	 /* Add in a bogus parameter because fsao expects there to be 5 and
	    only 4 are entered by the user. FSAO does not use this one
	    either. */
	 command = strcat(command, comma);
	 command = strcat(command, g_bogusParam);
	 
	 command = strcat(command, a_ptr_list_ptr[3][i]);
	 
         /* Must make the last element a ')&' */
         command = strcat(command, cparenAmper);
         }
 
      /* Remove the last '&' */
      command[strlen(command)-1] = '\0';

      shDebug(g_errorLevel, "Command = %s\n", command);

      /* Send the command to fsao */
      rstatus = saoDrawWrite(&(g_saoCmdHandle.saoHandle[a_saoindex]), D_ARROW,
			     a_excludeFlag, isAnnulus, command);
      if (rstatus != SH_SUCCESS)
	{
	shErrStackPush("Trouble when writing to FSAOimage.\nMake sure the fsao product has been setup before doing saoDisplay.\n");
	rstatus = SH_PIPE_WRITEERR;
        }
      }
   } 

return(rstatus);
} /* shSaoArrowDraw */

/*<AUTO EXTRACT>
**============================================================================
**
** ROUTINE: shSaoTextDraw
**
** DESCRIPTION:
**<HTML>
**  The shSaoTextDraw routine enables the user to place text on the FSAOimage
**  display.	
**</HTML>
**
** RETURN VALUES:
**   SH_SUCCESS           - Successful completion
**   SH_SAO_NUM_INV       - An invalid saoindex was specified.
**   SH_RANGE_ERR         - The specified saoindex value was too large.
**   SH_NO_FSAO           - No fSAO process associated with this process
**   SH_MALLOC_ERR        - Error mallocing enough space to contain the command
**   SH_PIPE_WRITE_MAX    - Could not write to fSAO process.
**   SH_PIPE_WRITEERR     - Error writing to pipe.
**
**</AUTO>
**============================================================================
*/
RET_CODE shSaoTextDraw
  (
   int  a_saoindex,          /* IN: fSAOimage program in which to draw the
                                    glyph.  If = SAOINDEXINITVAL then use the
                                    first fSAO process we know about. */
   char ***a_ptr_list_ptr,   /* IN: Pointer to a list of parameters, described
                                    more precisely above. */
   int  a_numGlyphs,         /* IN: Number of individual glyphs to send to
                                    fSAO. (See above) */
   int  a_numGlyphElems,     /* IN: Number of parameters in each glyph. (See
                                    above) */
   int  a_excludeFlag        /* IN: If TRUE, mark this glyph (or set of glyphs)                                    as an exclude glyph.  Same as exclude for
                                    SAOimage cursor regions. */
  )   
{ /* shSaoTextDraw */

RET_CODE rstatus;
int    i, j, k;
int    isAnnulus = FALSE;
int    quote = 34;        /* This is the double quote character */
char   *command = 0;

/* Check saoindex.  If not passed as option (= -1), use the first one that
   points to an fsao that belongs to us */
if ((rstatus = p_shCheckSaoindex(&a_saoindex)) == SH_SUCCESS)
   {
   /* Malloc enough space to hold the entire command.  Assume each
      parameter takes at most PARAM_MAX_SIZE characters. */
   command = (char *)alloca(a_numGlyphElems*PARAM_MAX_SIZE*a_numGlyphs);
   if (command == 0)
      {  
      /* Could not malloc the space. */
      shErrStackPush("Unable to alloca space for command list.\n");
      rstatus = SH_MALLOC_ERR;
      }
   else
      {
      /* Construct the ascii command that fsao expects. Do this once for each
         glyph we are supposed to create. */
      command[0] = '\0';       /* Initialize */
      genPtr2 = text;

      if (a_excludeFlag == TRUE)
	genPtr1 = exclude;
      else
	genPtr1 = space;

      for (i = 0 ; i < a_numGlyphs ; ++i)
	 {
         command = strcat(command, genPtr1);
         command = strcat(command, genPtr2);

  	 /* put in center points of glyph. reverse the order as we specified
	    them as 'row column' and fsao expects the input as 'column row' */
	 command = strcat(command, a_ptr_list_ptr[1][i]);
	 command = strcat(command, comma);
	 command = strcat(command, a_ptr_list_ptr[0][i]);
	 command = strcat(command, comma);
	 
	 /* add the string length as a parameter before the text string */
	 j = strlen(a_ptr_list_ptr[2][i]);
	 k = strlen(command);
	 sprintf(&(command[k]), "%d,%c", j, quote);
	 
	 /* The text string */
	 command = strcat(command, a_ptr_list_ptr[2][i]);
	 k = strlen(command);
	 sprintf(&(command[k]), "%c", quote);

         /* Must make the last element a ')&' */
         command = strcat(command, cparenAmper);
         }

      /* Remove the last '&' */
      command[strlen(command)-1] = '\0';

      shDebug(g_errorLevel, "Command = %s\n", command);
	 
      /* Send the command to fsao */
      rstatus = saoDrawWrite(&(g_saoCmdHandle.saoHandle[a_saoindex]), D_TEXT,
			     a_excludeFlag, isAnnulus, command);
      if (rstatus != SH_SUCCESS)
	{
	shErrStackPush("Trouble when writing to FSAOimage.\nMake sure the fsao product has been setup before doing saoDisplay.\n");
	rstatus = SH_PIPE_WRITEERR;
        }
      }
   }

return(rstatus);
} /* shSaoTextDraw */

/*<AUTO EXTRACT>
**============================================================================
**
** ROUTINE: shSaoPointDraw
**
** DESCRIPTION:
**<HTML>
**  The shSaoPointDraw routine enables the user to mark a point on the
**  FSAOimage display.	
**</HTML>
**
** RETURN VALUES:
**   SH_SUCCESS           - Successful completion
**   SH_SAO_NUM_INV       - An invalid saoindex was specified.
**   SH_RANGE_ERR         - The specified saoindex value was too large.
**   SH_NO_FSAO           - No fSAO process associated with this process
**   SH_MALLOC_ERR        - Error mallocing enough space to contain the command
**   SH_PIPE_WRITE_MAX    - Could not write to fSAO process.
**   SH_PIPE_WRITEERR     - Error writing to pipe.
**
**</AUTO>
**============================================================================
*/
RET_CODE shSaoPointDraw
  (
   int  a_saoindex,          /* IN: fSAOimage program in which to draw the
                                    glyph.  If = SAOINDEXINITVAL then use the
                                    first fSAO process we know about. */
   char ***a_ptr_list_ptr,   /* IN: Pointer to a list of parameters, described
                                    more precisely above. */
   int  a_numGlyphs,         /* IN: Number of individual glyphs to send to
                                    fSAO. (See above) */
   int  a_numGlyphElems,     /* IN: Number of parameters in each glyph. (See
                                    above) */
   int  a_excludeFlag        /* IN: If TRUE, mark this glyph (or set of glyphs)                                    as an exclude glyph.  Same as exclude for
                                    SAOimage cursor regions. */
  )   
{ /* SHSaoPointDraw */

RET_CODE rstatus;
int    i, j;
int    isAnnulus = FALSE;
char   *command = 0;

/* Check saoindex.  If not passed as option (= -1), use the first one that
   points to an fsao that belongs to us */
if ((rstatus = p_shCheckSaoindex(&a_saoindex)) == SH_SUCCESS)
   {
   /* Malloc enough space to hold the entire command.  Assume each
      parameter takes at most PARAM_MAX_SIZE characters. */
   command = (char *)alloca(a_numGlyphElems*PARAM_MAX_SIZE*a_numGlyphs);
   if (command == 0)
      {
      /* Could not malloc the space. */
      shErrStackPush("Unable to alloca space for command list.\n");
      rstatus = SH_MALLOC_ERR;
      }
   else
      {
      /* Construct the ascii command that fsao expects. Do this once for each
         glyph we are supposed to create. */
      command[0] = '\0';       /* Initialize */
      genPtr2 = point;

      if (a_excludeFlag == TRUE)
	genPtr1 = exclude;
      else
	genPtr1 = space;

      for (i = 0 ; i < a_numGlyphs ; ++i)
	 {
         command = strcat(command, genPtr1);
         command = strcat(command, genPtr2);

	 /* put in center points of glyph. reverse the order as we specified
	    them as 'row column' and fsao expects the input as 'column row' */
	 for (j = 0 ; j < a_numGlyphElems ; j = j + 2)
	    {
	    command = strcat(command, a_ptr_list_ptr[j+1][i]);
	    command = strcat(command, comma);
	    command = strcat(command, a_ptr_list_ptr[j][i]);
	    command = strcat(command, comma);
	    }

 	 /* Must remove the last comma in params buffer and make it a ')&' */
	 command[strlen(command) - 1] = '\0';
         command = strcat(command, cparenAmper);
         }

      /* Remove the last '&' */
      command[strlen(command)-1] = '\0';

      shDebug(g_errorLevel, "Command = %s\n", command);
	 
      /* Send the command to fsao */
      rstatus = saoDrawWrite(&(g_saoCmdHandle.saoHandle[a_saoindex]),
			     D_POINT, a_excludeFlag, isAnnulus, command);
      if (rstatus != SH_SUCCESS)
	{
	shErrStackPush("Trouble when writing to FSAOimage.\nMake sure the fsao product has been setup before doing saoDisplay.\n");
	rstatus = SH_PIPE_WRITEERR;
        }
      }
   }

return(rstatus);
} /* shSaoPointDraw */

/*<AUTO EXTRACT>
**============================================================================
**
** ROUTINE: shSaoResetDraw
**
** DESCRIPTION:
**<HTML>
**  The shSaoResetDraw routine enables the user to reset the FSAOimage display 
**  by erasing all the previously drawn glyphs.  This routine performs the same
**  function as the SAOimage cursor reset button.
**</HTML>
**
** RETURN VALUES:
**   SH_SUCCESS           - Successful completion
**   SH_SAO_NUM_INV       - An invalid saoindex was specified.
**   SH_RANGE_ERR         - The specified saoindex value was too large.
**   SH_NO_FSAO           - No fSAO process associated with this process
**
**</AUTO>
**============================================================================
*/
RET_CODE shSaoResetDraw
  (
   int a_saoindex    /* IN: fSAO program to send the reset command to */
  )   
{ /* shSaoResetDraw */

RET_CODE rstatus;

/* Check saoindex.  If not passed as option (= -1), use the first one that
   points to an fsao that belongs to us */
if ((rstatus = p_shCheckSaoindex(&a_saoindex)) == SH_SUCCESS)
   {
   /* Send the command to fsao */
   rstatus = saoResetWrite(&(g_saoCmdHandle.saoHandle[a_saoindex]));
   if (rstatus != SH_SUCCESS)
      {
      shErrStackPush("Trouble when writing to FSAOimage.\nMake sure the fsao product has been setup before doing saoDisplay.\n");
      rstatus = SH_PIPE_WRITEERR;
      }
   }

return(rstatus);
} /* shSaoResetDraw */


/*<AUTO EXTRACT>
**============================================================================
**============================================================================
**
** ROUTINE: shSaoLabel
**
** DESCRIPTION:
**<HTML>
**  The shSaoLabel routine enables the user to toggle the displaying of the
**  labels for drawn glyphs on the FSAOimage display.  This routine performs
**  the same function as the SAOimage cursor label button.  If the passed
**  value is equal to -1, this routine acts as a toggle.
**</HTML>
**
** RETURN VALUES:
**   SH_SUCCESS           - Successful completion
**   SH_SAO_NUM_INV       - An invalid saoindex was specified.
**   SH_RANGE_ERR         - The specified saoindex value was too large.
**   SH_NO_FSAO           - No fSAO process associated with this process
**
**</AUTO>
**============================================================================
*/
RET_CODE shSaoLabel
  (
   int a_saoindex,    /* IN: fSAO program to send the reset command to */
   int a_labelType    /* IN: If = TRUE, turn label on, if = FALSE, turn
			 label off, if = -1, toggle label. */
  )   
{ /* shSaoLabel */

RET_CODE rstatus;

/* Check saoindex.  If not passed as option (= -1), use the first one that
   points to an fsao that belongs to us */
if ((rstatus = p_shCheckSaoindex(&a_saoindex)) == SH_SUCCESS)
   {
   /* Send the command to fsao */
   rstatus = saoLabelWrite(&(g_saoCmdHandle.saoHandle[a_saoindex]),
			   a_labelType);
   if (rstatus != SH_SUCCESS)
      {
      shErrStackPush("Trouble when writing to FSAOimage.\nMake sure the fsao product has been setup before doing saoDisplay.\n");
      rstatus = SH_PIPE_WRITEERR;
      }
   }

return(rstatus);
} /* shSaoLabel */
/*<AUTO EXTRACT>
**============================================================================
**============================================================================
**
** ROUTINE: shSaoGlyph
**
** DESCRIPTION:
**<HTML>
**  The shSaoGlyph routine enables the user to toggle the displaying of the
**  glyphs on the FSAOimage display.  If the passed
**  value is equal to -1, this routine acts as a toggle.
**</HTML>
**
** RETURN VALUES:
**   SH_SUCCESS           - Successful completion
**   SH_SAO_NUM_INV       - An invalid saoindex was specified.
**   SH_RANGE_ERR         - The specified saoindex value was too large.
**   SH_NO_FSAO           - No fSAO process associated with this process
**
**</AUTO>
**============================================================================
*/
RET_CODE shSaoGlyph
  (
   int a_saoindex,    /* IN: fSAO program to send the reset command to */
   int a_glyphType    /* IN: If = TRUE, turn glyph on, if = FALSE, turn
			 glyph off, if = -1, toggle glyph. */
  )   
{ /* shSaoGlyph */

RET_CODE rstatus;

/* Check saoindex.  If not passed as option (= -1), use the first one that
   points to an fsao that belongs to us */
if ((rstatus = p_shCheckSaoindex(&a_saoindex)) == SH_SUCCESS)
   {
   /* Send the command to fsao */
   rstatus = saoGlyphWrite(&(g_saoCmdHandle.saoHandle[a_saoindex]),
			   a_glyphType);
   if (rstatus != SH_SUCCESS)
      {
      shErrStackPush("Trouble when writing to FSAOimage.\nMake sure the fsao product has been setup before doing saoDisplay.\n");
      rstatus = SH_PIPE_WRITEERR;
      }
   }

return(rstatus);
} /* shSaoGlyph */
