proc testMatch {} {

  if {[tst_vCloseMatch]!=0} {
    error "Trouble in tst_afCloseMatch"
    return 1
  }

  if {[tst_vDiMatch]!=0} {
    error "Trouble in tst_vDiMatch"
    return 1
  }
  return 0

  if {[tst_vDiMatch2]!=0} {
    error "Trouble in tst_vDiMatch2"
    return 1
  }
  return 0

}

proc tst_vCloseMatch {} {

    global chainMeasured chainExpected transOriginal transFit

    # Return 0 if successful; 1 if there is a problem
    global chainExpected chainMeasured trans
    seedSet 5555559
    set npt 10

    # Make the chain of measured points
    set chainMeasured [chainNew PT]
    loop i 0 $npt {
	tmPtAdd $chainMeasured [expr 1000*[randGet]] [expr 500+1000*[randGet]]
    }

    # Make up a trans
    set transOriginal [transNew]
    handleSet $transOriginal.a 10.2
    handleSet $transOriginal.b 0.99
    handleSet $transOriginal.c 0.001
    handleSet $transOriginal.d 15.3
    handleSet $transOriginal.e -0.001
    handleSet $transOriginal.f 0.99

    # Make up the transformed chain and chains with errors

    set chainExpected [chainNew PT]

    set chainError [ chainNew PT ]

    loop i 0 $npt {
	set h [chainElementGetByPos $chainMeasured $i]
	set x [exprGet $h.row]
	set y [exprGet $h.col]
	set pt [ptNew]
	handleSet $pt.row [expr [exprGet $transOriginal.a] + \
		[exprGet $transOriginal.b] * $x + \
		[exprGet $transOriginal.c] * $y]
	handleSet $pt.col [expr [exprGet $transOriginal.d] + \
		[exprGet $transOriginal.e] * $x + \
		[exprGet $transOriginal.f] * $y]
	chainElementAddByPos $chainExpected $pt

        set ptErr [ ptNew ]
        handleSet $ptErr.row 0.1
        handleSet $ptErr.col 0.1
        chainElementAddByPos $chainError $ptErr

	handleDel $pt
	handleDel $ptErr
    }

    # create the vectors 
    set vxe [ vFromChain $chainExpected row ]
    set vye [ vFromChain $chainExpected col ]
    set vxm [ vFromChain $chainMeasured row ]
    set vym [ vFromChain $chainMeasured col ]
    set vxeErr [ vFromChain $chainError row ]
    set vyeErr [ vFromChain $chainError col ]
    set vxmErr [ vFromChain $chainError row ]
    set vymErr [ vFromChain $chainError col ]

    set transFit [transNew]

    vCloseMatch $transFit $vxe $vye $vxm $vym $vxeErr $vyeErr $vxmErr $vymErr

    if {[tmch [exprGet $transOriginal.a] [exprGet $transFit.a]] !=0} {
	return 1
    }
    if {[tmch [exprGet $transOriginal.b] [exprGet $transFit.b]] !=0} {
	return 1
    }
    if {[tmch [exprGet $transOriginal.c] [exprGet $transFit.c]] !=0} {
	return 1
    }
    if {[tmch [exprGet $transOriginal.d] [exprGet $transFit.d]] !=0} {
	return 1
    }
    if {[tmch [exprGet $transOriginal.e] [exprGet $transFit.e]] !=0} {
	return 1
    }
    if {[tmch [exprGet $transOriginal.f] [exprGet $transFit.f]] !=0} {
	return 1
    }
    foreach var {dRow0 dRow1 dRow2 dRow3 dCol0 dCol1 dCol2 dCol3 csRow csCol ccRow ccCol riCut} {
	if {[tmch 0. [exprGet $transFit.$var]] !=0} {
	    return 1
	}
    }
    chainDestroy $chainExpected
    chainDestroy $chainMeasured
    chainDestroy $chainError
    vectorExprDel $vxe; vectorExprDel $vye
    vectorExprDel $vxm; vectorExprDel $vym
    vectorExprDel $vxeErr; vectorExprDel $vyeErr
    vectorExprDel $vxmErr; vectorExprDel $vymErr
    transDel $transFit; transDel $transOriginal
    return 0
}

proc tmPtAdd {chain row col} {
    set pt [ptNew]
    handleSet $pt.row $row
    handleSet $pt.col $col
    chainElementAddByPos $chain $pt
    handleDel $pt
}

# proc tmPtAdd2 {chain row col mag} {
#     set pt [ptNew]
#     handleSet $pt.row $row
#     handleSet $pt.col $col
#     handleSet $pt.radius $mag
#     chainElementAddByPos $chain $pt
#     handleDel $pt
# }

proc tmch {value expected} {
    set delta [expr $value-$expected]
    if { $delta > .1} {return 1}
    if { -$delta > .1} {return 1}
    return 0
}

proc tst_vDiMatch {} {

    global chainMeasured chainExpected transOriginal transFit 
    global vxe vye vxm vym trans

    # Return 0 if successful; 1 if there is a problem
    seedSet 5555559
    set npt 10

    # Make up a trans
    set transOriginal [transNew]
    handleSet $transOriginal.a 10
    handleSet $transOriginal.b 1
    handleSet $transOriginal.c 0
    handleSet $transOriginal.d 20
    handleSet $transOriginal.e 0
    handleSet $transOriginal.f 1

    # Make up the measured chain
    set vxe [vFromL \
	    "105.95 103.48  90.79  85.64  78.84  70.08  53.06  44.50  19.16  15.64"]
    set vye [vFromL \
	    "556.69 597.28 553.15 578.13 520.41 606.38 559.47 605.30 604.32 579.66"]
    set chainMeasured [chainNew PT 10]
    vToChain $vxe $chainMeasured row
    vToChain $vye $chainMeasured col
    vectorExprDel $vxe
    vectorExprDel $vye

    # Make up the transformed chain

    set chainExpected [chainNew PT]
    loop i 0 $npt {
	set h [chainElementGetByPos $chainMeasured $i]
	set x [exprGet $h.row]
	set y [exprGet $h.col]
	set pt [ptNew]
	handleSet $pt.row [expr [exprGet $transOriginal.a] + \
		[exprGet $transOriginal.b] * $x + \
		[exprGet $transOriginal.c] * $y]
	handleSet $pt.col [expr [exprGet $transOriginal.d] + \
		[exprGet $transOriginal.e] * $x + \
		[exprGet $transOriginal.f] * $y]
	chainElementAddByPos $chainExpected $pt
	handleDel $pt
    }

    # get the VECTORSs 
    chainSort $chainExpected row
    chainSort $chainMeasured col
    set vxe [vFromChain $chainExpected row]
    set vye [vFromChain $chainExpected col]
    set vxm [vFromChain $chainMeasured row]
    set vym [vFromChain $chainMeasured col]

    # Do the fit


    set transFit [transNew]
    handleSet $transFit.a [expr [exprGet $transOriginal.a]+106]
    handleSet $transFit.b [expr [exprGet $transOriginal.b]+.05]
    handleSet $transFit.c [exprGet $transOriginal.c]
    handleSet $transFit.d [expr [exprGet $transOriginal.d]-30]
    handleSet $transFit.e [expr [exprGet $transOriginal.e]+.02]
    handleSet $transFit.f [exprGet $transOriginal.f]

    #echo ORIGINAL --------------
    #testTrans $transOriginal $vxe $vye $vxm $vym /XWINDOW
    #echo [exprGet $transOriginal]

    #echo BEFORE ------------------
    #testTrans $transFit $vxe $vye $vxm $vym /XWINDOW
    #echo [exprGet $transFit]
    vDiMatch $transFit $vxe $vye $vxm $vym 600 600 10 -nfit 6
    #echo AFTER -------------------
    #testTrans $transFit $vxe $vye $vxm $vym /XWINDOW
    #echo [exprGet $transFit]

    if {[tmch [exprGet $transOriginal.a] [exprGet $transFit.a]] !=0} {
	return 1
    }
    if {[tmch [exprGet $transOriginal.b] [exprGet $transFit.b]] !=0} {
	return 1
    }
    if {[tmch [exprGet $transOriginal.c] [exprGet $transFit.c]] !=0} {
	return 1
    }
    if {[tmch [exprGet $transOriginal.d] [exprGet $transFit.d]] !=0} {
	return 1
    }
    if {[tmch [exprGet $transOriginal.e] [exprGet $transFit.e]] !=0} {
	return 1
    }
    if {[tmch [exprGet $transOriginal.f] [exprGet $transFit.f]] !=0} {
	return 1
    }
    foreach var {dRow0 dRow1 dRow2 dRow3 dCol0 dCol1 dCol2 dCol3 csRow csCol ccRow ccCol riCut} {
	if {[tmch 0. [exprGet $transFit.$var]] !=0} {
	    return 1
	}
    }
    chainDestroy $chainExpected; chainDestroy $chainMeasured
    vectorExprDel $vxe; vectorExprDel $vye
    vectorExprDel $vxm; vectorExprDel $vym
    
    return 0
}

proc tst_vDiMatch2 {} {

    global chainMeasured chainExpected transOriginal transFit 
    global vxe vye vme vxm vym vmm trans

    # Return 0 if successful; 1 if there is a problem
    set npt 10

    # Make the chain of measured points
    set vxe [vFromL \
	    "105.95 103.48  90.79  85.64  78.84  70.08  53.06  44.50  19.16  15.64"]
    set vye [vFromL \
	    "556.69 597.28 553.15 578.13 520.41 606.38 559.47 605.30 604.32 579.66"]
    set vme [vFromL \
	    " 10.1 12.4 9.8 9.7 11.2 14.2 8.7 9.2 6.6 14.1"]

    set chainMeasured [chainNew PT 10]
    vToChain $vxe $chainMeasured row
    vToChain $vye $chainMeasured col
    vToChain $vme $chainMeasured radius
    vectorExprDel $vxe
    vectorExprDel $vye
    vectorExprDel $vme


    # Make up a trans
    set transOriginal [transNew]
    handleSet $transOriginal.a 10
    handleSet $transOriginal.b 1
    handleSet $transOriginal.c 0
    handleSet $transOriginal.d 20
    handleSet $transOriginal.e 0
    handleSet $transOriginal.f 1
    # Make up the transformed chain

    set zeroPointIn 3.45

    set chainExpected [chainNew PT]
    loop i 0 $npt {
	set h [chainElementGetByPos $chainMeasured $i]
	set x [exprGet $h.row]
	set y [exprGet $h.col]
	set mag [exprGet $h.radius]
	set pt [ptNew]
	handleSet $pt.row [expr [exprGet $transOriginal.a] + \
		[exprGet $transOriginal.b] * $x + \
		[exprGet $transOriginal.c] * $y]
	handleSet $pt.col [expr [exprGet $transOriginal.d] + \
		[exprGet $transOriginal.e] * $x + \
		[exprGet $transOriginal.f] * $y]
	handleSet $pt.radius [expr $mag+$zeroPointIn]

	chainElementAddByPos $chainExpected $pt
	handleDel $pt
    }

    # get the VECTORSs 
    chainSort $chainExpected row
    chainSort $chainMeasured col
    set vxe [vFromChain $chainExpected row]    
    set vye [vFromChain $chainExpected col]
    set vme [vFromChain $chainExpected radius]

    set vxm [vFromChain $chainMeasured row]
    set vym [vFromChain $chainMeasured col]
    set vmm [vFromChain $chainMeasured radius]

    # Do the fit

    set transFit [transNew]
    handleSet $transFit.a [expr [exprGet $transOriginal.a]+106]
    handleSet $transFit.b [expr [exprGet $transOriginal.b]+.05]
    handleSet $transFit.c [exprGet $transOriginal.c]
    handleSet $transFit.d [expr [exprGet $transOriginal.d]-30]
    handleSet $transFit.e [expr [exprGet $transOriginal.e]+.02]
    handleSet $transFit.f [exprGet $transOriginal.f]

    #echo ORIGINAL --------------
    #testTrans $transOriginal $vxe $vye $vxm $vym /XWINDOW
    #echo [exprGet $transOriginal]

    #echo BEFORE ------------------
    #testTrans $transFit $vxe $vye $vxm $vym /XWINDOW
    #echo [exprGet $transFit]
    set l [vDiMatch2 $transFit $vxe $vye $vme $vxm $vym $vmm \
	    600 600 10 -nfit 6]
    #echo AFTER -------------------
    #testTrans $transFit $vxe $vye $vxm $vym /XWINDOW
    #echo [exprGet $transFit]

    if {[tmch [keylget l zeroPointOut] $zeroPointIn] != 0} {
	return 1
    }
    if {[tmch [exprGet $transOriginal.a] [exprGet $transFit.a]] !=0} {
	return 1
    }
    if {[tmch [exprGet $transOriginal.b] [exprGet $transFit.b]] !=0} {
	return 1
    }
    if {[tmch [exprGet $transOriginal.c] [exprGet $transFit.c]] !=0} {
	return 1
    }
    if {[tmch [exprGet $transOriginal.d] [exprGet $transFit.d]] !=0} {
	return 1
    }
    if {[tmch [exprGet $transOriginal.e] [exprGet $transFit.e]] !=0} {
	return 1
    }
    if {[tmch [exprGet $transOriginal.f] [exprGet $transFit.f]] !=0} {
	return 1
    }
    foreach var {dRow0 dRow1 dRow2 dRow3 dCol0 dCol1 dCol2 dCol3 csRow csCol ccRow ccCol riCut} {
	if {[tmch 0. [exprGet $transFit.$var]] !=0} {
	    return 1
	}
    }
    return 0
}

