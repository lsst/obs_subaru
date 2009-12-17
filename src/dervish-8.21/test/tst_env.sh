#!/bin/sh
#!
#!  This shell script will test to insure that the TCLX/TCL/TK
#!  library environment variables still point to the correct
#!  areas.  That way if TCLX/TCL/TK changes the way they do
#!  things  (as they have in the past) maybe we can detect it
#!  sooner.
#!
#! Files to test for
tclxlibrary=$TCLX_LIBRARY/TclInit.tcl
tkxlibrary=$TKX_LIBRARY/tk.tcl
tklibrary=$TK_LIBRARY/library/tk.tcl
tcllibrary=$TCL_LIBRARY/init.tcl

status=0
if [ ! -f $tclxlibrary ]
then
   echo "TEST-ERR: Could not find in TCLX_LIBRARY $tclxlibrary" 1>&2
   status=1
fi

if [ ! -f $tkxlibrary ]
then
   echo "TEST-ERR: Could not find in TKX_LIBRARY $tkxlibrary" 1>&2
   status=1
fi

if [ ! -f $tklibrary ]
then
   echo "TEST-ERR: Could not find in TK_LIBRARY $tklibrary" 1>&2
   status=1
fi

if [ ! -f $tcllibrary ]
then
   echo "TEST-ERR: Could not find in TCL_LIBRARY $tcllibrary" 1>&2
   status=1
fi

exit $status
