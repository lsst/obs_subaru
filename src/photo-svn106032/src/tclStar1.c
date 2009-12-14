/*
 * TCL support for STAR1 type in photo.
 *
 * ENTRY POINT		SCOPE
 * tclStar1Declare	public	Declare all the verbs defined in this module
 *
 * REQUIRED PRODUCTS:
 *	FTCL		TCL + XTCL + Fermilab extensions
 *
 * Heidi Newberg
 * Nan Ellman
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "dervish.h"
#include "phDgpsf.h"
#include "phStar1.h"
#include "phUtils.h"

static char *module = "phTclStructsFacil";    /* name of this set of code */

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * TCL VERB: phPsChainDel
 *
 * DESCRIPTION:
 * Destroy a chain of PStamps and all the elements on it
 *
 * return: nothing
 *
 * </AUTO>
 */

void
phPsChainDel(
	       CHAIN *chain		/* chain of OBJCs to destroy */
	       )
{
    REGION *ps=NULL;
    int nel;

    shAssert(chain != NULL &&
	     shChainTypeGet(chain) == shTypeGetFromName("REGION"));

    nel = chain->nElements;
   
    while(--nel >= 0) {
	ps = shChainElementRemByPos(chain, nel);
	shRegDel(ps);
    }
    
    shChainDel(chain);
}

/*****************************************************************************/
/*
 * return a handle to a new STAR1
 */
static char *tclStar1New_use =
  "USAGE: star1New";
static char *tclStar1New_hlp =
  "create a new STAR1";

static int
tclStar1New(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE handle;
   char name[HANDLE_NAMELEN];

   shErrStackClear();

   if(argc != 1) {
      Tcl_SetResult(interp,tclStar1New_use,TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * ok, get a handle for our new STAR1
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   handle.ptr = phStar1New();
   handle.type = shTypeGetFromName("STAR1");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new star1 handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete a STAR1
 */
static char *tclStar1Del_use =
  "USAGE: star1Del star1";
static char *tclStar1Del_hlp =
  "Delete a STAR1";

static int
tclStar1Del(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *star1;
   char *opts = "star1";

   shErrStackClear();

   ftclParseSave("star1Del");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      star1 = ftclGetStr("star1");
      if(p_shTclHandleAddrGet(interp,star1,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclStar1Del_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phStar1Del(handle->ptr);
   (void) p_shTclHandleDel(interp,star1);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}


/*****************************************************************************/
/*
 * Relax a STAR1
 */
static char *tclStar1Relax_use =
  "USAGE: star1Relax star1";
static char *tclStar1Relax_hlp =
  "Delete some structures from STAR1 (those not needed for photometry)";

static int
tclStar1Relax(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *star1;
   char *opts = "star1";

   shErrStackClear();

   ftclParseSave("star1Relax");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      star1 = ftclGetStr("star1");
      if(p_shTclHandleAddrGet(interp,star1,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclStar1Relax_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phStar1Relax(handle->ptr);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * return a handle to a new STAR1PC
 */
static char *tclStar1pcNew_use =
  "USAGE: star1pcNew";
static char *tclStar1pcNew_hlp =
  "create a new STAR1PC";

static int
tclStar1pcNew(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE handle;
   char name[HANDLE_NAMELEN];

   shErrStackClear();

   if(argc != 1) {
      Tcl_SetResult(interp,tclStar1pcNew_use,TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * ok, get a handle for our new STAR1PC
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   handle.ptr = phStar1pcNew();
   handle.type = shTypeGetFromName("STAR1PC");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new star1pc handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete a STAR1PC
 */
static char *tclStar1pcDel_use =
  "USAGE: star1pcDel star1pc";
static char *tclStar1pcDel_hlp =
  "Delete a STAR1PC";

static int
tclStar1pcDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *star1pc;
   char *opts = "star1pc";

   shErrStackClear();

   ftclParseSave("star1pcDel");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      star1pc = ftclGetStr("star1pc");
      if(p_shTclHandleAddrGet(interp,star1pc,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclStar1pcDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phStar1pcDel(handle->ptr);
   (void) p_shTclHandleDel(interp,star1pc);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}



/*****************************************************************************/
/*
 * return a handle to a new FRAME_INFORMATION
 */
static char *tclFrameInformationNew_use =
  "USAGE: frameinformationNew size";
static char *tclFrameInformationNew_hlp =
  "create a new FRAME_INFORMATION where size is size of starflags";

static int
tclFrameInformationNew(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE handle;
   char name[HANDLE_NAMELEN];
   int size;

   shErrStackClear();

   if(argc != 2) {
      Tcl_SetResult(interp,tclFrameInformationNew_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if (Tcl_GetInt(interp, argv[1], &size)!=TCL_OK) return (TCL_ERROR);

/*
 * get a handle for new FRAME_INFORMATION
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   handle.ptr = phFrameInformationNew(size);
   handle.type = shTypeGetFromName("FRAME_INFORMATION");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new FRAME_INFORMATION handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete a FRAME_INFORMATION
 */
static char *tclFrameInformationDel_use =
  "USAGE: frameinformationDel star1";
static char *tclFrameInformationDel_hlp =
  "Delete a FRAME_INFORMATION";

static int
tclFrameInformationDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *frame_info;
   char *opts = "frame_info";

   shErrStackClear();

   ftclParseSave("frameinformationDel");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      frame_info = ftclGetStr("frame_info");
      if(p_shTclHandleAddrGet(interp,frame_info,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclFrameInformationDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phFrameInformationDel(handle->ptr);
   (void) p_shTclHandleDel(interp,frame_info);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}




/*****************************************************************************/
/*
 *  Call phStar1MakeAtPos to create a new STAR1 structure at a given position
 *  in a region.
 */
static char *tclStar1MakeAtPos_use =
  "USAGE: star1MakeAtPos reg row col regsize";
static char *tclStar1MakeAtPos_hlp =
  "Create a new STAR1 using data at the given position in the given region";

static int
tclStar1MakeAtPos(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   char *regstr, name[HANDLE_NAMELEN];
   char *opts = "reg row col regsize";
   int regsize;
   float row, col;
   HANDLE *handle, starHandle;
   STAR1 *star;

   shErrStackClear();

   ftclParseSave("star1MakeAtPos");
   if (ftclFullParseArg(opts,argc,argv) != 0) {
      regstr = ftclGetStr("reg");
      if (p_shTclHandleAddrGet(interp, regstr, &handle) != TCL_OK) {
         return(TCL_ERROR);
      }
      if (handle->type != shTypeGetFromName("REGION")) {
         Tcl_SetResult(interp, "tclStar1MakeAtPos: first arg is not a REGION",
                  TCL_STATIC);
         return(TCL_ERROR);
      }

      row = (float) ftclGetDouble("row");
      col = (float) ftclGetDouble("col");
      regsize = (int) ftclGetInt("regsize");
   } 
   else {
      Tcl_SetResult(interp, tclStar1MakeAtPos_use, TCL_STATIC);
      return(TCL_ERROR);
   }

   if ((star = phStar1MakeAtPos(handle->ptr, row, col, regsize)) == NULL) {
      Tcl_SetResult(interp, "tclStar1MakeAtPos: phStar1MakeAtPos returns with error",
              TCL_STATIC);
      return(TCL_ERROR);
   }
   if (p_shTclHandleNew(interp, name) != TCL_OK) {
      phStar1Del(star);
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   starHandle.ptr = star;
   starHandle.type = shTypeGetFromName("STAR1");
   if (p_shTclHandleAddrBind(interp, starHandle, name) != TCL_OK) {
      phStar1Del(star);
      Tcl_SetResult(interp, "can't bind to new STAR1 handle", TCL_STATIC);
      return(TCL_ERROR);
   }
   
   Tcl_SetResult(interp, name, TCL_VOLATILE);
   return(TCL_OK);
}


/*****************************************************************************
 *
 *  Call phStar1MakeFromReg to create a new STAR1 structure from a given
 *  region.
 */
static char *tclStar1MakeFromReg_use =
  "USAGE: star1MakeFromReg reg sky skyerr skysig fparams aperture psf_def_rad cr_min_s cr_min_e cr_cond3 cr_cond32 threshold sigma_multi sigma_badsky second_peak";
static char *tclStar1MakeFromReg_hlp =
  "Create a new STAR1 using data from the entire given region with the given sky and other values";

static int
tclStar1MakeFromReg(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   char *str, name[HANDLE_NAMELEN];
   char *opts = "reg sky skyerr skysig fparams aperture psf_def_rad threshold";
   float sky, skyerr, skysig, sigma_multi, sigma_badsky, second_peak, cr_min_s, cr_cond3, cr_cond32;
   int aperture, threshold, cr_min_e;

   HANDLE *handle, starHandle;
   FRAMEPARAMS *fparams;
   STAR1 *star;
   REGION *reg;

   shErrStackClear();

   ftclParseSave("star1MakeFromReg");
   if (ftclFullParseArg(opts,argc,argv) != 0) {
      str = ftclGetStr("reg");
      if (p_shTclHandleAddrGet(interp, str, &handle) != TCL_OK) {
         return(TCL_ERROR);
      }
      if (handle->type != shTypeGetFromName("REGION")) {
         Tcl_SetResult(interp, "tclStar1MakeFromReg: first arg is not a REGION",
                  TCL_STATIC);
         return(TCL_ERROR);
      }
      reg = handle->ptr;

      str = ftclGetStr("fparams");
      if (p_shTclHandleAddrGet(interp, str, &handle) != TCL_OK) {
         return(TCL_ERROR);
      }
      if (handle->type != shTypeGetFromName("FRAMEPARAMS")) {
         Tcl_SetResult(interp, "tclStar1MakeFromReg: "
		       "fourth arg is not a FRAMEPARAMS", TCL_STATIC);
         return(TCL_ERROR);
      }
      fparams = handle->ptr;

      sky = ftclGetDouble("sky");
      skyerr = ftclGetDouble("skyerr");
      skysig = ftclGetDouble("skysig");
      aperture = ftclGetInt("aperture");
      cr_min_s = ftclGetDouble("cr_min_s"); 
      cr_min_e = ftclGetInt("cr_min_e"); 
      cr_cond3 = ftclGetDouble("cr_cond3");
      cr_cond32 = ftclGetDouble("cr_cond32");
      threshold = ftclGetInt("threshold");
      sigma_multi = ftclGetDouble("sigma_multi");
      sigma_badsky = ftclGetDouble("sigma_badsky");
      second_peak = ftclGetDouble("second_peak");
   } else {
      Tcl_SetResult(interp, tclStar1MakeFromReg_use, TCL_STATIC);
      return(TCL_ERROR);
   }
   star = phStar1MakeFromReg(reg, 0,
			     sky, skyerr, skysig, fparams, NULL, NULL,
			     aperture, 
			     MAX(reg->nrow,reg->ncol),cr_min_s,cr_min_e,cr_cond3, 
			     cr_cond32,threshold,sigma_multi,sigma_badsky,second_peak);

   if (p_shTclHandleNew(interp, name) != TCL_OK) {
      phStar1Del(star);
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   starHandle.ptr = star;
   starHandle.type = shTypeGetFromName("STAR1");
   if (p_shTclHandleAddrBind(interp, starHandle, name) != TCL_OK) {
      phStar1Del(star);
      Tcl_SetResult(interp, "can't bind to new STAR1 handle", TCL_STATIC);
      return(TCL_ERROR);
   }
   
   Tcl_SetResult(interp, name, TCL_VOLATILE);
   return(TCL_OK);
}



/*****************************************************************************
 *  Given a structure containing the DA's version of a postage stamp,
 *  create a new REGION structure to hold the data in the postage stamp.
 *  Return a handle holding the new REGION.
 */
static char *tclDAStampToRegion_use = 
  "USAGE: DAStampToRegion DAstruct";
static char *tclDAStampToRegion_hlp = 
  "Translate a DA postage-stamp structure into a REGION.";

static int
tclDAStampToRegion(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   int i, npixels, nside, row, col;
   U16 *dap, *pixp, rowc, colc;
   void *dastruct_ptr;
   char *dastr, name[HANDLE_NAMELEN];
   char *opts = "DAstruct";
   TYPE type;
   const SCHEMA *schema_ptr;
   REGION *new;
   SCHEMA_ELEM elem;
   HANDLE *handle, regHandle;

   shErrStackClear();

   ftclParseSave("DAStampToRegion");
   if (ftclFullParseArg(opts,argc,argv) != 0) {
      dastr = ftclGetStr("DAstruct");
      if (p_shTclHandleAddrGet(interp, dastr, &handle) != TCL_OK) {
         return(TCL_ERROR);
      }
      if (handle->type != shTypeGetFromName("DASTRUCT")) {
         Tcl_SetResult(interp, "tclDAStampToRegion: arg is not a DASTRUCT",
                  TCL_STATIC);
         return(TCL_ERROR);
      }
      dastruct_ptr = handle->ptr;
      schema_ptr = shSchemaGet("DASTRUCT");
   
      /* find the "britstar" field of this structure */
      for (i = 0; i < schema_ptr->nelem; i++) {
	 elem = schema_ptr->elems[i];
	 if (strcmp(elem.name, "britstar") == 0) {
	    break;
	 }
      }
      shAssert(strcmp(elem.name, "britstar") == 0);

      /* check to make sure that the structure is a square */
      npixels = elem.i_nelem;
      nside = (int) sqrt(npixels);
      shAssert(nside*nside == npixels);

      /* create a new region and set its pixel values */
      new = shRegNew("DAStampToRegion", nside, nside, TYPE_U16);
      dap = (U16 *) shElemGet(dastruct_ptr, &elem, &type);
      for (row = 0; row < nside; row++) {
	 pixp = &(new->rows_u16[row][0]);
	 for (col = 0; col < nside; col++) {
	    *pixp++ = *dap++;
	 }
      }

      /* find the "bsline" field of this structure */
      for (i = 0; i < schema_ptr->nelem; i++) {
	 elem = schema_ptr->elems[i];
	 if (strcmp(elem.name, "bsline") == 0) {
	    break;
	 }
      }
      shAssert(strcmp(elem.name, "bsline") == 0);
      rowc = *((U16 *) shElemGet(dastruct_ptr, &elem, &type));
      new->row0 = rowc - nside/2;

      /* find the "bscol" field of this structure */
      for (i = 0; i < schema_ptr->nelem; i++) {
	 elem = schema_ptr->elems[i];
	 if (strcmp(elem.name, "bscol") == 0) {
	    break;
	 }
      }
      shAssert(strcmp(elem.name, "bscol") == 0);
      colc = *((U16 *) shElemGet(dastruct_ptr, &elem, &type));
      new->col0 = colc - nside/2;
      
      /* bind the new REGION to a handle */
      if (p_shTclHandleNew(interp, name) != TCL_OK) {
	 shRegDel(new);
         shTclInterpAppendWithErrStack(interp);
         return(TCL_ERROR);
      }
      regHandle.ptr = new;
      regHandle.type = shTypeGetFromName("REGION");
      if (p_shTclHandleAddrBind(interp, regHandle, name) != TCL_OK) {
         shRegDel(new);
         Tcl_SetResult(interp, "can't bind to new REGION handle", TCL_STATIC);
         return(TCL_ERROR);
      }

      Tcl_SetResult(interp, name, TCL_VOLATILE);
   }
   else {
      return(TCL_ERROR);
   }

   return(TCL_OK);
}


/*****************************************************************************
 *  Given a tblcol containing the DA's version of a postage stamps,
 *  create chain containing the postage stamps as REGIONs. 
 *  Return a handle to the CHAIN of stamps.
 */
static char *tclTblcolToPsChain_use = 
  "USAGE: TblcolToPsChain tblcol";
static char *tclTblcolToPsChain_hlp = 
  "Translate a tblcol of DA postage stamps to a CHAIN of REGIONS";

static int
tclTblcolToPsChain(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{

    int i,j;
    int nstamps, size, npix, nbytes, skip;
    U16 *rowc, *colc;
    U16 **stamp;
    char *str, name[HANDLE_NAMELEN];
    char *opts = "tblcol";
    int tblFldIdx;
    ARRAY *array;
    TBLFLD *tblFld;
    TBLCOL *tblcol = NULL;
    CHAIN *chain=shChainNew("REGION");
    REGION *ps;
    HANDLE  chainHandle;

    shErrStackClear();

   ftclParseSave("tblcolToPsChain");
   if (ftclFullParseArg(opts,argc,argv) != 0) {
      str = ftclGetStr("tblcol");
      if(shTclAddrGetFromName(interp,str,(void**)&tblcol,"TBLCOL")!=TCL_OK) {
	  return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclTblcolToPsChain_use,TCL_STATIC);
      return(TCL_ERROR);
   }

    /* This is temporary to try reading both old and new stamp files */
   if (shHdrGetInt(&tblcol->hdr,"BSCOUNT",&nstamps)==SH_SUCCESS) {
       printf("Assuming old ps files\n");

       for (i=0; i<nstamps; i++) {
	   shTblFldLoc(tblcol,-1,"britstar",0,SH_CASE_INSENSITIVE,&array,
		       &tblFld,&tblFldIdx);
	   stamp = ((U16 **)array->arrayPtr);
	   
	   /* Get size of region, assuming it is square */
	   /* array.dim[0] is the number of potential stamps so */
	   /* array.dim[1] is number of pixels/stamp. */

	   npix = array->dim[1];
	   size = (int) sqrt(npix);
	   shAssert(size*size == npix);       
	   ps = shRegNew("tblcolToPsChain", size, size, TYPE_U16);

	   skip=0;
	   nbytes = array->data.size * size;
	   for (j=0; j<size; j++) {
	       memcpy((void *)ps->rows_u16[j],(void *)(stamp[i]+skip),nbytes);
	       skip += size;
	   }

#if FLOATING_PHOTO
	   {
	      REGION *tmp = shRegNew(ps->name, size, size, TYPE_PIX);
	      shRegIntCopy(tmp, ps); shRegIntConstAdd(tmp, SOFT_BIAS, 0);
	      shRegDel(ps);
	      ps = tmp;
	   }
#endif
	   /* Get row0 and col0 */
       

	   shChainElementAddByPos(chain,ps,"REGION",TAIL,AFTER);
	   shTblFldLoc(tblcol,-1,"bsline",0,SH_CASE_INSENSITIVE,&array,
		       &tblFld,&tblFldIdx);
	   rowc = ((U16 *)array->arrayPtr);
	   ps->row0 = rowc[i] - size/2;

	   shTblFldLoc(tblcol,-1,"bscol",0,SH_CASE_INSENSITIVE,&array,
		       &tblFld,&tblFldIdx);
	   colc = ((U16 *)array->arrayPtr);
	   ps->col0 = colc[i] - size/2;
       }
   } else {
       /* We must be reading new fang file */

       nstamps = tblcol->rowCnt;
       for (i=0; i<nstamps; i++) {
	   shTblFldLoc(tblcol,-1,"pixelmap",0,SH_CASE_INSENSITIVE,&array,
		    &tblFld,&tblFldIdx);
	   stamp = ((U16 **)array->arrayPtr);

	   /* Get size of region, assuming it is square */
	   /* array.dim[0] is the number of potential stamps so */
	   /* array.dim[1] is number of pixels/stamp. */

	   npix = array->dim[1];
	   size = (int) sqrt(npix);
	   shAssert(size*size == npix);       
	   ps = shRegNew("tblcolToPsChain", size, size, TYPE_U16);
	   
	   skip=0;
	   nbytes = array->data.size * size;
	   for (j=0; j<size; j++) {
	      memcpy((void *)ps->rows_u16[j],(void *)(stamp[i]+skip),nbytes);
	      skip += size;
	   }

	   /* Get row0 and col0 */
#if FLOATING_PHOTO
	   {
	      REGION *tmp = shRegNew(ps->name, size, size, TYPE_PIX);
	      shRegIntCopy(tmp, ps); shRegIntConstAdd(tmp, SOFT_BIAS, 0);
	      shRegDel(ps);
	      ps = tmp;
	   }
#endif
       
	   shChainElementAddByPos(chain,ps,"REGION",TAIL,AFTER);
	   shTblFldLoc(tblcol,-1,"midrow",0,SH_CASE_INSENSITIVE,&array,
		       &tblFld,&tblFldIdx);
	   rowc = ((U16 *)array->arrayPtr);
	   ps->row0 = rowc[i] - size/2;

	   shTblFldLoc(tblcol,-1,"midcol",0,SH_CASE_INSENSITIVE,&array,
		       &tblFld,&tblFldIdx);
	   colc = ((U16 *)array->arrayPtr);
	   ps->col0 = colc[i] - size/2;
       }
   }
    /* bind the new CHAIN to a handle */
    if (p_shTclHandleNew(interp, name) != TCL_OK) {

	/* Delete the chain and its contents */
	phPsChainDel(chain);

	shTclInterpAppendWithErrStack(interp);
	return(TCL_ERROR);
    }
    chainHandle.ptr = chain;
    chainHandle.type = shTypeGetFromName("CHAIN");
    if (p_shTclHandleAddrBind(interp, chainHandle, name) != TCL_OK) {

	/* Delete the chain and its contents */
	phPsChainDel(chain);

	Tcl_SetResult(interp, "can't bind to new CHAIN handle", TCL_STATIC);
	return(TCL_ERROR);
    }
    
    Tcl_SetResult(interp, name, TCL_VOLATILE);
    return(TCL_OK);
}

/*****************************************************************************/
/*
 * return a handle to a new SYMDIAG
 */
static char *tclSymdiagNew_use =
  "USAGE: symdiag1New";
static char *tclSymdiagNew_hlp =
  "create a new SYMDIAG";

static int
tclSymdiagNew(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE handle;
   char name[HANDLE_NAMELEN];

   shErrStackClear();

   if(argc != 1) {
      Tcl_SetResult(interp,tclSymdiagNew_use,TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * ok, get a handle for our new SYMDIAG
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   handle.ptr = phSymdiagNew();
   handle.type = shTypeGetFromName("SYMDIAG");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new symdiag handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}
/*****************************************************************************/
/*
 * Delete a SYMDIAG
 */
static char *tclSymdiagDel_use =
  "USAGE: symdiagDel symdiag";
static char *tclSymdiagDel_hlp =
  "Delete a SYMDIAG";

static int
tclSymdiagDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *symdiag;
   char *opts = "symdiag";

   shErrStackClear();

   ftclParseSave("symdiagDel");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      symdiag = ftclGetStr("symdiag");
      if(p_shTclHandleAddrGet(interp,symdiag,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclSymdiagDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phSymdiagDel(handle->ptr);
   (void) p_shTclHandleDel(interp,symdiag);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}

/**************************************************************************
 * Declare my new tcl verbs to tcl
 */
void
phTclStar1Declare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"star1New",
                (Tcl_CmdProc *)tclStar1New,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclStar1New_hlp, tclStar1New_use);

   shTclDeclare(interp,"star1Del",
                (Tcl_CmdProc *)tclStar1Del,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclStar1Del_hlp, tclStar1Del_use);

   shTclDeclare(interp,"star1Relax",
                (Tcl_CmdProc *)tclStar1Relax,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclStar1Relax_hlp, tclStar1Relax_use);

   shTclDeclare(interp,"star1pcNew",
                (Tcl_CmdProc *)tclStar1pcNew,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclStar1pcNew_hlp, tclStar1pcNew_use);

   shTclDeclare(interp,"star1pcDel",
                (Tcl_CmdProc *)tclStar1pcDel,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclStar1pcDel_hlp, tclStar1pcDel_use);

   shTclDeclare(interp,"frameinformationNew",
                (Tcl_CmdProc *)tclFrameInformationNew,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclFrameInformationNew_hlp, tclFrameInformationNew_use);

   shTclDeclare(interp,"frameinformationDel",
                (Tcl_CmdProc *)tclFrameInformationDel,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclFrameInformationDel_hlp, tclFrameInformationDel_use);

   shTclDeclare(interp,"symdiagNew",
                (Tcl_CmdProc *)tclSymdiagNew,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclSymdiagNew_hlp, tclSymdiagNew_use);

   shTclDeclare(interp,"symdiagDel",
                (Tcl_CmdProc *)tclSymdiagDel,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclSymdiagDel_hlp, tclSymdiagDel_use);

   shTclDeclare(interp,"star1MakeAtPos",
                (Tcl_CmdProc *)tclStar1MakeAtPos,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclStar1MakeAtPos_hlp, tclStar1MakeAtPos_use);

   shTclDeclare(interp,"star1MakeFromReg",
                (Tcl_CmdProc *)tclStar1MakeFromReg,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclStar1MakeFromReg_hlp, tclStar1MakeFromReg_use);

#if 0
   shTclDeclare(interp,"star1MakeWithDgpsf",
                (Tcl_CmdProc *)tclStar1MakeWithDgpsf,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclStar1MakeWithDgpsf_hlp, tclStar1MakeWithDgpsf_use);
#endif

   shTclDeclare(interp,"DAStampToRegion",
                (Tcl_CmdProc *)tclDAStampToRegion,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclDAStampToRegion_hlp, tclDAStampToRegion_use);
   shTclDeclare(interp,"tblcolToPsChain",
                (Tcl_CmdProc *)tclTblcolToPsChain,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclTblcolToPsChain_hlp, tclTblcolToPsChain_use);
}

