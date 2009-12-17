#
# Test code for fits2Schema
#
# This file should produce no output unless a test fails, in which case
# it should signal an error and return a helpful string
#
#
proc tstFits2Schema {} {
    global chain inChain
    set chain [chainNew PT]
    ptAdd $chain 1 10
    ptAdd $chain 2 20
    ptAdd $chain 3 30
    ptAdd $chain 4 40
    schema2Fits $chain test.fits
    set inChain [fits2Schema test.fits INPT]
    set v [vFromChain $inChain col]
    loop i 0 4 {
	if { [exprGet $v.vec<$i>] != [expr 10*($i+1)]} {
	    # something is wrong
	    error "trouble with chain read from test.fits"
	}
    }
    exec rm test.fits

    set oldChain [fits2Schema old.fits OLDPT]
    set v [vFromChain $oldChain col]
    loop i 0 4 {
	if { [exprGet $v.vec<$i>] != [expr 10*($i+1)]} {
	    # something is wrong
	    error "trouble with chain read from old.fits"
	}
    }
    return 0
   
}

proc ptAdd {chain r c} {
    set pt [ptNew]
    handleSet $pt.row $r
    handleSet $pt.col $c
    chainElementAddByPos $chain $pt
    handleDel $pt
    return
}

if {[tstFits2Schema] != 0} {
   error "tstFits2Schema failed "
}






