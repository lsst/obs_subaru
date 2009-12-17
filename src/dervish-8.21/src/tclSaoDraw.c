/*<AUTO>
******************************************************************************
**
** FILE:  tclSaoDraw.c
**
**	This file contains TCL verb routines used
**	to draw objects on FSAOimage displays.
**</AUTO>
**   ENTRY POINT	    SCOPE   DESCRIPTION
**   -------------------------------------------------------------------------
**   tclListArgsParse       local   Parse the arguments that are TCL lists.
**   drawArgsParse	    local   Parse command line for SAO drawing routines
**   tclSaoCircleDraw	    local   TCL routine to send an SAO circle region to
**                                  SAOimage.
**   tclSaoEllipseDraw	    local   TCL routine to send an SAO ellipse region to
**                                  SAOimage.
**   tclSaoBoxDraw	    local   TCL routine to send an SAO box region to
**                                  SAOimage.
**   tclSaoPolygonDraw	    local   TCL routine to send an SAO polygon region to
**                                  SAOimage.
**   tclSaoArrowDraw	    local   TCL routine to send an SAO arrow region to
**                                  SAOimage.
**   tclSaoTextDraw	    local   TCL routine to send an SAO text region to
**                                  SAOimage.
**   tclSaoPointDraw	    local   TCL routine to send an SAO point region to
**                                  SAOimage.
**   tclSaoResetDraw	    local   TCL routine to reset the SAO regions drawn
**                                  in SAOimage by the Draw TCL commands.
**   tclSaoLabel	    local   Send a SAO label command to fsao
**   shTclSaoDrawDeclare    public  World's interface to SAO drawing TCL verbs
**
** ENVIRONMENT:
**	ANSI C.
**
** AUTHORS:     Creation date: Aug. 17, 1992
**
******************************************************************************
******************************************************************************
*/
#include <stdio.h>
#include <sys/types.h>  /* Needed for select function */
#include <sys/time.h>	/* Needed for timeval struct */
#include <string.h>	/* Needed for strcat */
#include <errno.h>
#include <unistd.h>     /* Prototype for read() and friends */
#include <ctype.h>      /* Prototype for isdigit() and friends */
#include <stdlib.h>     /* Prototype for atoi(). God, I'm starting to hate */
                        /* the IRIX C compiler */
#include <alloca.h>
#include "tcl.h"
#include "region.h"
#include "shCSao.h"
#include "dervish_msg_c.h"        /* Murmur message file for Dervish */
#include "shTclUtils.h"
#include "shCUtils.h"
#include "shCErrStack.h"
#include "shTclErrStack.h"

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
static RET_CODE tclSaoCircleDraw(ClientData, Tcl_Interp *, int, char **);
static RET_CODE tclSaoEllipseDraw(ClientData, Tcl_Interp *, int, char **);
static RET_CODE tclSaoBoxDraw(ClientData, Tcl_Interp *, int, char **);
static RET_CODE tclSaoPolygonDraw(ClientData, Tcl_Interp *, int, char **);
static RET_CODE tclSaoArrowDraw(ClientData, Tcl_Interp *, int, char **);
static RET_CODE tclSaoTextDraw(ClientData, Tcl_Interp *, int, char **);
static RET_CODE tclSaoPointDraw(ClientData, Tcl_Interp *, int, char **);
static RET_CODE tclSaoResetDraw(ClientData, Tcl_Interp *, int, char **);
static RET_CODE tclSaoLabel(ClientData, Tcl_Interp *, int, char **);
static RET_CODE tclSaoLabel(ClientData, Tcl_Interp *, int, char **);
static RET_CODE tclListArgsParse(Tcl_Interp *, char **, int *, int *, char ***);
static RET_CODE drawArgsParse(int, char **, char **, int *, int *, int *);

/*----------------------------------------------------------------------------
**
** LOCAL DEFINITIONS
*/
static char *TCLUSAGE_SAODRAWCIRCLE =
 "USAGE: saoDrawCircle <row> <column> <radius> [-ei] [-s FSAO #]\n";
static char *TCLUSAGE_SAODRAWELLIPSE =
 "USAGE: saoDrawEllipse <row> <column> <rowRadius> <colRadius> <angle> [-ei] [-s FSAO #]\n";
static char *TCLUSAGE_SAODRAWBOX =
 "USAGE: saoDrawBox <row> <column> <rowDiameter> <colDiameter> <angle> [-ei] [-s FSAO #]\n";
static char *TCLUSAGE_SAODRAWPOLYGON =
 "USAGE: saoDrawPolygon <row> <column> [<row> <column>] [...] [-ei] [-s FSAO #]\n";
static char *TCLUSAGE_SAODRAWARROW =
 "USAGE: saoDrawArrow <row> <column> <length> <angle> [-ei] [-s FSAO #]\n";
static char *TCLUSAGE_SAODRAWTEXT =
 "USAGE: saoDrawText <row> <column> <text> [-ei] [-s FSAO #]\n";
static char *TCLUSAGE_SAODRAWPOINT =
 "USAGE: saoDrawPoint <row> <column> [<row> <column>] [...] [-ei] [-s FSAO #]\n";
static char *TCLUSAGE_SAORESET =
 "USAGE: saoReset [FSAO #]\n";
static char *TCLUSAGE_SAOLABEL =
 "USAGE: saoLabel [[on] | [off]] [FSAO #] \n";
static char *TCLUSAGE_SAOGLYPH =
 "USAGE: saoGlyph [[on] | [off]] [FSAO #] \n";

/*
** Used in all the glyph drawing routines
*/

/* Not used --
static char   oparen[2] = "(";
static char   cparen[2] = ")";
static char   comma[2]  = ",";
*/

/*----------------------------------------------------------------------------
**
** GLOBAL BOGUS PARAMETER FOR ARROW GLYPHS DRAWN ON THE FSAO DISPLAY.  FSAO
** EXPECTS THERE TO BE 5 PARAMETERS BUT THE USER ONLY INPUTS 4.  FSAO IGNORES
** THE VALUE OF THIS PARAMETER TOO.
*/

/* static char	g_bogusParam[] = "10,"; */	/* not used */

/*----------------------------------------------------------------------------
**
** GLOBAL ERROR LEVEL FOR CALL TO shDebug
**
*/
static int   g_errorLevel = 1;

/* External variable s referenced here */
extern SAOCMDHANDLE g_saoCmdHandle;


/*============================================================================
**============================================================================
**
** ROUTINE: tclListArgsParse
*/
static RET_CODE tclListArgsParse
  (
    Tcl_Interp  *a_interp,		/* IN: pointer to TCL interpreter */
    char	**a_ptr_params_ptr,	/* IN: array of pointers to params */
    int         *a_numparams,		/* MOD: number of parameters found */
    int         *a_numelems,		/* OUT: number of list elements */
    char        ***a_ptr_list_ptr	/* OUT: points to the parameter lists */
  )   
/*
** DESCRIPTION:
**	Expand the TCL list arguments out.  Each list must have the same number
**	of entries.
**
** RETURN VALUES:
**	TCL_OK
**	TCL_ERROR
**
** CALLED BY:
**	tclSaoCircleDraw
**	tclSaoEllipseDraw
**	tclSaoBoxDraw
**	tclSaoPolygonDraw
**	tclSaoArrowDraw
**	tclSaoTextDraw
**	tclSaoPointDraw
**
** CALLS TO:
**
**============================================================================
*/
{ /* tclListArgsParse */

RET_CODE rstatus = SH_SUCCESS;
int  i;
int  i_listArgc = 0;		/* value of listArgc for first list parameter */
int  listArgc;
char **listArgv;

/* Each parameter in ptr_params_ptr is possibly a TCL list.  Expand each
   parameter out, and put the pointer to the list of expanded parameters
   in ptr_list_ptr. */

for (i = 0 ; i < *a_numparams ; ++i)
   {
   if (Tcl_SplitList(a_interp, a_ptr_params_ptr[i], &listArgc, &listArgv)
                     != TCL_OK)
      {
      rstatus = SH_CANT_SPLIT_LIST;
      shErrStackPush("ERROR : error from Tcl_SplitList when processing parameter number - %d\n", i);
      *a_numparams = 0;		/* do not want to free any listArgv's */
      break;			/* return with the error */
      }
   else
      {
      /* Store the list pointer where we can use it later */
      a_ptr_list_ptr[i] = listArgv;

      if (i_listArgc == 0)
         {
         i_listArgc = listArgc;	       /* fill in initial value for compares */
         *a_numelems = i_listArgc;	/* return to sender */
         }
      else
         {
         /* All the listArgc values had better be the same. */
         if (listArgc != i_listArgc)
            {
            rstatus = SH_LIST_ELEM_MISMATCH;
            shErrStackPush("ERROR : Number of elements in parameter TCL lists must be equal.\n");
            shErrStackPush("Passed lists have elements numbering %d and %d.\n", i_listArgc, listArgc);
            *a_numparams = ++i;		/* only free malloced listArgv's */
            break;
            }
         }
      }
   }

return(rstatus);
} /* tclListArgsParse */

/*============================================================================
**============================================================================
**
** ROUTINE: drawArgsParse
*/
static RET_CODE drawArgsParse
  (
    int 	argc,			/* IN */
    char 	*argv[],		/* IN */
    char	**ptr_params_ptr,	/* MOD: array of pointers to params */
    int         *numparams,		/* MOD: number of parameters found*/
    int		*saoindex,		/* MOD: index into sao handles */
    int		*exclude_flag		/* MOD: exclude/include SAO region */
  )   
/*
** DESCRIPTION:
**	Parse the command line for the SAO drawing routines
**
** RETURN VALUES:
**	TCL_OK
**	TCL_ERROR
**
** CALLED BY:
**	tclSaoCircleDraw
**	tclSaoEllipseDraw
**	tclSaoBoxDraw
**	tclSaoPolygonDraw
**	tclSaoArrowDraw
**	tclSaoTextDraw
**	tclSaoPointDraw
**
** CALLS TO:
**
**============================================================================
*/
{ /* drawArgsParse */

int   i = 1;
int   j;
RET_CODE rstatus;
char  dash[2];

dash[0] = '-';
dash[1] = '\0';
*numparams = 0;		/* none found yet */

/*---------------------------------------------------------------------------
**
** CHECK COMMAND ARGUMENTS
*/
while (i < argc)
   {
   if (argv[i][0] == dash[0])	/* is an option OR a negative number! */
      {
      j = 1;
switch_here:
      switch (argv[i][j]) 
         {
         case 'e':
            /* This region is labeled EXCLUDE */
            *exclude_flag = TRUE;
            ++j;
            if (argv[i][j] != '\0')
               {
               goto switch_here;
               }
            break;
         case 'i':
            /* This region is labeled INCLUDE - this is the default */
            *exclude_flag = FALSE;
            ++j;
            if (argv[i][j] != '\0')
               {
               goto switch_here;
               }
            break;
         case 's':
            /* This is the SAO number - which copy of fsao to write to */
            *saoindex = atoi(argv[i+1]) - 1;	/* Index starts at 0 not 1 */
            if (*saoindex < 0)
               {
               /* Invalid number, must be greater than 0 */
               rstatus = SH_SAO_NUM_INV;
               shErrStackPush("ERROR : Invalid SAO number - must be > zero.");
               return(rstatus);
               }
            else
               {
               if ((*saoindex > CMD_SAO_MAX) || 
                   (g_saoCmdHandle.saoHandle[*saoindex].state == SAO_UNUSED))
                  {
                  rstatus = SH_RANGE_ERR;
                  shErrStackPush("ERROR : No fSAO associated with SAO number.");
                  return(rstatus);
                  }
               }
            /* We don't allow any other option after the 's' option ! 
	       and we consumed an additionnal argument for the SAO number */
            i++;          
            break;
	  case '0': case '1': case '2': case '3': case '4': 
	  case '5': case '6': case '7': case '8': case '9': case '.':
	    /* This is a negative number ! */
	    ptr_params_ptr[*numparams] = argv[i];
	    ++*numparams;
	    break;
         default:
            /* Invalid option argument */
            rstatus = SH_DRAW_OPT_INV;
            shErrStackPush("ERROR : Invalid number of parameters.\n");
            return(rstatus);
	 }
      }
   else
      {
      /* Point the next pointer at the parameter */
      ptr_params_ptr[*numparams] = argv[i];
      ++*numparams;
      }
   i++;						/* Go to next parameter */
   }

/*---------------------------------------------------------------------------
**
** RETURN
*/
return(SH_SUCCESS);

} /* drawArgsParse */



/*============================================================================  
**============================================================================
**
** ROUTINE: p_shSaoIndexCheck
*/
RET_CODE p_shSaoIndexCheck
  (
  int	*saoindex
  )   
/*
** DESCRIPTION:
**	Check the sao index to see if it is valid
**
** RETURN VALUES:
**	SH_SUCCESS
**	dervish_nofsao
**
** CALLED BY:
**	tclSaoResetDraw
**	tclSaoCircleDraw
**	tclSaoEllipseDraw
**	tclSaoBoxDraw
**	tclSaoPolygonDraw
**	tclSaoArrowDraw
**	tclSaoTextDraw
**	tclSaoPointDraw
**
** CALLS TO:
**
**============================================================================
*/
{ /* p_shSaoIndexCheck */

/* look for first saoindex that belongs to us */
++(*saoindex);			/* we started at -1 */
while (*saoindex < CMD_SAO_MAX)
   {
   if (g_saoCmdHandle.saoHandle[*saoindex].state == SAO_INUSE)
      {
      break;
      }
   ++(*saoindex);
   }
if (*saoindex == CMD_SAO_MAX)
   {
   /* No fSAO was found that belongs to this process */
   goto err_exit;
   }

/*---------------------------------------------------------------------------
**
** RETURN
*/
return(SH_SUCCESS);

err_exit:
return(SH_NO_FSAO);

} /* p_shSaoIndexCheck */




/*
**============================================================================
**============================================================================
**
** ROUTINE: tclSaoCircleDraw
**
**<AUTO>
** TCL VERB: saoDrawCircle
** 
** DESCRIPTION:
**  The saoCircleDraw TCl command enables the DERVISH user to place a circle of
**  user defined size on the FSAOimage display.  All 'glyphs' drawn on the
**  display by any saoDraw command can be saved into a file and then
**  redrawn on a future display.  This saving and restoring of glyphs is
**  done through the FSAOimage cursor read and write commands available on the
**  FSAOimage button panel.
**
** TCL SYNTAX:
**  saoDrawCircle  row column radius [...] [-ei] [-s num]
**     row      Row coordinates for center of circle. (May be a Tcl list.)
**     column   Column coordinates for center of circle. (May be a Tcl list.)
**     radius   Radius of the circle. (May be a Tcl list.)
**     [-e]     Mark this glyph as an exclude glyph.  Same as exclude
**              for SAOimage cursor regions.
**     [-i]     Mark this glyph as an include glyph.  Same as include
**              for SAOimage cursor regions. This is the default.
**     [-s]     Optional, FSAOimage program in which to draw glyph.
**              The default is to draw the glyph in the lowest
**              numbered FSAOimage program belonging to the current
**              DERVISH process.
**  The "..." in the above definition symbolizes additional radius values. 
**  When more than one radius value is specified, an annulus is drawn for each
**  radius centered at the row and column coordinates.  The resulting annulus
**  is treated as a single glyph.
** 
**  When any of the parameters are passed as a Tcl list, all of the parameters
**  must consist of Tcl lists with the same number or arguments.  The drawing
**  of the glyphs represented in the lists are treated as separate glyphs (not
**  annuli) butare drawn on the display all at once.  This method allows many
**  glyphs to be drawn without a display redraw in between each one.
**</AUTO>
**
** RETURN VALUES:
**  TCL_OK      Successful completion.
**  TCL_ERROR   Error occurred.   The Interp result string will contain the error
**		string.
**
** CALLED BY:
**	TCL command (saoGetNumber)
**
** CALLS TO:
**
**============================================================================
*/
static RET_CODE tclSaoCircleDraw
  (
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
  )   
{ /* tclSaoCircleDraw */

int    retStat = TCL_OK;
int    i;
RET_CODE rstatus;
int    saoindex = SAOINDEXINITVAL;
int    exclude_flag = FALSE;
int    annulus = FALSE;
int    params_found;
int    paramnum = 4;
int    elemsFound;		/* number of list elements found */
char   **ptr_params_ptr = 0;	/* pointer to list of parameter pointers */
char   ***ptr_list_ptr = 0;

/*---------------------------------------------------------------------------
**
** CHECK COMMAND ARGUMENTS
*/
if (argc < paramnum)
   {
   /* Wrong number of arguments given - offer some help */
   Tcl_SetResult(interp, TCLUSAGE_SAODRAWCIRCLE, TCL_VOLATILE);
   retStat = TCL_ERROR;
   }
else
   {
   ptr_params_ptr = (char **)alloca(((argc-1)*sizeof(char*)));
   if (ptr_params_ptr == 0)
      {
      Tcl_SetResult(interp,
		    "Unable to alloca space for parameter pointer list",
		    TCL_VOLATILE);
      retStat = TCL_ERROR;
      }
   else
      {
      /* Extract out the arguments */
      rstatus = drawArgsParse(argc, argv, ptr_params_ptr, &params_found,
			      &saoindex, &exclude_flag);
   
      shDebug(g_errorLevel, "Params_found,saoindex,exclude_flag = %d %d %d\n",
	      params_found,saoindex,exclude_flag);
   
      if (rstatus != SH_SUCCESS)
	 {
	 shTclInterpAppendWithErrStack(interp);	/* Append errors from stack */
	 if (rstatus == SH_DRAW_OPT_INV)
	   Tcl_AppendResult(interp, "\n", TCLUSAGE_SAODRAWCIRCLE,
			    (char *)NULL);
	 retStat = TCL_ERROR;
	 }
      else if (params_found < (paramnum - 1))  /* Don't count command itself */
	 {
	 Tcl_SetResult(interp, "Invalid number of parameters.", TCL_VOLATILE);
	 Tcl_AppendResult(interp, "\n", TCLUSAGE_SAODRAWCIRCLE, (char *)NULL);
	 retStat = TCL_ERROR;
	 }
      else
	 {
	 /* Now split the TCL lists into parameter lists - get space first */
	 ptr_list_ptr = (char ***)alloca(((params_found)*sizeof(char**)));
	 if (ptr_list_ptr == 0)
	    {
	    Tcl_SetResult(interp,
			 "Unable to alloca space for list parameter pointer",
			 TCL_VOLATILE);
	    retStat = TCL_ERROR;
	    }
	 else
	    {
	    rstatus = tclListArgsParse(interp, ptr_params_ptr, &params_found,
				       &elemsFound, ptr_list_ptr);
      
	    if (rstatus != SH_SUCCESS)
	       {
	       shTclInterpAppendWithErrStack(interp); /* Get errs from stack */
	       retStat = TCL_ERROR;
	       }
	    else
	       {
	       /* Check to see if this is an annulus - a circle within a
		  circle */
	       if (params_found > (paramnum-1))
		  {
		  /* There are more parameters than the x and y centers and the
		     radius.  The other parameters must be further radii, thus
		     this is an annulus. */
		  annulus = TRUE;
		  }
	       
	       /* construct the correct draw command and send it to fsao */
	       if ((rstatus = shSaoCircleDraw(saoindex, ptr_list_ptr,
					      elemsFound, params_found,
					      exclude_flag, annulus))
		   != SH_SUCCESS)
		  {
		  shTclInterpAppendWithErrStack(interp);
		  retStat = TCL_ERROR;
		  }
	       }
	    }
	 }
      }
   }
/*---------------------------------------------------------------------------
**
** RETURN
*/
/* We do not need to free the ptr_list_ptr itself as it was gotten by alloca */
if (ptr_list_ptr != 0)
   {
   for( i = 0 ; i < params_found ; ++i)
      free(ptr_list_ptr[i]);
   }

return(retStat);

} /* tclSaoCircleDraw */


/*
**============================================================================
**============================================================================
**
** ROUTINE: tclSaoEllipseDraw
**
**<AUTO>
** TCL VERB: saoDrawEllipse
**
** DESCRIPTION:
**  The saoEllipseDraw TCL command enables the DERVISH user to place a ellipse of
**  user defined size on the FSAOimage display.  All 'glyphs' drawn on the 
**  display by any saoDraw command can be saved into a file and then redrawn
**  on a future display.  This saving and restoring of glyphs is done through
**  the FSAOimage cursor read and write commands available on the FSAOimage
**  button panel.
**
** TCL SYNTAX:
**  saoDrawEllipse  row column rowradius colradius [...] angle [-ei] [-s num]
**     row       Row coordinates for center of ellipse. (May be a Tcl list.)
**     column    Column coordinates for center of ellipse. (May be a Tcl list.)
**     rowradius Number of rows for radius of ellipse. (May be a Tcl list.)
**     colradius Number of columns for radius of ellipse. (May be a Tcl list.)
**     angle     Rotation angle of ellipse. (May be a Tcl list.)
**     [-e]      Mark this glyph as an exclude glyph.  Same as exclude
**               for SAOimage cursor regions.
**     [-i]      Mark this glyph as an include glyph.  Same as include
**               for SAOimage cursor regions. This is the default.
**     [-s]      Optional, FSAOimage program in which to draw glyph.
**               The default is to draw the glyph in the lowest
**               numbered FSAOimage program belonging to the current
**               DERVISH process. 
**
**  The "..." in the above definition symbolizes additional pairs of rowradius
**  and colradius values.  When more than one pair of radius values is
**  specified, an ellipse is drawn for each pair centered at the row and column
**  coordinates.  The resulting series of ellipses is treated as a single glyph.
** 
**  When any of the parameters are passed as a Tcl list, all of the parameters
**  mustconsist of Tcl lists with the same number or arguments.  The drawing
**  of the glyphs represented in the lists are treated as separate glyphs (not
**  annuli) but are drawn on the display all at once.  This method allows many
**  glyphs to be drawn without a display redraw in between each one.
**</AUTO>
**
** RETURN VALUES:
**  TCL_OK       Successful completion.
**  TCL_ERROR    Error occurred.   The Interp result string will contain the
**               error string
**
** CALLED BY:
**	TCL command (saoGetNumber)
**
** CALLS TO:
**
**============================================================================
*/
static RET_CODE tclSaoEllipseDraw
  (
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
  )   
{ /* tclSaoEllipseDraw */

int    retStat = TCL_OK;
int    i;
RET_CODE rstatus;
int    saoindex = SAOINDEXINITVAL;
int    exclude_flag = FALSE;
int    annulus = FALSE;
int    paramnum = 6;
int    params_found;
int    elemsFound;              /* number of list elements found */
char   **ptr_params_ptr = 0;    /* pointer to list of parameter pointers */
char   ***ptr_list_ptr = 0;

/*---------------------------------------------------------------------------
**
** CHECK COMMAND ARGUMENTS
*/
if (argc < paramnum)
   {
   /* Wrong number of arguments given - offer some help */
   Tcl_SetResult(interp, TCLUSAGE_SAODRAWELLIPSE, TCL_VOLATILE);
   retStat = TCL_ERROR;
   }
else
   {
   ptr_params_ptr = (char **)alloca(((argc-1)*sizeof(char*)));
   if (ptr_params_ptr == 0)
      {
      Tcl_SetResult(interp,
		    "Unable to alloca space for parameter pointer list",
		    TCL_VOLATILE);
      retStat = TCL_ERROR;
      }
   else
      {
      /* Extract out the arguments */
      rstatus = drawArgsParse(argc, argv, ptr_params_ptr, &params_found,
			      &saoindex, &exclude_flag);
      
      shDebug(g_errorLevel, "Params_found,saoindex,exclude_flag = %d %d %d\n",
	      params_found,saoindex,exclude_flag);
      
      if (rstatus != SH_SUCCESS)
	 {
	 shTclInterpAppendWithErrStack(interp);	/* Append errors from stack */
	 if (rstatus == SH_DRAW_OPT_INV)
	   Tcl_AppendResult(interp, "\n", TCLUSAGE_SAODRAWELLIPSE,
			    (char *)NULL);
	 retStat = TCL_ERROR;
	 }
      else if (((params_found/2)*2) == params_found)
	 {
	 /* make sure has an odd number of parameters -
	    saoDrawEllipse row col rowrad1 colrad1 rowrad2 colrad2... angle */
	 Tcl_SetResult(interp, TCLUSAGE_SAODRAWELLIPSE, TCL_VOLATILE);
	 retStat = TCL_ERROR;
	 }
      else if (params_found < (paramnum - 1))  /* Don't count command itself */
	 {
	 Tcl_SetResult(interp, "Invalid number of parameters.", TCL_VOLATILE);
	 Tcl_AppendResult(interp, "\n", TCLUSAGE_SAODRAWELLIPSE, (char *)NULL);
	 retStat = TCL_ERROR;
	 }
      else
	 {
	 /* Now split the TCL lists into parameter lists - get space first */
	 ptr_list_ptr = (char ***)alloca(((params_found)*sizeof(char**)));
	 if (ptr_list_ptr == 0)
	    {
	    Tcl_SetResult(interp,
			  "Unable to alloca space for list parameter pointer",
			  TCL_VOLATILE);
	    retStat = TCL_ERROR;
	    }
	 else
	    {
	    rstatus = tclListArgsParse(interp, ptr_params_ptr, &params_found,
				       &elemsFound, ptr_list_ptr);
	    
	    if (rstatus != SH_SUCCESS)
	       {
	       shTclInterpAppendWithErrStack(interp); /* Get errs from stack */
	       retStat = TCL_ERROR;
	       }
	    else
	       {
	       /* Check to see if this is an annulus - a circle within a
		  circle */
	       if (params_found > (paramnum-1))
		  {
		  /* There are more parameters than the x and y centers and the
		     radius.  The other parameters must be further radii, thus
		     this is an annulus. */
		  annulus = TRUE;
		  }
	       
	       /* construct the correct draw command and send it to fsao */
	       if ((rstatus = shSaoEllipseDraw(saoindex, ptr_list_ptr,
					       elemsFound, params_found,
					       exclude_flag, annulus))
		   != SH_SUCCESS)
		  {
		  shTclInterpAppendWithErrStack(interp);
		  retStat = TCL_ERROR;
		  }
	       }
	    }
	 }
      }
   }

/*---------------------------------------------------------------------------
**
** RETURN
*/
/* We do not need to free the ptr_list_ptr itself as it was gotten by alloca */
if (ptr_list_ptr != 0)
   {
   for( i = 0 ; i < params_found ; ++i)
      free(ptr_list_ptr[i]);
   }

return(retStat);
} /* tclSaoEllipseDraw */

/*
**============================================================================
**============================================================================
**
** ROUTINE: tclSaoBoxDraw
**
**<AUTO>
** TCL VERB: saoDrawBox
**
** DESCRIPTION:
**  The saoBoxDraw TCL command enables the DERVISH user to place a box of  
**  user defined size on the FSAOimage display.  All 'glyphs' drawn on the
**  display by any saoDraw command can be saved into a file and then redrawn on
**  a future display.  This saving and restoring of glyphs is done through the
**  FSAOimage cursor read and write commands available on the FSAOimage button
**  panel. 
**
** TCL SYNTAX:
**  saoDrawBox  row column rowdiam coldiam [...] angle [-ei] [-s num]
**     row         Row coordinates for center of box. (May be a Tcl list.)
**     column      Column coordinates for center of box.(May be a Tcl list.)
**     rowdiam     Number of rows for diameter of box. (May be a Tcl list.)
**     coldiam     Number of columns for diameter of box. (May be a Tcl list).
**     angle       Rotation angle of box. (May be a Tcl list.)
**     [-e]        Mark this glyph as an exclude glyph.  Same as exclude
**                 for SAOimage cursor regions.
**     [-i]        Mark this glyph as an include glyph.  Same as include
**                 for SAOimage cursor regions. This is the default.
**     [-s]        Optional, FSAOimage program in which to draw glyph.
**                 The default is to draw the glyph in the lowest
**                 numbered FSAOimage program belonging to the current
**                 DERVISH process. 
**
**  The "..." in the above definition symbolizes additional pairs of rowdiam
**  and coldiam values.  When more than one pair of diameter values is
**  specified, a box is drawn for each pair centered at the row and column
**  coordinates.  The resulting series of boxes is treated as a single glyph.
** 
**  When any of the parameters are passed as a Tcl list, all of the parameters
**  mustconsist of Tcl lists with the same number or arguments.  The drawing
**  of the glyphs represented in the lists are treated as separate glyphs (not
**  annuli) but are drawn on the display all at once.  This method allows many
**  glyphs to be drawn without a display redraw in between each one.
**</AUTO>
** 
** RETURN VALUES:
**  TCL_OK         Successful completion.  
**  TCL_ERROR      Error occurred.  The Interp result string will contain the
**                 error string
**
** CALLED BY:
**	TCL command (saoGetNumber)
**
** CALLS TO:
**
**============================================================================
*/
static RET_CODE tclSaoBoxDraw
  (
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
  )   
{ /* tclSaoBoxDraw */

int    retStat = TCL_OK;
int    i;
RET_CODE rstatus;
int    saoindex = SAOINDEXINITVAL;
int    exclude_flag = FALSE;
int    annulus = FALSE;
int    paramnum = 6;
int    params_found;
int    elemsFound;              /* number of list elements found */
char   **ptr_params_ptr = 0;    /* pointer to list of parameter pointers */
char   ***ptr_list_ptr = 0;

/*---------------------------------------------------------------------------
**
** CHECK COMMAND ARGUMENTS
*/
if (argc < paramnum)
   {
   /* Wrong number of arguments given - offer some help */
   Tcl_SetResult(interp, TCLUSAGE_SAODRAWBOX, TCL_VOLATILE);
   retStat = TCL_ERROR;
   }
else
   {
   ptr_params_ptr = (char **)alloca(((argc-1)*sizeof(char*)));
   if (ptr_params_ptr == 0)
      {
      Tcl_SetResult(interp,
		    "Unable to alloca space for parameter pointer list",
		    TCL_VOLATILE);
      retStat = TCL_ERROR;
      }
   else
      {
      /* Extract out the arguments */
      rstatus = drawArgsParse(argc, argv, ptr_params_ptr, &params_found,
			      &saoindex, &exclude_flag);
      
      shDebug(g_errorLevel, "Params_found,saoindex,exclude_flag = %d %d %d\n",
	      params_found,saoindex,exclude_flag);
      
      if (rstatus != SH_SUCCESS)
	 {
	 shTclInterpAppendWithErrStack(interp);	/* Append errors from stack */
	 if (rstatus == SH_DRAW_OPT_INV)
	   Tcl_AppendResult(interp, "\n", TCLUSAGE_SAODRAWBOX, (char *)NULL);
	 retStat = TCL_ERROR;
	 }
      else if (((params_found/2)*2) == params_found)
	 {
	 /* make sure has an odd number of parameters -
	    saoDrawBox row col rowrad1 colrad1 rowrad2 colrad2... angle */
	 Tcl_SetResult(interp, TCLUSAGE_SAODRAWBOX, TCL_VOLATILE);
	 retStat = TCL_ERROR;
	 }
      else if (params_found < (paramnum - 1))  /* Don't count command itself */
	 {
	 Tcl_SetResult(interp, "Invalid number of parameters.", TCL_VOLATILE);
	 Tcl_AppendResult(interp,"\n", TCLUSAGE_SAODRAWBOX, (char *)NULL);
	 retStat = TCL_ERROR;
	 }
      else
	 {
	 /* Now split the TCL lists into parameter lists - get space first */
	 ptr_list_ptr = (char ***)alloca(((params_found)*sizeof(char**)));
	 if (ptr_list_ptr == 0)
	    {
	    Tcl_SetResult(interp,
			  "Unable to alloca space for list parameter pointer",
			  TCL_VOLATILE);
	    retStat = TCL_ERROR;
	    }
	 else
	    {
	    rstatus = tclListArgsParse(interp, ptr_params_ptr, &params_found,
				       &elemsFound, ptr_list_ptr);
	    
	    if (rstatus != SH_SUCCESS)
	       {
	       shTclInterpAppendWithErrStack(interp); /* Get errs from stack */
	       retStat = TCL_ERROR;
	       }
	    else
	       {
	       /* Check to see if this is an annulus - a circle within a
		  circle */
	       if (params_found > (paramnum-1))
		  {
		  /* There are more parameters than the x and y centers and the
		     radius.  The other parameters must be further radii, thus
		     this is an annulus. */
		  annulus = TRUE;
		  }
	       
	       /* construct the correct draw command and send it to fsao */
	       if ((rstatus = shSaoBoxDraw(saoindex, ptr_list_ptr,
					   elemsFound, params_found,
					   exclude_flag, annulus))
		   != SH_SUCCESS)
		  {
		  shTclInterpAppendWithErrStack(interp);
		  retStat = TCL_ERROR;
		  }
	       }
	    }
	 }
      }
   }
/*---------------------------------------------------------------------------
**
** RETURN
*/
/* We do not need to free the ptr_list_ptr itself as it was gotten by alloca */
if (ptr_list_ptr != 0)
   {
   for( i = 0 ; i < params_found ; ++i)
      free(ptr_list_ptr[i]);
   }

return(retStat);
} /* tclSaoBoxDraw */


/*
**============================================================================
**============================================================================
**
** ROUTINE: tclSaoPolygonDraw
**
**<AUTO>
** TCL VERB: saoDrawPolygon
**
** DESCRIPTION:
**  The saoPolygonDraw TCL command enables the dervish user to place a polygon of
**  user defined size on the FSAOimage display.  All 'glyphs' drawn on the  
**  display by any saoDraw command can saved into a file and then redrawn on a 
**  future display.  This saving and restoring of glyphs is done through the
**  FSAOimage cursor read and write commands available on the FSAOimage button
**  panel.
**
** TCL SYNTAX:
**  saoDrawPolygon  row column ... [-ei] [-s num]
**     row          Row coordinate for polygon. (May be a Tcl list.)
**     column       Column coordinate for polygon. (May be a Tcl list.)
**     ...          Additional row and column pairs specifying the polygon.
**                  The polygon will be closed automatically.
**     [-e]         Mark this glyph as an exclude glyph.  Same as exclude
**                  for SAOimage cursor regions.
**     [-i]         Mark this glyph as an include glyph.  Same as include
**                  for SAOimage cursor regions. This is the default.
**     [-s]         Optional, FSAOimage program in which to draw glyph.
**                  The default is to draw the glyph in the lowest
**                  numbered FSAOimage program belonging to the current
**                  DERVISH process. 
** 
**  When any of the parameters are passed as a Tcl list, all of the parameters
**  mustconsist of Tcl lists with the same number or arguments.  The drawing
**  of the glyphs represented in the lists are treated as separate glyphs
**  but are drawn on the display all at once.  This method allows many
**  glyphs to be drawn without a display redraw in between each one.
**</AUTO>
**
** RETURN VALUES:
**  TCL_OK         Successful completion.
**  TCL_ERROR      Error occurred.  The Interp result string will contain the
**                 error string.
**
** CALLED BY:
**  TCL command (saoGetNumber)
**
** CALLS TO:
**
**============================================================================
*/
static RET_CODE tclSaoPolygonDraw
  (
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
  )   
{ /* tclSaoPolygonDraw */

int    retStat = TCL_OK;
int    i;
RET_CODE rstatus;
int    saoindex = SAOINDEXINITVAL;
int    exclude_flag = FALSE;
int    paramnum = 3;
int    params_found;
int    elemsFound;              /* number of list elements found */
char   **ptr_params_ptr = 0;    /* pointer to list of parameter pointers */
char   ***ptr_list_ptr = 0;

/*---------------------------------------------------------------------------
**
** CHECK COMMAND ARGUMENTS
*/
if (argc < paramnum)
   {
   /* Wrong number of arguments given - offer some help */
   Tcl_SetResult(interp, TCLUSAGE_SAODRAWPOLYGON, TCL_VOLATILE);
   retStat = TCL_ERROR;
   }
else
   {
   ptr_params_ptr = (char **)alloca(((argc-1)*sizeof(char*)));
   if (ptr_params_ptr == 0)
      {
      Tcl_SetResult(interp,
		    "Unable to alloca space for parameter pointer list",
		    TCL_VOLATILE);
      retStat = TCL_ERROR;
      }
   else
      {
      /* Extract out the arguments */
      rstatus = drawArgsParse(argc, argv, ptr_params_ptr, &params_found,
			      &saoindex, &exclude_flag);
      
      shDebug(g_errorLevel, "Params_found,saoindex,exclude_flag = %d %d %d\n",
	      params_found,saoindex,exclude_flag);
      
      if (rstatus != SH_SUCCESS)
	 {
	 shTclInterpAppendWithErrStack(interp);	/* Append errors from stack */
	 if (rstatus == SH_DRAW_OPT_INV)
	   Tcl_AppendResult(interp, "\n", TCLUSAGE_SAODRAWPOLYGON,
			    (char *)NULL);
	 retStat = TCL_ERROR;
	 }
      else if (((params_found/2)*2) != params_found)
	 {
	 /* make sure has an even number of parameters -
	    saoDrawPolygon row col row1 col1 row2 col2... */
	 Tcl_SetResult(interp, TCLUSAGE_SAODRAWPOLYGON, TCL_VOLATILE);
	 retStat = TCL_ERROR;
	 }
      else if (params_found < (paramnum - 1))  /* Don't count command itself */
	 {
	 Tcl_SetResult(interp, "Invalid number of parameters.", TCL_VOLATILE);
	 Tcl_AppendResult(interp, "\n", TCLUSAGE_SAODRAWPOLYGON, (char *)NULL);
	 retStat = TCL_ERROR;
	 }
      else
	 {
	 /* Now split the TCL lists into parameter lists - get space first */
	 ptr_list_ptr = (char ***)alloca(((params_found)*sizeof(char**)));
	 if (ptr_list_ptr == 0)
	    {
	    Tcl_SetResult(interp,
			  "Unable to alloca space for list parameter pointer",
			  TCL_VOLATILE);
	    retStat = TCL_ERROR;
	    }
	 else
	    {
	    rstatus = tclListArgsParse(interp, ptr_params_ptr, &params_found,
				       &elemsFound, ptr_list_ptr);
	    
	    if (rstatus != SH_SUCCESS)
	       {
	       shTclInterpAppendWithErrStack(interp); /* Get errs from stack */
	       retStat = TCL_ERROR;
	       }
	    else
	       {
	       /* construct the correct draw command and send it to fsao */
	       if ((rstatus = shSaoPolygonDraw(saoindex, ptr_list_ptr,
					       elemsFound, params_found,
					       exclude_flag))
		   != SH_SUCCESS)
		  {
		  shTclInterpAppendWithErrStack(interp);
		  retStat = TCL_ERROR;
		  }
	       }
	    }
	 }
      }
   }
/*---------------------------------------------------------------------------
**
** RETURN
*/
/* We do not need to free the ptr_list_ptr itself as it was gotten by alloca */
if (ptr_list_ptr != 0)
   {
   for( i = 0 ; i < params_found ; ++i)
      free(ptr_list_ptr[i]);
   }

return(retStat);
} /* tclSaoPolygonDraw */

/*
**============================================================================
**============================================================================
**
** ROUTINE: tclSaoArrowDraw
**
**<AUTO>
** TCL VERB: saoDrawArrow
**
** DESCRIPTION:
**  The saoArrowDraw TCL command enables the DERVISH user to place an arrow of
**  user defined size on the FSAOimage display.  All 'glyphs' drawn on the
**  display by any saoDraw command can be saved into a file and then redrawn
**  on a future display.  This saving and restoring of glyphs is done through
**  the FSAOimage cursor read and write commands available on the FSAOimage
**  button panel.
**
** TCL SYNTAX:
**  saoDrawArrow  row column length angle [-ei] [-s num]
**    row       Row coordinates for center of arrow. (May be a Tcl list.)
**    column    Column coordinates for center of arrow. (May be a Tcl list.)
**    length    Length of arrow. (May be a Tcl list.)
**    angle     Rotation angle of arrow. (May be a Tcl list.)
**    [-e]      Mark this glyph as an exclude glyph.  Same as exclude
**              for SAOimage cursor regions.
**    [-i]      Mark this glyph as an include glyph.  Same as include
**              for SAOimage cursor regions. This is the default.
**    [-s]      Optional, FSAOimage program in which to draw glyph.
**              The default is to draw the glyph in the lowest
**              numbered FSAOimage program belonging to the current
**              DERVISH process.
**
**  When any of the parameters are passed as a Tcl list, all of the parameters
**  mustconsist of Tcl lists with the same number or arguments.  The drawing
**  of the glyphs represented in the lists are treated as separate glyphs but
**  are drawn on the display all at once.  This method allows many glyphs to
**  be drawn without a display redraw in between each one.
**</AUTO>
**
** RETURN VALUES:
**  TCL_OK      Successful completion.
**  TCL_ERROR   Error occurred.  The Interp result string will contain the 
**              error string.
**
** CALLED BY:
**  TCL command (saoGetNumber)
**
** CALLS TO:
**
**============================================================================
*/
static RET_CODE tclSaoArrowDraw
  (
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
  )   
{ /* tclSaoArrowDraw */

int    retStat = TCL_OK;
int    i;
RET_CODE rstatus;
int    saoindex = SAOINDEXINITVAL;
int    exclude_flag = FALSE;
int    paramnum = 5;
int    params_found;
int    elemsFound;              /* number of list elements found */
char   **ptr_params_ptr = 0;    /* pointer to list of parameter pointers */
char   ***ptr_list_ptr = 0;

/*---------------------------------------------------------------------------
**
** CHECK COMMAND ARGUMENTS
*/
if (argc < paramnum)
   {
   /* Wrong number of arguments given - offer some help */
   Tcl_SetResult(interp, TCLUSAGE_SAODRAWARROW, TCL_VOLATILE);
   retStat = TCL_ERROR;
   }
else
   {
   ptr_params_ptr = (char **)alloca(((argc-1)*sizeof(char*)));
   if (ptr_params_ptr == 0)
      {
      Tcl_SetResult(interp,
		    "Unable to alloca space for parameter pointer list",
		    TCL_VOLATILE);
      retStat = TCL_ERROR;
      }
   else
      {
      /* Extract out the arguments */
      rstatus = drawArgsParse(argc, argv, ptr_params_ptr, &params_found,
			      &saoindex, &exclude_flag);
      
      shDebug(g_errorLevel, "Params_found,saoindex,exclude_flag = %d %d %d\n",
	      params_found,saoindex,exclude_flag);
      
      if (rstatus != SH_SUCCESS)
	 {
	 shTclInterpAppendWithErrStack(interp);	/* Append errors from stack */
	 if (rstatus == SH_DRAW_OPT_INV)
	   Tcl_AppendResult(interp, "\n", TCLUSAGE_SAODRAWARROW, (char *)NULL);
	 retStat = TCL_ERROR;
	 }
      else if (params_found < (paramnum - 1))  /* Don't count command itself */
	 {
	 Tcl_SetResult(interp, "Invalid number of parameters.", TCL_VOLATILE);
	 Tcl_AppendResult(interp, "\n", TCLUSAGE_SAODRAWARROW, (char *)NULL);
	 retStat = TCL_ERROR;
	 }
      else
	 {
	 /* Now split the TCL lists into parameter lists - get space first */
	 ptr_list_ptr = (char ***)alloca(((params_found)*sizeof(char**)));
	 if (ptr_list_ptr == 0)
	    {
	    Tcl_SetResult(interp,
			  "Unable to alloca space for list parameter pointer",
			  TCL_VOLATILE);
	    retStat = TCL_ERROR;
	    }
	 else
	    {
	    rstatus = tclListArgsParse(interp, ptr_params_ptr, &params_found,
				       &elemsFound, ptr_list_ptr);
	    
	    if (rstatus != SH_SUCCESS)
	       {
	       shTclInterpAppendWithErrStack(interp); /* Get errs from stack */
	       retStat = TCL_ERROR;
	       }
	    else
	       {
	       /* construct the correct draw command and send it to fsao */
	       if ((rstatus = shSaoArrowDraw(saoindex, ptr_list_ptr,
					     elemsFound, params_found,
					     exclude_flag))
		   != SH_SUCCESS)
		  {
		  shTclInterpAppendWithErrStack(interp);
		  retStat = TCL_ERROR;
		  }
	       }
	    }
	 }
      }
   }
/*---------------------------------------------------------------------------
**
** RETURN
*/
/* We do not need to free the ptr_list_ptr itself as it was gotten by alloca */
if (ptr_list_ptr != 0)
   {
   for( i = 0 ; i < params_found ; ++i)
      free(ptr_list_ptr[i]);
   }

return(retStat);
} /* tclSaoArrowDraw */

/*
**============================================================================
**============================================================================
**
** ROUTINE: tclSaoTextDraw
**
**<AUTO>
** TCL VERB: saoDrawText
**
** DESCRIPTION:
**  The saoTextDraw TCl command enables the DERVISH user to place text on the
**  FSAOimage display.  All 'glyphs' drawn on the display by any saoDraw
**  command can be saved into a file and then redrawn on a future display.
**  This saving and restoring of glyphs is done through the FSAOimage cursor
**  read and write commands available on the FSAOimage button panel.
**
** TCL SYNTAX:
**  saoDrawText  row column text [-ei] [-s num]
**     row          Row coordinates for lower left corner of text start.
**                  (May be a Tcl list.)
**     column       Column coordinates for lower left corner of text start.
**                  (May be a Tcl list.)
**     text         The text, must be enclosed in quotation marks.
**                  (May be a Tcl list.)
**     [-e]         Mark this glyph as an exclude glyph.  Same as exclude
**                  for SAOimage cursor regions.
**     [-i]         Mark this glyph as an include glyph.  Same as include
**                  for SAOimage cursor regions. This is the default.
**     [-s]         Optional, FSAOimage program in which to draw glyph.
**                  The default is to draw the glyph in the lowest
**                  numbered FSAOimage program belonging to the current
**                  DERVISH process.
** 
**  When any of the parameters are passed as a Tcl list, all of the parameters
**  mustconsist of Tcl lists with the same number or arguments.  The drawing
**  of the glyphs represented in the lists are treated as separate glyphs
**  but are drawn on the display all at once.  This method allows many
**  glyphs to be drawn without a display redraw in between each one.
**</AUTO>
**
** RETURN VALUES:
**  TCL_OK          Successful completion.
**  TCL_ERROR       Error occurred.  The Interp result string will contain the
** 		    error string.
**
** CALLED BY:
**	TCL command (saoGetNumber)
**
** CALLS TO:
**
**============================================================================
*/
static RET_CODE tclSaoTextDraw
  (
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
  )   
{ /* tclSaoTextDraw */

int    retStat = TCL_OK;
int    i;
RET_CODE rstatus;
int    saoindex = SAOINDEXINITVAL;
int    exclude_flag = FALSE;
int    paramnum = 4;
int    params_found;
int    elemsFound;              /* number of list elements found */
char   **ptr_params_ptr = 0;    /* pointer to list of parameter pointers */
char   ***ptr_list_ptr = 0;

/*---------------------------------------------------------------------------
**
** CHECK COMMAND ARGUMENTS
*/
if (argc < paramnum)
   {
   /* Wrong number of arguments given - offer some help */
   Tcl_SetResult(interp, TCLUSAGE_SAODRAWTEXT, TCL_VOLATILE);
   retStat = TCL_ERROR;
   }
else
   {
   ptr_params_ptr = (char **)alloca(((argc-1)*sizeof(char*)));
   if (ptr_params_ptr == 0)
      {
      Tcl_SetResult(interp,
		    "Unable to alloca space for parameter pointer list",
		    TCL_VOLATILE);
      retStat = TCL_ERROR;
      }
   else
      {
      /* Extract out the arguments */
      rstatus = drawArgsParse(argc, argv, ptr_params_ptr, &params_found,
			      &saoindex, &exclude_flag);
      
      shDebug(g_errorLevel, "Params_found,saoindex,exclude_flag = %d %d %d\n",
	      params_found,saoindex,exclude_flag);
      
      if (rstatus != SH_SUCCESS)
	 {
	 shTclInterpAppendWithErrStack(interp);	/* Append errors from stack */
	 if (rstatus == SH_DRAW_OPT_INV)
	   Tcl_AppendResult(interp, "\n", TCLUSAGE_SAODRAWTEXT, (char *)NULL);
	 retStat = TCL_ERROR;
	 }
      else if (params_found < (paramnum - 1))  /* Don't count command itself */
	 {
	 Tcl_SetResult(interp, "Invalid number of parameters.", TCL_VOLATILE);
	 Tcl_AppendResult(interp, "\n", TCLUSAGE_SAODRAWTEXT, (char *)NULL);
	 retStat = TCL_ERROR;
	 }
      else
	 {
	 /* Now split the TCL lists into parameter lists - get space first */
	 ptr_list_ptr = (char ***)alloca(((params_found)*sizeof(char**)));
	 if (ptr_list_ptr == 0)
	    {
	    Tcl_SetResult(interp,
			  "Unable to alloca space for list parameter pointer",
			  TCL_VOLATILE);
	    retStat = TCL_ERROR;
	    }
	 else
	    {
	    rstatus = tclListArgsParse(interp, ptr_params_ptr, &params_found,
				       &elemsFound, ptr_list_ptr);
	 
	    if (rstatus != SH_SUCCESS)
	       {
	       shTclInterpAppendWithErrStack(interp); /* Get errs from stack */
	       retStat = TCL_ERROR;
	       }
	    else
	       {
	       /* construct the correct draw command and send it to fsao */
	       if ((rstatus = shSaoTextDraw(saoindex, ptr_list_ptr,
					    elemsFound, params_found,
					    exclude_flag))
		   != SH_SUCCESS)
		  {
		  shTclInterpAppendWithErrStack(interp);
		  retStat = TCL_ERROR;
		  }
	       }
	    }
	 }
      }
   }
/*---------------------------------------------------------------------------
**
** RETURN
*/
/* We do not need to free the ptr_list_ptr itself as it was gotten by alloca */
if (ptr_list_ptr != 0)
   {
   for( i = 0 ; i < params_found ; ++i)
      free(ptr_list_ptr[i]);
   }

return(retStat);
} /* tclSaoTextDraw */

/*
**============================================================================
**============================================================================
**
** ROUTINE: tclSaoPointDraw
**
** DESCRIPTION:
**  The saoPointDraw TCL command enables the DERVISH user to mark a point on the
**  FSAOimage display.   All 'glyphs' drawn on the display by any saoDraw
**  command can be saved into a file and then redrawn on a future display. This
**  saving and restoring of glyphs is done through the FSAOimage cursor read
**  and write commands available on the FSAOimage button panel.
** 
<AUTO>
** TCL VERB: saoDrawPoint
** 
** TCL SYNTAX:
**  saoDrawPoint  row column [...] [-ei] [-s num]
**     row          Row coordinates for point. (May be a Tcl list.)
**     column       Column coordinates for point. (May be a Tcl list.)
**     [-e]         Mark this glyph as an exclude glyph.  Same as exclude
**                  for SAOimage cursor regions.
**     [-i]         Mark this glyph as an include glyph.  Same as include
**                  for SAOimage cursor regions. This is the default.
**     [-s]         Optional, FSAOimage program in which to draw glyph.
**                  The default is to draw the glyph in the lowest
**                  numbered FSAOimage program belonging to the current
**                  DERVISH process.
**
**  The "..." in the above definition symbolizes additional pairs of row
**  and column values.  When more than one pair of values is
**  specified, a point is drawn for each pair of coordinates.  
**  Each point is treated as a separate glyph.
** 
**  When any of the parameters are passed as a Tcl list, all of the parameters
**  mustconsist of Tcl lists with the same number or arguments.  The drawing
**  of the glyphs represented in the lists are treated as separate glyphs (not
**  annuli) but are drawn on the display all at once.  This method allows many
**  glyphs to be drawn without a display redraw in between each one.
</AUTO>
**
** RETURN VALUES:
**	TCL_OK      Successful completion.
**	TCL_ERROR   Error occurred.  The Interp result string will contain the
**                  error string.
**
** CALLED BY:
**	TCL command (saoGetNumber)
**
** CALLS TO:
**
**============================================================================
*/
static RET_CODE tclSaoPointDraw
  (
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
  )   
{ /* tclSaoPointDraw */
int    retStat = TCL_OK;
RET_CODE rstatus;
int    i;
int    saoindex = SAOINDEXINITVAL;
int    exclude_flag = FALSE;
int    paramnum = 3;
int    params_found;
int    elemsFound;              /* number of list elements found */
char   **ptr_params_ptr = 0;    /* pointer to list of parameter pointers */
char   ***ptr_list_ptr = 0;

/*---------------------------------------------------------------------------
**
** CHECK COMMAND ARGUMENTS
*/
if (argc < paramnum)
   {
   /* Wrong number of arguments given - offer some help */
   Tcl_SetResult(interp, TCLUSAGE_SAODRAWPOINT, TCL_VOLATILE);
   retStat = TCL_ERROR;
   }
else
   {
   ptr_params_ptr = (char **)alloca(((argc-1)*sizeof(char*)));
   if (ptr_params_ptr == 0)
      {
      Tcl_SetResult(interp,
		    "Unable to alloca space for parameter pointer list",
		    TCL_VOLATILE);
      retStat = TCL_ERROR;
      }
   else
      {
      /* Extract out the arguments */
      rstatus = drawArgsParse(argc, argv, ptr_params_ptr, &params_found,
			      &saoindex, &exclude_flag);
      
      shDebug(g_errorLevel, "Params_found,saoindex,exclude_flag = %d %d %d\n",
	      params_found,saoindex,exclude_flag);
      
      if (rstatus != SH_SUCCESS)
	 {
	 shTclInterpAppendWithErrStack(interp);	/* Append errors from stack */
	 if (rstatus == SH_DRAW_OPT_INV)
	   Tcl_AppendResult(interp, "\n", TCLUSAGE_SAODRAWPOINT, (char *)NULL);
	 retStat = TCL_ERROR;
	 }
      else if (((params_found/2)*2) != params_found)
	 {
	 /* make sure has an even number of parameters -
	    saoDrawPoint row col row1 col1 row2 col2... */
	 Tcl_SetResult(interp, TCLUSAGE_SAODRAWPOINT, TCL_VOLATILE);
	 retStat = TCL_ERROR;
	 }
      else if (params_found < (paramnum - 1))  /* Don't count command itself */
	 {
	 Tcl_SetResult(interp, "Invalid number of parameters.", TCL_VOLATILE);
	 Tcl_AppendResult(interp, "\n", TCLUSAGE_SAODRAWPOINT, (char *)NULL);
	 retStat = TCL_ERROR;
	 }
      else
	 {
	 /* Now split the TCL lists into parameter lists - get space first */
	 ptr_list_ptr = (char ***)alloca(((params_found)*sizeof(char**)));
	 if (ptr_list_ptr == 0)
	    {
	    Tcl_SetResult(interp,
			  "Unable to alloca space for list parameter pointer",
			  TCL_VOLATILE);
	    retStat = TCL_ERROR;
	    }
	 else
	    {
	    rstatus = tclListArgsParse(interp, ptr_params_ptr, &params_found,
				       &elemsFound, ptr_list_ptr);
	    
	    if (rstatus != SH_SUCCESS)
	       {
	       shTclInterpAppendWithErrStack(interp); /* Get errs from stack */
	       retStat = TCL_ERROR;
	       }
	    else
	       {
	       /* construct the correct draw command and send it to fsao */
	       if ((rstatus = shSaoPointDraw(saoindex, ptr_list_ptr,
					    elemsFound, params_found,
					    exclude_flag))
		   != SH_SUCCESS)
		  {
		  shTclInterpAppendWithErrStack(interp);
		  retStat = TCL_ERROR;
		  }
	       }
	    }
	 }
      }
   }
/*---------------------------------------------------------------------------
**
** RETURN
*/
/* We do not need to free the ptr_list_ptr itself as it was gotten by alloca */
if (ptr_list_ptr != 0)
   {
   for( i = 0 ; i < params_found ; ++i)
      free(ptr_list_ptr[i]);
   }

return(retStat);

} /* tclSaoPointDraw */

/*
**============================================================================
**============================================================================
**
** ROUTINE: tclSaoResetDraw
**
**<AUTO>
** TCL VERB: saoReset
**
** DESCRIPTION:
**  The saoResetDraw TCL command enables the DERVISH user to reset the FSAOimage
**  display by erasing all the previously drawn glyphs.  This command performs
**  the same fuchtion as the SAOimage cursor reset button.
** 
** TCL SYNTAX:
**  saoReset  [-s num]
**     [-s]         Optional, FSAOimage program.  The default is to send
**                  the command to the lowest numbered FSAOimage program
**                  belonging to the current DERVISH process. 
**</AUTO>
**
** RETURN VALUES:
**  TCL_OK          Successful completion.
**  TCL_ERROR       Error occurred.  The Interp result string will contain the 
**                  error string.
**
** CALLED BY:
**	TCL command (saoReset)
**
** CALLS TO:
**
**============================================================================
*/
static RET_CODE tclSaoResetDraw
  (
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
  )   
{ /* tclSaoResetDraw */
int      retStat = TCL_OK;
int      saoindex = SAOINDEXINITVAL;

/*---------------------------------------------------------------------------
**
** CHECK COMMAND ARGUMENTS
*/
if (argc > 2)
   {
   /* Wrong number of arguments */
   Tcl_SetResult(interp, TCLUSAGE_SAORESET, TCL_VOLATILE);
   retStat = TCL_ERROR;
   }
else
   {
   if (argc == 2)
      {
      /* The argument is the saoindex number  - we hope */
      saoindex = atoi(argv[1]) - 1;	/* Index starts at 0 not 1 */
      }
   
   /* send the command to fsao */
   if (shSaoResetDraw(saoindex) != SH_SUCCESS)
      {
      shTclInterpAppendWithErrStack(interp);
      retStat = TCL_ERROR;
      }
   }

return(retStat);
} /* tclSaoResetDraw */


/*
**============================================================================
**============================================================================
**
** ROUTINE: tclSaoLabel
**
**<AUTO>
** TCL VERB: saoLabel
**
** DESCRIPTION:
**  The saoLabel TCL command enables the DERVISH user to toggle the displaying
**  of the labels for drawn glyphs on the FSAOimage display.  This command
**  performs the same function as the SAOimage cursor label button.  If no
**  parameters are passed, the saoLabel command acts as a toggle.
**
** TCL SYNTAX:
**  saoLabel  [on] [off] [-s num]
**     on           Turns the labeling on
**     off          Turns the labeling off
**     [-s]         Optional, FSAOimage program.  The default is to send
**                  the command to the lowest numbered FSAOimage program
**                  belonging to the current DERVISH process.
**</AUTO>
**
** RETURN VALUES:
**	TCL_OK      Successful completion.
**	TCL_ERROR   Error occurred.  The Interp result string will contain
**                  the error string.
**
** CALLED BY:
**	TCL command (saoReset)
**
** CALLS TO:
**
**============================================================================
*/
static RET_CODE tclSaoLabel
  (
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
  )   
{ /* tclSaoLabel */

int    retStat = TCL_OK;
int    i = 1;
int    saoindex = SAOINDEXINITVAL;
int    label_type = -1;			/* assume just do a toggle */

/*---------------------------------------------------------------------------
**
** CHECK COMMAND ARGUMENTS
*/
if (argc > 3)
   {
   /* Wrong number of arguments */
   Tcl_SetResult(interp, TCLUSAGE_SAOLABEL, TCL_VOLATILE);
   retStat = TCL_ERROR;
   }
else
   {
   if (argc > 1)
      {
      while (i < argc)
	 {
	 if (isdigit(argv[i][0]))
	    {
	    /* This is a digit, so will assume it is the saonumber */
	    saoindex = atoi(argv[i]) - 1;	/* Index starts at 0 not 1 */
	    }
	 else
	    {
	    /* Is not a digit so make sure it is either 'ON' or 'OFF' */
	    if (!strcmp(argv[i], "on"))
	       {
	       /* The label should be set ON */
	       label_type = 1;
	       }
	    else if (!strcmp(argv[i], "off"))
	       {
	       /* The label should be set OFF */
	       label_type = 0;
	       }
	    else
	       {
	       /* Invalid parameter */
	       Tcl_SetResult(interp, "Invalid command line parameter value.",
			     TCL_VOLATILE);
	       Tcl_AppendResult(interp, "\n", TCLUSAGE_SAOLABEL, (char *)NULL);
	       retStat = TCL_ERROR;
	       }
	    }
	 i++;					/* Go to next parameter */
	 }
      }

   if (retStat == TCL_OK)
      {
      /* send the command to fsao */
      if (shSaoLabel(saoindex, label_type) != SH_SUCCESS)
	 {
	 shTclInterpAppendWithErrStack(interp);
	 retStat = TCL_ERROR;
	 }
      }
   }

return(retStat);
} /* tclSaoLabel */

/*
**============================================================================
**============================================================================
**
** ROUTINE: tclSaoGlyph
**
**<AUTO>
** TCL VERB: saoGlyph
**
** DESCRIPTION:
**  The saoGlyph TCL command enables the DERVISH user to toggle the displaying
**  of the glyphs on the FSAOimage display. If no
**  parameters are passed, the saoGlyph command acts as a toggle.
**
** TCL SYNTAX:
**  saoGlyph  [on] [off] [-s num]
**     on           Turns the glyphs on
**     off          Turns the glyphs off
**     [-s]         Optional, FSAOimage program.  The default is to send
**                  the command to the lowest numbered FSAOimage program
**                  belonging to the current DERVISH process.
**</AUTO>
**
** RETURN VALUES:
**	TCL_OK      Successful completion.
**	TCL_ERROR   Error occurred.  The Interp result string will contain
**                  the error string.
**
** CALLED BY:
**	TCL command (saoGlyph)
**
** CALLS TO:
**
**============================================================================
*/
static RET_CODE tclSaoGlyph
  (
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
  )   
{ /* tclSaoGlyph */

int    retStat = TCL_OK;
int    i = 1;
int    saoindex = SAOINDEXINITVAL;
int    glyph_type = -1;			/* assume just do a toggle */

/*---------------------------------------------------------------------------
**
** CHECK COMMAND ARGUMENTS
*/
if (argc > 3)
   {
   /* Wrong number of arguments */
   Tcl_SetResult(interp, TCLUSAGE_SAOGLYPH, TCL_VOLATILE);
   retStat = TCL_ERROR;
   }
else
   {
   if (argc > 1)
      {
      while (i < argc)
	 {
	 if (isdigit(argv[i][0]))
	    {
	    /* This is a digit, so will assume it is the saonumber */
	    saoindex = atoi(argv[i]) - 1;	/* Index starts at 0 not 1 */
	    }
	 else
	    {
	    /* Is not a digit so make sure it is either 'ON' or 'OFF' */
	    if (!strcmp(argv[i], "on"))
	       {
	       /* The label should be set ON */
	       glyph_type = 1;
	       }
	    else if (!strcmp(argv[i], "off"))
	       {
	       /* The label should be set OFF */
	       glyph_type = 0;
	       }
	    else
	       {
	       /* Invalid parameter */
	       Tcl_SetResult(interp, "Invalid command line parameter value.",
			     TCL_VOLATILE);
	       Tcl_AppendResult(interp, "\n", TCLUSAGE_SAOGLYPH, (char *)NULL);
	       retStat = TCL_ERROR;
	       }
	    }
	 i++;					/* Go to next parameter */
	 }
      }

   if (retStat == TCL_OK)
      {
      /* send the command to fsao */
      if (shSaoGlyph(saoindex, glyph_type) != SH_SUCCESS)
	 {
	 shTclInterpAppendWithErrStack(interp);
	 retStat = TCL_ERROR;
	 }
      }
   }

return(retStat);
} /* tclSaoGlyph */

/*
 * ROUTINE:
 *   shTclSaoDrawDeclare
 *
 * CALL:
 *   (void) shTclSaoDrawDeclare(
 *             Tcl_Interp *interp
 *             CMD_HANDLE *cmdHandlePtr
 *          );
 *
 * DESCRIPTION:
 *   This routine makes availaible to rest of the world all of SAO's
 *   drawing verbs
 *
 * RETURN VALUE:
 *   Nothing
 */
void shTclSaoDrawDeclare(Tcl_Interp *interp)
{

   char *helpFacil = "shFsao";

   shTclDeclare(interp, "saoDrawCircle", (Tcl_CmdProc *) tclSaoCircleDraw,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                "Draw a circle on the FSAO display.", TCLUSAGE_SAODRAWCIRCLE);

   shTclDeclare(interp, "saoDrawEllipse", (Tcl_CmdProc *) tclSaoEllipseDraw,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                "Draw an ellipse on the FSAO display.", 
                TCLUSAGE_SAODRAWELLIPSE);
                
   shTclDeclare(interp, "saoDrawBox", (Tcl_CmdProc *) tclSaoBoxDraw,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                "Draw a box on the FSAO display.", TCLUSAGE_SAODRAWBOX);

   shTclDeclare(interp, "saoDrawPolygon", (Tcl_CmdProc *) tclSaoPolygonDraw,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                "Draw a polygon on the FSAO display.", TCLUSAGE_SAODRAWPOLYGON);

   shTclDeclare(interp, "saoDrawArrow", (Tcl_CmdProc *) tclSaoArrowDraw,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                "Draw an arrow on the FSAO display.", TCLUSAGE_SAODRAWARROW);

   shTclDeclare(interp, "saoDrawText", (Tcl_CmdProc *) tclSaoTextDraw,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                "Write text on the FSAO display.", TCLUSAGE_SAODRAWTEXT);

   shTclDeclare(interp, "saoDrawPoint", (Tcl_CmdProc *) tclSaoPointDraw,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                "Mark a point on the FSAO display.", TCLUSAGE_SAODRAWPOINT);

   shTclDeclare(interp, "saoReset", (Tcl_CmdProc *) tclSaoResetDraw,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                "Remove all drawn glyphs from the FSAO display.", 
                TCLUSAGE_SAORESET);

   shTclDeclare(interp, "saoLabel", (Tcl_CmdProc *) tclSaoLabel,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                "Toggle the labels of all glyphs on the FSAO display.", 
                TCLUSAGE_SAOLABEL);

   /* do not need this anymore. glyph speed up is accomplished in fsao
      itself.  however do not get rid of it as it may be useful in the
      future.
      shTclDeclare(interp, "saoGlyph", (Tcl_CmdProc *) tclSaoGlyph,
                   (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
		   "Toggle the glyphs on the FSAO display.", 
		   TCLUSAGE_SAOGLYPH);
   */
   return;
}
