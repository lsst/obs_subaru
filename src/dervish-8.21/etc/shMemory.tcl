#
# Procedures to make dealing with memory a little easier
#
# memBlocksGetRange n1 n2       Like memBlocksGet, but only return blocks
#				with IDs in n1--n2 inclusive. If n2 is 0,
#				it's taken to be the maximum ID
# handleListRange n1 n2 [-page]	List handles that map to a given range of
#				memory IDs, n1--n2 inclusive. If n2 is 0,
#				it's taken to be the maximum ID. With -page,
#				stop for input every 20 lines
# memBlocksPrintRange n1 n2 [-ignore] [-only] [-page]
#				Print blocks with IDs in range n1--n2 inclusive
#				n2 == 0 means print up to the maximum ID.
#				-ignore ignores files which are elements of
#				the tcl array memFilesIgnore
#				-only prints blocks from files which are
#				elements of the tcl array memFilesOnly
#				-page pauses every 20 lines
#
# For example:
#				foreach f {shArray.c shChain.c shTbl.c} {
#				   set memFilesIgnore($f) 1
#				}
#				foreach f {region.c} {
#				   set memFilesOnly($f) 1
#				}
#
set SEP "\n\t\t\t"

# memBlocksGetRange n1 n2       Like memBlocksGet, but only return blocks
#				with IDs in n1--n2 inclusive. If n2 is 0,
#				it's taken to be the maximum ID
set hh "Usage:  memBlocksGetRange n1 n2\n$SEP\
Like memBlocksGet, but only return blocks$SEP\
with IDs in n1--n2 inclusive.\n$SEP\
n2 == 0 means get up to the maximun ID.$SEP"
ftclHelpDefine shMemory memBlocksGetRange $hh

set hh "Usage:  handleListRange n1 n2 \[-page\]\n$SEP\
List handles that map to a given range of $SEP\
memory IDs, n1--n2 inclusive.\n$SEP\
n2 == 0 means list up to the maximum ID.$SEP\
-page: stop for input every 20 lines.$SEP"
ftclHelpDefine shMemory handleListRange $hh 

set hh "Usage: memBlocksPrintRange n1 n2 \[-ignore\] \[-only\] \[-page\]\n$SEP\
Print blocks with IDs in range n1--n2 inclusive.\n$SEP\
n2 == 0 means print up to the maximum ID.$SEP\
-ignore: ignores files which are elements of$SEP\
\tthe tcl array memFilesIgnore$SEP\
-only: prints blocks from files which are$SEP\
\telements of the tcl array memFilesOnly$SEP\
-page: pauses every 20 lines"
ftclHelpDefine shMemory memBlocksPrintRange $hh

set hh "Usage: shMemTest \[-noprint\]\n$SEP\
On error returns a list where the first element is a$SEP\
list of the handles that were not freed, and the$SEP\
second element is a list of the memory blocks$SEP\
that have not been freed.  Only memory obtained$SEP\
using shMalloc will appear on this list.  In$SEP\
addition this list can be printed if the -print$SEP\
option is used."
ftclHelpDefine shMemory shMemTest $hh

unset hh
unset SEP

proc handleListRange { n1 n2 args } {
   set args [eval "concat $args"]
   
   set page 0
   if {[lsearch [set args] -page] != -1} {
      set page 1
   }

   set mem [memBlocksGet]
   if {$n2 == 0} {
      set n2 [memSerialNumber]
   }
   set ctr 0
   foreach h [handleList] {
      if [catch {keylget mem [lindex $h 2]} id_val] {
	 if {[lindex $h 2] != "0x0"} {
	    echo Cannot look up $h
	 }
      } else {
	 set id [lindex $id_val 0]
	 if {$id >= $n1 && $id <= $n2} {
	    echo $h ($id_val)
	    if {$page} {
	       incr ctr
	       if {$ctr == 20} {
		  set ctr 0
		  puts -nonewline [format "more...\r"]
		  set r [gets stdin]
		  if {$r == "n" || $r == "q"} {
		     return
		  }
	       }
	    }
	 }
      }
   }
}

###############################################################################

proc memBlocksGetRange { n1 n2 } {
   if {$n2 == 0} {
      set n2 [memSerialNumber]
   }

   catch {unset ret}
   set mem [memBlocksGet]
   foreach m $mem {
      set id [lindex [lindex $m 1] 0]
      if {$id >= $n1 && $id <= $n2} {
	 lappend ret $m
      }
   }
   if [catch {set ret}] {
      set ret {}
   }
   return $ret
}

###############################################################################

proc memBlocksPrintRange { n1 n2 args } {
   set args [eval "concat $args"]
   
   set ignore 0; set only 0; set page 0
   if {[lsearch $args -ignore] != -1} {
      set ignore 1
   }
   if {[lsearch $args -only] != -1} {
      set only 1
   }
   if {[lsearch $args -out] != -1} {
      if [catch {set file [lindex $args [expr [lsearch $args -out]+1]]}] {
	 error "Please specify a filename with -out"
      }
      set fd [open $file w]
   } else {
      set fd stdout
   }
   if {[lsearch $args -page] != -1} {
      set page 1
   }
   if {$page && $fd != "stdout"} {
      echo "-page makes no sense with -out"
      set page 0
   }

   set mem [memBlocksGetRange $n1 $n2]
   set ctr 0
   foreach m $mem {
      if $ignore {
	 global memFilesIgnore
	 set file [lindex [lindex $m 1] 2]
	 if {[catch {set memFilesIgnore($file)}] == 0} {
	    continue;			# $file is on ignore list
	 }
      }
      if $only {
	 global memFilesOnly
	 set file [lindex [lindex $m 1] 2]
	 if {[catch {set memFilesOnly($file)}]} {
	    # $file isn't on $file only list
	    set line [lindex [lindex $m 1] 3]
	    if {[catch {set memFilesOnly($file:$line)}]} {
	       # $file isn't on $file:$line only list either
	       continue;
	    }
	 }
      }
      
      puts $fd $m
      if $page { 
	 incr ctr
	 if {$ctr == 20} {
	    set ctr 0
	    puts -nonewline [format "more... \r"]
	    set r [gets stdin]
	    if {$r == "n" || $r == "q"} {
	       return
	    }
	 }
      }
   }
   if {$fd != "stdout"} {
      close $fd
   }
}

# This procedure will return an error if it encounters any of the following
#  situations -
#
#       handleList returns a handle
#       memBlocksGet returns a list
#
# EXCEPTIONS:
#    The list elements from shHash.c, tclSchemaDefine.c
#
# RETURN :
#   Success -- Nothing
#   Failure -- A Tcl list of the form :
#
#                  { {{hh} {hh} {hh}} {{kk} {kk} {kk}} }
#            where
#               hh are the left over handles formatted as returned from a 
#                  call to handleList ({name type address})
#               kk are the left over memory blocks formatted as returned
#                  from a call to memBlocksGet
#                  ({address {serial_number size [file lineno] [type handle]}})
#
# This procedure can then be used to test for memory leaks.

proc shMemTest {args} {

    if {[lsearch $args -print] != -1} {
	set print 1
    } else {
	set print 0
    }

    set err 0
    set errHandles ""
    set errMemBlocks ""
    set memBlocksExcept  [list shHash.c tclSchemaDefine.c]
    set memBlocksExceptSize [llength $memBlocksExcept]

    set handles [handleList]
    set memBlocks [memBlocksGet]
    
#
# This section is no longer needed as the h0 problem does not exist anymore
# It is kept here as reference in case we need to use it again.
#
#    set handleExcept "h0 UNKNOWN 0x0"
#    set size [llength $handles]
#    loop i 0 $size {
#	set element [lindex $handles $i]
#	if {$element != $handleExcept} {
#	    set err 1
#	    lappend errHandles $element
#	}
#    }
    set errHandles $handles

    set size [llength $memBlocks]
    loop i 0 $size {
	set element [lindex $memBlocks $i]
	set subElem [lindex $element 1]
	set memBlocksList "[lindex $subElem 3]"
	set found 0
	loop j 0 $memBlocksExceptSize {
	    if {$memBlocksList == [lindex $memBlocksExcept $j]} {
		set found 1
	    }
	}
	if {$found != 1} {
	    set err 2
	    lappend errMemBlocks $element
	}
    }

    if {$err != 0} {
	if {$print == 1} {
	    echo These are the extra handles -- $errHandles
	    echo These are the extra memory blocks -- $errMemBlocks\n
	} else {
	    return "{$errHandles} {$errMemBlocks}"
	}
    } else {
	return
    }

}

