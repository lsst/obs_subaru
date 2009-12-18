proc testSlalib {} {

    foreach cmd [list \
	    tst_pm tst_vft tst_planetRiseSet tst_tstamps tst_radToDeg \
	    tst_radFromDeg tst_lbToXyz tst_azelpa tst_twilight \
	    tst_coordConv tst_dayFromTime tst_coordPrecess \
	    tst_sepGet tst_vS2tp tst_vTp2s tst_planetRaDec \
	    tst_azEl2HaDec
    ] {
	if {[$cmd] != 0} {
	    error "Error in $cmd in testSlalib"
	    return 1
	}
    }
    return 0
}

proc tst_tstamps {} {
    set tstamp [tstampFromMJD 106631.96]
    set MJD [tstampToMJD $tstamp]
    set error [expr $MJD-106631.96]
    if ([abs $error]>0.00001) {
      echo "Failed in tstamps"
      return 1
    }
    set tstamp1 [tstampNew 2150 10 28 23 2 0]
    set diff [deltaTstamp $tstamp $tstamp1]
    if ([abs [expr $diff-24.]]>0.00001) {
      echo "Failed in tstamps"
      return 1
    }
    tstampIncr $tstamp1 $diff
    if ([string compare "2150/10/28" [tstampToDate $tstamp]]) {
      echo "Failed in tstamps"
      return 1
    }
    if ([string compare "23:02:24.0000" [tstampToTime $tstamp]]) {
      echo "Failed in tstamps"
      return 1
    }
    set LST [tstampToLst $tstamp ]
    tstampIncr $tstamp 240
    set LSTdiff [expr [tstampToLst $tstamp ]-$LST]
    if ([abs [expr $LSTdiff-1]]>0.01) {
      echo "Failed in tstamps"
      return 1
    }
    tstampDel $tstamp
    tstampDel $tstamp1
    return 0
}

proc tst_radToDeg {} {
    set deg [radToDeg 3.1415926535]
    set err [abs [expr $deg-180]]
    if ($err>0.00001) {
      echo "Failed in radToDeg"
      return 1
    }
    return 0
}

proc tst_radFromDeg {} {
    set rad [radFromDeg 180]
    set err [abs [expr $rad-3.1415926535]]
    if ($err>0.00001) {
      echo "Failed in radFromDeg"
      return 1
    }
    return 0
}

proc tst_lbToXyz {} {
    set list [lbToXyz 90. 45.]
    set list2 [xyzToLb [lindex $list 0] [lindex $list 1] [lindex $list 2]]
    set err [abs [expr [lindex $list2 0]-90.]]
    if ($err>0.00001) {
      echo "Failed in lbToXyz"
      return 1
    }
    set err [abs [expr [lindex $list2 1]-45.]]
    if ($err>0.00001) {
      echo "Failed in lbToXyz"
      return 1
    }
    return 0
}

proc tst_azelpa {} {
    set tstamp [tstampNew 1995 3 21 8 25 35.45]
    set list [surveyToAzelpa 12.3 23.4 $tstamp]
    if {[abs [expr  11.63440894-[keylget list az]]] > 0.001} {
	echo list is $list
	return 1
    }
    if {[abs [expr    68.11900944-[keylget list el]]] > 0.001} {
	echo list is $list
	return 1
    }
    if {[abs [expr  -89.30610458-[keylget list pa]]] > 0.001} {
	echo list is $list
	return 1
    }
    tstampDel $tstamp
    return 0
}


proc tst_twilight {} {
    set tstamp [tstampNew 1995 5 9 8 0 0]
    set list [tstampToTwilight $tstamp]
    if {[abs [expr 7.233333-[keylget list duration]]] > 0.01} {
	echo duration is [keylget list duration]
	return 1
    }
    tstampDel $tstamp
    set sunset [keylget list sunset]
    tstampDel $sunset
    set sunrise [keylget list sunrise]
    tstampDel $sunrise

    return 0
}

proc tst_coordPrecess {} {
    # Return 0 if successful; 1 if there is a problem
    set list [coordPrecess 0 0 2000 2000 FK4]
    if ([keylget list ra]!=0) {return 1}
    if ([keylget list dec]!=0) {return 1}
    return 0
}

proc tst_sepGet {} {
   if {[abs [expr [sepGet 0 0 0 35.7]-35.7]]>0.0001} {return 1}
   return 0
}

proc tst_planetRiseSet {} {
    set tstamp [tstampFromMJD 50389.25]
    set np 3
    set times [planetRiseSet $tstamp $np -zd 91.0]
    set rise [keylget times Rise]
    set set1 [keylget times Set]
    set risemjd [tstampToMJD $rise]
    if {[expr abs($risemjd-50389.231)] > 0.005} {
	echo "Error in planetRiseSet"
	return 1
    }
    tstampDel $tstamp
    tstampDel $rise
    tstampDel $set1
    


    return 0
}

proc tst_planetRaDec {} {
    set tstamp [tstampFromMJD 50389.25]
    set np 3
    set radec [planetRaDec $tstamp $np]
    set ra [keylget radec ra]
    set dec [keylget radec dec]
    if { ([expr abs($ra-120.25)] > 0.1) || ([expr abs($dec-15.23)] > 0.1)} {
	echo "Error in planetRaDec"
	return 1
    }
    tstampDel $tstamp
    return 0
}

proc ptAdd {chain row col} {
    set pt [ptNew]
    handleSet $pt.row $row
    handleSet $pt.col $col
    chainElementAddByPos $chain $pt
    handleDel $pt
}

proc slCheck {value expected} {
    set delta [expr $value-$expected]
    if { $delta > .1} {return 1}
    if { -$delta > .1} {return 1}
    return 0
}

proc tst_coordConv {} {
  set DMS [radToDMS -0.798706]
  set rad [radFromDMS $DMS]
  set diff [abs [expr $rad-(-0.798706)]]
  if ($diff>0.0001) {error "Error with tst_coordConv"}
  set HMS [radToHMS -0.798706]
  set rad [radFromHMS $HMS]
  set diff [abs [expr $rad-(-0.798706)-2*3.1415926535]]
  if ($diff>0.0001) {error "Error with tst_coordConv"}
  set DMS [degToDMS -0.798706]
  set deg [DMSToDeg $DMS]
  set diff [abs [expr $deg-(-0.798706)]]
  if ($diff>0.0001) {error "Error with tst_coordConv"}
  set HMS [degToHMS -0.798706]
  set deg [HMSToDeg $HMS]
  set diff [abs [expr $deg-(-0.798706)-360.]]
  if ($diff>0.0001) {error "Error with tst_coordConv"}
  return 0
}

proc tst_dayFromTime {} {
  set day [dayFromTime 12:35:40]
  set diff [abs [expr $day-(0.524769)]]
  if ($diff>0.0001) {error "Error with tst_dayFromTime"}
  return 0
}

proc tst_vS2tp {} {    
    global vx vy
    set vx [vectorExprNew 1]
    handleSet $vx.vec<0> 0.345
    set vy [vectorExprNew 1]
    handleSet $vy.vec<0> -0.087
    
    vS2tp $vx $vy 0.345 -0.087    
    if ([abs [exprGet $vx.vec<0>]]>0.0001) {return 1}
    if ([abs [exprGet $vy.vec<0>]]>0.0001) {return 1}    
    vectorExprDel $vx
    vectorExprDel $vy

    return 0
}

proc tst_vTp2s {} {    
    global vx vy
    set vx [vectorExprNew 1]
    handleSet $vx.vec<0> 0.345
    set vy [vectorExprNew 1]
    handleSet $vy.vec<0> -0.087
    
    vS2tp $vx $vy 0.345 -0.087    
    vTp2s $vx $vy 0.345 -0.087
    if ([abs [exprGet $vx.vec<0>]-0.345]>0.0001) {return 1}
    if ([abs [exprGet $vy.vec<0>]+0.087]>0.0001) {return 1}    
    vectorExprDel $vx
    vectorExprDel $vy
    return 0
}

proc tst_vft { {doplot 0} } {
    global t sunset sunrise pg
    set vx [vFromL "-1.1 1.1" "X (AU)"]
    set vy [vFromL "-1.1 1.1" "Y (AU)"]
    if {$doplot} {
	pgstateSet $pg -symb 1 -just 1 -plotTitle \
		"earth velocity *10^6  observatory velocity *10^8 AU/sec"
	vPlot $pg $vx $vy
	pgPoint 0.0 0.0 4
	pgSfs 2
    }
    set ml "jan feb mar apr may jun jul aug sep oct nov dec"
    set im 0
    foreach m $ml {
	incr im
	set t(t.$m) [tstampNew 1997  $im 1 7 0 0]
    }
    foreach d $ml {
	set l [tstampToTwilight $t(t.$d)]
	set sunset [keylget l sunset]
	set sunrise [keylget l sunrise]
	set t(m.$d) [expr 0.5*([tstampToMJD $sunset]+[tstampToMJD $sunrise])]
	set t(duration.$d) [expr [tstampToMJD $sunrise]-[tstampToMJD $sunset]]
	#echo --------------- $d duration=$t(duration.$d)
	#tstampShow "sunset" $sunset
	#tstampShow "midnight" [tstampFromMJD $t(m.$d)]
	#tstampShow "sunrise" $sunrise
	set t(tm.$d) [tstampFromMJD $t(m.$d)]
	set t(p.$d) [earthPosVel $t(m.$d)]
	foreach p "x y xdot ydot" {set $p [keylget t(p.$d) $p]}
	set x2e [expr $x+1000000*$xdot]
	set y2e [expr $y+1000000*$ydot]


	set t(v.$d) [vectorExprNew 6]
	velFromTstamp $t(tm.$d) $t(v.$d)

	set norm [sqrt $x2e*$x2e+$y2e*$y2e]
	set xobs [expr $x + 0.2*$x2e/$norm]
	set yobs [expr $y + 0.2*$y2e/$norm]


	set x2o [expr $xobs+100000000*[exprGet $t(v.$d).vec<3>]]
	set y2o [expr $yobs+100000000*[exprGet $t(v.$d).vec<4>]]

	if {$doplot} {
	    pgPtext [expr 0.6*$x] [expr 0.6*$y] 0.0 0.5 $d
	    pgCirc $x $y 0.2
	    pgArrow $x $y $x2e $y2e
	    pgPoint $xobs $yobs 3
	    pgArrow $xobs $yobs $x2o $y2o
	}	    
	tstampDel $sunset
	tstampDel $sunrise
    }
    
    # see if the values in t(v.dec) are what they were in October, 1997
    set answers "1.37501e-05 3.3159e-05 2.29613e-05 -2.41799e-09 1.00267e-09 0"
    loop i 0 6 {
	set calc [exprGet $t(v.dec).vec<$i>]
	set answ [lindex $answers $i]
	if {$answ == 0} {
	    set diff [expr $calc-$answ]
	} else {
	    set diff [expr ($calc-$answ)/$answ]
	}
	if {[abs $diff] > 0.01} {
	    error "tst_vft:  Trouble with i=$i calc=$calc answ=$answ"
	}

    }
    foreach m $ml {tstampDel $t(t.$m)}
    foreach m $ml {tstampDel $t(tm.$m)}
    foreach m $ml {vectorExprDel $t(v.$m)}
    vectorExprDel $vx
    vectorExprDel $vy

    return 0
}

proc tstampShow {m t} {
    echo [format "%20s %s %s" $m [tstampToDate $t] [tstampToTime $t]]
    return
}

proc tst_pm { } {
    global mjd1950 mjd2000 list
    set ts1950 [tstampNew 1950 1 1 0 0 0]
    set mjd1950 [tstampToMJD $ts1950]
    set ts2000 [tstampNew 2000 1 1 0 0 0]
    set mjd2000 [tstampToMJD $ts2000]

    set list [pmApply 10 20 0.01 0.02 $mjd1950 $mjd2000]
    if {[abs [keylget list ra]-10.503169] > 0.00001} {
	error "bad ra in tst_pm"
    }
    if {[abs [keylget list dec]-20.999131] > 0.00001} {
	error "bad dec in tst_pm"
    }
    tstampDel $ts1950
    tstampDel $ts2000
    return 0
}

proc tst_azEl2HaDec {} {
    global at_site_latitude
    set list [atAzEl2HaDec 180 0 $at_site_latitude]
    if {[abs [expr  0.-[keylget list ha]]] > 0.00001} {
	echo list a is $list
	return 1
    }
    if {[abs [expr  $at_site_latitude-90.-[keylget list dec]]] > 0.00001} {
	echo list b is $list
	return 1
    }
    set list [atAzEl2HaDec 0 0 $at_site_latitude]
    if {[abs [expr  -180.-[keylget list ha]]] > 0.00001} {
	echo list c is $list
	return 1
    }
    if {[abs [expr  90.-$at_site_latitude-[keylget list dec]]] > 0.00001} {
	echo list d is $list
	return 1
    }
    set list [atAzEl2HaDec 180 [expr 100.-$at_site_latitude] $at_site_latitude]
    if {[abs [expr  0.-[keylget list ha]]] > 0.00001} {
	echo list e is $list
	return 1
    }
    if {[abs [expr  10.-[keylget list dec]]] > 0.00001} {
	echo list f is $list
	return 1
    }

#    set list [atAzEl2HaDec 165 $at_site_latitude $at_site_latitude]
#    if {[abs [expr  -61.568390-[keylget list ha]]] > 0.00001} {
#	echo list g is $list
#	return 1
#    }

#    if {[abs [expr  0.888717-[keylget list dec]]] > 0.00001} {
#	echo list h is $list
#	return 1
#    }

    return 0
}
