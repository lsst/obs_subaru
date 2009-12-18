proc testSmoothReg {} {

  if {[tst_smoothReg]!=0} {
    error "Trouble in tst_smoothReg"
    return 1
  }
  return 0

}

proc tst_smoothReg {} {

    set inRegion  [regNew 512 512 -type FL32]
    set outRegion [regNew 512 512 -type FL32]

    regSetWithDbl $inRegion 10000.
    regFluctuateAsSqrt $inRegion

    if { [catch { regSmoothWithMedian $inRegion $outRegion 5 } ] } {
	return 1
    }

    regDel $inRegion
    regDel $outRegion

    return 0

}