proc ok {a b errorString} {
    global tolerance
    if {abs($a) < $tolerance} {
	if {abs($b) > $tolerance} {
	    echo "TEST-ERR: $errorString: $a $b $tolerance"
	    return 1
	}
    } else {
	if {abs(2.*($a-$b)/($a+$b)) > $tolerance} {
	    echo "TEST-ERR: $errorString: $a $b $tolerance"
	    return 1
	}
    }
    return 0
}

proc testTrans {} {

  if { [tst_trans] != 0} {
    error "Error with tst_trans"
  }
  if { [tst_vTrans] != 0} {
    error "Error with tst_vTrans"
  }
  return 0

}

proc tst_trans {} {
    global tolerance at_asec2Deg

    set t [transNew]
    handleSet $t.a 0
    handleSet $t.b 1
    handleSet $t.c 0
    handleSet $t.d 0
    handleSet $t.e 0
    handleSet $t.f 1
    handleSet $t.dRow0  0.1
    handleSet $t.dRow1  0.01
    handleSet $t.dRow2  0.001
    handleSet $t.dRow3  0.0001
    handleSet $t.dCol0  0.2
    handleSet $t.dCol1  0.02
    handleSet $t.dCol2  0.002
    handleSet $t.dCol3  0.0002
    handleSet $t.csRow -0.03
    handleSet $t.csCol 0.04
    handleSet $t.ccRow 0.01
    handleSet $t.ccCol -0.02
    handleSet $t.riCut 1.1

    set t2 [transNew]
    handleSet $t2.a 4.5
    handleSet $t2.b 0.01
    handleSet $t2.c 0.0002
    handleSet $t2.d -3.3
    handleSet $t2.e 0.00021
    handleSet $t2.f 0.011
    handleSet $t2.dRow0  0.1
    handleSet $t2.dRow1  0.01
    handleSet $t2.dRow2  0.001
    handleSet $t2.dRow3  0.0001
    handleSet $t2.dCol0  0.2
    handleSet $t2.dCol1  0.02
    handleSet $t2.dCol2  0.002
    handleSet $t2.dCol3  0.0002
    handleSet $t2.csRow -0.03
    handleSet $t2.csCol 0.04
    handleSet $t2.ccRow 0.01
    handleSet $t2.ccCol -0.02
    handleSet $t2.riCut 100

    set row 40.5
    set rowErr .1
    set col 10.
    set colErr .2

    foreach arr "cosmicMag cosmicScatter mag magErr" {
       if [info exists $arr] { unset $arr }
       array set $arr [list u 0  g 0  r 0  i 0  z 0]
    }
    set cosmicMag(u) 0.7
    set cosmicScatter(u) 0.2

    transCosmicMagSet cosmicMag
    transCosmicScatterSet cosmicScatter

    # Test each possible color path through the code
    foreach tuple {
	{0 u  0  0  0   0   40.9   10.8   0.010056 0.040224 0 \
	                    4.91116 -3.172611 1.084369453e-06 5.1761894449e-06}
	{1 u  0 .1  0   0   40.9   10.8   0.010065 0.04024  0 \
	                    4.91116 -3.172611 1.0081096e-6 4.8694838665e-6}
	{2 u .2 .1  0   .2  40.894 10.808 0.010065 0.04024  0 \
	                    4.9111016 -3.17252426 1.0081096e-6 4.8694838665e-6}
	{3 u .2 .51 0   .2  40.879 10.828 0.010092 0.040288 1 \
		           4.9109556 -3.17230741 1.01081152e-6 4.87529305712e-6}
	{4 u .2 .1  1.3 .51 40.879 10.828 0.010092 0.040288 1 \
	                    4.9111016 -3.17252426 1.0081096e-6 4.8694838665e-6}
	{5 u .2 .1  1.3 .1  40.91  10.78  0.010056 0.040224 0 \
	                    4.9111016 -3.17252426 1.0081096e-6 4.8694838665e-6}
    } {
	set id [lindex $tuple 0]
	if {$id == 0} {
	    handleSet $t2.muErr 1.0
	    handleSet $t2.nuErr 2.0
	} else {
	    handleSet $t2.muErr 0
	    handleSet $t2.nuErr 0
	}

        set filter [lindex $tuple 1]
        set color [lindex $tuple 2]
        set colorErr [lindex $tuple 3]
        set ri [lindex $tuple 4]
	set riErr [lindex $tuple 5]

        if {$filter != "u"} {
	   echo "TEST-ERR: " \
	       "trans test assumes all positions are in u, and all colors u-g"
	}
        set mag(u) [expr $mag(g) + $color]
        set magErr(u) [expr sqrt(pow($colorErr,2) - pow($magErr(g), 2))]
        set mag(r) [expr $mag(i) + $ri]
        set magErr(r) [expr sqrt(pow($riErr,2) - pow($magErr(i), 2))]

	# First test with unity affine transform portion
	set muTruth [lindex $tuple 6]
	set nuTruth [lindex $tuple 7]
	set muErrTruth [expr sqrt([lindex $tuple 8])]
	set nuErrTruth [expr sqrt([lindex $tuple 9])]
	set status [lindex $tuple 10]
        set results [transApply $t $filter $row $rowErr $col $colErr \
			-mag mag -magErr magErr]

	set tolerance 1.e-9
	foreach var {mu nu muErr nuErr} {
	    set $var [keylget results $var]
	    if {[ok [set $var] [set [set var]Truth] "transApply:$var:$id"] \
		    !=0} {
		return 1
	    }
	}
	if {[keylget results status] != $status} {fail "transApply:status:$id"}
        set results [transInverseApply $t $filter $mu $muErr $nu $nuErr \
			-mag mag -magErr magErr]
	foreach var {row col} {
	    if {[ok [keylget results $var] [set $var] \
		    "transInverseApply:$var:$id"] != 0} {
		return 1
	    }
	}
	set tolerance 2.e-2
	foreach var {rowErr colErr} {
	    if {[ok [keylget results $var] [set $var] \
		    "transInverseApply:$var:$id"] != 0} {
		return 1
	    }
	}
	if {[keylget results status] != $status} {fail "transApply:status:$id"}

	# Now test with non-unity affine portion
	set muTruth [lindex $tuple 11]
	set nuTruth [lindex $tuple 12]
	set muErrTruth [expr sqrt([lindex $tuple 13])]
	set nuErrTruth [expr sqrt([lindex $tuple 14])]
        set results [transApply $t2 $filter $row $rowErr $col $colErr \
			-mag mag -magErr magErr]			 
	set tolerance 1.e-9
	foreach var {mu nu muErr nuErr} {
	    set $var [keylget results $var]
	    if {[ok [set $var] [set [set var]Truth] "transApply2:$var:$id"] \
		    != 0} {
		return 1
	    }
	}
	if {$id == 0} {
	    set muErr [expr sqrt(1.00720896e-6)]
	    set nuErr [expr sqrt(4.8675474696e-6)]
	    set trowErr [expr sqrt($rowErr * $rowErr + [exprGet $t2.muErr] * \
		    [exprGet $t2.muErr] * $at_asec2Deg * $at_asec2Deg / \
		    (.01 * .01))]
	    set tcolErr [expr sqrt($colErr * $colErr + [exprGet $t2.nuErr] * \
		    [exprGet $t2.nuErr] * $at_asec2Deg * $at_asec2Deg / \
		    (.011 * .011))]
	} else {
	    set trowErr $rowErr
	    set tcolErr $colErr
	}
        set results [transInverseApply $t2 $filter $mu $muErr $nu $nuErr \
			-mag mag -magErr magErr]			 
	if {$id == 0} {
	    handleSet $t2.muErr 0
	    handleSet $t2.nuErr 0
	}
	foreach var {row col} {
	    if {[ok [keylget results $var] [set $var] \
		    "transInverseApply2:$var:$id"] != 0} {
		return 1
	    }
	}
	set tolerance 2.e-2
	foreach var {rowErr colErr} {
	    if {[ok [keylget results $var] [set t$var] \
		    "transInverseApply2:$var:$id"] != 0} {
		return 1
	    }
	}
    }
    transDel $t
    transDel $t2
    return 0
}

proc trCheck {value expected} {
    #    echo $value $expected
    set delta [expr $value-$expected]
    if { $delta > .1} {return 1}
    if { -$delta > .1} {return 1}
    return 0
}

proc ptAdd2 {chain row col} {
    set pt [ptNew]
    handleSet $pt.row $row
    handleSet $pt.col $col
    chainElementAddByPos $chain $pt
    handleDel $pt
}

proc ch2 {value expected} {
    set delta [expr $value-$expected]
    #echo $value $expected $delta
    if { $delta > .1} {return 1}
    if { -$delta > .1} {return 1}
    return 0
}

proc tst_vTrans {} {
    global transOriginal transFit

    # return 0 if successful; 1 if there is a problem
    
    seedSet 39393939
    set npt 10

    # Make the original chain
    set chainExpected [chainNew PT]
    loop i 0 $npt {
	ptAdd2 $chainExpected [randGet] [randGet]]
    }

    # make some zero errors    
    set verr [vectorExprNew $npt]

    # make the VECTORs
    set vxe [vFromChain $chainExpected row]
    set vye [vFromChain $chainExpected col]
    set vxm [vFromChain $chainExpected row]
    set vym [vFromChain $chainExpected col]

    # transform the e points.  The transform take Measured -> expected
    set transOriginal [transNew]
    handleSet $transOriginal.a 0.2
    handleSet $transOriginal.b 0.9
    handleSet $transOriginal.c 0.1
    handleSet $transOriginal.d 0.3
    handleSet $transOriginal.e -0.1
    handleSet $transOriginal.f 0.9
    vTrans $transOriginal $vxe $vye

    # now see if the fitter gets the same transform back

    set transFit [transNew]

    vFitxy $transFit $vxe $vye -vyee $verr $vxm $vym 6
    #vFitxy $transFit $vxe $vye $vxm $vym 6

    # any problems?
    if {[ch2 [exprGet $transOriginal.a] [exprGet $transFit.a]] !=0} {
	return 1
    }
    if {[ch2 [exprGet $transOriginal.b] [exprGet $transFit.b]] !=0} {
	return 1
    }
    if {[ch2 [exprGet $transOriginal.c] [exprGet $transFit.c]] !=0} {
	return 1
    }
    if {[ch2 [exprGet $transOriginal.d] [exprGet $transFit.d]] !=0} {
	return 1
    }
    if {[ch2 [exprGet $transOriginal.e] [exprGet $transFit.e]] !=0} {
	return 1
    }
    if {[ch2 [exprGet $transOriginal.f] [exprGet $transFit.f]] !=0} {
	return 1
    }
    foreach var {dRow0 dRow1 dRow2 dRow3 dCol0 dCol1 dCol2 dCol3 csRow csCol ccRow ccCol riCut} {
	if {[ch2 0. [exprGet $transFit.$var]] !=0} {
	    return 1
	}
    }
    vectorExprDel $verr
    vectorExprDel $vxe
    vectorExprDel $vye
    vectorExprDel $vxm
    vectorExprDel $vym
    chainDestroy $chainExpected
    transDel $transOriginal
    transDel $transFit
    return 0
}


