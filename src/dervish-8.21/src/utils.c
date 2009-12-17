/*****************************************************************************
******************************************************************************
**
** ABSTRACT:
**	This file contains utility routines for C code in dervish and beyond
**
** ENTRY POINT		SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
** shEndedness		Public	Return endedness of machine (big or little).
** shNth		Public	Return a character suffix for ordinal numbers.
** shStrnlen		Public	Return string length.
** shStrMatch		Public	Match a string against a pattern.
** shStrcmp		Public	Compare strings with case sensitivity choice.
** shStrncmp		Public	Compare strings with case sensitivity choice.
** shKeyLocate		Public	Locate a keyword in a table.
** shGetBoolean		Public	Convert a boolean string to a value.
** shNaNFloat		Public	Return a float  Not-a-Number (NaN).
** shNaNDouble		Public	Return a double Not-a-Number (NaN).
** shNaNFloatTest	Public	Indicate whether a float  is an NaN.
** shNaNDoubleTest	Public	Indicate whether a double is an NaN.
**
** ENVIRONMENT:
**	ANSI C.
**
** REQUIRED PRODUCTS:
**
** AUTHOR:
**	Robert Lupton (rhl@astro.princeton.edu)
**	Tom Nicinski (Fermilab)
**
******************************************************************************
******************************************************************************
*/
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "shCAssert.h"
#include "shCErrStack.h"
#include "shCEnvscan.h"
#include "shCUtils.h"

/*****************************************************************************/
/*
 * Debugging messages
 */
static int shVerbose = 0;

void
shDebug(int level, char *fmt, ...)
{
#if !defined(NDEBUG)
   va_list args;

   if(level <= shVerbose) {
      va_start(args,fmt);
      fprintf(stderr,"Debug(%d): ",level);
      vfprintf(stderr,fmt,args);
      fprintf(stderr,"\n");
      fflush(stderr);
      va_end(args);
   }
#endif
}

/*
 * Set the verbosity, returning the old value. If the specified value
 * is negative the old value is unchanged (although you could achieve this
 * with current = shVerboseSet(shVerboseSet(n)) for any n).
 */
int
shVerboseSet(int newVerbose)
{
   int old = shVerbose;
   if(newVerbose >= 0) {
      shVerbose = newVerbose;
   }
   return(old);
}

/*****************************************************************************/
/*
 * And error messages
 *
 * function to call when science module receives a non-fatal error and
 * needs to tell framework. The message is printed, and the string is
 * pushed onto the error stack.
 *
 * Note that if the message ends in a newline it will be removed before
 * passing it on to the error stack
 */
void
shError(char *fmt, ...)
{
   va_list args;
   char buff[1000];
   int len;

   va_start(args,fmt);
   vsprintf(buff,fmt,args);
   va_end(args);

   if(buff[len = strlen(buff)] == '\n') {
      buff[len] = '\0';
   }
   shErrStackPush("%s",buff);
   fprintf(stderr,"Error: %s\n",buff);
   fflush(stderr);
}

/*
 * and here's the fatal handler
 */
void
shFatal(char *fmt, ...)
{
   va_list args;

   va_start(args,fmt);
   fprintf(stderr,"Fatal error: ");
   vfprintf(stderr,fmt,args);
   fprintf(stderr,"\n");
   fflush(stderr);
   va_end(args);
   abort();
}
/******************************************************************************/

SHENDEDNESS	   shEndedness
   (
   void
   )

/*++
 * ROUTINE:	   shEndednes
 *
 * RETURN VALUES:
 *
 *   SH_BIG_ENDIAN
 *   SH_LITTLE_ENDIAN
 *
 * SIDE EFFECTS:
 *
 *   o	Compilation error if system types do not permit one to determine the
 *	endedness of the machine.
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/*    shEndedness */

#if (UINT_MAX == UCHAR_MAX)
#error Data types inadequate to determine machine endedness
#endif

   /*
    * Consider a 4-byte word set to a value of 1:
    *
    *   lclVal on a little-endian machine --.
    *                                       |
    *       +-------+-------+-------+-------+
    *       |   0   |   0   |   0   |   1   |
    *       +-------+-------+-------+-------+
    *       | 
    *       `-- lclVal on a big-endian machine
    */

   unsigned int		 lclVal = 1;

   return ((*((unsigned char *)&lclVal) == 0) ? SH_BIG_ENDIAN : SH_LITTLE_ENDIAN);

   }	/*    shEndedness */

/******************************************************************************/

char		*  shNth
   (
            long  int	 value		/* IN:    Ordinal value               */
   )

/*++
 * ROUTINE:	   shNth
 *
 *	Return a character suffix to designate an ordinal value.
 *
 * RETURN VALUES:
 *
 *   o	Pointer to char: "st", "nd", "rd", or "th"
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	Characters are assumed to be 1 byte in length.
 *
 *   o	The returned strings are kept in static storage.  Nothing prevents users
 *	from modifying these, causing "invalid" strings to be returned.
 *--
 ******************************************************************************/

   {	/*   shNth */

   /*
    * Declarations.
    */

static            char	*lcl_suffixes[4] = { "st", "nd", "rd", "th" };
                  char	*lcl_suffix;
                  char	 lcl_decNum[21 /* strlen ("-9223372036854775808") */ + 1];
                  char	*lcl_onesPos;
                  char	*lcl_tensPos;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Check quickly for some values.
    */

   if ((value >= 0) && (value <= 20))
      {
      lcl_suffix = (value == 1) ? lcl_suffixes[0]
                 : (value == 2) ? lcl_suffixes[1]
                 : (value == 3) ? lcl_suffixes[2]
                                : lcl_suffixes[3];
      }
   else
      {
      sprintf (lcl_decNum, "%02ld", value);	/* Will be at least 2 digits  */
      lcl_tensPos = lcl_decNum + strlen (lcl_decNum) - 2;
      lcl_onesPos = lcl_tensPos + 1;

      lcl_suffix  = (*lcl_tensPos == '1') ? lcl_suffixes[3]
                  : (*lcl_onesPos == '1') ? lcl_suffixes[0]
                  : (*lcl_onesPos == '2') ? lcl_suffixes[1]
                  : (*lcl_onesPos == '3') ? lcl_suffixes[2]
                                          : lcl_suffixes[3];
      }

   return (lcl_suffix);

   }	/*   shNth */

/******************************************************************************/

size_t		   shStrnlen
   (
                  char	*s,		/* IN:    String to measure           */
   size_t		 n		/* IN:    Length restriction          */
   )

/*++
 * ROUTINE:	   shStrnlen
 *
 *	Return the length of a string.  Counting of the length is continued
 *	until either a null character ('\000') is encountered, or the length
 *	limit is reached.  If the length limit is reached, then that plus 1
 *	is returned.
 *
 * RETURN VALUES:
 *
 *   o	string length or the length limit plus 1.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/*   shStrnlen */

   /*
    * Declarations.
    */

   register                char	*lcl_str       = s;
   register size_t		 lcl_strLenLim = n;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    *   o   lcl_strLenLim is postdecremented to protect it against possible
    *       underflow on the first test.
    */

   while (  *lcl_str != '\000')
      {
             lcl_str++;
      if (lcl_strLenLim-- <= 1) { break; }
      }

   return ((*lcl_str == '\000') ? (lcl_str - s) : (n + 1));

   }	/*   shStrnlen */

/******************************************************************************/

RET_CODE	   shStrMatch
   (
                  char	*cand,		/* IN:    Candidate string            */
                  char	*pattern,	/* IN:    Matching pattern            */
                  char	*wildSet,	/* IN:    Wildcard set                */
   SHCASESENSITIVITY	 caseSensitive	/* IN:    Case sensitive match?       */
   )

/*++
 * ROUTINE:	   shStrMatch
 *
 *	Match the passed CANDidate string against the match PATTERN.  Wildcard
 *	specifiers are treated as such:
 *
 *	   *	(asterisk)	indicates zero or more characters.  Any charac-
 *				ters will match.
 *
 *	Only the wildcard specifiers in WILDSET are actually used.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *			The candidate string matches the pattern.
 *
 *   SH_NOEXIST		Failure.
 *			The candidate string did not match the pattern.
 *
 * SIDE EFFECTS:
 *
 *   o	If the candidate string contains any wildcard characters, it's likely
 *	that it will not match the pattern.  Wildcard specifiers are never
 *	treated as literals in the pattern.
 *
 * SYSTEM CONCERNS:	None
 *
 *   o	The wildcards here are taken from shStrWildSet defined in shCUtils.h. If
 *	that ever changes, then the code here should change to reflect that.
 *--
 ******************************************************************************/

   {	/* shStrMatch */

   /*
    * Declarations.
    */

   register RET_CODE	 lcl_status = SH_SUCCESS;

            long  int	 lcl_candLen;		/* Candidate length left      */
            long  int	 lcl_patternLen;	/* Pattern   length left      */
            long  int	 lcl_candLeft;		/* Candidate length left: temp*/
            long  int	 lcl_patternLeft;	/* Pattern   length left: temp*/

            long  int	 lcl_pattameLen;	/* Token w/o wildcard length  */
   register       char	*lcl_patWild = NULL;	/* Token w/  wildcard         */
   register long  int	 lcl_patWildLen;	/* Token w/  wildcard length  */

            long  int	 lcl_matchIdx;
   register long  int	 lcl_patIdx;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Break up the pattern string into tokens with and without any wildcard
    * characters.
    */

        lcl_candLen    = strlen (cand);
   if ((lcl_patternLen = strlen (pattern)) == 0)
      {
      lcl_status = SH_NOEXIST;			/* Nothing to match properly  */
      goto rtn_return;
      }

   lcl_patWildLen = 0;				/* No wildcard in pattern yet */

   for ( ; (lcl_patternLen > 0) || (lcl_patWildLen > 0); ) /*No _candlen check*/
      {
      lcl_pattameLen = strcspn (pattern, wildSet); /* Locate wilds in pattern */

      /*
       * If no non-wildcard portion of the pattern exists, either we've hit a
       * a wildcard portion or there's no pattern left.  For the latter, the
       * outstanding wildcard portion of the pattern (if it exists) must still
       * be matched.
       */

      if ((lcl_pattameLen == 0) && (lcl_patWildLen == 0)) /* No wild outstand */
         {
         /*
          * Found a wildcard character.  Group together any following wildcards
          * into a wildcard token from the pattern.  It is not processed now.
          *
          *   o   Control goes back to the top of the main FOR loop to process
          *       the immediately following non-wildcard portion of the pattern.
          */

         lcl_patWildLen = strspn (pattern, wildSet);
         lcl_patWild    = pattern;
                          pattern    += lcl_patWildLen;
                      lcl_patternLen -= lcl_patWildLen;
         }

      else /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

         {
         /*
          * Got a non-wildcard portion of the pattern or there's no pattern and
          * a wildcard portion of the pattern is still outstanding.
          */

         if (lcl_patWildLen == 0)		/* Outstanding wildcards?     */
            {
            /*
             * No wildcard token outstanding.  So, it's a straight match against
             * the candidate string.
             */

            if (shStrncmp (cand, pattern, lcl_pattameLen, caseSensitive) != 0)
               {
               lcl_status = SH_NOEXIST;		/* No match here              */
               goto rtn_return;
               }

            /*
             * The non-wildcard token matches the candidate string.
             *
             *   o   Control goes back to the top of the main FOR loop.
             */

            lcl_candLen    -= lcl_pattameLen;
            lcl_patternLen -= lcl_pattameLen;
                pattern    += lcl_pattameLen;
                cand       += lcl_pattameLen;
            }

         else /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

            {
            /*
             * A wildcard token is outstanding.  Locate the non-wildcard portion
             * of the pattern within the candidate string.  Then, determine
             * whether the wildcard token matches the candidate string also.
             *
             *                          .------------ pattern
             *     lcl_patWild --.      |
             *                   |   .--^--.
             *                   V   V     V
             *    pattern   ---+---+---------+---+---
             *    string       | * | abcdefg | * |
             *              ---+---+---------+---+---
             *                 |    \         \
             *                 |     \         \
             *    candidate ---+------+---------+---
             *    string       |      | abcdefg |
             *              ---+------+---------+---
             *                | |    |  |
             *       ----.----' `--.-'  `----- lcl_matchIdx
             *           |         |
             *    already matched  `---- cand (area to match lcl_patWild)
             *
             * After matching the non-wildcard/wildcard portions of the pattern,
             * shStrMatch is called recursively to match the rest of the pat-
             * tern.  If the remainder of the pattern does not match, then the
             * original matching of the non-wildcard/wildcard portions is not
             * the correct "local" match and matching the non-wildcard portion
             * continues.  Consider the following example:
             *
             *                       .--- lcl_matchIdx
             *                       V
             *              ---+---+----+---+----+---	The first local match of
             *    candidate    | t | om | t | om |	the non-wildcard portion
             *              ---+---+----+---+----+---	occurs at lcl_matchIdx.
             *                 |   |\    \		The wildcard part also
             *                 |   | \    \		matches.  But, the over-
             *              ---+---+--+----+---		all match of both the
             *    pattern      | t |* | om |		pattern and candidate
             *              ---+---+--+----+---		strings fails.
             *                      |   |
             *        lcl_patWild --'   |
             *                pattern --'     .--- lcl_matchIdx
             *                                V
             *              ---+---+----+---+----+---	The next local match of
             *    candidate    | t | om | t | om |	the non-wildcard portion
             *              ---+---+----+---+----+---	of the pattern against
             *                 |   |    ___/ ___/	the candidate also suc-
             *                 |   |   /    /		ceeds with the wildcard
             *              ---+---+--+----+---		portion matching too. In
             *    pattern      | t |* | om |		this case, both strings
             *              ---+---+--+----+---		also match overall.
             *
             */

            for (lcl_matchIdx = 0,		/* Begin matching at cand [0] */
                 lcl_status   = SH_NOEXIST; lcl_status != SH_SUCCESS; )
               {
               /*
                * Match the non-wildcard portion of the pattern, if it exists.
                */

               if (lcl_pattameLen == 0)		/* Match non-wildcard portion?*/
                  {
                  lcl_status = SH_SUCCESS;	/* Succeed at matching nothing*/
                  }
               else
                  {
                  for ( ; lcl_matchIdx <= (lcl_candLen - lcl_pattameLen); lcl_matchIdx++)
                     {
                     if (shStrncmp (&(cand [lcl_matchIdx]), pattern, lcl_pattameLen, caseSensitive) == 0)
                        {
                        lcl_status = SH_SUCCESS;		/* Got a match*/
                        break;
                        }
                     }

                  if (lcl_status != SH_SUCCESS)	/* If there's no local match, */
                     {				/* then the overall candidate */
                     goto rtn_return;		/* and pattern do not match   */
                     }
                  }

               /*
                * Matched the non-wildcard portion of the pattern (or it does
                * not exist) beyond the wildcard to the candidate string at
                * lcl_matchIdx.  The wildcard portion of the pattern is now
                * matched against the candidate string (before lcl_matchIdx).
                *
                * NOTE: This wildcard checking is very simplistic.  For more
                *       complicated wildcard expressions, a recursive routine
                *       may be needed to let it find the right matching between
                *       the candidate string (portion) and the wildcard token.
                */

               for (lcl_patIdx = 0; (lcl_patIdx < lcl_patWildLen) && (lcl_status == SH_SUCCESS); lcl_patIdx++)
                  {
                  switch (lcl_patWild [lcl_patIdx])
                     {
                     case '*' :				/* Always matches     */
                                break;
                     default  :
                                lcl_status = SH_NOEXIST; /* Unknown wild char */
                                goto rtn_return;
                     }
                  }

               /*
                * Both the non-wildcard and wildcard portions of the pattern
                * matched a local portion of the candidate string.  Check to see
                * if this was the "correct" match by seeing if the rest of the
                * candidate string and pattern match.
                *
                *   o   Since shStrMatch is called recursively to perform the
                *       matching, there's no need to continue matching at this
                *       level if it returns successfully.  The recursive call
                *       will have tested the string all the way to the end.
                */

               if (lcl_pattameLen != 0)
                  {
                  if (lcl_status == SH_SUCCESS)
                     {
                     lcl_candLeft    = lcl_candLen    - (lcl_pattameLen + lcl_matchIdx);
                     lcl_patternLeft = lcl_patternLen -  lcl_pattameLen;

                     if (lcl_patternLeft > 0)
                        {
                        if ((lcl_status = shStrMatch (&(cand    [lcl_pattameLen + lcl_matchIdx]),
                                                      &(pattern [lcl_pattameLen]), wildSet, caseSensitive)) == SH_SUCCESS)
                           {
                           goto rtn_return;
                           }
                        }
                     else			/* There's no more pattern. If*/
                        {			/* there's no more candidate, */
                        if (lcl_candLeft == 0)	/* then strings match fully!  */
                           {
                           goto rtn_return;
                           }
                        else			/* Otherwise, pattern ran out */
                           {			/* to soon.  Try to get wilds */
                           lcl_status = SH_NOEXIST;	/* to cover more of   */
                           }				/* candidate string.  */
                        }
                     }
                  lcl_matchIdx++;		/* Try furthur down candidate */
                  }
               else
                  {
                  /*
                   * There's only the wildcard token at the end of the pattern.
                   * Therefore, the current status indicates whether the candi-
                   * date string matched the pattern.
                   */

                  goto rtn_return;	/* No pattern left to match against   */
                  }

               }/* for (lcl_matchIdx = 0, ...; lcl_status != SH_SUCCESS; )    */

            }	/* if (lcl_patWildLen == 0) else                              */

         }	/* if ((lcl_pattameLen == 0) && (lcl_patWildLen == 0)) else   */

      }		/* for ( ; (lcl_patternLen > 0) || (lcl_patWildLen > 0); )    */

      /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

   if ((lcl_candLen != 0) || (lcl_patternLen != 0))
      {
      lcl_status = SH_NOEXIST;
      }

rtn_return: ;

   return (lcl_status);

   }	/* shStrMatch */

/******************************************************************************/

int		   shStrcmp
   (
                  char	*s1,		/* IN:    Comparison string           */
                  char	*s2,		/* IN:    Comparison string           */
   SHCASESENSITIVITY	 caseSensitive	/* IN:    Case sensitive match?       */
   )

/*++
 * ROUTINE:	   shStrcmp
 *
 *	Equivalent to ANSI C's strcmp (), except that the comparison's case
 *	sensitivity is taken into account.
 *
 * RETURN_VALUES:
 *
 *   < 0	s1 is less than s2.
 *
 *     0	Both strings are equivalent.
 *
 *   > 0	s1 is greater than s1.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* shStrcmp */

   /*
    * Declarations.
    */

   register unsigned       char	*lcl_s1 = ((unsigned char *)s1);
   register unsigned       char	*lcl_s2 = ((unsigned char *)s2);

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   if (caseSensitive)
      {
      return (strcmp (s1, s2));
      }

   /*
    * Case insensitive.
    */

   if ((lcl_s1 == ((unsigned char *)0)) && (lcl_s2 == ((unsigned char *)0))) { return ( 0); }
   if  (lcl_s1 == ((unsigned char *)0))                                      { return (-1); }
   if                                      (lcl_s2 == ((unsigned char *)0))  { return ( 1); }

   do {
      if (toupper (*lcl_s1) < toupper (*lcl_s2)) { return (-1); }
      if (toupper (*lcl_s1) > toupper (*lcl_s2)) { return ( 1); }
      if (*lcl_s1 == ((unsigned char)0))
         {
         break;
         }
                lcl_s2++;			/* Need only 1 test against   */
      } while (*lcl_s1++ != ((unsigned char)0));/* null-terminator.           */

   return (0);					/* Exact match                */

   }	/* shStrcmp */

/******************************************************************************/

int		   shStrncmp
   (
                  char	*s1,		/* IN:    Comparison string           */
                  char	*s2,		/* IN:    Comparison string           */
   size_t		 n,		/* IN:    Max chars to compare        */
   SHCASESENSITIVITY	 caseSensitive	/* IN:    Case sensitive match?       */
   )

/*++
 * ROUTINE:	   shStrncmp
 *
 *	Equivalent to ANSI C's strncmp (), except that the comparison's case
 *	sensitivity is taken into account.
 *
 * RETURN_VALUES:
 *
 *   < 0	s1 is less than s2.
 *
 *     0	Both strings are equivalent.
 *
 *   > 0	s1 is greater than s1.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* shStrncmp */

   /*
    * Declarations.
    */

   register size_t		 lcl_charIdx;
   register unsigned       char	*lcl_s1 = ((unsigned char *)s1);
   register unsigned       char	*lcl_s2 = ((unsigned char *)s2);

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   if (caseSensitive)
      {
      return (strncmp (s1, s2, n));
      }

   /*
    * Case insensitive.
    */

   if ((lcl_s1 == ((unsigned char *)0)) && (lcl_s2 == ((unsigned char *)0))) { return ( 0); }
   if  (lcl_s1 == ((unsigned char *)0))                                      { return (-1); }
   if                                      (lcl_s2 == ((unsigned char *)0))  { return ( 1); }

   for (lcl_charIdx = 0; (lcl_charIdx < n); lcl_charIdx++)
      {
      if (toupper (*lcl_s1) < toupper (*lcl_s2)) { return (-1); }
      if (toupper (*lcl_s1) > toupper (*lcl_s2)) { return ( 1); }
      if (*lcl_s1 == ((unsigned char)0))
         {
         break;
         }
      }

   return (0);					/* Exact match                */

   }	/* shStrncmp */

/******************************************************************************/

RET_CODE	 shKeyLocate
   (
                  char	*keyword,	/* IN:    Keyword to match            */
   SHKEYENT		 tbl[],		/* IN:    Keyword table               */
   unsigned       int	*value,		/* OUT:   Matching entry value        */
   SHCASESENSITIVITY	 caseSensitive	/* IN:    Case sensitive match?       */
   )

/* ROUTINE:	 shKeyLocate
 *
 *	Locate a keyword in a table.  If located, the index within the table is
 *	returned.  No check is made to see that the keyword is unique within the
 *	table.  The search is linear, thus, the first entry matching the search
 *	criteria is used.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *			The keyword was located.
 *
 *   SH_NOEXIST		Failure.
 *			The keyword was not located in the table.  No index was
 *			returned.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* shKeyLocate */

   /*
    * Declarations.
    */

            RET_CODE		 lcl_status = SH_NOEXIST;

   register unsigned       int	 lcl_tblidx;
   register SHKEYENT		*lcl_tbl = tbl;		/* For performance    */

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Perform a simple linear search through the table.
    */

   for (lcl_tblidx = 0; lcl_tbl[lcl_tblidx].keyword != ((char *)NULL); lcl_tblidx++)
      {
      if (shStrcmp (keyword, lcl_tbl[lcl_tblidx].keyword, caseSensitive) == 0)
         {
         *value     = lcl_tbl[lcl_tblidx].value;
         lcl_status = SH_SUCCESS;
         break;
         }
      }

   return (lcl_status);

   }	/* shKeyLocate */

/******************************************************************************/

RET_CODE	 shBooleanGet
   (
                  char	*keyword,	/* IN:    Keyword to convert          */
   LOGICAL		*boolVal	/* OUT:   Boolean value               */
   )

/* ROUTINE:	 shBooleanGet
 *
 *	Parse a keyword and return its corresponding boolean value.  The legal
 *	keywords are (case-insensitive):
 *
 *	   +---------------+----------------+
 *	   |  True Values  |  False Values  |
 *	   +---------------+----------------+
 *	   |       1       |        0       |
 *	   |     true      |      false     |
 *	   |      on       |       off      |
 *	   |     yes       |       no       |
 *	   +---------------+----------------+
 *
 *	NOTE:	These values match those supported by Tcl_GetBoolean ().  This
 *		routine is provided to avoid the need for a Tcl interpreter. Or
 *		if a Tcl interpreter is available, the interp result string is
 *		not set upon failure.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *			The keyword was translated to a boolean value.
 *
 *   SH_INVARG		Failure.
 *			The keyword does not translate to a boolean value.  No
 *			boolean value was returned.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* shBooleanGet */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status;
   unsigned       int	 lcl_boolVal;
   SHKEYENT		 lcl_boolNames[] = {	{"1",    1},	{"0",     0},
						{"t",    1},	{"f",     0},
	/* This table is ordered (to some    */	{"y",    1},	{"n",     0},
	/* degree) such that the most common */	{"true", 1},	{"false", 0},
	/* values are tested first to reduce */	{"yes",  1},	{"no",    0},
	/* matching times. Unambiguous abbre-*/	{"on",   1},	{"off",   0},
	/* viations are allowed.             */	{"tru",  1},	{"fals",  0},
						{"tr",   1},	{"fal",   0},
						{"ye",   1},	{"fa",    0},
								{"of",    0},
	/* Terminate the table               */	{ ((char *)0), 0 } };

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   if ((lcl_status = shKeyLocate (keyword, lcl_boolNames, &lcl_boolVal, SH_CASE_INSENSITIVE)) == SH_SUCCESS)
      {
      if (boolVal != ((LOGICAL *)0)) { *boolVal = ((LOGICAL)lcl_boolVal); }
      }
   else
      {
      lcl_status = SH_INVARG;
      }

   return (lcl_status);

   }	/* shBooleanGet */

/******************************************************************************/

float		 shNaNFloat
   (
   void
   )

/* ROUTINE:	 shNaNFloat
 *
 *	Return an IEEE single precision quiet (non-signalling) Not-a-Number
 *	(NaN).
 *
 * RETURN VALUES:
 *
 *   o	A quiet (non-signalling) Not-a-Number (NaN).
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *
 *   o	If more than one NaN is to be set, the NaN returned by this routine
 *	should be saved and used rather than calling this routine repeatedly;
 *	this should be more efficient.
 *
 *   o	Signalling is determined by the sign bit: 0 is signalling, 1 is quiet.
 *--
 ******************************************************************************/

   {	/* shNaNFloat */

   union
      {
      unsigned char	 byte[sizeof (float)];	/* IEEE single precision      */
      float		 val;
      }				 lcl_IEEE;
                  int		 lcl_byteIdx;

   shAssert (   (sizeof (lcl_IEEE.val)  == 4)	/* IEEE single precision size */
             && (sizeof (lcl_IEEE.byte) == 4));

   for (lcl_byteIdx = sizeof (lcl_IEEE.byte) - 1; lcl_byteIdx >= 0; lcl_byteIdx--)
      {
      lcl_IEEE.byte[lcl_byteIdx] = 0xFF;
      }

   return (lcl_IEEE.val);

   }	/* shNaNFloat */

/******************************************************************************/

double		 shNaNDouble
   (
   void
   )

/* ROUTINE:	 shNaNDouble
 *
 *	Return an IEEE double precision quiet (non-signalling) Not-a-Number
 *	(NaN).
 *
 * RETURN VALUES:
 *
 *   o	A quiet (non-signalling) Not-a-Number (NaN).
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *
 *   o	If more than one NaN is to be set, the NaN returned by this routine
 *	should be saved and used rather than calling this routine repeatedly;
 *	this should be more efficient.
 *
 *   o	Signalling is determined by the sign bit: 0 is signalling, 1 is quiet.
 *--
 ******************************************************************************/

   {	/* shNaNDouble */

   union
      {
      unsigned char	 byte[sizeof (double)];	/* IEEE double precision      */
      double		 val;
      }				 lcl_IEEE;
                  int		 lcl_byteIdx;

   shAssert (   (sizeof (lcl_IEEE.val)  == 8)	/* IEEE double precision size */
             && (sizeof (lcl_IEEE.byte) == 8));

   for (lcl_byteIdx = sizeof (lcl_IEEE.byte) - 1; lcl_byteIdx >= 0; lcl_byteIdx--)
      {
      lcl_IEEE.byte[lcl_byteIdx] = 0xFF;
      }

   return (lcl_IEEE.val);

   }	/* shNaNDouble */

/******************************************************************************/

int		 shNaNFloatTest
   (
   float		val		/* IN:    Floating-point value        */
   )

/* ROUTINE:	 shNaNFloatTest
 *
 *	Determine whether the value is an IEEE single precision floating-point
 *	Not-a-Number (NaN).  All NaN forms (signalling and quiet) are handled.
 *
 * RETURN VALUES:
 *
 *      0	if the value is not an NaN.
 *   != 0	if the value is     an NaN.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* shNaNFloatTest */

#include <limits.h>

#if ((INT_MIN != -2147483648) || (INT_MAX != 2147483647))
#error	"int" expected to be 4 bytes
#endif

   return (   ((*((int *)&val) &  0x7F800000) == 0x7F800000)
           && ((*((int *)&val) & ~0x7F800000) !=          0));

   }	/* shNaNFloatTest */

/******************************************************************************/

int              shDenormFloatTest
   (
   float val                           /* IN:    Floating-point value        */
   ) 
/* ROUTINE:	 shDenormFloatTest
 *
 *	Determine whether the value is an IEEE single precision denormalized 
 *      floating-point.
 *
 * RETURN VALUES:
 *
 *      0	if the value is not denormalized.
 *   != 0	if the value is     denormalized.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

{
  return (
	     ((*((int *)&val) &  0x7F800000) == 0)
	  && ((*((int *)&val) & ~0x7F800000) != 0)
	  );
}

/******************************************************************************/

int		 shNaNDoubleTest
   (
   double		val		/* IN:    Floating-point value        */
   )

/* ROUTINE:	 shNaNDoubleTest
 *
 *	Determine whether the value is an IEEE double precision floating-point
 *	Not-a-Number (NaN).  All NaN forms (signalling and quiet) are handled.
 *
 * RETURN VALUES:
 *
 *      0	if the value is not an NaN.
 *   != 0	if the value is     an NaN.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* shNaNDoubleTest */

#include <limits.h>

#if ((INT_MIN != -2147483648) || (INT_MAX != 2147483647))
#error	"int" expected to be 4 bytes
#endif
#ifdef SDSS_LITTLE_ENDIAN
#define LOW 0
#define HIGH 1
#else
#define LOW 1
#define HIGH 0
#endif
   /* reminder: we check for:
        all exponant bits set to 1
	AND mantisse not equal to zero
   */
   return (   (    (((int *)&val)[HIGH] &  0x7FF00000) == 0x7FF00000)
           && (   ((((int *)&val)[HIGH] & ~0x7FF00000) !=          0)
               ||  (((int *)&val)[LOW]                 !=          0)
	       )
	   );

   }	/* shNaNDoubleTest */

/******************************************************************************/

int              shDenormDoubleTest
   (
   double val                           /* IN:    Floating-point value        */
   ) 
/* ROUTINE:	 shDenormDoubleTest
 *
 *	Determine whether the value is an IEEE double precision denormalized 
 *      floating-point.
 *
 * RETURN VALUES:
 *
 *      0	if the value is not denormalized.
 *   != 0	if the value is     denormalized.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

{
  return (
	     (   (((int *)&val)[HIGH] &  0x7FF00000) == 0)
	  && (   (   (((int *)&val)[HIGH] & ~0x7FF00000) != 0)
	          || (((int *)&val)[LOW]                 != 0)
	     )
	 );
}

/******************************************************************************/
