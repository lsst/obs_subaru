#
# Misc astrotools utilities
#

#
# Define help strings for commands; for each command you want help on,
# you must make "procname -help" work, and say e.g.
#    lappend ftclHelp_procs showFocus
#
# Note that the options list for shTclParseArg may be declared within the
# proc and set_ftclHelp used to declare the help information to ftcl.
#
# I typically do this just above the start of the definition; the array
# name (here ftclHelp_procs) is arbitrary
#
# and at the end of the file say:
#    set_ftclHelp $prodName ftclHelp_procs
#
proc set_ftclHelp {help_class _proc_list} {
   upvar $_proc_list proc_list

   if [info exists proc_list] {
      foreach p $proc_list {
	 if [catch {
	    ftclHelpDefine help_class $p "\n[eval $p -help]";
	 } msg] {
	    echo $msg
	 }
      }
      unset proc_list
   }
}


#
# Return the inverse hyperbolic sin of x
#
proc asinh {x} {
   if {$x >= 0} {
         return [expr log($x + sqrt(1 + pow($x,2)))]
	} else {
		return [expr -log(-$x + sqrt(1 + pow($x,2)))]
	}
}


