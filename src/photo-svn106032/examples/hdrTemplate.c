/*****************************************************************************
******************************************************************************
**
** FILE:
**	fileName.c
**
** ABSTRACT:
**	This file contains ...
**
** ENTRY POINT		SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
** routineName1		public  Modify the ... 
** routineName2		local	Communicate with ... 
** routineName3		local	Copy in the ... 
**
** ENVIRONMENT:
**	ANSI C.
**
** REQUIRED PRODUCTS:
**	productJoe
**	productShmoe
**
** AUTHORS:
**	Creation date:  May 7, 1993
**	George Jetson
**	Smokey Stover
**
******************************************************************************
******************************************************************************
*/
#include <aaa.h>
#include <bbb.h>
#include "ccc.h"

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
#define MOIBLY	32	    
#define PINCAMB	75

/*----------------------------------------------------------------------------
**
** GLOBAL VARIABLES
*/
static int g_myglobal;	    /* It's suggested to prefix all global variables
			       (if any) with a "g_" */



/*============================================================================  
**============================================================================
**
** ROUTINE: routineName1
**
** DESCRIPTION:
**	This is a template - not a real routine description. 
**	Delete sections that are not used (i.e., if there are no global
**	variables referenced, delete the GLOBALS_REFERENCED: line).
**	A suggestion when writing routines is to prefix all parameters with
**	an "a_", in order to easily distinguish them from local variables.
**
** RETURN VALUES:
**	0   -   Successful completion.
**	1   -   Failed miserably.
**
** CALLS TO:
**	routineName27
**	routineName36
**	routineName98
**
** GLOBALS REFERENCED:
**	g_myglobal
**
**============================================================================
*/
unsigned long routineName1
   (
   int		a_number,	/* IN : Total number to initialize */
   my_struct	*a_struc,	/* MOD: Structure to modify */
   long		*a_numIntrpts	/* OUT: Number of Ctrl C interrupts */
   )   
{
unsigned long	status;
char		*mychar;
.
.
.
return(1);
}
