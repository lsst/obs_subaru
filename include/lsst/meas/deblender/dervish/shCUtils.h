#ifndef	SHCUTILS_H
#define	SHCUTILS_H

#include <stdlib.h>
#include "dervish_msg_c.h"
#include "shCSchema.h"			/* For LOGICAL */

/*
 * An integral type large enough to hold a data pointer
 */
typedef long IPTR;
#define TO_IPTR(P) ((IPTR)(P))		/* convert a pointer to an IPTR */

/*
 * Error handlers
 *
 * function to call when science module receives a non-fatal error and needs to
 * tell framework. eg   
 *   shError("Function_Name: no position for star %d",star_id);
 *
 * function to call when pipeline module receives a fatal error, e.g.
 *   shFatal("Function_Name: Can't malloc object %d",id);
 *
 * There is also shDebug(level,fmt,...) where the level determines
 * at what verbosity the message should be produced:
 *	0	Always interesting, at least to the code's author
 *	1	Interesting if things are going wrong
 *	2	Probably more than you want
 * if NDEBUG is defined no messages are produced, otherwise they are printed
 * if level is >= shVerbose, which can be set with shVerboseSet(). The old
 * value is always returned, the value is unchanged if you ask for a negative
 * value.
 */

#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

int  shVerboseSet(int level);
void shDebug(int level, char *fmt,...);
void shError(char *fmt,...);

/*
 * shAssert() has identical semantics to the usual assert() in <assert.h>;
 * you must include "shCAssert.h" to use it
 */

/*
 * function to call when pipeline module receives a fatal error, e.g.
 *   shFatal("Function_Name: Can't malloc object %d",id);
 */
void shFatal(char *fmt,...);


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/*
 * Miscellany.
 *
 *   o	shEndedness returns the word orientation of the machine.
 */

typedef enum
   {
   SH_BIG_ENDIAN    = 0,				/* Big-endian machine         */
   SH_LITTLE_ENDIAN = 1				/* Little-endian machine      */
   }				SHENDEDNESS;

/*
 *   o	shKeyLocate relies on a structure for its keywords table. An entry con-
 *	sists of a pointer to a null-terminated string (the idea is to build the
 *	table at compile time) and a value associated with the keyword.
 *
 *	The table is terminated with an entry where the KEYWORD pointer is NULL.
 */

typedef	struct
   {
                  char	*keyword;		/* Null-terminated string     */
   unsigned       int	 value;			/* Keyword value              */
   }				 SHKEYENT;	/* Keyword table entry        */

/*
 *   o	shStrMatch () allows wildcards in the pattern.  The following are all
 *	the legal wildcards:
 *
 *	   *	Match zero or more characters.
 *
 *   o	shStrMatch (), shStrcmp (), and shStrncmp () accept an argument con-
 *	trolling case sensitive compares.
 */

#define	shStrWildSet	"*"

typedef enum
   {
   SH_CASE_INSENSITIVE = 0,	/* M U S T   be a false value                 */
   SH_CASE_SENSITIVE   = 1	/* M U S T   be a true  value                 */
   }				 SHCASESENSITIVITY;

/******************************************************************************/

SHENDEDNESS	 shEndedness	(void);
char		*shNth		(long int value);
size_t		 shStrnlen	(char *s, size_t n);
RET_CODE	 shStrMatch	(char *cand, char *pattern, char *wildSet, SHCASESENSITIVITY caseSensitive);
int		 shStrcmp	(char *s1, char *s2,           SHCASESENSITIVITY caseSensitive);
int		 shStrncmp	(char *s1, char *s2, size_t n, SHCASESENSITIVITY caseSensitive);
RET_CODE	 shKeyLocate	(char *keyword, SHKEYENT tbl[], unsigned int *value, SHCASESENSITIVITY caseSensitive);
RET_CODE	 shBooleanGet	(char *keyword, LOGICAL *boolVal);
float		 shNaNFloat	(void);
double		 shNaNDouble	(void);
int		 shNaNFloatTest	(float  val);
int		 shDenormFloatTest (float  val);
int		 shNaNDoubleTest(double val);
int		 shDenormDoubleTest (double val);

/******************************************************************************/

/******************************************************************************/
/*
 * Sets:  This concept comes from Pascal.
 *
 *	A set is declared in two steps:
 *
 *	   typedef enum { setElem1, setElem2, ... } setType;
 *
 *	   shDeclSet (setType)  variable_name;
 *
 *	                  or
 *
 *	   typedef shDeclSet (setType)	type_name;
 *
 *	For the shDeclSet, setType is there only for documentation purposes.  It
 *	does not do any type checking, etc.  The set is left in an uninitialized
 *	state.  One element, shEmptySet, can be used to initialize the set to a
 *	known state.
 *
 *	shDeclSet defines a set to be contained within an unsigned longword.
 *	Thus, the enum declaration must not have more than 32 elements.  Also,
 *	no enum element should have explicit values assigned to it unless the
 *	user can guarantee that all element values will fall between 0 and 31
 *	inclusive.
 *
 *	Three set operators are provided:
 *
 *	   shInSet   (setName, setElem)	test if setElem is a member of setName.
 *	   shAddSet  (setName, setElem)	add     setElem to   set setName.
 *	   shClrSet  (setName, setElem)	remove  setElem from set setName.
 *	   shMaskSet (         setElem)	return  setElem as a mask in a set.
 *
 *	shInSet returns zero (0) if the element is not a member of the set, or
 *	non-zero if the element is a member of the set.  These return values
 *	will work as expected with conditional tests.  But, they should not
 *	be counted upon to match TRUE and FALSE.
 */

#define	shDeclSet(setType)		unsigned long  int
#define	shEmptySet			0
#define	shInSet(  setName, setElem)	((setName) &   (1 << (setElem)))
#define	shAddSet( setName, setElem)	((setName) |=  (1 << (setElem)))
#define	shClrSet( setName, setElem)	((setName) &= ~(1 << (setElem)))
#define	shMaskSet(         setElem)	               (1 << (setElem))

/******************************************************************************/

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif
