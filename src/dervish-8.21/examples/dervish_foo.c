/*
 * This file contains a template for users writing TCL main programs which
 * utilize the command line editing and FSAOimage interface features of
 * Dervish. In addition, it declares and uses a dumpable, list capable, type FOO
 */
#include <stdlib.h>
#include <stdio.h>
#include <termio.h>
#include <sys/types.h>
#include "dervish.h"

/*
 * This prototype should really go in a header file, but it would be
 * very lonely in this trivial example so let's agree not to bother.
 */
void tsTclFooDeclare(Tcl_Interp *interp);
/*
 * prototype the function to load dervish SCHEMA. We'd have to machine generate
 * this header file, so again just type it in
 */
RET_CODE tsSchemaLoadFromTest(void);	/* load the DERVISH schema definitions */

int
main(int ac, char *av[])
{
   CMD_HANDLE handle;

   if(shMainTcl_Init(&handle, &ac, av, "foo> ") != SH_SUCCESS) {
      exit(1);
   }
   (void)tsSchemaLoadFromTest();
   
   /*
    * Declare TCL commands
    */
   ftclCreateCommands(handle.interp);	/* Fermi enhancements to TCL   */
   ftclCreateHelp (handle.interp);	/* Define help command         */
   ftclMsgDefine (handle.interp);	/* Define help for tcl verbs   */
   ftclParseInit(handle.interp);	/* Ftcl command parsing */
   tsTclFooDeclare(handle.interp);
   
   shMainTcl_Declare(&handle);
   
   shTk_MainLoop(&handle);

   exit(0);
}
 
