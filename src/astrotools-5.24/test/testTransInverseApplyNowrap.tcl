
proc simpleVTransApply {trans vrow vcol} {
    loop i 0 [exprGet $vrow.dimen] {
	set row [exprGet $vrow.vec<$i>]
	set col [exprGet $vcol.vec<$i>]
	set keyl [transApply $trans r $row 0 $col 0]
	handleSet $vrow.vec<$i> [keylget keyl mu]
	handleSet $vcol.vec<$i> [keylget keyl nu]
    }
}

proc simpleVTransInverseApply {trans vrow vcol} {
    loop i 0 [exprGet $vrow.dimen] {
	set row [exprGet $vrow.vec<$i>]
	set col [exprGet $vcol.vec<$i>]
	set keyl [transInverseApplyNowrap $trans r $row 0 $col 0]
	handleSet $vrow.vec<$i> [keylget keyl row]
	handleSet $vcol.vec<$i> [keylget keyl col]
    }
}

proc testTransInverseApplyNowrap { {doPlot 0} } {
    global pg vxe vye vxm vym
    set trans [transNew]
    set lx ""
    set ly ""
    for {set x -1000} {$x < 1000} {set x [expr $x+10]} {
	lappend lx $x
	lappend ly 0
    }
    set vxe [vFromL $lx x]
    set vye [vFromL $ly y]
    set vxm [vectorExprEval $vxe]
    set vym [vectorExprEval $vye]

    simpleVTransInverseApply $trans $vxm $vym
    if {$doPlot} {
	pgstateSet $pg -isNewplot 1 -just 1 -symb 1 -icMark 1 \
		    -plotTitle "x transInverseApply vs. x original"
	vPlot $pg $vxe $vxm
    }
    loop i 0 [exprGet $vxe.dimen] {
	set xe [exprGet $vxe.vec<$i>]
	set xm [exprGet $vxm.vec<$i>]
	if {$xe != $xm} {
	    echo trouble in testTransInverseApplyNowrap:  i=$i xe=$xe xm=$xm
	    return 1
	}
    }

    return 0
}








