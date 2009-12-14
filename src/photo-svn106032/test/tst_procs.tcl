#
# Misc procs used in test suites
#

####################################################################
#
# given a list of numbers, add those values to the specified row
#    of the given region
#
proc add_to_reg { reg row0 col0 lis } {
   set col $col0
   foreach elem $lis {
      set val [expr [regPixGet $reg $row0 $col]+$elem]
      regPixSetWithDbl $reg $row0 $col $val
      incr col
   }
}

####################################################################
# 
# Add an empty mask to a region
#
proc add_mask {reg} {
   set mask [spanmaskNew -nrow [exprGet $reg.nrow] -ncol [exprGet $reg.ncol]]
   handleSetFromHandle $reg.mask &$mask
   handleDel $mask

   return $reg
}


################################################################
# check that the "field" of the given "struct" is equal to the
#   given "value", to within an amount "diff".  
#   return 0 if the test succeeds, or 1 if it fails
#
# diff may be of the form val|val|val
#  val may of the form 1.23, or 2.5%, or 1.5s (or 1.5S); the latter form
# specified an error in units of the estimated sigma (if available).
#
# If $struct looks like a number, it'll be taken to be the measured value,
# rather than trying to evaluate $struct->$field
#
# If there's a global "msgs", all error messages will be appended to it
#
proc check_it { struct field value eps} {
   if {[info global msgs] != ""} {
      global msgs
   }
   
   set is_number [regexp {^[+-]?[0-9]+} $struct]

   if $is_number {
      set measured $struct
   } else {
      set measured [exprGet $struct->$field]
   }

   if {[regexp -nocase {.*nan0x} $measured]} {
      set msg "$field is not a valid number ($measured)"
      echo "$msg\007";			# this must be a bug
      lappend msgs "$msg"
      
      return 1
   }
   
   set diff [expr abs($measured-$value)]
#
# parse the requested tolerances, which can be a | separated list
#
   set abs_tol -1; set percent_tol -1; set sigma_tol -1
   foreach e [split $eps "|"] {
      if {[regexp {([0-9]+(\.[0-9]+)?)([%sS])} $e foo neps foo type]} {
	 if {$type == "%"} {
	    set percent_tol [expr $neps*$value/100.0]
	 } else {
	    if {!$is_number} {
	       set sigma [exprGet $struct->${field}Err]
	       set sigma_tol [expr $neps*$sigma]
	    }
	 }
      } else {
	 set abs_tol $e
      }
   }
#
# now see if we failed. We have to fail all criteria to be adjudged a failure
#
   global verbose;
   if { ($abs_tol < 0 || $diff > $abs_tol) &&
	($percent_tol < 0 || $diff > $percent_tol) &&
	($sigma_tol <= 0 || $diff > $sigma_tol) } {
      set msg "should find $field = $value, but is $measured\n"
      set msg "$msg               |delta| = [format %.3g $diff] == "
      if {$value != 0.0} {
	 set msg "$msg [format %.3g [expr 100.0*$diff/$value]]% "
      }
      if {$sigma_tol > 0} {
	 set msg "$msg [format %.3g [expr $diff/$sigma]]sigma "
      }
      set msg "$msg (tolerance $eps)"

      echo "$msg";
      lappend msgs "$msg"

      return 1
   } elseif {[info exists verbose] && $verbose} {
      echo "Expected $field = $value, is $measured"
      puts -nonewline "               |delta| = [format %.3g $diff] == "
      if {$value != 0.0} {
	 puts -nonewline "[format %.3g [expr 100.0*$diff/$value]]% "
      }
      puts "(tolerance $eps)"
   }
   
   return 0
}

###############################################################################
#
# Print all the error messages
#
proc print_msgs {{abort 0}} {
   global msgs;

   foreach msg $msgs {
      echo "TEST-ERR: $msg"
   }

   if $abort {
      error ""
   }
}

###############################################################################
#
# Setup the PSF for measureObjects
#
proc tst_set_psf {psf band} {
   set reg [regNew 61 61]

   regIntSetVal $reg [softBiasGet]
   dgpsfAddToReg $reg $psf 30.5 30.5 -a 10000

   set psf [fitPsfFromReg $reg 0 -sigma 1];# we don't own this psf
   fitCellColorSet $band $psf.coeffs
   handleDel $psf

   regDel $reg
}

