/* Support for the CCDPARS structure */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dervish.h"
#include "phCcdpars.h"
#include "phUtils.h"


#define MAXSTRLEN    100

#if 0
   /* private functions */
static int
get_defect_type(char *str, int *type);

static int
get_defect_cval(char *str, int *cval);

#endif 


CCDDEFECT *
phCcddefectNew(void)
{
  CCDDEFECT *new = shMalloc(sizeof(CCDDEFECT));

  new->iRow = -1;
  new->iCol = -1;
  new->dfcol0 = -1;
  new->dfncol = -1;
  new->dftype = (DFTYPE)-1;
  new->dfaction = (DFACTION)-1;
  
  return(new);
}

void
phCcddefectDel(CCDDEFECT *ccddefect)
{
  shFree(ccddefect);
}

CCDPARS *
phCcdparsNew(void)
{
  int i;
  CCDPARS *new = shMalloc(sizeof(CCDPARS));

    new->iRow = -1;              
    new->iCol = -1;              
    new->filter = -1;            
    new->rowRef = -1;            
    new->colRef = -1;            
    new->xPos = -1.0;           
    new->yPos = -1.0;           
    new->rotation = -1.0;       
    new->nrows = 1;
    new->ncols = 1;
    new->nDataCol = -1;          
    new->nDataRow = -1;          
    new->amp0 = -1;              
    new->amp1 = -1;              
    new->sPrescan0 = -1;        
    new->nPrescan0 = -1;        
    new->sPrescan1 = -1;        
    new->nPrescan1 = -1;        
    new->sPostscan0 = -1;        
    new->nPostscan0 = -1;        
    new->sPostscan1 = -1;        
    new->nPostscan1 = -1;        
    new->sPreBias0 = -1;            
    new->nPreBias0 = -1;            
    new->sPreBias1 = -1;            
    new->nPreBias1 = -1;            
    new->sPostBias0 = -1;            
    new->nPostBias0 = -1;            
    new->sPostBias1 = -1;            
    new->nPostBias1 = -1;            
    new->sData0 = -1;            
    new->nData0 = -1;            
    new->sData1 = -1;            
    new->nData1 = -1;            
    new->biasLevel0 = -1.0;     
    new->biasLevel1 = -1.0;     
    new->readNoise0 = -1.0;     
    new->readNoise1 = -1.0;     
    new->gain0 = -1.0;          
    new->gain1 = -1.0;          
    new->fullWell0 = -1.0;      
    new->fullWell1 = -1.0;      
    new->xOff = -1.0;           
    new->yOff = -1.0;           
    new->rowOffset = -1.0;      
    new->frameOffset = -1.0;      
    new->rowBinning = new->colBinning = -1;
    new->ccdIdent = -1;          
    new->CCDDefect = NULL;        

  new->linearity_type = LINEAR_NONE;
  new->n_linear_coeffs0 = new->n_linear_coeffs1 = 0;
  for(i = 0; i < N_LINEAR_COEFFS; i++) {
     new->linear_coeffs0[i] = new->linear_coeffs1[i] = 0;
  }

  return(new);
}

void
phCcdparsDel(CCDPARS *ccdpars) 
{
  CCDDEFECT *defect;

  if(ccdpars == NULL) return;

  if (ccdpars->CCDDefect != NULL) {
    while ((defect = shChainElementRemByPos(ccdpars->CCDDefect, HEAD)) != NULL) {
      phCcddefectDel(defect);
    }
    shChainDel(ccdpars->CCDDefect);
  }
  shFree(ccdpars);
}


#if 0
/* Set the repair action based on teh defect type */
void 
phCCDdefectActionSet(CHAIN *defects) 
{

    CURSOR_T curs = shChainCursorNew(defects);
    CCDDEFECT *defect;

    defect = (CCDDEFECT *) shChainCursorSet(defects,curs,HEAD);
    while ((defect = (CCDDEFECT *)shChainWalk(defects,curs,NEXT)) != NULL) {
	defect->dfaction = DEFECT_ACTION(defect->dftype);
    }
    shChainCursorDel(defects,curs);
}

#endif
#if 0
/****************************************************************************
 * This function creates a new CCDPARS structure and fills it with the 
 * proper values, read from a parameter file.
 * 
 * Input: position of CCD in camera (row 1-6, col 1-5)
 *        "inipar" _must_ have been called already so that the proper
 *              CCD parameter file is ready to be read
 *
 * Output: a new CCD structure with fields filled, or NULL
 *
 */

CCDPARS *
phCcdparsFetchById(int row, int col)
{
   int i, ndefect, ival;
   char key[MAXSTRLEN], strval[MAXSTRLEN];
   CCDPARS *ccd;
   CCDDEFECT *defect;
   
   /* first, see if the given row, col is valid */
   sprintf(key, "ccd_%1d%1d_version", row, col);
   if (ftclGetParI(key, &ival) != TCL_OK) {
      shError("phCcdparsFetchById: invalid row, col (%d, %d)", row, col);
      return(NULL);
   }

   /* allocate a new CCDPARS structure */
   ccd = phCcdparsNew();
   ccd->ccd_location = 10*row + col;
   ccd->ccd_version = ival;

   /* read in the fields, one by one */
   sprintf(key, "ccd_%1d%1d_bscol1", row, col);
   if (ftclGetParI(key, &(ccd->bscol1)) != TCL_OK) {
      shError("phCcdparsFetchById: can't read bscol1 for ccd %d", 
	       ccd->ccd_location);
      phCcdparsDel(ccd);
      return(NULL);
   }
   sprintf(key, "ccd_%1d%1d_bncol1", row, col);
   if (ftclGetParI(key, &(ccd->bncol1)) != TCL_OK) {
      shError("phCcdparsFetchById: can't read bncol1 for ccd %d", 
	       ccd->ccd_location);
      phCcdparsDel(ccd);
      return(NULL);
   }
   sprintf(key, "ccd_%1d%1d_readnoise1", row, col);
   if (ftclGetParF(key, &(ccd->readnoise1)) != TCL_OK) {
      shError("phCcdparsFetchById: can't read readnoise1 for ccd %d", 
	       ccd->ccd_location);
      phCcdparsDel(ccd);
      return(NULL);
   }
   sprintf(key, "ccd_%1d%1d_gain1", row, col);
   if (ftclGetParF(key, &(ccd->gain1)) != TCL_OK) {
      shError("phCcdparsFetchById: can't read gain1 for ccd %d", 
	       ccd->ccd_location);
      phCcdparsDel(ccd);
      return(NULL);
   }
   sprintf(key, "ccd_%1d%1d_splitcol", row, col);
   if (ftclGetParI(key, &(ccd->splitcol)) != TCL_OK) {
      shError("phCcdparsFetchById: can't read splitcol for ccd %d", 
	       ccd->ccd_location);
      phCcdparsDel(ccd);
      return(NULL);
   }
   sprintf(key, "ccd_%1d%1d_bscol2", row, col);
   if (ftclGetParI(key, &(ccd->bscol2)) != TCL_OK) {
      shError("phCcdparsFetchById: can't read bscol2 for ccd %d", 
	       ccd->ccd_location);
      phCcdparsDel(ccd);
      return(NULL);
   }
   sprintf(key, "ccd_%1d%1d_bncol2", row, col);
   if (ftclGetParI(key, &(ccd->bncol2)) != TCL_OK) {
      shError("phCcdparsFetchById: can't read bncol2 for ccd %d", 
	       ccd->ccd_location);
      phCcdparsDel(ccd);
      return(NULL);
   }
   sprintf(key, "ccd_%1d%1d_readnoise2", row, col);
   if (ftclGetParF(key, &(ccd->readnoise2)) != TCL_OK) {
      shError("phCcdparsFetchById: can't read readnoise2 for ccd %d", 
	       ccd->ccd_location);
      phCcdparsDel(ccd);
      return(NULL);
   }
   sprintf(key, "ccd_%1d%1d_gain2", row, col);
   if (ftclGetParF(key, &(ccd->gain2)) != TCL_OK) {
      shError("phCcdparsFetchById: can't read gain2 for ccd %d", 
	       ccd->ccd_location);
      phCcdparsDel(ccd);
      return(NULL);
   }

   /* allocate a CCDDEFECT CHAIN, even if there are no defects; use an
      empty CHAIN, rather than a NULL pointer, in that case. */
   if ((ccd->CCDDefect = shChainNew("CCDDEFECT")) == NULL) {
      shError("phCcdparsFetchById: can't create CHAIN for defects");
      return(NULL);
   }

   /* now, figure out how many CCDDEFECT structures we'll need */
   sprintf(key, "ccd_%1d%1d_ndefect", row, col);
   if (ftclGetParI(key, &ndefect) != TCL_OK) {
      shError("phCcdparsFetchById: can't read ndefect for ccd %d", 
	       ccd->ccd_location);
      phCcdparsDel(ccd);
      return(NULL);
   }

   /* for each one, create a new CCDDEFECT structure, read it in, and
      add the new structure to the chain */
   for (i = 0; i < ndefect; i++) {
      defect = phCcddefectNew();

      /* get the defect type, and decode it */
      sprintf(key, "ccd_%1d%1d_dftype_%d", row, col, i+1);
      if (ftclGetParS(key, strval, MAXSTRLEN-1) != TCL_OK) {
         shError("phCcdparsFetchById: can't read dftype_%d for ccd %d", 
	       i+1, ccd->ccd_location);
         phCcdparsDel(ccd);
         return(NULL);
      }
      if (get_defect_type(strval, &(defect->defect_flag)) != SH_SUCCESS) {
         shError("phCcdparsFetchById: invalid dftype_%d for ccd %d ..%s..", 
	       i+1, ccd->ccd_location, strval);
         phCcdparsDel(ccd);
         return(NULL);
      }

      /* get the defect starting col */
      sprintf(key, "ccd_%1d%1d_dfscol_%d", row, col, i+1);
      if (ftclGetParI(key, &(defect->defect_scol)) != TCL_OK) {
         shError("phCcdparsFetchById: can't read dfscol_%d for ccd %d", 
	       i+1, ccd->ccd_location);
         phCcdparsDel(ccd);
         return(NULL);
      }

      /* get the defect correction value, and decode it */
      sprintf(key, "ccd_%1d%1d_dfcval_%d", row, col, i+1);
      if (ftclGetParS(key, strval, MAXSTRLEN-1) != TCL_OK) {
         shError("phCcdparsFetchById: can't read dfcval_%d for ccd %d", 
	       i+1, ccd->ccd_location);
         phCcdparsDel(ccd);
         return(NULL);
      }
      if (get_defect_cval(strval, &(defect->defect_cval)) != SH_SUCCESS) {
         shError("phCcdparsFetchById: invalid dfcval_%d for ccd %d ..%s..", 
	       i+1, ccd->ccd_location, strval);
         phCcdparsDel(ccd);
         return(NULL);
      }

      shChainElementAddByPos(ccd->CCDDefect, defect, 
	    "CCDDEFECT", TAIL, AFTER);
   }
/*
 * check that there weren't more than ndefect defects
 */
   sprintf(key, "ccd_%1d%1d_dftype_%d", row, col, ndefect + 1);
   if(ftclGetParS(key, strval, MAXSTRLEN-1) == TCL_OK) {
      shError("phCcdparsFetchById: "
	      "There are > ndefect == %d defects in the parameter file for CCD %d",
	      ndefect, ccd->ccd_location);
      phCcdparsDel(ccd);
      return(NULL);
   }
   /* all done! */
   return(ccd);
}
      

/**************************************************************************
 * translate a string name for a CCD defect into the equivalent #define
 * value.  place the value in the second argument.   
 *
 * return SH_SUCCESS if all goes well, or SH_GENERIC_ERROR if the
 *   given string isn't recognized.
 */

static int
get_defect_type(char *str, int *type)
{
   if (strcmp(str, "drkcur") == 0) {
      *type = DRKCUR;
   }
   else if (strcmp(str, "blkcol") == 0) {
      *type = BLKCOL;
   }
   else if (strcmp(str, "badblk") == 0) {
      *type = BADBLK;
   }
   else if (strcmp(str, "depcol") == 0) {
      *type = DEPCOL;
   }
   else if (strcmp(str, "tgpair") == 0) {
      *type = TGPAIR;
   }
   else if (strcmp(str, "hotcol") == 0) {
      *type = HOTCOL;
   }
   else if (strcmp(str, "ctecol") == 0) {
      *type = CTECOL;
   }
   else if (strcmp(str, "intrmd") == 0) {
      *type = INTRMD;
   }
   else {
      shError("get_defect_type: string ..%s.. not recognized", str);
      return(SH_GENERIC_ERROR);
   }

   return(SH_SUCCESS);
}


/**************************************************************************
 * translate a string name for a CCD corrective action into the equivalent 
 * #define value.  place the value in the second argument.   
 *
 * return SH_SUCCESS if all goes well, or SH_GENERIC_ERROR if the
 *   given string isn't recognized.
 */

static int
get_defect_cval(char *str, int *cval)
{
   if (strcmp(str, "badcol") == 0) {
      *cval = BADCOL;
   }
   else if (strcmp(str, "addcol") == 0) {
      *cval = ADDCOL;
   }
   else if (strcmp(str, "filcol") == 0) {
      *cval = FILCOL;
   }
   else {
      shError("get_defect_cval: string ..%s.. not recognized", str);
      return(SH_GENERIC_ERROR);
   }

   return(SH_SUCCESS);
}
      
#endif




