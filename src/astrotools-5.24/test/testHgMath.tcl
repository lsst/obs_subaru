proc testHgMath {} {

#  mninit -iwr=33
 
    # tst_hgGaussFit tst_PolyHg tst_proco 
    foreach cmd [list \
	    tst_hg2 \
	    tst_regColorPlot2 tst_regPlot
    ] {
	if {[$cmd] != 0} {
	    error "Trouble in $cmd in testHgMath"
	    return 1
	}
    }
    echo Done
    return 0
}

proc tst_hgGaussFit {{const 1000} {nrow 100} {plot 0} } {
    global pg hg smfit mean errMean sigma echo expectedErr

    set hg [hgDefine [hgNew]]
    hgFill $hg 5 -weight 1
    hgFill $hg 6 -weight 5
    hgFill $hg 7 -weight 6
    set fitFunc [hgGaussFit $hg]

    set doML 1
    set doChi2 1
    set sig [sqrt $const]
    set region [regNew $nrow $nrow]
    regSetWithDbl $region $const
    seedSet 1111119
    regFluctuateAsSqrt $region -gain 1.0

    set hg [hgNew]
    hgDefine $hg -min [expr $const-5*$sig] -max [expr $const+5*$sig] -nbin 200
    hgReg $hg $region

    set fitFunc [hgGaussFit $hg ]

    set mean   [ exprGet $fitFunc.va<0>->vec<0> ]
    set sigma  [ exprGet $fitFunc.va<0>->vec<1> ]
    set norm   [ exprGet $fitFunc.va<0>->vec<2> ]

    set meanErr [ exprGet $fitFunc.sigma->vec<0> ]
    set sigErr  [ exprGet $fitFunc.sigma->vec<1> ]
    set normErr [ exprGet $fitFunc.sigma->vec<2> ]

    set expectedErr [expr [sqrt $const]/$nrow]

    if {[abs [expr $mean-$const]] > 1.0} {
	error "Mean from hgGaussFit is wrong"
    }
    if {[abs [expr $sigma-[sqrt $const]]] > 0.5 } {
	error "Sigma from hgGaussFit is wrong"
    }
    if {[abs [expr $norm - 15811 ]] > 2.0 } {
	error "Sigma from hgGaussFit is wrong"
    }
    regDel $region
    hgDel $hg
    funcDel $fitFunc
    return 0
}


proc tst_PolyHg { } {

# create a histogram from the eqn
#       y = 2.45 + 3.41*x + 16.3*x^2

    set hg [hgDefine [hgNew] -min 0.0 -max 10.0 -nbin 10]
    loop i 0 10 {
      set x [ expr $i + 0.5 ]
      set y [ expr 2.45 + 3.41 * $x + 16.3 * $x * $x ]
      hgFill $hg $x -weight $y
    }

    memBlocksPrint
    echo second memBlocksPrint
    set result [ hgPolyFit $hg 2 ]
    memBlocksPrint
    echo third memBlocksPrint

    set theFunc [ keylget result "FUNC" ]
    set theCoef [ keylget result "COEF" ]
    
    echo Here in PolyHg
    hgDel $hg
    memBlocksPrint


# extract the fit parameters
    loop ct 0 3 {
      set a($ct) [ exprGet $theCoef.vec<$ct> ]
    }

# make sure the parameters are correct
   set retcode 0

   set delta [ expr $a(0) -  2.45 ]
   if { $delta > 0.0001 } { retcode++ }
   set delta [ expr $a(1) -  3.41 ]
   if { $delta > 0.0001 } { retcode++ }
   set delta [ expr $a(2) - 16.3 ]
   if { $delta > 0.0001 } { retcode++ }

   echo Here in PolyHg
   hgDel $hg
   vectorExprDel $theCoef
   memBlocksPrint
   return $retcode
}


proc pi {{dev /xwindow}} {
    global pg
    set pg [pgstateNew]
    pgstateSet $pg -dev $dev
    pgstateSet $pg -nxwindow 2 -ixwindow 0 -nywindow 2 -iywindow 1
    pgstateOpen $pg
}

proc proco1 {sys} {
    global pg pv

    set pv(xo) [vectorExprNew 612]
    set pv(yo) [vectorExprNew 612]

    set i 0
    for {set thisy -80} {$thisy < 90} {set thisy [expr $thisy + 10]} {
	for {set thisx 5} {$thisx < 360} {set thisx [expr $thisx+10]} {
	    if {$sys == 8} {
		set list [proco $thisx $thisy \
			-mode 0 -sys $sys -rho 0 -a0 0 -d0 90 -dx -1 -dy 1]
	    } else {
		set list [proco $thisx $thisy \
			-mode 0 -sys $sys -rho 0 -a0 180 -d0 0 -dx -1 -dy 1]
	    }
	    handleSet $pv(xo).vec<$i> [keylget list xout]
	    handleSet $pv(yo).vec<$i> [keylget list yout]
	    incr i
	}
    }
    pgstateSet $pg -just 1 -symb 1 -plotTitle "proco test for sys=$sys"
    pgstateSet $pg -xopt " " -yopt " "
    pgstateSet $pg -icMark $sys
    vPlot $pg $pv(xo) $pv(yo)
}

proc pt1 {{dev /xwindow}} {
    
    global pg
    pi $dev
    pgAsk 0; sleep 1; foreach sys "1 2 3 4" {proco1 $sys}
    pgAsk 0; sleep 1; foreach sys "5 6 7 8" {proco1 $sys}
    pgAsk 0; sleep 1; foreach sys "9 10" {proco1 $sys}
    pgAsk 0; sleep 1; pgstateClose $pg
}

proc pt2 { } {
    set rho 45
    set a0 45
    set d0 45
    set dx -0.01
    set dy 0.02

    set sys 1

    set a 44.5
    set d 45.5

    set lst1 [proco $a $d  -a0 $a0 -d0 $d0 -dx $dx -dy $dy -rho $rho -sys $sys -mode 0]
    set x [keylget lst1 xout]
    set y [keylget lst1 yout]
    echo [format "%10.5f %10.5f %10.5f %10.5f" $a $d $x $y]

    set lst2 [proco $x $d  -a0 $a0 -d0 $d0 -dx $dx -dy $dy -rho $rho -sys $sys -mode 1]
    set a2 [keylget lst2 xout]
    set y2 [keylget lst2 yout]
    echo [format "%10.5f %10.5f %10.5f %10.5f" $a2 $d $x $y2]
    if {[notClose $a2 $a]} {error "proco mode 1: a=$a a2=$a2"}
    if {[notClose $y2 $y]} {error "proco mode 1: y=$y y2=$y2"}

    set lst3 [proco $a $y  -a0 $a0 -d0 $d0 -dx $dx -dy $dy -rho $rho -sys $sys -mode 2]
    set x3 [keylget lst3 xout]
    set d3 [keylget lst3 yout]
    echo [format "%10.5f %10.5f %10.5f %10.5f" $a $d3 $x3 $y]
    if {[notClose $x3 $x]} {error "proco mode 2: x=$x x3=$x3"}
    if {[notClose $d3 $d]} {error "proco mode 2: d=$d d3=$d3"}

    set lst4 [proco $x $y  -a0 $a0 -d0 $d0 -dx $dx -dy $dy -rho $rho -sys $sys -mode 3]
    set a4 [keylget lst4 xout]
    set d4 [keylget lst4 yout]
    echo [format "%10.5f %10.5f %10.5f %10.5f" $a4 $d4 $x $y]
    if {[notClose $a4 $a]} {error "proco mode 3: a=$a a4=$a4"}
    if {[notClose $d4 $d]} {error "proco mode 3: d=$d d4=$d4"}

}

proc notClose {a b} {
    if {[abs [expr $a-$b]] > 0.0001} {
	return 1
    } else {
	return 0
    }
}

proc tst_proco { } {
    pt2
    pt1
    return 0
}

proc tst_regColorPlot2 { } {
    global reg pg tr
    pgInit
    set reg [regNew 100 100 -type FL32]
    regSetWithDbl $reg 100
    set sr [subRegNew $reg 100 50 0 0]
    regSetWithDbl $sr 200
    regDel $sr
    loop i 0 100 {
	set v [expr 100+$i]
	regPixSetWithDbl $reg $i 25 $v
	regPixSetWithDbl $reg $i 26 $v
	regPixSetWithDbl $reg $i 49 $v
	regPixSetWithDbl $reg $i 50 $v
	regPixSetWithDbl $reg $i 75 $v
	regPixSetWithDbl $reg $i 76 $v
    }
    set tr [transNew]
    pgstateSet $pg -just 1 -xopt "" -yopt "" -xfract 0.0 -yfract 0.0
    foreach i "1 2 3 4 5 6" {
	pgAsk 0
	regColorPlot $pg $reg 100 200 $tr 0 $i -ylab "cmode=$i"
	sleep 1
    }
    pgAsk 0
    pgstateClose $pg
    regDel $reg
    transDel $tr
    pgstateDel $pg
    return 0
}

proc tst_regPlot { } {
    global reg pg tr
    pgInit
    set reg [regNew 100 100 -type FL32]
    regSetWithDbl $reg 100
    set sr [subRegNew $reg 100 50 0 0]
    regSetWithDbl $sr 200
    regDel $sr
    loop i 0 100 {
	set v [expr 100+$i]
	regPixSetWithDbl $reg $i 25 $v
	regPixSetWithDbl $reg $i 26 $v
	regPixSetWithDbl $reg $i 49 $v
	regPixSetWithDbl $reg $i 50 $v
	regPixSetWithDbl $reg $i 75 $v
	regPixSetWithDbl $reg $i 76 $v
    }
    set tr [transNew]
    pgstateSet $pg -just 1 -xopt "" -yopt "" -xfract 0.0 -yfract 0.0
    foreach mode "0 1 2" {
	pgAsk 0
	regColorPlot $pg $reg 100 200 $tr $mode -xlab "mode=$mode"
	sleep 2
    }
    pgAsk 0
    pgstateClose $pg
    regDel $reg
    transDel $tr
    pgstateDel $pg
    return 0
}

proc tst_hg2 { } {
    global h pg

    pgInit

    seedSet 1234771

    set h(hg2) [hg2New 0 100 100 150 250 100 \
	    -xlab "xlab" -ylab "ylab" -name "y vs. x"]

    loop i 0 10000 {
	set x [randNormalGet -mean  25 -sigma 10]
	set y [randNormalGet -mean 225 -sigma 10]
	hg2Fill $h(hg2) $x $y 1.0
    }
	
    loop i 0 5000 {
	set x [randNormalGet -mean  75 -sigma 10]
	set y [randNormalGet -mean 175 -sigma 10]
	hg2Fill $h(hg2) $x $y 1.0
    }
    
    set h(cvec) [vFromL "5 50 500 5000"]
    pgAsk 0
    hg2Cont $pg $h(hg2) $h(cvec)
    
    pgstateSet $pg -just 1
    pgAsk 0
    hg2Gray $pg $h(hg2) 0 10
    
    vectorExprDel $h(cvec)
    pgAsk 0
    pgstateClose $pg
    hg2Del $h(hg2)
    pgstateDel $pg

    return 0
}






