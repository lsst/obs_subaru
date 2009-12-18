proc testImSim {} {
global r
  set r [regNew 1024 1024 -type FL32]
  if { [tst_expDisk] != 0 } {
    error "Error with tst_expDisk"
  }
  if { [tst_DeVauc] != 0 } {
    error "Error with tst_DeVauc"
  }
  if { [tst_wingAdd] != 0} {
    error "Error with tst_wingAdd"
  }
  if { [tst_deltaAdd] != 0} {
    error "Error with tst_deltaAdd"
  }
  regDel $r
  return 0

}

proc tst_deltaAdd {} {
global r
	echo deltaAdd
	regSetWithDbl $r 0
	deltaAdd $r 511.5 511.5 8000 
	set peak [regPixGet $r 511 511]
	echo peak is $peak vs predicted 8000
	if { $peak < 7999 } {return 1}
	set outer [regPixGet $r 511 521]
	echo outer is $outer vs predicted 0
	if { $outer > 1 } {return 1}
	return 0
}

proc tst_wingAdd {} {
global r
	echo wingAdd
	regSetWithDbl $r 0
	wingAdd $r 511.5 511.5 8000 -2 0 1 5 -fast
	set peak [regPixGet $r 511 511]
	echo peak is $peak vs predicted 0
	if { $peak > 1 } {return 1}
	set outer [regPixGet $r 511 521]
	echo outer is $outer vs predicted 2.666
	if { $outer <2 || $outer > 3} { return 1}
	return 0
}

proc tst_DeVauc {} {
global r
	echo DeVauc
	regSetWithDbl $r 0
	deVaucAdd $r 511.5 511.5 8000 115.0032 0 1 -peak
	set peak [regPixGet $r 511 511]
	echo predicted vs peak: 8000 $peak
	regSetWithDbl $r 0
	deVaucAdd $r 511.5 511.5 8000 0.0016 0 1 -fast
	set peak [regPixGet $r 511 511]
	set out [regStatsFind $r]
	set counts [expr 1024*1024*[lindex [lindex $out 0] 1]]
	echo predicted vs counts: 8000 $counts
    return 0
}

proc tst_expDisk {} {
global r
	echo ExpDisk
	regSetWithDbl $r 0
	expDiskAdd $r 511.5 511.5 8000 64 0 1 -peak
	set peak [regPixGet $r 511 511]
	echo predicted vs peak: 8000 $peak
	regSetWithDbl $r 0
	expDiskAdd $r 511.5 511.5 8000 64 0 1  -fast
	set out [regStatsFind $r]
	set counts [expr 1024*1024*[lindex [lindex $out 0] 1]]
	echo predicted vs counts: 8000 $counts
    return 0
}

proc tdvs { } {
    global tdv
    set size 600
    set loc  300
    foreach i "0 1" {
	set tdv(reg.$i) [regNew $size $size -type FL32] 
	regSetWithDbl $tdv(reg.$i) 0.0
    }
    set row $loc
    set col $loc
    set counts 1.0
    set rs 1.0
    set maxRad [expr 10.0*$rs]
    set theta 10.0
    set axisRatio 0.5
    set ngrid 3
    set co [deVaucAddSlow $tdv(reg.0) $loc $loc $counts $rs $maxRad $theta \
	    $axisRatio $ngrid]
    echo co=$co
    return
}

proc dvStudy { } {
    global dvs
    set size 1024
    set loc 512
    set dvs(reg) [regNew $size $size -type FL32]
    set row $loc
    set col $loc
    set counts 1.0
    set rs 1.0
    set theta 0.0
    set axisRatio 1.0
    set ngrid 4

    set dvs(l.rs) ""
    set dvs(l.maxRad) ""
    set dvs(l.co) ""

    foreach rs "1 2 3 4" {
	set maxRad [expr 20.0*$rs]
	
	foreach axisRatio "0.25 0.5 0.75 1.0" {
	    regSetWithDbl $dvs(reg) 0.0
	    set norm [expr [sqrt $axisRatio]/($rs*$rs)]
	    set co [deVaucAddSlow $dvs(reg) $loc $loc [expr $norm*$counts] \
		    $rs $maxRad $theta $axisRatio $ngrid]
	    lappend dvs(l.co) $co
	    lappend dvs(l.ar) $axisRatio
	    lappend dvs(l.rs) $rs
	}
    }
    set dvs(v.co) [vFromL $dvs(l.co) counts]
    set dvs(v.ar) [vFromL $dvs(l.ar) axisRatio]
    set dvs(v.rs) [vFromL $dvs(l.rs) Rs]
}


proc dvPlot { } {
    global dvs pg
    pgstateSet $pg -symb 1 -isNewplot 1
    vPlot $pg $dvs(v.ar) $dvs(v.co)
    
    
}

proc devaucTime { } {
    global pg vmult vco lap
    set treg [regNew 2048 2048 -type FL32]
    set row 500; set col 500; set rs 2; set theta 0.0; set ar 1.0; set ngrid 4
    set lco ""
    set lMult ""
    timerStart
    for {set rsMult 0.5} {$rsMult < 20} {set rsMult [expr $rsMult+0.5]} {
	set maxRad [expr $rsMult*$rs]
	set counts [expr $ar/(0.0106*$rs*$rs)]
	loop i 0 3 {
	    set co [deVaucAddSlow $treg $row $col $counts $rs $maxRad \
		    $theta $ar $ngrid]
	}
	echo rsMult $co
	lappend lco $co
	lappend lMult $rsMult
    }
    set lap [timerLap]
    set vco [vFromL $lco counts]
    set vmult [vFromL $lMult "extent in Rs"]
    pgstateSet $pg -symb 3
    vPlot $pg $vmult $vco
    
}



proc tstDeVauc { {ar 0.7} } {
    global ted
    loop theta -1 10 {
	set ted(reg.$theta) [regNew 1000 1000 -type FL32]
	regSetWithDbl $ted(reg.$theta) 0.0
	set co [deVaucAddSlow $ted(reg.$theta) \
		500 500 1 2.0 10 $theta $ar 1]
	echo theta=$theta co=$co
    }
}

proc tstDeVauc { {ar 0.7} {theta 0.0} } {
    global ted
    foreach pf ".1 .2 .3 .4 .5" {
	set ted(reg.$theta) [regNew 1000 1000 -type FL32]
	regSetWithDbl $ted(reg.$theta) 0.0
	set co [deVaucAddSlow $ted(reg.$theta) \
		[expr 500+$pf] 500 1 2.0 10 $theta $ar 1]
	echo pf=$pf co=$co
    }
}

proc tstExpDisk { {ar 0.7} } {
    global ted
    loop theta -1 10 {
	set ted(reg.$theta) [regNew 1000 1000 -type FL32]
	regSetWithDbl $ted(reg.$theta) 0.0
	set co [expDiskAddSlow $ted(reg.$theta) \
		500 500 1 2.0 10 $theta $ar 1]
	echo theta=$theta co=$co
    }
}
