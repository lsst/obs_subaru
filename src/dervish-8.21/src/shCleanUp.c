/*
 * FILE:
 *   shSaoIo.c
 *
 * ABSTRACT:
 *    Callback Routine to clean up any mess left by dervish. Dervish
 *    registers a callback to this routine (shCleanUp). Any clean
 *    up work that needs to be done should be called from within
 *    this routine.
 *    This localizes the need to pepper atexit()s all over dervish
 *    code. Since shCleanUp() is called upon the exit of dervish,
 *    one can call their personal clean up functions within the
 *    body of shCleanUp(). To keep the name space from being
 *    polluted too much with externals, it is HIGHLY RECOMMENDED
 *    that all clean up functions called from shCleanUp() be
 *    static to this source file.
 *
 * ENTRY POINT            SCOPE    DESCRIPTION
 * ----------------------------------------------------------------------------
 * shCleanUp		  public   Public interface
 * p_shSaoImageCleanFunc  private  Clean up function for sao images
 *
 * AUTHOR:
 *   Vijay K. Gurbani, March 14, 1994.
 *   Fermi National Accelerator Laboratory
 *   Batavia, Illinois.
 *
 * ========================================================================
 */

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

#if !defined(_POSIX_SOURCE) && defined __sgi
extern int kill (pid_t pid, int sig);
#endif

#include "shCSao.h"  /* Needed for SHSAOPROC definitions */

/*
 * Global variables and external function prototypes.
 */
extern SHSAOPROC g_saoProc[CMD_SAO_MAX];

/*
 * These are the prototypes for all the routines that shCleanUp() calls to 
 * clean up any mess created by dervish. If at all possible, all clean up
 * routines should be declared static; only shCleanUp() should call them
 * (these rules are violated by p_shSaoImageCleanFunc(), but since these
 * rules were made _after_ p_shSaoImageCleanFunc() was written, I guess we
 * will have to bend them).
 */

/*
 * CALL:
 * 
 */
void
shCleanUp(void)
{
   p_shSaoImageCleanFunc();

   return;
}

/*
 * ROUTINE: p_shSaoImageCleanFunc
 *
 * CALL:
 *   (static void) p_shSaoImageCleanFunc()
 *
 * DESCRIPTION:
 *   A function to clean up the mess created by dervish. In this case, 
 *   the 'mess' consists of cleaning up (or terminating) copies of spawned
 *   sao image executables upon the exit of dervish.
 *
 * RETURN VALUE:
 *   Nothing.
 *
 * CALLED BY:
 *   shCleanUp();
 */
void p_shSaoImageCleanFunc(void)
{
   int    i;
 
   for ( i = 0 ; i < CMD_SAO_MAX ; ++i)   {
      if (g_saoProc[i].saonum != INIT_SAONUM_VAL)   {
         (void)kill(g_saoProc[i].pid, SIGKILL);
      }
   }
    /*
     * If the user was a conscientious person, (s)he may already
     * have terminated all sao images before exiting dervish. In that
     * case, the above kill will simply fail which is fine (hence the
     * cast to void). We couldn't care less.
     */
}

