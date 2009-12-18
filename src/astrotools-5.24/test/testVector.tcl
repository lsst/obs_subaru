 proc testVector {{doPlot 0} } {
    seedSet 1111113
    global pg
    global smfit hg vx vy
    set hg [hgDefine [hgNew] -min 0 -max 100 -nbin 50]
    loop i 0 400 {
	hgFill $hg [randNormalGet -mean 53 -sigma 5]
    }
    loop i 0 [exprGet $hg.nbin] {
	set c [exprGet $hg.contents<$i>]
	handleSet $hg.error<$i> [sqrt $c]
    }

    set vx [vectorExprNew 10]
    set vy [vectorExprNew 10]
    handleSet $vx.vec<0> 30; handleSet $vy.vec<0> 1
    handleSet $vx.vec<1> 40; handleSet $vy.vec<1> 1
    handleSet $vx.vec<2> 45; handleSet $vy.vec<2> 1
    handleSet $vx.vec<3> 50; handleSet $vy.vec<3> 100
    handleSet $vx.vec<4> 55; handleSet $vy.vec<4> 1
    handleSet $vx.vec<5> 60; handleSet $vy.vec<5> 1
    handleSet $vx.vec<6> 65; handleSet $vy.vec<6> 1
    handleSet $vx.vec<7> 70; handleSet $vy.vec<7> 1
    handleSet $vx.vec<8> 80; handleSet $vy.vec<8> 1
    handleSet $vx.vec<9> 85; handleSet $vy.vec<9> 1


    echo do the fit
    set smfit [hgVectorFit $hg $vx $vy -shift 0 -deltaShift 300 -iprt -1 -maxit 500 -zeroError 100000]
    echo smfit is $smfit

    if {$doPlot} {
	pgstateSet $pg -smfit $smfit -isNewplot 1
	hgPlot $pg $hg
	pgstateSet $pg -isNewplot 0 -symb 6
    }

    loop i 0 2 {
	if {[tempCheck $smfit $i]} {
	    error "error with parameter $i"
	}
    }
    return 0
}

proc tempCheck {smfit i} {
    set answers "-19.81096 2.42114"
    set delta [abs [expr "[exprGet (*$smfit.fitparam<$i>).value] - \
	    [lindex $answers $i]"]]
    if {$delta > 0.1} {
	return 1
    } else {
	return 0
    }
}

    



