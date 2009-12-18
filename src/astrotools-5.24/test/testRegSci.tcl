proc testRegSci {} {

    foreach cmd [list \
	    tst_regPixRangeSubstitute tst_regStatNoiseAdd \
	    tst_regSigmaClip tst_regGaussiansAdd tst_regMap \
	    tst_regMedianFindAsDbl tst_regModeFind tst_regConvolve \
	    tst_regSkyFind tst_regBin tst_regMedianFind \
	    tst_regMedianFindByColumn tst_regMedianFindByRow \
	    tst_regClippedMeanFind tst_regCorrectWithVec \
	    tst_regPixValSubstitute tst_regCorrectQuick
    ] {
	if {[$cmd] != 0} {
	    error "Trouble in $cmd in testRegSci"
	    return 1
	}
    }
    return 0
}



proc tst_regPixRangeSubstitute {} {

  set reg [regNew 10 10 -type FL32]
  regSetWithDbl $reg 100
  regStatNoiseAdd $reg
  regPixRangeSubstitute $reg 0 1000 50
  set list [regStatsFind $reg]
  if ([abs [keylget list mean]-50.]>0.1) {return 1}
  if ([abs [keylget list sigma]-0.]>0.1) {return 1}
  regDel $reg

  return 0

}

proc tst_regStatNoiseAdd {} {

  set reg [regNew 100 100 -type FL32]
  regSetWithDbl $reg 100
  regStatNoiseAdd $reg
  set list [regStatsFind $reg]
  if ([abs [keylget list mean]-100]>0.5) {return 1}
  if ([abs [keylget list sigma]-10]>0.5) {return 1}
  regDel $reg

  return 0

}

proc tst_regSigmaClip {} {

  set reg [regNew 9 9 -type FL32]
  regSetWithDbl $reg 50.3
  set list [regSigmaClip $reg 3 3]
  if ([abs [keylget list mean]-50.3]>0.01) {return 1}
  if ([abs [keylget list sigma]]>0.01) {return 1}
  regDel $reg

  return 0

}

proc tst_regGaussiansAdd {} {

  set reg [regReadAsFits [regNew] regSciTest.fits]
  set reg2 [regNew 9 9 -type FL32]
  regSetWithDbl $reg2 0
  regGaussiansAdd $reg2 4.5 4.5 50
  regSubtract $reg2 $reg
  set list [regStatsFind $reg2]
  if ([abs [expr abs([keylget list high]) + abs([keylget list low]) ]]>0.01) {return 1}
  regDel $reg
  regDel $reg2

  return 0
}

proc tst_regMap {} {

  set reg0 [regReadAsFits [regNew] regSciTest.fits]
  regMap $reg0 -rowoff 0 -coloff 0 -theta [expr 3.1415926535/2.0] -mag 0.333333
  regMap $reg0 -rowoff 0 -coloff 0 -theta [expr 3.1415926535/2.0] -mag 3.0
  set reg1 [regReadAsFits [regNew] regSciTest.fits]
  regSubtract $reg1 $reg0
  set list [regStatsFind $reg1]
  if ([abs [expr [keylget list high]-[keylget list low]]]>11.0) {return 1}
  regDel $reg0
  regDel $reg1

  return 0
}

proc tst_regMedianFindAsDbl {} {

  set reg [regNew 9 9]
  regSetWithDbl $reg 10
  set median [regMedianFindAsDbl $reg]

  if ([expr $median-10]!=0) {return 1}
  regDel $reg

  return 0
}

proc tst_regModeFind {} {

  set reg [regNew 9 9]
  regSetWithDbl $reg 10
  set mode [regModeFind $reg]

  if ([expr $mode-10]!=0) {return 1}
  regDel $reg

  return 0
}

proc tst_regCorrectWithVec {} {
    global reg vec
    #there are three cases under the sheets. FLOAT, U16  and all others

    #test case 1
    set reg [regNew 512 512 -type FL32]
    regSetWithDbl $reg 1
    # make two element vector, 1.0 and 1.2
    set vec [vectorExprEval 1,1.2,.2]  
    regCorrectWithVec $reg $vec
    set stats [regStatsFind $reg]
    if {[keylget stats sigma] > 0.000003} {
	echo bad sigma,[keylget stats sigma] test case 1; return 1}
    if {[keylget stats mean]  != 1.2} {
	echo bad mean, [keylget stats mean]  test case 1; return 1}
    if {[keylget stats high]  != 1.2} {
	echo bad high, [keylget stats high]  test case 1; return 1}
    if {[keylget stats low]   != 1.2} {
	echo bad low,  [keylget stats low]   test case 1; return 1}
    regDel $reg
    vectorExprDel $vec

    #test case 2
    set reg [regNew 512 512 -type FL32]
    regSetWithDbl $reg 0.5
    # make two element vector, 1.0 and 1.2
    set vec [vectorExprEval 1,1.25,.25]  
    regCorrectWithVec $reg $vec
    set stats [regStatsFind $reg]
    if {[keylget stats sigma] != 0.0} {
	echo bad sigma,[keylget stats sigma] test case 2; return 1}
    if {[keylget stats mean]  != 0.5625} {
	echo bad mean, [keylget stats mean]  test case 2; return 1}
    if {[keylget stats high]  != 0.5625} {
	echo bad high, [keylget stats high]  test case 2; return 1}
    if {[keylget stats low]   != 0.5625} {
	echo bad low,  [keylget stats low]   test case 2; return 1}
    regDel $reg
    vectorExprDel $vec


    #test case 3
    set reg [regNew 513 615 -type U16]
    regSetWithDbl $reg 20
    # make twenty-two element vector, set all to 1.0, except 20th to 1.005
    set vec [vectorExprEval 0,22,1]
    vectorExprSet $vec "( $vec == 20 ? 1.005 : 1.0)"
    regCorrectWithVec $reg $vec
    set stats [regStatsFind $reg]
    if {[keylget stats sigma] > 0.3001} {
	echo bad sigma,[keylget stats sigma] test case 3.1; return 1}
    if {[keylget stats sigma] < 0.2990} {
	echo bad sigma,[keylget stats sigma] test case 3.2; return 1}
    if {[keylget stats mean]  > 20.101} {
	echo bad mean, [keylget stats mean]  test case 3.3; return 1}
    if {[keylget stats mean]  < 20.098} {
	echo bad mean, [keylget stats mean]  test case 3.4; return 1}
    if {[keylget stats high]  != 21.0}  {
	echo bad high, [keylget stats high]  test case 3.5; return 1}
    if {[keylget stats low]   != 20.0}  {
	echo bad low,  [keylget stats low]   test case 3.6; return 1}
    regDel $reg
    vectorExprDel $vec

    #test case 4
    set reg [regNew 615 1000  -type S16]
    regSetWithDbl $reg 19
    set vec [vectorExprEval 0,19,1]
    vectorExprSet $vec "( $vec == 19 ? 1.01 : 0.0)"
    regCorrectWithVec $reg $vec
    set stats [regStatsFind $reg]
    if {[keylget stats sigma] > 0.394} {
	echo bad sigma,[keylget stats sigma] test case 4.1; return 1}
    if {[keylget stats sigma] < 0.388} {
	echo bad sigma,[keylget stats sigma] test case 4.2; return 1}
    if {[keylget stats mean]  > 19.20} {
	echo bad mean, [keylget stats mean]  test case 4.3; return 1}
    if {[keylget stats mean]  < 19.18} {
	echo bad mean, [keylget stats mean]  test case 4.4; return 1}
    if {[keylget stats high]  != 20.0}  {
	echo bad high, [keylget stats high]  test case 4.5; return 1}
    if {[keylget stats low]   != 19.0}  {
	echo bad low,  [keylget stats low]   test case 4.6; return 1}
    regDel $reg
    vectorExprDel $vec

# test Case 5 float regions where the list does not correspond to integers
    set reg [regNew 10 1  -type FL32]
    regSetWithDbl $reg 3
    set vec [vectorExprEval 1,2,1]
    regCorrectWithVec $reg $vec -firstC  0 -lastC  1000
    set stats [regStatsFind $reg]
    if {[keylget stats mean]  > 3.00901} {
	echo bad mean, [keylget stats mean]  test case 4.3; return 1}
    if {[keylget stats mean]  < 3.00898} {
	echo bad mean, [keylget stats mean]  test case 4.4; return 1}
    regDel $reg
    vectorExprDel $vec


    return 0

}

proc tst_regDistortWithVec { } {
    global rd
    set rd(reg) [regNew 100 100 -type FL32]
    regSetWithDbl $rd(reg) 1000
    regGaussiansAdd $rd(reg) 50 60 1000000
    regGaussiansAdd $rd(reg) 50 40 10000000
    set rd(reg.0) [regNewFromReg $rd(reg)]
    set v [vFromL "0.90 .91 .92 .93 .94 .95 .96 .97 .98 .99"]
    regDistortWithVec $rd(reg) $v -firstC 2000 -lastC 10000000
    set rd(reg.1) [regNewFromReg $rd(reg)]
    regCorrectWithVec $rd(reg) $v -firstC 2000 -lastC 10000000
    set rd(reg.2) [regNewFromReg $rd(reg)]

    regDivide $rd(reg.0) $rd(reg.1)    -regOut [set rd(reg.0by1) [regNew 100 100 -type FL32] ]
    regDivide $rd(reg.0) $rd(reg.2)    -regOut [set rd(reg.0by2) [regNew 100 100 -type FL32] ]
    regDivide $rd(reg.1) $rd(reg.2)    -regOut [set rd(reg.1by2) [regNew 100 100 -type FL32] ]

}

proc tst_regConvolve {} {

  set reg0 [regReadAsFits [regNew] regSciTest.fits]
  set reg [regNew 9 9 -type FL32]
  regSetWithDbl $reg 0
  regPixSetWithDbl $reg 4 4 314.13768299999998
  regConvolve $reg $reg0

  regSubtract $reg $reg0
  set list [regStatsFind $reg]
  if ([keylget list high]>[expr [keylget list low]+.05]) {return 1}
  regDel $reg0
  regDel $reg

  return 0
}

proc tst_regSkyFind {} {

  set reg [regNew 9 9 -type FL32]
  regSetWithDbl $reg 10
  regPixSetWithDbl $reg 4 4 11
  regPixSetWithDbl $reg 4 5 9
  set sky [regSkyFind $reg]

  if ([abs [expr $sky-10]]>0.5) {return 1}
  regDel $reg

  return 0
}

proc tst_regBin {} {
# there is specil case  code in regBin to speed i up. It is important 
# to test inputs types of FL32 U16 and another
  foreach t {U16 FL32 U8} {
     set reg [regNew 9 9 -type $t]
     regSetWithDbl $reg 3
     set reg2 [regBin $reg -both 3]
     set list [regStatsFind $reg2]
  
     if (([regMedianFindAsDbl $reg2]!=27)||([keylget list high]>[keylget list low])) {return 1}
     regDel $reg
     regDel $reg2
  }
  return 0
}

proc tst_regMedianFind {} {

  set reg1 [regNew 9 9 -type FL32] ; regSetWithDbl $reg1 0
  set reg2 [regNew 9 9 -type FL32] ; regSetWithDbl $reg2 1
  set reg3 [regNew 9 9 -type FL32] ; regSetWithDbl $reg3 2

  regMedianFind "$reg1 $reg2 $reg3"

  regSubtract $reg1 $reg2
  set list [regStatsFind $reg1]
  if (([regMedianFindAsDbl $reg1]!=0)||([keylget list high]>[keylget list low])) {return 1}

  regDel $reg1
  regDel $reg2
  regDel $reg3

  return 0
}


proc tst_regMedianFindByColumn {} {

  set nrow 9
  set ncol 9

  # test A: find the median of a region with same values along every col
  set input_reg [regNew $nrow $ncol -type U16]
  loop row 0 $nrow {
    loop col 0 $ncol {
      regPixSetWithDbl $input_reg $row $col $col
    }
  }
  set output_reg [regNew 1 $ncol -type FL32]
  
  regMedianFindByColumn $input_reg $output_reg
  loop col 0 $ncol {
    set val [regPixGet $output_reg 0 $col]
    if { $val != $col } {
      echo "TEST-ERR: regMedianByColumn fails test A"
      return 1
    }
  }
  regDel $input_reg
  regDel $output_reg

  # test B: now find median of region with different valus along every col
  set input_reg [regNew $nrow $ncol -type U16]
  loop row 0 $nrow {
    loop col 0 $ncol {
      regPixSetWithDbl $input_reg $row $col $row
    }
  }
  set output_reg [regNew 1 $ncol -type FL32]
  regMedianFindByColumn $input_reg $output_reg
  # this is what the correct answer _ought_ to be in each column
  set correct [expr $nrow/2]
  loop col 0 $ncol {
    set val [regPixGet $output_reg 0 $col]
    if { $val != $correct } {
      echo "TEST-ERR: regMedianByColumn fails test B"
      return 1
    }
  }
  regDel $input_reg
  regDel $output_reg

  return 0
}
  

proc tst_regMedianFindByRow {} {

  set nrow 9
  set ncol 9

  # test A: find the median of a region with same values along every row
  set input_reg [regNew $nrow $ncol -type U16]
  loop row 0 $nrow {
    loop col 0 $ncol {
      regPixSetWithDbl $input_reg $row $col $row
    }
  }
  set output_reg [regNew $nrow 1 -type FL32]
  regMedianFindByRow $input_reg $output_reg
  loop row 0 $nrow {
    set val [regPixGet $output_reg $row 0]
    if { $val != $row } {
      echo "TEST-ERR: regMedianByRow fails test A"
      return 1
    }
  }
  regDel $input_reg
  regDel $output_reg

  # test B: now find median of region with different valus along every row
  set input_reg [regNew $nrow $ncol -type U16]
  loop row 0 $nrow {
    loop col 0 $ncol {
      regPixSetWithDbl $input_reg $row $col $col
    }
  }
  set output_reg [regNew $nrow 1 -type FL32]
  regMedianFindByRow $input_reg $output_reg
  # this is what the correct answer _ought_ to be in each column
  set correct [expr $ncol/2]
  loop row 0 $nrow {
    set val [regPixGet $output_reg $row 0]
    if { $val != $correct } {
      echo "TEST-ERR: regMedianByRow fails test B"
      return 1
    }
  }
  regDel $input_reg
  regDel $output_reg
  
  return 0
}


proc tst_regClippedMeanFind {} {

  set reg1 [regNew 3 3 -type FL32]
  set reg2 [regNew 3 3 -type FL32]
  set reg3 [regNew 3 3 -type FL32]
  regSetWithDbl $reg1 3
  regSetWithDbl $reg2 3
  regSetWithDbl $reg3 3
  regPixSetWithDbl $reg3 1 1 1000
  regClippedMeanFind "$reg1 $reg2 $reg3" 3 1
  set list [regStatsFind $reg1]
  if ([abs [keylget list mean]-3.0]>0.01) {echo "Failed in tst_regClippedMeanFind"; return 1}

  set flag 0
  regDel $reg1; regDel $reg2; regDel $reg3

  # first, we make some tests with FL32 regions ...

  set reg1 [regNew 9 9 -type FL32] ; regSetWithDbl $reg1 1
  set reg2 [regNew 9 9 -type FL32] ; regSetWithDbl $reg2 2
  set reg3 [regNew 9 9 -type FL32] ; regSetWithDbl $reg3 3
  set reg4 [regNew 9 9 -type FL32] ; regSetWithDbl $reg4 4
  set reg5 [regNew 9 9 -type FL32] ; regSetWithDbl $reg5 5
  set reg6 [regNew 9 9 -type FL32] ; regSetWithDbl $reg6 6
  set reg7 [regNew 9 9 -type FL32] ; regSetWithDbl $reg7 7
  set reg8 [regNew 9 9 -type FL32] ; regSetWithDbl $reg8 8
  set reg9 [regNew 9 9 -type FL32] ; regSetWithDbl $reg9 9
  set target [regNew 9 9 -type FL32]

  # test A: no clipping
  set niter 1
  set nsigma 0
  regClippedMeanFind "$reg1 $reg2 $reg3 $reg4 $reg5 $reg6 $reg7 $reg8 $reg9" \
                            -regtarget $target $niter $nsigma
  set val [format "%.3f" [regPixGet $target 5 5]]
  if { $val != 5.000 } {
    echo "TEST-ERR: regClippedMeanFind fails test A: pix (5,5) is $val, not 5.000"
    set flag 1
  }
  set val [format "%.3f" [regPixGet $target 2 8]]
  if { $val != 5.000 } {
    echo "TEST-ERR: regClippedMeanFind fails test A: pix (2,8) is $val, not 5.000"
    set flag 1
  }

  # test B: add clipping, of a single iteration
  set niter 2
  set nsigma 1
  regClippedMeanFind "$reg1 $reg2 $reg3 $reg4 $reg5 $reg6 $reg7 $reg8 $reg9" \
                              -regtarget $target $niter $nsigma
  set val [format "%.3f" [regPixGet $target 5 5]]
  if { $val != 5.000 } {
    echo "TEST-ERR: regClippedMeanFind fails test B: pix (5,5) is $val, not 5.000"
    set flag 1
  }
  set val [format "%.3f" [regPixGet $target 2 8]]
  if { $val != 5.000 } {
    echo "TEST-ERR: regClippedMeanFind fails test B: pix (2,8) is $val, not 5.000"
    set flag 1
  }

  #
  # turn this off since it now returns more gracefully
  #
  # test C: make sure that 'nsigma = -1' is caught as an error
  # set niter 1
  # set nsigma -1
  # if { [catch { regClippedMeanFind \
  #        "$reg1 $reg2 $reg3 $reg4 $reg5 $reg6 $reg7 $reg8 $reg9" \
  #         -regtarget $target $niter $nsigma } ] != 1 } {
  #   echo "TEST-ERR: regClippedMeanFind fails test C: allowed nsigma = -1"
  #   set flag 1
  # }

  regDel $reg1; regDel $reg2; regDel $reg3
  regDel $reg4; regDel $reg5; regDel $reg6
  regDel $reg7; regDel $reg8; regDel $reg9

  regDel $target


  # next, we make some tests with U16 regions (which we handle in
  #   a special case, so it's necessary to do this)

  set reg1 [regNew 9 9 -type U16] ; regSetWithDbl $reg1 1
  set reg2 [regNew 9 9 -type U16] ; regSetWithDbl $reg2 2
  set reg3 [regNew 9 9 -type U16] ; regSetWithDbl $reg3 3
  set reg4 [regNew 9 9 -type U16] ; regSetWithDbl $reg4 4
  set reg5 [regNew 9 9 -type U16] ; regSetWithDbl $reg5 7 
  set reg6 [regNew 9 9 -type U16] ; regSetWithDbl $reg6 6
  set reg7 [regNew 9 9 -type U16] ; regSetWithDbl $reg7 7
  set reg8 [regNew 9 9 -type U16] ; regSetWithDbl $reg8 8
  set reg9 [regNew 9 9 -type U16] ; regSetWithDbl $reg9 9
  set target [regNew 9 9 -type U16]

  # test D: no clipping; true mean is 5.22, U16 should be 5.00
  set niter 1
  set nsigma 0
  regClippedMeanFind "$reg1 $reg2 $reg3 $reg4 $reg5 $reg6 $reg7 $reg8 $reg9" \
                            -regtarget $target $niter $nsigma
  set val [format "%.3f" [regPixGet $target 5 5]]
  if { $val != 5.000 } {
    echo "TEST-ERR: regClippedMeanFind fails test D: pix (5,5) is $val, not 5.000"
    set flag 1
  }
  set val [format "%.3f" [regPixGet $target 2 8]]
  if { $val != 5.000 } {
    echo "TEST-ERR: regClippedMeanFind fails test D: pix (2,8) is $val, not 5.000"
    set flag 1
  }

  # test E: now make true mean 5.55, U16 should be 6.00
  regSetWithDbl $reg5 10

  set niter 1
  set nsigma 0
  regClippedMeanFind "$reg1 $reg2 $reg3 $reg4 $reg5 $reg6 $reg7 $reg8 $reg9" \
                              -regtarget $target $niter $nsigma
  set val [format "%.3f" [regPixGet $target 5 5]]
  if { $val != 6.000 } {
    echo "TEST-ERR: regClippedMeanFind fails test E: pix (5,5) is $val, not 6.000"
    set flag 1
  }
  set val [format "%.3f" [regPixGet $target 2 8]]
  if { $val != 6.000 } {
    echo "TEST-ERR: regClippedMeanFind fails test E: pix (2,8) is $val, not 6.000"
    set flag 1
  }


  regDel $reg1; regDel $reg2; regDel $reg3
  regDel $reg4; regDel $reg5; regDel $reg6
  regDel $reg7; regDel $reg8; regDel $reg9

  regDel $target

  return $flag
}


proc tst_regPixValSubstitute {} {

  set flag 0

  set reg [regNew 9 9 -type FL32] 
  regSetWithDbl $reg 1

  # test A: find a zero
  regPixSetWithDbl $reg 5 4 0
  regPixValSubstitute $reg 0 1
  set val [format "%.3f" [regPixGet $reg 5 4]]
  if { $val != 1.000 } {
    echo "TEST-ERR: regPixValSubstitute fails test A: pix (5,4) is $val, not 1.000"
    set flag 1
  }

  # test B: find and replace a negative number
  regPixSetWithDbl $reg 5 5 -1
  regPixValSubstitute $reg -1 1.3
  set val [format "%.3f" [regPixGet $reg 5 5]]
  if { $val != 1.300 } {
    echo "TEST-ERR: regPixValSubstitute fails test B: pix (5,5) is $val, not 1.300"
    set flag 1
  }

  # test C: find and replace a big number
  regPixSetWithDbl $reg 5 6 70000
  regPixValSubstitute $reg 70000 -1.2
  set val [format "%.3f" [regPixGet $reg 5 6]]
  if { $val != -1.200 } {
    echo "TEST-ERR: regPixValSubstitute fails test C: pix (5,6) is $val, not -1.200"
    set flag 1
  }

  # test D: test the -compare option
  regPixSetWithDbl $reg 5 6 70000
  regPixValSubstitute $reg 7 -1.2 -compare 2
  set val [format "%.3f" [regPixGet $reg 5 6]]
  if { $val != -1.200 } {
    echo "TEST-ERR: regPixValSubstitute fails test D: pix (5,6) is $val, not -1.200"
    set flag 1
  }

  # all done, clean up
  regDel $reg

  return $flag
}


proc tst_regCorrectQuick {} {

    # recall the regCorrectQuick works only on U16 regions

    #####################################################################
    # test case 1: should produce no change at all
    #
    set reg [regNew 512 512 -type U16]
    regSetWithDbl $reg 2000
    # make vector with all elements equal to 1.0
    set lvec { 1.0 1.0 1.0 1.0 }
    set firstC 1000
    set lastC  4000
    set vec [vFromL $lvec]

    regCorrectQuick $reg $vec -firstC $firstC -lastC $lastC

    set stats [regStatsFind $reg]
    if {[keylget stats sigma] > 0.000003} {
	echo bad sigma,[keylget stats sigma] test case 1; return 1}
    if {[keylget stats mean]  != 2000.0} {
	echo bad mean, [keylget stats mean]  test case 1; return 1}
    if {[keylget stats high]  != 2000.0} {
	echo bad high, [keylget stats high]  test case 1; return 1}
    if {[keylget stats low]   != 2000.0} {
	echo bad low,  [keylget stats low]   test case 1; return 1}
    regDel $reg
    vectorExprDel $vec

    #####################################################################
    # test case 2: should produce exact factor of 1.2
    #
    set reg [regNew 512 512 -type U16]
    regSetWithDbl $reg 2000
    # make vector with all elements equal to 1.2
    set lvec { 1.2 1.2 1.2 1.2 }
    set firstC 1000
    set lastC  4000
    set vec [vFromL $lvec]

    regCorrectQuick $reg $vec -firstC $firstC -lastC $lastC

    set stats [regStatsFind $reg]
    if {[keylget stats sigma] > 0.000003} {
	echo bad sigma,[keylget stats sigma] test case 1; return 1}
    if {[keylget stats mean]  != 2400.0} {
	echo bad mean, [keylget stats mean]  test case 1; return 1}
    if {[keylget stats high]  != 2400.0} {
	echo bad high, [keylget stats high]  test case 1; return 1}
    if {[keylget stats low]   != 2400.0} {
	echo bad low,  [keylget stats low]   test case 1; return 1}
    regDel $reg
    vectorExprDel $vec


    #####################################################################
    # test case 3: check for gradual ramp-up of the input
    #
    set reg [regNew 10 10 -type U16]
    loop row 0 10 {
      loop col 0 10 {
        regPixSetWithDbl $reg $row $col [expr $row+1]
      }
    }
    # make vector with all elements equal to 1.2
    set lvec { 1.0 2.0 3.0 4.0 5.0 6.0 7.0 8.0 9.0 10.0 }
    set firstC 1 
    set lastC  10
    set vec [vFromL $lvec]

    regCorrectQuick $reg $vec -firstC $firstC -lastC $lastC
       
    # now check that each pixel has correct value
    loop row 0 9 {
      set expect [expr ($row+1)*($row+1)]
      loop col 0 10 {
        set val [regPixGet $reg $row $col]
        if { $val != $expect } {
	  echo "test 3: bad pixel ($row, $col) is $val, should be $expect"
          return 1
        }
      }
    }
    regDel $reg
    vectorExprDel $vec

    #####################################################################
    # test case 4: check for proper rounding of integer values
    #
    set reg [regNew 10 100 -type U16]
    loop row 0 10 {
      loop col 0 100 {
        regPixSetWithDbl $reg $row $col [expr ($row+1)*1000]
      }
    }
    # make vector with elements gradually increasing
    set lvec { 1.0 1.000001 1.00001 1.0001 1.0001 \
                                    1.0001 1.0001 1.0001 1.0001 1.0001 }
    set firstC 1000
    set lastC  10000
    set vec [vFromL $lvec]

    regCorrectQuick $reg $vec -firstC $firstC -lastC $lastC
       
    # now check that each pixel has correct value
    loop row 0 9 {
      set expect [expr ($row+1)*1000]
      set sum1 0
      set sum2 0
      loop col 0 100 {
        set val [regPixGet $reg $row $col]
        if { $val == $expect } {
          set sum1 [expr $sum1+1]
        } elseif { $val == [expr $expect+1] } {
          set sum2 [expr $sum2+1]
        } else {
	  echo "test 4: bad pixel ($row, $col) is $val, should be $expect"
          return 1
        }
      }
      #
      # now check to see that the ratio of value N to value N+1 is reasonable
      set min_expect 0
      set max_expect 0
      if { $row == 0 } {
        # row 0 should have no values N+1
        if { $sum2 > 0 } {
          echo "test 4: row 0 had $sum2 improper values"
          return 1
        }
      } elseif { $row == 1 } {
        # row 1 should have, at most, 3 values N+1
        if { $sum2 > 3 } {
          echo "test 4: row 1 had $sum2 rounded-up values, more than 3"
          return 1
        }
      } elseif { $row == 2 } {
        # row 2 should have, at most, 15 values N+1
        if { $sum2 > 15 } {
          echo "test 4: row 2 had $sum2 rounded-up values, more than 15"
          return 1
        }
      } else {
        #
        # rows 3-10 should have, I would guess,
        #
        #     an average of   N = ((row+1)/10)*100 rounded-up values
        #     at least        N*0.8    
        #     at most         N*1.2
        #
        # we don't really know, for sure, because the rounding process
        # uses random numbers.  But my guess is that this extra 20%
        # provides a decent upper and lower bound on the number we can expect
        set min_expect [expr (($row+1)/10.0)*80]
        set max_expect [expr (($row+1)/10.0)*120]
        if { $sum2 > $max_expect } {
          echo "test 4: row $row had $sum2 rounded-up values, more than $max_expect"
          return 1
        }
        if { $sum2 < $min_expect } {
          echo "test 4: row $row had $sum2 rounded-up values, less than $min_expect"
          return 1
        }
      }

      # this line purely for debugging, not for real use
      #  echo "row $row: sum1 = $sum1, sum2 = $sum2, min = $min_expect, max = $max_expect "
  
   
    }
    regDel $reg
    vectorExprDel $vec

    return 0
}








