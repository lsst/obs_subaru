proc testSurveyGeometry {} {

    if {[nodeGet] != 95.0} {
	error "nodeGet does not return 95.0"
	return 1
    }

    foreach cmd [list \
	    tst_atRefro tst_stripe_eta tst_nuFromScanLine tst_lookupCCD \
	    tst_defineGC tst_GCDecFromRa tst_angleGCToRa tst_coordconversion\
	    tst_surveyGCconversion tst_Conversion tst_cobe
    ] {
	echo "Calling $cmd"
	flush stdout
	if {[$cmd] != 0} {
	    error "Error in $cmd in testSurveyGeometry -- $cmd exited non zero"
	    return 1
	}   
    }
    return 0    
}

proc tst_atRefro { } {

  set answer [atRefro 2.0 6000.0]
  if {[expr abs($answer - 0.00036644)] > 1e-6} {
	return 1
  } 
	return 0
}

proc tst_stripe_eta {  } {
   set eta [etaFromStripeNumber 23]
   set incl [inclFromStripeNumber 23]
   set sn [stripeNumberFromEta 147.5]
   set nm [mtPatchNameFromSurvey 5 32.6 2]
   set su [surveyFromMtPatchName $nm]
   set lam [keylget su lambda]
   set et  [keylget su eta]

  if { [expr abs($eta - 0.0)] > 1e-6 } {
   return 1
  }
  if { [expr abs($incl - 32.5)] > 1e-6 } {
   return 1
  }
  if { [expr abs($sn - 82)] > 1e-6 } {
   return 1
  }
  if { $nm != "mtPatch-100-36-2" } {
   return 1
  }
  if { [expr abs($et-32.5)] > 1e-6} {
   return 1
  }
  if { [expr abs($lam-5.0)] > 1e-6} {
   return 1
  }
	return 0

}

proc tst_nuFromScanLine {} {

    set answer [nuFromScanLine opCamera-test.par 3 N]
    set diff [expr $answer--0.10531986111111111]
    if ($diff>0.0001) {return 1}
    return 0

}


proc tst_lookupCCD { } {
    global pg pgDevice list at_deg2Rad
    if {![info exists pgDevice]} {
	set pgDevice /NULL
    }

    pgInit $pgDevice
    pgAsk 0

    set xRot 0
    set yRot 0
    set plateRotation 0


    set xl ""
    set yl ""
    set cl [param2Chain opCamera-test.par hdr] 
    set scalef [keylget hdr scalef]
    loop i 0 [llength $cl] {
	set typelist [chainTypeGet [lindex $cl $i]]
	if {[keylget typelist type] == "CCDGEOMETRY"} {
	    set ccdchn [lindex $cl $i]
	    set ccdsize [chainSize $ccdchn]
	} elseif {[keylget typelist type] == "DEWARGEOMETRY"} {
	    set dewarchn [lindex $cl $i]
	    set dewarsize [chainSize $dewarchn]
	}
    }
    set ccdcur [chainCursorNew $ccdchn]
    while {[set ccd [chainWalk $ccdchn $ccdcur]] != ""} {
	set camRow [exprGet $ccd.camRow]
	set camCol [exprGet $ccd.camCol]
	set list [lookupCCDAstrom $cl $hdr $camRow $camCol \
		$xRot $yRot $plateRotation]
	lappend xl [keylget list xPos]
	lappend yl [keylget list yPos]
    }
    chainCursorDel $ccdchn $ccdcur

    set xv [vFromL $xl x(mm)]
    set yv [vFromL $yl y(mm)]
    set xmin [expr [vExtreme $xv min] - 100]
    set xmax [expr [vExtreme $xv max] + 100]
    set ymin [expr [vExtreme $yv min] - 100]
    set ymax [expr [vExtreme $yv max] + 100]
    set xmin -400; set xmax 400; set ymin -400; set ymax 400
    pgstateSet $pg -plotTitle opCamera-test.par -just 1
    pgstateSet $pg -xfract 0.1 -yfract 0.1
    vPlot $pg $xv $yv \
	    -xmin $xmin -xmax $xmax -ymin $ymin -ymax $ymax

    set dewarcur [chainCursorNew $dewarchn]
    set radians [expr $plateRotation*$at_deg2Rad]
    set ct [cos $radians]
    set st [sin $radians]

    while {[set dewar [chainWalk $dewarchn $dewarcur]] != ""} {
	set xb [exprGet $dewar.xb]
	set yb [exprGet $dewar.yb]
	set xpt [expr $ct*($xb-$xRot) - $st*($yb-$yRot) + $xRot]
	set ypt [expr $st*($xb-$xRot) + $ct*($yb-$yRot) + $yRot]

	pgPoint $xpt $ypt 4
    }
    chainCursorDel $dewarchn $dewarcur
    pgPoint $xRot $yRot 5
    set ccdcur [chainCursorNew $ccdchn]

    while {[set ccd [chainWalk $ccdchn $ccdcur]] != ""} {
	set camRow [exprGet $ccd.camRow]
	set camCol [exprGet $ccd.camCol]
	set list [lookupCCDAstrom $cl $hdr $camRow $camCol \
		$xRot $yRot $plateRotation]
	foreach p "rowRef colRef xPos yPos angle arcsecPerPixel" {
	    set $p [keylget list $p]
	}
	# draw a box for each CCD, assuming that each rowRef,colRef is
	# the center of the CCD.
	set radians [expr $at_deg2Rad*$angle]
	set ct [cos $radians]
	set st [sin $radians]
	set xl ""; set yl ""
	
	set x0 [expr $xPos - $colRef*$arcsecPerPixel/$scalef]
	set y0 [expr $yPos - $rowRef*$arcsecPerPixel/$scalef]
	lappend xl [x0y02x $x0 $y0 $xPos $yPos $radians]
	lappend yl [x0y02y $x0 $y0 $xPos $yPos $radians]
	
	set x0 [expr $xPos + $colRef*$arcsecPerPixel/$scalef]
	set y0 [expr $yPos - $rowRef*$arcsecPerPixel/$scalef]
	lappend xl [x0y02x $x0 $y0 $xPos $yPos $radians]
	lappend yl [x0y02y $x0 $y0 $xPos $yPos $radians]
	
	set x0 [expr $xPos + $colRef*$arcsecPerPixel/$scalef]
	set y0 [expr $yPos + $rowRef*$arcsecPerPixel/$scalef]
	lappend xl [x0y02x $x0 $y0 $xPos $yPos $radians]
	lappend yl [x0y02y $x0 $y0 $xPos $yPos $radians]
	
	set x0 [expr $xPos - $colRef*$arcsecPerPixel/$scalef]
	set y0 [expr $yPos + $rowRef*$arcsecPerPixel/$scalef]
	lappend xl [x0y02x $x0 $y0 $xPos $yPos $radians]
	lappend yl [x0y02y $x0 $y0 $xPos $yPos $radians]
	
	pgSfs 2
	pgPoly $xl $yl
    }
    chainCursorDel $ccdchn $ccdcur

    #if {$pgDevice == "/xwindow"} {
	#exec sleep 4
    #}
    #pgAsk 0
    #pgstateClose $pg

    pgstateDel $pg
    vectorExprDel $xv
    vectorExprDel $yv
    loop i 0 [llength $cl] {
	chainDestroy [lindex $cl $i]
    }

    return 0

}

proc x0y02x {x0 y0 xPos yPos radians} {
    set x1 [expr $x0-$xPos]
    set y1 [expr $y0-$yPos]
    set ct [cos $radians]
    set st [sin $radians]
    set x2 [expr $ct*$x1 - $st*$y1]
    return [expr $x2 + $xPos]
}

proc x0y02y {x0 y0 xPos yPos radians} {
    set x1 [expr $x0-$xPos]
    set y1 [expr $y0-$yPos]
    set ct [cos $radians]
    set st [sin $radians]
    set y2 [expr $st*$x1 + $ct*$y1]
    return [expr $y2 + $yPos]
}

proc tst_defineGC {} {

  set result [defineGC 0 0 90 5]
  set node [keylget result node]
  set inc [keylget result inc]

  if ($node!=0.0||$inc!=5.0) {return 1}

  return 0
}

proc tst_GCDecFromRa {} {

  set result [GCDecFromRa 90 -node 0.00000000 -inclination 5.00000000]

  if ($result!=5.0) {return 1}

  return 0
}

proc tst_angleGCToRa {} {

  set result [angleGCToRa 0 0 -node 0.00000000 -inclination 5.00000000]

  if ($result!=5.0) {return 1}

  return 0
}

proc tst_coordconversion {} {

  set gal [eqToGal 0.0 0.0]
  set eq [galToEq [keylget gal gLong] [keylget gal gLat]]
  set radiff [keylget eq ra]
  if ($radiff>0.00001) {return 1}
  set decdiff [keylget eq dec]
  if ($decdiff>0.00001) {return 1}

  set surv [eqToSurvey 0.0 0.0]
  set eq [surveyToEq [keylget surv lambda] [keylget surv eta]]
  set radiff [keylget eq ra]
  if ($radiff>0.00001) {return 1}
  set decdiff [keylget eq dec]
  if ($decdiff>0.00001) {return 1}

  set GC [eqToGC 0.1 0.1]
  set eq [GCToEq [keylget GC mu] [keylget GC nu]]
  set radiff [expr [keylget eq ra]-0.1]
  if ($radiff>0.00001) {return 1}
  set decdiff [expr [keylget eq dec]-0.1]
  if ($decdiff>0.00001) {return 1}

  return 0
}

proc tst_Conversion {} {
    set v1 [vFromL {0 1 2 3 4 5 6 7 8 9}]
    set v2 [vFromL {1 3 5 7 9 11 7 8 9 10}]
    set v1o [vectorExprEval $v1]
    set v2o [vectorExprEval $v2]
    vEqToGC $v1 $v2
    vGCToEq $v1 $v2
    loop i 0 [exprGet $v1o.dimen] {
	#echo $i [exprGet $v1.vec<$i>] [exprGet $v2.vec<$i>]
	if {[abs [expr [exprGet $v1.vec<$i>]-[exprGet $v1o.vec<$i>]]] >0.00001 } {
	    return 1
	}
	if {[abs [expr [exprGet $v2.vec<$i>]-[exprGet $v2o.vec<$i>]]] >0.00001 } {
	    return 1
	}
    }   
    vEqToGal $v1 $v2
    vGalToEq $v1 $v2
    loop i 0 [exprGet $v1o.dimen] {
	#echo $i [exprGet $v1.vec<$i>] [exprGet $v2.vec<$i>]
	if {[abs [expr [exprGet $v1.vec<$i>]-[exprGet $v1o.vec<$i>]]] >0.00001 } {
	    return 1
	}
	if {[abs [expr [exprGet $v2.vec<$i>]-[exprGet $v2o.vec<$i>]]] >0.00001 } {
	    return 1
	}
    }
    vEqToSurvey $v1 $v2
    vSurveyToEq $v1 $v2
    loop i 0 [exprGet $v1o.dimen] {
	#echo $i [exprGet $v1.vec<$i>] [exprGet $v2.vec<$i>]
	if {[abs [expr [exprGet $v1.vec<$i>]-[exprGet $v1o.vec<$i>]]] >0.00001 } {
	    return 1
	}
	if {[abs [expr [exprGet $v2.vec<$i>]-[exprGet $v2o.vec<$i>]]] >0.00001 } {
	    return 1
	}
    }
    vectorExprDel $v1
    vectorExprDel $v2
    vectorExprDel $v1o
    vectorExprDel $v2o
    return 0
}

proc tst_cobe { } {
    set list [cobePixToCoord 123 g]
    if {[abs [keylget list longitude]-63.90127182] > 0.01} {
	error "bad g longitude"
    }
    set list [cobePixToCoord 123 g]
    if {[abs [keylget list latitude]-(-10.40113831)] > 0.01} {
	error "bad g longitude"
    }
    set list [cobePixToCoord 123 q]
    if {[abs [keylget list longitude]-307.75900269] > 0.01} {
	error "bad q longitude"
    }
    set list [cobePixToCoord 123 q]
    if {[abs [keylget list latitude]-21.59944344] > 0.01} {
	error "bad q latitude"
    }
    set list [cobePixToCoord 123 e]
    if {[abs [keylget list longitude]-317.15798950] > 0.01} {
	error "bad e longitude"
    }
    set list [cobePixToCoord 123 e]
    if {[abs [keylget list latitude]-39.06019211] > 0.01} {
	error "bad e latitude"
    }
    if {[coordToCobePix 307.75900269  21.59944344 q] != 123} {
	error "bad coord to pix q"
    }
    if {[coordToCobePix 307.75900269  21.59944344 g] != 218759} {
	error "bad coord to pix g"
    }
    if {[coordToCobePix 307.75900269  21.59944344 e] != 324851} {
	error "bad coord to pix e"
    }

    return 0
}

proc tst_vcobe {{beg 100000} {end 101000} } {
    global pg

    set coord g

    set c(pixel) [vectorExprEval $beg,$end]
    set n [exprGet $c(pixel).dimen]
    vNameSet [set c(longitude) [vectorExprNew $n]] longitude
    vNameSet [set c(latitude) [vectorExprNew $n]]  latitude
    vCobePixToCoord $c(pixel) $coord $c(longitude) $c(latitude)
    vPlot $pg $c(longitude) $c(latitude)

    set c(outpix) [vectorExprNew $n]
    vCoordToCobePix $c(longitude) $c(latitude) $coord $c(outpix)

    set c(deltapix) [vectorExprEval $c(outpix)-$c(pixel)]
    vPlot $pg $c(pixel) $c(deltapix)
}

proc tst_surveyGCconversion {} {

# this test of surveyToGC assumes surveyToEq and eqToGC work

  set startMu 23.33432
  set startNu 45.34

  set expectedEq  [ surveyToEq $startMu $startNu ]
  set expectedRA  [ keylget expectedEq ra ]
  set expectedDec [ keylget expectedEq dec ]

  set expectedGC  [ eqToGC $expectedRA $expectedDec ]
  set expectedMu  [ keylget expectedGC mu ]
  set expectedNu  [ keylget expectedGC nu ]

  set newGC [ surveyToGC $startMu $startNu ]
  set newMu [ keylget newGC mu ]
  set newNu [ keylget newGC nu ]

  set deltaMu [ expr abs( $expectedMu - $newMu ) ]
  set deltaNu [ expr abs( $expectedNu - $newNu ) ]

  if { ($deltaNu > 0.0001) || ($deltaMu > 0.0001) } {
    return 1
}

# this test of GCToSurvey assumes GCToEq and eqToSurvey work

  set startMu 34.232
  set startNu  3.1234

  set expectedEq  [ GCToEq $startMu $startNu ]
  set expectedRA  [ keylget expectedEq ra ]
  set expectedDec [ keylget expectedEq dec ]

  set expectedSurvey [ eqToSurvey $expectedRA $expectedDec ]
  set expectedLambda [ keylget expectedSurvey lambda ]
  set expectedEta    [ keylget expectedSurvey eta ]

  set newGC [ GCToSurvey $startMu $startNu ]
  set newLambda [ keylget newGC lambda ]
  set newEta    [ keylget newGC eta ]

  set deltaLambda [ expr abs( $expectedLambda - $newLambda ) ]
  set deltaEta    [ expr abs( $expectedEta - $newEta ) ]
  if { ($deltaLambda > 0.0001) || ($deltaEta > 0.0001) } {
    return 1
  }

  return 0;
}

