# Steve Kent additions/modifications to dervishStartup.tcl
#
# chainInit		Initialise an iterator for a linked list
# chainNext		Advance an iterator
# chainPrev		Retreat an iterator
# chainList		List all handles to chains and their type
###############################################################################
#
# Chain operations
#
###############################################################################
#
# Return an iteration block as a keyed list
#
proc chainInit {chain iterator} {
	upvar 1 $iterator iter
	set cursor [chainCursorNew $chain]
	keylset iter "CHAIN" $chain "CURSOR" $cursor "HANDLE" ""
   }
#
# given an iterator block, advance it to the next element of the chain
#
proc chainNext { iterator } {
	upvar 1 $iterator iter
	set chain [keylget iter "CHAIN"]
	set cursor [keylget iter "CURSOR"]
	set handle [keylget iter "HANDLE"]
	if {$handle != ""} then {handleDel $handle}
	if {[chainSize $chain] != 0} then {
		set handle [chainWalk $chain $cursor NEXT]
	} else {
		set handle ""
		}
# Are we at end of list?
	if {$handle == ""} then {
		set iter ""
		chainCursorDel $chain $cursor
		return ""
		}
	set iter ""
	keylset iter "CHAIN" $chain "CURSOR" $cursor "HANDLE" $handle
	return $handle
}

#
# given an iterator block, advance it to the previous element of the chain
#
proc chainPrev { iterator } {
	upvar 1 $iterator iter
	set chain [keylget iter "CHAIN"]
	set cursor [keylget iter "CURSOR"]
	set handle [keylget iter "HANDLE"]
	if {$handle != ""} then {handleDel $handle}
	if {[chainSize $chain] != 0} then {
		set handle [chainWalk $chain $cursor PREVIOUS]
	} else {
		set handle ""
		}
# Are we at end of list?
	if {$handle == ""} then {
		set iter ""
		chainCursorDel $chain $cursor
		return ""
		}
	set iter ""
	keylset iter "CHAIN" $chain "CURSOR" $cursor "HANDLE" $handle
	return $handle
   }

#Remove the chain link at the current position.
proc chainPurge { iterator } {
	upvar 1 $iterator iter
	set chain [keylget iter "CHAIN"]
	set cursor [keylget iter "CURSOR"]
	set iter ""
	set handle [chainElementRemByCursor $chain $cursor]
	keylset iter "CHAIN" $chain "CURSOR" $cursor "HANDLE" ""
	return $handle
}

############################################################################
proc chainList {} {
   set list [handleListFromType CHAIN]
   foreach hndl $list {
        echo $hndl [chainTypeDefine $hndl]
        }
   }




