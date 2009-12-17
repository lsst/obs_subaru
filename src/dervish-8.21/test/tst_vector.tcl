proc test_vHg {} {
#   Remove the following global that makes no sense if
#   We clean up.
#   global chain vec hg
    set vec [vectorExprEval 0,10,.5]
    set chain [chainNew PT]
    loop i 0 20 {
	chainElementAddByPos $chain [ptNew]
    }
    vToChain $vec $chain row
    if { [exprGet [chainElementGetByPos $chain 3].row] != 1.5} {return 1}
    
    set hg [hgNewFromV $vec]
    set entries [exprGet $hg.entries]

    if { [hgMean $hg] < 5.041} {return 1}
    if { [hgMean $hg] > 5.043} {return 1}
    
    hgFillFromV $hg $vec
 
    set  entriesNow [exprGet $hg.entries]
    if { [expr $entries*2] != $entriesNow} {return 1}
      
    if { [hgMean $hg] < 5.041} {return 1}
    if { [hgMean $hg] > 5.043} {return 1}
  
    # chainPage $chain

    # vToChain
    # hgFillFromV
    # hgNewFromV

    # Clean up ... this is opposed to the global directive!
    hgDel $hg
    vectorExprDel $vec
    loop i 0 20 {
	ptDel [chainElementGetByPos $chain $i]
    }
    chainDel $chain
    
    return 0
}

proc test_vPlotChain {{out /XWINDOW}} {
#   Remove the following global that makes no sense if
#   We clean up.
#    global x y pg chain

    set chain [chainNew PT]
    chainElementAddByPos $chain [ptDefine [ptNew] 3 5 1]
    chainElementAddByPos $chain [ptDefine [ptNew] 2 3 2]
    chainElementAddByPos $chain [ptDefine [ptNew] 3 8 3]
    chainElementAddByPos $chain [ptDefine [ptNew] 5 3 4]
    chainElementAddByPos $chain [ptDefine [ptNew] 2 2 5]
    chainElementAddByPos $chain [ptDefine [ptNew] 7 3 6]
    set y [vFromChain $chain row]
    set x [vFromChain $chain col]
    vNameSet $x "col"
    vNameSet $y "row"
    set pg [pgstateNew]
    pgstateSet $pg -device $out -plotTitle "Title of Plot"
    pgstateOpen $pg 
    vPlot $pg $x $y
    chainPage [chainFromPlot $pg $chain]    
    
    # Clean up ... this is opposed to the global directive!
    pgDel $pg
    vectorExprDel $x
    vectorExprDel $y
    loop i 0 6 {
	ptDel [chainElementGetByPos $chain $i]
    }
    chainDel $chain

    return 0
}

proc test_vExtreme {} {
    global vec vMask

    set vec [vectorExprEval -1,20,.5]
    if {[vExtreme $vec "max"]!=20} {return 1}
    if {[vExtreme $vec "min"]!=-1} {return 1}
    if {[vLimit $vec "max"]!=22.1} {return 1}
    if {[vLimit $vec "min"]!=-3.100} {return 1}

    set vMask [vectorExprNew 43]
    if {[vExtreme $vec "max" -vMask $vMask]!=0} {return 1}
    if {[vExtreme $vec "min" -vMask $vMask]!=0} {return 1}
    if {[vLimit $vec "max" -vMask $vMask]!=2} {return 1}
    if {[vLimit $vec "min" -vMask $vMask]!=-2} {return 1}
    vectorExprDel $vMask

    set vMask [vectorExprNew 43]
    vectorExprSet $vMask $vMask+1
    if {[vExtreme $vec "max" -vMask $vMask]!=20} {return 1}
    if {[vExtreme $vec "min" -vMask $vMask]!=-1} {return 1}
    if {[vLimit $vec "max" -vMask $vMask]!=22.1} {return 1}
    if {[vLimit $vec "min" -vMask $vMask]!=-3.1} {return 1}
    vectorExprDel $vMask

    set vec2 [vectorExprNew 0]
    if {[vExtreme $vec2 "max"]!=0} {return 1}
    if {[vExtreme $vec2 "min"]!=0} {return 1}
    if {[vLimit $vec2 "max"]!=0} {return 1}
    if {[vLimit $vec2 "min"]!=0} {return 1}

    vectorExprDel $vec
    vectorExprDel $vec2

    return 0
}

proc test_vStat {} { 
#   Remove the following global that makes no sense if
#   We clean up.
#    global norm vec i

    set norm [vectorExprNew 400]
    loop i 1 400 {
	handleSet $norm.vec<$i> [randNormalGet -mean 100 -sigma 10]
    }
    # echo [vMean $norm] [vSigma $norm]
    if {[vMean $norm]<98} {return 1}
    if {[vMean $norm]>102} {return 2}
    if {[vSigma $norm]<8} {return 3}
    if {[vSigma $norm]>12} {return 4}
    vectorExprDel $norm
    set vec [vectorExprEval -1,20,.5]
    if {[vMean $vec]!=9.5} {return 5}
    if {[vSigma $vec]!=6.278269} {return 6}
    
    if {[vMedian $vec]!=9.5} {return 7}
   
    vectorExprSet $vec 1,1
    if {[vMedian $vec]!=1} {return 7}
    vectorExprSet $vec 1,2
    if {[vMedian $vec]!=1.5} {return 7}
    vectorExprSet $vec 1,3
    if {[vMedian $vec]!=2} {return 7}
    vectorExprSet $vec 1,4
    if {[vMedian $vec]!=2.5} {return 7}
    handleSet $vec.vec<2> 4
    if {[vMedian $vec]!=3} {return 7}

    # Clean up ... this is opposed to the global directive!
    vectorExprDel $vec
    
    return 0
}

proc test_vPlot {{out /PS}} {
#   Remove the following global that makes no sense if
#   We clean up.
#    global x y pg
    set x [vectorExprEval -10,1]
    set y [vectorExprEval 1,10]    
    set pg [pgstateNew]
    pgstateSet $pg -device $out -plotTitle "Title of Plot"
    pgstateOpen $pg 
    if {[vNameSet $x "x values"]!="x values"} { return 1}
    if {[vNameGet $x]!="x values"} { return 2}
    if {[vNameGet $y]!="none"} { return 3}
    vPlot $pg $x $y

    # Clean up ... this is opposed to the global directive!
    pgstateClose $pg
    pgstateDel $pg
    vectorExprDel $x
    vectorExprDel $y
    
    return 0
}

proc test_vGeneral {} {
    set x [vectorExprNew 30]
    vectorExprSet $x 1,100,2
    if {[exprGet $x.vec<5>]!=11} {
	return 1
    }  
    set y [vectorExprEval $x<(5,10)>]
    if {[exprGet $y.vec<5>]!=21} {
	return 1
    }

    set a [vectorExprEval "1,2,1.0e-01"]
    if {[exprGet $a.vec<5>]!=1.5} {
	return 1
    }

    set a1 [vectorExprEval "1,21,1.0e+01"]
    if {[exprGet $a1.vec<2>]!=21} {
	return 1
    }

    set b [vectorExprEval "$a-3"]
    if {[exprGet $b.vec<5>]!=-1.5} {
	return 1
    }
   #
   # Check precedence of , is below that of -
   #
    set b [vectorExprSet $b "(0,dimen($b)-1)"]
    if {[vectorExprGet $b<0>] != 0} {
	return 1
    }

    set b [vectorExprSet $b "((0,dimen($b))-1)"]
    if {[vectorExprGet $b<0>] != -1} {
	return 1
    }

    set MultRad2Deg 5.7296e+01
    set v [vFromL "1 2 3 4 5"]
    vectorExprSet $v $MultRad2Deg*$v

    set v2 [vFromL "1 2 3 4 5"]      
    vectorExprSet $v2 57.296*$v2     
    if {[exprGet $v2.vec<2>]!=[exprGet $v.vec<2>]} {
	return 1
    }
    
    set l "3.0e-7 2.0e+7 3.0e+7"   
    set v3 [vectorExprEval \{$l\} ]
    if {[exprGet $v3.vec<1>]!=2.0e+7} {
	return 1
    }

    set l "123123123123123123123"
    set v4 [vectorExprEval "$l"]
	if {[expr ([exprGet $v4.vec<0>]-1.23123123123e+20)/1.23123123123e+20] >= 1.0e-12 } {
		return 1
	}
    vectorExprDel $v4

    # Clean up ... this is opposed to the global directive!
    vectorExprDel $x
    vectorExprDel $y
    vectorExprDel $a
    vectorExprDel $a1
    vectorExprDel $b
    vectorExprDel $v
    vectorExprDel $v2
    vectorExprDel $v3
    
    
    return 0
}

proc test_vSigmaClip { } {
    seedSet 12345557
    set x [vectorExprNew 100]
    loop i 0 100 {
	handleSet $x.vec<$i> [randNormalGet -mean 5 -sigma 2]
    }
    set list [vSigmaClip $x]
    set mean [keylget list mean]
    if {[abs "$mean - 5.341614"] > 0.25} {
	echo bad mean in test_vSigmaClip: mean=$mean
	return 1
    }    
    set sqrtVar [keylget list sqrtVar]
    if {[abs "$sqrtVar - 1.948531"] > 0.25} {
	echo bad sqrtVar in test_vSigmaClip: sqrtVar=$sqrtVar
	return 1
    }
    vectorExprDel $x
    return 0
}

#
# Test code for hg verbs
#

echo Testing vectors

if {[test_vHg] != 0} {
    error "error from test_vHg"
}

if {[test_vExtreme] != 0} {
    error "error from test_vExtreme"
}

if {[test_vStat] != 0} {
    error "error from test_vStat"
}

if {[test_vPlot] != 0} {
    error "error from test_vPlot"
}

if {[test_vGeneral] != 0} {
    error "error from test_vGeneral"
}

if {[test_vSigmaClip] != 0} {
    error "error from test_vSigmaClip"
}

# Test for any memory leaks
set rtn [shMemTest]
if { $rtn != "" } {
    echo TEST-ERR: Memory leaks in tst_vector.tcl
    echo TEST-ERR: $rtn
    error "Error in tst_vector"
    return 1
}
return 0
exit 0




