proc testLinearFits {{out /ps}} {
   if { [tst_vLinEqn] != 0 } {
	error "Error with tst_vLinEqn in testLinearFits.tcl"
   }  
   if { [tst_vLinFitBces] != 0 } {
       error "Error with tst_vLinFitBces in testLinearFits.tcl"
   }   
    return 0
}

proc compare {a b} {
    set delta [expr $a-$b]
    set delta [sqrt [expr $delta*$delta]]
    if {$delta<0.0001} {
	return 0
    } else {
	return 1
    }
}

proc tst_vLinEqn {} {

# solve the matrix problem:
#
#   | 2 -3 |  x = | -13 |
#   | 3  2 |  y   | -13 |
#
# which has the solution (-5, 1)
# 
  set x1 { 2 3}
  set x2 {-3 2}
  set y {-13 -13}

  set vx1 [vFromL $x1 x1]
  set vx2 [vFromL $x2 x2]
  set vy  [vFromL $y  y]

  set eqnFunc [ vLinEqn $vy "$vx1 $vx2" ]
  echo $eqnFunc

  set retcode 0
  if { [exprGet $eqnFunc.va<0>->vec<0>] != -5.0 } { set retcode 1 }
  if { [exprGet $eqnFunc.va<0>->vec<1>] !=  1.0 } { set retcode 1 }

# solve the matrix problem:
#
# | -3  2 |      | -19 |
# |  4  1 |  x = |  18 |
# |  0  1 |  y   |  -2 |
# |  1  0 |      |   5 |
#
#  which has the solution (1, 2)
#
  set x1 {-3 4 0 1}
  set x2 {2 1 1 0}
  set y {-19 18 -2 5}

  vectorExprDel $vx1 
  vectorExprDel $vx2 
  set vx1 [vFromL $x1 x1]
  set vx2 [vFromL $x2 x2]
  set vy  [vFromL $y  y]

#  funcDel $eqnFunc
  set eqnFunc [ vLinEqn $vy "$vx1 $vx2" ]
  echo $eqnFunc

  if { [exprGet $eqnFunc.va<0>->vec<0>] != 5.0 } { set retcode [ expr $retcode + 10 ] }
  if { [exprGet $eqnFunc.va<0>->vec<1>] != -2.0 } { set retcode [ expr $retcode + 10 ] }

# solve the matrix problem:
#
# | 2  0 -1 |  x   | 2 |
# | 6  5  3 |  y = | 7 |
# | 2 -1  0 |  z   | 4 |
#
# which has the solution (1.5, -1, 1)
#
  set x1 {2 6 2}
  set x2 {0 5 -1}
  set x3 {-1 3 0}
  set y {2 7 4}

  vectorExprDel $vx1; vectorExprDel $vx2

  set vx1 [vFromL $x1 x1]
  set vx2 [vFromL $x2 x2]
  set vx3 [vFromL $x3 x3]
  set vy  [vFromL $y  y]

#  funcDel $eqnFunc
  set eqnFunc [ vLinEqn $vy "$vx1 $vx2 $vx3" ]
  echo $eqnFunc

  if { [exprGet $eqnFunc.va<0>->vec<0>] !=  1.5 } { set retcode [ expr $retcode + 100 ] }
  if { [exprGet $eqnFunc.va<0>->vec<1>] != -1.0 } { set retcode [ expr $retcode + 100 ] }
  if { [exprGet $eqnFunc.va<0>->vec<2>] !=  1.0 } { set retcode [ expr $retcode + 100 ] }

  vectorExprDel $vx1; vectorExprDel $vx2; vectorExprDel $vx3
# funcDel $eqnFunc


  return $retcode
}

proc tst_vLinFitBces {} {
    # y = 2x +1   , with 0.3 errors on y(1) and y(2)
    set x {0 1 2 3 4 5 }
    set y  {1 3.3 4.7 7 9 11}
    set xe  {0.01 0.01 0.01 0.01 0.01 0.01}
    set ye  {0.01 0.01 0.01 0.01 0.01 0.01}
    set ce  {0.01 0.01 0.01 0.01 0.01 0.01}
    set ce  {0.0 0.0 0.0 0.0 0.0 0.0}

    set vx [vFromL $x x]
    set vy  [vFromL $y   y]
    set vxe  [vFromL $xe  xerr]
    set vye  [vFromL $ye  yerr]
    set vce  [vFromL $ce  co-err]

    set list [vLinFitBces $vx $vxe $vy $vye $vce]
    set ok 0
    foreach thing "slope yInt slopeErr yIntErr yIntErrBoot slopeErrBoot" {
	set data($thing) [keylget list $thing]
	set max 4
	if {$thing == "slopeErr" || $thing == "slopeErrBoot"} { set max 6}
    	loop i 0 $max {
		set item [lindex $data($thing) $i]
		if {$thing == "slope" || $thing == "slopeBoot"} { set correct 1.985 }
		if {$thing == "yInt" || $thing == "yIntBoot"} { set correct 1.035 }
		if {$thing == "slopeErr"} {
			if {$i==0} { set correct 0.0266 }
			if {$i==1} { set correct 0.0256 }
			if {$i==2} { set correct 0.0260 }
			if {$i==3} { set correct 0.0257 }
			if {$i==4} { set correct 0.0261 }
			if {$i==5} { set correct 0.0258 }
		}
		if {$thing == "slopeErrBoot"} {
			if {$i==0} { set correct 0.0485 }
			if {$i==1} { set correct 0.0457 }
			if {$i==2} { set correct 0.0469 }
			if {$i==3} { set correct 0.0462 }
			if {$i==4} { set correct 0.0291 }
			if {$i==5} { set correct 0.0284 }
		}
		if {$thing == "yIntErr"} {
			if {$i==0} { set correct 0.13 }
			if {$i==1} { set correct 0.13 }
			if {$i==2} { set correct 0.13 }
			if {$i==3} { set correct 0.13 }
		}
		if {$thing == "yIntErrBoot"} {
			if {$i==0} { set correct 0.192 }
			if {$i==1} { set correct 0.190 }
			if {$i==2} { set correct 0.191 }
			if {$i==3} { set correct 0.190 }
		}
		if {[fracCompare $item $correct] != 0} { set ok 1 }
    	}
    }

    vectorExprDel $vx
    vectorExprDel $vy
    vectorExprDel $vxe
    vectorExprDel $vye
    vectorExprDel $vce

    return $ok
}

proc fracCompare {a b} {
    set delta [expr ($a-$b)/$b]
    if {$delta<0.005} {
	return 0
    } else {
	return 1
    }
}

