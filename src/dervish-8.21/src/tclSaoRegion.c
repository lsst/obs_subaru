/*<AUTO>
 *
 * FILE:
 *   tclSaoRegion
 *
 * ABSTRACT:
 *   This file contains all routines that deal with regions and the SAO image
 *</AUTO>
 *
 * ENTRY POINT            SCOPE    DESCRIPTION
 * ----------------------------------------------------------------------------
 * tclSaoRegionList       local    TCL routine to display regions associated
 *                                    with SAO images
 * tclSaoRegionGet        local    TCL routine to return a TCL list of FSAO
 *                                    image numbers in use
 * displayRegion          local    Display region information
 * matchRegion            local    Match region name and TCL handle name
 * p_shSaoRegionSet       private  Set the region name and count in internal
 *                                    structure
 */

#include <stdio.h>
#include <string.h>  /* Prototypes of str*() functions */
#include <ctype.h>   /* Prototypes of toupper() and friends */
#include <stdlib.h>
#include <alloca.h>

#include "tcl.h"
#include "region.h"
#include "shCSao.h"
#include "shTclUtils.h"
#include "shCAssert.h"

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

/* External variable s referenced here */
extern SAOCMDHANDLE g_saoCmdHandle;

/*
 * Prototypes for functions used in this source file only
 */
static void displayRegion(CMD_SAOHANDLE *a_shl, int saoindex, int *first_time);
static int  tclSaoRegionList(ClientData, Tcl_Interp *, int, char **);
static int  matchRegion(CMD_SAOHANDLE *a_shl, const char *);
static int  tclSaoRegionGet(ClientData, Tcl_Interp *, int, char **);

static char *tclSaoRegionListHelp = "List regions currently displayed in FSAO";
static char *tclSaoRegionListUse =  "saoListRegion [regionName]";
/*
 * ROUTINE: 
 *   tclSaoRegionList
 *
 *<AUTO>
 * TCL VERB: saoListRegion
 *
 * DESCRIPTION:
 *  List the contents of the shared memory that keeps a record of which regions
 *  are loaded into which copies of FSAOimage.  saoListRegion will list region
 *  information for the current process only, or all processes running
 *  FSAOimage.  Information may also be obtained for a specific region or for
 *  all regions that are loaded into FSAOimage.  NOTE: no information is
 *  available for a region unless it is loaded into a currently running copy
 *  of FSAOimage.
 *
 * SYNTAX:
 *  saoListRegion [-a] [regname]
 *	[-a]    Optional switch. If present, information for all
 *		FSAOimages loaded with the named region will be       
 *		listed.  If no regionname is given, information for all      
 *		loaded regions will be listed.  If not given, only           
 *		loaded regions associated with the current process will
 *		be listed.
 *	regname	Optional argument. Name of loaded region for which
 *		information is requested.  If not given, information
 *		for all regions will be listed.
 *
 * OUTPUT:
 *	The information listed consists of the following -
 *
 *               Region Name     SAO Number      Process ID      Current?
 *               ========================================================
 *               reg0            1               1836            X
 *
 *                      The mark in the 'Current?' column means reg0 was
 *                      loaded into FSAOimage 1 by the current process.
 *
 * EXAMPLES:
 *	saoListRegion		List information about all regions loaded
 *				into FSAOimage and belonging to the current
 *				process.
 *	saoListRegion reg0	List information about a any reg0 that was
 *				loaded into FSAOimage by the current process.
 *	saoListRegion reg0 -a	List information about any reg0 that was
 *				loaded into FSAOimage by any process.
 *	saoListRegion -a	List information about all regions loaded
 *				into FSAOimage by any process.
 *</AUTO>
 *
 * RETURN VALUES:
 *	TCL_OK
 *	TCL_ERROR
 *
 */
static int
tclSaoRegionList(ClientData clientData, Tcl_Interp *interp, int argc, 
                 char *argv[])
{
int	    i;
int	do_all;		/* output all regions or only specified region info*/
int	first_time = TRUE; /* records # calls to SAOOutputRegion */
char    *region_name = NULL;	
int	got_match = 0;	/* keep track of number of regions matched */

/*---------------------------------------------------------------------------
**
** CHECK COMMAND ARGUMENTS
*/
if (argc == 1)
   {
   /* Output all the corresponding regions and sao indices for current pid */
   do_all = TRUE;
   }
else if (argc == 2)		
   {
   /* Output the specified region and sao index for the current pid */
   region_name = argv[1];	/* get the region name */
   do_all = FALSE;
   }
else
   {
   /* Wrong number of arguments */
   Tcl_SetResult(interp, tclSaoRegionListUse, TCL_VOLATILE);
   goto err_exit;
   }
   

/*---------------------------------------------------------------------------
**
** CYCLE THROUGH THE LIST OF REGIONS AND OUTPUT THE ONES ASKED FOR
*/
for (i=0; i < CMD_SAO_MAX; i++)
   {
   if (g_saoCmdHandle.saoHandle[i].state == SAO_INUSE)	/* Only look at ones in use */
      {
      if (do_all)			/* Get all regions */
         {
         ++got_match;
         displayRegion(&(g_saoCmdHandle.saoHandle[i]), (i+1), &first_time);
         }
      else				/* Get region that matches input name */
         {
         if (matchRegion(&(g_saoCmdHandle.saoHandle[i]), region_name))
            {
            ++got_match;
            displayRegion(&(g_saoCmdHandle.saoHandle[i]), (i+1), &first_time);
            }
         }
      }
   }

/* if no matches were found - put out a message */
if (got_match == 0)
   {
   /* No region name was entered */
   if (do_all)
      {
      Tcl_SetResult(interp, "Warning: No regions read in",
                    TCL_VOLATILE);
      goto err_exit;
      }

   else
      {
      /* Want regions belonging to any process */
      Tcl_SetResult(interp,
                    "Warning: No regions read in with this name", TCL_VOLATILE);
      goto err_exit;
      }
   }

/*---------------------------------------------------------------------------
**
** RETURN
*/
return(TCL_OK);

err_exit:
return(TCL_ERROR);

}

/*
 * ROUTINE:
 *   tclSaoRegionGet
 *
 * <AUTO>
 * TCL VERB: saoGetRegion
 *
 * DESCRIPTION:
 *   This extension returns a (possibly empty) Tcl list of FSAO numbers 
 *   currently in use. 
 *
 * SYNTAX:
 *   saoGetRegion
 *
 * RETURNS:
 *   TCL_OK : always, except if it cannot allocate memory, in which case there
 *            is no successful return. Tcl Interpreter will contain either an
 *            empty list, or a list of all FSAO numbers currently in use.
 * </AUTO>
 *   
 */
static int tclSaoRegionGet(ClientData clientData, Tcl_Interp *a_interp, 
                           int a_argc, char **a_argv)
{
   char *pHandles[CMD_SAO_MAX];
   char *temp;
   int  i, nHandles;
   
   Tcl_ResetResult(a_interp);

   for (i = nHandles = 0; i < CMD_SAO_MAX; i++)
        if (g_saoCmdHandle.saoHandle[i].state == SAO_INUSE)  {
            temp = (char *) alloca(5);
            pHandles[nHandles] = temp;   /* Need to do this for alloca */
            if (pHandles == NULL)
                shFatal("saoGetRegion: cannot allocate memory!!\n");
            sprintf(pHandles[nHandles], "%d", i+1);
            nHandles++;
        }
   pHandles[nHandles] = NULL;

   if (nHandles)  {
       Tcl_AppendResult(a_interp, Tcl_Merge(nHandles, pHandles), (char *) NULL);
   }
   
   return TCL_OK;
}

/*
 * ROUTINE: 
 *   displayRegion
 *
 * CALL:
 *   (static void) displayRegion(
 *                    CMD_SAOHANDLE *a_shl,  // IN : Cmd SAO handle (internal
 *                                           //          structure ptr)
 *                    int saoindex,    //IN : sao number this region belongs to
 *                    int *first_time  //MOD : records if this is the first time
 *                 );
 *
 * DESCRIPTION:
 *      Output the region information.
 *
 * RETURN VALUES:
 *      None.
 *
 */
static void 
displayRegion(CMD_SAOHANDLE *a_shl, int saoindex, int *first_time)
{
   /* If this is the first time, output the header and get our process id */
   if (*first_time == TRUE)  {
      *first_time = FALSE;
      printf("\nRegion Name\tSAO Number\n");
      printf("============================================\n");
   }
 
   /* Ouput the information stored in this handle for this region. */
 
   printf("%-16s%d\t\n", a_shl->region, saoindex);

   return;
}

/*
 * ROUTINE: 
 *   matchRegion
 *
 * CALL:
 *   (static int) matchRegion(
 *                   CMD_SAOHANDLE *a_shl,  // IN : Cmd SAO handle (internal
 *                                          //      structure ptr)
 *                   const char    *a_name  // IN : Region name (single 
 *                );                        //      character ASCII)
 *
 * DESCRIPTION:
 *	Match the region name and the name stored in the handle.
 *
 * RETURN VALUES:
 *	TRUE  if there is a match
 *	FALSE if no match
 */
static int
matchRegion(CMD_SAOHANDLE *a_shl, const char *a_name)
{
int	i = 0;

while (a_name[i] != '\0')		/* loop through the region name */
   {
   if (toupper(a_name[i]) != toupper(a_shl->region[i]))
      {
      /* The region names do not match */
      return(FALSE);
      }
   else
      {
      /* The region names do match so far */
      ++i;
      }
   }

if (a_shl->region[i] != '\0')
   {
   /* The stored region name does not end when the input name ends.  Thus they
      are not the same, there are extra letters. */
   return(FALSE);
   }

return(TRUE);
}

/*
 * ROUTINE: 
 *   p_shSaoRegionSet
 *
 * CALL:
 *   (void) p_shSaoRegionSet(
 *             CMD_SAOHANDLE *a_shl,  // MOD: Cmd SAO handle (internal struct*)
 *             char          *a_name  // IN : Region name
 *          );
 *
 * DESCRIPTION:
 *      Set the region name and the region count in the structure (handle)
 *      used by SAO routines.
 *
 * RETURN VALUES:
 *      None.
 *
 */
void 
p_shSaoRegionSet(CMD_SAOHANDLE *a_shl, char *a_name)
{
 
   strcpy (a_shl->region, a_name); /*???*/
 
   /* Get the current region count.  This counter is incremented each time the
    * region is loaded (or modified).  We will save a copy in the SAOhandle
    * structure and compare it with the current count whenever a mouse button
    * click occurs in SAOimage (we don't want to return a mouse button command
    * if the region was re-loaded.
    */
   /*??? a_shl->region_cnt = shRegionGetModCntr(a_name); */
}

/*
 * ROUTINE:
 *   shTclSaoRegionDeclare
 *
 * CALL:
 *   (void) shTclSaoRegionDeclare(
 *               Tcl_Interp *a_interp     // IN: TCL interpreter
 *               CMD_HANDLE *cmdHandlePtr // IN: Command handle pointer
 *          );
 *
 * DESCRIPTION:
 *   The world's access to SAO region function.
 *
 * RETURN VALUE:
 *   Nothing
 */
void
shTclSaoRegionDeclare(Tcl_Interp *a_interp)
{
   char *helpFacil = "shFsao";
 
   shTclDeclare(a_interp, "saoListRegion", (Tcl_CmdProc *) tclSaoRegionList,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                tclSaoRegionListHelp, tclSaoRegionListUse);
   shTclDeclare(a_interp, "saoGetRegion", (Tcl_CmdProc *) tclSaoRegionGet,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil, 
                "Return a TCL list containing FSAO numbers in use",
                "saoGetRegion");
 
   return;
}
