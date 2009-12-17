#
# This file have been reorganized to "source" a number of smaller files.
# File name clue you in to what the procs do.  Comments have been
# moved to the file with the proc.  -Chris
#
source [envscan \$DERVISH_DIR]/etc/shTools.tcl
source [envscan \$DERVISH_DIR]/etc/shGlobal.tcl
source [envscan \$DERVISH_DIR]/etc/shParse.tcl
source [envscan \$DERVISH_DIR]/etc/shHdr.tcl
source [envscan \$DERVISH_DIR]/etc/shPlot.tcl
source [envscan \$DERVISH_DIR]/etc/shRegPix.tcl
source [envscan \$DERVISH_DIR]/etc/shHistory.tcl
source [envscan \$DERVISH_DIR]/etc/shHandle.tcl
source [envscan \$DERVISH_DIR]/etc/shDump.tcl
source [envscan \$DERVISH_DIR]/etc/shAlias.tcl
source [envscan \$DERVISH_DIR]/etc/shTapes.tcl
source [envscan \$DERVISH_DIR]/etc/shMemory.tcl
source [envscan \$DERVISH_DIR]/etc/shPipe.tcl
source [envscan \$DERVISH_DIR]/etc/shPlotAsciiFile.tcl
source [envscan \$DERVISH_DIR]/etc/fits2Schema.tcl
source [envscan \$DERVISH_DIR]/etc/shP2C.tcl
source [envscan \$DERVISH_DIR]/contrib/screenEdit.tcl
source [envscan \$DERVISH_DIR]/etc/ds9.tcl

proc dervishVersion {} {
   set name "\$Name: v8_21 $";

   if {$name != [dervishName]} {
      error "TCL and C Name strings don't match: $name v. [dervishName]"
   }
   
   if {[regexp {Name: ([^\* ]+) \$$} $name foo name]} {
      return $name
   } else {
      global env
      
      if [info exists env(DERVISH_DIR)] {
	 set version [file tail $env(DERVISH_DIR)]
      } else {
	 set version "unknown"
      }

      return "NOCVS:$version"
   }
}

#
# Make it possible to get ftcl help back in case someone has helpfully
# provided their own version
#
catch {
   rename help help.ftcl; proc help {args} { help.ftcl $args }
}

# With the advent of Tk, a side effect was the disapperance of the quit 
# command. This happened because quit was used in ftclCmdLine() routines
# internally. Since older versions of dervish (before integration with Tk)
# used ftclCmdLine() routines quit was recognized. But now, we have to
# make an alias
#  - vijay
####

proc quit {} {
   exit 0
}






