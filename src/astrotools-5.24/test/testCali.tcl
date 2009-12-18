proc testCali {} {

    if {[testLups]!=0} {
        echo "Error in testLups"
        return 1
    }
    if {[testCaliInvert]!=0} {
        echo "Error in testCaliInvert"
        return 1
    }
    set list [caliApply 100 10 2187 46 1 1 2 -24 0.1 0.01 0.001 1 -1.65 0.1 -zpAirmass 1.3 -zpColor 1.0]
	echo $list
    if {[abs 18.775-[keylget list calMag]] > 0.001} {
	echo bad calMag
	return 1
    }
    if {[abs 0.1086-[keylget list calMagErr]] > 0.001} {
	echo bad calMagErr
	return 1
    }
    if {[abs 0.0-[keylget list status]] > 0.001} {
	echo bad status
	return 1
    }
    if {[abs 0-[keylget list retInt]] > 0.001} {
	echo bad retInt
	return 1
    }

    set list [lcaliApply 100 10 2187 46 1 1 2 -24 0.1 0.01 0.001 1 -1.65 0.1]
	echo $list
    if {[abs 18.759-[keylget list calMag]] > 0.001} {
	echo bad lcalMag
	return 1
    }
    if {[abs 0.108-[keylget list calMagErr]] > 0.001} {
	echo bad lcalMagErr
	return 1
    }
    if {[abs 0.0-[keylget list status]] > 0.001} {
	echo bad lstatus
	return 1
    }
    if {[abs 0-[keylget list retInt]] > 0.001} {
	echo bad lretInt
	return 1
    }
  return 0

}

proc testLups {} {

  set list [cnts2Lup 400 10]
  set list2 [lup2Cnts [keylget list luptitude] [keylget list error]]

  if {[abs 400-[keylget list2 counts]] > 0.001} {
	return 1
  }
  if {[abs 10-[keylget list2 error]] > 0.001} {
	return 1
  }

  return 0

}

proc testCaliInvert { } {
    
    set  calMag(g.0) 19
    set cCalMag(g.0) 20
    set  calMag(r.0) 20
    set cCalMag(r.0) 21
    set  calMag(i.0) 21
    set cCalMag(i.0) 22
    set  calMag(z.0) 22
    set cCalMag(z.0) 21
    set intTime(0) 55
    set airmass(0) 1.0
    set zeropt(0) -27.0
    set extinct(0) 0.2
    set colorTerm(0) -0.05
    set secColorTerm(0) -0.03

    set counts(g.0) [caliInvert \
	    $calMag(g.0) $cCalMag(g.0) $intTime(0) $airmass(0) $zeropt(0) \
	    $extinct(0) $colorTerm(0) $secColorTerm(0) 1 -zpAirmass 1.3 -zpColor 1.0]
    echo counts(g.0) is $counts(g.0)

    set counts(r.0) [caliInvert \
	    $calMag(r.0) $cCalMag(r.0) $intTime(0) $airmass(0) $zeropt(0) \
	    $extinct(0) $colorTerm(0) $secColorTerm(0) 1 -zpAirmass 1.3 -zpColor 1.0]
    echo counts(r.0) is $counts(r.0)

    set counts(i.0) [caliInvert \
	    $calMag(i.0) $cCalMag(i.0) $intTime(0) $airmass(0) $zeropt(0) \
	    $extinct(0) $colorTerm(0) $secColorTerm(0) 1 -zpAirmass 1.3 -zpColor 1.0]
    echo counts(i.0) is $counts(i.0)

    set counts(z.0) [caliInvert \
	    $calMag(z.0) $cCalMag(z.0) $intTime(0) $airmass(0) $zeropt(0) \
	    $extinct(0) $colorTerm(0) $secColorTerm(0) -1 -zpAirmass 1.3 -zpColor 1.0]
    echo counts(z.0) is $counts(z.0)

    set list(g) [caliApply $counts(g.0) 10 $counts(r.0) 10 \
	    $intTime(0) $intTime(0) $airmass(0) $zeropt(0) $extinct(0) \
	    $colorTerm(0) $secColorTerm(0) 1 1.0 1.0 -zpAirmass 1.3 -zpColor 1.0]
    echo list(g) is $list(g)

    set cm [keylget list(g) calMag]
    set del [expr $calMag(g.0) - $cm]
    if {[abs $del] > 0.00001} {
	error "trouble with g:  calMag(g.0)=$calMag(g.0)  calMag=$cm"
    }
 
    set list(r) [caliApply $counts(r.0) 10 $counts(i.0) 10 \
	    $intTime(0) $intTime(0) $airmass(0) $zeropt(0) $extinct(0) \
	    $colorTerm(0) $secColorTerm(0) 1 1.0 1.0 -zpAirmass 1.3 -zpColor 1.0]
    echo list(r) is $list(r) 
    set cm [keylget list(r) calMag]
    set del [expr $calMag(r.0) - $cm]
    if {[abs $del] > 0.00001} {
	error "trouble with r:  calMag(r.0)=$calMag(r.0)  calMag=$cm"
    }


    set list(i) [caliApply $counts(i.0) 10 $counts(z.0) 10 \
	    $intTime(0) $intTime(0) $airmass(0) $zeropt(0) $extinct(0) \
	    $colorTerm(0) $secColorTerm(0) 1 1.0 1.0 -zpAirmass 1.3 -zpColor 1.0]
    echo list(i) is $list(i) 
    set cm [keylget list(i) calMag]
    set del [expr $calMag(i.0) - $cm]
    if {[abs $del] > 0.00001} {
	error "trouble with i:  calMag(i.0)=$calMag(i.0)  calMag=$cm"
    }

    set list(z) [caliApply $counts(z.0) 10 $counts(i.0) 10 \
	    $intTime(0) $intTime(0) $airmass(0) $zeropt(0) $extinct(0) \
	    $colorTerm(0) $secColorTerm(0) -1 1.0 1.0 -zpAirmass 1.3 -zpColor 1.0]
    echo list(z) is $list(z) 
    set cm [keylget list(z) calMag]
    set del [expr $calMag(z.0) - $cm]
    if {[abs $del] > 0.00001} {
	error "trouble with z:  calMag(z.0)=$calMag(z.0)  calMag=$cm"
    }

    set counts(i.0) [lcaliInvert \
	    $calMag(i.0) $cCalMag(i.0) $intTime(0) $airmass(0) $zeropt(0) \
	    $extinct(0) $colorTerm(0) $secColorTerm(0) 1]
    echo counts(i.0) is $counts(i.0)

    set counts(z.0) [lcaliInvert \
	    $calMag(z.0) $cCalMag(z.0) $intTime(0) $airmass(0) $zeropt(0) \
	    $extinct(0) $colorTerm(0) $secColorTerm(0) -1]
    echo counts(z.0) is $counts(z.0)

    set list(i) [lcaliApply $counts(i.0) 10 $counts(z.0) 10 \
	    $intTime(0) $intTime(0) $airmass(0) $zeropt(0) $extinct(0) \
	    $colorTerm(0) $secColorTerm(0) 1 1.0 1.0]
    echo list(i) is $list(i) 
    set cm [keylget list(i) calMag]
    set del [expr $calMag(i.0) - $cm]
    if {[abs $del] > 0.00001} {
	error "luptitude trouble with i:  calMag(i.0)=$calMag(i.0)  calMag=$cm"
    }

    set list(z) [lcaliApply $counts(z.0) 10 $counts(i.0) 10 \
	    $intTime(0) $intTime(0) $airmass(0) $zeropt(0) $extinct(0) \
	    $colorTerm(0) $secColorTerm(0) -1 1.0 1.0]
    echo list(z) is $list(z) 
    set cm [keylget list(z) calMag]
    set del [expr $calMag(z.0) - $cm]
    if {[abs $del] > 0.00001} {
	error "luptitude trouble with z:  calMag(z.0)=$calMag(z.0)  calMag=$cm"
    }


    return 0
    
}


proc testCaliPlot { } {
    global pg
    set  calMag(g.0) 19
    set cCalMag(g.0) 20
    set  calMag(r.0) 20
    set cCalMag(r.0) 21
    set  calMag(i.0) 21
    set cCalMag(i.0) 22
    set  calMag(z.0) 22
    set cCalMag(z.0) 21
    set intTime(0) 55
    set airmass(0) 1.0
    set zeropt(0) -27.0
    set extinct(0) 0.2
    set colorTerm(0) -0.05
    set secColorTerm(0) -0.03

    pgstateSet $pg -plotTitle \
	    [format "g=%d r=%d t=%d secz=%3.1f zp=%5.1f ext=%3.1f ct=%5.2f sct=%5.2f" $calMag(g.0) $cCalMag(g.0) $intTime(0) $airmass(0) $zeropt(0) $extinct(0) $colorTerm(0) $secColorTerm(0)]
    set lx ""
    set ly ""
    for {set calMag(g.1) 18.0} {$calMag(g.1) < 23.0} \
	    {set calMag(g.1) [expr $calMag(g.1)+0.05]} {
	lappend lx $calMag(g.1)
	lappend ly [caliInvert $calMag(g.1) $calMag(r.0) $intTime(0) \
		$airmass(0) \
		$zeropt(0) $extinct(0) $colorTerm(0) $secColorTerm(0) 1]
    }
    set vx [vFromL $lx "calMag g"]
    set vy [vFromL $ly "counts"]
    pgstateSet $pg -isLine 1 
    vPlot $pg $vx $vy
    vectorExprDel $vx; vectorExprDel $vy

    set lx ""
    set ly ""
    for {set calMag(r.1) 18.0} {$calMag(r.1) < 20.0} \
	    {set calMag(r.1) [expr $calMag(r.1)+0.01]} {
	lappend lx $calMag(r.1)
	lappend ly [caliInvert $calMag(g.0) $calMag(r.1) $intTime(0) \
		$airmass(0) \
		$zeropt(0) $extinct(0) $colorTerm(0) $secColorTerm(0) 1]
    }
    set vx [vFromL $lx "calMag r"]
    set vy [vFromL $ly "counts in g"]
    pgstateSet $pg -isLine 1 
    vPlot $pg $vx $vy
    vectorExprDel $vx; vectorExprDel $vy

    set lx ""
    set ly ""
    for {set intTime(1) 1.0} {$intTime(1) < 100} \
	    {set intTime(1) [expr $intTime(1)+1]} {
	lappend lx $intTime(1)
	lappend ly [caliInvert $calMag(g.0) $calMag(r.0) $intTime(1) \
		$airmass(0) \
		$zeropt(0) $extinct(0) $colorTerm(0) $secColorTerm(0) 1]
    }
    set vx [vFromL $lx "intTime"]
    set vy [vFromL $ly "counts"]
    pgstateSet $pg -isLine 1 
    vPlot $pg $vx $vy
    vectorExprDel $vx; vectorExprDel $vy

    set lx ""
    set ly ""
    for {set airmass(1) 1.0} {$airmass(1) < 15} \
	    {set airmass(1) [expr $airmass(1)+0.05]} {
	lappend lx $airmass(1)
	lappend ly [caliInvert $calMag(g.0) $calMag(r.0) $intTime(0) \
		$airmass(1) \
		$zeropt(0) $extinct(0) $colorTerm(0) $secColorTerm(0) 1]
    }
    set vx [vFromL $lx "airmass"]
    set vy [vFromL $ly "counts"]
    pgstateSet $pg -isLine 1 
    vPlot $pg $vx $vy
    vectorExprDel $vx; vectorExprDel $vy

    set lx ""
    set ly ""
    for {set zeropt(1) -28} {$zeropt(1) < -26} \
	    {set zeropt(1) [expr $zeropt(1)+0.05]} {
	lappend lx $zeropt(1)
	lappend ly [caliInvert $calMag(g.0) $calMag(r.0) $intTime(0) \
		$airmass(0) \
		$zeropt(1) $extinct(0) $colorTerm(0) $secColorTerm(0) 1]
    }
    set vx [vFromL $lx "zeropt"]
    set vy [vFromL $ly "counts"]
    pgstateSet $pg -isLine 1 
    vPlot $pg $vx $vy
    vectorExprDel $vx; vectorExprDel $vy


    set lx ""
    set ly ""
    for {set extinct(1) 0.0} {$extinct(1) < 1.0} \
	    {set extinct(1) [expr $extinct(1)+0.01]} {
	lappend lx $extinct(1)
	lappend ly [caliInvert $calMag(g.0) $calMag(r.0) $intTime(0) \
		$airmass(0) \
		$zeropt(0) $extinct(1) $colorTerm(0) $secColorTerm(0) 1]
    }
    set vx [vFromL $lx "extinction"]
    set vy [vFromL $ly "counts"]
    pgstateSet $pg -isLine 1 
    vPlot $pg $vx $vy
    vectorExprDel $vx; vectorExprDel $vy


    set lx ""
    set ly ""
    for {set colorTerm(1) -0.2} {$colorTerm(1) < 0.2} \
	    {set colorTerm(1) [expr $colorTerm(1)+0.01]} {
	lappend lx $colorTerm(1)
	lappend ly [caliInvert $calMag(g.0) $calMag(r.0) $intTime(0) \
		$airmass(0) \
		$zeropt(0) $extinct(0) $colorTerm(1) $secColorTerm(0) 1]
    }
    set vx [vFromL $lx "colorTerm"]
    set vy [vFromL $ly "counts"]
    pgstateSet $pg -isLine 1 
    vPlot $pg $vx $vy
    vectorExprDel $vx; vectorExprDel $vy

    set lx ""
    set ly ""
    for {set secColorTerm(1) -0.1} {$secColorTerm(1) < 0.1} \
	    {set secColorTerm(1) [expr $secColorTerm(1)+0.005]} {
	lappend lx $secColorTerm(1)
	lappend ly [caliInvert $calMag(g.0) $calMag(r.0) $intTime(0) \
		$airmass(0) \
		$zeropt(0) $extinct(0) $colorTerm(0) $secColorTerm(1) 1]
    }
    set vx [vFromL $lx "secColorTerm"]
    set vy [vFromL $ly "counts"]
    pgstateSet $pg -isLine 1 
    vPlot $pg $vx $vy
    vectorExprDel $vx; vectorExprDel $vy

}
