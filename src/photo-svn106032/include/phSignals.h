#if !defined(PHSIGNALS_H)
#define PHSIGNALS_H
/*
 * Here are a set of macros to handle catching signals for photo
 *
 * We could use the tclX 'signal trap ABRT "command"', except that
 * it's only active when at the TCL level, which doesn't help much for
 * dealing with failed assertions and such like deep within code.
 *
 * The package provided here uses setjmp/longjmp to return control to the
 * TCL interpreter --- this of course has all the dangers of open file
 * descriptors, leaking memory, and so forth that the tclX design avoids;
 * but it is probably better than simply exiting.
 *
 * If a tclX handler is in place, this package will re-signal the signal
 * after reinstating the tclX handler; if it isn't, the you'll get an error
 * return with a suitable message.
 *
 * Although this was written to deal with SIGABRT, it is applicable to
 * any other of the first NSIG signals (e.g. SIGINT, SIGSEGV).
 *
 * Usage:
 *  in one source file, say:
 *	DECLARE_CATCH;
 *
 *  then, in any routine in which you want to catch signals, say (e.g):
 *	CATCH("procedurename", SIGABRT);
 *	code....;
 *	END_CATCH(SIGABRT);
 *
 * (or
 *	CATCH("procedurename", SIGABRT);
 *	CATCH("procedurename", SIGSEGV);
 *	code....;
 *	END_CATCH(SIGSEGV);
 *	END_CATCH(SIGABRT);
 * )
 *
 * Remember, _you_ are responsible for cleaning up memory, handles, open
 * files, and so forth.
 */
#include <setjmp.h>
#include <signal.h>
#include <time.h>

#if !defined(NSIG)
#   define NSIG 32			/* largest signal value that can be
					   caught */
#endif
extern jmp_buf p_phJmpbuf[];		/* the setjmp's environment */
extern int p_phLongjmpIsOk[NSIG];	/* are we OK to longjmp? */
extern void ((*p_phOldSig[NSIG])(int));	/* preinstalled handlers */
void p_phSignalCatch(int sig);		/* the signal catcher itself */
/*
 * This #define must be used exactly once, outside any function
 */
#define DECLARE_CATCH \
\
void \
p_phSignalCatch(int sig) \
{ \
   signal(sig,p_phSignalCatch);		/* shouldn't be needed, but it's */\
    					/* notoriously hard to ensure that */\
    					/* the right signal library is used */\
   \
   if(sig < 0 || sig >= NSIG) { \
      signal(SIGABRT, SIG_DFL); \
      shFatal("Attempt to longjmp for an invalid signal %d",sig); \
   } \
   if(!p_phLongjmpIsOk[sig]) { \
      signal(SIGABRT, SIG_DFL); \
      shFatal("Longjmp for signal %d is not initialised",sig); \
   } \
   \
   longjmp(p_phJmpbuf[sig],1); \
} \
\
jmp_buf p_phJmpbuf[NSIG]; \
int p_phLongjmpIsOk[NSIG]; \
void ((*p_phOldSig[NSIG])(int))

/*
 * Here are the macros for users to use to catch signals
 *
 *   CATCH("procedurename", SIGABRT, ReturnValue);
 *   code....;
 *   END_CATCH(SIGABRT, ReturnValue);
 * In functions returning void, ReturnValue may be blank
 */
#define CATCH(PROC, SIG, RETVAL) \
{ \
   if(SIG < 0 && SIG >= NSIG) { \
      shErrStackPush(PROC ": signal " #SIG " is out of range " \
		     "(%d isn't in 0--%d)",SIG, NSIG - 1); \
      return RETVAL; \
   } \
   \
   if(p_phLongjmpIsOk[SIG]) { \
      signal(SIG,p_phOldSig[SIG]);	/* don't longjmp during setup */ \
   } \
   \
   if(setjmp(p_phJmpbuf[SIG]) != 0) {	/* we got here by longjmp */ \
      shErrStackPush(PROC ": caught signal " #SIG); \
      p_phLongjmpIsOk[SIG] = 0; \
      \
      signal(SIG,p_phOldSig[SIG]); \
      if(p_phOldSig[SIG] != SIG_DFL) { \
         if(raise(SIGABRT) != 0) { \
            signal(SIGABRT, SIG_DFL); \
            shFatal(PROC ": failed to raise signal %d",SIG); \
         } \
	 signal(SIG,p_phOldSig[SIG]);	/* it seems to get set to SIG_DFL */ \
      } \
      return RETVAL; \
   } \
   p_phOldSig[SIG] = signal(SIG,p_phSignalCatch); \
   p_phLongjmpIsOk[SIG] = 1; 

#define END_CATCH(SIG, RETVAL) \
   if(SIG < 0 && SIG >= NSIG) { \
      shErrStackPush(__FILE__ "::%d: signal " #SIG " is out of range " \
		     "(%d isn't in 0--%d)", __LINE__, SIG, NSIG - 1); \
      return RETVAL; \
   } \
   if(p_phLongjmpIsOk[SIG]) { \
      signal(SIG,p_phOldSig[SIG]); \
   } \
   p_phLongjmpIsOk[SIG] = 0; \
}

/*
 * stuff to do with the timerSet command
 */
#define TIMER_USTIME 0			/* use user + system CPU time */
#define TIMER_UTIME 1			/* use user CPU time */
#define TIMER_STIME 2			/* use system CPU time */
#define TIMER_ETIME 3			/* use elapsed wall clock time */

extern time_t p_phTimeLimit;		/* time when timer should go off */
extern volatile int phTimeLeft;		/* time until timer should go off */
extern int p_phSignalAlarm;		/* should we send a SIGUSR1 when
					   timer expires? */
extern int p_phTimeType;		/* what sort of time do we want? */
#endif
