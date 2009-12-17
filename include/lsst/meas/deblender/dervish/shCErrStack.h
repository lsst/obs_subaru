#ifndef SHCERRSTACK_H
#define SHCERRSTACK_H

#include <stdio.h>			/* for FILE */

/*
 * This is the public interface to the ErrorStack package. ErrorStack
 * is a package that maintains a pre-determined stack of error messages.
 * This is a circular stack, such that only the N most recent error messages
 * are saved. The stack is 1..N levels deep where N is an optionally 
 * configurable (compile time) parameter.
 *
 * 
 * PUBLIC INTERFACE:
 * 
 *
 * Public Function              Return Value/Explanation
 * ---------------              ------------------------
 *
 * void
 * shErrStackPush(char *, ...)    Pushes the error specified in it's parameter
 *                              onto the error stack. 
 * void
 * shErrStackPushPerror(char *, ...) Pushes the error given by it's arguments
 *                              onto the error stack, and append the message
 *                              that perror() would print
 * const
 * char *shErrStackGetNext(void)  Get the next error message on the stack. On
 *                              reaching the end of the stack, a NULL is
 *                              returned
 * const char *
 * shErrStackGetEarliest(void)    Get the earliest error message on the stack. 
 *                              Will return NULL if stack is empty.
 * void
 * shErrStackClear(void)          Clears out the error stack. After a call to
 *                              this function, previous error stack contents
 *                              have been lost forever
 * int
 * shErrStackOverflow(void)       Informs if an error stack overflow has
 *                              occurred, i.e. if the stack has gone one 
 *                              full circle
 * 
 * void
 * shErrStackPrint(FILE *fil)   Prints the error stack to fil.

 * PRIVATE INTERFACE:
 * 
 *
 * Private Function             Return Value/Explanation
 * ----------------             ------------------------
 *
 */

#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

void        shErrStackPush        (const char *a_format, ...);
void        shErrStackPushPerror  (const char *format, ...);
const char *shErrStackGetNext     (void);
const char *shErrStackGetEarliest (void);
void        shErrStackClear       (void);
int         shErrStackOverflow    (void);

void        shErrStackPrint(FILE *fil);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

/*
 * Define flag constants.
 */

#define	shErrStack_ClearOnPush	(1 << 0)	/* Clear stack on next push   */

#endif
