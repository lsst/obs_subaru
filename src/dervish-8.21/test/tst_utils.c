/*
 * Utilities to write tst code with
 *
 * Typical usage would be:
 *
 *    status = (shListAdd(foolist, foo, ADD_BACK) == SH_SUCCESS) ? PASS : FAIL;
 *    report("putting element on list");
 *
 * or, if a failed shAssert is expected,
 * 
 *    if(setjmp(jmpbuf) == 0) {
 *       init_jmp = 1;
 * 
 *       shFooDel((FOO *)foolist->first->next);
 *       status = FAIL;
 *    } else {
 *       status = PASS;
 *    }
 *    report("freeing element on a list");
 *    init_jmp = 0;
 *
 * You may prefer to use the macros START_PROTECT and END_PROTECT as
 * defined in tst_utils.h
 *
 * Robert Lupton (rhl@astro.princeton.edu)
 */
#include <stdio.h>
#include "tst_utils.h"
#include "shCUtils.h"

int init_jmp = 0;			/* is jmp_buf initialiased? */
jmp_buf jmpbuf;				/* catch shFatal */
int test_errors = 0;				/* number of errors seen */
STATUS status = PASS;			/* did the last test fail? */

/*****************************************************************************/
/*
 * trap SIGABRT (from shFatal)
 */
static void
hand_abrt(int sig)
{
   (void)signal(SIGABRT,hand_abrt);
   if(init_jmp) {
      shError("Caught shFatal, continuing"); /* not really kosher in a
						signal handler... */
      longjmp(jmpbuf,1);
   }
}

void
trap_abrt(void)
{
   (void)signal(SIGABRT,hand_abrt);
}

/*****************************************************************************/
/*
 * A function to report error or success messages. In case of failure,
 * status is reset to PASS and the number of errors seen in incremented
 */
void
report(char *fmt,...)
{
   va_list args;

   va_start(args,fmt);
   fprintf(stderr,status == PASS ? "TEST-OK:  " : "TEST-ERR: ");
   vfprintf(stderr,fmt,args);
   fprintf(stderr,"\n");
   fflush(stderr);
   va_end(args);

   if(status != PASS) {
      test_errors++;
      status = PASS;
   }
}
