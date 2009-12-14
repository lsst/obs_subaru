/*
 * TCL support for STARC type in photo.
 *
 * ENTRY POINT		SCOPE
 * tclStarcDeclare	public	Declare all the verbs defined in this module
 *
 * REQUIRED PRODUCTS:
 *	FTCL		TCL + XTCL + Fermilab extensions
 *
 * Heidi Newberg
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "dervish.h"
#include "astrotools.h"
#include "phStarc.h"

static char *module = "phTclStructsFacil";    /* name of this set of code */

static int
parse_filters(char *string, int *nfilter, char ***name_array);

   /*
    * This array is set by "createMtstdIndex", used by "mtstd2starc"
    * and freed by "freeMtstdIndex".  We set it to NULL to denote
    * that it is un-set.
    */
static int *mtstd_filter_index = NULL;


/*****************************************************************************/
/*
 * return a handle to a new STARC
 */
static char *tclStarcNew_use =
  "USAGE: starcNew ncolor";
static char *tclStarcNew_hlp =
  "create a new STARC with ncolor magnitude fields";

static int
tclStarcNew(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE handle;
   char name[HANDLE_NAMELEN];
   int ncolors;

   shErrStackClear();

   if(argc != 2) {
      Tcl_SetResult(interp,tclStarcNew_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if (Tcl_GetInt(interp, argv[1], &ncolors)!=TCL_OK) return (TCL_ERROR);
/*
 * ok, get a handle for our new STARC
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   handle.ptr = phStarcNew(ncolors);
   handle.type = shTypeGetFromName("STARC");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new starc handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete a STARC
 */
static char *tclStarcDel_use =
  "USAGE: starcDel starc";
static char *tclStarcDel_hlp =
  "Delete a STARC";

static int
tclStarcDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *starc;
   char *opts = "starc";

   shErrStackClear();

   ftclParseSave("starcDel");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      starc = ftclGetStr("starc");
      if(p_shTclHandleAddrGet(interp,starc,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclStarcDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phStarcDel(handle->ptr);
   (void) p_shTclHandleDel(interp,starc);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Put the filter information in a STARC
 */
static char *tclStarcFilterSet_use =
  "USAGE: starcFilterSet starc filterlist";
static char *tclStarcFilterSet_hlp =
  "Put the filter information in a STARC";

static int
tclStarcFilterSet(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
  STARC *starc = NULL;
  char *string;
  char *opts = " starc filterlist";
  int l_argc, i;
  char **l_argv;

  shErrStackClear();

  ftclParseSave("starcFilterSet");
  if(ftclFullParseArg(opts,argc,argv) != 0) {
    string = ftclGetStr("starc");
    if(shTclAddrGetFromName(interp,string,(void**)&starc,"STARC"
)
      !=TCL_OK) {
      return(TCL_ERROR);
    }
    /* Make the ascii list accessible to C */
    if (Tcl_SplitList(interp, argv[2], &l_argc, &l_argv) == TCL_ERROR) {
      Tcl_SetResult(interp,"Error parsing list of filters",TCL_VOLATILE);
      return (TCL_ERROR);
    }
  } else {
      Tcl_SetResult(interp,tclStarcFilterSet_use,TCL_STATIC);
      return(TCL_ERROR);
   }
  if (l_argc!=starc->ncolors) {
    Tcl_SetResult(interp, "Number of filters doesn't match ncolors",
      TCL_VOLATILE);
    return (TCL_ERROR);
  }
  for (i=0;i<l_argc;i++) {
    strncpy(starc->mag[i]->passBand, l_argv[i], FILTER_MAXNAME);
  }
  Tcl_SetResult(interp, argv[1], TCL_VOLATILE);
  return(TCL_OK);
}




/**************************************************************************
 * given a handle to a MTSTD structure, and a time and filterstring, 
 * create a new STARC and set its field values accordingly.
 *
 * This version designed to work with .mtstd files created by the MT
 * pipeline as of 6/16/95, rather than with Stoughton's simulations.
 */

static char *tclMtstd2Starc_use =
  "USAGE: mtstd2Starc mtstd mjd ps_filters filters"; 
static char *tclMtstd2Starc_hlp =
  "Convert a MTSTD structure to a STARC structure, and return a handle to the new STARC.";

static int
tclMtstd2Starc(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   int i, nfilter, ps_nfilter;
   int number_filled;
   char *mtstdstr, *ps_filterstr, *filterstr, name[HANDLE_NAMELEN];
   char **ps_filternames, **filternames;   
   char *opts = "mtstd mjd ps_filters filters";
   double mjd;
   double lambda, lambdaerr, eta, etaerr;
   double ra2, dec2, dra1, dra2, dra3;
   double *magp, *magerrp;
   void *mtstd_ptr;
   STARC *starc_ptr;
   HANDLE *mtstdHandle, starcHandle;
   TYPE type;
   const SCHEMA *schema_ptr;
   SCHEMA_ELEM elem;

   shErrStackClear();

   if (mtstd_filter_index == NULL) {
      Tcl_SetResult(interp, "you must call createMtstdIndex first",
	       TCL_STATIC);
      return(TCL_ERROR);
   }

   ftclParseSave("mtstd2Starc");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      mtstdstr = ftclGetStr("mtstd");
      if (p_shTclHandleAddrGet(interp, mtstdstr, &mtstdHandle) != TCL_OK) {
         return(TCL_ERROR);
      }
      if (mtstdHandle->type != shTypeGetFromName("MTSTD")) {
	 Tcl_SetResult(interp, "arg is not an MTSTD", TCL_STATIC);
         return(TCL_ERROR);
      }
      mtstd_ptr = (void *) mtstdHandle->ptr;
      mjd = ftclGetDouble("mjd");
      filterstr = ftclGetStr("filters");
      parse_filters(filterstr, &nfilter, &filternames);
      ps_filterstr = ftclGetStr("ps_filters");
      parse_filters(ps_filterstr, &ps_nfilter, &ps_filternames);
   } else {
      Tcl_SetResult(interp,tclMtstd2Starc_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   /* 
    * make sure that there are at least as many passbands in the MTSTD
    * structure as we need (which are listed in "ps_filternames").
    */
   shAssert(ps_nfilter <= nfilter);


   /* use this to decode the MTSTD structure */
   schema_ptr = shSchemaGet("MTSTD");
   
   /* and create a new STARC structure */
   starc_ptr = phStarcNew(nfilter);

   /* copy the time information */
   starc_ptr->mjd = mjd;
   
   /*
    * now, deal with one field of the MTSTD structure at a time 
    */

   /* 
    * the "numberfilled" value, which tells us how many "mag" and "magErr"
    * and "passBand" elements have meaningful data.
    */
   for (i = 0; i < schema_ptr->nelem; i++) {
      elem = schema_ptr->elems[i];
      if (strcmp(elem.name, "numberfilled") == 0) {
	 break;
      }
   }
   shAssert(strcmp(elem.name, "numberfilled") == 0);
   shAssert(strcmp(elem.type, "INT") == 0);
   number_filled = (int) *((long *) shElemGet(mtstd_ptr, &elem, &type));
   shAssert(number_filled >= nfilter);
   

   /* the "lambda" value, which we place into "lambda" */
   for (i = 0; i < schema_ptr->nelem; i++) {
      elem = schema_ptr->elems[i];
      if (strcmp(elem.name, "lambda") == 0) {
	 break;
      }
   }
   shAssert(strcmp(elem.name, "lambda") == 0);
   shAssert(strcmp(elem.type, "DOUBLE") == 0);
   lambda = *((double *) shElemGet(mtstd_ptr, &elem, &type));

   /* next, the "lambdaerr" value, which we place into "lambdaerr"  */
   for (i = 0; i < schema_ptr->nelem; i++) {
      elem = schema_ptr->elems[i];
      if (strcmp(elem.name, "lambdaerr") == 0) {
	 break;
      }
   }
   shAssert(strcmp(elem.name, "lambdaerr") == 0);
   shAssert(strcmp(elem.type, "DOUBLE") == 0);
   lambdaerr = *((double *) shElemGet(mtstd_ptr, &elem, &type));

   /* next, the "eta" value, which we place into "eta" */
   for (i = 0; i < schema_ptr->nelem; i++) {
      elem = schema_ptr->elems[i];
      if (strcmp(elem.name, "eta") == 0) {
	 break;
      }
   }
   shAssert(strcmp(elem.name, "eta") == 0);
   shAssert(strcmp(elem.type, "DOUBLE") == 0);
   eta = *((double *) shElemGet(mtstd_ptr, &elem, &type));

   /* and the "etaerr" value, which we place into "etaerr" */
   for (i = 0; i < schema_ptr->nelem; i++) {
      elem = schema_ptr->elems[i];
      if (strcmp(elem.name, "etaerr") == 0) {
	 break;
      }
   }
   shAssert(strcmp(elem.name, "etaerr") == 0);
   shAssert(strcmp(elem.type, "DOUBLE") == 0);
   etaerr = *((double *) shElemGet(mtstd_ptr, &elem, &type));

   /* at this point, we can convert from Survey (lambda, eta) to (RA, Dec) */
   atSurveyToEq(lambda, eta, &(starc_ptr->ra), &(starc_ptr->dec));

#ifdef DEBUG
   printf("mtstd %4d  lambda eta  %9.5lf %9.5lf  RA Dec %9.5lf %9.5lf\n",
       starc_ptr->id, lambda, eta, starc_ptr->ra, starc_ptr->dec);
#endif

   /* 
    * we can also convert "lambdaerr" and "etaerr" to uncertainties in
    * RA and Dec; the way we do it is to add these values to the actual
    * "lambda" and "eta", convert to (RA, Dec), and subtract the resulting
    * position from the actual (RA, Dec) using true (lambda, eta).  We have
    * to be careful about wrapping around zeros when we do the comparison.
    * One way to try to catch coord wraparound is to try all three possible
    * combinations of
    *
    *                  ra1 - ra2              appropriate most of the time
    *                  (ra1 + 360) - ra2      appropriate if ra1=1, ra2 = 359
    *                  ra1 - (ra2 + 360)      appropriate if ra1=359, ra2 = 1
    *
    * and choose the smallest of the three differences as the true difference.
    */
   atSurveyToEq(lambda + lambdaerr, eta + etaerr, &ra2, &dec2);
   /* first, calculate "raerr" */
   dra1 = fabs(ra2 - starc_ptr->ra);
   dra2 = fabs(ra2 + 360.0 - starc_ptr->ra);
   dra3 = fabs(ra2 - (starc_ptr->ra + 360.0));
   starc_ptr->raErr = (dra1 < dra2 ? dra1 : dra2);
   starc_ptr->raErr = (starc_ptr->raErr < dra3 ? starc_ptr->raErr : dra3);
   /* now, calculate "decErr" -- this, unlike "raErr", can't wraparound */
   starc_ptr->decErr = fabs(dec2 - starc_ptr->dec);
   

   /* 
    * the next three fields, "mag", "magerr", and "passBand", are arrays.
    * There _may_ be more elements in each array than there are filters;
    * if so, then we should ignore all those extras.  We can use
    * the "number_filled" or "nfilter" values to tell us how many
    * of these fields are filled with valid data.  
    *
    * Let's use "nfilter".
    *
    * Moreover, we can use the "mtstd_filter_index[]" array to tell us which
    * element of the MTSTD structure belongs in which element of the
    * new STARC.  We can use it to ensure that the elements of the
    * STARC will be in the "official" order, and contain no extra passbands.
    */

   /* now, for the "mag" data. */
   for (i = 0; i < schema_ptr->nelem; i++) {
      elem = schema_ptr->elems[i];
      if (strcmp(elem.name, "mag") == 0) {
	 break;
      }
   }
   shAssert(strcmp(elem.name, "mag") == 0);
   shAssert(strcmp(elem.type, "DOUBLE") == 0);
   shAssert(elem.i_nelem >= nfilter);
   magp = (double *) shElemGet(mtstd_ptr, &elem, &type);
   for (i = 0; i < ps_nfilter; i++) {
      starc_ptr->mag[i]->mag = (float) magp[mtstd_filter_index[i]];
   }

   /* now, for the "magerr" data. */
   for (i = 0; i < schema_ptr->nelem; i++) {
      elem = schema_ptr->elems[i];
      if (strcmp(elem.name, "magerr") == 0) {
	 break;
      }
   }
   shAssert(strcmp(elem.name, "magerr") == 0);
   shAssert(strcmp(elem.type, "DOUBLE") == 0);
   shAssert(elem.i_nelem >= nfilter);
   magerrp = (double *) shElemGet(mtstd_ptr, &elem, &type);
   for (i = 0; i < ps_nfilter; i++) {
      starc_ptr->mag[i]->magErr = (float) magerrp[mtstd_filter_index[i]];
   }

   /* and now copy the "official" filternames into the "mag" structure */
   for (i = 0; i < ps_nfilter; i++) {
      strncpy(starc_ptr->mag[i]->passBand, ps_filternames[i], 
		     FILTER_MAXNAME - 1);
   }


   /* bind the new STARC to a handle */
   if (p_shTclHandleNew(interp, name) != TCL_OK) {
      phStarcDel(starc_ptr);
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   starcHandle.ptr = starc_ptr;
   starcHandle.type = shTypeGetFromName("STARC");
   if (p_shTclHandleAddrBind(interp, starcHandle, name) != TCL_OK) {
      phStarcDel(starc_ptr);
      Tcl_SetResult(interp, "can't bind to new STARC handle", TCL_STATIC);
      return(TCL_ERROR);
   }
   Tcl_SetResult(interp, name, TCL_VOLATILE);

   /* release memory we allocated herein */
   for (i = 0; i < nfilter; i++) {
      shFree(filternames[i]);
   }
   for (i = 0; i < ps_nfilter; i++) {
      shFree(ps_filternames[i]);
   }
   shFree(filternames);
   shFree(ps_filternames);

   return(TCL_OK);
}


static int
parse_filters(char *string, int *nfilter, char ***name_array)
{
   int i, pos, len, num;


   /* first, figure out how many filters there are in the string */
   len = strlen(string);
   num = 0;
   for (pos = 0; pos < len; pos++) {
      while ((string[pos] == ' ') && (pos < len)) {
	 pos++;
      }
      if (pos == len) {
	 break;
      }
      num++;
      while ((string[pos] != ' ') && (string[pos] != '\0')) {
	 pos++;
      }
   }

   /* at this point, we know there are "num" names in the string */
   *nfilter = num;
   *name_array = (char **) shMalloc(num*sizeof(char *));
   for (i = 0; i < num; i++) {
      (*name_array)[i] = (char *) shMalloc(FILTER_MAXNAME*sizeof(char));
   }
   pos = 0;
   for (i = 0; i < num; i++) {
      if (pos >= len) {
	 shError("parse_filters: ran out of values in string");
	 return(SH_GENERIC_ERROR);
      }
      if (sscanf(string + pos, "%s", (*name_array)[i]) != 1) {
	 shError("parse_filters: scanf failed");
	 return(SH_GENERIC_ERROR);
      }
      shAssert(strlen((*name_array)[i]) < FILTER_MAXNAME);
      /* skip to the end of this name */
      while ((string[pos] != ' ') && (string[pos] != '\0')) {
	 pos++;
      }
      /* and skip all blanks */
      while ((string[pos] == ' ') && (pos < len)) {
	 pos++;
      }
   }

   return(SH_SUCCESS);
}


   /* 
    * figure out which element of "filternames" corresponds to 
    * each element of "ps_filternames".  Fill the array "filter_index"
    * in the following way: the i'th element contains the index of the
    * i'th element of "ps_filternames" in "filternames".  Thus, if
    *
    *    ps_filternames[]       = "u" "g" "r" "i" "z"
    *    mtstd_filternames[]    = "u" "g" "i" "r" "q" "z"
    *
    * then we have
    *
    *    mtstd_filter_index[]   =  0   1   3   2   5
    *
    * this routine allocates the space for "mtstd_filter_index" array,
    * and fills its elements.  One must call "freeMtstdIndex" when
    * all finished.
    */

static char *tclCreateMtstdIndex_use =
  "USAGE: createMtstdIndex ps_filters mtstd_filters"; 
static char *tclCreateMtstdIndex_hlp =
  "Create a lookup table which converts the FILTERS listed in header of .mtstd file to the 'official' filterlist.";

static int
tclCreateMtstdIndex(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   int i, j;
   int mtstd_nfilter, ps_nfilter;
   char *ps_filterstr, *mtstd_filterstr;
   char **ps_filternames, **mtstd_filternames;   
   char *opts = "ps_filters mtstd_filters";

   shErrStackClear();

   ftclParseSave("createMtstdIndex");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      ps_filterstr = ftclGetStr("ps_filters");
      parse_filters(ps_filterstr, &ps_nfilter, &ps_filternames);
      mtstd_filterstr = ftclGetStr("mtstd_filters");
      parse_filters(mtstd_filterstr, &mtstd_nfilter, &mtstd_filternames);
   } else {
      Tcl_SetResult(interp,tclCreateMtstdIndex_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   shAssert(ps_nfilter <= mtstd_nfilter);
   mtstd_filter_index = (int *) shMalloc(ps_nfilter*sizeof(int));
   
   for (i = 0; i < ps_nfilter; i++) {
      mtstd_filter_index[i] = -1;
      for (j = 0; j < mtstd_nfilter; j++) {
	 if (strcmp(ps_filternames[i], mtstd_filternames[j]) == 0) {
	    mtstd_filter_index[i] = j;
	    break;
	 }
      }

      /*
       * if an "official" filter wasn't found in MTSTD set, then 
       * print error messages and assert! 
       */
      if (mtstd_filter_index[i] == -1) {
	 shFatal("create_filter_index: can't find filter %s in MTSTD list\n",
		     ps_filternames[i]);
      }
   }

   return(TCL_OK);
}


   /***********************************************************************
    * free the static var "mtstd_filter_index", and set it to NULL.
    * 
    * Inform the user if the variable is already NULL.
    */

static char *tclFreeMtstdIndex_use =
  "USAGE: freeMtstdIndex "; 
static char *tclFreeMtstdIndex_hlp =
  "Free a lookup table used to read in .mtstd files";

static int
tclFreeMtstdIndex(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   char *opts = "";

   shErrStackClear();

   ftclParseSave("freeMtstdIndex");
   if(ftclFullParseArg(opts,argc,argv) == 0) {
      Tcl_SetResult(interp,tclFreeMtstdIndex_use,TCL_STATIC);
      return(TCL_ERROR);
   }
   
   if (mtstd_filter_index == NULL) {
      Tcl_SetResult(interp, "table is not currently allocated",
	       TCL_STATIC);
      return(TCL_OK);
   }
   
   shFree(mtstd_filter_index);
   mtstd_filter_index = NULL;
   return(TCL_OK);
}



/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclStarcDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"starcNew",
                (Tcl_CmdProc *)tclStarcNew,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclStarcNew_hlp, tclStarcNew_use);

   shTclDeclare(interp,"starcDel",
                (Tcl_CmdProc *)tclStarcDel,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclStarcDel_hlp, tclStarcDel_use);

   shTclDeclare(interp,"starcFilterSet",
                (Tcl_CmdProc *)tclStarcFilterSet,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclStarcFilterSet_hlp, tclStarcFilterSet_use);

   shTclDeclare(interp,"mtstd2Starc",
                (Tcl_CmdProc *)tclMtstd2Starc,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclMtstd2Starc_hlp, tclMtstd2Starc_use);

   shTclDeclare(interp,"createMtstdIndex",
                (Tcl_CmdProc *)tclCreateMtstdIndex,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclCreateMtstdIndex_hlp, tclCreateMtstdIndex_use);

   shTclDeclare(interp,"freeMtstdIndex",
                (Tcl_CmdProc *)tclFreeMtstdIndex,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclFreeMtstdIndex_hlp, tclFreeMtstdIndex_use);
}



