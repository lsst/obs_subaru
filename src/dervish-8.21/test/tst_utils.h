#if !defined(TST_UTILS_H)
#define TST_UTILS_H
/*
 * external symbols and definitions used in writing C test suites
 *
 * The number of errors at the end of the test suite is in test_errors,
 * to report an error set status to FAIL and call report; if you
 * want a message reporting successful operation, set status to PASS
 */
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include "shCUtils.h"
/*
 * These are used to run code that is expected to fail. At the top of
 * your main program say
 *	INIT_PROTECT;
 * then where the bad code is, say
 *	START_PROTECT;
 *	bad code here;
 *	END_PROTECT;
 * If the code leads to a SIGABRT (shFatal or a failed shAssert) then
 * status is set to FAIL, otherwise it's set to PASS, so you can now
 * simply say
 *	report("bad code");
 */
#define INIT_PROTECT trap_abrt()
#define START_PROTECT if(setjmp(jmpbuf) == 0) { init_jmp = 1;
#define END_PROTECT   status = FAIL; } else { status = PASS; } init_jmp = 0;


typedef enum { PASS, FAIL } STATUS;

extern int init_jmp;			/* is jmp_buf initialiased? */
extern jmp_buf jmpbuf;			/* catch shFatal */
extern int test_errors;			/* number of errors seen */
extern STATUS status;			/* did we pass the last test? */

void trap_abrt(void);
void report(char *fmt,...);

#endif
