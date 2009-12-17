/*++
 * FACILITY:	Dervish
 *
 * ABSTRACT:	ARRAY handling under Tcl.
 *
 *   Public
 *   --------------------------	------------------------------------------------
 *
 *   Private
 *   --------------------------	------------------------------------------------
 *   p_shTclListValRet		Return an array of values.
 *   p_shTclArrayPrimRet	Return an array of primitive types.
 *
 * ENVIRONMENT:	ANSI C.
 *		tclArray.c
 *
 * AUTHOR: 	Tom Nicinski, Creation date: 29-Nov-1993
 *--
 ******************************************************************************/

/******************************************************************************/
/*						.----------------------------.
 * Outstanding Issues / Problems / Etc.		|  Last Update: 12-Mar-1994  |
 *						`----------------------------'
 *
 * This section lists out all the current problems with ARRAY processing and any
 * suggested fixes and enhancements.  Please keep this section   C U R R E N T.
 *
 *- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * Routine:	p_shTclArrayPrimRet
 *
 * Problem:	If the ARRAY contains indirect data (nStar is non-zero), no
 *		data is returned.  The data must be directly inside the ARRAY
 *		data area.
 *
 * Solution:	If nStar indicates data indirection, it is possible to follow
 *		that indirection to the ultimate data and then print it.  Or,
 *		the pointer to the data (the information contained in the ARRAY)
 *		can be returned.  From the Tcl level, it is likely that the data
 *		at the end of the pointers is of interest, so this is the solu-
 *		tion that should be pursued.
 *
 *- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 ******************************************************************************/

/******************************************************************************/

#include	<alloca.h>
#include	<stdio.h>
#include	<string.h>

#include	"ftcl.h"

#include	"dervish_msg_c.h"
#include	"shTclHandle.h"
#include	"shCSchema.h"
#include	"shCAlign.h"
#include	"shCUtils.h"
#include	"shTclAlign.h"

#include	"shTclUtils.h"  		/* For shTclDeclare ()        */
#include	"shCSao.h"			/* For shTclVerbs.h           */
#include	"shTclVerbs.h"			/* Control C++ name mangling  */
						/* ... of shTclArrayDeclare ()*/
#include	"shTclArray.h"
#include	"shCArray.h"

/******************************************************************************/

/******************************************************************************/
/*
 * Tcl command declarations.
 */

#define	arrayCLI_Def(symbol, usage, syntax, helpText)\
   static char	*shTclArray##symbol##Name   =        "array" #symbol,\
		*shTclArray##symbol##Use    = "USAGE: array" #symbol " " usage,\
		*shTclArray##symbol##Syntax = syntax,\
		*shTclArray##symbol##Help   = helpText

/******************************************************************************/

/******************************************************************************/

int		 p_shTclListValRet
   (
   Tcl_Interp		 *interp,	/* INOUT: Tcl interpreter structure   */
   TYPE			  valuesType,	/* IN:    Object schema of values     */
                  char	 *values,	/* IN:    List of values passed       */
                  char	 *valuesEmpty,	/* IN:    Escape char indicating empty*/
                  int	  valCntMin,	/* IN:    Minimum values required     */
                  int	  valCntMax,	/* IN:    Maximum values permitted    */
   ARRAY_TRAILOP	  trailZero,	/* IN:    Zero unspecified values?    */
                  int	 *valCnt,	/* OUT:   Number  values passed       */
   void			 *val,		/* OUT:   Passed  values              */
   ARRAY_VALNONNEG	  valNonNeg,	/* IN:    Positive-only values?       */
   void			 *valMax,	/* IN:    Maximum values (optional)   */
                  int	  valMaxIdxLo,	/* IN:    Check starting at this index*/
                  int	  valMaxIdxHi,	/* IN:    Check up to       this index*/
                  char	 *valLabelName,	/* IN:    What is being passed        */
                  char	 *valLabelNames,/* IN:    What is being passed: plural*/
                  char	 *valLabel1st,	/* IN:    1st value is also this      */
                  int	  strLen1stMax,	/* IN:    STR: max strlen w/ null term*/
                  int	  strLenMax,	/* IN:    STR: max strlen w/ null term*/
   ARRAY_STRTRUNC	  strTrunc,	/* IN:    STR: Permit truncation?     */
                  int	 *strLenLast	/* IN:    STR: Length of last string  */
   )

/*++
 * ROUTINE:	 p_shTclListValRet
 *
 *	Parse a list of numbers, boolean values, or strings.
 *
 *	   o	A minimum and maximum number of values is enforced.
 *	   o	Numbers can be limited to positive values.
 *	   o	Some or all of the values can be constrained to be less than a
 *		maximum value (for numeric types only).
 *
 *	The numbers can be of varying size, specified implicitly by the object
 *	schema.
 *
 *	Strings are handled slightly differently:
 *
 *	   o	valCntMax is the maximum number of strings, not characters.
 *
 *	   o	valCnt is the number of strings read in, not characters.
 *
 *	   o	Each value (lcl_valuesList) is assigned to the start of a
 *		character string and is null-terminated.  The characters
 *		between values are NOT assigned consecutively (as for numeric
 *		or LOGICAL data).  The first string is handled specially, as
 *		it can start in the midst of a string (within the receiving
 *		array).
 *
 *	For example, an application of this routine would be to parse array
 *	indices or dimensions.  The valNonNeg and valLabel1st parameters are
 *	provided for such an application.  valNonNeg permits checking to force
 *	values to be greater than or equal to zero.  valLabel1st can be used
 *	by error messages to indicate more information about the 1st value.
 *
 * RETURN VALUES:
 *
 *   TCL_OK		Success.
 *			The interp result string is empty.
 *
 *   TCL_ERROR		Failure.
 *			No values were returned.  The interp result string
 *			contains an error message.
 *
 * SIDE EFFECTS:
 *
 *   o	valMax is ignored for LOGICAL object schemas.
 *
 *   o	Values are placed into the output array (val) as they're parsed.  An
 *	invalid value will cause parsing to stop, but any previous elements of
 *	the output array (val) will have been overwritten.
 *
 *	   o	The exception to this rule is for character strings.  If overly
 *		long strings cannot be truncated, all string lengths are tested
 *		prior to any copying to the output array.
 *
 * SYSTEM CONCERNS:
 *
 *   o	If valMax is passed, the caller must guarantee valCntMax members. Other-
 *	wise, it's possible to walk off the end of the array.  Furthermore, each
 *	element of valMax corresponds to val (thus, if valMaxIdxLo is non-zero,
 *	val[valMaxIdxLo] is compared against valMax[valMaxIdxLo]).
 *--
 ******************************************************************************/

   {	/* p_shTblListValRet */

   /*
    * Declarations.
    */

                  int	  lcl_status     = TCL_OK;
                  int	  lcl_tokCnt;
                  char	**lcl_valuesList = ((char **)0);  /* Tcl_Split result */
                  int	  lcl_valuesCnt  = 0;	/* # values passed            */
                  int	  lcl_valIdx;
                  char	  lcl_sign[1+1];
                  char	  lcl_char;
                  int	  lcl_int;		/* Used as temp for casting   */
                  int	 *lcl_strLen;
                  char	  lcl_format[32+1];	/* sscanf () format string    */
                  char	  lcl_num0[132+1];	/* Make this larger than any  */
                  char	  lcl_num1[132+1];	/* number and some characters */
						/* for generating error msgs. */

   TYPE			  lcl_schemaTypeUCHAR   = shTypeGetFromName ("UCHAR");
   TYPE			  lcl_schemaTypeCHAR    = shTypeGetFromName ( "CHAR");
   TYPE			  lcl_schemaTypeSCHAR	= shTypeGetFromName ("SCHAR");
   TYPE			  lcl_schemaTypeUSHORT  = shTypeGetFromName ("USHORT");
   TYPE			  lcl_schemaTypeSHORT   = shTypeGetFromName ( "SHORT");
   TYPE			  lcl_schemaTypeULONG   = shTypeGetFromName ("ULONG");
   TYPE			  lcl_schemaTypeLONG    = shTypeGetFromName ( "LONG");
   TYPE			  lcl_schemaTypeUINT    = shTypeGetFromName ("UINT");
   TYPE			  lcl_schemaTypeINT     = shTypeGetFromName ("INT");
   TYPE			  lcl_schemaTypeFLOAT   = shTypeGetFromName ("FLOAT");
   TYPE			  lcl_schemaTypeDOUBLE  = shTypeGetFromName ("DOUBLE");
   TYPE			  lcl_schemaTypeLOGICAL = shTypeGetFromName ("LOGICAL");
   TYPE			  lcl_schemaTypeSTR     = shTypeGetFromName ("STR");

                  char	 *lcl_nl = "";
   static         char	 *    nl = "\n";

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Determine whether we handle the data type.
    *
    *   o   Some data types override passed parameters.
    */

        if (valuesType == lcl_schemaTypeUCHAR)   { valNonNeg = SH_ARRAY_VAL_NON_NEG; }
   else if (valuesType == lcl_schemaTypeCHAR)    {                               }
   else if (valuesType == lcl_schemaTypeSCHAR)   {                               }
   else if (valuesType == lcl_schemaTypeUSHORT)  { valNonNeg = SH_ARRAY_VAL_NON_NEG; }
   else if (valuesType == lcl_schemaTypeSHORT)   {                               }
   else if (valuesType == lcl_schemaTypeULONG)   { valNonNeg = SH_ARRAY_VAL_NON_NEG; }
   else if (valuesType == lcl_schemaTypeLONG)    {                               }
   else if (valuesType == lcl_schemaTypeUINT)    { valNonNeg = SH_ARRAY_VAL_NON_NEG; }
   else if (valuesType == lcl_schemaTypeINT)     {                               }
   else if (valuesType == lcl_schemaTypeFLOAT)   {                               }
   else if (valuesType == lcl_schemaTypeDOUBLE)  {                               }
   else if (valuesType == lcl_schemaTypeLOGICAL) { valNonNeg = SH_ARRAY_VAL_ALL;    } /* For nice error messages (no "positive") */
   else if (valuesType == lcl_schemaTypeSTR)     {                               }
   else
      {
      lcl_status = TCL_ERROR;
      if (valCnt != ((int *)0)) { *valCnt = 0; }	/* No values returned */
      sprintf (lcl_num0, "%s", __FILE__);
      sprintf (lcl_num1, "%d", __LINE__ - 2);
      Tcl_AppendResult (interp, "Internal error: ", shNameGetFromType (valuesType), " not handled (",
                                 lcl_num0, " line ", lcl_num1, ")", ((char *)0));
      goto rtn_return;
      }

   /*
    * Start getting the values.
    */

   if (values != ((char *)0)) {
      if (Tcl_SplitList (interp, values, &lcl_valuesCnt, &lcl_valuesList) != TCL_OK)
         {
         lcl_status = TCL_ERROR;
         Tcl_AppendResult (interp, "Parsing error\n",                 ((char *)0));
         Tcl_AppendResult (interp, "Invalid list of ", valLabelNames, ((char *)0));
         goto rtn_return;
         }
   } else {
      lcl_valuesCnt = 0;
   }

   if ((lcl_valuesCnt == 1) && (valuesEmpty != ((char *)0))) /* Work around if*/
      {							/* caller used ftcl-  */
      if (strcmp (lcl_valuesList[0], valuesEmpty) == 0)	/* FullParseArgs() to */
         {					/* get values. It can't return*/
         lcl_valuesCnt = 0;			/* null string as default.  A */
         }					/* special default indicates  */
      }						/* no values were passed.     */

   /*
    * Check whether the correct number of values were passed.
    */

        if (lcl_valuesCnt < valCntMin)
      {
      lcl_status = TCL_ERROR;
      sprintf (lcl_num1, "%d", valCntMin);
      Tcl_AppendResult (interp, lcl_nl, "Too few ",  valLabelNames, " specified (",
                                         (valCntMin != valCntMax) ? "minimum of " : "", lcl_num1, " required)", ((char *)0));
                                lcl_nl = nl;
      goto rtn_return;
      }
   else if (lcl_valuesCnt > valCntMax)
      {
      lcl_status = TCL_ERROR;
      sprintf (lcl_num1, "%d", valCntMax);
      Tcl_AppendResult (interp, lcl_nl, "Too many ", valLabelNames, " specified (",
                                         (valCntMax != valCntMin) ? "maximum of " : "", lcl_num1,
                                         (valCntMax != valCntMin) ? " permitted)" : " required)", ((char *)0));
                                lcl_nl = nl;
      goto rtn_return;
      }

   if (lcl_valuesCnt == 0)
      {
      goto rtn_return;
      }

   /*
    * Check the values.
    *
    *   o   If indicated, check whether the values are positive.
    *
    *   o   Fill remaining dimensions with zero.
    *
    *   NOTE:   A macro is used to improve readability while allowing the data
    *           type test to take place outside the loop.
    *
    *              o   The only difference between lcl_valCheck/U is that the
    *                  "U" version is for unsigned data types.  No check is
    *                  made to see if the scanfed value is less than zero,
    *                  thus getting rid of any compilation warnings about
    *                  checking unsigned values for being less than 0.
    *
    *   NOTE:   sscanf () is used rather than Tcl_GetInt (), et. al to control
    *           the interp result string.  The format string has two components.
    *
    *              o   The primary conversion specifier.
    *
    *              o   The conversion specifier (" %c") makes sure that the
    *                  primary conversion terminated due to end-of-string
    *                  rather than an invalid character.
    */

   if (strlen (valLabel1st) > 0) { sprintf (lcl_num1, " (%s)", valLabel1st); }
                           else  {          lcl_num1[0] = ((char)0);         }

#define	lcl_valBad(typeType, typeLabel)					\
   lcl_status = TCL_ERROR;						\
   sprintf (lcl_num0, "%d", lcl_valIdx + 1);				\
   Tcl_AppendResult (interp, lcl_nl, "Bad ", lcl_num0, shNth (lcl_valIdx + 1)," ", valLabelName, (lcl_valIdx == 0) ? lcl_num1 : "",\
                                     ": must be a ", (valNonNeg) ? "positive " : "", typeLabel, ((char *)0));			\
                             lcl_nl = nl

#define	lcl_valZero(typeType)						\
   if (trailZero) {							\
      for ( ; lcl_valIdx < valCntMax; lcl_valIdx++)			\
         {								\
         ((typeType *)val)[lcl_valIdx] = ((typeType)0);			\
         }								\
      }

#define	lcl_valCheck(typeType, typeLabel)				\
   if ((lcl_tokCnt != 1) || (valNonNeg && (   (((typeType *)val)[lcl_valIdx] < ((typeType)0))					\
                                           || (sscanf (lcl_valuesList[lcl_valIdx], " %1[-]", lcl_sign) == 1))))			\
      {									\
      lcl_valBad (typeType, typeLabel);					\
      }									\
   else if (((typeType *)valMax) != ((typeType *)0)) if ((valMaxIdxLo <= lcl_valIdx) && (lcl_valIdx <= valMaxIdxHi))		\
                                                     if  (((typeType *)val)[lcl_valIdx] >= ((typeType *)valMax)[lcl_valIdx])	\
      {									\
      lcl_status = TCL_ERROR;						\
      sprintf (lcl_num0, "%d", lcl_valIdx + 1);				\
      Tcl_AppendResult (interp, lcl_nl, lcl_num0, shNth (lcl_valIdx + 1), " ", valLabelName,					\
                                         (lcl_valIdx == 0) ? lcl_num1 : "", " out of bounds", ((char *)0)); lcl_nl = nl;	\
      }
#define	lcl_valCheckU(typeType, typeLabel)				\
   if ((lcl_tokCnt != 1) || (valNonNeg && (				\
                                              (sscanf (lcl_valuesList[lcl_valIdx], " %1[-]", lcl_sign) == 1))))			\
      {									\
      lcl_valBad (typeType, typeLabel);					\
      }									\
   else if (((typeType *)valMax) != ((typeType *)0)) if ((valMaxIdxLo <= lcl_valIdx) && (lcl_valIdx <= valMaxIdxHi))		\
                                                     if  (((typeType *)val)[lcl_valIdx] >= ((typeType *)valMax)[lcl_valIdx])	\
      {									\
      lcl_status = TCL_ERROR;						\
      sprintf (lcl_num0, "%d", lcl_valIdx + 1);				\
      Tcl_AppendResult (interp, lcl_nl, lcl_num0, shNth (lcl_valIdx + 1), " ", valLabelName,					\
                                         (lcl_valIdx == 0) ? lcl_num1 : "", " out of bounds", ((char *)0)); lcl_nl = nl;	\
      }

#define	lcl_valGet(checkRtn, typeType, typeLabel, convSpec)		\
   sprintf (lcl_format, " %s %%c", convSpec);				\
   for (lcl_valIdx = 0; lcl_valIdx < lcl_valuesCnt; lcl_valIdx++)	\
      {									\
      lcl_tokCnt = sscanf (lcl_valuesList[lcl_valIdx], lcl_format, &((typeType *)val)[lcl_valIdx], &lcl_char);			\
      checkRtn (typeType, typeLabel);					\
      }									\
      lcl_valZero (typeType)

#define	lcl_valGetCast(checkRtn, typeType, typeLabel, convSpec, castVar)\
   sprintf (lcl_format, " %s %%c", convSpec);				\
   for (lcl_valIdx = 0; lcl_valIdx < lcl_valuesCnt; lcl_valIdx++)	\
      {									\
      lcl_tokCnt = sscanf (lcl_valuesList[lcl_valIdx], lcl_format, &castVar, &lcl_char);					\
      ((typeType *)val)[lcl_valIdx] = ((typeType)castVar);		\
      checkRtn (typeType, typeLabel);					\
      }									\
      lcl_valZero (typeType)

        if (valuesType == lcl_schemaTypeUCHAR)   { lcl_valGetCast (lcl_valCheckU, unsigned      char, "integer",  "%u", lcl_int); }
   else if (valuesType == lcl_schemaTypeCHAR)    { lcl_valGetCast (lcl_valCheck,
		     /* warning could be removed by */
		     /* changing to lcl_valCheckU because char is always unsigned */
                char, "integer",  "%d", lcl_int); }
   else if (valuesType == lcl_schemaTypeSCHAR)   { lcl_valGetCast (lcl_valCheck,    signed      char, "integer",  "%d", lcl_int); }
   else if (valuesType == lcl_schemaTypeUSHORT)  { lcl_valGet     (lcl_valCheckU, unsigned short int, "integer", "%hu"); }
   else if (valuesType == lcl_schemaTypeSHORT)   { lcl_valGet     (lcl_valCheck,           short int, "integer", "%hd"); }
   else if (valuesType == lcl_schemaTypeULONG)   { lcl_valGet     (lcl_valCheckU, unsigned long  int, "integer", "%lu"); }
   else if (valuesType == lcl_schemaTypeLONG)    { lcl_valGet     (lcl_valCheck,           long  int, "integer", "%ld"); }
   else if (valuesType == lcl_schemaTypeUINT)    { lcl_valGet     (lcl_valCheckU, unsigned       int, "integer",  "%u"); }
   else if (valuesType == lcl_schemaTypeINT)     { lcl_valGet     (lcl_valCheck,                 int, "integer",  "%d"); }
   else if (valuesType == lcl_schemaTypeFLOAT)   { lcl_valGet     (lcl_valCheck,  float,          "real number",  "%f"); }
   else if (valuesType == lcl_schemaTypeDOUBLE)  { lcl_valGet     (lcl_valCheck,  double,         "real number", "%lf"); }
   else if (valuesType == lcl_schemaTypeLOGICAL)
           {
           for (lcl_valIdx = 0; lcl_valIdx < lcl_valuesCnt; lcl_valIdx++)
              {
              if (shBooleanGet (lcl_valuesList[lcl_valIdx], &((LOGICAL *)val)[lcl_valIdx]) != SH_SUCCESS)
                 {
                 lcl_valBad (LOGICAL, "boolean");
                 }
              }
           lcl_valZero (LOGICAL);
           }
   else if (valuesType == lcl_schemaTypeSTR)
           {
           if ((lcl_strLen = ((int *)alloca (lcl_valuesCnt * sizeof (int)))) == ((int *)0))
              {
              lcl_status = TCL_ERROR;
              Tcl_AppendResult (interp, lcl_nl, "Unable to allocate temporary space for string lengths", ((char *)0)); lcl_nl = nl;
              goto rtn_return;
              }
           for (lcl_valIdx = 0; lcl_valIdx < lcl_valuesCnt; lcl_valIdx++)
              {
              lcl_strLen[lcl_valIdx] = strlen (lcl_valuesList[lcl_valIdx]);
              }
              if (lcl_strLen[0]          >= strLen1stMax)
                 {
                  lcl_strLen[0]           = strLen1stMax - 1;
                 if (!strTrunc)
                    {
                    lcl_status = TCL_ERROR;
                    Tcl_AppendResult (interp, lcl_nl,    "1",   shNth (             1), " ", valLabelName, lcl_num1, " too long",
                                     ((char *)0)); lcl_nl = nl;
                    }
                 }
           for (lcl_valIdx = 1; lcl_valIdx < lcl_valuesCnt; lcl_valIdx++)
              {
              if (lcl_strLen[lcl_valIdx] >= strLenMax) /* Don't forget null*/
                 {
                  lcl_strLen[lcl_valIdx]  = strLenMax - 1;	/* Protect us */
                 if (!strTrunc)
                    {
                    lcl_status = TCL_ERROR;
                    sprintf (lcl_num0, "%d", lcl_valIdx + 1);
                    Tcl_AppendResult (interp, lcl_nl, lcl_num0, shNth (lcl_valIdx + 1), " ", valLabelName,           " too long",
                                     ((char *)0)); lcl_nl = nl;
                    }
                 }
              }
           if (lcl_status != TCL_OK)
              {
              goto rtn_return;
              }

              memcpy (val, lcl_valuesList[         0], strLen1stMax - 1); ((char *)val)[strLen1stMax - 1] = ((char)0);
                      val = ((void *)(((char *)val) +  strLen1stMax));
           for (lcl_valIdx = 1; lcl_valIdx < lcl_valuesCnt; lcl_valIdx++)
              {
              memcpy (val, lcl_valuesList[lcl_valIdx], strLenMax    - 1); ((char *)val)[strLenMax    - 1] = ((char)0);
                      val = ((void *)(((char *)val) +  strLenMax));
              }

           if (strLenLast != ((int *)0)) { *strLenLast = lcl_strLen[lcl_valuesCnt-1] + 1; } /* + 1 for null-terminator */
           }

#undef	lcl_valBad
#undef	lcl_valZero
#undef	lcl_valCheck
#undef	lcl_valGet
#undef	lcl_valGetCast

   if (lcl_status != TCL_OK)
      {
      goto rtn_return;
      }

   /*
    * All done.
    */
   
rtn_return : ;

   if (lcl_status == TCL_OK)
      {
      if (valCnt != ((int *)0)) { *valCnt = lcl_valuesCnt; }
      }

   if (lcl_valuesList != ((char **)0)) { free (lcl_valuesList); }

   return (lcl_status);

   }	/* p_shTblListValRet */

/******************************************************************************/

int		 p_shTclArrayPrimRet
   (
   Tcl_Interp		 *interp,	/* INOUT: Tcl interpreter structure   */
   ARRAY		 *array,	/* IN:    Array descriptor            */
                  int	  level,	/* IN:    Level in array              */
   unsigned       char	**elemPtr,	/* INOUT: Pointer into array          */
            long  int	 *elemCnt,	/* INOUT: Number of elements to show  */
   unsigned       int	  depth		/* IN:    Must be zero from user call */
   )

/*++
 * ROUTINE:	 p_shTclArrayPrimRet
 *
 *	Return an array of primitive types as a Tcl list. If not all indices are
 *	passed (level is not the leaf level), then a list of lists (of lists of
 *	...) is returned to represent arrays of arrays of ...
 *
 * RETURN VALUES:
 *
 *   TCL_OK		Success.
 *			The interp result string contains the Tcl list of array
 *			elements.
 *
 *   TCL_ERROR		Failure.
 *			A data type that is not handled by this code was
 *			encountered.  The interp result string contains an
 *			error message.
 *
 * SIDE EFFECTS:
 *
 *   o	Character strings are returned as such.  If they contain unbalanced
 *	braces ({}), problems can occur when users try, with Tcl, to index into
 *	an array of character strings.
 *
 *   o	Indirect data, where nStar is non-zero, is not handled.
 *
 * SYSTEM CONCERNS:
 *
 *   o	No bounds checking is performed.
 *
 *   o	No checks are made on the element count (elemCnt) to return a sensible
 *	number of elements for an array.
 *
 *   o	This can be quite inefficient, with respect to speed and memory fragmen-
 *	tation, since Tcl_AppendResult () is used for each array element.
 *--
 ******************************************************************************/

   {	/* p_shTclArrayPrimRet */

   /*
    * Declarations.
    */

                  int	 lcl_status = TCL_OK;
   unsigned long  int	 lcl_elemIdx;
   SHALIGN_TYPE		 lcl_alignType;
            long  int	 lcl_charMax;
                  char	*lcl_prefixList = "";	/* Prefix Tcl list            */
                  char	*lcl_prefixElem = "";	/* Prefix array element       */

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   if (array->nStar != 0)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, "Indirect data (nStar != 0) is not handled", ((char *)0));
      goto rtn_return;
      }

   if (level < 0)
      {
       level = 0;				/* Fix up a bit               */
       depth++;					/* Force additional bracing   */
      }

   if (level < (array->dimCnt - 1))
      {
      /*
       * Recursively bounce down the array pointers till we get to leaf data.
       */

      for (lcl_elemIdx = 0; ((lcl_elemIdx < array->dim[level]) && (*elemCnt > 0)); lcl_elemIdx++)
         {
         if (depth > 0) { Tcl_AppendResult (interp, lcl_prefixList, "{", ((char *)0)); lcl_prefixList = " "; }
         if ((lcl_status = p_shTclArrayPrimRet (interp, array, level + 1, elemPtr, elemCnt, depth + 1)) != TCL_OK)
            {
            goto rtn_return;
            }
         if (depth > 0) { Tcl_AppendResult (interp,                 "}", ((char *)0)); }
         }
      }
   else  /* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */
      {
      /*
       * Return the actual data.
       */

      lcl_alignType = shAlignSchemaToAlignMap (array->data.schemaType);

      if (array->data.schemaType == shTypeGetFromName ("STR"))
         {
         if ((lcl_charMax = (*elemCnt < array->dim[level]) ? *elemCnt : array->dim[level]) > 0)
            {
            if ((lcl_status = p_shTclAlignPrimElemRet (interp, lcl_alignType, array->data.schemaType, lcl_charMax, *elemPtr, "")
                )          != TCL_OK)
                {
               goto rtn_return;
               }
            *elemPtr += lcl_charMax;
            *elemCnt -= lcl_charMax;
            }
         }
      else				/* Show data values as numbers        */
         {
         for (lcl_elemIdx = 0; ((lcl_elemIdx < array->dim[level]) && (*elemCnt > 0)); lcl_elemIdx++, (*elemCnt)--)
            {
            if ((lcl_status = p_shTclAlignPrimElemRet (interp, lcl_alignType, array->data.schemaType, 0, *elemPtr, lcl_prefixElem))
                           != TCL_OK)
               {
               goto rtn_return;
               }
            lcl_prefixElem = " ";
           *elemPtr       += array->data.incr;
            }
         }
      }

rtn_return : ;

   return (lcl_status);

   }	/* p_shTclArrayPrimRet */

/******************************************************************************/

void		 p_shTclArrayDeclare
   (
   Tcl_Interp	*interp			/* IN:    Tcl Interpreter structure   */
   )

/*++
 * ROUTINE:	 p_shTclArrayDeclare
 *
 * DESCRIPTION:
 *
 *	Declare Tcl commands for this module.
 *
 * RETURN VALUES:	None
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	This routine should only be called once per task.
 *--
 ******************************************************************************/

   {	/* p_shTclArrayDeclare */

   /*
    * Declarations.
    */

#define	shTclArrayCLI(TclCmd, rtn, TclHelp, TclUse)\
   shTclDeclare (interp, TclCmd, (Tcl_CmdProc *)(rtn), (ClientData)(0), (Tcl_CmdDeleteProc *)(NULL), "shArray", TclHelp, TclUse)

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#undef	shTclArrayCLI

   }	/* p_shTclArrayDeclare */

/******************************************************************************/
