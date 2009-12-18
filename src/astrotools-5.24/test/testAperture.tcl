proc testAperture { } {
    if {[testMeanfip] != 0} {
	error "Error from testMeanfip"
    }
    if {[testMedianfip] != 0} {
	error "Error from testMedianfip"
    }
    if {[testCmfip] != 0} {
	error "Error from testCmfip"
    }
    return 0
}

proc testMeanfip { } {
    set reg [regNew 100 100 -type FL32]
    regSetWithDbl $reg 10000
    regFluctuateAsSqrt $reg -gain 1
    set list [meanFindInAperture $reg 50 50.2 10.3]
    foreach trip "{mean 10000 10} {meanErr 75 10} {pixels 330 20} \
	    {retVal 0 0.1}" {
	set p [lindex $trip 0]
	set v [lindex $trip 1]
	set e [lindex $trip 2]
	set $p [keylget list $p]
	if {[abs [set $p]-$v] > $e} {
	    error "Error in testMeanfip:  p=$p is [set $p] not $v within $e"
	}
    }
    regDel $reg
    return 0
}

proc testMedianfip { } {
    set reg [regNew 100 100 -type FL32]
    regSetWithDbl $reg 10000
    regFluctuateAsSqrt $reg -gain 1
    set list [medianFindInAperture $reg 50 50.2 10.3]
    foreach trip "{mean 10000 10} {meanErr -1 0.1} {pixels 330 20} \
	    {retVal 0 0.1}" {
	set p [lindex $trip 0]
	set v [lindex $trip 1]
	set e [lindex $trip 2]
	set $p [keylget list $p]
	if {[abs [set $p]-$v] > $e} {
	    error "Error in testMedianfip:  p=$p is [set $p] not $v within $e"
	}
    }
    regDel $reg
    return 0
}

proc testCmfip { } {
    set reg [regNew 100 100 -type FL32]
    regSetWithDbl $reg 10000
    regFluctuateAsSqrt $reg -gain 1
    set list [clippedMeanFindInAperture $reg 50 50.2 10.3]
    foreach trip "{mean 10000 10} {meanErr 75 10} {pixels 330 20} \
	    {retVal 0 0.1}" {
	set p [lindex $trip 0]
	set v [lindex $trip 1]
	set e [lindex $trip 2]
	set $p [keylget list $p]
	if {[abs [set $p]-$v] > $e} {
	    error "Error in testCmfip:  p=$p is [set $p] not $v within $e"
	}
    }
    regDel $reg
    return 0
}



