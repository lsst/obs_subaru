#ifndef SHTCLERRSTACK_H
#define SHTCLERRSTACK_H

#include "ftcl.h"

/*
 * This is the public Tcl interface to the ErrorStack package. ErrorStack
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
 * void shTclInterpAppendWithErrStack (...);
 *
 * void	
 * shErrStackTclTrace(...)	Callout for Tcl_Eval prior to execution of a
 *				command by Tcl_Eval.  It is used to reset the
 *				automatic clearing of the error stack.
 */

#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

void        shErrStackTclTrace    (ClientData clientData, Tcl_Interp *interp, int level, char *command,
                                   Tcl_CmdProc *cmdProc, ClientData cmdClientData, int argc, char *argv []);

void shTclInterpAppendWithErrStack (Tcl_Interp *interp);


#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif
